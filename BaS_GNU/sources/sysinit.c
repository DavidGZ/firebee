/*
 * File:		sysinit.c
 * Purpose:		Power-on Reset configuration of the Firebee board.
 *
 * Notes: 
 *
 */
#include <stdint.h>

#include "MCF5475.h"
#include "startcf.h"


static const uint8_t *FPGA_FLASH_DATA = (uint8_t *) 0xe0700000L;
static const uint8_t *FPGA_FLASH_DATA_END = (uint8_t *) 0xe0800000L;

extern unsigned long _VRAM;
extern unsigned long _Bas_base;
extern unsigned long BaS;
extern unsigned long _BOOT_FLASH[];
extern int copy_end();
extern int wait_10us();
extern int wait_1ms();
extern int wait_10ms();
extern int wait_50us();

extern unsigned long rt_cacr;

#define uart_out_word(a)	MCF_PSC0_PSCTB_8BIT = (a);

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
	uart_out_word('SLT ');
	uart_out_word('OK! ');

	MCF_SLT0_STCNT = 0xffffffff;
	MCF_SLT0_SCR = 0x05;

	uart_out_word(0x0a0d);
}

/*
 * init GPIO ETC.
 */
void init_gpio(void)
{
	/*
	 * pad register P.S.:FBCTL and FBCS set correctly at reset
	 */
	MCF_PAD_PAR_DMA = 0b11111111;	/* NORMAL ALS DREQ DACK */
	MCF_PAD_PAR_FECI2CIRQ = 0b1111001111001111;	/* FEC0 NORMAL, FEC1 ALS I/O, I2C, #INT5..6 */
	MCF_PAD_PAR_PCIBG = 0b0000001000111111;	/* #PCI_BG4=#TBST,#PIC_BG3=I/O,#PCI_BG2..0=NORMAL */
	MCF_PAD_PAR_PCIBR = 0b0000001000111111;	/* #PCI_BR4=#INT4,#PIC_BR3=INPUT,#PCI_BR2..0=NORMAL */
	MCF_PAD_PAR_PSC3 = 0b00001100;	/* PSC3=TX,RX CTS+RTS=I/O */
	MCF_PAD_PAR_PSC1 = 0b11111100;	/* PSC1 NORMAL SERIELL */
	MCF_PAD_PAR_PSC0 = 0b11111100;	/* PSC0 NORMAL SERIELL */
	MCF_PAD_PAR_DSPI = 0b0001111111111111;	/* DSPI NORMAL */
	MCF_PAD_PAR_TIMER = 0b00101101;	/* TIN3..2=#IRQ3..2;TOUT3..2=NORMAL */
// ALLE OUTPUTS NORMAL LOW

// ALLE DIR NORMAL INPUT = 0
	MCF_GPIO_PDDR_FEC1L = 0b00011110;	/* OUT: 4=LED,3=PRG_DQ0,2=#FPGA_CONFIG,1=PRG_CLK(FPGA) */
}

/********************************************************************/
// init serial
/********************************************************************/

void init_serial(void)
{
	/* PSC0: SER1 */
	MCF_PSC0_PSCSICR = 0;	// UART
	MCF_PSC0_PSCCSR = 0xDD;
	MCF_PSC0_PSCCTUR = 0x00;
	MCF_PSC0_PSCCTLR = 36;	// BAUD RATE = 115200
	MCF_PSC0_PSCCR = 0x20;
	MCF_PSC0_PSCCR = 0x30;
	MCF_PSC0_PSCCR = 0x40;
	MCF_PSC0_PSCCR = 0x50;
	MCF_PSC0_PSCCR = 0x10;
	MCF_PSC0_PSCIMR = 0x8700;
	MCF_PSC0_PSCACR = 0x03;
	MCF_PSC0_PSCMR1 = 0xb3;
	MCF_PSC0_PSCMR2 = 0x07;
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

	uart_out_word('SERI');
	uart_out_word('AL O');
	uart_out_word('K!  ');
	uart_out_word(0x0a0d);
}

