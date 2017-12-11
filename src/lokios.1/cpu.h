#ifndef __KERNEL_CPU_H
#define __KERNEL_CPU_H

#include "hdr/compiler.h"
#include "kernel/kassert.h"
#include "k++/vector.h"
#include <stdint.h>
#include <stddef.h>

namespace kernel
{
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
        uint64_t    rsrv1[2];

        tss64       tss;
        uint64_t    rsrv2;

        struct
        {
            uint64_t    lo;
            uint64_t    hi;
        } idt[128];

        uint8_t     rsrv3[1872];

        void register_exception_vector(size_t v, void (*handler)());
    };
    KASSERT(sizeof(cpu) == 4096);

    extern vector<cpu*> cpus;

    cpu* init_main_cpu();
}

#endif /* __KERNEL_CPU_H */
