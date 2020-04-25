// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

//
#include <types.h>
//
//
#include <defs.h>
//
#include <param.h>
//
#include <memlayout.h>
//
#include <mmu.h>
//
#include <spinlock.h>

void freerange(void *vstart, void *vend);
extern char end[];	// first address after kernel loaded from ELF file
					// defined by the kernel linker script in kernel.ld
					//内核文件加载之后的首地址，由链接脚本进行定义

struct run {
	struct run *next;
};

struct {
	struct spinlock lock;  //内核锁，通过特定函数进行修改
	int use_lock;		   //表示是否需要用上面这个锁
	struct run *freelist;  //空闲链表
} kmem;					   //kem是用于内核内存管理的数据结构

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.

//这个函数将内核后面的一小块内存初始化为空闲链表
void kinit1(void *vstart, void *vend) {
	initlock(&kmem.lock, "kmem");
	kmem.use_lock = 0;
	freerange(vstart, vend);
}

void kinit2(void *vstart, void *vend) {
	freerange(vstart, vend);
	kmem.use_lock = 1;
}

void freerange(void *vstart, void *vend) {
	char *p;
	p = (char *)PGROUNDUP((uint)vstart);
	for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
		kfree(p);  //将end[]靠后的一块连续内存按照PGSIZE分割为链表
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
//释放v指向的物理内存到链表
void kfree(char *v) {
	struct run *r;

	if ((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
		panic("kfree");

	// Fill with junk to catch dangling refs.
	//使用垃圾1将v填充，如果有指针错误的地使用这块数据，可以通过垃圾数据判断出来
	memset(v, 1, PGSIZE);

	if (kmem.use_lock)		  //获取锁
		acquire(&kmem.lock);  //内部由testandset进行封装
	r = (struct run *)v;
	r->next = kmem.freelist;
	kmem.freelist = r;	//将free的空间添加到内核空闲链表中
	if (kmem.use_lock)
		release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
//内核从kmem.freelist申请4kb内存，
char *kalloc(void) {
	struct run *r;

	if (kmem.use_lock)
		acquire(&kmem.lock);
	r = kmem.freelist;
	if (r)
		kmem.freelist = r->next;
	if (kmem.use_lock)
		release(&kmem.lock);
	return (char *)r;
}