/********************************************************************/
/* Initialize DDR DIMMs on the EVB board */
/********************************************************************/
/*
 * Check to see if the SDRAM has already been initialized
 * by a run control tool
 */
void init_ddram(void)
{
	MCF_PSC0_PSCTB_8BIT = 'DDRA';
	if (!(MCF_SDRAMC_SDCR & MCF_SDRAMC_SDCR_REF)) {
		/* Basic configuration and initialization */
		MCF_SDRAMC_SDRAMDS = 0x000002AA;	// SDRAMDS configuration
		MCF_SDRAMC_CS0CFG = 0x0000001A;	// SDRAM CS0 configuration (128Mbytes 0000_0000 - 07FF_FFFF)
		MCF_SDRAMC_CS1CFG = 0x0800001A;	// SDRAM CS1 configuration (128Mbytes 0800_0000 - 0FFF_FFFF)
		MCF_SDRAMC_CS2CFG = 0x1000001A;	// SDRAM CS2 configuration (128Mbytes 1000_0000 - 07FF_FFFF)
		MCF_SDRAMC_CS3CFG = 0x1800001A;	// SDRAM CS3 configuration (128Mbytes 1800_0000 - 1FFF_FFFF)
//      MCF_SDRAMC_SDCFG1       = 0x53722938;   // SDCFG1
		MCF_SDRAMC_SDCFG1 = 0x73622830;	// SDCFG1
//      MCF_SDRAMC_SDCFG2       = 0x24330000;   // SDCFG2
		MCF_SDRAMC_SDCFG2 = 0x46770000;	// SDCFG2
//      MCF_SDRAMC_SDCR         = 0xE10F0002;   // SDCR + IPALL
		MCF_SDRAMC_SDCR = 0xE10D0002;	// SDCR + IPALL
		MCF_SDRAMC_SDMR = 0x40010000;	// SDMR (write to LEMR)
//      MCF_SDRAMC_SDMR         = 0x05890000;   // SDRM (write to LMR)
		MCF_SDRAMC_SDMR = 0x048D0000;	// SDRM (write to LMR)
//      MCF_SDRAMC_SDCR         = 0xE10F0002;   // SDCR + IPALL
		MCF_SDRAMC_SDCR = 0xE10D0002;	// SDCR + IPALL
//      MCF_SDRAMC_SDCR         = 0xE10F0004;   // SDCR + IREF (first refresh)
		MCF_SDRAMC_SDCR = 0xE10D0004;	// SDCR + IREF (first refresh)
//      MCF_SDRAMC_SDCR         = 0xE10F0004;   // SDCR + IREF (second refresh)
		MCF_SDRAMC_SDCR = 0xE10D0004;	// SDCR + IREF (second refresh)
///     MCF_SDRAMC_SDMR         = 0x01890000;   // SDMR (write to LMR)
		MCF_SDRAMC_SDMR = 0x008D0000;	// SDMR (write to LMR)
//      MCF_SDRAMC_SDCR         = 0x710F0F00;   // SDCR (lock SDMR and enable refresh)
		MCF_SDRAMC_SDCR = 0x710D0F00;	// SDCR (lock SDMR and enable refresh)
	}
	MCF_PSC0_PSCTB_8BIT = 'M OK';
	MCF_PSC0_PSCTB_8BIT = '!   ';
	MCF_PSC0_PSCTB_8BIT = 0x0a0d;
}

/*
 * init FB_CSx
 */
