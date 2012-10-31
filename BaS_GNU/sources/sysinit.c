/*
 * File:		sysinit.c
 * Purpose:		Power-on Reset configuration of the Firebee board.
 *
 * Notes: 
 *
 */

#include "MCF5475.h"
#include "startcf.h"
#include "cache.h"
#include "sysinit.h"
#include "bas_printf.h"

extern void xprintf_before_copy(const char *fmt, ...);
#define xprintf	 xprintf_before_copy

#define UNUSED(x)       (void)(x)               /* Unused variable         */

extern volatile long _VRAM;	/* start address of video ram from linker script */

/*
 * wait routines
 */
void wait_10ms(void)
{
	register uint32_t target = MCF_SLT_SCNT(0) - 1320000;

	while (MCF_SLT_SCNT(0) > target);
}

void wait_1ms(void)
{
	register uint32_t target = MCF_SLT_SCNT(0) - 132000;

	while (MCF_SLT_SCNT(0) > target);
}

void wait_100us(void)
{
	register uint32_t target = MCF_SLT_SCNT(0) - 13200;

	while (MCF_SLT_SCNT(0) > target);
}

void wait_50us(void)
{
	register uint32_t target = MCF_SLT_SCNT(0) - 6600;

	while (MCF_SLT_SCNT(0) > target);
}

void wait_10us(void)
{
	register uint32_t target = MCF_SLT_SCNT(0) - 1320;

	while (MCF_SLT_SCNT(0) > target);
}

void wait_1us(void)
{
	register uint32_t target = MCF_SLT_SCNT(0) - 132;

	while (MCF_SLT_SCNT(0) > target);
}

/*
 * init SLICE TIMER 0 
 * all  = 32.538 sec = 30.736mHz
 * BYT0 = 127.1ms/tick = 7.876Hz   offset 0
 * BYT1 = 496.5us/tick = 2.014kHz  offset 1
 * BYT2 = 1.939us/tick = 515.6kHz  offset 2
 * BYT3 = 7.576ns/tick = 132.00MHz offset 3
 * count down!!! 132MHz!!!
 */
void init_slt(void)
{
	xprintf("slice timer initialization: ");
	MCF_SLT0_STCNT = 0xffffffff;
	MCF_SLT0_SCR = MCF_SLT_SCR_TEN | MCF_SLT_SCR_RUN;	/* enable and run continuously */
	xprintf("finished\r\n");
}

/*
 * init GPIO general purpose I/O module
 */
