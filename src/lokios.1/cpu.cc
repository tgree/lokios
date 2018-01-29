#include "cpu.h"
#include "cpuid.h"
#include "thread.h"
#include "task.h"
#include "console.h"
#include "spinlock.h"
#include "pmtimer.h"
#include "mm/page.h"
#include "interrupts/lapic.h"

using kernel::console::printf;

extern "C" void _thread_jump(kernel::thread* t) __attribute__((noreturn));

static kernel::spinlock cpus_lock;
kernel::vector<kernel::cpu*> kernel::cpus;

static void
idle_loop()
{
    for (;;)
        asm ("hlt;");
}

void
kernel::init_this_cpu()
{
    cpu* c         = new cpu;
    c->idle_thread = new thread(idle_loop);

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

    // Record the cpu* in the GS_BASE MSR.
    wrmsr((uint64_t)c,IA32_GS_BASE);
    kassert(get_current_cpu() == c);

    // Insert us into the cpus list.
    with (cpus_lock)
    {
        c->cpu_number = cpus.size();
        cpus.emplace_back(c);
    }

    // Fill in some cpuid feature flags.
    printf("CPU%zu Max Basic CPUID Selector: 0x%08X\n",
           c->cpu_number,cpuid(0).eax);
    printf("CPU%zu Max Extnd CPUID Selector: 0x%08X\n",
           c->cpu_number,cpuid(0x80000000).eax);
    char brand[49];
    for (size_t i=0; i<3; ++i)
        cpuid(0x80000002+i,0,brand + 16*i);
    brand[48] = '\0';
    printf("CPU%zu CPU Brand: %s\n",c->cpu_number,brand);

    // Check for FXSAVE/FXRSTOR support.
    auto cpuid1 = cpuid(1);
    printf("CPU%zu CPUID 1: 0x%08X:0x%08X:0x%08X:0x%08X\n",
           c->cpu_number,cpuid1.eax,cpuid1.ebx,cpuid1.ecx,cpuid1.edx);
    kassert(cpuid1.edx & (1 << 25));    // SSE availability.
    kassert(cpuid1.edx & (1 << 24));    // FXSAVE/FXRSTOR availability.

    // APIC info.
    printf("CPU%zu Initial APIC ID: %u\n",c->cpu_number,cpuid1.ebx >> 24);

    // Start executing.
    thread* t;
    with (kernel_task->threads_lock)
    {
        if (kernel_task->runnable_threads.empty())
            t = c->idle_thread;
        else
        {
            t = klist_front(kernel_task->runnable_threads,tcb.link);
            t->tcb.link.unlink();
            kernel_task->running_threads.push_back(&t->tcb.link);
        }
    }
    _thread_jump(t);
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

void
kernel::init_ap_cpus()
{
    // Start the APs one at a time.
    uint8_t lapic_id = get_lapic_id();
    size_t expected_cpu_count = cpus.size();
    for (const auto& lc : lapic_configs)
    {
        if (lc.apic_id == lapic_id)
            continue;

        send_init_ipi(lc.apic_id);
        pmtimer::wait_us(20*1000);
        send_sipi_ipi(lc.apic_id);

        size_t elapsed_ms;
        for (elapsed_ms = 0;
             elapsed_ms < 200 && cpus.size() != expected_cpu_count + 1;
             ++elapsed_ms)
        {
            pmtimer::wait_us(1000);
        }

        kassert(cpus.size() == expected_cpu_count + 1);
        printf("CPU%zu arrived in %zums.\n",expected_cpu_count,elapsed_ms);
        ++expected_cpu_count;
    }
}
