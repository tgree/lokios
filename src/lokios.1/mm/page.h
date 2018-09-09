#ifndef __KERNEL_PAGE_H
#define __KERNEL_PAGE_H

#include "e820.h"
#include "kern/types.h"
#include <string.h>

namespace kernel
{
#define PAGE_SIZE           4096UL
#define PAGE_OFFSET_MASK    (PAGE_SIZE-1)
#define PAGE_PFN_MASK       ~PAGE_OFFSET_MASK
#define __PAGE_ALIGNED__    __ALIGNED__(PAGE_SIZE)

#define HPAGE_SIZE          (2*1024*1024UL)
#define HPAGE_OFFSET_MASK   (HPAGE_SIZE-1)
#define HPAGE_PFN_MASK      ~HPAGE_OFFSET_MASK

#define GPAGE_SIZE          (1024*1024*1024UL)
#define GPAGE_OFFSET_MASK   (GPAGE_SIZE-1)
#define GPAGE_PFN_MASK      ~GPAGE_OFFSET_MASK

    void* page_alloc();
    inline void* page_zalloc()
    {
        void* p = page_alloc();
        memset(p,0,PAGE_SIZE);
        return p;
    }
    void page_free(void*);

    size_t page_count_total();
    size_t page_count_free();

    struct page_allocator
    {
        constexpr static size_t len   = PAGE_SIZE;
        constexpr static bool movable = true;
        inline void* alloc()       {return kernel::page_alloc();}
        inline void  free(void* p) {kernel::page_free(p);}
    };

    void page_preinit(const e820_map* m, uintptr_t top_addr,
                      dma_addr64 kernel_end);
    void page_init(const e820_map* m, uintptr_t top_addr);
}

#endif /* __KERNEL_PAGE_H */
