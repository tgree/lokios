#ifndef __KERNEL_PAGE_H
#define __KERNEL_PAGE_H

#include "klist.h"
#include "local_vector.h"
#include "region_set.h"

namespace kernel
{
#define PAGE_SIZE           4096UL
#define PAGE_OFFSET_MASK    (PAGE_SIZE-1)
#define PAGE_PFN_MASK       ~PAGE_OFFSET_MASK

#define PAGE_FLAG_RESERVED  (1<<0)
    struct meta_page
    {
        uint64_t    flags;
    };

    struct page
    {
        klink   link;
        char    data[];
    };

    void page_init(local_vector<region>& ram);
}

#endif /* __KERNEL_PAGE_H */
