/*
 * File:		sysinit.c
 * Purpose:		Power-on Reset configuration of the COLDARI board.
 *
 * Notes: 
 *
 */
#include "MCF5475.h"
#include "startcf.h"

extern unsigned long far __VRAM;
extern unsigned long far __Bas_base;
extern unsigned long far BaS;
extern unsigned long far __BOOT_FLASH[]; 
extern int copy_end();
extern int warte_10us();
extern int warte_1ms();
extern int warte_10ms();
extern int warte_50us();

extern unsigned long far rt_cacr;

/********************************************************************/
// init SLICE TIMER 0 
// all  = 32.538 sec = 30.736mHz
// BYT0 = 127.1ms/tick = 7.876Hz   offset 0
// BYT1 = 496.5us/tick = 2.014kHz  offset 1
// BYT2 = 1.939us/tick = 515.6kHz  offset 2
// BYT3 = 7.576ns/tick = 132.00MHz offset 3
// count down!!! 132MHz!!!
/********************************************************************/

void init_slt(void)
{
	asm
	{
		lea 		MCF_SLT0_STCNT,a0
		move.l		#0xffffffff,(a0)
		lea 		MCF_SLT0_SCR,a0
		move.b		#0x05,(a0)
			
	}
		MCF_PSC0_PSCTB_8BIT = 'SLT ';
		MCF_PSC0_PSCTB_8BIT = 'OK! ';
		MCF_PSC0_PSCTB_8BIT = 0x0a0d;
}

/********************************************************************/
// init GPIO ETC.
/********************************************************************/

void init_gpio(void)
{


// PAD REGISTER P.S.:FBCTL UND FBCS WERDEN RICHTIG GESETZT BEIM RESET 
	MCF_PAD_PAR_DMA 		= 0b11111111;			// NORMAL ALS DREQ DACK
	MCF_PAD_PAR_FECI2CIRQ 	= 0b1111001111001111;	// FEC0 NORMAL, FEC1 ALS I/O, I2C, #INT5..6
	MCF_PAD_PAR_PCIBG 		= 0b0000001000111111;	// #PCI_BG4=#TBST,#PIC_BG3=I/O,#PCI_BG2..0=NORMAL
	MCF_PAD_PAR_PCIBR 		= 0b0000001000111111;	// #PCI_BR4=#INT4,#PIC_BR3=INPUT,#PCI_BR2..0=NORMAL
	MCF_PAD_PAR_PSC3		= 0b00001100;			// PSC3=TX,RX CTS+RTS=I/O
	MCF_PAD_PAR_PSC1		= 0b11111100;			// PSC1 NORMAL SERIELL
	MCF_PAD_PAR_PSC0		= 0b11111100;			// PSC0 NORMAL SERIELL
	MCF_PAD_PAR_DSPI		= 0b0001111111111111;	// DSPI NORMAL
	MCF_PAD_PAR_TIMER		= 0b00101101;			// TIN3..2=#IRQ3..2;TOUT3..2=NORMAL
// ALLE OUTPUTS NORMAL LOW

// ALLE DIR NORMAL INPUT = 0
	MCF_GPIO_PDDR_FEC1L		= 0b00011110;			// OUT: 4=LED,3=PRG_DQ0,2=#FPGA_CONFIG,1=PRG_CLK(FPGA)	

}

/********************************************************************/
// init seriel
/********************************************************************/

