#include "cpu.h"
#include "cpuid.h"
#include "thread.h"
#include "task.h"
#include "console.h"
#include "spinlock.h"
#include "pmtimer.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "interrupts/lapic.h"
#include <new>

using kernel::console::printf;

extern "C" void _thread_jump(kernel::thread* t) __attribute__((noreturn));

kernel::vector<kernel::cpu*> kernel::cpus;

kernel::cpu::cpu(void (*entry_func)()):
    cpu_addr(this),
    jiffies(0),
    cpu_number(cpus.size()),
    flags(cpu_number == 0 ? CPU_FLAG_BSP : 0),
    stack_guard(0xA1B2C3D4E5F60718),
    gdt{0x0000000000000000,     // Unused/reserved.
        0x00209A0000000000,     // Code descriptor
        0x0000920000000000,     // Data descriptor
        0x0000000000000000,     // Unused/reserved.
        TSS_DESC_0((uint64_t)&tss,sizeof(tss)),
        TSS_DESC_1((uint64_t)&tss,sizeof(tss))},
    ones(0xFFFF)
{
    // Create the scheduler thread.
    schedule_thread = new thread(entry_func,false);

    // Initialize the msix freelist.
    for (auto& e : msix_entries)
    {
        if (&e - msix_entries < 128)
            continue;
        free_msix_interrupts.push_back(&e.link);
    }

    // Initialize the TSS.
    memset(&tss,0,sizeof(tss));
    tss.iomap_base = sizeof(tss);
    for (size_t i=1; i<nelems(tss.ist); ++i)
        tss.ist[i] = (uint64_t)page_zalloc() + PAGE_SIZE - 32;

    // Initialize the IDT.
    memset(&idt,0,sizeof(idt));
}

void
kernel::cpu::claim_current_cpu()
{
    // Load the GDT, TR and IDT.
    lgdt((uint64_t)gdt,sizeof(gdt)-1);
    ltr(4*sizeof(gdt[0]));
    lidt((uint64_t)idt,sizeof(idt)-1);

    // Record the cpu* in the GS_BASE MSR.
    wrmsr((uint64_t)this,IA32_GS_BASE);
    kassert(get_current_cpu() == this);
}

kernel::work_entry*
kernel::cpu::alloc_msix_interrupt()
{
    kernel::work_entry* wqe = klist_front(free_msix_interrupts,link);
    free_msix_interrupts.pop_front();
    return wqe;
}

void*
kernel::cpu::operator new(size_t size)
{
    kassert(size == sizeof(cpu));
    size = round_up_pow2(size,PAGE_SIZE);

    uint16_t cpu_number = cpus.size();
    char* region = (char*)get_cpu_region(cpu_number);
    for (size_t i=0; i<size/PAGE_SIZE; ++i)
    {
        kernel_task->pt.map_4k_page(region + i*PAGE_SIZE,
                                    virt_to_phys(page_alloc()),
                                    PAGE_FLAGS_DATA);
    }
    return region;
}

void
kernel::cpu::operator delete(void*)
{
    kernel::panic("cpu deletion not supported!");
}

void
kernel::init_this_cpu(void (*entry_func)())
{
    // Allocate the CPU object and claim the current CPU.
    cpu* c = new cpu(entry_func);
    c->claim_current_cpu();

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
    _thread_jump(c->schedule_thread);
}

void
kernel::register_cpu()
{
    // Insert us into the cpus list.
    cpus.emplace_back(get_current_cpu());
}

void
kernel::cpu::register_exception_vector(size_t v, void (*handler)())
{
    // In 64-bit mode, the IDT can contain either an interrupt gate or a trap
    // gate (task gates are not supported in 64-bit mode, although everything
    // requires that a TSS has been set up via the LTR instruction since the
    // processor is going to use it to set interrupt handler stack pointers).
    // The only difference between a trap gate and an interrupt gate is that an
    // interrupt gate will automatically clear IF when it is invoked and a trap
    // gate will not.  We will use interrupt gates exclusively.
    //
    // Stack switching.  Two types of stack switching are possible:
    //  1. If gate.IST != 0, the processor will unconditionally switch to
    //     TSS.IST[gate.IST] when invoking the interrupt handler.
    //  2. If gate.IST == 0 and gate.DPL < CPL, the processor will switch to
    //     TSS.RSP[gate.DPL] when invoking the interrupt handler.
    // Note that this means if gate.IST == 0 and gate.DPL >= CPL then we won't
    // perform a stack switch and the current stack will be used by the
    // processor for pushing the interrupt stack frame.  This may be useful to
    // reduce interrupt latency for the scheduler wakeup and ticker handlers.
    //
    // For now, we are always setting gate.IST = 1 to use the first IST entry.
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
