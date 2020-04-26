/*
main.c主要用来进行一些初始化操作
1. 启动其他CPU
2. 初始化内存管理等

*/

#include <types.h>
//
//types.h必须首先被包含，这行注释用来防止错误的代码格式化
#include <defs.h>
#include <memlayout.h>
#include <mmu.h>
#include <param.h>
#include <proc.h>
#include <x86.h>

static void startothers(void);						 //启动其他CPU的前置声明
static void mpmain(void) __attribute__((noreturn));	 //真正主函数的前置声明
extern pde_t *kpgdir;
extern char end[];	// first address after kernel loaded from ELF file
					//内核加载之后的首地址

//Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
// 这里是kernel的C代码入口，从entry.S来到此处
// 启动用CPU从这里开始运行C代码
// 首先要初始化内存管理以使内存分配器可以正常运行
int main(void) {
	//54

	kinit1(end, P2V(4 * 1024 * 1024));	// 申请内核使用的初始物理页表，从内核首地址开始申请4mb空间
	kvmalloc();							// 内核页表
	mpinit();							// 检测CPU架构是否支持
	lapicinit();						// 中断控制器
	seginit();							// 段描述符
	picinit();							// 禁用 8259A 中断控制器
	ioapicinit();						// 另一个中断控制器
	consoleinit();						// 初始化终端硬件console hardware
	uartinit();							// serial port

	pinit();									 // process table
	tvinit();									 // 中断向量trap vectors
	binit();									 // buffer cache
	fileinit();									 // file table
	ideinit();									 // disk
	pciinit();									 // pci devices各种设备的驱动在这里初始化
	netinit();									 // 注册一系列协议处理函数 networking
	startothers();								 // start other processors
	kinit2(P2V(4 * 1024 * 1024), P2V(PHYSTOP));	 // must come after startothers()
	userinit();									 // first user process
	mpmain();									 // finish this processor's setup
}

// Other CPUs jump here from entryother.S.
static void mpenter(void) {
	switchkvm();
	seginit();
	lapicinit();
	mpmain();
}

// Common CPU setup code.
static void mpmain(void) {
	cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
	idtinit();							// load idt register
	xchg(&(currentcpu()->started), 1);	// tell startothers() we're up
	scheduler();						// start running processes
}

pde_t entrypgdir[];	 // For entry.S

// Start the non-boot (AP) processors.
//开启其他没有被用来boot的CPU
static void startothers(void) {
	extern uchar _binary_entryother_start[], _binary_entryother_size[];
	uchar *code;
	struct cpu *c;
	char *stack;

	// Write entry code to unused memory at 0x7000.
	// The linker has placed the image of entryother.S in
	// _binary_entryother_start.
	//
	code = P2V(0x7000);
	memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);

	for (c = cpus; c < cpus + ncpu; c++) {
		if (c == currentcpu())	// 如果
			continue;

		// Tell entryother.S what stack to use, where to enter, and what
		// pgdir to use. We cannot use kpgdir yet, because the AP processor
		// is running in low  memory, so we use entrypgdir for the APs too.
		stack = kalloc();
		*(void **)(code - 4) = stack + KSTACKSIZE;
		*(void (**)(void))(code - 8) = mpenter;
		*(int **)(code - 12) = (void *)V2P(entrypgdir);

		lapicstartap(c->apicid, V2P(code));

		// wait for cpu to finish mpmain()
		while (c->started == 0)
			;
	}
}

// The boot page table used in entry.S and entryother.S.
// Page directories (and page tables) must start on page boundaries,
// hence the __aligned__ attribute.
// PTE_PS in a page directory entry enables 4Mbyte pages.

__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
	// Map VA's [0, 4MB) to PA's [0, 4MB)
	[0] = (0) | PTE_P | PTE_W | PTE_PS,
	// Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
	[KERNBASE >>
		PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