void init_seriel(void)
{
		
// PSC0: SER1 ----------
 		MCF_PSC0_PSCSICR = 0;		// UART
		MCF_PSC0_PSCCSR = 0xDD;
		MCF_PSC0_PSCCTUR = 0x00;	 
		MCF_PSC0_PSCCTLR = 36;		// BAUD RATE = 115200
		MCF_PSC0_PSCCR = 0x20;
		MCF_PSC0_PSCCR = 0x30;
		MCF_PSC0_PSCCR = 0x40;
		MCF_PSC0_PSCCR = 0x50;
		MCF_PSC0_PSCCR = 0x10;
		MCF_PSC0_PSCIMR = 0x8700;
		MCF_PSC0_PSCACR = 0x03;
		MCF_PSC0_PSCMR1= 0xb3;
		MCF_PSC0_PSCMR2= 0x07;
		MCF_PSC0_PSCRFCR = 0x0F;
		MCF_PSC0_PSCTFCR = 0x0F;
		MCF_PSC0_PSCRFAR = 0x00F0;
		MCF_PSC0_PSCTFAR = 0x00F0;
		MCF_PSC0_PSCOPSET = 0x01;
		MCF_PSC0_PSCCR = 0x05;
// PSC3: PIC ----------
 		MCF_PSC3_PSCSICR = 0;		// UART
		MCF_PSC3_PSCCSR = 0xDD;
		MCF_PSC3_PSCCTUR = 0x00;	 
		MCF_PSC3_PSCCTLR = 36;		// BAUD RATE = 115200
		MCF_PSC3_PSCCR = 0x20;
		MCF_PSC3_PSCCR = 0x30;
		MCF_PSC3_PSCCR = 0x40;
		MCF_PSC3_PSCCR = 0x50;
		MCF_PSC3_PSCCR = 0x10;
		MCF_PSC3_PSCIMR = 0x0200;	// receiver interrupt enable
		MCF_PSC3_PSCACR = 0x03;
		MCF_PSC3_PSCMR1= 0xb3;
		MCF_PSC3_PSCMR2= 0x07;
		MCF_PSC3_PSCRFCR = 0x0F;
		MCF_PSC3_PSCTFCR = 0x0F;
		MCF_PSC3_PSCRFAR = 0x00F0;
		MCF_PSC3_PSCTFAR = 0x00F0;
		MCF_PSC3_PSCOPSET = 0x01;
		MCF_PSC3_PSCCR = 0x05;
		MCF_INTC_ICR32 = 0x3F;		//MAXIMALE PRIORITY/**********/

		MCF_PSC0_PSCTB_8BIT = 0x0a0d;
		MCF_PSC0_PSCTB_8BIT = 'SERI';
		MCF_PSC0_PSCTB_8BIT = 'AL O';
		MCF_PSC0_PSCTB_8BIT = 'K!  ';
		MCF_PSC0_PSCTB_8BIT = 0x0a0d;	
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
  if (!(MCF_SDRAMC_SDCR & MCF_SDRAMC_SDCR_REF))
   {

	/* Basic configuration and initialization */
	MCF_SDRAMC_SDRAMDS 	= 0x000002AA;	// SDRAMDS configuration
	MCF_SDRAMC_CS0CFG 	= 0x0000001A;   // SDRAM CS0 configuration (128Mbytes 0000_0000 - 07FF_FFFF)
	MCF_SDRAMC_CS1CFG 	= 0x0800001A;   // SDRAM CS1 configuration (128Mbytes 0800_0000 - 0FFF_FFFF)
	MCF_SDRAMC_CS2CFG 	= 0x1000001A;   // SDRAM CS2 configuration (128Mbytes 1000_0000 - 07FF_FFFF)
	MCF_SDRAMC_CS3CFG 	= 0x1800001A;   // SDRAM CS3 configuration (128Mbytes 1800_0000 - 1FFF_FFFF)
//  	MCF_SDRAMC_SDCFG1 	= 0x53722938;   // SDCFG1
	MCF_SDRAMC_SDCFG1 	= 0x73622830;   // SDCFG1
//  	MCF_SDRAMC_SDCFG2 	= 0x24330000;   // SDCFG2
	MCF_SDRAMC_SDCFG2 	= 0x46770000;   // SDCFG2
//   	MCF_SDRAMC_SDCR 	= 0xE10F0002;   // SDCR + IPALL
   	MCF_SDRAMC_SDCR 	= 0xE10D0002;   // SDCR + IPALL
   	MCF_SDRAMC_SDMR 	= 0x40010000;   // SDMR (write to LEMR)
//   	MCF_SDRAMC_SDMR 	= 0x05890000;   // SDRM (write to LMR)
   	MCF_SDRAMC_SDMR 	= 0x048D0000;   // SDRM (write to LMR)
//   	MCF_SDRAMC_SDCR 	= 0xE10F0002;   // SDCR + IPALL
   	MCF_SDRAMC_SDCR 	= 0xE10D0002;   // SDCR + IPALL
//   	MCF_SDRAMC_SDCR 	= 0xE10F0004;   // SDCR + IREF (first refresh)
   	MCF_SDRAMC_SDCR 	= 0xE10D0004;   // SDCR + IREF (first refresh)
//   	MCF_SDRAMC_SDCR 	= 0xE10F0004;   // SDCR + IREF (second refresh)
  	MCF_SDRAMC_SDCR 	= 0xE10D0004;   // SDCR + IREF (second refresh)
///   	MCF_SDRAMC_SDMR 	= 0x01890000;   // SDMR (write to LMR)
   	MCF_SDRAMC_SDMR 	= 0x008D0000;   // SDMR (write to LMR)
//   	MCF_SDRAMC_SDCR 	= 0x710F0F00;   // SDCR (lock SDMR and enable refresh)
   	MCF_SDRAMC_SDCR 	= 0x710D0F00;   // SDCR (lock SDMR and enable refresh)
   }
		MCF_PSC0_PSCTB_8BIT = 'M OK';
		MCF_PSC0_PSCTB_8BIT = '!   ';
		MCF_PSC0_PSCTB_8BIT = 0x0a0d;
}
/********************************************************************/
	/* init FB_CSx /*
/********************************************************************/
void init_fbcs()
{
		MCF_PSC0_PSCTB_8BIT = 'FBCS';
		/* Flash */
 	MCF_FBCS0_CSAR 	= 0xE0000000;						// FLASH ADRESS
 	MCF_FBCS0_CSCR 	= 0x00001180;						// 16 bit 4ws aa
 	MCF_FBCS0_CSMR 	= 0x007F0001;					 	// 8MB on 
		
	MCF_FBCS1_CSAR	= 0xFFF00000;						// ATARI I/O ADRESS
	MCF_FBCS1_CSCR 	= MCF_FBCS_CSCR_PS_16 				// 16BIT PORT 
					| MCF_FBCS_CSCR_WS(8)				// DEFAULT 8WS
 					| MCF_FBCS_CSCR_AA;					// AA 
	MCF_FBCS1_CSMR 	= (MCF_FBCS_CSMR_BAM_1M 
					| MCF_FBCS_CSMR_V);

	MCF_FBCS2_CSAR	= 0xF0000000;						// NEUER I/O ADRESS-BEREICH
	MCF_FBCS2_CSCR 	= MCF_FBCS_CSCR_PS_32				// 32BIT PORT
					| MCF_FBCS_CSCR_WS(8)				// DEFAULT 4WS
					| MCF_FBCS_CSCR_AA;					// AA 
	MCF_FBCS2_CSMR 	= (MCF_FBCS_CSMR_BAM_128M 			// F000'0000-F7FF'FFFF
					| MCF_FBCS_CSMR_V);
 	
	MCF_FBCS3_CSAR	= 0xF8000000;						// NEUER I/O ADRESS-BEREICH
	MCF_FBCS3_CSCR 	= MCF_FBCS_CSCR_PS_16				// 16BIT PORT
					| MCF_FBCS_CSCR_AA;					// AA 
	MCF_FBCS3_CSMR 	= (MCF_FBCS_CSMR_BAM_64M 			// F800'0000-FBFF'FFFF
					| MCF_FBCS_CSMR_V);
 
	MCF_FBCS4_CSAR	= 0x40000000;						// VIDEO RAM BEREICH, #FB_CS3 WIRD NICHT BEN�TZT, DECODE DIREKT AUF DEM FPGA
	MCF_FBCS4_CSCR 	= MCF_FBCS_CSCR_PS_32				// 32BIT PORT
					| MCF_FBCS_CSCR_BSTR				// BURST READ ENABLE
					| MCF_FBCS_CSCR_BSTW;				// BURST WRITE ENABLE
	MCF_FBCS4_CSMR 	= (MCF_FBCS_CSMR_BAM_1G 			// 4000'0000-7FFF'FFFF
					| MCF_FBCS_CSMR_V);

		MCF_PSC0_PSCTB_8BIT = ' OK!';
		MCF_PSC0_PSCTB_8BIT = 0x0a0d;
}

