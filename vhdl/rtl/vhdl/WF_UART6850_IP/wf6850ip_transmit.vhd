----------------------------------------------------------------------
----                                                              ----
---- 6850 compatible IP Core    					              ----
----                                                              ----
---- This file is part of the SUSKA ATARI clone project.          ----
---- http://www.experiment-s.de                                   ----
----                                                              ----
---- Description:                                                 ----
---- UART 6850 compatible IP core                                 ----
----                                                              ----
---- 6850's transmitter unit.                                     ----
----                                                              ----
----                                                              ----
---- To Do:                                                       ----
---- -                                                            ----
----                                                              ----
---- Author(s):                                                   ----
---- - Wolfgang Foerster, wf@experiment-s.de; wf@inventronik.de   ----
----                                                              ----
----------------------------------------------------------------------
----                                                              ----
---- Copyright (C) 2006 - 2008 Wolfgang Foerster                  ----
----                                                              ----
---- This source file may be used and distributed without         ----
---- restriction provided that this copyright statement is not    ----
---- removed from the file and that any derivative work contains  ----
---- the original copyright notice and the associated disclaimer. ----
----                                                              ----
---- This source file is free software; you can redistribute it   ----
---- and/or modify it under the terms of the GNU Lesser General   ----
---- Public License as published by the Free Software Foundation; ----
---- either version 2.1 of the License, or (at your option) any   ----
---- later version.                                               ----
----                                                              ----
---- This source is distributed in the hope that it will be       ----
---- useful, but WITHOUT ANY WARRANTY; without even the implied   ----
---- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      ----
---- PURPOSE. See the GNU Lesser General Public License for more  ----
---- details.                                                     ----
----                                                              ----
---- You should have received a copy of the GNU Lesser General    ----
---- Public License along with this source; if not, download it   ----
---- from http://www.gnu.org/licenses/lgpl.html                   ----
----                                                              ----
----------------------------------------------------------------------
-- 
-- Revision History
-- 
-- Revision 2K6A  2006/06/03 WF
--   Initial Release.
-- Revision 2K6B  2006/11/07 WF
--   Modified Source to compile with the Xilinx ISE.
-- Revision 2K8A  2008/07/14 WF
--   Minor changes.
-- Revision 2K8B  2008/11/01 WF
--   Fixed the T_DRE process concerning the TDRE <= '1' setting.
--   Thanks to Lyndon Amsdon finding the bug.
--

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity WF6850IP_TRANSMIT is
  port (
		CLK					: in std_logic;
        RESETn				: in std_logic;
		MCLR				: in std_logic;

        CS					: in std_logic_vector(2 downto 0);
        E		       		: in std_logic;   
        RWn              	: in std_logic;
        RS					: in std_logic;

        DATA_IN		        : in std_logic_vector(7 downto 0);   

		CTSn				: in std_logic;

		TC					: in std_logic_vector(1 downto 0);
		WS					: in std_logic_vector(2 downto 0);
		CDS					: in std_logic_vector(1 downto 0);

        TXCLK				: in std_logic;

		TDRE				: buffer std_logic;
        TXDATA				: out std_logic
       );                                              
end entity WF6850IP_TRANSMIT;

architecture BEHAVIOR of WF6850IP_TRANSMIT is
	type TR_STATES is (IDLE, LOAD_SHFT, START, SHIFTOUT, PARITY, STOP1, STOP2);
	signal TR_STATE, TR_NEXT_STATE	: TR_STATES;
	signal CLK_STRB		: std_logic;
	signal DATA_REG		: std_logic_vector(7 downto 0);
	signal SHIFT_REG	: std_logic_vector(7 downto 0);
	signal BITCNT		: unsigned(2 downto 0);
	signal PARITY_I		: std_logic;
