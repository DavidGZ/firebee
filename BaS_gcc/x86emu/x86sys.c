/****************************************************************************
*
*						Realmode X86 Emulator Library
*
*            	Copyright (C) 1996-1999 SciTech Software, Inc.
* 				     Copyright (C) David Mosberger-Tang
* 					   Copyright (C) 1999 Egbert Eich
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Language:		ANSI C
* Environment:	Any
* Developer:    Kendall Bennett
*
* Description:  This file includes subroutines which are related to
*				programmed I/O and memory access. Included in this module
*				are default functions with limited usefulness. For real
*				uses these functions will most likely be overriden by the
*				user library.
*
****************************************************************************/
/* $XFree86: xc/extras/x86emu/src/x86emu/sys.c,v 1.5 2000/08/23 22:10:01 tsi Exp $ */

#include "radeonfb.h"
#include "pci.h"

#include "x86emu.h"
#include "x86regs.h"
#include "x86debug.h"
#include "x86prim_ops.h"

extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t inl(uint16_t port);
extern void outb(uint8_t val, uint16_t port);
extern void outw(uint16_t val, uint16_t port);
extern void outl(uint32_t val, uint16_t port);

/*------------------------- Global Variables ------------------------------*/

X86EMU_sysEnv _X86EMU_env;	/* Global emulator machine state */
X86EMU_intrFuncs _X86EMU_intrTab[256];
extern struct radeonfb_info *rinfo_biosemu;
extern uint32_t offset_mem;

/*----------------------------- Implementation ----------------------------*/

/*
 * PARAMETERS:
 * addr	- Emulator memory address to read
 * 
 * RETURNS:
 * Byte value read from emulator memory.
 * 
 * REMARKS:
 * Reads a byte value from the emulator memory. 
 */
inline uint8_t X86API rdb(uint32_t addr)
{
	uint8_t val;
	
	if ((addr >= 0xA0000) && (addr <= 0xBFFFF))
	{
		val = *(uint8_t *) (offset_mem + addr);
		dbg("%s: rdb(%x) = %x\r\n", __FUNCTION__, addr, val);
	}
	else
	{
		DB(if (DEBUG_MEM_TRACE())
		{
			dbg("%s: %p 1 -> %x\r\n", __FUNCTION__, addr, val);
		} )
	}
	return val;
}
/*
 * PARAMETERS:
 * addr	- Emulator memory address to read
 * 
 * RETURNS:
 * Word value read from emulator memory.
 * 
 * REMARKS:
 * Reads a word value from the emulator memory.
 */
uint16_t X86API rdw(uint32_t addr)
{
	uint16_t val;
	
	if ((addr >= 0xA0000) && (addr <= 0xBFFFF))
	{
		val = swpw(*(uint16_t *)(offset_mem+addr));
		dbg("%s: rdw(%x) = %x\r\n", __FUNCTION__, addr, val);
	}
	else
	{
		val = (uint16_t) (* (uint8_t *)(M.mem_base + addr));
		val |= (((uint16_t)( *(uint8_t *)(M.mem_base + addr + 1))) << 8);
	}
	DB(if (DEBUG_MEM_TRACE())
	{
		dbg("%s: %p 2 -> %x\r\n", __FUNCTION__, addr, val);
	} )
	return val;
}

/*
 * PARAMETERS:
 * addr	- Emulator memory address to read
 * 
 * RETURNS:
 * Long value read from emulator memory.
 * REMARKS:
 * Reads a long value from the emulator memory. 
 */
inline uint32_t X86API rdl(uint32_t addr)
{
	uint32_t val;

	if ((addr >= 0xA0000) && (addr <= 0xBFFFF))
	{
		val = swpl(*(uint32_t *)(offset_mem + addr));
		dbg("%s: rdl(%x) = %x\r\n", __FUNCTION__, addr, val);
	}
	else
	{
		val = swpl(*(uint32_t *)(M.mem_base + addr));
	}
	DB(if (DEBUG_MEM_TRACE())
	{

		dbg("%s: %p 4 -> %x\r\n", __FUNCTION__, addr, val);
	} )
	return val;
}

/*
 * PARAMETERS:
 * addr	- Emulator memory address to read
 * val		- Value to store
 * 
 * REMARKS:
 * Writes a byte value to emulator memory.
 */
inline void X86API wrb(uint32_t addr, uint8_t val)
{
	if ((addr >= 0xA0000) && (addr <= 0xBFFFF))
	{
		*(uint8_t *)(offset_mem + addr) = val;
		dbg("%s: wrb(%x) = %x\r\n", __FUNCTION__, addr, val);
	}
	else
	{
		if (addr >= M.mem_size)
		{
			DB(
			{
				dbg("%s: mem_ptr: %p out of range!\r\n", __FUNCTION__, addr);
			})
			HALT_SYS();
		}
		*(uint8_t *)(M.mem_base + addr) = val;
	}
	DB(if (DEBUG_MEM_TRACE())
	{
		dbg("%s: %p 1 < %x\r\n", __FUNCTION__, addr, val);
	} )
}

