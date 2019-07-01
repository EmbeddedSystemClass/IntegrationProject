/** @file HL_sys_main.c 
*   @brief Application main file
*   @date 11-Dec-2018
*   @version 04.07.01
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* 
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com  
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "HL_sys_common.h"

/* USER CODE BEGIN (1) */
/* Include FreeRTOS scheduler files */
#include "FreeRTOS.h"
#include "os_task.h"
#include "os_semphr.h"

/* Include HET header file - types, definitions and function declarations for system driver */
#include "HL_system.h"
#include "HL_het.h"
#include "HL_esm.h"

#include "mpu9250.h"

#include "HL_gio.h"
#include "HL_sci.h"
#include "HL_eqep.h"
#include "HL_etpwm.h"
#include <stdio.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "ipv4/lwip/ip_addr.h"
#include "lwip/udp.h"

#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

void vTask1(void *pvParameters);
void pidTask(void *pvParameters);
void pwmTask(void *pvParameters);
void udpTask(void *pvParameters);
void sciTask(void *pvParameters);
void imuTask(void *pvParameters);
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName );
extern void EMAC_LwIP_Main (uint8_t * emacAddress);

void udp_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port);
void const_velocity(int preCNT, int setCNT, int *error, float c_time);
void fl2in(float a, int *b);

void set_Status(uint8 bit); /* bit0 : Charge / bit1 : 각도 */
void reset_Status(uint8 bit);

void convert(unsigned char *ch, unsigned char *fl); /* ASCII to float */

#define VOLTAGE     0
#define DEGREE      1
#define MCU_READY   2
#define MCU_GO      3

/* Define Task Handles */
xTaskHandle xTask1Handle;
xTaskHandle xTask2Handle;
xTaskHandle xTask3Handle;
xTaskHandle xTask4Handle;
xTaskHandle xTask5Handle;
xTaskHandle xTask6Handle;
/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
#define SCI_DEBUG   1 // If this value set 1, sciREG1 prints Debug messages.
#define I2C_DEBUG   0 // If this value set 1, I2C_2 & MPU will work
#define IMU_DEBUG   1 // If this value set 1, AHRSv1 will work

#if SCI_DEBUG
uint8_t sciTest[] = {"SCI very well\r\n"};
uint8 length = sizeof(sciTest);
uint8_t txtCRLF[] = {'\r', '\n'};

uint8_t txtlwIP[] = {"lwIP Initializing......\n\r"};
uint8_t txtOK[] = {"Success!!!\n\r"};
#endif

/* for Velocity */
char vbuf[32] = {0};
unsigned int vbuflen = 0;
/* for CMPA */
char cbuf[32] = {0};
unsigned int cbuflen = 0;

/*********************************************************** For PID ***********************************************************************/
int pwm_CMPA;
char buf[32] = {0};
unsigned int buflen = 0;

/* eQEP, PID 변수 */
uint32 pcnt;    // 엔코더 cnt값
float velocity; // 엔코더가 측정한 속도값
float ppr = 3000; // 엔코더 ppr
// ppr = 3000, 감속비 24, Quadrature 모드
// 1 res의 pcnt = 3000 * 24 * 4
// 1 res의 pcnt = 288000
int setCNT; // 20rpm으로 동작시키기 위한 10ms당 cnt값
int error[2] = {0,0};

// Count 초기화 주기 10ms
float Unit_freq = VCLK3_FREQ * 10000.0;
// eqepSetUnitPeriod(eqepREG1, Unit_freq);

// 엔코더 1ch 펄스당 degree
float ppd = 0.00125; //   360 / (ppr * 24 * 4)
// 제어주기
float c_time = 0.01; //   Unit_freq / (VCLK3_FREQ * 1000000.0)

#define Kp  2.63;
#define Ki  8.4;
#define Kd  0.00015;

float ierr;
float derr;

float P_term;
float I_term;
float D_term;
/*******************************************************************************************************************************************/

/*********************************************************** For UDP ***********************************************************************/
struct udp_pcb *pcb;

char udp_msg[3] = {0};
uint8_t err_bind[] = {"bind error!!!\r\n"};
struct pbuf *p;

