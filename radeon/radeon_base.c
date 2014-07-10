/*
 *	radeon_base.c
 *
 *	framebuffer driver for ATI Radeon chipset video boards
 *
 *	Copyright 2003	Ben. Herrenschmidt <benh@kernel.crashing.org>
 *	Copyright 2000	Ani Joshi <ajoshi@kernel.crashing.org>
 *
 *	i2c bits from Luca Tettamanti <kronos@kronoz.cjb.net>
 *	
 *	Special thanks to ATI DevRel team for their hardware donations.
 *
 *	...Insert GPL boilerplate here...
 *
 *	Significant portions of this driver apdated from XFree86 Radeon
 *	driver which has the following copyright notice:
 *
 *	Copyright 2000 ATI Technologies Inc., Markham, Ontario, and
 *                     VA Linux Systems Inc., Fremont, California.
 *
 *	All Rights Reserved.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation on the rights to use, copy, modify, merge,
 *	publish, distribute, sublicense, and/or sell copies of the Software,
 *	and to permit persons to whom the Software is furnished to do so,
 *	subject to the following conditions:
 *
 *	The above copyright notice and this permission notice (including the
 *	next paragraph) shall be included in all copies or substantial
 *	portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * 	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 *	THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *	DEALINGS IN THE SOFTWARE.
 *
 *	XFree86 driver authors:
 *
 *	   Kevin E. Martin <martin@xfree86.org>
 *	   Rickard E. Faith <faith@valinux.com>
 *	   Alan Hourihane <alanh@fairlite.demon.co.uk>
 *
 */

#define RADEON_VERSION "0.2.0"

#include "fb.h"
#include "i2c.h"
#include "pci.h"
#include "radeonfb.h"
#include "edid.h"
#include "ati_ids.h"
#include "driver_mem.h"
#include "bas_printf.h"
#include "exceptions.h"		/* for set_ipl() */

#define DBG_RADEON
#ifdef DBG_RADEON
#define dbg(format, arg...) do { xprintf("DEBUG: " format, ##arg); } while (0)
#else
#define dbg(format, arg...) do { ; } while (0)
#endif /* DBG_RADEON */

extern void run_bios(struct radeonfb_info *rinfo);

#define MAX_MAPPED_VRAM	(2048*2048*4)
#define MIN_MAPPED_VRAM	(1024*768*4)

