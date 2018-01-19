#ifndef __KERNEL_MM_SLAB_H
#define __KERNEL_MM_SLAB_H

#include "page.h"
#include "k++/klist.h"
#include <new>

namespace kernel
{
    template<size_t ElemSize>
    struct slab
    {
        struct free_elem
        {
            klink   link;
            uint8_t pad[ElemSize - sizeof(klink)];
        };
        KASSERT(sizeof(free_elem) == ElemSize);

        struct page_footer
        {
            uint32_t        signature;
            uint32_t        usage_count;
            klink           link;
            struct slab*    slab;
            uint8_t         rsrv[40];
        };
        KASSERT(sizeof(page_footer) == 64);

        struct page
        {
            uint8_t     elem_base[PAGE_SIZE - sizeof(page_footer)];
            page_footer footer;

            static constexpr size_t elem_count = sizeof(elem_base)/ElemSize;
        };

        klist<page>         pages;
        klist<free_elem>    free_elems;

        inline page* page_from_elem(void* p)
        {
            return (page*)((uintptr_t)p & PAGE_PFN_MASK);
        }

        void page_alloc()
        {
            void* kp              = kernel::page_alloc();
            page* p               = new(kp) page();
            p->footer.signature   = 0x12345678;
            p->footer.usage_count = 0;
            p->footer.slab        = this;
            free_elem* fe         = (free_elem*)p->elem_base;
            for (size_t i=0; i<p->elem_count; ++i)
            {
                new(fe) free_elem;
                free_elems.push_back(&fe->link);
                ++fe;
            }
            pages.push_back(&p->footer.link);
        }

        void* alloc()
        {
            if (free_elems.empty())
                page_alloc();

            free_elem* fe = klist_front(free_elems,link);
            free_elems.pop_front();
            fe->~free_elem();

            page* p = page_from_elem(fe);
            ++p->footer.usage_count;

            return fe;
        }

        void free(void* e)
        {
            page* p = page_from_elem(e);
            kassert(p->footer.slab == this);
            kassert((uintptr_t)p % ElemSize == 0);
            --p->footer.usage_count;

            free_elem* fe = new(e) free_elem;
            free_elems.push_back(&fe->link);
        }

        ~slab()
        {
            free_elems.clear();
            while (!pages.empty())
            {
                page* p = klist_front(pages,footer.link);
                pages.pop_front();
                kernel::page_free(p);
            }
        }
    };
}

#endif /* __KERNEL_MM_SLAB_H */