void init_gpio(void)
{
	/*
	 * pad register P.S.:FBCTL and FBCS set correctly at reset
	 */

	/*
	 * configure all four 547x GPIO module DMA pins:
	 *
	 * /DACK1 - DMA acknowledge 1
 	 * /DACK0 - DMA acknowledge 0
 	 * /DREQ1 - DMA request 1
 	 * /DREQ0 - DMA request 0
 	 *
	 * for DMA operation
	 */
	MCF_PAD_PAR_DMA = MCF_PAD_PAR_DMA_PAR_DACK0_DACK0 |
			MCF_PAD_PAR_DMA_PAR_DACK1_DACK1 |
			MCF_PAD_PAR_DMA_PAR_DREQ1_DREQ1 |
			MCF_PAD_PAR_DMA_PAR_DREQ0_DREQ0;

	/*
	 * configure FEC0 pin assignment on GPIO module as FEC0
	 * configure FEC1 pin assignment (PAR_E17, PAR_E1MII) as GPIO,
	 * /IRQ5 and /IRQ6  from GPIO (needs to be disabled on EPORT module, which also can
	 * use those INTs).
	 */
	MCF_PAD_PAR_FECI2CIRQ = MCF_PAD_PAR_FECI2CIRQ_PAR_E07 |
			MCF_PAD_PAR_FECI2CIRQ_PAR_E0MII |
			MCF_PAD_PAR_FECI2CIRQ_PAR_E0MDIO |
			MCF_PAD_PAR_FECI2CIRQ_PAR_E0MDC |
			MCF_PAD_PAR_FECI2CIRQ_PAR_E1MDIO_E1MDIO |
			MCF_PAD_PAR_FECI2CIRQ_PAR_E1MDC_E1MDC |
			MCF_PAD_PAR_FECI2CIRQ_PAR_SDA |
			MCF_PAD_PAR_FECI2CIRQ_PAR_SCL |
			MCF_PAD_PAR_FECI2CIRQ_PAR_IRQ6 |
			MCF_PAD_PAR_FECI2CIRQ_PAR_IRQ5;

	/*
	 * configure PCI Grant pin assignment on GPIO module:
	 *
	 * /PCIBG4 used as FlexBus /TBST
	 * /PCIBG3 used as general purpose I/O
	 * /PCIBG2 used as /PCIBG2
	 * /PCIBG1 used as /PCIBG1
	 * /PCIBG0 used as /PCIBG0
	 */
	MCF_PAD_PAR_PCIBG = MCF_PAD_PAR_PCIBG_PAR_PCIBG4_TBST |
			MCF_PAD_PAR_PCIBG_PAR_PCIBG3_GPIO |
			MCF_PAD_PAR_PCIBG_PAR_PCIBG2_PCIBG2 |
			MCF_PAD_PAR_PCIBG_PAR_PCIBG1_PCIBG1 |
			MCF_PAD_PAR_PCIBG_PAR_PCIBG0_PCIBG0;

	/*
	 * configure PCI request pin assignment on GPIO module:
	 * /PCIBR4 as /IRQ4
	 * /PCIBR3 as GPIO (PIC)
	 * /PCIBR2 as /PCIBR2
	 * /PCIBR1 as /PCIBR1
	 * /PCIBR0 as /PCIBR0
	 */
	MCF_PAD_PAR_PCIBR = MCF_PAD_PAR_PCIBR_PAR_PCIBR4_IRQ4 |
			MCF_PAD_PAR_PCIBR_PAR_PCIBR3_GPIO |
			MCF_PAD_PAR_PCIBR_PAR_PCIBR2_PCIBR2 |
			MCF_PAD_PAR_PCIBR_PAR_PCIBR1_PCIBR1 |
			MCF_PAD_PAR_PCIBR_PAR_PCIBR0_PCIBR0;

	/*
	 * configure PSC3 pin assignment on GPIO module:
	 * /PSC3CTS as /PSC3PTS
	 * /PSC3RTS as /PSC3RTS
	 * PSC3RXD as PSC3RXD
	 * PSC3TXD as PSC3TXD
	 */

	MCF_PAD_PAR_PSC3 = MCF_PAD_PAR_PSC3_PAR_TXD3 | MCF_PAD_PAR_PSC3_PAR_RXD3;

	/*
	 * Configure PSC1 pin assignment on GPIO module:
	 * - all pins configured for serial interface operation
	 */

	MCF_PAD_PAR_PSC1 = MCF_PAD_PAR_PSC1_PAR_CTS1_CTS |
			MCF_PAD_PAR_PSC1_PAR_RTS1_RTS |
			MCF_PAD_PAR_PSC1_PAR_RXD1 |
			MCF_PAD_PAR_PSC1_PAR_TXD1;

	/*
	 * Configure PSC0 Pin Assignment on GPIO module:
	 * - all pins configured for serial interface operation
	 */

	MCF_PAD_PAR_PSC0 = MCF_PAD_PAR_PSC0_PAR_CTS0_CTS |
			MCF_PAD_PAR_PSC0_PAR_RTS0_RTS |
			MCF_PAD_PAR_PSC0_PAR_RXD0 |
			MCF_PAD_PAR_PSC0_PAR_TXD0;

	MCF_PAD_PAR_DSPI = 0b0001111111111111;	/* DSPI NORMAL */

	MCF_PAD_PAR_TIMER = 0b00101101;	/* TIN3..2=#IRQ3..2;TOUT3..2=NORMAL */

	// ALLE OUTPUTS NORMAL LOW


	// ALLE DIR NORMAL INPUT = 0
	MCF_GPIO_PDDR_FEC1L = 0b00011110;	/* OUT: 4=LED,3=PRG_DQ0,2=#FPGA_CONFIG,1=PRG_CLK(FPGA) */
}

/*
 * init serial
 */
