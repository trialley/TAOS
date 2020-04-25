# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if i386-jos-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-jos-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-jos-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-jos-elf-', set your TOOLPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
# -Os 
CFLAGS = -I../include -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -MD -ggdb3 -m32 -Werror -fno-omit-frame-pointer -Wno-unused-variable -Wno-unused-function -Wno-address-of-packed-member
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# 自动调用
ASFLAGS =  -I../include -m32 -gdwarf-2 -Wa,-divide
# FreeBSD ld wants ``elf_i386_fbsd''
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)


OBJS = \
	device/kbd.o\
	device/uart.o\
	fs/fs.o\
	fs/file.o\
	fs/bio.o\
	mem/kalloc.o\
	mem/vm.o\
	console.o\
	exec.o\
	ide.o\
	ioapic.o\
	lapic.o\
	log.o\
	main.o\
	mp.o\
	pci.o\
	picirq.o\
	pipe.o\
	printfmt.o\
	proc.o\
	sleeplock.o\
	spinlock.o\
	string.o\
	swtch.o\
	syscall.o\
	sysfile.o\
	sysproc.o\
	trapasm.o\
	trap.o\
	vectors.o\

NET_OBJS = \
	device/e1000.o\
	device/ethernet.o\
	device/mt19937ar.o\
	\
	net/arp.o\
	net/ip.o\
	net/icmp.o\
	net/socket.o\
	net/net.o\
	net/sysnet.o\
	net/syssocket.o\
	net/tcp.o\
	net/udp.o\
	net/common.o\

OBJS += $(NET_OBJS)

all:kernel.elf
	cp kernel.elf ../build/kernel.elf

#内核主体
kernel.elf: $(OBJS) entry.o entryother initcode kernel.ld
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel.elf    entry.o -melf_i386  $(OBJS)  $(GCC_LIB) -b binary initcode entryother
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel  entry.o -melf_i386 $(OBJS) $(GCC_LIB) -b binary initcode entryother
	$(OBJDUMP) -S kernel > kernel.asm
	$(OBJDUMP) -t kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym


entryother: entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c entryother.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o bootblockother.o entryother.o
	$(OBJCOPY) -S -O binary -j .text bootblockother.o entryother
	$(OBJDUMP) -S bootblockother.o > entryother.asm

initcode: initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm


# tags: $(OBJS) entryother.S _init
# 	etags *.S *.c

vectors.S: 
#vectors.pl
	./vectors.pl > ./vectors.S



clean:
	-rm -f *.o *.d *.asm *.sym vectors.S entryother	initcode initcode.out kernel.elf kernel

