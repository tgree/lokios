#ifndef __KERNEL_CPU_H
#define __KERNEL_CPU_H

#include "msr.h"
#include "spinlock.h"
#include "hdr/compiler.h"
#include "kernel/kassert.h"
#include "k++/vector.h"
#include <stdint.h>
#include <stddef.h>

namespace kernel
{
    struct thread;
    struct work_entry;

    struct tss64
    {
        uint32_t    rsrv0;
        uint64_t    rsp[3];
        uint64_t    ist[8];     // IST0 is reserved.
        uint8_t     rsrv1[10];
        uint16_t    iomap_base;
    } __PACKED__;
    KASSERT(sizeof(tss64) == 104);

#define TSS_DESC_0(base,limit) \
    ((((base)  << 32) & 0xFF00000000000000) | \
     (((limit) << 32) & 0x000F000000000000) | \
                        0x0000890000000000  | \
     (((base)  << 16) & 0x000000FFFFFF0000) | \
     (((limit) <<  0) & 0x000000000000FFFF))
#define TSS_DESC_1(base,limit) \
    (((base) >> 32) & 0x00000000FFFFFFFF)

#define CPU_FLAG_BSP    0x01
    struct cpu
    {
        uint64_t            gdt[6];
        size_t              cpu_number;
        volatile uint64_t   jiffies;

        tss64       tss;
        uint16_t    ones;
        uint8_t     rsrv2[5];
        uint8_t     flags;

        struct
        {
            uint64_t    lo;
            uint64_t    hi;
        } idt[128];

        klist<work_entry>   work_queue;
        thread*             schedule_thread;

        uint8_t     rsrv4[1712];

        spinlock    work_queue_lock;

        void register_exception_vector(size_t v, void (*handler)());
    };
    KASSERT(sizeof(cpu) == 4096);
    KASSERT(offsetof(cpu,jiffies) == 56);

    static inline cpu* get_current_cpu()
    {
        // The cpu* is stored in the GS_BASE MSR.
        return (cpu*)rdmsr(IA32_GS_BASE);
    }

    extern vector<cpu*> cpus;

    void register_cpu();

    void init_this_cpu(void (*entry_func)());
    void init_ap_cpus();
}

#endif /* __KERNEL_CPU_H */