void init_serial(void)
{
	/* PSC0: SER1 */
	MCF_PSC0_PSCSICR = 0;		/* PSC control register: select UART mode */
	MCF_PSC0_PSCCSR = 0xDD;		/* use TX and RX baud rate from PSC timer */
	MCF_PSC0_PSCCTUR = 0x00;	/* =\ */
	MCF_PSC0_PSCCTLR = 36;		/* divide sys_clk by 36 => BAUD RATE = 115200 bps */
	MCF_PSC0_PSCCR = 0x20;		/* reset receiver and RxFIFO */
	MCF_PSC0_PSCCR = 0x30;		/* reset transmitter and TxFIFO */
	MCF_PSC0_PSCCR = 0x40;		/* reset all error status */
	MCF_PSC0_PSCCR = 0x50;		/* reset break change interrupt */
	MCF_PSC0_PSCCR = 0x10;		/* reset MR pointer */
	MCF_PSC0_PSCIMR = 0x8700;	/* enable input port change interrupt, enable delta break interrupt, */
								/* enable receiver interrupt/request, enable transceiver interrupt/request */

	MCF_PSC0_PSCACR = 0x03;		/* enable state change of CTS */
	MCF_PSC0_PSCMR1 = 0xb3;		/* 8 bit, no parity */
	MCF_PSC0_PSCMR2 = 0x07;		/* 1 stop bit */
	MCF_PSC0_PSCRFCR = 0x0F;
	MCF_PSC0_PSCTFCR = 0x0F;
	MCF_PSC0_PSCRFAR = 0x00F0;
	MCF_PSC0_PSCTFAR = 0x00F0;
	MCF_PSC0_PSCOPSET = 0x01;
	MCF_PSC0_PSCCR = 0x05;

	/* PSC3: PIC */
	MCF_PSC3_PSCSICR = 0;	// UART
	MCF_PSC3_PSCCSR = 0xDD;
	MCF_PSC3_PSCCTUR = 0x00;
	MCF_PSC3_PSCCTLR = 36;	// BAUD RATE = 115200
	MCF_PSC3_PSCCR = 0x20;
	MCF_PSC3_PSCCR = 0x30;
	MCF_PSC3_PSCCR = 0x40;
	MCF_PSC3_PSCCR = 0x50;
	MCF_PSC3_PSCCR = 0x10;
	MCF_PSC3_PSCIMR = 0x0200;	// receiver interrupt enable
	MCF_PSC3_PSCACR = 0x03;
	MCF_PSC3_PSCMR1 = 0xb3;
	MCF_PSC3_PSCMR2 = 0x07;
	MCF_PSC3_PSCRFCR = 0x0F;
	MCF_PSC3_PSCTFCR = 0x0F;
	MCF_PSC3_PSCRFAR = 0x00F0;
	MCF_PSC3_PSCTFAR = 0x00F0;
	MCF_PSC3_PSCOPSET = 0x01;
	MCF_PSC3_PSCCR = 0x05;
	MCF_INTC_ICR32 = 0x3F;	//MAXIMALE PRIORITY/**********/

	xprintf("serial interfaces initialization: finished\r\n");
}

/********************************************************************/
/* Initialize DDR DIMMs on the EVB board */
/********************************************************************/
void init_ddram(void)
{
	xprintf("SDRAM controller initialization: ");

	/*
	 * Check to see if the SDRAM has already been initialized
	 * by a run control tool
	 */
	if (!(MCF_SDRAMC_SDCR & MCF_SDRAMC_SDCR_REF)) {
		/* Basic configuration and initialization */
		MCF_SDRAMC_SDRAMDS = 0x000002AA;/* SDRAMDS configuration */
		MCF_SDRAMC_CS0CFG = 0x0000001A;	/* SDRAM CS0 configuration (128Mbytes 0000_0000 - 07FF_FFFF) */
		MCF_SDRAMC_CS1CFG = 0x0800001A;	/* SDRAM CS1 configuration (128Mbytes 0800_0000 - 0FFF_FFFF) */
		MCF_SDRAMC_CS2CFG = 0x1000001A;	/* SDRAM CS2 configuration (128Mbytes 1000_0000 - 07FF_FFFF) */
		MCF_SDRAMC_CS3CFG = 0x1800001A;	/* SDRAM CS3 configuration (128Mbytes 1800_0000 - 1FFF_FFFF) */
		MCF_SDRAMC_SDCFG1 = 0x73622830;	/* SDCFG1 */
		MCF_SDRAMC_SDCFG2 = 0x46770000;	/* SDCFG2 */
		MCF_SDRAMC_SDCR = 0xE10D0002;	/* SDCR + IPALL */
		MCF_SDRAMC_SDMR = 0x40010000;	/* SDMR (write to LEMR) */
		MCF_SDRAMC_SDMR = 0x048D0000;	/* SDRM (write to LMR) */
		MCF_SDRAMC_SDCR = 0xE10D0002;	/* SDCR + IPALL */
		MCF_SDRAMC_SDCR = 0xE10D0004;	/* SDCR + IREF (first refresh) */
		MCF_SDRAMC_SDCR = 0xE10D0004;	/* SDCR + IREF (second refresh) */
		MCF_SDRAMC_SDMR = 0x008D0000;	/* SDMR (write to LMR) */
		MCF_SDRAMC_SDCR = 0x710D0F00;	/* SDCR (lock SDMR and enable refresh) */

		xprintf("finished\r\n");
	}
	else
	{
		xprintf("skipped. Already initialized (running from RAM)\r\n");
	}
}

