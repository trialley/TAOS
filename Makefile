BUILD_USER_DIR = 
#./build/user/
BUILD_KERNEL_DIR = ./build/kernel/
BUILD_BOOT_DIR = ./build/boot/

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

# If the makefile can't find QEMU, specify its path here
# QEMU = qemu-system-i386

# Try to infer the correct QEMU
ifndef QEMU
QEMU = $(shell if which qemu > /dev/null; \
	then echo qemu; exit; \
	elif which qemu-system-i386 > /dev/null; \
	then echo qemu-system-i386; exit; \
	elif which qemu-system-x86_64 > /dev/null; \
	then echo qemu-system-x86_64; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in Makefile?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
# -Os 
CFLAGS = -I./include -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -MD -ggdb3 -m32 -Werror -fno-omit-frame-pointer -Wno-unused-variable -Wno-unused-function -Wno-address-of-packed-member
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS =  -I./include -m32 -gdwarf-2 -Wa,-divide
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

all:
	-mkdir build
	-mkdir build/kernel
	-mkdir build/boot
	-mkdir build/disk
	make -C userprog
	make -C kernel
	make -C boot
	make build/xv6.img

build/xv6.img:./build/bootblock ./build/kernel.elf
	dd if=/dev/zero of=./build/xv6.img count=10000
	dd if=./build/bootblock of=./build/xv6.img conv=notrunc
	dd if=./build/kernel.elf of=./build/xv6.img seek=1 conv=notrunc

###########################

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o



# fs.img: mkfs README $(UPROGS)
# 	./mkfs fs.img README $(UPROGS)

clean:
	# rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	# */*.o *.o */*.d *.d */*.asm *.asm */*.sym vectors.S bootblock entryother \
	# initcode initcode.out */kernel.elf xv6.img fs.img kernelmemfs \
	# xv6memfs.img mkfs .gdbinit \
	# $(UPROGS)
	make -C boot clean
	make -C kernel clean
	make -C userprog clean
	-rm -rf build
	# -rm -rf xv6.img
# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 2
endif

#QEMUNET = -netdev user,id=n1,hostfwd=udp::10007-:7,hostfwd=tcp::10007-:7 -device e1000,netdev=n1 -object filter-dump,id=f1,netdev=n1,file=n1.pcap \
          -netdev tap,id=n2,ifname=tap0 -device e1000,netdev=n2 -object filter-dump,id=f2,netdev=n2,file=n2.pcap

QEMUOPTS = -drive file=./build/fs.img,index=1,media=disk,format=raw -drive file=./build/xv6.img,index=0,media=disk,format=raw -smp $(CPUS) -m 512 $(QEMUEXTRA) $(QEMUNET)

qemu: fs.img xv6.img
	$(QEMU) -serial mon:stdio $(QEMUOPTS)

qemu-nox: fs.img xv6.img
	$(QEMU) -nographic $(QEMUOPTS)

.gdbinit: #.gdbinit.tmpl
	# sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

qemu-gdb: fs.img xv6.img .gdbinit
	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)

qemu-nox-gdb:all
	$(QEMU) -nographic $(QEMUOPTS) -S -s
	#$(QEMUGDB)
	#  > test.log 2>&1 &

docker-build:
	docker build -t xv6-net .

docker-run:
	docker run -it --name xv6-net --rm --device=/dev/net/tun --cap-add=NET_ADMIN xv6-net make run

run: xv6.img fs.img
	ip tuntap add mode tap name tap0
	ip addr add 172.16.100.1/24 dev tap0
	ip link set tap0 up
	$(QEMU) $(QEMUOPTS) 
	# -nographic 

.PHONY: dist-test dist docker-build docker-run run
