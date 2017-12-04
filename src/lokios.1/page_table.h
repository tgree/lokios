#ifndef __KERNEL_PAGE_TABLE_H
#define __KERNEL_PAGE_TABLE_H

#include "page.h"
#include "vector.h"
#include "x86.h"

namespace kernel
{
    // The PAT, PCD and PWT bits select an entry in the page attribute table.
    // The lokios bootloader reprograms the PAT MSR so that the PAT table is:
    //       0  1  2  3  4  5  6  7
    //      WB WT WC UC WB WT WC UC
    // In particular, since this configuration is repeated in both halves of
    // the MSR it means that the PAT bit in the page tables is "don't care".
    // We force it to 0 for 2M and 1G entries and force it to 1 for 4K entries
    // which has two effects:
    //  1. The physical address mask becomes consistent across all page table
    //     levels.
    //  2. Bit 7 can now be used at the 4K level to also indicate that no child
    //     node is present.  This is useful because we no longer need to
    //     special-case 4K pages.

    // These are all the user-visible flags.
#define PAGE_FLAG_NOEXEC        0x8000000000000000UL
#define PAGE_LEVEL_MASK         0x3000000000000000UL
#define PAGE_PADDR_MASK         0x000000FFFFFFF000UL
#define PAGE_FLAG_USER_PAGE     0x0000000000000080UL
#define PAGE_CACHE_MASK         0x000000000000000CUL
#define PAGE_CACHE_WB           0x0000000000000000UL
#define PAGE_CACHE_WT           0x0000000000000004UL
#define PAGE_CACHE_WC           0x0000000000000008UL
#define PAGE_CACHE_UC           0x000000000000000CUL
#define PAGE_FLAG_WRITEABLE     0x0000000000000002UL
#define PAGE_FLAG_PRESENT       0x0000000000000001UL

    // Flags that the user can modify; any other bits present when trying to
    // map a page are ignored.
#define PAGE_USER_FLAGS \
    (PAGE_FLAG_WRITEABLE | PAGE_FLAG_NOEXEC | PAGE_CACHE_MASK)

    // Given a PTE, extract its page table level (and hence it's page size).
#define PTE_TO_LEVEL(pte)   (((pte) & PAGE_LEVEL_MASK) >> 60)

    // Number of levels in the page table.
#define PAGE_TABLE_HEIGHT   4

    class page_table
    {
        uint64_t* cr3;

        uint64_t*   alloc_pte(uint64_t* entries, uint64_t vaddr, size_t level);
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

        inline page_table_iterator&       begin() {return *this;}
        inline const page_table_iterator& end()   {return *this;}
        void operator++();

        inline void* get_vaddr() const
        {
            uint64_t vaddr = 0;
            if (stack[0].index >= 0x0100)
                vaddr |= 0xFFFF000000000000;
            for (size_t i=0; i<=level; ++i)
                vaddr |= (((uint64_t)stack[i].index) << (39 - (level*9)));
            return (void*)vaddr;
        }

        inline page_table_entry operator*() const
        {
            return page_table_entry{get_vaddr(),
                                    stack[level].entries[stack[level].index]};
        }

        inline bool operator!=(const page_table_iterator&) const
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
