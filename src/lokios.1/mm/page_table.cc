#include "page_table.h"
#include <string.h>

kernel::page_table::page_table()
{
    cr3 = (uint64_t*)page_alloc();
    memset(cr3,0,PAGE_SIZE);
}

kernel::page_table::~page_table()
{
    for (auto pte : page_table_nonleaf_iterator(cr3))
        page_free((void*)(pte.pte & PAGE_PADDR_MASK));
    page_free(cr3);
}

uint64_t*
kernel::page_table::alloc_pte(uint64_t* entries, uint64_t vaddr, size_t level,
    size_t depth)
{
    uint16_t index = ((vaddr >> 39) & 0x01FF);
    uint64_t* pte = &entries[index];
    if (!level)
        return pte;

    if (!(*pte & PAGE_FLAG_PRESENT))
    {
        uint64_t child = (uint64_t)page_alloc();
        memset((void*)child,0,PAGE_SIZE);
        *pte = ((depth << 60)     |
                child             |
                PAGE_FLAG_PRESENT |
                PAGE_FLAG_WRITEABLE);
    }

    return alloc_pte((uint64_t*)(*pte & PAGE_PADDR_MASK),vaddr << 9,level-1,
                     depth+1);
}

void
kernel::page_table::map_page(void* _vaddr, uint64_t paddr, uint64_t flags,
    size_t level, uint64_t offset_mask)
{
    uintptr_t vaddr = (uintptr_t)_vaddr;
    kassert((vaddr & offset_mask) == 0);
    kassert((vaddr >> 47) == 0x00000 || (vaddr >> 47) == 0x1FFFF);
    kassert((paddr & offset_mask) == 0);

    uint64_t* pte = alloc_pte(cr3,vaddr,level);
    uint64_t val = ((level << 60)             |
                    paddr                     |
                    (flags & PAGE_USER_FLAGS) |
                    PAGE_FLAG_USER_PAGE       |
                    PAGE_FLAG_PRESENT);
    if (*pte & PAGE_FLAG_PRESENT)
    {
        kassert((*pte & PAGE_KERNEL_FLAGS) == (val & PAGE_KERNEL_FLAGS));
        return;
    }

    *pte = ((level << 60)             |
            paddr                     |
            (flags & PAGE_USER_FLAGS) |
            PAGE_FLAG_USER_PAGE       |
            PAGE_FLAG_PRESENT);
}

void
kernel::page_table::map_page(void* vaddr, uint64_t paddr, size_t size,
    uint64_t flags)
{
    kassert(is_pow2(size));
    kassert((size & (PAGE_SIZE | HPAGE_SIZE | GPAGE_SIZE)) != 0);
    size_t level = (39 - ulog2(size))/9;
    map_page(vaddr,paddr,flags,level,size-1);
}

void
kernel::page_table_iterator::operator++()
{
    while (stack[0].index != 512)
    {
        // Advance and make sure we didn't hit the end.
        size_t index = ++stack[level].index;
        if (stack[0].index == 512)
            break;

        // If we exhausted this level, pop back to the parent level.
        if (index == 512)
        {
            // If the user cares, yield it.
            index = stack[--level].index;
            uint64_t pte = stack[level].entries[index];
            if ((pte & set_mask) == set_flags)
                break;

            // Not interesting, continue.
            continue;
        }

        // Handle the case where the slot is populated.
        uint64_t pte = stack[level].entries[index];
        if (pte & PAGE_FLAG_PRESENT)
        {
            // Sanity that we've built things properly.
            kassert(PTE_TO_LEVEL(pte) == level);

            // If this maps a page we don't recurse.
            if (pte & PAGE_FLAG_USER_PAGE)
            {
                // If the user cares, yield.
                if ((pte & set_mask) == set_flags)
                    break;

                // Otherwise continue to the next slot.
                continue;
            }

            // This entry maps a child node in the tree.  Recurse into it; we'll
            // yield this current node after finishing the children.
            kassert(++level < nelems(stack));
            stack[level].entries = (uint64_t*)(pte & PAGE_PADDR_MASK);
            stack[level].index   = -1;
            continue;
        }

        // Finally, for an unpopulated slot check if the user cares.
        if ((pte & set_mask) == set_flags)
            break;
    }
}
