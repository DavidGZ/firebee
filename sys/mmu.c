#include "mmu.h"
#include "acia.h"
#include "exceptions.h"

/*
 * mmu.c
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
 * derived from original assembler sources:
 * Copyright 2010 - 2012 F. Aschwanden
 * Copyright 2013        M. Froeschle
 */

#define ACR_BA(x)                   ((x) & 0xffff0000)
#define ACR_ADMSK(x)                (((x) & 0xffff) << 16)
#define ACR_E(x)                    (((x) & 1) << 15)

#define ACR_S(x)                    (((x) & 3) << 13)
#define ACR_S_USERMODE              0
#define ACR_S_SUPERVISOR_MODE       1
#define ACR_S_ALL                   2

#define ACR_AMM(x)                  (((x) & 1) << 10)

#define ACR_CM(x)                   (((x) & 3) << 5)
#define ACR_CM_CACHEABLE_WT         0x0
#define ACR_CM_CACHEABLE_CB         0x1
#define ACR_CM_CACHE_INH_PRECISE    0x2
#define ACR_CM_CACHE_INH_IMPRECISE  0x3

#define ACR_SP(x)           (((x) & 1) << 3)
#define ACR_W(x)            (((x) & 1) << 2)

#include <stdint.h>
#include "bas_printf.h"
#include "bas_types.h"
#include "MCF5475.h"
#include "pci.h"
#include "cache.h"
#include "util.h"

#if defined(MACHINE_FIREBEE)
#include "firebee.h"
#elif defined(MACHINE_M5484LITE)
#include "m5484l.h"
#elif defined(MACHINE_M54455)
#include "m54455.h"
#else
#error "unknown machine!"
#endif /* MACHINE_FIREBEE */

#define DBG_MMU
#ifdef DBG_MMU
#define dbg(format, arg...) do { xprintf("DEBUG (%s()): " format, __FUNCTION__, ##arg);} while(0)
#else
#define dbg(format, arg...) do {;} while (0)
#endif /* DBG_MMU */
#define err(format, arg...) do { xprintf("ERROR (%s()): " format, __FUNCTION__, ##arg); xprintf("system halted\r\n"); } while(0); while(1)


/*
 * set ASID register
 * saves new value to rt_asid and returns former value
 */
inline uint32_t set_asid(uint32_t value)
{
    extern long rt_asid;
    uint32_t ret = rt_asid;

    __asm__ __volatile__(
        "movec		%[value],ASID\n\t"
        : /* no output */
        : [value] "r" (value)
        : "memory"
    );

    rt_asid = value;

    return ret;
}


/*
 * set ACRx register
 * saves new value to rt_acrx and returns former value
 */
inline uint32_t set_acr0(uint32_t value)
{
    extern uint32_t rt_acr0;
    uint32_t ret = rt_acr0;

    __asm__ __volatile__(
        "movec		%[value],ACR0\n\t"
        : /* not output */
        : [value] "r" (value)
        : "memory"
    );
    rt_acr0 = value;

    return ret;
}

/*
 * set ACRx register
 * saves new value to rt_acrx and returns former value
 */
inline uint32_t set_acr1(uint32_t value)
{
    extern uint32_t rt_acr1;
    uint32_t ret = rt_acr1;

    __asm__ __volatile__(
        "movec		%[value],ACR1\n\t"
        : /* not output */
        : [value] "r" (value)
        : "memory"
    );
    rt_acr1 = value;

    return ret;
}


/*
 * set ACRx register
 * saves new value to rt_acrx and returns former value
 */
inline uint32_t set_acr2(uint32_t value)
{
    extern uint32_t rt_acr2;
    uint32_t ret = rt_acr2;

    __asm__ __volatile__(
        "movec		%[value],ACR2\n\t"
        : /* not output */
        : [value] "r" (value)
        : "memory"
    );
    rt_acr2 = value;

    return ret;
}

