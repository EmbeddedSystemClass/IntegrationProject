-- Copyright 1986-2017 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2017.4 (lin64) Build 2086221 Fri Dec 15 20:54:30 MST 2017
-- Date        : Thu Jun 27 19:57:34 2019
-- Host        : sdr-Samsung-DeskTop-System running 64-bit Ubuntu 16.04.3 LTS
-- Command     : write_vhdl -force -mode synth_stub
--               /home/sdr/project/IntegrationProject/mercenary/fpga_sine_wave/fpga_sine_wave.srcs/sources_1/bd/fpga_sine_wave/ip/fpga_sine_wave_rst_ps7_0_50M_0/fpga_sine_wave_rst_ps7_0_50M_0_stub.vhdl
-- Design      : fpga_sine_wave_rst_ps7_0_50M_0
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z010clg400-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity fpga_sine_wave_rst_ps7_0_50M_0 is
  Port ( 
    slowest_sync_clk : in STD_LOGIC;
    ext_reset_in : in STD_LOGIC;
    aux_reset_in : in STD_LOGIC;
    mb_debug_sys_rst : in STD_LOGIC;
    dcm_locked : in STD_LOGIC;
    mb_reset : out STD_LOGIC;
    bus_struct_reset : out STD_LOGIC_VECTOR ( 0 to 0 );
    peripheral_reset : out STD_LOGIC_VECTOR ( 0 to 0 );
    interconnect_aresetn : out STD_LOGIC_VECTOR ( 0 to 0 );
    peripheral_aresetn : out STD_LOGIC_VECTOR ( 0 to 0 )
  );

end fpga_sine_wave_rst_ps7_0_50M_0;

architecture stub of fpga_sine_wave_rst_ps7_0_50M_0 is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "slowest_sync_clk,ext_reset_in,aux_reset_in,mb_debug_sys_rst,dcm_locked,mb_reset,bus_struct_reset[0:0],peripheral_reset[0:0],interconnect_aresetn[0:0],peripheral_aresetn[0:0]";
attribute x_core_info : string;
attribute x_core_info of stub : architecture is "proc_sys_reset,Vivado 2017.4";
begin
end;