void init_fbcs()
{
	MCF_PSC0_PSCTB_8BIT = 'FBCS';

	/* Flash */
	MCF_FBCS0_CSAR = 0xE0000000;	// FLASH ADRESS
	MCF_FBCS0_CSCR = 0x00001180;	// 16 bit 4ws aa
	MCF_FBCS0_CSMR = 0x007F0001;	// 8MB on 

	MCF_FBCS1_CSAR = 0xFFF00000;	// ATARI I/O ADRESS
	MCF_FBCS1_CSCR = MCF_FBCS_CSCR_PS_16	// 16BIT PORT 
	    | MCF_FBCS_CSCR_WS(8)	// DEFAULT 8WS
	    | MCF_FBCS_CSCR_AA;	// AA 
	MCF_FBCS1_CSMR = (MCF_FBCS_CSMR_BAM_1M | MCF_FBCS_CSMR_V);

	MCF_FBCS2_CSAR = 0xF0000000;	// NEUER I/O ADRESS-BEREICH
	MCF_FBCS2_CSCR = MCF_FBCS_CSCR_PS_32	// 32BIT PORT
	    | MCF_FBCS_CSCR_WS(8)	// DEFAULT 4WS
	    | MCF_FBCS_CSCR_AA;	// AA 
	MCF_FBCS2_CSMR = (MCF_FBCS_CSMR_BAM_128M	// F000'0000-F7FF'FFFF
			  | MCF_FBCS_CSMR_V);

	MCF_FBCS3_CSAR = 0xF8000000;	// NEUER I/O ADRESS-BEREICH
	MCF_FBCS3_CSCR = MCF_FBCS_CSCR_PS_16	// 16BIT PORT
	    | MCF_FBCS_CSCR_AA;	// AA 
	MCF_FBCS3_CSMR = (MCF_FBCS_CSMR_BAM_64M	// F800'0000-FBFF'FFFF
			  | MCF_FBCS_CSMR_V);

	MCF_FBCS4_CSAR = 0x40000000;	// VIDEO RAM BEREICH, #FB_CS3 WIRD NICHT BENÜTZT, DECODE DIREKT AUF DEM FPGA
	MCF_FBCS4_CSCR = MCF_FBCS_CSCR_PS_32	// 32BIT PORT
	    | MCF_FBCS_CSCR_BSTR	// BURST READ ENABLE
	    | MCF_FBCS_CSCR_BSTW;	// BURST WRITE ENABLE
	MCF_FBCS4_CSMR = (MCF_FBCS_CSMR_BAM_1G	// 4000'0000-7FFF'FFFF
			  | MCF_FBCS_CSMR_V);

	MCF_PSC0_PSCTB_8BIT = ' OK!';
	MCF_PSC0_PSCTB_8BIT = 0x0a0d;
}

/*
 * load FPGA
 */
void init_fpga(void)
{
	register uint8_t *fpga_data;
	register int i;

	uart_out_word('FPGA');

	MCF_GPIO_PODR_FEC1L |= (1 << 1);
	MCF_GPIO_PODR_FEC1L |= (1 << 2);

	while ((!MCF_GPIO_PPDSDR_FEC1L & (1 << 0)) && (!MCF_GPIO_PPDSDR_FEC1L & (1 << 5)));

	wait_10us();
	MCF_GPIO_PODR_FEC1L |= (1 << 2);
	wait_10us();

	while (!MCF_GPIO_PPDSDR_FEC1L & (1 << 0))
	{
		warte10us();
	}
	
	/*
	 * excerpt from an Altera configuration manual:
	 * The low-to-high transition of nCONFIG on the FPGA begins the configuration cycle. The
	 * configuration cycle consists of 3 stages�reset, configuration, and initialization.
	 * While nCONFIG is low, the device is in reset. When the device comes out of reset,
	 * nCONFIG must be at a logic high level in order for the device to release the open-drain
 	 * nSTATUS pin. After nSTATUS is released, it is pulled high by a pull-up resistor and the FPGA
	 * is ready to receive configuration data. Before and during configuration, all user I/O pins
	 * are tri-stated. Stratix series, Arria series, and Cyclone series have weak pull-up resistors
	 * on the I/O pins which are on, before and during configuration.
	 *
	 * To begin configuration, nCONFIG and nSTATUS must be at a logic high level. You can delay
	 * configuration by holding the nCONFIG low. The device receives configuration data on its
	 * DATA0 pins. Configuration data is latched into the FPGA on the rising edge of DCLK. After
	 * the FPGA has received all configuration data successfully, it releases the CONF_DONE pin,
	 * which is pulled high by a pull-up resistor. A low to high transition on CONF_DONE indicates
	 * configuration is complete and initialization of the device can begin.
	 */
	fpga_data = (uint8_t *) FPGA_FLASH_DATA;
	do
	{
		uint8_t value = *fpga_data++;
		for (i = 0; i < 8; i++)
		{

			if ((value << i) & 0b10000000)
			{
				/* bit set -> toggle DATA0 to high */
				MCF_GPIO_PODR_FEC1L |= (1 << 3);
			}
			else
			{
				/* bit is cleared -> toggle DATA0 to low */
				MCF_GPIO_PODR_FEC1L &= ~(1 << 3);
			}
			/* toggle DCLK -> FPGA reads the bit */
			MCF_GPIO_PODR_FEC1L |= 1;
			MCF_GPIO_PODR_FEC1L &= ~1;
		}
	} while ((!MCF_GPIO_PPDSDR_FEC1L & (1 << 5)) && (fpga_data < FPGA_FLASH_DATA_END));

	if (fpga_data < FPGA_FLASH_DATA_END)
	{
		for (i = 0; i < 4000; i++)
		{
			/* toggle a little more since it's fun ;) */
			MCF_GPIO_PODR_FEC1L |= 1;
			MCF_GPIO_PODR_FEC1L &= ~1;
		}
	}
	else
	{
		MCF_PSC0_PSCTB_8BIT = ' NOT';
	}
	MCF_PSC0_PSCTB_8BIT = 'OK! ';
	MCF_PSC0_PSCTB_8BIT = 0x0d0a;
}