/*
 * set ACRx register
 * saves new value to rt_acrx and returns former value
 */
inline uint32_t set_acr3(uint32_t value)
{
    extern uint32_t rt_acr3;
    uint32_t ret = rt_acr3;

    __asm__ __volatile__(
        "movec		%[value],ACR3\n\t"
        : /* not output */
        : [value] "r" (value)
        : "memory"
    );
    rt_acr3 = value;

    return ret;
}

inline uint32_t set_mmubar(uint32_t value)
{
    extern uint32_t rt_mmubar;
    uint32_t ret = rt_mmubar;

    __asm__ __volatile__(
        "movec		%[value],MMUBAR\n\t"
        : /* no output */
        : [value] "r" (value)
        : "memory"
    );
    rt_mmubar = value;
    NOP();

    return ret;
}

/*
 * translation table for virtual address ranges. Holds the physical_offset (which must be added to a virtual
 * address to get its physical counterpart) for memory ranges.
 */
struct virt_to_phys
{
    uint32_t start_address;
    uint32_t length;
    uint32_t physical_offset;
};

static struct virt_to_phys translation[] =
{
    /* virtual  , length    , offset    */
    { 0x00000000, 0x00e00000, 0x60000000 },     /* map first 14 MByte to first 14 Mb of video ram */
    { 0x00e00000, 0x00100000, 0x00000000 },     /* map TOS to SDRAM */
    { 0x00f00000, 0x00100000, 0xff000000 },     /* map Falcon I/O area to FPGA */
    { 0x01000000, 0x1f000000, 0x00000000 },     /* map rest of ram virt = phys */
};
static int num_translations = sizeof(translation) / sizeof(struct virt_to_phys);

static inline uint32_t lookup_phys(uint32_t virt)
{
    int i;

    for (i = 0; i < num_translations; i++)
    {
        if (virt >= translation[i].start_address && virt < translation[i].start_address + translation[i].length)
        {
            return virt + translation[i].physical_offset;
        }
    }
    err("virtual address 0x%lx not found in translation table!\r\n", virt);
    return -1;
}

struct page_descriptor
{
    uint8_t cache_mode          : 2;
    uint8_t supervisor_protect  : 1;
    uint8_t read                : 1;
    uint8_t write               : 1;
    uint8_t execute             : 1;
    uint8_t global              : 1;
    uint8_t locked              : 1;
};

static struct page_descriptor pages[65536];   /* 512 Mb RAM */

/*
 * map a page of memory using virt addresses with the Coldfire MMU.
 *
 * Theory of operation: the Coldfire MMU in the Firebee has 64 TLB entries, 32 for data (DTLB), 32 for
 * instructions (ITLB). Mappings can either be done locked (normal MMU TLB misses will not consider them
 * for replacement) or unlocked (mappings will reallocate using a LRU scheme when the MMU runs out of
 * TLB entries). For proper operation, the MMU needs at least two ITLBs and/or four free/allocatable DTLBs
 * per instruction as a minimum, more for performance. Thus locked pages (that can't be touched by the
 * LRU algorithm) should be used sparsingly.
 *
 *
 */