/********************************************************************/
	/* FPGA LADEN /*
/********************************************************************/


void init_fpga(void)
{

		MCF_PSC0_PSCTB_8BIT = 'FPGA';
asm
	{
  		lea			MCF_GPIO_PODR_FEC1L,a1				// register adresse:write
  		lea			MCF_GPIO_PPDSDR_FEC1L,a2			// reads
  		bclr		#1,(a1)								// clk auf low
		bclr		#2,(a1)								// #config=low 
test_nSTATUS:
		btst		#0,(a2)								// nSTATUS==0
		bne			test_nSTATUS						// nein->
		btst		#5,(a2)								// conf done==0
		bne			test_nSTATUS						// nein->
		jsr			warte_10us							// warten
		bset		#2,(a1)								// #config=high
		jsr			warte_10us							// warten
test_STATUS:
		btst		#0,(a2)								// status high?
		beq			test_STATUS							// nein->
		jsr			warte_10us							// warten

  		lea			0xE0700000,a0						// startadresse fpga daten
word_send_loop:
		cmp.l		#0xE0800000,a0
		bgt			fpga_error
		move.b		(a0)+,d0							// 32 bit holen
		moveq		#8,d1								// 32 bit ausgeben
bit_send_loop:
		lsr.l		#1,d0								// bit rausschieben
		bcs			bit_is_1							
		bclr		#3,(a1)
		bra			bit_send
bit_is_1:
		bset		#3,(a1)
bit_send:
		bset		#1,(a1)								// clock=high
		bclr		#1,(a1)								// clock=low
		subq.l		#1,d1
		bne			bit_send_loop						// wiederholen bis fertig
		btst		#5,(a2)								// fpga fertig, conf_done=high?
		beq			word_send_loop						// nein, next word->
		move.l		#4000,d1
overclk:
		bset		#1,(a1)								// clock=high
		nop
		bclr		#1,(a1)								// clock=low
		subq.l		#1,d1
		bne			overclk								// weiter bis fertig
  		bra			init_fpga_end

//---------------------------------------------------------
wait_pll:
  		lea			MCF_SLT0_SCNT,a3
  		move.l		(a3),d0
  		move.l		#100000,d6			// ca 1ms
wait_pll_loop:
		tst.w		(a1)
		bpl			wait_pll_ok
		move.l		(a3),d1
		sub.l		d0,d1
		add.l		d6,d1
		bpl			wait_pll_loop
wait_pll_ok:
		rts
// fertig
fpga_error:
 	}
		MCF_PSC0_PSCTB_8BIT = ' NOT';
init_fpga_end:
		MCF_PSC0_PSCTB_8BIT = ' OK!';
		MCF_PSC0_PSCTB_8BIT = 0x0a0d;

// init pll		
		MCF_PSC0_PSCTB_8BIT = 'PLL ';
asm
{
		lea		0xf0000600,a0
		lea		0xf0000800,a1
		bsr		wait_pll
		move.w	#27,0x48(a0)		// loopfilter r
		bsr		wait_pll
		move.w	#1,0x08(a0)			// charge pump I
		bsr		wait_pll
		move.w	#12,0x0(a0)			// N counter high = 12
		bsr		wait_pll
		move.w	#12,0x40(a0)		// N counter low = 12
		bsr		wait_pll
		move.w	#1,0x114(a0)		// ck1 bypass
		bsr		wait_pll
		move.w	#1,0x118(a0)		// ck2 bypass
		bsr		wait_pll
		move.w	#1,0x11c(a0)		// ck3 bypass
		bsr		wait_pll
		move.w	#1,0x10(a0)			// ck0 high = 1
		bsr		wait_pll
		move.w	#1,0x50(a0)			// ck0 low	= 1
		
		bsr		wait_pll
		move.w	#1,0x144(a0)		// M odd division
		bsr		wait_pll
		move.w	#1,0x44(a0)			// M low = 1

		bsr		wait_pll
		move.w	#145,0x04(a0)		// M high = 145 = 146MHz

		bsr		wait_pll
		clr.b	(a1)				// set
}
		MCF_PSC0_PSCTB_8BIT = 'SET!';
		MCF_PSC0_PSCTB_8BIT = 0x0a0d;
}		

