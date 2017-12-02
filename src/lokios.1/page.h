#ifndef __KERNEL_PAGE_H
#define __KERNEL_PAGE_H

#include "klist.h"
#include "e820.h"

namespace kernel
{
#define PAGE_SIZE           4096UL
#define PAGE_OFFSET_MASK    (PAGE_SIZE-1)
#define PAGE_PFN_MASK       ~PAGE_OFFSET_MASK

    struct page
    {
        klink   link;
        char    data[];
    };

    void page_preinit(const e820_map* m, uintptr_t top_addr);
}

#endif /* __KERNEL_PAGE_H */
