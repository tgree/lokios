#ifndef __KERNEL_PAGE_TABLE_H
#define __KERNEL_PAGE_TABLE_H

#include "page.h"
#include "page_flags.h"
#include "kernel/x86.h"

class tmock_test;

namespace kernel
{
    class page_table
    {
        uint64_t* cr3;

        uint64_t*   alloc_pte(uint64_t* entries, uint64_t vaddr, size_t level,
                              size_t depth = 0);
        void        map_page(void* vaddr, uint64_t paddr, uint64_t flags,
                             size_t level, uint64_t offset_mask);

    public:
        // Activate the table.
        inline void activate() const {mtcr3((uint64_t)cr3);}

        // Map pages if you only know the size dynamically.
        void map_page(void* vaddr, uint64_t paddr, size_t size, uint64_t flags);

        // Map pages if you know the static size.
        inline void map_1g_page(void* vaddr, uint64_t paddr, uint64_t flags)
            {map_page(vaddr,paddr,flags,1,GPAGE_OFFSET_MASK);}
        inline void map_2m_page(void* vaddr, uint64_t paddr, uint64_t flags)
            {map_page(vaddr,paddr,flags,2,HPAGE_OFFSET_MASK);}
        inline void map_4k_page(void* vaddr, uint64_t paddr, uint64_t flags)
            {map_page(vaddr,paddr,flags,3,PAGE_OFFSET_MASK);}

        // Create a blank table.
        page_table();
        ~page_table();

        friend class ::tmock_test;
    };

    // Type returned from the page table iterator classes.
    struct page_table_entry
    {
        void*       vaddr;
        uint64_t    pte;

        inline uint64_t get_paddr() const
        {
            return pte & PAGE_PADDR_MASK;
        }

        inline uint64_t get_len() const
        {
            return (1UL << (39 - PTE_TO_LEVEL(pte)*9));
        }
    };

    // Iterate over all present PTEs according to a masking rule specified by
    // the user.  A PTE will be yielded if:
    //      (pte & set_mask) == set_flags
    struct page_table_iterator
    {
        size_t level;
        struct
        {
            uint64_t*   entries;
            size_t      index;
        } stack[PAGE_TABLE_HEIGHT];

        const uint64_t  set_mask;
        const uint64_t  set_flags;

        inline page_table_iterator& begin()     {return *this;}
        inline kernel::end_sentinel end() const {return kernel::end_sentinel();}
        void operator++();

        inline void* get_vaddr() const
        {
            uint64_t vaddr = 0;
            if (stack[0].index >= 0x0100)
                vaddr |= 0xFFFF000000000000;
            for (size_t i=0; i<=level; ++i)
                vaddr |= (((uint64_t)stack[i].index) << (39 - (i*9)));
            return (void*)vaddr;
        }

        inline page_table_entry operator*() const
        {
            return page_table_entry{get_vaddr(),
                                    stack[level].entries[stack[level].index]};
        }

        inline bool operator!=(kernel::end_sentinel) const
        {
            return stack[0].index != 512;
        }

        page_table_iterator(uint64_t* cr3, uint64_t set_mask,
            uint64_t set_flags):
                level(0),
                set_mask(set_mask),
                set_flags(set_flags)
        {
            stack[0].entries = cr3;
            stack[0].index   = -1;
            ++(*this);
        }
    };

    // This iterates over all nodes that are marked as present.
    struct page_table_present_iterator : public page_table_iterator
    {
        page_table_present_iterator(uint64_t* cr3):
            page_table_iterator(cr3,PAGE_FLAG_PRESENT,PAGE_FLAG_PRESENT) {}
    };

    // This iterator iterates over all of the populated leaf nodes in the page
    // table (i.e. the PTEs that directly describe a present 1G, 2M or 4K
    // page).
    struct page_table_leaf_iterator : public page_table_iterator
    {
        page_table_leaf_iterator(uint64_t* cr3):
            page_table_iterator(cr3,PAGE_FLAG_USER_PAGE,PAGE_FLAG_USER_PAGE) {}
    };

    // This iterator iterates over all of the populated internal nodes in the
    // page table that aren't leaf nodes (i.e. they contain pointers to a
    // child level in the page table).  The traversal is depth-first; if you
    // want to remove the child page from the page table that is safe because
    // we've already iterated over it.
    struct page_table_nonleaf_iterator : public page_table_iterator
    {
        page_table_nonleaf_iterator(uint64_t* cr3):
            page_table_iterator(cr3,PAGE_FLAG_USER_PAGE | PAGE_FLAG_PRESENT,
                                PAGE_FLAG_PRESENT) {}
    };
}

#endif /* __KERNEL_PAGE_TABLE_H */
