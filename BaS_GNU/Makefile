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

TCPREFIX=m68k-atari-mint-

CC=$(TCPREFIX)gcc
LD=$(TCPREFIX)ld

INCLUDE=-Iinclude
CFLAGS=-mcfv4e -Wno-multichar -Os -fomit-frame-pointer
#CFLAGS=-mcfv4e -Wno-multichar -S -O3 -fomit-frame-pointer
SRCDIR=sources
OBJDIR=objs

EXEC=bas.hex

CSRCS= \
	$(SRCDIR)/startcf.c \
	$(SRCDIR)/sysinit.c \
	$(SRCDIR)/BaS.c \
	$(SRCDIR)/sd_card.c \
	$(SRCDIR)/last.c

ASRCS= \
	$(SRCDIR)/mmu.S \
	$(SRCDIR)/exceptions.S \
	$(SRCDIR)/supervisor.S \
	$(SRCDIR)/ewf.S \
	$(SRCDIR)/illegal_instruction.S 

COBJS=$(patsubst $(SRCDIR)/%.o,$(OBJDIR)/%.o,$(patsubst %.c,%.o,$(CSRCS)))
AOBJS=$(patsubst $(SRCDIR)/%.o,$(OBJDIR)/%.o,$(patsubst %.S,%.o,$(ASRCS)))

OBJS=$(COBJS) $(AOBJS)
	
all: $(EXEC)

$(EXEC): $(OBJS)
	echo "generating executable"

clean:
	rm $(EXEC) *.o
	
$(OBJDIR)/%.o:$(SRCDIR)/%.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@
	
$(OBJDIR)/%.o:$(SRCDIR)/%.S
	$(CC) -c $(CFLAGS) -Wa,--bitwise-or $(INCLUDE) $< -o $@