void wait_pll(void)
{
	do {
		wait1ms();
	} while (! * (uint16_t *) 0xf0000800);
}

void init_pll(void)
{
}

#ifdef _NOT_USED_
// init pll             
	       MCF_PSC0_PSCTB_8BIT = 'PLL '; asm {
	       lea 0xf0000600, a0 lea 0xf0000800, a1 bsr wait_pll move.w
#27,0x48(a0)			// loopfilter r
	       bsr wait_pll move.w
#1,0x08(a0)			// charge pump I
	       bsr wait_pll move.w
#12,0x0(a0)			// N counter high = 12
	       bsr wait_pll move.w
#12,0x40(a0)			// N counter low = 12
	       bsr wait_pll move.w
#1,0x114(a0)			// ck1 bypass
	       bsr wait_pll move.w
#1,0x118(a0)			// ck2 bypass
	       bsr wait_pll move.w
#1,0x11c(a0)			// ck3 bypass
	       bsr wait_pll move.w
#1,0x10(a0)			// ck0 high = 1
	       bsr wait_pll move.w
#1,0x50(a0)			// ck0 low      = 1
	       bsr wait_pll move.w
#1,0x144(a0)			// M odd division
	       bsr wait_pll move.w
#1,0x44(a0)			// M low = 1
	       bsr wait_pll move.w
#145,0x04(a0)			// M high = 145 = 146MHz
	       bsr wait_pll clr.b(a1)	// set
	       }
	       MCF_PSC0_PSCTB_8BIT = 'SET!'; MCF_PSC0_PSCTB_8BIT = 0x0a0d;}


/*
 * INIT VIDEO DDR RAM
 */

void init_video_ddr(void) {
	asm {

// init video ram
	       moveq.l #0xB,d0
	       move.w d0, 0xF0000400	//set cke=1, cs=1 config=1
	       nop lea __VRAM, a0	//zeiger auf video ram
	       nop move.l #0x00050400,(a0)		//IPALL
	       nop move.l #0x00072000,(a0)		//load EMR pll on
	       nop move.l #0x00070122,(a0)		//load MR: reset pll, cl=2 BURST=4lw
	       nop move.l #0x00050400,(a0)		//IPALL
	       nop move.l #0x00060000,(a0)		//auto refresh
	       nop move.l #0x00060000,(a0)		//auto refresh
	       nop move.l #0000070022,(a0)		//load MR dll on
	       nop move.l #0x01070002,d0			// fifo on, refresh on, ddrcs und cke on, video dac on,
	       move.l d0, 0xf0000400}
	       }