int mmu_map_8k_page(uint32_t virt, uint8_t asid)
{
    const uint32_t size_mask = 0xffffe000;                      /* 8k pagesize */
    int page_index = (virt & size_mask) / DEFAULT_PAGE_SIZE;	/* index into page_descriptor array */
    struct page_descriptor *page = &pages[page_index];          /* attributes of page to map */

    uint32_t phys = lookup_phys(virt);                          /* virtual to physical translation of page */

    if (phys == -1)
        return 0;

#ifdef DBG_MMU
    register int sp asm("sp");
    dbg("page_descriptor: 0x%02x, ssp = 0x%08x\r\n", * (uint8_t *) page, sp);
#endif /* DBG_MMU */
    /*
     * add page to TLB
     */
    MCF_MMU_MMUTR = (virt & 0xfffffc00) |                           /* virtual address */
        MCF_MMU_MMUTR_ID(asid) |        							/* address space id (ASID) */
        (page->global ? MCF_MMU_MMUTR_SG : 0) |						/* shared global */
        MCF_MMU_MMUTR_V;											/* valid */

    MCF_MMU_MMUDR = (phys & 0xfffffc00) |                 			/* physical address */
        MCF_MMU_MMUDR_SZ(MMU_PAGE_SIZE_8K) |						/* page size */
        MCF_MMU_MMUDR_CM(page->cache_mode) |                        /* cache mode */
        (page->supervisor_protect ? MCF_MMU_MMUDR_SP : 0) |         /* supervisor protect */
        (page->read ? MCF_MMU_MMUDR_R : 0) |                    	/* read access enable */
        (page->write ? MCF_MMU_MMUDR_W : 0) |                       /* write access enable */
        (page->execute ? MCF_MMU_MMUDR_X : 0) |                     /* execute access enable */
        (page->locked ? MCF_MMU_MMUDR_LK : 0);

    MCF_MMU_MMUOR = MCF_MMU_MMUOR_ITLB | 	/* instruction */
        MCF_MMU_MMUOR_ACC |     			/* access TLB */
        MCF_MMU_MMUOR_UAA;      			/* update allocation address field */
    dbg("ITLB: MCF_MMU_MMUOR = %08x\r\n", MCF_MMU_MMUOR);

    MCF_MMU_MMUOR = MCF_MMU_MMUOR_ACC |		/* access TLB, data */
        MCF_MMU_MMUOR_UAA;					/* update allocation address field */

    dbg("mapped virt=0x%08x to phys=0x%08x\r\n", virt & size_mask, phys & size_mask);

    dbg("DTLB: MCF_MMU_MMUOR = %08x\r\n", MCF_MMU_MMUOR);

    return 1;
}

int mmu_map_8k_instruction_page(uint32_t virt, uint8_t asid)
{
    const uint32_t size_mask = ~ (DEFAULT_PAGE_SIZE - 1);       /* 8k pagesize */
    int page_index = (virt & size_mask) / DEFAULT_PAGE_SIZE;	/* index into page_descriptor array */
    struct page_descriptor *page = &pages[page_index];          /* attributes of page to map */
    int ipl;
    uint32_t phys = lookup_phys(virt);                          /* virtual to physical translation of page */

    if (phys == -1)
        return 0;

#ifdef DBG_MMU
    register int sp asm("sp");
    dbg("page_descriptor: 0x%02x, ssp = 0x%08x\r\n", * (uint8_t *) page, sp);
#endif /* DBG_MMU */

    /*
     * add page to TLB
     */

    ipl = set_ipl(7);                                               /* do not disturb */

    MCF_MMU_MMUAR = (virt & size_mask);

    MCF_MMU_MMUTR = (virt & size_mask) |                            /* virtual address */
        MCF_MMU_MMUTR_ID(asid) |        							/* address space id (ASID) */
        (page->global ? MCF_MMU_MMUTR_SG : 0) |						/* shared global */
        MCF_MMU_MMUTR_V;											/* valid */

    __asm__ __volatile("" : : : "memory");                          /* MMU commands must be exactly in sequence */

    MCF_MMU_MMUDR = (phys & size_mask) |                 			/* physical address */
        MCF_MMU_MMUDR_SZ(MMU_PAGE_SIZE_8K) |						/* page size */
        MCF_MMU_MMUDR_CM(page->cache_mode) |                        /* cache mode */
        (page->supervisor_protect ? MCF_MMU_MMUDR_SP : 0) |         /* supervisor protect */
        (page->read ? MCF_MMU_MMUDR_R : 0) |                    	/* read access enable */
        (page->write ? MCF_MMU_MMUDR_W : 0) |                       /* write access enable */
        (page->execute ? MCF_MMU_MMUDR_X : 0) |                     /* execute access enable */
        (page->locked ? MCF_MMU_MMUDR_LK : 0);

    __asm__ __volatile("" : : : "memory");                          /* MMU commands must be exactly in sequence */

    MCF_MMU_MMUOR = MCF_MMU_MMUOR_ITLB | 	/* instruction */
        MCF_MMU_MMUOR_ACC |     			/* access TLB */
        MCF_MMU_MMUOR_UAA;      			/* update allocation address field */

    __asm__ __volatile("" : : : "memory");                          /* MMU commands must be exactly in sequence */

    set_ipl(ipl);

    dbg("mapped virt=0x%08x to phys=0x%08x\r\n", virt & size_mask, phys & size_mask);

    dbg("ITLB: MCF_MMU_MMUOR = %08x\r\n", MCF_MMU_MMUOR);
    return 1;
}