/*
 * initialize FlexBus chip select registers
 */
void init_fbcs()
{
	xprintf("FlexBus chip select registers initialization: ");

	/* Flash */
	MCF_FBCS0_CSAR = 0xE0000000;	/* flash base address */
	MCF_FBCS0_CSCR = MCF_FBCS_CSCR_PS_16 |
			MCF_FBCS_CSCR_WS(4)|
			MCF_FBCS_CSCR_AA;
	MCF_FBCS0_CSMR = MCF_FBCS_CSMR_BAM_8M |
			MCF_FBCS_CSMR_V;				/* 8 MByte on */

	MCF_FBCS1_CSAR = 0xFFF00000;			/* ATARI I/O ADRESS */
	MCF_FBCS1_CSCR = MCF_FBCS_CSCR_PS_16	/* 16BIT PORT */
	    | MCF_FBCS_CSCR_WS(8)				/* DEFAULT 8WS */
	    | MCF_FBCS_CSCR_AA;					/* AA */
	MCF_FBCS1_CSMR = MCF_FBCS_CSMR_BAM_1M | MCF_FBCS_CSMR_V;

	MCF_FBCS2_CSAR = 0xF0000000;			// NEUER I/O ADRESS-BEREICH
	MCF_FBCS2_CSCR = MCF_FBCS_CSCR_PS_32	// 32BIT PORT
	    | MCF_FBCS_CSCR_WS(8)				// DEFAULT 4WS
	    | MCF_FBCS_CSCR_AA;					// AA
	MCF_FBCS2_CSMR = (MCF_FBCS_CSMR_BAM_128M	// F000'0000-F7FF'FFFF
			  | MCF_FBCS_CSMR_V);

	MCF_FBCS3_CSAR = 0xF8000000;			// NEUER I/O ADRESS-BEREICH
	MCF_FBCS3_CSCR = MCF_FBCS_CSCR_PS_16	// 16BIT PORT
	    | MCF_FBCS_CSCR_AA;	// AA 
	MCF_FBCS3_CSMR = (MCF_FBCS_CSMR_BAM_64M	// F800'0000-FBFF'FFFF
			  | MCF_FBCS_CSMR_V);

	MCF_FBCS4_CSAR = 0x40000000;			// VIDEO RAM BEREICH, #FB_CS3 WIRD NICHT BENÜTZT, DECODE DIREKT AUF DEM FPGA
	MCF_FBCS4_CSCR = MCF_FBCS_CSCR_PS_32	// 32BIT PORT
	    | MCF_FBCS_CSCR_BSTR				// BURST READ ENABLE
	    | MCF_FBCS_CSCR_BSTW;				// BURST WRITE ENABLE
	MCF_FBCS4_CSMR = MCF_FBCS_CSMR_BAM_1G	// 4000'0000-7FFF'FFFF
			  | MCF_FBCS_CSMR_V;

	xprintf("finished\r\n");
}


void wait_pll(void)
{
	register uint32_t trgt = MCF_SLT0_SCNT - 100000;
	do {
	} while ((* (volatile int16_t *) 0xf0000800 < 0) && MCF_SLT0_SCNT > trgt);
}

static volatile uint8_t *pll_base = (volatile uint8_t *) 0xf0000600;

void init_pll(void)
{
	xprintf("FPGA PLL initialization: ");

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x48) = 27;	/* loopfilter  r */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x08) = 1;		/* charge pump 1 */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x00) = 12;		/* N counter high = 12 */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x40) = 12;		/* N counter low = 12 */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x114) = 1;		/* ck1 bypass */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x118) = 1;		/* ck2 bypass */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x11c) = 1;		/* ck3 bypass */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x10) = 1;		/* ck0 high  = 1 */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x50) = 1;		/* ck0 low = 1 */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x144) = 1;		/* M odd division */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x44) = 1;		/* M low = 1 */

	wait_pll();
	* (volatile uint16_t *) (pll_base + 0x04) = 145;	/* M high = 145 = 146 MHz */

	wait_pll();

	* (volatile uint8_t *) 0xf0000800 = 0;				/* set */

	xprintf("finished\r\n");
}



/*
 * INIT VIDEO DDR RAM
 */

