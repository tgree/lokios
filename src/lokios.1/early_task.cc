#include "early_task.h"
#include "console.h"
#include "kassert.h"
#include "interrupts/interrupt.h"
#include "hdr/x86.h"
#include "hdr/types.h"

using kernel::_kassert;
using kernel::console::printf;

struct early_interrupt_state
{
    union
    {
        struct
        {
            uint64_t    rax;
            uint64_t    rbx;
            uint64_t    rcx;
            uint64_t    rdx;
            uint64_t    rbp;
            uint64_t    rsi;
            uint64_t    rdi;
        };
        uint64_t    r[16];
    };

    uint64_t    vecnum;
    uint64_t    error_code;
    uint64_t    rip;
    uint64_t    cs;
    uint64_t    rflags;
    uint64_t    rsp;
    uint64_t    ss;
};
KASSERT(offsetof(early_interrupt_state,rdi) == 48);
KASSERT(offsetof(early_interrupt_state,r[8]) == 64);
KASSERT(offsetof(early_interrupt_state,vecnum) == 128);

// Note: This should match what we get from the bootloader!
static uint64_t early_gdt[] __CACHE_ALIGNED__ = {
    0,                  // Reserved
    0x00CF92000000FFFF, // 32-bit data segment
    0x00CF9A000000FFFF, // 32-bit code segment
    0x000092000000FFFF, // 16-bit data segment
    0x00009A000000FFFF, // 16-bit code segment
    0x0000920000000000, // 64-bit data segment
    0x00209A0000000000, // 64-bit code segment
    0,                  // For TSS
    0,                  // For TSS
};