int mmu_map_8k_data_page(uint32_t virt, uint8_t asid)
{
    uint16_t ipl;
    const uint32_t size_mask = ~ (DEFAULT_PAGE_SIZE - 1);       /* 8k pagesize */
    int page_index = (virt & size_mask) / DEFAULT_PAGE_SIZE;	/* index into page_descriptor array */
    struct page_descriptor *page = &pages[page_index];          /* attributes of page to map */

    uint32_t phys = lookup_phys(virt);                          /* virtual to physical translation of page */

    if (phys == -1)
        return 0;

#ifdef DBG_MMU
    register int sp asm("sp");
    dbg("page_descriptor: 0x%02x, ssp = 0x%08x\r\n", * (uint8_t *) page, sp);
#endif /* DBG_MMU */

    /*
     * add page to TLB
     */

    ipl = set_ipl(7);                                               /* do not disturb */

    MCF_MMU_MMUTR = (virt & size_mask) |                            /* virtual address */
        MCF_MMU_MMUTR_ID(asid) |        							/* address space id (ASID) */
        (page->global ? MCF_MMU_MMUTR_SG : 0) |						/* shared global */
        MCF_MMU_MMUTR_V;											/* valid */

    MCF_MMU_MMUDR = (phys & size_mask) |                 			/* physical address */
        MCF_MMU_MMUDR_SZ(MMU_PAGE_SIZE_8K) |						/* page size */
        MCF_MMU_MMUDR_CM(page->cache_mode) |                        /* cache mode */
        (page->supervisor_protect ? MCF_MMU_MMUDR_SP : 0) |         /* supervisor protect */
        (page->read ? MCF_MMU_MMUDR_R : 0) |                    	/* read access enable */
        (page->write ? MCF_MMU_MMUDR_W : 0) |                       /* write access enable */
        (page->execute ? MCF_MMU_MMUDR_X : 0) |                     /* execute access enable */
        (page->locked ? MCF_MMU_MMUDR_LK : 0);

    MCF_MMU_MMUOR = MCF_MMU_MMUOR_ACC |		/* access TLB, data */
        MCF_MMU_MMUOR_UAA;					/* update allocation address field */

    set_ipl(ipl);
    dbg("mapped virt=0x%08x to phys=0x%08x\r\n", virt & size_mask, phys & size_mask);

    dbg("DTLB: MCF_MMU_MMUOR = %08x\r\n", MCF_MMU_MMUOR);

    return 1;
}

/*
 * map a page of memory using virt and phys as addresses with the Coldfire MMU.
 *
 * Theory of operation: the Coldfire MMU in the Firebee has 64 TLB entries, 32 for data (DTLB), 32 for
 * instructions (ITLB). Mappings can either be done locked (normal MMU TLB misses will not consider them
 * for replacement) or unlocked (mappings will reallocate using a LRU scheme when the MMU runs out of
 * TLB entries). For proper operation, the MMU needs at least two ITLBs and/or four free/allocatable DTLBs
 * per instruction as a minimum, more for performance. Thus locked pages (that can't be touched by the
 * LRU algorithm) should be used sparsingly.
 *
 *
 */
