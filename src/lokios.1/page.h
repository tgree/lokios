#ifndef __KERNEL_PAGE_H
#define __KERNEL_PAGE_H

#include "types.h"
#include "klist.h"
#include "e820.h"

namespace kernel
{
#define PAGE_SIZE           4096UL
#define PAGE_OFFSET_MASK    (PAGE_SIZE-1)
#define PAGE_PFN_MASK       ~PAGE_OFFSET_MASK

#define HPAGE_SIZE          (2*1024*1024UL)
#define HPAGE_OFFSET_MASK   (HPAGE_SIZE-1)
#define HPAGE_PFN_MASK      ~HPAGE_OFFSET_MASK
    struct page
    {
        klink   link;
        char    data[];
    };

    void* page_alloc();
    void page_free(void*);

    struct page_raii : public non_copyable
    {
        void*   addr;

        inline operator void*() {return addr;}
        inline page_raii():addr(page_alloc()) {}
        inline ~page_raii() {page_free(addr);}
    };

    void page_preinit(const e820_map* m, uintptr_t top_addr);
}

#endif /* __KERNEL_PAGE_H */