/********************************************************************/
	/* INIT VIDEO DDR RAM /*
/********************************************************************/

void init_video_ddr(void)
{
	asm
	{

// init video ram
		moveq.l	#0xB,d0
		move.w	d0,0xF0000400				//set cke=1, cs=1 config=1
		nop
		lea		__VRAM,a0					//zeiger auf video ram
		nop
		move.l	#0x00050400,(a0)			//IPALL
		nop
		move.l	#0x00072000,(a0)			//load EMR pll on
		nop
		move.l	#0x00070122,(a0)			//load MR: reset pll, cl=2 BURST=4lw
		nop
		move.l	#0x00050400,(a0)			//IPALL
		nop
		move.l	#0x00060000,(a0)			//auto refresh
		nop
		move.l	#0x00060000,(a0)			//auto refresh
		nop
		move.l	#0000070022,(a0)			//load MR dll on
		nop
		move.l	#0x01070002,d0				// fifo on, refresh on, ddrcs und cke on, video dac on,
		move.l	d0,0xf0000400
	}
}

/********************************************************************/
	/* video mit aufl�sung 1280x1000 137MHz /*
/********************************************************************/

void video_1280_1024(void)
{
extern int wait_pll;

	asm
	{


// SPEICHER F�LLEM

//testmuster 1
		lea		__VRAM,a2
		lea		__VRAM+0x600000,a3
		clr.l	d0
		move.l	#0x1000102,d1
loop5:	move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		move.l  d0,(a2)+
		add.l	d1,d0
flo6:	cmp.l	a2,a3
		bgt		loop5

// screen setzen
//horizontal 1280
		lea		0xffff8282,a0
		move.w	#1800,(a0)+
		move.w	#1380,(a0)+
		move.w	#99,(a0)+
		move.w	#100,(a0)+
		move.w	#1379,(a0)+
		move.w	#1500,(a0)
//vertical 1024
		lea		0xffff82a2,a0
		move.w	#1150,(a0)+
		move.w	#1074,(a0)+
		move.w	#49,(a0)+
		move.w	#50,(a0)+
		move.w	#1073,(a0)+
		move.w	#1100,(a0)+
// acp video on		
		move.l	#0x01070207,d0
		move.l	d0,0xf0000400
		
		
// clut setzen
		lea		0xf0000000,a0
		move.l	#0xffffffff,(a0)+
		move.l	#0xff,(a0)+
		move.l	#0xff00,(a0)+
		move.l	#0xff0000,(a0) 
		
//		halt
		
	}

}
/********************************************************************/
	/* INIT PCI /*
/********************************************************************/