/* 표시할 항목
     * 충전상태 ( BIT 0 )
     *          0: 충전중
     *          1: 완충(700V)
     * 각도 ( BIT 1 )
     *          0: 조준 중
     *          1: 조준 완료
     * READY( BIT 2 )
     *          0: Task don't start
     *          1: Task Start!
     *
 */
uint8 status_flag; /* 1 : Charge / 2 : 각도  / 3 : All Ready / 4 : Go */
/*******************************************************************************************************************************************/

/*********************************************************** For AHRS **********************************************************************/
// 플래쉬 저장
uint8_t fw_save[] = {0x02,0x09,0x02,0x18,0x07,0x00,0x00,0x01,0x00,0x00,0x00,0x22,0x03};
// 오일러 각 중 roll 데이터 주셈
uint8_t send_roll[] = {0x02,0x09,0x02,0x3c,0x35,0x00,0x01,0x00,0x00,0x00,0x00,0x74,0x03};
// rs232 동기화 데이터 보내지 마라
uint8_t period_stop[] = {0x02,0x09,0x02,0x18,0x15,0x00,0x00,0x00,0x00,0x00,0x00,0x2f,0x03};
uint8_t rx[13] = {0};

int setDGR; // DSP에서 준 각도 값 ( 100배 뻥튀기 해서 옴 )
float setRoll; // (float)setDGR / 100.0
float roll; // IMU 측정값
/*******************************************************************************************************************************************/

/* USER CODE END */

uint8	emacAddress[6U] = 	{0x00U, 0x08U, 0xeeU, 0x03U, 0xa6U, 0x6cU};
uint32 	emacPhyAddress	=	1U;

int main(void)
{
/* USER CODE BEGIN (3) */
    /*clear the ESM error manually*/
    esmREG->SR1[2] = 0xFFFFFFFFU;
    esmREG->SSR2   = 0xFFFFFFFF;
    esmREG->EKR = 0x0000000A;
    esmREG->EKR = 0x00000000;

    /* clear MCU status */
    status_flag = 0x00;

    sciInit();
    i2cInit();
    //VCLK3_FREQ; // 37.500F
    etpwmInit();
    QEPInit();

    etpwmStartTBCLK();
    eqepEnableCounter(eqepREG1);
    eqepEnableUnitTimer(eqepREG1);

    /* Set high end timer GIO port pin direction to all output */
    gioInit();
    gioSetDirection(gioPORTA, 0xFFFF);
    gioSetDirection(gioPORTB, 0xFFFF);

#if SCI_DEBUG
    sciSend(sciREG1, sizeof(txtlwIP), txtlwIP);
#endif
    EMAC_LwIP_Main(emacAddress);
#if SCI_DEBUG
    sciSend(sciREG1, sizeof(txtOK), txtOK);
#endif

#if 1
    pcb = udp_new();
#if 0
    udp_bind(pcb, IP_ADDR_ANY, 7777);
#else
    while(udp_bind(pcb, IP_ADDR_ANY, 7777))
    {
        wait(1000000);
#if SCI_DEBUG
        sciSend(sciREG1, sizeof(err_bind), err_bind);
#endif
    }
#endif
    udp_recv(pcb, udp_echo_recv, NULL);
#endif
    wait(100000);
    set_Status(MCU_READY); // MCU Initialize finish.

    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(udp_msg), PBUF_RAM);
    udp_msg[0] ='s';
    udp_msg[1] = status_flag;
    udp_msg[2] = 0;
    memcpy(p->payload,udp_msg, sizeof(udp_msg));
    udp_sendto(pcb, p, IP_ADDR_BROADCAST, 7777);

    pbuf_free(p);

    while(!(status_flag & 0x08))
    {
        printf("Waiting for DSP....\n");
        wait(10000);
    }
    // ((configMAX_PRIORITIES-1)|portPRIVILEGE_BIT)
    /* Create Task 1 */
    //    if (xTaskCreate(vTask1,"Task1", configMINIMAL_STACK_SIZE, NULL, 1, &xTask1Handle) != pdTRUE)
#if 1
    if (xTaskCreate(vTask1,"Task1", configMINIMAL_STACK_SIZE, NULL, 1, &xTask1Handle) != pdTRUE)
    {
        /* Task could not be created */
        while(1);
    }
#endif
    /* Create Task 2 */