int mmu_map_page(uint32_t virt, uint32_t phys, enum mmu_page_size sz, uint8_t page_id, const struct page_descriptor *flags)
{
    int size_mask;
    int ipl;

    switch (sz)
    {
        case MMU_PAGE_SIZE_1M:
            size_mask = ~ (0x00010000 - 1);
            break;

        case MMU_PAGE_SIZE_8K:
            size_mask = ~ (0x2000 - 1);
            break;

        case MMU_PAGE_SIZE_4K:
            size_mask = ~ (0x1000 - 1);
            break;

        case MMU_PAGE_SIZE_1K:
            size_mask = ~ (0x400 - 1);
            break;

        default:
            err("illegal map size %d\r\n", sz);
    }

    /*
     * add page to TLB
     */

    ipl = set_ipl(7);

    MCF_MMU_MMUTR = ((uint32_t) virt & size_mask) |					/* virtual address */
        MCF_MMU_MMUTR_ID(page_id) |                                 /* address space id (ASID) */
        (flags->global ? MCF_MMU_MMUTR_SG : 0) |					/* shared global */
        MCF_MMU_MMUTR_V;											/* valid */
    NOP();

    MCF_MMU_MMUDR = ((uint32_t) phys & size_mask) |					/* physical address */
        MCF_MMU_MMUDR_SZ(sz) |										/* page size */
        MCF_MMU_MMUDR_CM(flags->cache_mode) |
        (flags->read ? MCF_MMU_MMUDR_R : 0) |                       /* read access enable */
        (flags->write ? MCF_MMU_MMUDR_W : 0) |                      /* write access enable */
        (flags->execute ? MCF_MMU_MMUDR_X : 0) |                    /* execute access enable */
        (flags->locked ? MCF_MMU_MMUDR_LK : 0);
    NOP();

    MCF_MMU_MMUOR = MCF_MMU_MMUOR_ACC |		/* access TLB, data */
        MCF_MMU_MMUOR_UAA;					/* update allocation address field */
    NOP();

    MCF_MMU_MMUOR = MCF_MMU_MMUOR_ITLB | 	/* instruction */
        MCF_MMU_MMUOR_ACC |     			/* access TLB */
        MCF_MMU_MMUOR_UAA;      			/* update allocation address field */

    set_ipl(ipl);

    dbg("mapped virt=0x%08x to phys=0x%08x\r\n", virt, phys);

    return 1;
}