#define	 PCI_MEMORY_OFFSET	(0x80000000)
#define	 PCI_MEMORY_SIZE	(0x40000000)
#define	 PCI_IO_OFFSET		(0xD0000000)
#define	 PCI_IO_SIZE		(0x10000000)


void init_PCI(void)
{

		MCF_PSC0_PSCTB_8BIT = 'PCI ';
asm
	{
 	// Setup the arbiter
		move.l #MCF_PCIARB_PACR_INTMPRI \
		 + MCF_PCIARB_PACR_EXTMPRI(0x1F) \
		 + MCF_PCIARB_PACR_INTMINTEN \
		 + MCF_PCIARB_PACR_EXTMINTEN(0x1F),D0
		move.l D0,MCF_PCIARB_PACR
	// Setup burst parameters
		move.l #MCF_PCI_PCICR1_CACHELINESIZE(4) + MCF_PCI_PCICR1_LATTIMER(32),D0
		move.l D0,MCF_PCI_PCICR1
		move.l #MCF_PCI_PCICR2_MINGNT(16) + MCF_PCI_PCICR2_MAXLAT(16),D0
		move.l D0,MCF_PCI_PCICR2
	// Turn on error signaling
		move.l #MCF_PCI_PCIICR_TAE + MCF_PCI_PCIICR_IAE + MCF_PCI_PCIICR_REE + 32,D0
		move.l D0,MCF_PCI_PCIICR
		move.l #MCF_PCI_PCIGSCR_SEE,D0
		or.l D0,MCF_PCI_PCIGSCR
	// Configure Initiator Windows */
		move.l #PCI_MEMORY_OFFSET + ((PCI_MEMORY_SIZE - 1) >> 8),D0
		clr.w D0
		move.l D0,MCF_PCI_PCIIW0BTAR // Initiator Window 0 Base / Translation Address Register
	
		move.l #PCI_IO_OFFSET+((PCI_IO_SIZE-1)>>8),D0
		clr.w D0
		move.l D0,MCF_PCI_PCIIW1BTAR // Initiator Window 1 Base / Translation Address Register

		clr.l MCF_PCI_PCIIW2BTAR     // not used
		
		move.l #MCF_PCI_PCIIWCR_WINCTRL0_MEMRDLINE + MCF_PCI_PCIIWCR_WINCTRL1_IO,D0
		move.l D0,MCF_PCI_PCIIWCR	   // Initiator Window Configuration Register

	/* Clear PCI Reset and wait for devices to reset */
		move.l #~MCF_PCI_PCIGSCR_PR,D0
		and.l D0,MCF_PCI_PCIGSCR
	}

		MCF_PSC0_PSCTB_8BIT = 'OK! ';
		MCF_PSC0_PSCTB_8BIT = 0x0a0d;
}
/********************************************************************/
	/* test UPC720101 (USB) /*
/********************************************************************/

