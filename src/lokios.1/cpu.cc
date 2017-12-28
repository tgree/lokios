#include "cpu.h"
#include "x86.h"
#include "cpuid.h"
#include "thread.h"
#include "console.h"
#include "mm/page.h"

using kernel::console::printf;

kernel::vector<kernel::cpu*> kernel::cpus;

kernel::cpu*
kernel::init_main_cpu()
{
    cpu* c = new cpu;

    // Start by setting up the GDT.
    c->gdt[0] = 0x0000000000000000;     // Unused/reserved.
    c->gdt[1] = 0x00209A0000000000;     // Code descriptor
    c->gdt[2] = 0x0000920000000000;     // Data descriptor
    c->gdt[4] = TSS_DESC_0((uint64_t)&c->tss,sizeof(c->tss));
    c->gdt[5] = TSS_DESC_1((uint64_t)&c->tss,sizeof(c->tss));
    lgdt((uint64_t)c->gdt,sizeof(c->gdt)-1);

    // Fill in the TSS and load the TR.
    memset(&c->tss,0,sizeof(c->tss));
    c->tss.iomap_base = sizeof(c->tss);
    c->ones           = 0xFFFF;
    for (size_t i=1; i<nelems(c->tss.ist); ++i)
        c->tss.ist[i] = (uint64_t)page_zalloc() + PAGE_SIZE - 32;
    ltr(4*sizeof(c->gdt[0]));

    // Load the IDT.
    memset(&c->idt,0,sizeof(c->idt));
    lidt((uint64_t)c->idt,sizeof(c->idt)-1);

    // Fill in some cpuid feature flags.
    printf("Max Basic CPUID Selector: 0x%08X\n",cpuid(0).eax);
    printf("Max Extnd CPUID Selector: 0x%08X\n",cpuid(0x80000000).eax);
    char brand[49];
    for (size_t i=0; i<3; ++i)
        cpuid(0x80000002+i,0,brand + 16*i);
    brand[48] = '\0';
    printf("CPU Brand: %s\n",brand);
    printf("Initial APIC ID: %u\n",cpuid(1).ebx >> 24);

    // Done.
    cpus.emplace_back(c);
    return c;
}

void
kernel::cpu::register_exception_vector(size_t v, void (*handler)())
{
    uint64_t p = (uint64_t)handler;
    idt[v].hi = ((p >> 32) & 0x00000000FFFFFFFF);
    idt[v].lo = ((p << 32) & 0xFFFF000000000000) |
                             0x00008E0100080000  |
                ((p >>  0) & 0x000000000000FFFF);
}
