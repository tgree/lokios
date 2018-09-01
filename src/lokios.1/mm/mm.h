// Kernel Memory Map
// =================
// We define the upper half of the virtual address range for all kernel space.
// The kernel address space is divided up as follows (from high to low
// addresses):
//
//      0xFFFFFFFFC0000000 - 0xFFFFFFFFC0800000: kernel
//      0xFFFFFFFFnnnn0000 - 0xFFFFFFFFBFFFFFFF: thread space for tid nnnn
//      0xFFFFFFFEnnnn0000 - 0xFFFFFFFEFFFFFFFF: cpu space for cpu nnnn
//      0xFFFF9003E0000000 - 0xFFFF900400000000: eth31 space
//      ...                - ...                 ...
//      0xFFFF900000000000 - 0xFFFF900020000000: eth0 space
//      0xFFFF800000000000 - 0xFFFF8FFFFFFFFFFF: used to map all RAM
//
//
// Bootloader Memory Map
// =====================
// Upon boot, the lokios.0.ld provides a page table defined in the .ld file.
// This page table provides the following mappings:
//
//      Virtual Start        Virtual End        Physical Start     Len
//      ------------------   ------------------ ------------------ ---
//      0xFFFFFFFFC0000000 - 0xFFFFFFFFCFFFFFFF 0x0000000000000000 16M
//      0xFFFF800000000000 - 0xFFFF80000FFFFFFF 0x0000000000000000 16M
//      0x0000000000000000 - 0x000000000FFFFFFF 0x0000000000000000 16M
//
// Upon entry to lokios.1, we'll clone the upper-half mappings into the kernel
// page table and discard the mapping that starts at address 0.
//
//
// Kernel 0xFFFF800000000000 RAM Layout
// ====================================
// This region is used to map all of physical RAM.  To facilitate booting, we
// do have a bit of a static layout:
//
//      Virtual Start        Virtual End        Description
//      ------------------   ------------------ ----------------
//      0xFFFF800000000000 - 0xFFFF8000001FFFFF 2M of low memory
//      0xFFFF800000200000 - 0xFFFF8.. + _sbrk  start of sbrk area
//      0xFFFF800000800000 - end of RAM         buddy allocator pages
#ifndef __KERNEL_MM_H
#define __KERNEL_MM_H

#include "e820.h"
#include "page_flags.h"
#include "kern/types.h"
#include "kern/thread.h"

#define KERNEL_SBRK_END 0x00800000

#define MM_ETH_BEGIN    0xFFFF900000000000
#define MM_ETH_END      0xFFFF900400000000
#define MM_ETH_COUNT    32
#define MM_ETH_STRIDE   (MM_ETH_END - MM_ETH_BEGIN)/MM_ETH_COUNT

namespace kernel
{
    struct cpu;

    struct dma_alp
    {
        dma_addr64  paddr;
        size_t      len;
    };

    extern dma_addr64 zero_page_dma;
    extern dma_addr64 trash_page_dma;

    // Maps a physical address into the 0xFFFF800000000000 region and returns
    // the virtual address.  A 4K mapping will be used although you shouldn't
    // rely on that behavior.
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

    // Maps a physical range into the specified virtual address.
    void mmap(void* vaddr, dma_addr64 paddr, size_t len,
              uint64_t flags = PAGE_FLAGS_DATA);
    void munmap(void* vaddr, size_t len);

    inline thread* get_thread_region(thread_id id)
    {
        return (thread*)(0xFFFFFFFF00000000 | ((uint64_t)id << 16));
    }

    inline cpu* get_cpu_region(uint16_t n)
    {
        return (cpu*)(0xFFFFFFFE00000000 | ((uint64_t)n << 16));
    }

    // These work to convert between virtual and physical addresses in the
    // 0xFFFF800000000000 region.  They don't work anywhere else!
    dma_addr64 virt_to_phys(const void* v);
    void* phys_to_virt_maybe_0(dma_addr64 p);
    void* phys_to_virt(dma_addr64 p);

    // This converts a virtual address to a physical one by looking it up in
    // the page table.
    dma_addr64 xlate(const void* v);

    template<typename T>
    T inline phys_read(dma_addr64 p)
    {
        return *(T*)phys_to_virt(p);
    }

    void preinit_mm(const e820_map* m, dma_addr64 bitmap_base);
    void init_mm(const e820_map* m);
}

#endif /* __KERNEL_MM_H */