/********************************************************************/
	/* video mit auflösung 1280x1000 137MHz /*
	   /******************************************************************* */

void video_1280_1024(void) {
	extern int wait_pll;

	asm {
		// SPEICHER FÜLLEM
		//testmuster 1
		lea __VRAM, a2
		lea __VRAM + 0x600000,a3
		clr.l d0
		move.l #0x1000102,d1
 loop5:
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 move.l d0, (a2) +
 	 	 add.l d1, d0
flo6:
		cmp.l a2, a3
		bgt loop5
// screen setzen
//horizontal 1280
	       lea 0xffff8282, a0 move.w
#1800,(a0)+
	       move.w
#1380,(a0)+
	       move.w
#99,(a0)+
	       move.w
#100,(a0)+
	       move.w
#1379,(a0)+
	       move.w
#1500,(a0)
//vertical 1024
	       lea 0xffff82a2, a0 move.w
#1150,(a0)+
	       move.w
#1074,(a0)+
	       move.w
#49,(a0)+
	       move.w
#50,(a0)+
	       move.w
#1073,(a0)+
	       move.w
#1100,(a0)+
// acp video on         
	       move.l
#0x01070207,d0
	       move.l d0, 0xf0000400
// clut setzen
	       lea 0xf0000000, a0 move.l
#0xffffffff,(a0)+
	       move.l
#0xff,(a0)+
	       move.l
#0xff00,(a0)+
	       move.l
#0xff0000,(a0)
//              halt
	       }

	       }

#endif

#define	 PCI_MEMORY_OFFSET	(0x80000000)
#define	 PCI_MEMORY_SIZE	(0x40000000)
#define	 PCI_IO_OFFSET		(0xD0000000)
#define	 PCI_IO_SIZE		(0x10000000)

/*
 * INIT PCI
 */
void init_PCI(void) {
	MCF_PSC0_PSCTB_8BIT = 'PCI ';

	MCF_PCIARB_PACR = MCF_PCIARB_PACR_INTMPRI
		 + MCF_PCIARB_PACR_EXTMPRI(0x1F)
		 + MCF_PCIARB_PACR_INTMINTEN
		 + MCF_PCIARB_PACR_EXTMINTEN(0x1F);

	// Setup burst parameters
	MCF_PCI_PCICR1 = MCF_PCICR1_CACHELINESIZE(4) + MCF_PCI_PCICR1_LATTIMER(32);
	MCF_PCI_PCICR2 = MCF_PCICR2_MINGNT(16) + MCF_PCI_PCICR2_MAXLAT(16);

	// Turn on error signaling
	MCF_PCI_PCIICR = MCF_PCI_PCIICR_TAE + MCF_PCI_PCIICR_TAE * MCF_PCI_PCIICR_REE + 32;
	MCF_PCI_PCIGSCR |= MCF_PCI_PCIGSCR_SEE;

	/* Configure Initiator Windows */
	/* initiator window 0 base / translation adress register */
	MCF_PCI_PCIIW0BTAR = PCI_MEMORY_OFFSET + ((PCI_MEMORY_SIZE -1) >> 8) & 0xffff0000;

	/* initiator window 1 base / translation adress register */
	MCF_PCI_PCIIW1BTAR = PCI_IO_OFFSET + ((PCI_IO_SIZE - 1) >> 8) & 0xffff0000;

	/* initiator window 2 base / translation address register */
	MCF_PCI_PCIIW2BTAR = 0L;	/* not used */

	/* initiator window configuration register */
	MCF_PCI_PCIIWCR = MCF_PCI_PCIIWCR_WINCTRL0_MEMRDLINE + MCF_PCI_PCIIWCR_WINCTRL1_IO;

	/* reset PCI devices */
	MCF_PCI_PCIGSCR &= ~MCF_PCI_PCIGSCR_PR;

	MCF_PSC0_PSCTB_8BIT = 'OK! ';
	MCF_PSC0_PSCTB_8BIT = 0x0d0a;
}
	