#define NOP() __asm__ __volatile__("nop\n\t" : : : "memory")

void init_video_ddr(void) {
	xprintf("init video RAM: ");

	* (volatile uint16_t *) 0xf0000400 = 0xb;	/* set cke = 1, cs=1, config = 1 */
	NOP();

	_VRAM = 0x00050400;	/* IPALL */
	NOP();

	_VRAM = 0x00072000;	/* load EMR pll on */
	NOP();

	_VRAM = 0x00070122;	/* load MR: reset pll, cl=2, burst=4lw */
	NOP();

	_VRAM = 0x00050400;	/* IPALL */
	NOP();

	_VRAM = 0x00060000;	/* auto refresh */
	NOP();

	_VRAM = 0x00060000;	/* auto refresh */
	NOP();

	_VRAM = 0000070022;	/* load MR dll on */
	NOP();

	* (uint32_t *) 0xf0000400 = 0x01070002; /* fifo on, refresh on, ddrcs und cke on, video dac on */

	xprintf("finished\r\n");
}


#define	 PCI_MEMORY_OFFSET	(0x80000000)
#define	 PCI_MEMORY_SIZE	(0x40000000)
#define	 PCI_IO_OFFSET		(0xD0000000)
#define	 PCI_IO_SIZE		(0x10000000)

/*
 * INIT PCI
 */
void init_PCI(void) {
	xprintf("PCI BUS controller initialization: ");

	MCF_PCIARB_PACR = MCF_PCIARB_PACR_INTMPRI
		 + MCF_PCIARB_PACR_EXTMPRI(0x1F)
		 + MCF_PCIARB_PACR_INTMINTEN
		 + MCF_PCIARB_PACR_EXTMINTEN(0x1F);

	// Setup burst parameters
	MCF_PCI_PCICR1 = MCF_PCI_PCICR1_CACHELINESIZE(4) + MCF_PCI_PCICR1_LATTIMER(32);
	MCF_PCI_PCICR2 = MCF_PCI_PCICR2_MINGNT(16) + MCF_PCI_PCICR2_MAXLAT(16);

	// Turn on error signaling
	MCF_PCI_PCIICR = MCF_PCI_PCIICR_TAE + MCF_PCI_PCIICR_TAE + MCF_PCI_PCIICR_REE + 32;
	MCF_PCI_PCIGSCR |= MCF_PCI_PCIGSCR_SEE;

	/* Configure Initiator Windows */
	/* initiator window 0 base / translation adress register */
	MCF_PCI_PCIIW0BTAR = (PCI_MEMORY_OFFSET + ((PCI_MEMORY_SIZE -1) >> 8)) & 0xffff0000;

	/* initiator window 1 base / translation adress register */
	MCF_PCI_PCIIW1BTAR = (PCI_IO_OFFSET + ((PCI_IO_SIZE - 1) >> 8)) & 0xffff0000;

	/* initiator window 2 base / translation address register */
	MCF_PCI_PCIIW2BTAR = 0L;	/* not used */

	/* initiator window configuration register */
	MCF_PCI_PCIIWCR = MCF_PCI_PCIIWCR_WINCTRL0_MEMRDLINE + MCF_PCI_PCIIWCR_WINCTRL1_IO;

	/* reset PCI devices */
	MCF_PCI_PCIGSCR &= ~MCF_PCI_PCIGSCR_PR;

	xprintf("finished\r\n");
}
	

/*
 * probe for UPC720101 (USB)
 */
void test_upd720101(void) 
{
	xprintf("UDP720101 USB controller initialization: ");

	/* select UPD720101 AD17 */
	MCF_PCI_PCICAR = MCF_PCI_PCICAR_E +
		MCF_PCI_PCICAR_DEVNUM(17) +
		MCF_PCI_PCICAR_FUNCNUM(0) +
		MCF_PCI_PCICAR_DWORD(0);

	if (* (uint32_t *) PCI_IO_OFFSET == 0x33103500)
	{
		MCF_PCI_PCICAR = MCF_PCI_PCICAR_E + 
			MCF_PCI_PCICAR_DEVNUM(17) +
			MCF_PCI_PCICAR_FUNCNUM(0) +
			MCF_PCI_PCICAR_DWORD(57);

		* (uint8_t *) PCI_IO_OFFSET = 0x20;
	}
	else
	{
		MCF_PSC0_PSCTB_8BIT = 'NOT ';

		MCF_PCI_PCICAR = MCF_PCI_PCICAR_DEVNUM(17) +
			MCF_PCI_PCICAR_FUNCNUM(0) +
			MCF_PCI_PCICAR_DWORD(57);
	}
	xprintf("finished\r\n");
}