/*
 * PARAMETERS:
 * addr	- Emulator memory address to read
 * val		- Value to store
 * 
 * REMARKS:
 * Writes a word value to emulator memory.
 */
inline void X86API wrw(uint32_t addr, uint16_t val)
{
	if ((addr >= 0xA0000) && (addr <= 0xBFFFF))
	{
		dbg("%s: wrw(%x) = %x\r\n", __FUNCTION__, addr, val);
		*(uint16_t *)(offset_mem+addr) = swpw(val);
	}
	else
	{
		if (addr > M.mem_size - 2)
		{
			DB(
			{
				dbg("%s: mem_ptr: %p out of range\r\n", __FUNCTION__, addr);
			})
			HALT_SYS();
		}
		*(uint8_t *)(M.mem_base + addr) = (uint8_t) val;
		*(uint8_t *)(M.mem_base + addr + 1) = (uint8_t) (val >> 8);
	}
	DB(if (DEBUG_MEM_TRACE())
	{
		dbg("%s: %p 2 <- %x\r\n", __FUNCTION__, addr, val);
	} )
}

/*
 * PARAMETERS:
 * addr	- Emulator memory address to read
 * val		- Value to store
 * 
 * REMARKS:
 * Writes a long value to emulator memory. 
 */
inline void X86API wrl(uint32_t addr, uint32_t val)
{
	if ((addr >= 0xA0000) && (addr <= 0xBFFFF))
	{
		dbg("%s: wrl(%x) = %x\r\n", __FUNCTION__, addr, val);
		*(uint32_t *)(offset_mem+addr) = swpl(val);
	}
	else
	{
		if (addr > M.mem_size - 4)
		{
			DB(
			{
				dbg("%s: mem_ptr: address %x out of range!\r\n", __FUNCTION__, addr);
			}
			)
			HALT_SYS();
		}
		*(uint32_t *)(M.mem_base + addr) = swpl(val);
	}
	DB(if (DEBUG_MEM_TRACE())
	{
		dbg("%s: %p 4 <- %x\r\n", __FUNCTION__, addr, val);
	} )
}

/*
 * PARAMETERS:
 * addr	- PIO address to read
 * RETURN:
 * 0
 * REMARKS:
 * Default PIO byte read function. Doesn't perform real inb.
 */
inline uint8_t X86API p_inb(X86EMU_pioAddr addr)
{
	DB(if (DEBUG_IO_TRACE())
	{
		dbg("%s: inb(%p)\r\n", __FUNCTION__);
	} )
	return inb(addr);
}

/*
 * PARAMETERS:
 * addr	- PIO address to read
 * RETURN:
 * 0
 * REMARKS:
 * Default PIO word read function. Doesn't perform real inw.
 */
inline uint16_t X86API p_inw(X86EMU_pioAddr addr)
{
	DB(if (DEBUG_IO_TRACE())
	{
		dbg("%s: inw(%p)\r\n", __FUNCTION__, addr);
	} )
	return inw(addr);
}

/*
 * PARAMETERS:
 * addr	- PIO address to read
 * RETURN:
 * 0
 * REMARKS:
 * Default PIO long read function. Doesn't perform real inl.
 */
inline uint32_t X86API p_inl(X86EMU_pioAddr addr)
{
	DB(if (DEBUG_IO_TRACE())
	{
		dbg("%s: inl %p\r\n", __FUNCTION__, addr);
	} )
	return inl(addr);
}

/*
 * PARAMETERS:
 * addr	- PIO address to write
 * val     - Value to store
 * REMARKS:
 * Default PIO byte write function. Doesn't perform real outb.
 */
inline void X86API p_outb(X86EMU_pioAddr addr, uint8_t val)
{
	DB(if (DEBUG_IO_TRACE())
	{
		dbg("%s: outb %x -> %x\r\n", __FUNCTION__, val, addr);
	} )
	outb(val, addr);
	return;
}

/*
 * PARAMETERS:
 * addr	- PIO address to write
 * val     - Value to store
 * REMARKS:
 * Default PIO word write function. Doesn't perform real outw.
 */
inline void X86API p_outw(X86EMU_pioAddr addr, uint16_t val)
{
	DB(if (DEBUG_IO_TRACE())
	{
		dbg("outw %x -> %x\r\n", __FUNCTION__, val, addr);
	} )
	outw(val, addr);
	return;
}

