#ifndef __KERNEL_MM_SLAB_H
#define __KERNEL_MM_SLAB_H

#include "page.h"
#include "k++/klist.h"
#include <new>

namespace kernel
{
    struct free_elem
    {
        klink link;
    };
    KASSERT(sizeof(free_elem) == 8);

    struct slab_page_footer
    {
        const uint32_t      signature;
        uint16_t            usage_count;
        uint16_t            rsrv0;
        klink               link;
        struct slab* const  slab;
        uint8_t             rsrv1[40];

        inline slab_page_footer(struct slab* slab):
            signature('SLAB'),
            usage_count(0),
            slab(slab)
        {
        }
    };
    KASSERT(sizeof(slab_page_footer) == 64);

    struct slab_page
    {
        uint8_t             data[PAGE_SIZE - sizeof(slab_page_footer)];
        slab_page_footer    footer;

        static constexpr uint16_t elem_count_for_elem_size(uint16_t elem_size)
        {
            return sizeof(data)/elem_size;
        }

        inline slab_page(slab* slab):footer(slab) {}
    };
    KASSERT(sizeof(slab_page) == PAGE_SIZE);

    class slab
    {
        klist<slab_page>    pages;
        klist<free_elem>    free_elems;
        const uint16_t      elem_size;
        const uint16_t      page_elem_count;

        inline slab_page* page_from_elem(void* p)
        {
            return (slab_page*)((uintptr_t)p & PAGE_PFN_MASK);
        }

        void page_alloc()
        {
            slab_page* p = new(kernel::page_alloc()) slab_page(this);
            uintptr_t ep = (uintptr_t)p->data;
            for (size_t i=0; i<page_elem_count; ++i)
            {
                free_elem* fe = (free_elem*)ep;
                new(fe) free_elem;
                free_elems.push_back(&fe->link);
                ep += elem_size;
            }

            pages.push_back(&p->footer.link);
        }

    public:
        void* _alloc()
        {
            if (free_elems.empty())
                page_alloc();

            free_elem* fe = klist_front(free_elems,link);
            free_elems.pop_front();
            fe->~free_elem();

            slab_page* p = page_from_elem(fe);
            ++p->footer.usage_count;

            return fe;
        }

        void* zalloc()
        {
            void* p = _alloc();
            memset(p,0,elem_size);
            return p;
        }

        template<typename T, typename ...Args>
        T* alloc(Args&& ...args)
        {
            kassert(sizeof(T) <= elem_size);
            return new(_alloc()) T(std::forward<Args>(args)...);
        }

        void free(void* e)
        {
            slab_page* p = page_from_elem(e);
            kassert(p->footer.slab == this);
            kassert((uintptr_t)p % elem_size == 0);
            --p->footer.usage_count;

            free_elem* fe = new(e) free_elem;
            free_elems.push_back(&fe->link);
        }

        template<typename T>
        void free(T* t)
        {
            t->~T();
            this->free((void*)t);
        }

        inline slab(size_t elem_size):
            elem_size(max(elem_size,sizeof(free_elem))),
            page_elem_count(slab_page::elem_count_for_elem_size(elem_size))
        {
        }

        ~slab()
        {
            free_elems.clear();
            while (!pages.empty())
            {
                slab_page* p = klist_front(pages,footer.link);
                pages.pop_front();
                kernel::page_free(p);
            }
        }
    };
}

#endif /* __KERNEL_MM_SLAB_H */