static void wait_i2c_transfer_finished(void)
{
	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));	/* wait until interrupt bit has been set */
	MCF_I2C_I2SR &= ~MCF_I2C_I2SR_IIF; 			/* clear interrupt bit (byte transfer finished */
}

/*
 * TFP410 (DVI) on
 */
void dvi_on(void) {
	uint8_t receivedByte;
	uint8_t dummyByte; /* only used for a dummy read */
	int num_tries = 0;
	
	xprintf("DVI digital video output initialization: ");

	MCF_I2C_I2FDR = 0x3c;		/* divide system clock by 1280: 100kHz standard */

	do {
		/* disable all i2c interrupt routing targets */
		MCF_I2C_I2ICR = 0x0; //~(MCF_I2C_I2ICR_IE | MCF_I2C_I2ICR_RE | MCF_I2C_I2ICR_TE | MCF_I2C_I2ICR_BNBE);

		/* disable i2c, disable i2c interrupts, slave, recieve, i2c = acknowledge, no repeat start */
		MCF_I2C_I2CR = 0x0;

		/* repeat start, transmit acknowledge */
		MCF_I2C_I2CR = MCF_I2C_I2CR_RSTA | MCF_I2C_I2CR_TXAK;

		receivedByte = MCF_I2C_I2DR;	/* read a byte */
		MCF_I2C_I2SR = 0x0;		/* clear status register */
		MCF_I2C_I2CR = 0x0;		/* disable i2c */

		MCF_I2C_I2ICR = MCF_I2C_I2ICR_IE;	/* route i2c interrupts to cpu */
		/* i2c enable, master mode, transmit acknowledge */
		MCF_I2C_I2CR = MCF_I2C_I2CR_IEN | MCF_I2C_I2CR_MSTA | MCF_I2C_I2CR_MTX;

		MCF_I2C_I2DR = 0x7a;				/* send data: address of TFP410 */

		wait_i2c_transfer_finished();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK)		/* next try if no acknowledge */
			continue;

		MCF_I2C_I2DR = 0x00;						/* send data: SUB ADRESS 0 */
		wait_i2c_transfer_finished();

		MCF_I2C_I2CR |= MCF_I2C_I2CR_RSTA;			/* repeat start */
		MCF_I2C_I2DR = 0x7b;						/* begin read */

		wait_i2c_transfer_finished();
		if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK)		/* next try if no acknowledge */
			continue;

		MCF_I2C_I2CR &= 0xef; //~MCF_I2C_I2CR_MTX;			/* switch to receive mode */
		dummyByte = MCF_I2C_I2DR; 						/* dummy read */

		wait_i2c_transfer_finished();

		MCF_I2C_I2CR |= MCF_I2C_I2CR_TXAK; 			/* transmit acknowledge enable */
		receivedByte = MCF_I2C_I2DR;						/* read a byte */

		wait_i2c_transfer_finished();

		MCF_I2C_I2CR = MCF_I2C_I2CR_IEN;			/* stop */

		dummyByte = MCF_I2C_I2DR; // dummy read

		if (receivedByte != 0x4c)
			continue;

		MCF_I2C_I2CR = 0x0; // stop
		MCF_I2C_I2SR = 0x0; // clear sr

		while ((MCF_I2C_I2SR & MCF_I2C_I2SR_IBB));	/* wait for bus free */

		MCF_I2C_I2CR = 0xb0; // on tx master
		MCF_I2C_I2DR = 0x7A;

		wait_i2c_transfer_finished();

		if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK)
			continue;

		MCF_I2C_I2DR = 0x08; // SUB ADRESS 8

		wait_i2c_transfer_finished();

		MCF_I2C_I2DR = 0xbf; // ctl1: power on, T:M:D:S: enable

		wait_i2c_transfer_finished();

		MCF_I2C_I2CR = 0x80; // stop
		dummyByte = MCF_I2C_I2DR; // dummy read
		MCF_I2C_I2SR = 0x0; // clear sr

		while ((MCF_I2C_I2SR & MCF_I2C_I2SR_IBB)); /* wait until bus free */

		MCF_I2C_I2CR = 0xb0;
		MCF_I2C_I2DR = 0x7A;

		wait_i2c_transfer_finished();

		if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK)
			continue;

		MCF_I2C_I2DR = 0x08; // SUB ADRESS 8

		wait_i2c_transfer_finished();

		MCF_I2C_I2CR |= 0x4; // repeat start
		MCF_I2C_I2DR = 0x7b; // beginn read

		wait_i2c_transfer_finished();

		if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK)
			continue;

		MCF_I2C_I2CR &= 0xef; // switch to rx
		dummyByte = MCF_I2C_I2DR; // dummy read

		wait_i2c_transfer_finished();
		MCF_I2C_I2CR |= 0x08; // txak=1

		wait_50us();

		receivedByte = MCF_I2C_I2DR;

		wait_i2c_transfer_finished();

		MCF_I2C_I2CR = 0x80; // stop

		dummyByte = MCF_I2C_I2DR; // dummy read
		num_tries++;
	} while ((receivedByte != 0xbf) && (num_tries < 10));

	if (num_tries >= 10) {
		xprintf("FAILED!\r\n");
	} else {
		xprintf("finished\r\n");
	}
	UNUSED(dummyByte);
	// Avoid warning
}


