#
# Makefile for Firebee BaS
#
# requires the GNU toolchain
# either the m68k-atari-mint version which allows creating the binaries
#
# or
#
# the m68k-elf version which also allows to use gdb together with bdm tools for debugging
#

# This Makefile is meant for cross compiling the BaS with Vincent Riviere's cross compilers.
# If you want to compile native on an Atari (you will need at least GCC 4.6.3), set
# TCPREFIX to be empty. 
TCPREFIX=m68k-atari-mint-

CC=$(TCPREFIX)gcc
LD=$(TCPREFIX)ld
CPP=$(TCPREFIX)cpp

INCLUDE=-Iinclude
CFLAGS=-mcpu=5474 -Wall -Wno-multichar -Os -fomit-frame-pointer

SRCDIR=sources
OBJDIR=objs

MAPFILE=bas.map

# Linker control file.
LDCFILE=bas.lk
LDCSRC=bas.lk.S

EXEC=bas.s19

CSRCS= \
	$(SRCDIR)/sysinit.c \
	$(SRCDIR)/init_fpga.c \
	$(SRCDIR)/BaS.c \
	$(SRCDIR)/cache.c \
	$(SRCDIR)/sd_card.c

ASRCS= \
	$(SRCDIR)/mmu.S \
	$(SRCDIR)/exceptions.S \
	$(SRCDIR)/supervisor.S \
	$(SRCDIR)/ewf.S \
	$(SRCDIR)/illegal_instruction.S 

STRT_SRC = startcf.S
STRT_OBJ = $(OBJDIR)/startcf.o

COBJS=$(patsubst $(SRCDIR)/%.o,$(OBJDIR)/%.o,$(patsubst %.c,%.o,$(CSRCS)))
AOBJS=$(patsubst $(SRCDIR)/%.o,$(OBJDIR)/%.o,$(patsubst %.S,%.o,$(ASRCS)))

OBJS=$(COBJS) $(AOBJS)
	
.PHONY all: $(EXEC)

.PHONY clean:
	@ rm -f $(EXEC) $(STRT_OBJ) $(OBJS) $(MAPFILE) $(LDCFILE) depend 

	
$(EXEC): $(STRT_OBJ) $(OBJS) $(LDCSRC)
	$(CPP) -P -DTARGET_ADDRESS=0xe0000000 $(LDCSRC) -o $(LDCFILE)
	$(LD) --oformat srec -Map $(MAPFILE) --cref -T $(LDCFILE) -s -o $@ 

ram: $(STRT_OBJ) $(OBJS) $(LDCSRC)
	$(CPP) -P -DTARGET_ADDRESS=0x01000000 $(LDCSRC) -o $(LDCFILE)
	$(LD) --oformat srec -Map $(MAPFILE) --cref -T $(LDCFILE) -s -o $@.s19
	
# compile init_fpga with -mbitfield for testing purposes
$(OBJDIR)/init_fpga.o:	CFLAGS += -mbitfield

$(OBJDIR)/%.o:$(SRCDIR)/%.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

$(OBJDIR)/%.o:$(SRCDIR)/%.S
	$(CC) -c $(CFLAGS) -Wa,--bitwise-or $(INCLUDE) $< -o $@

depend: $(ASRCS) $(CSRCS)
	$(CC) $(CFLAGS) $(INCLUDE) -M $(ASRCS) $(CSRCS) > depend
	
include depend
	