/*
 * test UPC720101 (USB) 
 */

void test_upd720101(void) 
{
	MCF_PSC0_PSCTB_8BIT = 'NEC ';

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
	MCF_PSC0_PSCTB_8BIT = 'OK! ';
	MCF_PSC0_PSCTB_8BIT = 0x0d0a;
}

/*
 * TFP410 (vdi) einschalten /*
 */
void vdi_on(void) {
	uint8_t RBYT;
	uint8_t DBYT;
	int versuche;
	int startzeit;
	
	MCF_PSC0_PSCTB_8BIT = 'DVI ';

	MCF_I2C_I2FDR = 0x3c;	// 100kHz standard
	versuche = 0;

loop_i2c:
	if (versuche++ > 10)
		goto next;
	
	MCF_I2C_I2ICR = 0x0;
	MCF_I2C_I2CR = 0x0;
	MCF_I2C_I2CR = 0xA;
	RBYT = MCF_I2C_I2DR;
	MCF_I2C_I2SR = 0x0;
	MCF_I2C_I2CR = 0x0;
	MCF_I2C_I2ICR = 0x01;
	MCF_I2C_I2CR = 0xb0;
	MCF_I2C_I2DR = 0x7a;	// ADRESSE TFP410

	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));	// warten auf fertig

	MCF_I2C_I2SR &= 0xfd;	// clear bit
	if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK)
		goto loop_i2c;	// ack erhalten? -> nein

tpf_410_ACK_OK:
	MCF_I2C_I2DR = 0x00;	// SUB ADRESS 0
	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));
	
	MCF_I2C_I2SR &= 0xfd;
	MCF_I2C_I2CR |= 0x4;	// repeat start
	MCF_I2C_I2DR = 0x7b;	// beginn read
	
	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));	// warten auf fertig

	MCF_I2C_I2SR &= 0xfd;	// clear bit

   if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK)
		goto loop_i2c;	// ack erhalten? -> nein

	MCF_I2C_I2CR &= 0xef;	// switch to rx 
	DBYT = MCF_I2C_I2DR;	// dummy read

	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));
	
	MCF_I2C_I2SR &= 0xfd;
	
	MCF_I2C_I2CR |= 0x08;	// txak=1
	RBYT = MCF_I2C_I2DR;
	
	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));
	
	MCF_I2C_I2SR &= 0xfd;
	MCF_I2C_I2CR = 0x80;	// stop

	DBYT = MCF_I2C_I2DR;	// dummy read

	if (RBYT != 0x4c)
		goto loop_i2c;

