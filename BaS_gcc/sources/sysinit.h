/*
 * File:		sysinit.h
 * Purpose:		Firebee Power-on Reset configuration
 *
 * Notes:
 *
 * This file is part of BaS_gcc.
 *
 * BaS_gcc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BaS_gcc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BaS_gcc.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2010 - 2012 F. Aschwanden
 * Copyright 2011 - 2012 V. Riviere
 * Copyright 2012        M. Froeschle
 *
 */

#ifndef __SYSINIT_H__
#define __SYSINIT_H__
#include <bas_types.h>

extern void wait_10us(void);
extern inline bool waitfor(uint32_t us, int (*condition)(void));

/* send a 16-bit word out on the serial port */
#define uart_out_word(a)	MCF_PSC0_PSCTB_8BIT = (a)

/* adresses where FPGA data lives in flash */
#define FPGA_FLASH_DATA ((uint8_t *) 0xe0700000L)
#define FPGA_FLASH_DATA_END ((uint8_t *) 0xe0800000L)

/* function(s) from init_fpga.c */
extern void init_fpga(void);

#endif /* __SYSINIT_H__ */