static no_iomap_tss64 early_tss __CACHE_ALIGNED__= {
    {0, {0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     sizeof(tss64)},
    0xFFFF,
};

struct idt_entry
{
    uint64_t    lo;
    uint64_t    hi;
};
static idt_entry early_idt[32] __CACHE_ALIGNED__;

extern "C" void _early_interrupt_entry_0();
extern "C" void _early_interrupt_entry_1();
extern "C" void _early_interrupt_entry_2();
extern "C" void _early_interrupt_entry_3();
extern "C" void _early_interrupt_entry_4();
extern "C" void _early_interrupt_entry_5();
extern "C" void _early_interrupt_entry_6();
extern "C" void _early_interrupt_entry_7();
extern "C" void _early_interrupt_entry_8();
extern "C" void _early_interrupt_entry_9();
extern "C" void _early_interrupt_entry_10();
extern "C" void _early_interrupt_entry_11();
extern "C" void _early_interrupt_entry_12();
extern "C" void _early_interrupt_entry_13();
extern "C" void _early_interrupt_entry_14();
extern "C" void _early_interrupt_entry_15();
extern "C" void _early_interrupt_entry_16();
extern "C" void _early_interrupt_entry_17();
extern "C" void _early_interrupt_entry_18();
extern "C" void _early_interrupt_entry_19();
extern "C" void _early_interrupt_entry_20();
extern "C" void _early_interrupt_entry_21();
extern "C" void _early_interrupt_entry_22();
extern "C" void _early_interrupt_entry_23();
extern "C" void _early_interrupt_entry_24();
extern "C" void _early_interrupt_entry_25();
extern "C" void _early_interrupt_entry_26();
extern "C" void _early_interrupt_entry_27();
extern "C" void _early_interrupt_entry_28();
extern "C" void _early_interrupt_entry_29();
extern "C" void _early_interrupt_entry_30();
extern "C" void _early_interrupt_entry_31();

extern "C" void
early_task_exception_handler(early_interrupt_state* s)
{
    printf("Early exception %lu error code 0x%016lX: %s\n",
           s->vecnum,s->error_code,kernel::get_exception_name(s->vecnum));
    cpuid_result ci = cpuid(0x01);
    printf("CPUID1 0x%08X 0x%08X 0x%08X 0x%08X\n",ci.eax,ci.ebx,ci.ecx,ci.edx);
    printf("   CR0 0x%016lX     CR2 0x%016lX\n",mfcr0(),mfcr2());
    printf("   CR3 0x%016lX     CR4 0x%016lX\n",mfcr3(),mfcr4());
    printf("   CR8 0x%016lX    EFER 0x%016lX\n",mfcr8(),rdmsr(IA32_EFER));
    printf("FSBASE 0x%016lX  GSBASE 0x%016lX\n",
           rdmsr(IA32_FS_BASE),rdmsr(IA32_GS_BASE));
    printf("    CS 0x%016lX      SS 0x%016lX\n",s->cs,s->ss);
    printf("   RIP 0x%016lX  RFLAGS 0x%016lX\n",s->rip,s->rflags);
    printf("   RSP 0x%016lX     RBP 0x%016lX\n",s->rsp,s->rbp);
    printf("   RAX 0x%016lX     RBX 0x%016lX\n",s->rax,s->rbx);
    printf("   RCX 0x%016lX     RDX 0x%016lX\n",s->rcx,s->rdx);
    printf("   RSI 0x%016lX     RDI 0x%016lX\n",s->rsi,s->rdi);
    printf("    R8 0x%016lX      R9 0x%016lX\n",s->r[8],s->r[9]);
    printf("   R10 0x%016lX     R11 0x%016lX\n",s->r[10],s->r[11]);
    printf("   R12 0x%016lX     R13 0x%016lX\n",s->r[12],s->r[13]);
    printf("   R14 0x%016lX     R15 0x%016lX\n",s->r[14],s->r[15]);
    kernel::halt();
}

static void
register_vector(size_t v, void (*handler)())
{
    kassert(v < NELEMS(early_idt));
    uint64_t p = (uint64_t)handler;
    early_idt[v].hi = ((p >> 32) & 0x00000000FFFFFFFF);
    early_idt[v].lo = ((p << 32) & 0xFFFF000000000000) |
                                   0x00008E0000300000  |
                      ((p >>  0) & 0x000000000000FFFF);
}

static void
load_tables()
{
    lgdt((uint64_t)early_gdt,sizeof(early_gdt)-1);
    ltr(7*sizeof(early_gdt[0]));
    lidt((uint64_t)early_idt,sizeof(early_idt)-1);
}

void
kernel::init_early_task_bsp()
{
    register_vector(0,_early_interrupt_entry_0);
    register_vector(1,_early_interrupt_entry_1);
    register_vector(2,_early_interrupt_entry_2);
    register_vector(3,_early_interrupt_entry_3);
    register_vector(4,_early_interrupt_entry_4);
    register_vector(5,_early_interrupt_entry_5);
    register_vector(6,_early_interrupt_entry_6);
    register_vector(7,_early_interrupt_entry_7);
    register_vector(8,_early_interrupt_entry_8);
    register_vector(9,_early_interrupt_entry_9);
    register_vector(10,_early_interrupt_entry_10);
    register_vector(11,_early_interrupt_entry_11);
    register_vector(12,_early_interrupt_entry_12);
    register_vector(13,_early_interrupt_entry_13);
    register_vector(14,_early_interrupt_entry_14);
    register_vector(15,_early_interrupt_entry_15);
    register_vector(16,_early_interrupt_entry_16);
    register_vector(17,_early_interrupt_entry_17);
    register_vector(18,_early_interrupt_entry_18);
    register_vector(19,_early_interrupt_entry_19);
    register_vector(20,_early_interrupt_entry_20);
    register_vector(21,_early_interrupt_entry_21);
    register_vector(22,_early_interrupt_entry_22);
    register_vector(23,_early_interrupt_entry_23);
    register_vector(24,_early_interrupt_entry_24);
    register_vector(25,_early_interrupt_entry_25);
    register_vector(26,_early_interrupt_entry_26);
    register_vector(27,_early_interrupt_entry_27);
    register_vector(28,_early_interrupt_entry_28);
    register_vector(29,_early_interrupt_entry_29);
    register_vector(30,_early_interrupt_entry_30);
    register_vector(31,_early_interrupt_entry_31);
    early_gdt[7] = TSS_DESC_0((uint64_t)&early_tss,sizeof(tss64));
    early_gdt[8] = TSS_DESC_1((uint64_t)&early_tss,sizeof(tss64));
    load_tables();
}

void
kernel::init_early_task_ap()
{
    load_tables();
}

void
kernel::early_task_release_tss()
{
    // Clear the TSS descriptor's BUSY bit.
    kassert((early_gdt[7] & 0x00000F0000000000) == 0x00000B0000000000);
    early_gdt[7] &= ~0x00000F0000000000;
    early_gdt[7] |=  0x0000090000000000;
}