begin
	-- The default condition in this statement is to ensure
	-- to cover all possibilities for example if there is a
	-- one hot decoding of the state machine with wrong states
	-- (e.g. not one of the given here).
	TXDATA <= 	'1' 			when TR_STATE = IDLE 		else
				'1' 			when TR_STATE = LOAD_SHFT 	else
				'0' 			when TR_STATE = START 		else
				SHIFT_REG(0) 	when TR_STATE = SHIFTOUT 	else
				PARITY_I		when TR_STATE = PARITY 		else
				'1'				when TR_STATE = STOP1 		else
				'1'				when TR_STATE = STOP2 		else '1';

	CLKDIV: process(CLK)
	variable CLK_LOCK	: boolean;
	variable STRB_LOCK	: boolean;
	variable CLK_DIVCNT	: unsigned (6 downto 0);
	begin
		if rising_edge(CLK) then
			if CDS = "00" then -- divider off
				if TXCLK = '0' and STRB_LOCK = false then  -- Works on negative TXCLK edge.
					CLK_STRB <= '1';
					STRB_LOCK := true;
				elsif TXCLK = '1' then
					CLK_STRB <= '0';
					STRB_LOCK := false;
				else
					CLK_STRB <= '0';
				end if;
			elsif TR_STATE = IDLE then
				-- preset the CLKDIV with the start delays
				if CDS = "01" then
					CLK_DIVCNT := "0010000"; -- div by 16 mode
				elsif CDS = "10" then
					CLK_DIVCNT := "1000000"; -- div by 64 mode
				end if;
				CLK_STRB <= '0';
			else
				-- Works on negative TXCLK edge:
				if CLK_DIVCNT > "0000000" and TXCLK = '0' and CLK_LOCK = false then
					CLK_DIVCNT := CLK_DIVCNT - 1;
					CLK_STRB <= '0';
					CLK_LOCK := true;
				elsif CDS = "01" and CLK_DIVCNT = "0000000" then
					CLK_DIVCNT := "0010000"; -- Div by 16 mode.
					if STRB_LOCK = false then
						STRB_LOCK := true;
						CLK_STRB <= '1';
					else
						CLK_STRB <= '0';
					end if;
				elsif CDS = "10" and CLK_DIVCNT = "0000000" then
					CLK_DIVCNT := "1000000"; -- Div by 64 mode.
					if STRB_LOCK = false then
						STRB_LOCK := true;
						CLK_STRB <= '1';
					else
						CLK_STRB <= '0';
					end if;
				elsif TXCLK = '1' then
					CLK_LOCK := false;
					STRB_LOCK := false;
					CLK_STRB <= '0';
				else
					CLK_STRB <= '0';
				end if;
			end if;
		end if;
	end process CLKDIV;

	DATAREG: process(RESETn, CLK)
	begin
		if RESETn = '0' then
			DATA_REG <= x"00";
		elsif rising_edge(CLK) then
			if MCLR = '1' then
				DATA_REG <= x"00";
			elsif WS(2) = '0' and CS = "011" and RWn = '0' and RS = '1' and E = '1' then
				DATA_REG <= '0' & DATA_IN(6 downto 0); -- 7 std_logic data mode.
			elsif WS(2) = '1' and CS = "011" and RWn = '0' and RS = '1' and E = '1' then
				DATA_REG <= DATA_IN; -- 8 std_logic data mode.
			end if;
		end if;
	end process DATAREG;	

	SHIFTREG: process(RESETn, CLK)
	begin
		if RESETn = '0' then
			SHIFT_REG <= x"00";
		elsif rising_edge(CLK) then
			if MCLR = '1' then
				SHIFT_REG <= x"00";
			elsif TR_STATE = LOAD_SHFT and TDRE = '0' then
				-- If during LOAD_SHIFT the transmitter data register
				-- is empty (TDRE = '1') the shift register will not
				-- be loaded. When additionally TC = "11", the break
				-- character (zero data and no stop std_logics) is sent.
				SHIFT_REG <= DATA_REG;
			elsif TR_STATE = SHIFTOUT and CLK_STRB = '1' then
				SHIFT_REG <= '0' & SHIFT_REG(7 downto 1); -- Shift right.
			end if;
		end if;
	end process SHIFTREG;	

	P_BITCNT: process(CLK)
	-- Counter for the data std_logics transmitted.
	begin
		if rising_edge(CLK) then
			if TR_STATE = SHIFTOUT and CLK_STRB = '1' then
				BITCNT <= BITCNT + 1;
			elsif TR_STATE /= SHIFTOUT then
				BITCNT <= "000";
			end if;
		end if;
	end process P_BITCNT;

	P_TDRE: process(RESETn, CLK)
	-- Transmit data register empty flag.
	begin
		if rising_edge(CLK) then
			if RESETn = '0' or MCLR = '1' then
				TDRE <= '1';
			else
				if TR_NEXT_STATE = START and TR_STATE /= START then
				-- Data has been loaded to shift register, thus data register is free again.
				-- Thanks to Lyndon Amsdon for finding a bug here. The TDRE is set to one once
				-- entering the state now.
					TDRE <= '1';
				end if;
				if  CS = "011" and RWn = '0' and RS = '1' then
					TDRE <= '0';
				end if;
			end if;
		end if;
	end process P_TDRE;

	PARITY_GEN: process(CLK)
	variable PAR_TMP	: std_logic;
	begin
		if rising_edge(CLK) then
		if TR_STATE = START then -- Calculate the parity during the start phase.
		    for i in 1 to 7 loop
		        if i = 1 then
		            PAR_TMP := SHIFT_REG(i-1) xor SHIFT_REG(i);
		        else
		            PAR_TMP := PAR_TMP xor SHIFT_REG(i);
		        end if;
		    end loop;
			if WS = "000" or WS = "010" or WS = "110" then -- Even parity.
				PARITY_I <= PAR_TMP;
			elsif WS = "001" or WS = "011" or WS = "111" then -- Odd parity.
				PARITY_I <= not PAR_TMP;
			else -- No parity for WS = "100" and WS = "101".
				PARITY_I <= '0';		
			end if;
		end if;
		end if;
	end process PARITY_GEN;

	TR_STATEREG: process(RESETn, CLK)
	begin
		if RESETn = '0' then
			TR_STATE <= IDLE;
		else
			if rising_edge(CLK) then
				if MCLR = '1' then
					TR_STATE <= IDLE;
				else
					TR_STATE <= TR_NEXT_STATE;
				end if;
			end if;
		end if;
	end process TR_STATEREG;
	
	TR_STATEDEC: process(TR_STATE, CLK_STRB, TC, BITCNT, WS, TDRE, CTSn)
	begin
		case TR_STATE is
			when IDLE =>
				if TDRE = '1' and TC = "11" then
					TR_NEXT_STATE <= LOAD_SHFT;
				elsif TDRE = '0' and CTSn = '0' then -- Start if data register is not empty.
					TR_NEXT_STATE <= LOAD_SHFT;
				else
					TR_NEXT_STATE <= IDLE;
				end if;
			when LOAD_SHFT =>
				TR_NEXT_STATE <= START;
			when START =>
				if CLK_STRB = '1' then
					TR_NEXT_STATE <= SHIFTOUT;
				else
					TR_NEXT_STATE <= START;
				end if;
			when SHIFTOUT =>
				if CLK_STRB = '1' then
					if BITCNT < "110" and WS(2) = '0' then
						TR_NEXT_STATE <= SHIFTOUT; -- Transmit 7 data std_logics.
					elsif BITCNT < "111" and WS(2) = '1' then
						TR_NEXT_STATE <= SHIFTOUT; -- Transmit 8 data std_logics.
					elsif WS = "100" or WS = "101" then
						if TDRE = '1' and TC = "11" then
							-- Break condition, do not send a stop std_logic.
							TR_NEXT_STATE <= IDLE;
						else
							TR_NEXT_STATE <= STOP1; -- No parity check enabled.
						end if;
					else
						TR_NEXT_STATE <= PARITY; -- Parity enabled.
					end if;
				else
					TR_NEXT_STATE <= SHIFTOUT;
				end if;
			when PARITY =>
				if CLK_STRB = '1' then
					if TDRE = '1' and TC = "11" then
						-- Break condition, do not send a stop std_logic.
						TR_NEXT_STATE <= IDLE;
					else
						TR_NEXT_STATE <= STOP1; -- No parity check enabled.
					end if;
				else
					TR_NEXT_STATE <= PARITY;
				end if;				
			when STOP1 =>
				if CLK_STRB = '1' and (WS = "000" or WS = "001" or WS = "100") then
					TR_NEXT_STATE <= STOP2; -- Two stop std_logics selected.
				elsif CLK_STRB = '1' then
					TR_NEXT_STATE <= IDLE; -- One stop std_logics selected.
				else
					TR_NEXT_STATE <= STOP1;
				end if;				
			when STOP2 =>
				if CLK_STRB = '1' then
					TR_NEXT_STATE <= IDLE;
				else
					TR_NEXT_STATE <= STOP2;
				end if;				
		end case;
	end process TR_STATEDEC;
end architecture BEHAVIOR;