void mmu_init(void)
{
    extern uint8_t _MMUBAR[];
    uint32_t MMUBAR = (uint32_t) &_MMUBAR[0];
    struct page_descriptor flags;
    int i;

    /*
     * clear all MMU TLB entries first
     */
    MCF_MMU_MMUOR = MCF_MMU_MMUOR_CA;       /* clears _all_ TLBs (including locked ones) */
    NOP();

    /*
     * prelaminary initialization of page descriptor 0 (root) table
     */
    for (i = 0; i < sizeof(pages) / sizeof(struct page_descriptor); i++)
    {
        uint32_t addr = i * DEFAULT_PAGE_SIZE;

        if (addr >= 0x00f00000 && addr < 0x00ffffff)
        {
            pages[i].cache_mode = CACHE_NOCACHE_PRECISE;
            pages[i].execute = 0;
            pages[i].read = 1;
            pages[i].write = 1;
            pages[i].execute = 0;
            pages[i].global = 1;
            pages[i].supervisor_protect = 1;
        }
        else if (addr >= 0x0 && addr < 0x00e00000)      /* ST-RAM, potential video memory */
        {
            pages[i].cache_mode = CACHE_WRITETHROUGH;
            pages[i].execute = 1;
            pages[i].supervisor_protect = 0;
            pages[i].read = 1;
            pages[i].write = 1;
            pages[i].execute = 1;
            pages[i].global = 1;
        }
        else if (addr >= 0x00e00000 && addr < 0x00f00000)      /* EmuTOS */
        {
            pages[i].cache_mode = CACHE_COPYBACK;
            pages[i].execute = 1;
            pages[i].supervisor_protect = 1;
            pages[i].read = 1;
            pages[i].write = 0;
            pages[i].execute = 1;
            pages[i].global = 1;
        }
        else
        {
            pages[i].cache_mode = CACHE_COPYBACK;
            pages[i].execute = 1;
            pages[i].read = 1;
            pages[i].write = 1;
            pages[i].supervisor_protect = 0;
            pages[i].global = 1;
        }
        pages[i].locked = 0;				/* not locked */
        pages[0].supervisor_protect = 0;    /* protect system vectors */
    }

    set_asid(0);			/* do not use address extension (ASID provides virtual 48 bit addresses) yet */


    /* set data access attributes in ACR0 and ACR1 */
    set_acr0(ACR_W(0) |								/* read and write accesses permitted */
            ACR_SP(0) |								/* supervisor and user mode access permitted */
            ACR_CM(ACR_CM_CACHE_INH_PRECISE) |		/* cache inhibit, precise (i/o area!) */
            ACR_AMM(0) |							/* control region > 16 MB */
            ACR_S(ACR_S_ALL) |						/* match addresses in user and supervisor mode */
            ACR_E(1) |								/* enable ACR */
#if defined(MACHINE_FIREBEE)
            ACR_ADMSK(0x7f) |						/* cover 2GB area from 0x80000000 to 0xffffffff */
            ACR_BA(0x80000000));					/* (equals area from 3 to 4 GB */
#elif defined(MACHINE_M5484LITE)
            ACR_ADMSK(0x7f) |						/* cover 2 GB area from 0x80000000 to 0xffffffff */
            ACR_BA(0x80000000));
#elif defined(MACHINE_M54455)
            ACR_ADMSK(0x7f) |
            ACR_BA(0x80000000));					/* FIXME: not determined yet for this machine */
#else
#error unknown machine!
#endif /* MACHINE_FIREBEE */



    // set_acr1(0x601fc000);
    set_acr1(ACR_W(0) |
            ACR_SP(0) |
            ACR_CM(0) |
#if defined(MACHINE_FIREBEE)
            ACR_CM(ACR_CM_CACHE_INH_PRECISE) |		/* ST RAM on the Firebee */
#elif defined(MACHINE_M5484LITE)
            ACR_CM(ACR_CM_CACHE_INH_PRECISE) |		/* Compact Flash on the M548xLITE */
#elif defined(MACHINE_M54455)
            ACR_CM(ACR_CM_CACHE_INH_PRECISE) |		/* FIXME: not determined yet for this machine */
#else
#error unknown machine!
#endif /* MACHINE_FIREBEE */
            ACR_AMM(0) |
            ACR_S(ACR_S_ALL) |
            ACR_E(1) |
            ACR_ADMSK(0x7f) |
            ACR_BA(0x00100000));

#ifdef _NOT_USED_
    /* set instruction access attributes in ACR2 and ACR3 */

    //set_acr2(0xe007c400);							/* flash area */
    set_acr2(ACR_W(0) |
            ACR_SP(0) |
            ACR_CM(0) |
            ACR_CM(ACR_CM_CACHE_INH_PRECISE) |
            ACR_AMM(1) |
            ACR_S(ACR_S_ALL) |
            ACR_E(1) |
            ACR_ADMSK(0x7) |
            ACR_BA(0xe0000000));