void test_upd720101(void)
{

		MCF_PSC0_PSCTB_8BIT = 'NEC ';
asm
	{
	// SELECT UPD720101 AD17
		MOVE.L	#MCF_PCI_PCICAR_E+MCF_PCI_PCICAR_DEVNUM(17)+MCF_PCI_PCICAR_FUNCNUM(0)+MCF_PCI_PCICAR_DWORD(0),D0
		MOVE.L	D0,MCF_PCI_PCICAR
		LEA		PCI_IO_OFFSET,A0
		MOVE.L	(A0),D1	
		move.l	#0x33103500,d0
		cmp.l	d0,d1
		beq		nec_ok
	}
		MCF_PSC0_PSCTB_8BIT = 'NOT ';
		goto	nec_not_ok;
nec_ok:
	asm 
	{
		MOVE.L	#MCF_PCI_PCICAR_E+MCF_PCI_PCICAR_DEVNUM(17)+MCF_PCI_PCICAR_FUNCNUM(0)+MCF_PCI_PCICAR_DWORD(57),D0
		MOVE.L	D0,MCF_PCI_PCICAR
		move.b	#0x20,(a0)
	}
nec_not_ok:
	asm 
	{
		MOVE.L	#MCF_PCI_PCICAR_DEVNUM(17)+MCF_PCI_PCICAR_FUNCNUM(0)+MCF_PCI_PCICAR_DWORD(57),D0
		MOVE.L	D0,MCF_PCI_PCICAR
	}
		MCF_PSC0_PSCTB_8BIT = 'OK! ';
		MCF_PSC0_PSCTB_8BIT = 0x0a0d;
}

/********************************************************************/
	/* TFP410 (vdi) einschalten /*
/********************************************************************/

void vdi_on(void)
{
 		uint8  RBYT, DBYT;
 		int versuche, startzeit;

 		 
		MCF_PSC0_PSCTB_8BIT = 'DVI ';
		MCF_I2C_I2FDR = 0x3c;			// 100kHz standard
		versuche = 0;
loop_i2c:
		if (versuche++>10) goto next;
		MCF_I2C_I2ICR = 0x0;
		MCF_I2C_I2CR = 0x0;
		MCF_I2C_I2CR = 0xA;
		RBYT = MCF_I2C_I2DR;
		MCF_I2C_I2SR = 0x0;
		MCF_I2C_I2CR = 0x0;
		MCF_I2C_I2ICR = 0x01;
 
		MCF_I2C_I2CR = 0xb0;
		
		MCF_I2C_I2DR = 0x7a;								// ADRESSE TFP410
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;			// warten auf fertig
		MCF_I2C_I2SR &= 0xfd;								// clear bit

		if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK) goto loop_i2c; // ack erhalten? -> nein

tpf_410_ACK_OK:
		MCF_I2C_I2DR = 0x00;								// SUB ADRESS 0
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;
		MCF_I2C_I2SR &= 0xfd;

		MCF_I2C_I2CR |= 0x4;								// repeat start
	
		MCF_I2C_I2DR = 0x7b;								// beginn read
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;			// warten auf fertig
		MCF_I2C_I2SR &= 0xfd;								// clear bit

		if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK) goto loop_i2c; // ack erhalten? -> nein


		MCF_I2C_I2CR &= 0xef;								// switch to rx	
		DBYT = MCF_I2C_I2DR;								// dummy read

		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;
		MCF_I2C_I2SR &= 0xfd;
		
		MCF_I2C_I2CR |= 0x08;								// txak=1

		RBYT = MCF_I2C_I2DR;

		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;
		MCF_I2C_I2SR &= 0xfd;

		MCF_I2C_I2CR = 0x80;								// stop
		DBYT = MCF_I2C_I2DR;								// dummy read

		if (RBYT!=0x4c) goto loop_i2c;
		

