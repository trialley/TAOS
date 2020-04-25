
//
#include <traps.h>
//
#include <types.h>
//
//
#include <x86.h>

// I/O Addresses of the two programmable interrupt controllers
#define IO_PIC1 0x20  // Master (IRQs 0-7)
#define IO_PIC2 0xA0  // Slave (IRQs 8-15)

// Don't use the 8259A interrupt controllers.  Xv6 assumes SMP hardware.
//不使用8259A中断控制器，xv6使用SMP硬件
void picinit(void) {
	// mask all interrupts
	outb(IO_PIC1 + 1, 0xFF);
	outb(IO_PIC2 + 1, 0xFF);
}

//PAGEBREAK!
// Blank page.