/*
 * AC97
 */
void init_ac97(void) {
	// PSC2: AC97 ----------
	int i;
	int zm;
	int va;
	int vb;
	int vc;
	
	xprintf("AC97 sound chip initialization: ");
	MCF_PAD_PAR_PSC2 = MCF_PAD_PAR_PSC2_PAR_RTS2_RTS	// PSC2=TX,RX BCLK,CTS->AC'97
	       | MCF_PAD_PAR_PSC2_PAR_CTS2_BCLK
			 | MCF_PAD_PAR_PSC2_PAR_TXD2
			 | MCF_PAD_PAR_PSC2_PAR_RXD2;
	MCF_PSC2_PSCMR1 = 0x0;
	MCF_PSC2_PSCMR2 = 0x0;
	MCF_PSC2_PSCIMR = 0x0300;
	MCF_PSC2_PSCSICR = 0x03;	//AC97           
	MCF_PSC2_PSCRFCR = 0x0f000000;
	MCF_PSC2_PSCTFCR = 0x0f000000;
	MCF_PSC2_PSCRFAR = 0x00F0;
	MCF_PSC2_PSCTFAR = 0x00F0;

	for (zm = 0; zm < 100000; zm++)	// wiederholen bis synchron
	{
		MCF_PSC2_PSCCR = 0x20;
		MCF_PSC2_PSCCR = 0x30;
		MCF_PSC2_PSCCR = 0x40;
		MCF_PSC2_PSCCR = 0x05;

		// MASTER VOLUME -0dB
		MCF_PSC2_PSCTB_AC97 = 0xE0000000;	//START SLOT1 + SLOT2, FIRST FRAME
		MCF_PSC2_PSCTB_AC97 = 0x02000000;	//SLOT1:WR REG MASTER VOLUME adr 0x02

		for (i = 2; i < 13; i++)
		{
			MCF_PSC2_PSCTB_AC97 = 0x0;	//SLOT2-12:WR REG ALLES 0
		}
		
		// read register
		MCF_PSC2_PSCTB_AC97 = 0xc0000000;	//START SLOT1 + SLOT2, FIRST FRAME
		MCF_PSC2_PSCTB_AC97 = 0x82000000;	//SLOT1:master volume

		for (i = 2; i < 13; i++)
		{
			MCF_PSC2_PSCTB_AC97 = 0x00000000;	//SLOT2-12:RD REG ALLES 0
		}
		wait_50us();
		
		va = MCF_PSC2_PSCTB_AC97;
		if ((va & 0x80000fff) == 0x80000800) {
			vb = MCF_PSC2_PSCTB_AC97;
			vc = MCF_PSC2_PSCTB_AC97;

			/* FIXME: that looks more than suspicious (Fredi?) */
			if ((va & 0xE0000fff) == 0xE0000800 & vb == 0x02000000 & vc == 0x00000000) {
				goto livo;
			}
		}
	}
	uart_out_word(' NOT');
livo:
	// AUX VOLUME ->-0dB
	MCF_PSC2_PSCTB_AC97 = 0xE0000000;	//START SLOT1 + SLOT2, FIRST FRAME
	MCF_PSC2_PSCTB_AC97 = 0x16000000;	//SLOT1:WR REG AUX VOLUME adr 0x16
	MCF_PSC2_PSCTB_AC97 = 0x06060000;	//SLOT1:VOLUME
	for (i = 3; i < 13; i++) {
		MCF_PSC2_PSCTB_AC97 = 0x0;	//SLOT2-12:WR REG ALLES 0
	}

	// line in VOLUME +12dB
	MCF_PSC2_PSCTB_AC97 = 0xE0000000;	//START SLOT1 + SLOT2, FIRST FRAME
	MCF_PSC2_PSCTB_AC97 = 0x10000000;	//SLOT1:WR REG MASTER VOLUME adr 0x02
	for (i = 2; i < 13; i++) {
		MCF_PSC2_PSCTB_AC97 = 0x0;	//SLOT2-12:WR REG ALLES 0
	}
	// cd in VOLUME 0dB
	MCF_PSC2_PSCTB_AC97 = 0xE0000000;	//START SLOT1 + SLOT2, FIRST FRAME
	MCF_PSC2_PSCTB_AC97 = 0x12000000;	//SLOT1:WR REG MASTER VOLUME adr 0x02
	for (i = 2; i < 13; i++) {
		MCF_PSC2_PSCTB_AC97 = 0x0;	//SLOT2-12:WR REG ALLES 0
	}
	// mono out VOLUME 0dB
	MCF_PSC2_PSCTB_AC97 = 0xE0000000;	//START SLOT1 + SLOT2, FIRST FRAME
	MCF_PSC2_PSCTB_AC97 = 0x06000000;	//SLOT1:WR REG MASTER VOLUME adr 0x02
	MCF_PSC2_PSCTB_AC97 = 0x00000000;	//SLOT1:WR REG MASTER VOLUME adr 0x02
	for (i = 3; i < 13; i++) {
		MCF_PSC2_PSCTB_AC97 = 0x0;	//SLOT2-12:WR REG ALLES 0
	}
	MCF_PSC2_PSCTFCR |= MCF_PSC_PSCTFCR_WFR;	//set EOF
	MCF_PSC2_PSCTB_AC97 = 0x00000000;	//last data
	xprintf(" finished\r\n");
}

