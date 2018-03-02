#ifndef __KERNEL_METAPAGE_H
#define __KERNEL_METAPAGE_H

#include "kernel/kassert.h"
#include <stddef.h>

namespace kernel
{
    struct metapage;

    extern metapage* metapage_table;

    enum metapage_type
    {
        MPTYPE_RAM  = 0,
        MPTYPE_IO   = 1,
        MPTYPE_NONE = 2,
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

    void metapage_init();
}

#endif /* __KERNEL_METAPAGE_H */