#if 1
    if (xTaskCreate(pidTask,"PID", configMINIMAL_STACK_SIZE, NULL, 6, &xTask2Handle) != pdTRUE)
    {
        /* Task could not be created */
        while(1);
    }
#endif
    /* Create Task 3 */
#if 0
    if(xTaskCreate(pwmTask, "PWM", configMINIMAL_STACK_SIZE, NULL, 2, &xTask3Handle) != pdTRUE)
    {
        while(1);
    }
#endif
    /* Create Task 4 */
#if SCI_DEBUG
    if(xTaskCreate(sciTask, "SCI", 2 * configMINIMAL_STACK_SIZE, NULL, 7, &xTask5Handle) != pdTRUE)
    {
        while(1);
    }
#endif
    /* Create Task 5 */
#if 1
    if(xTaskCreate(udpTask, "UDP", 4 * configMINIMAL_STACK_SIZE, NULL, 8, &xTask4Handle) != pdTRUE)
    {
        while(1);
    }
#endif
    /* Create Task 6 */
#if IMU_DEBUG
    if(xTaskCreate(imuTask, "IMU", configMINIMAL_STACK_SIZE, NULL, 5, &xTask6Handle) != pdTRUE)
    {
        while(1);
    }
#endif
    /* Start Scheduler */
    vTaskStartScheduler();

    /* Run forever */
    while(1);
/* USER CODE END */

    return 0;
}


/* USER CODE BEGIN (4) */

/* Task1 */
void vTask1(void *pvParameters)
{
#if 1
    for(;;)
    {
        taskENTER_CRITICAL();
            /* Taggle GIOB[6] with timer tick */
            gioSetBit(gioPORTB, 6, gioGetBit(gioPORTB, 6) ^ 1);
        taskEXIT_CRITICAL();

        vTaskDelay(300);
    }
#endif
}

void pidTask(void *pvParameters)
{
    /*
     * If you want to know position CNT value,
     * Use this Function 'eqepReadPosnCount(eqepREG1)'
     * but value clear every 10ms later.
     */
    for(;;)
    {
        taskENTER_CRITICAL();
        // 10ms마다 플래그 set
        if((eqepREG1->QFLG & 0x800) == 0x800)
        {
            pcnt = eqepReadPosnLatch(eqepREG1); // 정해놓은 시간동안 들어온 CNT 갯수
            velocity = ((float)pcnt * ppd / c_time) / 6.0; // rpm
            const_velocity(pcnt, setCNT, error, c_time);
            // Flag가 자동 초기화가 안됌.
            eqepClearInterruptFlag(eqepREG1, QEINT_Uto);
        }
        taskEXIT_CRITICAL();
        vTaskDelay(10);
    }
}

#if 0
void pwmTask(void *pvParameters)
{
    for(;;)
    {
        taskENTER_CRITICAL();
        if(pwm_CMPA < 3750)
        {
            etpwmREG1->CMPA = 10 * pwm_CMPA++;
        }
        else
            pwm_CMPA = 0;
        taskEXIT_CRITICAL();
        vTaskDelay(10);
    }
}
#endif

#if SCI_DEBUG
void sciTask(void *pvParameters)
{
    /* SCI Baud Rate : 230400
     * sciREG1
     */
#if 1
    sprintf(buf, "\033[H\033[JSCI_Baud_Rate = 230400\n\r\0");
    buflen = strlen(buf);
    sciSend(sciREG1, buflen, (uint8 *)buf);
    sciSend(sciREG1, sizeof(txtCRLF), txtCRLF);
    sciSend(sciREG1, length, sciTest);
#endif
    for(;;)
    {
        taskENTER_CRITICAL();
#if 0
        sprintf(buf, "CMPA = %d\t setCNT = %d\n\r\0", pwm_CMPA, setCNT);
        buflen = strlen(buf);
        sciSend(sciREG1, buflen, (uint8 *)buf);

        sprintf(buf, "Velocity = %d\n\r\0", (int)velocity);
        buflen = strlen(buf);
        sciSend(sciREG1, buflen, (uint8 *)buf);
#endif

#if 0
        printf("CMPA = %d\n", pwm_CMPA);
        printf("Velocity = %.2f\n", velocity);
#endif

#if SCI_DEBUG & I2C_DEBUG

        sprintf(buf, "roll = %d.%d \t pitch = %d.%d \t yaw = %d.%d \n\r\0",
                apa[0],apa[1], ara[0],ara[1], yd[0],yd[1]);
        buflen = strlen(buf);
        sciSend(sciREG1, buflen, (uint8 *) buf);
#endif

#if SCI_DEBUG
        vbuflen = strlen(vbuf);
        sciSend(sciREG1, vbuflen, (uint8 *) vbuf);
#endif
        taskEXIT_CRITICAL();
        vTaskDelay(500);
    }
}
#endif

