#ifndef __KERNEL_METAPAGE_H
#define __KERNEL_METAPAGE_H

#include "e820.h"
#include "kernel/kassert.h"
#include <stddef.h>

namespace kernel
{
    struct metapage;

    extern metapage* metapage_table;

    enum metapage_type
    {
        MPTYPE_NONE = 0,
        MPTYPE_RAM  = 1,
        MPTYPE_IO   = 2,
        MPTYPE_RSRV = 3,
    };

    struct metapage
    {
        uint32_t        link_pfn : 28,
                        in_use   : 1,
                        rsrv     : 1;
        metapage_type   type     : 2;
    };
    KASSERT(sizeof(metapage) == 4);

    metapage* metapage_queue_alloc(size_t npages);
    void metapage_queue_free(metapage* head);

    void metapage_init(const e820_map* m, uint64_t top_addr);
}

#endif /* __KERNEL_METAPAGE_H */