/* Symbols from the linker script */

extern uint8_t _FIRETOS[];
#define FIRETOS ((uint32_t)_FIRETOS) /* where FireTOS is stored in flash */

extern uint8_t _BAS_LMA[];
#define BAS_LMA ((uint32_t)_BAS_LMA) /* where the BaS is stored in flash */

extern uint8_t _BAS_VMA[];
#define BAS_VMA ((uint32_t)_BAS_VMA) /* where the BaS is run in RAM */

extern uint8_t _BAS_SIZE[];
#define BAS_SIZE ((uint32_t)_BAS_SIZE) /* size of the BaS, in bytes */

void initialize_hardware(void) {
	/* used in copy loop */
	uint32_t *src;	/* src address to read from flash */
	uint32_t *end;	/* end address to read from flash */
	uint32_t *dst;	/* destination address to copy to */
	uint32_t *jmp;	/* address of BaS() routine to jmp at after copy */

	/* Test for FireTOS switch: DIP switch #5 up */
	if (!(DIP_SWITCH & (1 << 6))) {
		/* Minimal hardware initialization */
		init_gpio();
		init_fbcs();
		init_ddram();
		init_fpga();

		/* FireTOS seems to have trouble to initialize the ST-RAM by itself, so... */
		/* Validate ST RAM */
		* (volatile uint32_t *) 0x42e = 0x00e00000;	/* phystop TOS system variable */
		* (volatile uint32_t *) 0x420 = 0x752019f3;	/* memvalid TOS system variable */
		* (volatile uint32_t *) 0x43a = 0x237698aa;	/* memval2 TOS system variable */
		* (volatile uint32_t *) 0x51a = 0x5555aaaa;	/* memval3 TOS system variable */

		/* Jump into FireTOS */
		typedef void void_func(void);
		void_func* FireTOS = (void_func*)FIRETOS;
		FireTOS(); // Should never return
		return;
	}

	init_gpio();
	init_serial();
	init_slt();
	init_fbcs();
	init_ddram();
	init_PCI();
	init_fpga();
	init_pll();
	init_video_ddr();
	dvi_on();
	test_upd720101();
	//video_1280_1024();
	init_ac97();

	/* copy the BaS code contained in flash to its final location */
	src = (uint32_t *)BAS_LMA;
	end = (uint32_t *)(BAS_LMA + BAS_SIZE);
	dst = jmp = (uint32_t *)BAS_VMA;

	/* FIXME: beware of possible alignment */
	do
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
	} while (src < end);

	/* we have copied a code area, so flush the caches */
	//flush_and_invalidate_caches();

	/* jump into the BaS in RAM */
	extern void BaS(void);
	BaS();
}
