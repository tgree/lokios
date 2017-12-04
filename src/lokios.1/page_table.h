#ifndef __KERNEL_PAGE_TABLE_H
#define __KERNEL_PAGE_TABLE_H

#include "page.h"
#include "vector.h"
#include "x86.h"

namespace kernel
{
    // The PAT, PCD and PWT bits select an entry in the page attribute table.
    // On power-up reset, the page attribute table has the following
    // configuration:
    //      P P P
    //      A C W
    //      T D T State
    //      - - - -----
    //      0 0 0 WB
    //      0 0 1 WT
    //      0 1 0 UC-
    //      0 1 1 UC
    //      1 0 0 WB
    //      1 0 1 WT
    //      1 1 0 UC-
    //      1 1 1 UC
    //      -----------

    // These map directly to indices in the page attribute table register.
    enum page_cache_flags
    {
        PAGE_WB       = 0,
        PAGE_WT       = 1,
        PAGE_UC_MINUS = 2,
        PAGE_UC       = 3,
    };

#define PAGE_FLAG_READABLE      (1<<0)
#define PAGE_FLAG_WRITEABLE     (1<<1)
#define PAGE_FLAG_NOEXEC        (1<<2)

    class page_table
    {
        uint64_t*           cr3;
        vector<page_raii>   pages;

        uint64_t*   alloc_pte(uint64_t* entries, uint64_t vaddr, size_t level);

        void    map_page(void* vaddr, uint64_t paddr, uint64_t page_flags,
                         page_cache_flags cache_flags, size_t level,
                         uint64_t pte_flags, uint64_t offset_mask);
    public:
        // Activate the table.
        inline void activate() const {mtcr3((uint64_t)cr3);}

        // Map pages.
        void    map_1g_page(void* vaddr, uint64_t paddr, uint64_t page_flags,
                            page_cache_flags cache_flags);
        void    map_2m_page(void* vaddr, uint64_t paddr, uint64_t page_flags,
                            page_cache_flags cache_flags);
        void    map_4k_page(void* vaddr, uint64_t paddr, uint64_t page_flags,
                            page_cache_flags cache_flags);
        void    map_page(void* vaddr, uint64_t paddr, size_t page_size,
                         uint64_t page_flags, page_cache_flags cache_flags);

        // Create a blank table.
        page_table();
    };

    // This iterator iterates over all of the populated leaf nodes in the page
    // table (i.e. the PTEs that directly describe a present 1G, 2M or 4K
    // page).
    struct page_table_leaf_iterator
    {
        struct stack_frame
        {
            uint64_t*   entries;
            int16_t     index;
        };

        struct page_table_entry
        {
            void*               vaddr;
            uint64_t            paddr;
            size_t              len;
            uint64_t            page_flags;
            page_cache_flags    cache_flags;
        };

        int         level;
        stack_frame stack[4];

        inline page_table_leaf_iterator&        begin() {return *this;}
        inline const page_table_leaf_iterator&  end()   {return *this;}
        void                                    operator++();
        page_table_entry                        operator*() const;

        inline bool operator!=(const page_table_leaf_iterator&) const
        {
            return stack[0].index != 512;
        }

        page_table_leaf_iterator(uint64_t* cr3):
            level(0)
        {
            stack[0].entries = cr3;
            stack[0].index   = -1;
            ++(*this);
        }
    };

    // This iterator iterates over all of the populated internal nodes in the
    // page table that aren't leaf nodes (i.e. they contain pointers to a
    // child level in the page table).  The traversal is depth-first; if you
    // want to remove the child page from the page table that is safe because
    // we've already iterated over it.
    struct page_table_nonleaf_iterator
    {
        struct stack_frame
        {
            uint64_t*   entries;
            int16_t     index;
        };

        int         level;
        stack_frame stack[3];

        inline page_table_nonleaf_iterator&         begin() {return *this;}
        inline const page_table_nonleaf_iterator&   end()   {return *this;}
        void                                        operator++();

        inline uint64_t& operator*() const
        {
            return stack[level].entries[stack[level].index];
        }

        inline bool operator!=(const page_table_nonleaf_iterator&) const
        {
            return stack[0].index != 512;
        }

        page_table_nonleaf_iterator(uint64_t* cr3):
            level(0)
        {
            stack[0].entries = cr3;
            stack[0].index   = -1;
            ++(*this);
        }
    };
}

#endif /* __KERNEL_PAGE_TABLE_H */