#endif /* _NOT_USED_ */

    set_acr1(0x0);
    set_acr2(0x0);
    /* disable ACR3 */
    set_acr3(0x0);

    set_mmubar(MMUBAR + 1);		/* set and enable MMUBAR */

    /* clear all MMU TLB entries */
    MCF_MMU_MMUOR = MCF_MMU_MMUOR_CA;

    /* create locked TLB entries */

    /*
     * Map (locked) the second last MB of physical SDRAM (this is where BaS .data and .bss reside) to the same
     * virtual address. This is also used (completely) when BaS is in RAM
     */
    flags.cache_mode = CACHE_NOCACHE_PRECISE;
    flags.read = 1;
    flags.write = 1;
    flags.execute = 1;
    flags.supervisor_protect = 1;		/* supervisor access only */
    flags.locked = 1;
    mmu_map_page(SDRAM_START + SDRAM_SIZE - 0x00200000, SDRAM_START + SDRAM_SIZE - 0x00200000, 0, MMU_PAGE_SIZE_1M, &flags);

    /*
     * map EmuTOS (locked for now)
     */
    flags.read = 1;
    flags.write = 1;
    flags.execute = 1;
    flags.locked = 1;
    //mmu_map_page(0xe00000, 0xe00000, MMU_PAGE_SIZE_1M, 0, &flags);

    /*
     * Map (locked) the very last MB of physical SDRAM (this is where the driver buffers reside) to the same
     * virtual address. Used uncached for drivers.
     */
    flags.cache_mode = CACHE_NOCACHE_PRECISE;
    flags.read = 1;
    flags.write = 1;
    flags.execute = 0;
    flags.supervisor_protect = 1;
    flags.locked = 1;
    mmu_map_page(SDRAM_START + SDRAM_SIZE - 0x00100000, SDRAM_START + SDRAM_SIZE - 0x00100000, 0, MMU_PAGE_SIZE_1M, &flags);
}

/*
 * enable the MMU. The Coldfire MMU can be used in two different modes
 * ... FIXME:
 */
void mmu_enable(void)
{
    MCF_MMU_MMUCR = MCF_MMU_MMUCR_EN;			/* MMU on */
    NOP();                                      /* force pipeline sync */
}

#ifdef DBG_MMU
void verify_mapping(uint32_t address)
{
    /* retrieve mapped page from MMU and make sure everything is correct */
    int ds;

    ds = * (int *) address;
    dbg("found 0x%08x at address\r\n", ds);
}
#endif /* DBG_MMU */

uint32_t mmutr_miss(uint32_t mmu_sr, uint32_t fault_address, uint32_t pc,
                uint32_t format_status)
{
    uint32_t fault = format_status & 0x0c030000;

    switch (fault)
    {
        /* if we have a real TLB miss, map the offending page */

        case 0x04010000:    /* TLB miss on opword of instruction fetch */
        case 0x04020000:    /* TLB miss on extension word of instruction fetch */
            dbg("MMU ITLB MISS accessing 0x%08x\r\n"
                "FS = 0x%08x\r\n"
                "MMUSR = 0x%08x\r\n"
                "PC = 0x%08x\r\n",
                fault_address, format_status, mmu_sr, pc);
            dbg("fault = 0x%08x\r\n", fault);
            mmu_map_8k_instruction_page(pc, 0);
            break;

        case 0x08020000:    /* TLB miss on data write */
        case 0x0c020000:    /* TLB miss on data read or read-modify-write */
            dbg("MMU DTLB MISS accessing 0x%08x\r\n"
                "FS = 0x%08x\r\n"
                "MMUSR = 0x%08x\r\n"
                "PC = 0x%08x\r\n",
                fault_address, format_status, mmu_sr, pc);
            dbg("fault = 0x%08x\r\n", fault);
            mmu_map_8k_data_page(fault_address, 0);
            break;

        /* else issue an bus error */
        default:
            dbg("bus error\r\n");
            return 1;       /* signal bus error to caller */
    }

#ifdef DBG_MMU
    xprintf("\r\n");
#endif /* DBG_MMU */

    return 0;   /* signal TLB miss handled to caller */
}



