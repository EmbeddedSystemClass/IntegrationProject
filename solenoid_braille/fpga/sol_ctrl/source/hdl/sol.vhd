----------------------------------------------------
--����ϴ� Duty�� �Է��ϸ� UART�� �޾Ƽ� PULSE �ð����� ����
----------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.std_logic_unsigned.all;

entity sol is
port(
		sol_clk			:	in	std_logic; --PLL�� CLK
		sol_reset		:	in	std_logic; --PLL�� RESET
		sol_in			:	in	std_logic_vector(31 downto 0); --UART���� �Է¹޾� sdk���� �� WDARA�� ����� RPM
		sol_out			:	out	std_logic_vector(35 downto 0);  --PULSE		
		sol_wren		:	in	std_logic
		);
end sol;

architecture Behavioral of sol is
	signal	PULSE_TEM	:std_logic_vector(31 downto 0)	:=(others=>'0'); --PULSe ���� ����
	signal	COUNT_CLK	:std_logic_vector(31 downto 0)	:=(others=>'0'); --CLK���� ���
	constant BASICHZ	:std_logic_vector(31 downto 0)	:= X"0000_2710"; --5KHz�� ���� X"0007_A120"; 100HZ
	
	attribute keep : string;	
	attribute keep of PULSE_TEM			: signal is "true";

begin

	rpm_sped_proc : process(sol_clk)
	begin
		if (sol_reset='0') then
			PULSE_TEM	<=	(others=>'0');
			COUNT_CLK	<=	(others=>'0');
		elsif rising_edge(fan_clk) then
			if (sol_wren='1') then					--pwm�Է� ���� ������
				PULSE_TEM	<=	(others=>'0');
				COUNT_CLK	<=	(others=>'0');
			else
				COUNT_CLK	<=	COUNT_CLK + '1';
				if (COUNT_CLK>=sol_in) then
					PULSE_TEM	<=	(others=>'0');
					if(COUNT_CLK= BASICHZ) then
						COUNT_CLK	<=	(others=>'0');

					end if;
				elsif(COUNT_CLK< sol_in) then
					PULSE_TEM 	<=	(others=>'1');				
				end if;			
			end if;
			sol_out(0)	<=	 not PULSE_TEM(0); --pulse���� �� out���� �ѱ�
			sol_out(1)	<=	 not PULSE_TEM(1); --pulse���� �� out���� �ѱ�
			sol_out(2)	<=	 not PULSE_TEM(2); --pulse���� �� out���� �ѱ�
		end if;
	end process;	

end Behavioral;