i2c_ok:
	MCF_I2C_I2CR = 0x0;	// stop
	MCF_I2C_I2SR = 0x0;	// clear sr

	while ((MCF_I2C_I2SR & MCF_I2C_I2SR_IBB));	// wait auf bus free

	MCF_I2C_I2CR = 0xb0;	// on tx master 
	MCF_I2C_I2DR = 0x7A;
	
	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));	// warten auf fertig
	
	MCF_I2C_I2SR &= 0xfd;	// clear bit

	if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK)
		goto loop_i2c;	// ack erhalten? -> nein
	  
	MCF_I2C_I2DR = 0x08;	// SUB ADRESS 8

	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));
	
	MCF_I2C_I2SR &= 0xfd;
	MCF_I2C_I2DR = 0xbf;	// ctl1: power on, T:M:D:S: enable

	MCF_I2C_I2CR = 0x80;	// stop
	DBYT = MCF_I2C_I2DR;	// dummy read
	MCF_I2C_I2SR = 0x0;	// clear sr

	while ((MCF_I2C_I2SR & MCF_I2C_I2SR_IBB));	// wait auf bus free

	MCF_I2C_I2CR = 0xb0;
	MCF_I2C_I2DR = 0x7A;
	
	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));	// warten auf fertig

	MCF_I2C_I2SR &= 0xfd;	// clear bit

	if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK)
		goto loop_i2c;	// ack erhalten? -> nein

	MCF_I2C_I2DR = 0x08;	// SUB ADRESS 8

	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));
	
	MCF_I2C_I2SR &= 0xfd;
	MCF_I2C_I2CR |= 0x4;	// repeat start
	MCF_I2C_I2DR = 0x7b;	// beginn read

	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));	// warten auf fertig

	MCF_I2C_I2SR &= 0xfd;	// clear bit

	if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK)
		goto loop_i2c;	// ack erhalten? -> nein
	  
	MCF_I2C_I2CR &= 0xef;	// switch to rx 
	DBYT = MCF_I2C_I2DR;	// dummy read

	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));
	
	MCF_I2C_I2SR &= 0xfd;
	MCF_I2C_I2CR |= 0x08;	// txak=1

	wait_50us();
	
	RBYT = MCF_I2C_I2DR;
	
	while (!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF));
	
	MCF_I2C_I2SR &= 0xfd;
	MCF_I2C_I2CR = 0x80;	// stop

	DBYT = MCF_I2C_I2DR;	// dummy read

	if (RBYT != 0xbf)
		goto loop_i2c;
	goto dvi_ok;
next:
	MCF_PSC0_PSCTB_8BIT = 'NOT ';
dvi_ok:
	MCF_PSC0_PSCTB_8BIT = 'OK! ';
	MCF_PSC0_PSCTB_8BIT = 0x0a0d;
	MCF_I2C_I2CR = 0x0;	// i2c off
}


/*
 * AC97
 */
void init_ac97(void) {
	// PSC2: AC97 ----------
	int i;
	int k;
	int zm;
	int x;
	int va;
	int vb;
	int vc;
	
	MCF_PSC0_PSCTB_8BIT = 'AC97';
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
			if ((va & 0xE0000fff) == 0xE0000800 & vb == 0x02000000 & vc == 0x00000000) {
				goto livo;}
			}
		}
		MCF_PSC0_PSCTB_8BIT = ' NOT';
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
ac97_end:
		MCF_PSC0_PSCTB_8BIT = ' OK!'; MCF_PSC0_PSCTB_8BIT = 0x0a0d;
}

void __initialize_hardware(void) {
_init_hardware:
asm(
	"move.l 	#0x000C8120,D0\n\t"
	"move.l	D0,rt_cacr\n\t"
	"movec		D0,CACR\n\t"
	"nop\n\t"
	);

	init_gpio();
	init_serial();
	init_slt();
	init_fbcs();
	init_ddram();

	/* do not initialize ports if DIP switch 5 = on */
	if (DIP_SWITCH & (1 << 6))
		init_PCI();

	init_fpga();
	init_video_ddr();
	vdi_on();

	/* do not initialize ports if DIP switch 5 = on */
	if (DIP_SWITCH & (1 << 6)) {
		test_upd720101();
		/* video_1280_1024(); */
		init_ac97();
	}

	asm volatile(
		"lea		copy_start,A0\n\t"
		"lea		BaS,A1\n\t"
		"sub.l		A0,A1\n\t"
		"move.l	#__Bas_base,A2\n\t"
		"move.l	A2,A3\n\t"
		"add.l		A1,A3\n\t"
		"lea		copy_end,A4\n\t"
"BaS_copy_loop:	/* copy 16 bytes per turn */\n\t"
		"move.l	(A0)+,(A2)+\n\t"
		"move.l	(A0)+,(A2)+\n\t"
		"move.l	(A0)+,(A2)+\n\t"
		"move.l	(A0)+,(A2)+\n\t"
		"cmp.l		A4,A0\n\t"
		"blt		BaS_copy_loop\n\t"
"\n\t"
		"intouch	A3\n\t"	/* we'd better update caches to contain the data we just copied */
		"jmp		(A3)\n\t"
"copy_start:\n\t"
		"nop\n\t"
		 : :);
}
