// Kernel Memory Map
// =================
// We define the upper half of the address range for all kernel space.  The
// address space is divided up as follows (from high to low addresses):
//
//      0xFFFFFFFFnnnn0000 - 0xFFFFFFFFFFFFFFFF: thread space for tid nnnn
//      0xFFFFFFFEnnnn0000 - 0xFFFFFFFEFFFFFFFF: cpu space for cpu nnnn
//      0xFFFF800000000000 - 0xFFFF8FFFFFFFFFFF: used to map all RAM
#ifndef __KERNEL_MM_H
#define __KERNEL_MM_H

#include "e820.h"
#include "page_table.h"
#include "kernel/thread.h"
#include "kernel/cpu.h"

namespace kernel
{
    typedef uint64_t dma_addr64;

    struct dma_alp
    {
        dma_addr64  paddr;
        size_t      len;
    };

    extern dma_addr64 zero_page_dma;
    extern dma_addr64 trash_page_dma;

    // Maps a physical address into the 0xFFFF800000000000 region and returns
    // the virtual address.  A 2M mapping will be used.
    void* pmap(dma_addr64 paddr, uint64_t flags = PAGE_FLAGS_DATA);
    void* pmap_range(dma_addr64 paddr, size_t len,
                     uint64_t flags = PAGE_FLAGS_DATA);
    inline void* iomap(dma_addr64 paddr, uint64_t flags = PAGE_FLAGS_IO)
    {
        return pmap(paddr,flags);
    }
    inline void* iomap_range(dma_addr64 paddr, size_t len,
                             uint64_t flags = PAGE_FLAGS_IO)
    {
        return pmap_range(paddr,len,flags);
    }

    inline thread* get_thread_region(thread_id id)
    {
        return (thread*)(0xFFFFFFFF00000000 | ((uint64_t)id << 16));
    }

    inline cpu* get_cpu_region(uint16_t n)
    {
        return (cpu*)(0xFFFFFFFE00000000 | ((uint64_t)n << 16));
    }

    void init_mm(const e820_map* m);
}

#endif /* __KERNEL_MM_H */