i2c_ok:
		MCF_I2C_I2CR = 0x0;									// stop
		MCF_I2C_I2SR = 0x0;									// clear sr
		while((MCF_I2C_I2SR & MCF_I2C_I2SR_IBB)) ; 			// wait auf bus free
		
		MCF_I2C_I2CR = 0xb0;								// on tx master	
		MCF_I2C_I2DR = 0x7A;
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;			// warten auf fertig
		MCF_I2C_I2SR &= 0xfd;								// clear bit

		if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK) goto loop_i2c; // ack erhalten? -> nein

		MCF_I2C_I2DR = 0x08;								// SUB ADRESS 8
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;
		MCF_I2C_I2SR &= 0xfd;

		MCF_I2C_I2DR = 0xbf;								// ctl1: power on, T:M:D:S: enable
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;			// warten auf fertig
		MCF_I2C_I2SR &= 0xfd;								// clear bit

		MCF_I2C_I2CR = 0x80;								// stop
		DBYT = MCF_I2C_I2DR;								// dummy read
		MCF_I2C_I2SR = 0x0;									// clear sr

		while((MCF_I2C_I2SR & MCF_I2C_I2SR_IBB)) ; 			// wait auf bus free

		MCF_I2C_I2CR = 0xb0;
		MCF_I2C_I2DR = 0x7A;
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;			// warten auf fertig
		MCF_I2C_I2SR &= 0xfd;								// clear bit

		if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK) goto loop_i2c; // ack erhalten? -> nein

		MCF_I2C_I2DR = 0x08;								// SUB ADRESS 8
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;
		MCF_I2C_I2SR &= 0xfd;

		MCF_I2C_I2CR |= 0x4;								// repeat start
		MCF_I2C_I2DR = 0x7b;								// beginn read
		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;			// warten auf fertig
		MCF_I2C_I2SR &= 0xfd;								// clear bit

		if (MCF_I2C_I2SR & MCF_I2C_I2SR_RXAK) goto loop_i2c; // ack erhalten? -> nein

		MCF_I2C_I2CR &= 0xef;								// switch to rx	

		DBYT = MCF_I2C_I2DR;								// dummy read

		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;
		MCF_I2C_I2SR &= 0xfd;
		
		MCF_I2C_I2CR |= 0x08;								// txak=1

		warte_50us();
		RBYT = MCF_I2C_I2DR;

		while(!(MCF_I2C_I2SR & MCF_I2C_I2SR_IIF)) ;
		MCF_I2C_I2SR &= 0xfd;

		MCF_I2C_I2CR = 0x80;								// stop
		DBYT = MCF_I2C_I2DR;								// dummy read

		if (RBYT!=0xbf) goto loop_i2c;
		
		goto	dvi_ok;
next:
		MCF_PSC0_PSCTB_8BIT = 'NOT ';
dvi_ok:
		MCF_PSC0_PSCTB_8BIT = 'OK! ';
		MCF_PSC0_PSCTB_8BIT = 0x0a0d;
		MCF_I2C_I2CR = 0x0;									// i2c off
}

