#ifndef __KERNEL_CPU_H
#define __KERNEL_CPU_H

#include "msr.h"
#include "hdr/compiler.h"
#include "kernel/kassert.h"
#include "k++/vector.h"
#include <stdint.h>
#include <stddef.h>

namespace kernel
{
    struct thread;

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

    struct cpu
    {
        uint64_t    gdt[6];
        size_t      cpu_number;
        uint64_t    rsrv1;

        tss64       tss;
        uint16_t    ones;
        uint8_t     rsrv2[6];

        struct
        {
            uint64_t    lo;
            uint64_t    hi;
        } idt[128];

        thread*     idle_thread;

        uint8_t     rsrv3[1864];

        void register_exception_vector(size_t v, void (*handler)());
    };
    KASSERT(sizeof(cpu) == 4096);

    static inline cpu* get_current_cpu()
    {
        // The cpu* is stored in the GS_BASE MSR.
        return (cpu*)rdmsr(IA32_GS_BASE);
    }

    extern vector<cpu*> cpus;

    void init_this_cpu();
    void init_ap_cpus();
}

#endif /* __KERNEL_CPU_H */