void udp_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
    if (p != NULL)
    {
        char *rx_pk = p->payload;
        switch(rx_pk[0])
        {
            case 's' :
#if 1
                setCNT = rx_pk[1] << 24U |
                         rx_pk[2] << 16U |
                         rx_pk[3] << 8U  |
                         rx_pk[4];
#endif
                setDGR = rx_pk[5] << 24U |
                         rx_pk[6] << 16U |
                         rx_pk[7] << 8U  |
                         rx_pk[8];
                setRoll = (float)setDGR / 100.0;
#if SCI_DEBUG
            sprintf(vbuf,"%d,%d\n\r",setCNT,setDGR);
#endif
                break;
                /* MCU가 준비되서 Ready signal을 전송하면 DSP에서 받고 준비되면 'g'를 보내서 MCU 전체 테스크 동작 시작. */
            case 'g' :
#if 1
                set_Status(MCU_GO);
                break;
#endif
            case 't' :
                /* DSP의 Trigger Signal. 레일건 or 레이저 발사. (gio A? B? n bit SET!) */
                break;

            default:
                /* 다른값이 날라오면 에러  or 재요청 (미구현) */
                break;
        }
        pbuf_free(p);
    }
}

void udpTask(void *pvParameters)
{
#if 0
    struct udp_pcb *pcb;

    char msg[] = "udp test\r\n";
    struct pbuf *p;
    err_t err;

    pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, 7777);
    udp_recv(pcb, udp_echo_recv, NULL);
#endif
    for(;;)
    {
        taskENTER_CRITICAL();
#if 0
        p = pbuf_alloc(PBUF_TRANSPORT, sizeof(msg), PBUF_RAM);
        memcpy(p->payload, msg, sizeof(msg));
        udp_sendto(pcb, p, IP_ADDR_BROADCAST, 7777);
#else
        p = pbuf_alloc(PBUF_TRANSPORT, sizeof(udp_msg), PBUF_RAM);

        udp_msg[0] ='s';
        udp_msg[1] = status_flag;
        udp_msg[2] = 0;

        memcpy(p->payload, udp_msg, sizeof(udp_msg));
        udp_sendto(pcb, p, IP_ADDR_BROADCAST, 7777);
#endif
        pbuf_free(p);

        taskEXIT_CRITICAL();

        vTaskDelay(10);
    }
}

void const_velocity(int preCNT, int setCNT, int *error, float c_time)
{
    error[0] = setCNT - preCNT;
    ierr += (float)error[0] * c_time;
    derr = (float)(error[0] - error[1]) / c_time;
    P_term = (float)error[0] * Kp;
    I_term = ierr * Ki;
    D_term = derr * Kd;

    error[1] = error[0];

    pwm_CMPA = (int)(P_term + I_term + D_term);

    if(pwm_CMPA < 0)
    {
        pwm_CMPA = -pwm_CMPA;
    }
    if(pwm_CMPA > 37500)
    {
        pwm_CMPA = 37500;
    }
    etpwmREG1->CMPA = pwm_CMPA;
}

void imuTask(void *pvParameters)
{
    for (;;)
    {
        taskENTER_CRITICAL();

        taskEXIT_CRITICAL();
        vTaskDelay(10);
    }
}

void set_Status(uint8 bit)
{
    status_flag |= 0x1U << bit;
}

void reset_Status(uint8 bit)
{
    status_flag  = status_flag &~(0x1U << bit);
}

void convert(unsigned char *arr, unsigned char *num)
{
    int i;
    for(i = 0; i < 4; i++)
        num[i]  = arr[10 - i];
}

/* USER CODE END */