#define CHIP_DEF(id, family, flags)					\
{													\
	PCI_VENDOR_ID_ATI, 								\
	id,												\
	PCI_ANY_ID,										\
	PCI_ANY_ID,										\
	0,												\
	0,												\
	(flags) | (CHIP_FAMILY_##family)				\
}

struct pci_device_id radeonfb_pci_table[] = 
{
	/* Mobility M6 */
	CHIP_DEF(PCI_CHIP_RADEON_LY, 	RV100,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RADEON_LZ,	RV100,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	/* Radeon VE/7000 */
	CHIP_DEF(PCI_CHIP_RV100_QY, 	RV100,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV100_QZ, 	RV100,	CHIP_HAS_CRTC2),
	/* Radeon IGP320M (U1) */
	CHIP_DEF(PCI_CHIP_RS100_4336,	RS100,	CHIP_HAS_CRTC2 | CHIP_IS_IGP | CHIP_IS_MOBILITY),
	/* Radeon IGP320 (A3) */
	CHIP_DEF(PCI_CHIP_RS100_4136,	RS100,	CHIP_HAS_CRTC2 | CHIP_IS_IGP), 
	/* IGP330M/340M/350M (U2) */
	CHIP_DEF(PCI_CHIP_RS200_4337,	RS200,	CHIP_HAS_CRTC2 | CHIP_IS_IGP | CHIP_IS_MOBILITY),
	/* IGP330/340/350 (A4) */
	CHIP_DEF(PCI_CHIP_RS200_4137,	RS200,	CHIP_HAS_CRTC2 | CHIP_IS_IGP),
	/* Mobility 7000 IGP */
	CHIP_DEF(PCI_CHIP_RS250_4437,	RS200,	CHIP_HAS_CRTC2 | CHIP_IS_IGP | CHIP_IS_MOBILITY),
	/* 7000 IGP (A4+) */
	CHIP_DEF(PCI_CHIP_RS250_4237,	RS200,	CHIP_HAS_CRTC2 | CHIP_IS_IGP),
	/* 8500 AIW */
	CHIP_DEF(PCI_CHIP_R200_BB,	R200,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R200_BC,	R200,	CHIP_HAS_CRTC2),
	/* 8700/8800 */
	CHIP_DEF(PCI_CHIP_R200_QH,	R200,	CHIP_HAS_CRTC2),
	/* 8500 */
	CHIP_DEF(PCI_CHIP_R200_QL,	R200,	CHIP_HAS_CRTC2),
	/* 9100 */
	CHIP_DEF(PCI_CHIP_R200_QM,	R200,	CHIP_HAS_CRTC2),
	/* Mobility M7 */
	CHIP_DEF(PCI_CHIP_RADEON_LW,	RV200,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RADEON_LX,	RV200,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	/* 7500 */
	CHIP_DEF(PCI_CHIP_RV200_QW,	RV200,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV200_QX,	RV200,	CHIP_HAS_CRTC2),
	/* Mobility M9 */
	CHIP_DEF(PCI_CHIP_RV250_Ld,	RV250,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV250_Le,	RV250,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV250_Lf,	RV250,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV250_Lg,	RV250,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	/* 9000/Pro */
	CHIP_DEF(PCI_CHIP_RV250_If,	RV250,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV250_Ig,	RV250,	CHIP_HAS_CRTC2),
	/* Mobility 9100 IGP (U3) */
	CHIP_DEF(PCI_CHIP_RS300_5835,	RS300,	CHIP_HAS_CRTC2 | CHIP_IS_IGP | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RS350_7835,	RS300,	CHIP_HAS_CRTC2 | CHIP_IS_IGP | CHIP_IS_MOBILITY),
	/* 9100 IGP (A5) */
	CHIP_DEF(PCI_CHIP_RS300_5834,	RS300,	CHIP_HAS_CRTC2 | CHIP_IS_IGP),
	CHIP_DEF(PCI_CHIP_RS350_7834,	RS300,	CHIP_HAS_CRTC2 | CHIP_IS_IGP),
	/* Mobility 9200 (M9+) */
	CHIP_DEF(PCI_CHIP_RV280_5C61,	RV280,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV280_5C63,	RV280,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	/* 9200 */
	CHIP_DEF(PCI_CHIP_RV280_5960,	RV280,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV280_5961,	RV280,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV280_5962,	RV280,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV280_5964,	RV280,	CHIP_HAS_CRTC2),
	/* 9500 */
	CHIP_DEF(PCI_CHIP_R300_AD,	R300,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R300_AE,	R300,	CHIP_HAS_CRTC2),
	/* 9600TX / FireGL Z1 */
	CHIP_DEF(PCI_CHIP_R300_AF,	R300,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R300_AG,	R300,	CHIP_HAS_CRTC2),
	/* 9700/9500/Pro/FireGL X1 */
	CHIP_DEF(PCI_CHIP_R300_ND,	R300,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R300_NE,	R300,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R300_NF,	R300,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R300_NG,	R300,	CHIP_HAS_CRTC2),
	/* Mobility M10/M11 */
	CHIP_DEF(PCI_CHIP_RV350_NP,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV350_NQ,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV350_NR,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV350_NS,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV350_NT,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV350_NV,	RV350,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	/* 9600/FireGL T2 */
	CHIP_DEF(PCI_CHIP_RV350_AP,	RV350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV350_AQ,	RV350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV360_AR,	RV350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV350_AS,	RV350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV350_AT,	RV350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV350_AV,	RV350,	CHIP_HAS_CRTC2),
	/* 9800/Pro/FileGL X2 */
	CHIP_DEF(PCI_CHIP_R350_AH,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_AI,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_AJ,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_AK,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_NH,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_NI,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R360_NJ,	R350,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R350_NK,	R350,	CHIP_HAS_CRTC2),
	/* Newer stuff */
	CHIP_DEF(PCI_CHIP_RV380_3E50,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV380_3E54,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV380_3150,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV380_3154,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV370_5B60,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV370_5B62,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV370_5B64,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV370_5B65,	RV380,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_RV370_5460,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_RV370_5464,	RV380,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_R420_JH,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JI,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JJ,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JK,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JL,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JM,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R420_JN,	R420,	CHIP_HAS_CRTC2 | CHIP_IS_MOBILITY),
	CHIP_DEF(PCI_CHIP_R420_JP,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UH,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UI,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UJ,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UK,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UQ,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UR,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_UT,	R420,	CHIP_HAS_CRTC2),
	CHIP_DEF(PCI_CHIP_R423_5D57,	R420,	CHIP_HAS_CRTC2),
	/* Original Radeon/7200 */
	CHIP_DEF(PCI_CHIP_RADEON_QD,	RADEON,	0),
	CHIP_DEF(PCI_CHIP_RADEON_QE,	RADEON,	0),
	CHIP_DEF(PCI_CHIP_RADEON_QF,	RADEON,	0),
	CHIP_DEF(PCI_CHIP_RADEON_QG,	RADEON,	0),
	{ 0, 0, 0, 0, 0, 0, 0 }
};


typedef struct
{
	uint16_t reg;
	uint32_t val;
} reg_val;


/* these common regs are cleared before mode setting so they do not
 * interfere with anything
 */
static reg_val common_regs[] =
{
	{ OVR_CLR, 0 },	
	{ OVR_WID_LEFT_RIGHT, 0 },
	{ OVR_WID_TOP_BOTTOM, 0 },
	{ OV0_SCALE_CNTL, 0 },
	{ SUBPIC_CNTL, 0 },
	{ VIPH_CONTROL, 0 },
	{ I2C_CNTL_1, 0 },
	{ GEN_INT_CNTL, 0 },
	{ CAP0_TRIG_CNTL, 0 },
	{ CAP1_TRIG_CNTL, 0 },
};

extern struct fb_info *info_fb;
#define rinfo ((struct radeonfb_info *) info_fb->par)
static uint32_t inreg(uint32_t addr)
{
	return INREG(addr);
}

static void outreg(uint32_t addr, uint32_t val)
{
	OUTREG(addr, val);
}

#undef rinfo
#undef INREG
#define INREG inreg
#undef OUTREG
#define OUTREG outreg

void _OUTREGP(struct radeonfb_info *rinfo, uint32_t addr, uint32_t val, uint32_t mask)
{
	uint32_t tmp;
	tmp = INREG(addr);
	tmp &= (mask);
	tmp |= (val);
	OUTREG(addr, tmp);
}

/*
 * Note about PLL register accesses:
 *
 * I have removed the spinlock on them on purpose. The driver now
 * expects that it will only manipulate the PLL registers in normal
 * task environment, where radeon_msleep() will be called, protected
 * by a semaphore (currently the console semaphore) so that no conflict
 * will happen on the PLL register index.
 *
 * With the latest changes to the VT layer, this is guaranteed for all
 * calls except the actual drawing/blits which aren't supposed to use
 * the PLL registers anyway
 *
 * This is very important for the workarounds to work properly. The only
 * possible exception to this rule is the call to unblank(), which may
 * be done at irq time if an oops is in progress.
 */
void radeon_pll_errata_after_index(struct radeonfb_info *rinfo)
{
	if (!(rinfo->errata & CHIP_ERRATA_PLL_DUMMYREADS))
		return;
	(void)INREG(CLOCK_CNTL_DATA);
	(void)INREG(CRTC_GEN_CNTL);
}

void radeon_pll_errata_after_data(struct radeonfb_info *rinfo)
{
	if (rinfo->errata & CHIP_ERRATA_PLL_DELAY)
	{
		/* we can't deal with posted writes here ... */
		radeon_msleep(5);
	}

	if (rinfo->errata & CHIP_ERRATA_R300_CG)
	{
		uint32_t save, tmp;
		save = INREG(CLOCK_CNTL_INDEX);
		tmp = save & ~(0x3f | PLL_WR_EN);
		OUTREG(CLOCK_CNTL_INDEX, tmp);
		tmp = INREG(CLOCK_CNTL_DATA);
		OUTREG(CLOCK_CNTL_INDEX, save);
	}
}

uint32_t __INPLL(struct radeonfb_info *rinfo, uint32_t addr)
{
	uint32_t data;

	OUTREG8(CLOCK_CNTL_INDEX, addr & 0x0000003f);
	radeon_pll_errata_after_index(rinfo);
	data = INREG(CLOCK_CNTL_DATA);
	radeon_pll_errata_after_data(rinfo);
	return data;
}

void __OUTPLL(struct radeonfb_info *rinfo, uint32_t index, uint32_t val)
{
	OUTREG8(CLOCK_CNTL_INDEX, (index & 0x0000003f) | 0x00000080);
	radeon_pll_errata_after_index(rinfo);
	OUTREG(CLOCK_CNTL_DATA, val);
	radeon_pll_errata_after_data(rinfo);
}

void __OUTPLLP(struct radeonfb_info *rinfo, uint32_t index, uint32_t val, uint32_t mask)
{
	uint32_t tmp;

	tmp  = __INPLL(rinfo, index);
	tmp &= (mask);
	tmp |= (val);
	__OUTPLL(rinfo, index, tmp);
}

static int round_div(int num, int den)
{
	return(num + (den / 2)) / den;
}

static uint32_t read_vline_crnt(struct radeonfb_info *rinfo)
{
	return((INREG(CRTC_VLINE_CRNT_VLINE) >> 16) & 0x3FF);
}

static int radeon_map_ROM(struct radeonfb_info *rinfo)
{
	uint16_t dptr;
	uint8_t rom_type;

	/*
	 * If this is a primary card, there is a shadow copy of the
	 * ROM somewhere in the first meg. We will just ignore the copy
	 * and use the ROM directly.
	 */

 	/* Fix from ATI for problem with Radeon hardware not leaving ROM enabled */

	uint32_t temp;

	temp = INREG(MPP_TB_CONFIG);
	temp &= 0x00ffffffu;
	temp |= 0x04 << 24;
	OUTREG(MPP_TB_CONFIG, temp);
	temp = INREG(MPP_TB_CONFIG);                                                                                                    
	
	if (rinfo->bios_seg == NULL)
	{
		dbg("%s: ROM failed to map\r\n", __FUNCTION__);
		return -1;
	}

	/* Very simple test to make sure it appeared */
	if (BIOS_IN16(0) != 0xaa55)
	{
		dbg("%s: Invalid ROM signature", __FUNCTION__);
		goto failed;
	}

	/* Look for the PCI data to check the ROM type */
	dptr = BIOS_IN16(0x18);

	/* Check the PCI data signature. If it's wrong, we still assume a normal x86 ROM
	 * for now, until I've verified this works everywhere. The goal here is more
	 * to phase out Open Firmware images.
	 *
	 * Currently, we only look at the first PCI data, we could iteratre and deal with
	 * them all, and we should use fb_bios_start relative to start of image and not
	 * relative start of ROM, but so far, I never found a dual-image ATI card
	 *
	 * typedef struct {
	 * 	u32	signature;	+ 0x00
	 * 	u16	vendor;		+ 0x04
	 * 	u16	device;		+ 0x06
	 * 	u16	reserved_1;	+ 0x08
	 * 	u16	dlen;		+ 0x0a
	 * 	u8	drevision;	+ 0x0c
	 * 	u8	class_hi;	+ 0x0d
	 * 	u16	class_lo;	+ 0x0e
	 * 	u16	ilen;		+ 0x10
	 * 	u16	irevision;	+ 0x12
	 * 	u8	type;		+ 0x14
	 * 	u8	indicator;	+ 0x15
	 * 	u16	reserved_2;	+ 0x16
	 * } pci_data_t;
	 */
	if (BIOS_IN32(dptr) !=  (('R' << 24) | ('I' << 16) | ('C' << 8) | 'P'))
	{
		dbg("%s: PCI DATA signature in ROM incorrect: %p\r\n", __FUNCTION__, BIOS_IN32(dptr));
		goto anyway;
	}
	rom_type = BIOS_IN8(dptr + 0x14);
	switch(rom_type)
	{
	case 0:
		dbg("%s: Found Intel x86 BIOS ROM Image\r\n", __FUNCTION__);
		break;
	case 1:
		dbg("%s: Found Open Firmware ROM Image\r\n", __FUNCTION__);
		goto failed;
	case 2:
		dbg("%s: Found HP PA-RISC ROM Image\r\n", __FUNCTION__);
		goto failed;
	default:
		dbg("%s: Found unknown type %d ROM Image\r\n", rom_type, __FUNCTION__);
		goto failed;
	}
anyway:
	/* Locate the flat panel infos, do some sanity checking !!! */
	rinfo->fp_bios_start = BIOS_IN16(0x48);
	dbg("%s: BIOS start offset: %p\r\n", __FUNCTION__, BIOS_IN16(0x48));

	/* Save BIOS PLL informations */
	{
		uint16_t pll_info_block = BIOS_IN16(rinfo->fp_bios_start + 0x30);

		dbg("%s: BIOS PLL info block offset: %p\r\n", __FUNCTION__, BIOS_IN16(rinfo->fp_bios_start + 0x30));
		rinfo->bios_pll.sclk		= BIOS_IN16(pll_info_block + 0x08);
		rinfo->bios_pll.mclk		= BIOS_IN16(pll_info_block + 0x0a);
		rinfo->bios_pll.ref_clk	= BIOS_IN16(pll_info_block + 0x0e);
		rinfo->bios_pll.ref_div	= BIOS_IN16(pll_info_block + 0x10);
		rinfo->bios_pll.ppll_min	= BIOS_IN32(pll_info_block + 0x12);
		rinfo->bios_pll.ppll_max	= BIOS_IN32(pll_info_block + 0x16);
	}
	return 0;

failed:
	rinfo->bios_seg = NULL;
	return -1; //-ENXIO;
}

/*
 * Read PLL infos from chip registers
 */
static int radeon_probe_pll_params(struct radeonfb_info *rinfo)
{
	uint8_t ppll_div_sel;
	unsigned Ns, Nm, M;
	unsigned sclk, mclk, tmp, ref_div;
	int hTotal, vTotal, num, denom, m, n;
	double hz, vclk;
	int32_t xtal;
	uint32_t start_tv, stop_tv;
	int timeout = 0;
	int ipl;

	/*
	 * Ugh, we cut interrupts, bad bad bad, but we want some precision
	 * here, so... --BenH
	 */
	dbg("%s: radeon_probe_pll_params\r\n", __FUNCTION__);

	/* Flush PCI buffers ? */
	tmp = INREG16(DEVICE_ID);

	ipl = set_ipl(0);

	start_tv = get_timer();
	while (read_vline_crnt(rinfo) != 0)
	{
		if ((get_timer() - start_tv) > US_TO_TIMER(10000000UL))    /* 10 sec */
		{
			timeout=1;
			break;
		}
	}

	if (!timeout)
	{
		start_tv = get_timer();
		while (read_vline_crnt(rinfo) == 0)
		{
			if ((get_timer() - start_tv) > US_TO_TIMER(1000000UL))   /* 1 sec */
			{
				timeout=1;
				break;
			}
		}
		if (!timeout)
		{
			while (read_vline_crnt(rinfo) != 0)
			{
				if ((get_timer() - start_tv) > US_TO_TIMER(10000000UL))    /* 10 sec */
				{
					timeout=1;
					break;
				}
			}
		}
	}
	stop_tv = get_timer();

	set_ipl(ipl);

	hz = US_TO_TIMER(1000000.0) / (double)(stop_tv - start_tv);
	dbg("%s:hz %d\r\n", __FUNCTION__, (int32_t) hz);

	hTotal = ((INREG(CRTC_H_TOTAL_DISP) & 0x1ff) + 1) * 8;
	vTotal = ((INREG(CRTC_V_TOTAL_DISP) & 0x3ff) + 1);
	dbg("%s:hTotal=%d\r\n", __FUNCTION__, hTotal);
	dbg("%s:vTotal=%d\r\n", __FUNCTION__, vTotal);

	vclk = (double) hTotal * (double) vTotal * hz;
	dbg("%s:vclk=%d\r\n", __FUNCTION__, (int) vclk);

	switch ((INPLL(PPLL_REF_DIV) & 0x30000) >> 16)
	{
		case 1:
			n = ((INPLL(M_SPLL_REF_FB_DIV) >> 16) & 0xff);
			m = (INPLL(M_SPLL_REF_FB_DIV) & 0xff);
			num = 2 * n;
			denom = 2 * m;
			break;

		case 2:
			n = ((INPLL(M_SPLL_REF_FB_DIV) >> 8) & 0xff);
			m = (INPLL(M_SPLL_REF_FB_DIV) & 0xff);
			num = 2 * n;
			denom = 2 * m;
			break;

		case 0:
		default:
			num = 1;
			denom = 1;
			break;
	}

	ppll_div_sel = INREG8(CLOCK_CNTL_INDEX + 1) & 0x3;
	radeon_pll_errata_after_index(rinfo);

	n = (INPLL(PPLL_DIV_0 + ppll_div_sel) & 0x7ff);
	m = (INPLL(PPLL_REF_DIV) & 0x3ff);

	num *= n;
	denom *= m;

	switch((INPLL(PPLL_DIV_0 + ppll_div_sel) >> 16) & 0x7)
	{
		case 1:
			denom *= 2;
			break;

		case 2:
			denom *= 4;
			break;
			
		case 3:
			denom *= 8;
			break;

		case 4:
			denom *= 3;
			break;

		case 6:
			denom *= 6;   
			break;

		case 7:
			denom *= 12;
			break;
	}
	vclk *= (double) denom;
	vclk /= (double) (1000 * num);
	xtal = (int32_t) vclk;

	if ((xtal > 26900) && (xtal < 27100))
		xtal = 2700;   /* 27 MHz */
	else if ((xtal > 14200) && (xtal < 14400))
		xtal = 1432;
	else if ((xtal > 29400) && (xtal < 29600))
		xtal = 2950;
	else
	{
		dbg("%s: xtal calculation failed: %d\r\n", __FUNCTION__, xtal);
		return -1; /* error */
	}

	tmp = INPLL(M_SPLL_REF_FB_DIV);
	ref_div = INPLL(PPLL_REF_DIV) & 0x3ff;

	Ns = (tmp & 0xff0000) >> 16;
	Nm = (tmp & 0xff00) >> 8;
	M = (tmp & 0xff);

	sclk = round_div((2 * Ns * xtal), (2 * M));
	mclk = round_div((2 * Nm * xtal), (2 * M));

	/* we're done, hopefully these are sane values */
	rinfo->pll.ref_clk = xtal;
	rinfo->pll.ref_div = ref_div;
	rinfo->pll.sclk = sclk;
	rinfo->pll.mclk = mclk;

	return 0;
}

/*
 * Retreive PLL infos by register probing...
 */
static void radeon_get_pllinfo(struct radeonfb_info *rinfo)
{
	/*
	 * In the case nothing works, these are defaults; they are mostly
	 * incomplete, however.  It does provide ppll_max and _min values
	 * even for most other methods, however.
	 */
	dbg("%s:\r\n", __FUNCTION__);

	switch(rinfo->chipset)
	{
		case PCI_DEVICE_ID_ATI_RADEON_QW:
		case PCI_DEVICE_ID_ATI_RADEON_QX:
			rinfo->pll.ppll_max = 35000;
			rinfo->pll.ppll_min = 12000;
			rinfo->pll.mclk = 23000;
			rinfo->pll.sclk = 23000;
			rinfo->pll.ref_clk = 2700;
			break;

		case PCI_DEVICE_ID_ATI_RADEON_QL:
		case PCI_DEVICE_ID_ATI_RADEON_QN:
		case PCI_DEVICE_ID_ATI_RADEON_QO:
		case PCI_DEVICE_ID_ATI_RADEON_Ql:
		case PCI_DEVICE_ID_ATI_RADEON_BB:
			rinfo->pll.ppll_max = 35000;
			rinfo->pll.ppll_min = 12000;
			rinfo->pll.mclk = 27500;
			rinfo->pll.sclk = 27500;
			rinfo->pll.ref_clk = 2700;
			break;

		case PCI_DEVICE_ID_ATI_RADEON_Id:
		case PCI_DEVICE_ID_ATI_RADEON_Ie:
		case PCI_DEVICE_ID_ATI_RADEON_If:
		case PCI_DEVICE_ID_ATI_RADEON_Ig:
			rinfo->pll.ppll_max = 35000;
			rinfo->pll.ppll_min = 12000;
			rinfo->pll.mclk = 25000;
			rinfo->pll.sclk = 25000;
			rinfo->pll.ref_clk = 2700;
			break;

		case PCI_DEVICE_ID_ATI_RADEON_ND:
		case PCI_DEVICE_ID_ATI_RADEON_NE:
		case PCI_DEVICE_ID_ATI_RADEON_NF:
		case PCI_DEVICE_ID_ATI_RADEON_NG:
			rinfo->pll.ppll_max = 40000;
			rinfo->pll.ppll_min = 20000;
			rinfo->pll.mclk = 27000;
			rinfo->pll.sclk = 27000;
			rinfo->pll.ref_clk = 2700;
			break;

		case PCI_DEVICE_ID_ATI_RADEON_QD:
		case PCI_DEVICE_ID_ATI_RADEON_QE:
		case PCI_DEVICE_ID_ATI_RADEON_QF:
		case PCI_DEVICE_ID_ATI_RADEON_QG:
		default:
			rinfo->pll.ppll_max = 35000;
			rinfo->pll.ppll_min = 12000;
			rinfo->pll.mclk = 16600;
			rinfo->pll.sclk = 16600;
			rinfo->pll.ref_clk = 2700;
			break;
	}
	rinfo->pll.ref_div = INPLL(PPLL_REF_DIV) & PPLL_REF_DIV_MASK;

	/*
	 * Check out if we have an X86 which gave us some PLL informations
	 * and if yes, retreive them
	 */
	if (!force_measure_pll && (rinfo->bios_seg != NULL))
	{
		rinfo->pll.sclk		= rinfo->bios_pll.sclk;
		rinfo->pll.mclk		= rinfo->bios_pll.mclk;
		rinfo->pll.ref_clk	= rinfo->bios_pll.ref_clk;
		rinfo->pll.ref_div	= rinfo->bios_pll.ref_div;
		rinfo->pll.ppll_min	= rinfo->bios_pll.ppll_min;
		rinfo->pll.ppll_max	= rinfo->bios_pll.ppll_max;
		dbg("%s: Retreived PLL infos from BIOS\r\n", __FUNCTION__);

		goto found;
	}

	/*
	 * We didn't get PLL parameters from either OF or BIOS, we try to
	 * probe them
	 */
	if (radeon_probe_pll_params(rinfo) == 0)
	{
		dbg("%s: Retreived PLL infos from registers\r\n", __FUNCTION__);
		goto found;
	}

	/*
	 * Fall back to already-set defaults...
	 */
	dbg("%s: Used default PLL infos\r\n", __FUNCTION__);

found:
	/*
	 * Some methods fail to retreive SCLK and MCLK values, we apply default
	 * settings in this case (200Mhz). If that really happne often, we could
	 * fetch from registers instead...
	 */
	if (rinfo->pll.mclk == 0)
		rinfo->pll.mclk = 20000;
	if (rinfo->pll.sclk == 0)
		rinfo->pll.sclk = 20000;

	dbg("%s: Reference=%d MHz (RefDiv=0x%x) Memory=%d MHz\r\n", __FUNCTION__,
			rinfo->pll.ref_clk / 100, rinfo->pll.ref_div, rinfo->pll.mclk / 100);
	dbg("%s: System=%d MHz PLL min %d, max %d\r\n", __FUNCTION__,
			rinfo->pll.sclk / 100, rinfo->pll.ppll_min, rinfo->pll.ppll_max);
}

static int var_to_depth(const struct fb_var_screeninfo *var)
{
	if (var->bits_per_pixel != 16)
		return var->bits_per_pixel;

	return(var->green.length == 5) ? 15 : 16;
}

int radeonfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct radeonfb_info *rinfo = info->par;
	struct fb_var_screeninfo v;
	int nom, den;
	uint32_t pitch;

	dbg("%s:\r\n", __FUNCTION__);

	/* clocks over 135 MHz have heat isues with DVI on RV100 */
	if ((rinfo->mon1_type == MT_DFP) && (rinfo->family == CHIP_FAMILY_RV100) && ((100000000 / var->pixclock) > 13500))
	{
		dbg("%s: mode %d x %d x %d", __FUNCTION__, var->xres, var->yres, var->bits_per_pixel);
		dbg("%s:  rejected, RV100 DVI clock over 135 MHz\r\n", __FUNCTION__);

		return -1; //-EINVAL;
	}

	if (radeon_match_mode(rinfo, &v, var))
		return -1; //-EINVAL;

	switch (v.bits_per_pixel)
	{
		case 0 ... 8:
			v.bits_per_pixel = 8;
			break;

		case 9 ... 16:
			v.bits_per_pixel = 16;
			break;

#if 0 /* Doesn't seem to work */
		case 17 ... 24:
			v.bits_per_pixel = 24;
			break;
#endif			
		case 25 ... 32:
			v.bits_per_pixel = 32;
			break;

		default:
			return -1; //-EINVAL;
	}

	switch (var_to_depth(&v))
	{
		case 8:
			nom = den = 1;
			v.red.offset = v.green.offset = v.blue.offset = 0;
			v.red.length = v.green.length = v.blue.length = 8;
			v.transp.offset = v.transp.length = 0;
			break;

		case 15:
			nom = 2;
			den = 1;
			v.red.offset = 10;
			v.green.offset = 5;
			v.blue.offset = 0;
			v.red.length = v.green.length = v.blue.length = 5;
			v.transp.offset = v.transp.length = 0;
			break;

		case 16:
			nom = 2;
			den = 1;
			v.red.offset = 11;
			v.green.offset = 5;
			v.blue.offset = 0;
			v.red.length = 5;
			v.green.length = 6;
			v.blue.length = 5;
			v.transp.offset = v.transp.length = 0;
			break;                          

		case 24:
			nom = 4;
			den = 1;
			v.red.offset = 16;
			v.green.offset = 8;
			v.blue.offset = 0;
			v.red.length = v.blue.length = v.green.length = 8;
			v.transp.offset = v.transp.length = 0;
			break;

		case 32:
			nom = 4;
			den = 1;
			v.red.offset = 16;
			v.green.offset = 8;
			v.blue.offset = 0;
			v.red.length = v.blue.length = v.green.length = 8;
			v.transp.offset = 24;
			v.transp.length = 8;
			break;

    default:
			dbg("radeonfb: mode %d x %d x %d rejected, color depth invalid\r\n ",
					var->xres, var->yres, var->bits_per_pixel);
			return -1; //-EINVAL;
	}

	if (v.yres_virtual < v.yres)
		v.yres_virtual = v.yres;
	if (v.xres_virtual < v.xres)
		v.xres_virtual = v.xres;

	/*
	 * XXX I'm adjusting xres_virtual to the pitch, that may help XFree
	 * with some panels, though I don't quite like this solution
	 */
	pitch = ((v.xres_virtual * ((v.bits_per_pixel + 1) / 8) + 0x3f) & ~(0x3f)) >> 6;
	v.xres_virtual = (pitch << 6) / ((v.bits_per_pixel + 1) / 8);

	if (((v.xres_virtual * v.yres_virtual * nom) / den) > info->screen_size)
	{
		dbg("%s: mode %d x %d rejected (screen size too small)\r\n", __FUNCTION__, v.xres_virtual, v.yres_virtual);
		return -1; //-EINVAL;
	}

	if (v.xres_virtual < v.xres)
		v.xres = v.xres_virtual;
	
	if (v.xoffset < 0)
		v.xoffset = 0;

	if (v.yoffset < 0)
		v.yoffset = 0;

	if (v.xoffset > v.xres_virtual - v.xres)
		v.xoffset = v.xres_virtual - v.xres - 1;

	if (v.yoffset > v.yres_virtual - v.yres)
		v.yoffset = v.yres_virtual - v.yres - 1;

	v.red.msb_right = v.green.msb_right = v.blue.msb_right = 0;
	v.transp.offset = v.transp.length = v.transp.msb_right = 0;

	dbg("%s: using mode %d x %d \r\n", __FUNCTION__, v.xres, v.yres);

	memcpy(var, &v, sizeof(v));

	return 0;
}

int radeonfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct radeonfb_info *rinfo = info->par;
//	DPRINT("radeonfb: radeonfb_pan_display\r\n");
	if ((var->xoffset + var->xres) > var->xres_virtual)
		return -1; //-EINVAL;

	if (((var->yoffset * var->xres_virtual) + var->xoffset) >=
	 (rinfo->mapped_vram - (var->yres * var->xres * (var->bits_per_pixel / 8))))
		return -1; //-EINVAL;

	if (rinfo->asleep)
		return 0;

	radeon_wait_for_fifo(rinfo, 2);
	rinfo->fb_offset = ((var->yoffset * var->xres_virtual + var->xoffset) * var->bits_per_pixel / 8) & ~7;
	rinfo->dst_pitch_offset = (rinfo->pitch << 22) | ((rinfo->fb_local_base + rinfo->fb_offset) >> 10);
	OUTREG(CRTC_OFFSET, rinfo->fb_offset);

	return 0;
}

short mirror;

int radeonfb_ioctl(unsigned int cmd, unsigned long arg, struct fb_info *info)
{
	struct radeonfb_info *rinfo = info->par;
	uint32_t tmp;
	uint32_t value = 0;

	switch(cmd)
	{
		/*
		 * TODO:  set mirror accordingly for non-Mobility chipsets with 2 CRTC's
		 *        and do something better using 2nd CRTC instead of just hackish
		 *        routing to second output
		 */
		case FBIO_RADEON_SET_MIRROR:
			if (!rinfo->is_mobility)
				return -1; //-EINVAL;
			radeon_wait_for_fifo(rinfo, 2);

			if (value & 0x01)
			{
				tmp = INREG(LVDS_GEN_CNTL);
				tmp |= (LVDS_ON | LVDS_BLON);
			}
			else
			{
				tmp = INREG(LVDS_GEN_CNTL);
				tmp &= ~(LVDS_ON | LVDS_BLON);
			}
			OUTREG(LVDS_GEN_CNTL, tmp);

			if (value & 0x02)
			{
				tmp = INREG(CRTC_EXT_CNTL);
				tmp |= CRTC_CRT_ON;
				mirror = 1;
			}
			else
			{
				tmp = INREG(CRTC_EXT_CNTL);
				tmp &= ~CRTC_CRT_ON;
				mirror = 0;
			}
			OUTREG(CRTC_EXT_CNTL, tmp);
			return 0;

		case FBIO_RADEON_GET_MIRROR:
			if (!rinfo->is_mobility)
				return -1; //-EINVAL;
			tmp = INREG(LVDS_GEN_CNTL);
			if ((LVDS_ON | LVDS_BLON) & tmp)
				value |= 0x01;
			tmp = INREG(CRTC_EXT_CNTL);
			if (CRTC_CRT_ON & tmp)
				value |= 0x02;
			return 0;

		default:
			return -1; //-EINVAL;
	}
	return -1; //-EINVAL;
}

int32_t radeon_screen_blank(struct radeonfb_info *rinfo, int32_t blank, int32_t mode_switch)
{
	uint32_t val;
	uint32_t tmp_pix_clks;
	int unblank = 0;

	if (rinfo->lock_blank)
		return 0;

	dbg("radeonfb: radeon_screen_blank\r\n");
	radeon_engine_idle();
	val = INREG(CRTC_EXT_CNTL);
	val &= ~(CRTC_DISPLAY_DIS | CRTC_HSYNC_DIS | CRTC_VSYNC_DIS);

	switch(blank)
	{
		case FB_BLANK_VSYNC_SUSPEND:
			val |= (CRTC_DISPLAY_DIS | CRTC_VSYNC_DIS);
			break;

		case FB_BLANK_HSYNC_SUSPEND:
			val |= (CRTC_DISPLAY_DIS | CRTC_HSYNC_DIS);
			break;

		case FB_BLANK_POWERDOWN:
			val |= (CRTC_DISPLAY_DIS | CRTC_VSYNC_DIS | CRTC_HSYNC_DIS);
			break;

		case FB_BLANK_NORMAL:
			val |= CRTC_DISPLAY_DIS;
			break;

		case FB_BLANK_UNBLANK:
		default:
			unblank = 1;
			break;
	}
	OUTREG(CRTC_EXT_CNTL, val);

	switch(rinfo->mon1_type)
	{
		case MT_DFP:
			if (unblank)
				OUTREGP(FP_GEN_CNTL, (FP_FPON | FP_TMDS_EN), ~(FP_FPON | FP_TMDS_EN));
			else
			{
				if (mode_switch || blank == FB_BLANK_NORMAL)
					break;
				OUTREGP(FP_GEN_CNTL, 0, ~(FP_FPON | FP_TMDS_EN));
			}
			break;

		case MT_LCD:
			rinfo->lvds_timer = 0;
			val = INREG(LVDS_GEN_CNTL);
			if (unblank)
			{
				uint32_t target_val = (val & ~LVDS_DISPLAY_DIS) | LVDS_BLON | LVDS_ON
				 | LVDS_EN | (rinfo->init_state.lvds_gen_cntl & (LVDS_DIGON | LVDS_BL_MOD_EN));
				if ((val ^ target_val) == LVDS_DISPLAY_DIS)
					OUTREG(LVDS_GEN_CNTL, target_val);
				else if ((val ^ target_val) != 0)
				{
					OUTREG(LVDS_GEN_CNTL, target_val & ~(LVDS_ON | LVDS_BL_MOD_EN));
					rinfo->init_state.lvds_gen_cntl &= ~LVDS_STATE_MASK;
					rinfo->init_state.lvds_gen_cntl |= target_val & LVDS_STATE_MASK;
					if (mode_switch)
					{
						radeon_msleep(rinfo->panel_info.pwr_delay);
						OUTREG(LVDS_GEN_CNTL, target_val);
					}
					else
					{
						rinfo->pending_lvds_gen_cntl = target_val;
						rinfo->lvds_timer = (int32_t)rinfo->panel_info.pwr_delay;
					}
				}
			}
			else
			{
				val |= LVDS_DISPLAY_DIS;
				OUTREG(LVDS_GEN_CNTL, val);
				/* We don't do a full switch-off on a simple mode switch */
				if (mode_switch || blank == FB_BLANK_NORMAL)
					break;

				/* Asic bug, when turning off LVDS_ON, we have to make sure
				 * RADEON_PIXCLK_LVDS_ALWAYS_ON bit is off
				 */
				tmp_pix_clks = INPLL(PIXCLKS_CNTL);
				if (rinfo->is_mobility || rinfo->is_IGP)
					OUTPLLP(PIXCLKS_CNTL, 0, ~PIXCLK_LVDS_ALWAYS_ONb);

				val &= ~(LVDS_BL_MOD_EN);
				OUTREG(LVDS_GEN_CNTL, val);

				wait(100);

				val &= ~(LVDS_ON | LVDS_EN);
				OUTREG(LVDS_GEN_CNTL, val);
				val &= ~LVDS_DIGON;
				rinfo->pending_lvds_gen_cntl = val;
				rinfo->lvds_timer = (int32_t)rinfo->panel_info.pwr_delay;
				rinfo->init_state.lvds_gen_cntl &= ~LVDS_STATE_MASK;
				rinfo->init_state.lvds_gen_cntl |= val & LVDS_STATE_MASK;

				if (rinfo->is_mobility || rinfo->is_IGP)
					OUTPLL(PIXCLKS_CNTL, tmp_pix_clks);
			}
			break;
		case MT_CRT:
			// todo: powerdown DAC
		default:
			break;
	}
	/* let fbcon do a soft blank for us */
	return(blank == FB_BLANK_NORMAL) ? -1 /* -EINVAL */ : 0;
}

int radeonfb_blank(int blank, struct fb_info *info)
{
	struct radeonfb_info *rinfo = info->par;

	if (rinfo->asleep)
		return 0;

	return radeon_screen_blank(rinfo, blank, 0);
}

static int radeon_setcolreg(unsigned regno, unsigned red, unsigned green,
                            unsigned blue, unsigned transp, struct fb_info *info)
{
	struct radeonfb_info *rinfo = info->par;
	uint32_t pindex;

	if (regno > 255)
		return 1;

	red >>= 8;
	green >>= 8;
	blue >>= 8;

	rinfo->palette[regno].red = red;
	rinfo->palette[regno].green = green;
	rinfo->palette[regno].blue = blue;

	/* default */
	pindex = regno;
	if (!rinfo->asleep)
	{
		radeon_wait_for_fifo(rinfo, 9);
		if (rinfo->bpp == 16)
		{
			pindex = regno * 8;
			if (rinfo->depth == 16 && regno > 63)
				return 1;
			if (rinfo->depth == 15 && regno > 31)
				return 1;

			/*
			 * For 565, the green component is mixed one order
			 * below
			 */
			if (rinfo->depth == 16)
			{
				OUTREG(PALETTE_INDEX, pindex>>1);
				OUTREG(PALETTE_DATA,(rinfo->palette[regno>>1].red << 16)
				 | (green << 8) | (rinfo->palette[regno>>1].blue));
				green = rinfo->palette[regno<<1].green;
			}
		}
		if (rinfo->depth != 16 || regno < 32)
		{
			OUTREG(PALETTE_INDEX, pindex);
			OUTREG(PALETTE_DATA, (red << 16) | (green << 8) | blue);
		}
	}
	return 0;
}

int radeonfb_setcolreg(unsigned regno, unsigned red, unsigned green,
    						unsigned blue, unsigned transp, struct fb_info *info)
{
	struct radeonfb_info *rinfo = info->par;
	uint32_t dac_cntl2, vclk_cntl = 0;
	int rc;

	if (!rinfo->asleep)
	{
		if (rinfo->is_mobility)
		{
			vclk_cntl = INPLL(VCLK_ECP_CNTL);
			OUTPLL(VCLK_ECP_CNTL, vclk_cntl & ~PIXCLK_DAC_ALWAYS_ONb);
		}

		/* Make sure we are on first palette */
		if (rinfo->has_CRTC2)
		{
			dac_cntl2 = INREG(DAC_CNTL2);
			dac_cntl2 &= ~DAC2_PALETTE_ACCESS_CNTL;
			OUTREG(DAC_CNTL2, dac_cntl2);
		}
	}
	rc = radeon_setcolreg(regno, red, green, blue, transp, info);
	if (!rinfo->asleep && rinfo->is_mobility)
		OUTPLL(VCLK_ECP_CNTL, vclk_cntl);

	return rc;
}

static void radeon_save_state(struct radeonfb_info *rinfo, struct radeon_regs *save)
{
	/* CRTC regs */
	save->crtc_gen_cntl = INREG(CRTC_GEN_CNTL);
	save->crtc_ext_cntl = INREG(CRTC_EXT_CNTL);
	save->crtc_more_cntl = INREG(CRTC_MORE_CNTL);
	save->dac_cntl = INREG(DAC_CNTL);
	save->crtc_h_total_disp = INREG(CRTC_H_TOTAL_DISP);
	save->crtc_h_sync_strt_wid = INREG(CRTC_H_SYNC_STRT_WID);
	save->crtc_v_total_disp = INREG(CRTC_V_TOTAL_DISP);
	save->crtc_v_sync_strt_wid = INREG(CRTC_V_SYNC_STRT_WID);
	save->crtc_pitch = INREG(CRTC_PITCH);
	save->surface_cntl = INREG(SURFACE_CNTL);

	/* FP regs */
	save->fp_crtc_h_total_disp = INREG(FP_CRTC_H_TOTAL_DISP);
	save->fp_crtc_v_total_disp = INREG(FP_CRTC_V_TOTAL_DISP);
	save->fp_gen_cntl = INREG(FP_GEN_CNTL);
	save->fp_h_sync_strt_wid = INREG(FP_H_SYNC_STRT_WID);
	save->fp_horz_stretch = INREG(FP_HORZ_STRETCH);
	save->fp_v_sync_strt_wid = INREG(FP_V_SYNC_STRT_WID);
	save->fp_vert_stretch = INREG(FP_VERT_STRETCH);
	save->lvds_gen_cntl = INREG(LVDS_GEN_CNTL);
	save->lvds_pll_cntl = INREG(LVDS_PLL_CNTL);
	save->tmds_crc = INREG(TMDS_CRC);
	save->tmds_transmitter_cntl = INREG(TMDS_TRANSMITTER_CNTL);
	save->vclk_ecp_cntl = INPLL(VCLK_ECP_CNTL);
	/* PLL regs */

	save->clk_cntl_index = INREG(CLOCK_CNTL_INDEX) & ~0x3f;
	radeon_pll_errata_after_index(rinfo);
	save->ppll_div_3 = INPLL(PPLL_DIV_3);
	save->ppll_ref_div = INPLL(PPLL_REF_DIV);
}

static void radeon_write_pll_regs(struct radeonfb_info *rinfo, struct radeon_regs *mode)
{
	int i;

	dbg("radeonfb: radeon_write_pll_regs\r\n");
	radeon_wait_for_fifo(rinfo, 20);

#if 0
	/* Workaround from XFree */
	if (rinfo->is_mobility)
	{
	   /* A temporal workaround for the occational blanking on certain laptop
		 * panels. This appears to related to the PLL divider registers
		 * (fail to lock?). It occurs even when all dividers are the same
		 * with their old settings. In this case we really don't need to
		 * fiddle with PLL registers. By doing this we can avoid the blanking
		 * problem with some panels.
	         */
		if ((mode->ppll_ref_div == (INPLL(PPLL_REF_DIV) & PPLL_REF_DIV_MASK))
		 && (mode->ppll_div_3 == (INPLL(PPLL_DIV_3) & (PPLL_POST3_DIV_MASK | PPLL_FB3_DIV_MASK))))
		{
			/* We still have to force a switch to selected PPLL div thanks to
			 * an XFree86 driver bug which will switch it away in some cases
			 * even when using UseFDev */
			OUTREGP(CLOCK_CNTL_INDEX,
				mode->clk_cntl_index & PPLL_DIV_SEL_MASK,
				~PPLL_DIV_SEL_MASK);
			radeon_pll_errata_after_index(rinfo);
			radeon_pll_errata_after_data(rinfo);
			return;
		}
	}
#endif

	/* Swich VCKL clock input to CPUCLK so it stays fed while PPLL updates*/
	OUTPLLP(VCLK_ECP_CNTL, VCLK_SRC_SEL_CPUCLK, ~VCLK_SRC_SEL_MASK);

	/* Reset PPLL & enable atomic update */
	OUTPLLP(PPLL_CNTL, PPLL_RESET | PPLL_ATOMIC_UPDATE_EN | PPLL_VGA_ATOMIC_UPDATE_EN,
	 ~(PPLL_RESET | PPLL_ATOMIC_UPDATE_EN | PPLL_VGA_ATOMIC_UPDATE_EN));

	/* Switch to selected PPLL divider */
	OUTREGP(CLOCK_CNTL_INDEX,	mode->clk_cntl_index & PPLL_DIV_SEL_MASK, ~PPLL_DIV_SEL_MASK);
	radeon_pll_errata_after_index(rinfo);
	radeon_pll_errata_after_data(rinfo);

	/* Set PPLL ref. div */
	if (rinfo->family == CHIP_FAMILY_R300 || rinfo->family == CHIP_FAMILY_RS300
	 || rinfo->family == CHIP_FAMILY_R350 || rinfo->family == CHIP_FAMILY_RV350)
	{
		if (mode->ppll_ref_div & R300_PPLL_REF_DIV_ACC_MASK)
		{
			/*
			 * When restoring console mode, use saved PPLL_REF_DIV
			 * setting.
			 */
			OUTPLLP(PPLL_REF_DIV, mode->ppll_ref_div, 0);
		}
		else
		{
			/* R300 uses ref_div_acc field as real ref divider */
			OUTPLLP(PPLL_REF_DIV,(mode->ppll_ref_div << R300_PPLL_REF_DIV_ACC_SHIFT),~R300_PPLL_REF_DIV_ACC_MASK);
		}
	}
	else
		OUTPLLP(PPLL_REF_DIV, mode->ppll_ref_div, ~PPLL_REF_DIV_MASK);

	/* Set PPLL divider 3 & post divider*/
	OUTPLLP(PPLL_DIV_3, mode->ppll_div_3, ~PPLL_FB3_DIV_MASK);
	OUTPLLP(PPLL_DIV_3, mode->ppll_div_3, ~PPLL_POST3_DIV_MASK);

	/* Write update */
	while (INPLL(PPLL_REF_DIV) & PPLL_ATOMIC_UPDATE_R);
	OUTPLLP(PPLL_REF_DIV, PPLL_ATOMIC_UPDATE_W, ~PPLL_ATOMIC_UPDATE_W);

	/* Wait read update complete */
	/* FIXME: Certain revisions of R300 can't recover here.  Not sure of
	   the cause yet, but this workaround will mask the problem for now.
	   Other chips usually will pass at the very first test, so the
	   workaround shouldn't have any effect on them. */

	for (i = 0; (i < 10000 && INPLL(PPLL_REF_DIV) & PPLL_ATOMIC_UPDATE_R); i++);
	OUTPLL(HTOTAL_CNTL, 0);

	/* Clear reset & atomic update */
	OUTPLLP(PPLL_CNTL, 0, ~(PPLL_RESET | PPLL_SLEEP | PPLL_ATOMIC_UPDATE_EN | PPLL_VGA_ATOMIC_UPDATE_EN));

	/* We may want some locking ... oh well */
	radeon_msleep(5);

	/* Switch back VCLK source to PPLL */
	OUTPLLP(VCLK_ECP_CNTL, VCLK_SRC_SEL_PPLLCLK, ~VCLK_SRC_SEL_MASK);
}

static void radeon_wait_vbl(struct fb_info *info)
{
	uint32_t cnt = INREG(CRTC_CRNT_FRAME);

	while (cnt == INREG(CRTC_CRNT_FRAME));
}

static void radeon_timer_func(void)
{
	struct fb_info *info = info_fb;
	struct radeonfb_info *rinfo = info->par;
	struct fb_var_screeninfo var;
	uint32_t x, y;
	int chg, disp;

#ifdef FIXME_LATER
	static int32_t start_timer;

	/* delayed LVDS panel power up/down */
	if (rinfo->lvds_timer)
	{
		if (!start_timer)
			start_timer = *_hz_200;

		if (((*_hz_200 - start_timer) * 5) >= (int32_t)rinfo->lvds_timer)
		{
			rinfo->lvds_timer = 0;
			radeon_engine_idle();
			OUTREG(LVDS_GEN_CNTL, rinfo->pending_lvds_gen_cntl);
		}
	}
	else
		start_timer = 0;
#endif /* FIXME_LATER */

	if (rinfo->RenderCallback != NULL)
		rinfo->RenderCallback(rinfo);

	if ((info->screen_mono != NULL) && info->update_mono)
	{
		int32_t foreground = 255, background = 0;
		uint8_t *src_buf = (uint8_t *)info->screen_mono;
		int skipleft = ((int)src_buf & 3) << 3;
		int dst_x = 0;
		int w = (int)info->var.xres_virtual;
		int h = (int)info->var.yres_virtual;

//		info->fbops->SetClippingRectangle(info,0,0,w-1,h-1);
		src_buf = (uint8_t*)((int32_t)src_buf & ~3);
		dst_x -= (int32_t)skipleft;
		w += (int32_t)skipleft;
		info->fbops->SetupForScanlineCPUToScreenColorExpandFill(info,(int)foreground,(int)background,3,0xffffffff);
		info->fbops->SubsequentScanlineCPUToScreenColorExpandFill(info,(int)dst_x,0,w,h,skipleft);

		while (--h >= 0)
		{
			info->fbops->SubsequentScanline(info, (unsigned long *) src_buf);
			src_buf += (info->var.xres_virtual >> 3);
		}

//		info->fbops->DisableClipping(info);
		if (info->update_mono > 0)
			info->update_mono = 0;
	}

	if ((info->var.xres_virtual != info->var.xres)
	 || (info->var.yres_virtual != info->var.yres))
	{
		int ipl;
		ipl = set_ipl(0);

		chg = 0;
		x = info->var.xoffset;
		y = info->var.yoffset;

		if (((x + info->var.xres) < info->var.xres_virtual) && (rinfo->cursor_x >= (info->var.xres - 8)))
		{
			x += 8;
			chg = 1;
		}
		else if ((x >= 8) && (rinfo->cursor_x <= 8))
		{
			x -= 8;
			chg = 1;
		}

		if (((y + info->var.yres) < info->var.yres_virtual) && (rinfo->cursor_y >= (info->var.yres - 8)))
		{
			y += 8;
			chg = 1;
		}
		else if ((y >=8) && (rinfo->cursor_y <= 8))
		{
			y -= 8;
			chg = 1;
		}

		if (chg)
		{
			memcpy(&var, &info->var, sizeof(struct fb_var_screeninfo));
			var.xoffset = x;
			var.yoffset = y;
			disp = rinfo->cursor_show;
			if (disp)
				info->fbops->HideCursor(info);

			fb_pan_display(info,&var);

			if (disp)
				info->fbops->ShowCursor(info);
		}
		set_ipl(ipl);
	}
}

/*
 * Apply a video mode. This will apply the whole register set, including
 * the PLL registers, to the card
 */
void radeon_write_mode(struct radeonfb_info *rinfo, struct radeon_regs *mode, int32_t regs_only)
{
	int i;
	int primary_mon = PRIMARY_MONITOR(rinfo);

	dbg("radeonfb: radeon_write_mode\r\n");

	if (!regs_only)
		radeon_screen_blank(rinfo, FB_BLANK_NORMAL, 0);

	radeon_wait_for_fifo(rinfo, 31);

	for (i = 0; i < 10; i++)
		OUTREG(common_regs[i].reg, common_regs[i].val);

	/* Apply surface registers */
	for (i = 0; i < 8; i++)
	{
		OUTREG(SURFACE0_LOWER_BOUND + 0x10*i, mode->surf_lower_bound[i]);
		OUTREG(SURFACE0_UPPER_BOUND + 0x10*i, mode->surf_upper_bound[i]);
		OUTREG(SURFACE0_INFO + 0x10*i, mode->surf_info[i]);
	}
	OUTREG(CRTC_GEN_CNTL, mode->crtc_gen_cntl);
	OUTREGP(CRTC_EXT_CNTL, mode->crtc_ext_cntl, ~(CRTC_HSYNC_DIS | CRTC_VSYNC_DIS | CRTC_DISPLAY_DIS));
	OUTREG(CRTC_MORE_CNTL, mode->crtc_more_cntl);
	OUTREGP(DAC_CNTL, mode->dac_cntl, DAC_RANGE_CNTL | DAC_BLANKING);
	OUTREG(CRTC_H_TOTAL_DISP, mode->crtc_h_total_disp);
	OUTREG(CRTC_H_SYNC_STRT_WID, mode->crtc_h_sync_strt_wid);
	OUTREG(CRTC_V_TOTAL_DISP, mode->crtc_v_total_disp);
	OUTREG(CRTC_V_SYNC_STRT_WID, mode->crtc_v_sync_strt_wid);
	rinfo->fb_offset = 0;
	rinfo->dst_pitch_offset = (rinfo->pitch << 22) | ((rinfo->fb_local_base + rinfo->fb_offset) >> 10);
	OUTREG(CRTC_OFFSET, rinfo->fb_offset);
#ifdef RADEON_TILING
	if (rinfo->tilingEnabled)
	{
		if (rinfo->family >= CHIP_FAMILY_R300)
			OUTREG(CRTC_OFFSET_CNTL, R300_CRTC_X_Y_MODE_EN | R300_CRTC_MICRO_TILE_BUFFER_DIS | R300_CRTC_MACRO_TILE_EN);
		else
			OUTREG(CRTC_OFFSET_CNTL, CRTC_OFFSET_CNTL__CRTC_TILE_EN);
	}
	else
#endif
		OUTREG(CRTC_OFFSET_CNTL, 0);

	OUTREG(CRTC_PITCH, mode->crtc_pitch);
	OUTREG(SURFACE_CNTL, mode->surface_cntl);
	radeon_write_pll_regs(rinfo, mode);

	if ((primary_mon == MT_DFP) || (primary_mon == MT_LCD))
	{
		radeon_wait_for_fifo(rinfo, 10);
		OUTREG(FP_CRTC_H_TOTAL_DISP, mode->fp_crtc_h_total_disp);
		OUTREG(FP_CRTC_V_TOTAL_DISP, mode->fp_crtc_v_total_disp);
		OUTREG(FP_H_SYNC_STRT_WID, mode->fp_h_sync_strt_wid);
		OUTREG(FP_V_SYNC_STRT_WID, mode->fp_v_sync_strt_wid);
		OUTREG(FP_HORZ_STRETCH, mode->fp_horz_stretch);
		OUTREG(FP_VERT_STRETCH, mode->fp_vert_stretch);
		OUTREG(FP_GEN_CNTL, mode->fp_gen_cntl);
		OUTREG(TMDS_CRC, mode->tmds_crc);
		OUTREG(TMDS_TRANSMITTER_CNTL, mode->tmds_transmitter_cntl);
	}

	if (!regs_only)
		radeon_screen_blank(rinfo, FB_BLANK_UNBLANK, 0);
	radeon_wait_for_fifo(rinfo, 2);
	OUTPLL(VCLK_ECP_CNTL, mode->vclk_ecp_cntl);
}

/*
 * Calculate the PLL values for a given mode
 */
static void radeon_calc_pll_regs(struct radeonfb_info *rinfo, struct radeon_regs *regs, uint32_t freq)
{
	static const struct
	{
		int divider;
		int bitvalue;
	} *post_div,
	post_divs[] =
	{
		{ 1,  0 },
		{ 2,  1 },
		{ 4,  2 },
		{ 8,  3 },
		{ 3,  4 },
		{ 16, 5 },
		{ 6,  6 },
		{ 12, 7 },
		{ 0,  0 },
	};
	int fb_div, pll_output_freq = 0;
	int uses_dvo = 0;

	/* Check if the DVO port is enabled and sourced from the primary CRTC. I'm
	 * not sure which model starts having FP2_GEN_CNTL, I assume anything more
	 * recent than an r(v)100...
	 */
#if 1
	/* XXX I had reports of flicker happening with the cinema display
	 * on TMDS1 that seem to be fixed if I also forbit odd dividers in
	 * this case. This could just be a bandwidth calculation issue, I
	 * haven't implemented the bandwidth code yet, but in the meantime,
	 * forcing uses_dvo to 1 fixes it and shouln't have bad side effects,
	 * I haven't seen a case were were absolutely needed an odd PLL
	 * divider. I'll find a better fix once I have more infos on the
	 * real cause of the problem.
	 */
	while (rinfo->has_CRTC2)
	{
		uint32_t fp2_gen_cntl = INREG(FP2_GEN_CNTL);
		uint32_t disp_output_cntl;
		int source;

		/* FP2 path not enabled */
		if ((fp2_gen_cntl & FP2_ON) == 0)
			break;

		/* Not all chip revs have the same format for this register,
		 * extract the source selection
		 */
		if (rinfo->family == CHIP_FAMILY_R200 || rinfo->family == CHIP_FAMILY_R300
		 || rinfo->family == CHIP_FAMILY_R350 || rinfo->family == CHIP_FAMILY_RV350)
		{
			source = (fp2_gen_cntl >> 10) & 0x3;
			/* sourced from transform unit, check for transform unit
			 * own source
			 */
			if (source == 3)
			{
				disp_output_cntl = INREG(DISP_OUTPUT_CNTL);
				source = (disp_output_cntl >> 12) & 0x3;
			}
		}
		else
			source = (fp2_gen_cntl >> 13) & 0x1;

		/* sourced from CRTC2 -> exit */
		if (source == 1)
			break;

		/* so we end up on CRTC1, let's set uses_dvo to 1 now */
		uses_dvo = 1;
		break;
	}
#else
	uses_dvo = 1;
#endif
	if (freq > rinfo->pll.ppll_max)
		freq = rinfo->pll.ppll_max;
	if (freq * 12 < rinfo->pll.ppll_min)
		freq = rinfo->pll.ppll_min / 12;
	for (post_div = &post_divs[0]; post_div->divider; ++post_div)
	{
		pll_output_freq = post_div->divider * freq;

		/*
		 * If we output to the DVO port (external TMDS), we don't allow an
		 * odd PLL divider as those aren't supported on this path
		 */
		if (uses_dvo && (post_div->divider & 1))
			continue;

		if (pll_output_freq >= rinfo->pll.ppll_min  &&
		    pll_output_freq <= rinfo->pll.ppll_max)
			break;
	}

	/* If we fall through the bottom, try the "default value"
	   given by the terminal post_div->bitvalue */
	if (!post_div->divider)
	{
		post_div = &post_divs[post_div->bitvalue];
		pll_output_freq = post_div->divider * freq;
	}

	/* If we fall through the bottom, try the "default value"
	   given by the terminal post_div->bitvalue */
	if ( !post_div->divider )
	{
		post_div = &post_divs[post_div->bitvalue];
		pll_output_freq = post_div->divider * freq;
	}
	fb_div = round_div(rinfo->pll.ref_div*pll_output_freq,rinfo->pll.ref_clk);
	regs->ppll_ref_div = rinfo->pll.ref_div;
	regs->ppll_div_3 = fb_div | (post_div->bitvalue << 16); 
}

int radeonfb_set_par(struct fb_info *info)
{
	struct radeonfb_info *rinfo = info->par;
	struct fb_var_screeninfo *mode = &info->var;
	struct radeon_regs *newmode;
	int hTotal, vTotal, hSyncStart, hSyncEnd, vSyncStart, vSyncEnd;
	// FIXME: int hSyncPol; this is not used anywhere
	// FIXME: int vSyncPol; this is not used anywhere
	// FIXME: int cSync;	this is not used anywhere
	static uint8_t hsync_adj_tab[] = {0, 0x12, 9, 9, 6, 5};
	static uint8_t hsync_fudge_fp[] = {2, 2, 0, 0, 5, 5};
	uint32_t sync, h_sync_pol, v_sync_pol, dotClock, pixClock;
	int i, freq;
	int format = 0;
	int nopllcalc = 0;
	int hsync_start;
	int hsync_fudge;
	// int bytpp; FIXME: this doesn't seem to be used anywhere
	int hsync_wid, vsync_wid;
	int primary_mon = PRIMARY_MONITOR(rinfo);
	int depth = var_to_depth(mode);
	int use_rmx = 0;

	newmode = (struct radeon_regs *) driver_mem_alloc(sizeof(struct radeon_regs));
	if (!newmode)
		return -1; //-ENOMEM;

	/* We always want engine to be idle on a mode switch, even
	 * if we won't actually change the mode
	 */
	dbg("radeonfb: radeonfb_set_par\r\n");
	radeon_engine_idle();
	hSyncStart = mode->xres + mode->right_margin;
	hSyncEnd = hSyncStart + mode->hsync_len;
	hTotal = hSyncEnd + mode->left_margin;

	vSyncStart = mode->yres + mode->lower_margin;
	vSyncEnd = vSyncStart + mode->vsync_len;
	vTotal = vSyncEnd + mode->upper_margin;

	pixClock = mode->pixclock;
	sync = mode->sync;

	h_sync_pol = sync & FB_SYNC_HOR_HIGH_ACT ? 0 : 1;
	v_sync_pol = sync & FB_SYNC_VERT_HIGH_ACT ? 0 : 1;

	if (primary_mon == MT_DFP || primary_mon == MT_LCD)
	{
		if (rinfo->panel_info.xres < mode->xres)
			mode->xres = rinfo->panel_info.xres;

		if (rinfo->panel_info.yres < mode->yres)
			mode->yres = rinfo->panel_info.yres;

		hTotal = mode->xres + rinfo->panel_info.hblank;
		hSyncStart = mode->xres + rinfo->panel_info.hOver_plus;
		hSyncEnd = hSyncStart + rinfo->panel_info.hSync_width;

		vTotal = mode->yres + rinfo->panel_info.vblank;
		vSyncStart = mode->yres + rinfo->panel_info.vOver_plus;
		vSyncEnd = vSyncStart + rinfo->panel_info.vSync_width;

		h_sync_pol = !rinfo->panel_info.hAct_high;
		v_sync_pol = !rinfo->panel_info.vAct_high;

		pixClock = 100000000 / rinfo->panel_info.clock;

		if (rinfo->panel_info.use_bios_dividers)
		{
			nopllcalc = 1;
			newmode->ppll_div_3 = rinfo->panel_info.fbk_divider | (rinfo->panel_info.post_divider << 16);
			newmode->ppll_ref_div = rinfo->panel_info.ref_divider;
		}
	}

	dotClock = 1000000000 / pixClock;
	freq = dotClock / 10; /* x100 */
	hsync_wid = (hSyncEnd - hSyncStart) / 8;

	if (hsync_wid == 0)
		hsync_wid = 1;
	else if (hsync_wid > 0x3f)	/* max */
		hsync_wid = 0x3f;

	if (mode->vmode & FB_VMODE_DOUBLE)
	{
		vSyncStart <<= 1;
		vSyncEnd <<= 1;
		vTotal <<= 1;
	}

	vsync_wid = vSyncEnd - vSyncStart;
	if (vsync_wid == 0)
		vsync_wid = 1;
	else if (vsync_wid > 0x1f)	/* max */
		vsync_wid = 0x1f;

	// FIXME: this doesn't seem to be used anywhere hSyncPol = mode->sync & FB_SYNC_HOR_HIGH_ACT ? 0 : 1;
	// FIXME: this doesn't seem to be used anywhere vSyncPol = mode->sync & FB_SYNC_VERT_HIGH_ACT ? 0 : 1;
	// FIXME: this doesn't seem to be used anywhere cSync = mode->sync & FB_SYNC_COMP_HIGH_ACT ? (1 << 4) : 0;
	format = radeon_get_dstbpp(depth);
	// FIXME: this doesn't seem to be used anywhere bytpp = mode->bits_per_pixel >> 3;

	if ((primary_mon == MT_DFP) || (primary_mon == MT_LCD))
		hsync_fudge = hsync_fudge_fp[format-1];
	else
		hsync_fudge = hsync_adj_tab[format-1];

	if (mode->vmode & FB_VMODE_DOUBLE)
		hsync_fudge = 0; /* todo: need adjust */		

	hsync_start = hSyncStart - 8 + hsync_fudge;
	newmode->crtc_gen_cntl = CRTC_EXT_DISP_EN | CRTC_EN | (format << 8);
	if (mode->vmode & FB_VMODE_DOUBLE)
		newmode->crtc_gen_cntl |= CRTC_DBL_SCAN_EN;
	if (mode->vmode & FB_VMODE_INTERLACED)
		newmode->crtc_gen_cntl |= CRTC_INTERLACE_EN;

	/* Clear auto-center etc... */
	newmode->crtc_more_cntl = rinfo->init_state.crtc_more_cntl;
	newmode->crtc_more_cntl &= 0xfffffff0;

	if ((primary_mon == MT_DFP) || (primary_mon == MT_LCD))
	{
		newmode->crtc_ext_cntl = VGA_ATI_LINEAR | XCRT_CNT_EN;
		if (mirror)
			newmode->crtc_ext_cntl |= CRTC_CRT_ON;
		newmode->crtc_gen_cntl &= ~(CRTC_DBL_SCAN_EN | CRTC_INTERLACE_EN);
	}
	else
		newmode->crtc_ext_cntl = VGA_ATI_LINEAR | XCRT_CNT_EN | CRTC_CRT_ON;

	newmode->dac_cntl = /* INREG(DAC_CNTL) | */ DAC_MASK_ALL | DAC_VGA_ADR_EN | DAC_8BIT_EN;
	newmode->crtc_h_total_disp = ((((hTotal / 8) - 1) & 0x3ff) | (((mode->xres / 8) - 1) << 16));
	newmode->crtc_h_sync_strt_wid = ((hsync_start & 0x1fff) | (hsync_wid << 16) | (h_sync_pol << 23));

	if (mode->vmode & FB_VMODE_DOUBLE)
		newmode->crtc_v_total_disp = ((vTotal - 1) & 0xffff) | (((mode->yres << 1) - 1) << 16);
	else
		newmode->crtc_v_total_disp = ((vTotal - 1) & 0xffff) | ((mode->yres - 1) << 16);

	newmode->crtc_v_sync_strt_wid = (((vSyncStart - 1) & 0xfff) | (vsync_wid << 16) | (v_sync_pol << 23));
	/* We first calculate the engine pitch */
	rinfo->pitch = ((mode->xres_virtual * ((mode->bits_per_pixel + 1) / 8) + 0x3f) & ~(0x3f)) >> 6;
	/* Then, re-multiply it to get the CRTC pitch */
	newmode->crtc_pitch = (rinfo->pitch << 3) / ((mode->bits_per_pixel + 1) / 8);
	newmode->crtc_pitch |= (newmode->crtc_pitch << 16);

	/*
	 * It looks like recent chips have a problem with SURFACE_CNTL,
	 * setting SURF_TRANSLATION_DIS completely disables the
	 * swapper as well, so we leave it unset now.
	 */
	newmode->surface_cntl = 0;

	if (rinfo->big_endian)
	{
		/* Setup swapping on both apertures, though we currently
		 * only use aperture 0, enabling swapper on aperture 1
		 * won't harm
		 */
		switch(mode->bits_per_pixel)
		{
			case 16:
				newmode->surface_cntl |= NONSURF_AP0_SWP_16BPP;
				newmode->surface_cntl |= NONSURF_AP1_SWP_16BPP;
				break;
			case 24:	
			case 32:
				newmode->surface_cntl |= NONSURF_AP0_SWP_32BPP;
				newmode->surface_cntl |= NONSURF_AP1_SWP_32BPP;
				break;
		}
	}

	/* Clear surface registers */
	for (i = 0; i < 8; i++)
	{
		newmode->surf_lower_bound[i] = 0;
		newmode->surf_upper_bound[i] = 0x1f;
		newmode->surf_info[i] = 0;
	}

	rinfo->bpp = mode->bits_per_pixel;
	rinfo->depth = depth;

	/* We use PPLL_DIV_3 */
	newmode->clk_cntl_index = 0x300;

	/* Calculate PPLL value if necessary */
	if (!nopllcalc)
		radeon_calc_pll_regs(rinfo, newmode, freq);

	newmode->vclk_ecp_cntl = rinfo->init_state.vclk_ecp_cntl;
	if ((primary_mon == MT_DFP) || (primary_mon == MT_LCD))
	{
		int hRatio, vRatio;
		if (mode->xres > rinfo->panel_info.xres)
			mode->xres = rinfo->panel_info.xres;
		if (mode->yres > rinfo->panel_info.yres)
			mode->yres = rinfo->panel_info.yres;
		newmode->fp_horz_stretch = (((rinfo->panel_info.xres / 8) - 1) << HORZ_PANEL_SHIFT);
		newmode->fp_vert_stretch = ((rinfo->panel_info.yres - 1) << VERT_PANEL_SHIFT);

		if (mode->xres != rinfo->panel_info.xres)
		{
			hRatio = round_div(mode->xres * HORZ_STRETCH_RATIO_MAX, rinfo->panel_info.xres);
			newmode->fp_horz_stretch = (((hRatio & HORZ_STRETCH_RATIO_MASK))
			 | (newmode->fp_horz_stretch & (HORZ_PANEL_SIZE | HORZ_FP_LOOP_STRETCH | HORZ_AUTO_RATIO_INC)));
			newmode->fp_horz_stretch |= (HORZ_STRETCH_BLEND | HORZ_STRETCH_ENABLE);
			use_rmx = 1;
		}

		newmode->fp_horz_stretch &= ~HORZ_AUTO_RATIO;
		if (mode->yres != rinfo->panel_info.yres)
		{
			vRatio = round_div(mode->yres * VERT_STRETCH_RATIO_MAX, rinfo->panel_info.yres);
			newmode->fp_vert_stretch = (((((uint32_t)vRatio) & VERT_STRETCH_RATIO_MASK))
			 | (newmode->fp_vert_stretch & (VERT_PANEL_SIZE | VERT_STRETCH_RESERVED)));
			newmode->fp_vert_stretch |= (VERT_STRETCH_BLEND | VERT_STRETCH_ENABLE);
			use_rmx = 1;
		}
		newmode->fp_vert_stretch &= ~VERT_AUTO_RATIO_EN;
		newmode->fp_gen_cntl = (rinfo->init_state.fp_gen_cntl
		 & ~(FP_SEL_CRTC2 | FP_RMX_HVSYNC_CONTROL_EN | FP_DFP_SYNC_SEL | FP_CRT_SYNC_SEL
		  | FP_CRTC_LOCK_8DOT | FP_USE_SHADOW_EN | FP_CRTC_USE_SHADOW_VEND | FP_CRT_SYNC_ALT));
		newmode->fp_gen_cntl |= (FP_CRTC_DONT_SHADOW_VPAR | FP_CRTC_DONT_SHADOW_HEND | FP_PANEL_FORMAT);

		if (IS_R300_VARIANT(rinfo) || (rinfo->family == CHIP_FAMILY_R200))
		{
			newmode->fp_gen_cntl &= ~R200_FP_SOURCE_SEL_MASK;
			if (use_rmx)
				newmode->fp_gen_cntl |= R200_FP_SOURCE_SEL_RMX;
			else
				newmode->fp_gen_cntl |= R200_FP_SOURCE_SEL_CRTC1;
		}
		else
			newmode->fp_gen_cntl |= FP_SEL_CRTC1;

		newmode->lvds_gen_cntl = rinfo->init_state.lvds_gen_cntl;
		newmode->lvds_pll_cntl = rinfo->init_state.lvds_pll_cntl;
		newmode->tmds_crc = rinfo->init_state.tmds_crc;
		newmode->tmds_transmitter_cntl = rinfo->init_state.tmds_transmitter_cntl;

		if (primary_mon == MT_LCD)
		{
			newmode->lvds_gen_cntl |= (LVDS_ON | LVDS_BLON);
			newmode->fp_gen_cntl &= ~(FP_FPON | FP_TMDS_EN);
		}
		else
		{
			/* DFP */
			newmode->fp_gen_cntl |= (FP_FPON | FP_TMDS_EN);
			newmode->tmds_transmitter_cntl &= ~(TMDS_PLLRST);
			/* TMDS_PLL_EN bit is reversed on RV (and mobility) chips */
			if (IS_R300_VARIANT(rinfo) || (rinfo->family == CHIP_FAMILY_R200) || !rinfo->has_CRTC2)
				newmode->tmds_transmitter_cntl &= ~TMDS_PLL_EN;
			else
				newmode->tmds_transmitter_cntl |= TMDS_PLL_EN;
			newmode->crtc_ext_cntl &= ~CRTC_CRT_ON;
		}

		newmode->fp_crtc_h_total_disp = (((rinfo->panel_info.hblank / 8) & 0x3ff) | (((mode->xres / 8) - 1) << 16));
		newmode->fp_crtc_v_total_disp = (rinfo->panel_info.vblank & 0xffff) | ((mode->yres - 1) << 16);
		newmode->fp_h_sync_strt_wid = ((rinfo->panel_info.hOver_plus & 0x1fff) | (hsync_wid << 16) | (h_sync_pol << 23));
		newmode->fp_v_sync_strt_wid = ((rinfo->panel_info.vOver_plus & 0xfff) | (vsync_wid << 16) | (v_sync_pol  << 23));
	}

	/* do it! */
	if (!rinfo->asleep)
	{
#if 0
		if (debug)
		{
			dbg("Press a key for write the video mode...\r\n");
			Bconin(2);
		}
#endif
		memcpy(&rinfo->state, newmode, sizeof(*newmode));
#ifdef RADEON_TILING
		rinfo->tilingEnabled = (mode->vmode & (FB_VMODE_DOUBLE | FB_VMODE_INTERLACED)) ? FALSE : TRUE;
#endif
		radeon_write_mode(rinfo, newmode, 0);
		/* (re)initialize the engine */
		radeon_engine_init(rinfo);
	}

	/* Update fix */
	info->fix.line_length = rinfo->pitch*64;
	info->fix.visual = rinfo->depth == 8 ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_DIRECTCOLOR;
	driver_mem_free(newmode);

	return 0;
}

static void radeonfb_check_modes(struct fb_info *info, struct mode_option *resolution)
{
	struct radeonfb_info *rinfo = info->par;

	radeon_check_modes(rinfo, resolution);
}

static struct fb_ops radeonfb_ops =
{
	.fb_check_var = radeonfb_check_var,
	.fb_set_par = radeonfb_set_par,
	.fb_setcolreg = radeonfb_setcolreg,
	.fb_pan_display = radeonfb_pan_display,
	.fb_blank = radeonfb_blank,
	.fb_sync = radeonfb_sync,
	.fb_ioctl = radeonfb_ioctl,
	.fb_check_modes = radeonfb_check_modes,
	.SetupForSolidFill = radeon_setup_for_solid_fill,
	.SubsequentSolidFillRect = radeon_subsequent_solid_fill_rect_mmio,
	.SetupForSolidLine = radeon_setup_for_solid_line_mmio,
	.SubsequentSolidHorVertLine = radeon_subsequent_solid_hor_vert_line_mmio,
	.SubsequentSolidTwoPointLine = radeon_subsequent_solid_two_point_line_mmio,
	.SetupForDashedLine = radeon_setup_for_dashed_line_mmio,
	.SubsequentDashedTwoPointLine = radeon_subsequent_dashed_two_point_line_mmio,
	.SetupForScreenToScreenCopy = radeon_setup_for_screen_to_screen_copy_mmio,
	.SubsequentScreenToScreenCopy = radeon_subsequent_screen_to_screen_copy_mmio,
	.ScreenToScreenCopy = radeon_screen_to_screen_copy_mmio,
	.SetupForMono8x8PatternFill = radeon_setup_for_mono_8x8_pattern_fill_mmio,
	.SubsequentMono8x8PatternFillRect = radeon_subsequent_mono_8x8_pattern_fill_rect_mmio,
	.SetupForScanlineCPUToScreenColorExpandFill = radeon_setup_for_scanline_cpu_to_screen_color_expand_fill_mmio,
	.SubsequentScanlineCPUToScreenColorExpandFill = radeon_subsequent_scanline_cpu_to_screen_color_expand_fill_mmio,
	.SubsequentScanline = radeon_subsequent_scanline_mmio,
	.SetupForScanlineImageWrite = radeon_setup_for_scanline_image_write_mmio,
	.SubsequentScanlineImageWriteRect = radeon_subsequent_scanline_image_write_rect_mmio,
	.SetClippingRectangle = radeon_set_clipping_rectangle_mmio,
	.DisableClipping = radeon_disable_clipping_mmio,
#ifdef RADEON_RENDER
	.SetupForCPUToScreenAlphaTexture = radeon_setup_for_cpu_to_screen_alpha_texture_mmio,
	.SetupForCPUToScreenTexture = radeon_setup_for_cpu_to_screen_texture_mmio,
	.SubsequentCPUToScreenTexture = radeon_subsequent_cpu_to_screen_texture_mmio,
#else
	.SetupForCPUToScreenAlphaTexture = NULL,
	.SetupForCPUToScreenTexture = NULL,
	.SubsequentCPUToScreenTexture = NULL,
#endif /* RADEON_RENDER */
	.SetCursorColors = radeon_set_cursor_colors,
	.SetCursorPosition = radeon_set_cursor_position,
	.LoadCursorImage = radeon_load_cursor_image,
	.HideCursor = radeon_hide_cursor,
	.ShowCursor = radeon_show_cursor,
	.CursorInit = radeon_cursor_init,
	.WaitVbl = radeon_wait_vbl,
};

static int radeon_set_fbinfo(struct radeonfb_info *rinfo)
{
	struct fb_info *info = rinfo->info;

	info->par = rinfo;
	info->fbops = &radeonfb_ops;
	info->ram_base = info->screen_base = rinfo->fb_base;
	info->screen_size = rinfo->mapped_vram;
	info->ram_size = rinfo->mapped_vram;
	if (info->screen_size > MAX_MAPPED_VRAM)
		info->screen_size = MAX_MAPPED_VRAM;
	else if (info->screen_size > MIN_MAPPED_VRAM)
		info->screen_size = MIN_MAPPED_VRAM;

	dbg("%s: ram_base %p\r\n", __FUNCTION__, info->screen_base);
	dbg("%s: ram_size %p\r\n", __FUNCTION__, info->ram_size);

	/* Fill fix common fields */
	memcpy(info->fix.id, rinfo->name, sizeof(info->fix.id));
	info->fix.smem_start = rinfo->fb_base_phys;
	info->fix.smem_len = rinfo->video_ram;
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
	info->fix.xpanstep = 8;
	info->fix.ypanstep = 1;
	info->fix.ywrapstep = 0;
	info->fix.type_aux = 0;
	info->fix.mmio_start = rinfo->mmio_base_phys;
	info->fix.mmio_len = RADEON_REGSIZE;
	info->fix.accel = FB_ACCEL_ATI_RADEON;
	return 0;
}

static void radeon_identify_vram(struct radeonfb_info *rinfo)
{
	uint32_t tmp;
	/* framebuffer size */
	if ((rinfo->family == CHIP_FAMILY_RS100)
	 || (rinfo->family == CHIP_FAMILY_RS200)
	 || (rinfo->family == CHIP_FAMILY_RS300))
	{
		uint32_t tom = INREG(NB_TOM);
		tmp = ((((tom >> 16) - (tom & 0xffff) + 1) << 6) * 1024);
		radeon_wait_for_fifo(rinfo, 6);
		OUTREG(MC_FB_LOCATION, tom);
		OUTREG(DISPLAY_BASE_ADDR, (tom & 0xffff) << 16);
		OUTREG(CRTC2_DISPLAY_BASE_ADDR, (tom & 0xffff) << 16);
		OUTREG(OV0_BASE_ADDR, (tom & 0xffff) << 16);
		/* This is supposed to fix the crtc2 noise problem. */
		OUTREG(GRPH2_BUFFER_CNTL, INREG(GRPH2_BUFFER_CNTL) & ~0x7f0000);
		if ((rinfo->family == CHIP_FAMILY_RS100) || (rinfo->family == CHIP_FAMILY_RS200))
		{
			/* This is to workaround the asic bug for RMX, some versions
			    of BIOS dosen't have this register initialized correctly. */
			OUTREGP(CRTC_MORE_CNTL, CRTC_H_CUTOFF_ACTIVE_EN, ~CRTC_H_CUTOFF_ACTIVE_EN);
		}
	}
	else
		tmp = INREG(CONFIG_MEMSIZE);
	/* mem size is bits [28:0], mask off the rest */
	rinfo->video_ram = tmp & CONFIG_MEMSIZE_MASK;
	/*
	 * Hack to get around some busted production M6's
	 * reporting no ram
	 */
	if (rinfo->video_ram == 0)
	{
		switch(rinfo->chipset)
		{
		  case PCI_CHIP_RADEON_LY:
			case PCI_CHIP_RADEON_LZ: rinfo->video_ram = 8192 * 1024; break;
			default: break;
		}
	}
	/*
	 * Now try to identify VRAM type
	 */
	if (rinfo->is_IGP || (rinfo->family >= CHIP_FAMILY_R300)
	 || (INREG(MEM_SDRAM_MODE_REG) & (1<<30)))
		rinfo->vram_ddr = 1;
	else
		rinfo->vram_ddr = 0;
	tmp = INREG(MEM_CNTL);
	if (IS_R300_VARIANT(rinfo))
	{
		tmp &=  R300_MEM_NUM_CHANNELS_MASK;
		switch(tmp)
		{
			case 0:  rinfo->vram_width = 64; break;
			case 1:  rinfo->vram_width = 128; break;
			case 2:  rinfo->vram_width = 256; break;
			default: rinfo->vram_width = 128; break;
		}
	}
	else if ((rinfo->family == CHIP_FAMILY_RV100)
	 || (rinfo->family == CHIP_FAMILY_RS100)
	 || (rinfo->family == CHIP_FAMILY_RS200))
	{
		if (tmp & RV100_MEM_HALF_MODE)
			rinfo->vram_width = 32;
		else
			rinfo->vram_width = 64;
	}
	else
	{
		if (tmp & MEM_NUM_CHANNELS_MASK)
			rinfo->vram_width = 128;
		else
			rinfo->vram_width = 64;
	}

	/*
	 * This may not be correct, as some cards can have half of channel disabled
	 * ToDo: identify these cases
	 */
	switch(rinfo->family)
	{
		case CHIP_FAMILY_LEGACY: dbg("%s chip type: %s\r\n", __FUNCTION__, "LEGACY"); break;	
		case CHIP_FAMILY_RADEON: dbg("%s chip type: %s\r\n", __FUNCTION__, "RADEON"); break;	
		case CHIP_FAMILY_RV100: dbg("%s chip type: %s\r\n", __FUNCTION__, "RV100"); break;	
		case CHIP_FAMILY_RS100: dbg("%s chip type: %s\r\n", __FUNCTION__, "RS100"); break;	
		case CHIP_FAMILY_RV200: dbg("%s chip type: %s\r\n", __FUNCTION__, "RV200"); break;	
		case CHIP_FAMILY_RS200: dbg("%s chip type: %s\r\n", __FUNCTION__, "RS200"); break;	
		case CHIP_FAMILY_R200: dbg("%s chip type: %s\r\n", __FUNCTION__, "R200"); break;	
		case CHIP_FAMILY_RV250: dbg("%s chip type: %s\r\n", __FUNCTION__, "RV250"); break;	
		case CHIP_FAMILY_RS300: dbg("%s chip type: %s\r\n", __FUNCTION__, "RS300"); break;	
		case CHIP_FAMILY_RV280: dbg("%s chip type: %s\r\n", __FUNCTION__, "RV280"); break;	
		case CHIP_FAMILY_R300: dbg("%s chip type: %s\r\n", __FUNCTION__, "R300"); break;	
		case CHIP_FAMILY_R350: dbg("%s chip type: %s\r\n", __FUNCTION__, "R350"); break;	
		case CHIP_FAMILY_RV350: dbg("%s chip type: %s\r\n", __FUNCTION__, "RV350"); break;	
		case CHIP_FAMILY_RV380: dbg("%s chip type: %s\r\n", __FUNCTION__, "RV380"); break;	
		case CHIP_FAMILY_R420: dbg("%s chip type: %s\r\n", __FUNCTION__, "R420"); break;	
		default: dbg("%s chip type: %s\r\n", "UNKNOW"); break;
	}	
	dbg("%s: found %d KB of %d bits wide %s video RAM\r\n", __FUNCTION__, rinfo->video_ram / 1024,
				rinfo->vram_width, rinfo->vram_ddr ? "DDR " : "SDRAM ");
}

int32_t radeonfb_pci_register(int32_t handle, const struct pci_device_id *ent)
{
	struct fb_info *info;
	struct radeonfb_info *rinfo;
	struct pci_rd *pci_rsc_desc;

	dbg("%s:\r\n", __FUNCTION__);
	info = framebuffer_alloc(sizeof(struct radeonfb_info));
	if (!info)
	{
		dbg("%s: could not allocate frame buffer\r\n", __FUNCTION__);
		return -1; // -ENOMEM;
	}

	rinfo = info->par;

	rinfo->info = info;
	rinfo->handle = handle;
	rinfo->name[11] = (char) (ent->device >> 8);
	rinfo->name[12] = (char) ent->device;
	rinfo->family = ent->driver_data & CHIP_FAMILY_MASK;
	rinfo->chipset = ent->device;
	rinfo->has_CRTC2 = (ent->driver_data & CHIP_HAS_CRTC2) != 0;
	rinfo->is_mobility = (ent->driver_data & CHIP_IS_MOBILITY) != 0;
	rinfo->is_IGP = (ent->driver_data & CHIP_IS_IGP) != 0;

	/* Set base addrs */
	dbg("%s: Set base addrs\r\n", __FUNCTION__);
	rinfo->fb_base_phys = rinfo->mmio_base_phys = rinfo->io_base_phys = 0xFFFFFFFF;
	rinfo->mapped_vram = 0;
	rinfo->mmio_base = rinfo->io_base = NULL;
	rinfo->bios_seg = NULL;

	pci_rsc_desc = pci_get_resource(handle);
	if ((int32_t) pci_rsc_desc >= 0)
	{
		uint16_t flags;
		do
		{
			dbg("%s: flags %p\r\n", __FUNCTION__, pci_rsc_desc->flags);
			dbg("%s:	start %p\r\n", __FUNCTION__, pci_rsc_desc->start);
			dbg("%s:	offset 0x%x\r\n", __FUNCTION__, pci_rsc_desc->offset);
			dbg("%s:	length 0x%x\r\n", __FUNCTION__, pci_rsc_desc->length);

			if (!(pci_rsc_desc->flags & FLG_IO))
			{
				if ((rinfo->fb_base_phys == 0xFFFFFFFF) && (pci_rsc_desc->length >= 0x100000))
				{
					rinfo->fb_base = (void *)(pci_rsc_desc->offset + pci_rsc_desc->start);
					rinfo->fb_base_phys = pci_rsc_desc->start;
					rinfo->mapped_vram = pci_rsc_desc->length;
//					rinfo->dma_offset = pci_rsc_desc->dmaoffset;
					if ((pci_rsc_desc->flags & FLG_ENDMASK) == ORD_MOTOROLA)
					{
						rinfo->big_endian = 0; /* host bridge make swapping intel -> motorola */
						dbg("%s: host bridge is big endian\r\n", __FUNCTION__);
					}
					else
					{
						rinfo->big_endian = 1; /* radeon make swapping intel -> motorola */
						dbg("%s: host bridge is little endian\r\n", __FUNCTION__);
					}
				}
				else if ((pci_rsc_desc->length >= RADEON_REGSIZE) && (pci_rsc_desc->length < 0x100000))
				{
					if (pci_rsc_desc->flags & FLG_ROM)
					{
						dbg("%s: FLG_ROM resource descriptor found\r\n", __FUNCTION__);
						dbg("%s:	start = %p, size = 0x%x\r\n", __FUNCTION__, pci_rsc_desc->start, pci_rsc_desc->length);
						dbg("%s:	bios_seg = %p\r\n", __FUNCTION__, rinfo->bios_seg);
						
						if (rinfo->bios_seg == NULL)
						{
							rinfo->bios_seg_phys = pci_rsc_desc->start;
							if (BIOS_IN16(0) == 0xaa55)
								rinfo->bios_seg = (void *) (pci_rsc_desc->offset + pci_rsc_desc->start);
							else
							{
								dbg("%s: BIOS_IN16(0) was %x (expected 0xaa55)\r\n", __FUNCTION__, BIOS_IN16(0));
								rinfo->bios_seg_phys = 0;
							}
						}
					}
					else
					{
						if (rinfo->mmio_base_phys == 0xFFFFFFFF)
						{		
							rinfo->mmio_base = (void *)(pci_rsc_desc->offset + pci_rsc_desc->start);
							rinfo->mmio_base_phys = pci_rsc_desc->start;
						}
					}
				}
			}
			else
			{
				if (rinfo->io_base_phys == 0xFFFFFFFF)
				{		
					rinfo->io_base = (void *)(pci_rsc_desc->offset + pci_rsc_desc->start);
					rinfo->io_base_phys = pci_rsc_desc->start;
				}
			}
			flags = pci_rsc_desc->flags;
			pci_rsc_desc = (struct pci_rd *) ((uint32_t) pci_rsc_desc->next + (uint32_t) pci_rsc_desc);
		} while (!(flags & FLG_LAST));
	}
	else
		dbg("%s: get_resource error\r\n", __FUNCTION__);

	/* map the regions */
	dbg("%s: map memory regions\r\n", __FUNCTION__);
	if (rinfo->mmio_base == NULL)
	{
		dbg("%s: cannot map MMIO\r\n", __FUNCTION__);
		framebuffer_release(info);
		return -2; //(-EIO);	
	}
	dbg("%s: mmio_base_phys %p, mmio_base %p\r\n", __FUNCTION__, rinfo->mmio_base_phys, rinfo->mmio_base);
	dbg("%s: io_base_phys %p, io_base %p\r\n", __FUNCTION__, rinfo->io_base_phys, rinfo->io_base);
	dbg("%s: fb_base_phys %p, fb_base %p\r\n", __FUNCTION__, rinfo->fb_base_phys, rinfo->fb_base);

	/*
	 * Check for errata
	 */
	dbg("%s: check for errata\r\n", __FUNCTION__);
	rinfo->errata = 0;
	if (rinfo->family == CHIP_FAMILY_R300
				&& (INREG(CONFIG_CNTL) & CFG_ATI_REV_ID_MASK) == CFG_ATI_REV_A11)
		rinfo->errata |= CHIP_ERRATA_R300_CG;
	if (rinfo->family == CHIP_FAMILY_RV200 || rinfo->family == CHIP_FAMILY_RS200)
		rinfo->errata |= CHIP_ERRATA_PLL_DUMMYREADS;
	if (rinfo->family == CHIP_FAMILY_RV100
				|| rinfo->family == CHIP_FAMILY_RS100
				|| rinfo->family == CHIP_FAMILY_RS200)
		rinfo->errata |= CHIP_ERRATA_PLL_DELAY;

	/*
	 * Map the BIOS ROM if any and retreive PLL parameters from
	 * the BIOS.
	 */
	dbg("%s: bios_seg_phys %p\r\n", __FUNCTION__, rinfo->bios_seg_phys);
	dbg("%s: map the BIOS ROM\r\n", __FUNCTION__);
	radeon_map_ROM(rinfo);

	/* Run VGA BIOS */
	if ((rinfo->bios_seg != NULL))
	{
		dbg("%s: run VGA BIOS\r\n", __FUNCTION__);
		run_bios(rinfo);
	}
	else
	{
		dbg("%s: could not run VGA bios - rinfo->bios_seg is NULL\r\n", __FUNCTION__);
	}

	dbg("%s: fixup display base address \r\n", __FUNCTION__);

	OUTREG(MC_FB_LOCATION, 0x7fff0000);
	rinfo->fb_local_base = 0;

	/* Fixup the display base addresses & engine offsets while we
	 * are at it as well
	 */
	OUTREG(DISPLAY_BASE_ADDR, 0);
	if (rinfo->has_CRTC2)
		OUTREG(CRTC2_DISPLAY_BASE_ADDR, 0);

	OUTREG(OV0_BASE_ADDR, 0);

	/* Get VRAM size and type */
	dbg("%s: get VRAM size\r\n", __FUNCTION__);
	radeon_identify_vram(rinfo);

	if ((rinfo->fb_base == NULL)
			|| ((rinfo->video_ram > rinfo->mapped_vram) && (rinfo->mapped_vram < MIN_MAPPED_VRAM * 2)))
	{
		dbg("%s: cannot map FB, video ram: %d KB\r\n", __FUNCTION__, rinfo->mapped_vram / 1024);
		framebuffer_release(info);
		return -2; //(-EIO);
	}
	else
	{
		dbg("%s: %d KB of VRAM mapped to %p\r\n", __FUNCTION__, rinfo->mapped_vram / 1024, rinfo->fb_base);
	}
	
	/* Get informations about the board's PLL */
	dbg("%s: get informations about the board's PLL\r\n", __FUNCTION__);
	radeon_get_pllinfo(rinfo);

#ifdef CONFIG_FB_RADEON_I2C
	/* Register I2C bus */
	dbg("%s: register I2C bus\r\n", __FUNCTION__);
	radeon_create_i2c_busses(rinfo);
#endif /* CONFIG_FB_RADEON_I2C */

	/* set all the vital stuff */
	dbg("%s: set all the vital stuff\r\n", __FUNCTION__);
	radeon_set_fbinfo(rinfo);

	/* set offscreen memory descriptor */
	dbg("%s: set offscreen memory descriptor\r\n", __FUNCTION__);
	offscreen_init(info);

	/* Probe screen types */
	dbg("%s: probe screen types, monitor_layout: 0x%x\r\n", __FUNCTION__, monitor_layout);
	radeon_probe_screens(rinfo, monitor_layout, (int) ignore_edid);

	/* Build mode list, check out panel native model */
	dbg("%s: build mode list\r\n", __FUNCTION__);
	radeon_check_modes(rinfo, &resolution);

	/*
	 * save current mode regs before we switch into the new one
	 * so we can restore this upon exit
	 */
	dbg("%s: save current mode\r\n", __FUNCTION__);
	radeon_save_state(rinfo, &rinfo->init_state);
	memcpy(&rinfo->state, &rinfo->init_state, sizeof(struct radeon_regs));

	/* Setup Power Management capabilities */
//	DPRINT("radeonfb: radeonfb_pci_register: setup power management\r\n");
//	radeonfb_pm_init(rinfo, (int)default_dynclk);

	dbg("%s: install VBL timer\r\n", __FUNCTION__);
	rinfo->lvds_timer = 0;
#ifndef DRIVER_IN_ROM
	install_vbl_timer(radeon_timer_func, 1); /* remove old vector */ 
#else
	install_vbl_timer(radeon_timer_func, 0);
#endif
	//rinfo->RageTheatreCrystal = rinfo->RageTheatreTunerPort=rinfo->RageTheatreCompositePort = rinfo->RageTheatreSVideoPort = -1;
	//rinfo->tunerType = -1;
	return(0);
}

#if 0

void radeonfb_pci_unregister(void)
{
	struct fb_info *info = info_fb;
	struct radeonfb_info *rinfo = info->par;
//	radeonfb_pm_exit(rinfo);
	uninstall_vbl_timer(radeon_timer_func);
	if (rinfo->mon1_EDID!=NULL)
		driver_mem_free(rinfo->mon1_EDID);
	if (rinfo->mon2_EDID!=NULL)
		driver_mem_free(rinfo->mon2_EDID);
	if (rinfo->mon1_modedb)
		fb_destroy_modedb(rinfo->mon1_modedb);
#ifdef CONFIG_FB_RADEON_I2C
	radeon_delete_i2c_busses(rinfo);
#endif        
	framebuffer_release(info);
}

#endif