/*
 * PARAMETERS:
 * addr	- PIO address to write
 * val     - Value to store
 * REMARKS:
 * Default PIO ;ong write function. Doesn't perform real outl.
 */
inline void X86API p_outl(X86EMU_pioAddr addr, uint32_t val)
{
	DB(if (DEBUG_IO_TRACE())
	{
		dbg("%s: outl %x -> %x\r\n", __FUNCTION__, val, addr);
	} )
	outl(val, addr);
	return;
}

/*------------------------- Global Variables ------------------------------*/

uint8_t(X86APIP sys_rdb) (uint32_t addr) = rdb;
uint16_t(X86APIP sys_rdw) (uint32_t addr) = rdw;
uint32_t(X86APIP sys_rdl) (uint32_t addr) = rdl;
void (X86APIP sys_wrb) (uint32_t addr, uint8_t val) = wrb;
void (X86APIP sys_wrw) (uint32_t addr, uint16_t val) = wrw;
void (X86APIP sys_wrl) (uint32_t addr, uint32_t val) = wrl;
uint8_t(X86APIP sys_inb) (X86EMU_pioAddr addr) = p_inb;
uint16_t(X86APIP sys_inw) (X86EMU_pioAddr addr) = p_inw;
uint32_t(X86APIP sys_inl) (X86EMU_pioAddr addr) = p_inl;
void (X86APIP sys_outb) (X86EMU_pioAddr addr, uint8_t val) = p_outb;
void (X86APIP sys_outw) (X86EMU_pioAddr addr, uint16_t val) = p_outw;
void (X86APIP sys_outl) (X86EMU_pioAddr addr, uint32_t val) = p_outl;

/*----------------------------- Setup -------------------------------------*/

#if 0 // cannot works whith data in flash
/****************************************************************************
PARAMETERS:
funcs	- New memory function pointers to make active

REMARKS:
This function is used to set the pointers to functions which access
memory space, allowing the user application to override these functions
and hook them out as necessary for their application.
****************************************************************************/
void X86EMU_setupMemFuncs(X86EMU_memFuncs * funcs)
{
	sys_rdb = funcs->rdb;
	sys_rdw = funcs->rdw;
	sys_rdl = funcs->rdl;
	sys_wrb = funcs->wrb;
	sys_wrw = funcs->wrw;
	sys_wrl = funcs->wrl;
}

/****************************************************************************
PARAMETERS:
funcs	- New programmed I/O function pointers to make active

REMARKS:
This function is used to set the pointers to functions which access
I/O space, allowing the user application to override these functions
and hook them out as necessary for their application.
****************************************************************************/
void X86EMU_setupPioFuncs(X86EMU_pioFuncs * funcs)
{
	sys_inb = funcs->inb;
	sys_inw = funcs->inw;
	sys_inl = funcs->inl;
	sys_outb = funcs->outb;
	sys_outw = funcs->outw;
	sys_outl = funcs->outl;
}
#endif

/****************************************************************************
PARAMETERS:
funcs	- New interrupt vector table to make active

REMARKS:
This function is used to set the pointers to functions which handle
interrupt processing in the emulator, allowing the user application to
hook interrupts as necessary for their application. Any interrupts that
are not hooked by the user application, and reflected and handled internally
in the emulator via the interrupt vector table. This allows the application
to get control when the code being emulated executes specific software
interrupts.
****************************************************************************/
void X86EMU_setupIntrFuncs(X86EMU_intrFuncs funcs[])
{
	int i;

	for (i = 0; i < 256; i++)
		_X86EMU_intrTab[i] = NULL;
	if (funcs) {
		for (i = 0; i < 256; i++)
			_X86EMU_intrTab[i] = funcs[i];
	}
}

/****************************************************************************
PARAMETERS:
int	- New software interrupt to prepare for

REMARKS:
This function is used to set up the emulator state to exceute a software
interrupt. This can be used by the user application code to allow an
interrupt to be hooked, examined and then reflected back to the emulator
so that the code in the emulator will continue processing the software
interrupt as per normal. This essentially allows system code to actively
hook and handle certain software interrupts as necessary.
****************************************************************************/
void X86EMU_prepareForInt(int num)
{
	push_word((uint16_t) M.x86.R_FLG);
	CLEAR_FLAG(F_IF);
	CLEAR_FLAG(F_TF);
	push_word(M.x86.R_CS);
	M.x86.R_CS = mem_access_word(num * 4 + 2);
	push_word(M.x86.R_IP);
	M.x86.R_IP = mem_access_word(num * 4);
	M.x86.intr = 0;
}

void X86EMU_setMemBase(void *base, unsigned long size)
{
	M.mem_base = (int) base;
	M.mem_size = size;
}
