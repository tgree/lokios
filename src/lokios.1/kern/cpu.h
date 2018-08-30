#ifndef __KERNEL_CPU_H
#define __KERNEL_CPU_H

#include "spinlock.h"
#include "schedule.h"
#include "hdr/compiler.h"
#include "hdr/x86.h"
#include "kern/kassert.h"
#include "k++/vector.h"
#include <stdint.h>
#include <stddef.h>

namespace kernel
{
    struct thread;
    struct work_entry;
    struct cpu;

#ifndef BUILDING_UNITTEST
    static inline cpu* get_current_cpu()
    {
        // The cpu* is stored in the GS_BASE MSR.
        return (cpu*)rdmsr(IA32_GS_BASE);
    }
#else
    cpu* get_current_cpu();
#endif

#define CPU_FLAG_BSP            (1<<0)
#define CPU_FLAG_VMX            (1<<1)
#define CPU_FLAG_RDRAND         (1<<2)
#define CPU_FLAG_PAGESIZE_1G    (1<<3)
#define CPU_FLAG_FXSAVE         (1<<4)
#define CPU_FLAG_SSE            (1<<5)
#define CPU_FLAG_POPCNT         (1<<6)
    struct cpu
    {
        // Page boundary.
        cpu* const          cpu_addr;                               // 0
        volatile uint64_t   jiffies;                                // 8
        const size_t        cpu_number;                             // 16
        uint8_t             rsrv[8];                                // 24
        thread*             schedule_thread;                        // 32
        uint64_t            stack_guard;                            // 40
        klist<work_entry>   free_msix_interrupts;                   // 48

        const uint32_t      max_basic_cpuid;                        // 56
        const uint32_t      max_extended_cpuid;                     // 60
        char                cpuid_brand[49];                        // 64
        uint8_t             flags;                                  // 121
        int8_t              apic_id;                                // 122
        int8_t              initial_apic_id;                        // 123
        volatile uint32_t   tlb_shootdown_counter;                  // 124

        struct scheduler    scheduler;                              // 128

        // Page boundary
        work_entry          msix_entries[256] __PAGE_ALIGNED__;     // 4096

        // Page boundary.
        struct
        {
            uint64_t    lo;
            uint64_t    hi;
        } idt[256] __PAGE_ALIGNED__;

        // Page boundary.
        const uint64_t  gdt[6] __PAGE_ALIGNED__;
        tss64           tss __CACHE_ALIGNED__;
        const uint16_t  ones;

        void claim_current_cpu();
        kernel::work_entry* alloc_msix_interrupt();
        void register_exception_vector(size_t v, void (*handler)());

        static  void*   operator new(size_t size);
        static  void    operator delete(void*);

        static inline void schedule_local_work(work_entry* wqe)
        {
            get_current_cpu()->scheduler.schedule_local_work(wqe);
        }

        static inline void schedule_timer(timer_entry* wqe, uint64_t dt10ms)
        {
            get_current_cpu()->scheduler.schedule_timer(wqe,dt10ms);
        }

        static inline void schedule_timer_ms(timer_entry* wqe, uint64_t dtms)
        {
            get_current_cpu()->scheduler.schedule_timer_ms(wqe,dtms);
        }

        static inline void schedule_timer_sec(timer_entry* wqe, uint64_t secs)
        {
            get_current_cpu()->scheduler.schedule_timer_sec(wqe,secs);
        }

        static inline void cancel_timer(timer_entry* wqe)
        {
            get_current_cpu()->scheduler.cancel_timer(wqe);
        }

        cpu(void (*entry_func)());
    };
    KASSERT(sizeof(cpu) < 65536);
    KASSERT(offsetof(cpu,cpu_addr) == 0);
    KASSERT(offsetof(cpu,jiffies) == 8);
    KASSERT(offsetof(cpu,stack_guard) == 0x28);
    KASSERT(offsetof(cpu,scheduler) == 128);
    KASSERT(offsetof(cpu,msix_entries) == 4096);

    extern vector<cpu*> cpus;

    void register_cpu();

    void init_this_cpu(void (*entry_func)());
    void init_ap_cpus();
}

#endif /* __KERNEL_CPU_H */