/********************************************************************/
	/* AC97 /*
/********************************************************************/
void init_ac97(void)
{
// PSC2: AC97 ----------
		int i,k,zm,x,va,vb,vc;

		MCF_PSC0_PSCTB_8BIT = 'AC97';
		MCF_PAD_PAR_PSC2 = MCF_PAD_PAR_PSC2_PAR_RTS2_RTS	// PSC2=TX,RX BCLK,CTS->AC'97
						 | MCF_PAD_PAR_PSC2_PAR_CTS2_BCLK
						 | MCF_PAD_PAR_PSC2_PAR_TXD2
 						 | MCF_PAD_PAR_PSC2_PAR_RXD2;  
 		MCF_PSC2_PSCMR1 = 0x0;		 	 
		MCF_PSC2_PSCMR2 = 0x0;	
		MCF_PSC2_PSCIMR = 0x0300;	 	 
		MCF_PSC2_PSCSICR = 0x03;		//AC97		 
		MCF_PSC2_PSCRFCR = 0x0f000000;
		MCF_PSC2_PSCTFCR = 0x0f000000;
		MCF_PSC2_PSCRFAR = 0x00F0;
		MCF_PSC2_PSCTFAR = 0x00F0;

	for ( zm = 0; zm<100000; zm++)		// wiederholen bis synchron
	{
		MCF_PSC2_PSCCR = 0x20;
		MCF_PSC2_PSCCR = 0x30;
		MCF_PSC2_PSCCR = 0x40;
		MCF_PSC2_PSCCR = 0x05;
// MASTER VOLUME -0dB
		MCF_PSC2_PSCTB_AC97 = 0xE0000000; //START SLOT1 + SLOT2, FIRST FRAME
		MCF_PSC2_PSCTB_AC97 = 0x02000000; //SLOT1:WR REG MASTER VOLUME adr 0x02
		for ( i = 2; i<13; i++ )
		{
			MCF_PSC2_PSCTB_AC97 = 0x0; //SLOT2-12:WR REG ALLES 0
		}
 //	 read register
 		MCF_PSC2_PSCTB_AC97 = 0xc0000000; //START SLOT1 + SLOT2, FIRST FRAME
 		MCF_PSC2_PSCTB_AC97 = 0x82000000; //SLOT1:master volume
		for ( i = 2; i<13; i++ )
		{
			MCF_PSC2_PSCTB_AC97 = 0x00000000; //SLOT2-12:RD REG ALLES 0
		}
		warte_50us();    
		va = MCF_PSC2_PSCTB_AC97;
		if ((va & 0x80000fff)==0x80000800)
		{
			vb = MCF_PSC2_PSCTB_AC97;
 			vc = MCF_PSC2_PSCTB_AC97;
			if ((va & 0xE0000fff)==0xE0000800 & vb==0x02000000 & vc==0x00000000)
			{
				goto livo;
			}
		}
	}
	MCF_PSC0_PSCTB_8BIT = ' NOT';
livo: 
// AUX VOLUME ->-0dB 
	MCF_PSC2_PSCTB_AC97 = 0xE0000000; //START SLOT1 + SLOT2, FIRST FRAME
	MCF_PSC2_PSCTB_AC97 = 0x16000000; //SLOT1:WR REG AUX VOLUME adr 0x16
	MCF_PSC2_PSCTB_AC97 = 0x06060000; //SLOT1:VOLUME 
	for ( i = 3; i<13; i++ )
	{
		MCF_PSC2_PSCTB_AC97 = 0x0; //SLOT2-12:WR REG ALLES 0
	}
 
// line in VOLUME +12dB
	MCF_PSC2_PSCTB_AC97 = 0xE0000000; //START SLOT1 + SLOT2, FIRST FRAME
	MCF_PSC2_PSCTB_AC97 = 0x10000000; //SLOT1:WR REG MASTER VOLUME adr 0x02
	for ( i = 2; i<13; i++ )
	{
		MCF_PSC2_PSCTB_AC97 = 0x0; //SLOT2-12:WR REG ALLES 0
	}
// cd in VOLUME 0dB
	MCF_PSC2_PSCTB_AC97 = 0xE0000000; //START SLOT1 + SLOT2, FIRST FRAME
	MCF_PSC2_PSCTB_AC97 = 0x12000000; //SLOT1:WR REG MASTER VOLUME adr 0x02
	for ( i = 2; i<13; i++ )
	{
		MCF_PSC2_PSCTB_AC97 = 0x0; //SLOT2-12:WR REG ALLES 0
	}
// mono out VOLUME 0dB
	MCF_PSC2_PSCTB_AC97 = 0xE0000000; //START SLOT1 + SLOT2, FIRST FRAME
	MCF_PSC2_PSCTB_AC97 = 0x06000000; //SLOT1:WR REG MASTER VOLUME adr 0x02
	MCF_PSC2_PSCTB_AC97 = 0x00000000; //SLOT1:WR REG MASTER VOLUME adr 0x02
	for ( i = 3; i<13; i++ )
	{
		MCF_PSC2_PSCTB_AC97 = 0x0; //SLOT2-12:WR REG ALLES 0
	}
	MCF_PSC2_PSCTFCR |= MCF_PSC_PSCTFCR_WFR; //set EOF
	MCF_PSC2_PSCTB_AC97 = 0x00000000; //last data

ac97_end:
	MCF_PSC0_PSCTB_8BIT = ' OK!';
	MCF_PSC0_PSCTB_8BIT = 0x0a0d;

}
/********************************************************************/

void __initialize_hardware(void)
{
_init_hardware:
asm
{
	// instruction cache on
	 	move.l		#0x000C8120,d0
	 	move.l		d0,rt_cacr
 		movec		d0,cacr
 		nop
}
		init_gpio();
   		init_seriel();
		init_slt();
		init_fbcs();
		init_ddram();
// Ports nicht initialisieren wenn DIP Switch 5 = on
asm
{
		move.b	DIP_SWITCH,d0					// dip schalter adresse
		btst.b	#6,d0
		beq		not_init_ports
}
 		init_PCI();		//pci braucht zeit
 not_init_ports:
		init_fpga();
		init_video_ddr();
 		vdi_on();
// Ports nicht initialisieren wenn DIP Switch 5 = on
asm
{
		move.b	DIP_SWITCH,d0					// dip schalter adresse
		btst.b	#6,d0
		beq		not_init_ports2
}
  		test_upd720101();
//  		video_1280_1024(); 
  		init_ac97();
not_init_ports2:

asm
{
/*****************************************************/
/* BaS kopieren 
/*****************************************************/
 		lea		copy_start,a0
 		lea		BaS,a1
 		sub.l	a0,a1
		move.l	#__Bas_base,a2
 		move.l	a2,a3
 		add.l	a1,a3
		lea		copy_end,a4
BaS_kopieren_loop:					// immer 16 bytes
		move.l	(a0)+,(a2)+
		move.l	(a0)+,(a2)+
		move.l	(a0)+,(a2)+
		move.l	(a0)+,(a2)+
		cmp.l	a4,a0
		blt		BaS_kopieren_loop
/*****************************************************/
		jmp		(a3)
 copy_start:
/********************************************************************/
}
}

