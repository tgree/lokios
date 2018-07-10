#include "page_table.h"
#include "mm.h"
#include "../tlb.h"
#include <string.h>

kernel::page_table::page_table()
{
    page_count = 1;
    cr3 = (uint64_t*)page_zalloc();
}

kernel::page_table::~page_table()
{
    for (auto pte : page_table_nonleaf_iterator(cr3))
    {
        page_free(phys_to_virt(pte.pte & PAGE_PADDR_MASK));
        --page_count;
    }
    --page_count;
    page_free(cr3);

    kassert(page_count == 0);
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
        ++page_count;
        auto child = page_zalloc();
        *pte = ((depth << 60)       |
                virt_to_phys(child) |
                PAGE_FLAG_PRESENT   |
                PAGE_FLAG_WRITEABLE);
    }

    return alloc_pte((uint64_t*)phys_to_virt(*pte & PAGE_PADDR_MASK),vaddr << 9,
                     level-1,depth+1);
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

    *pte = val;
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
kernel::page_table::unmap_page(void* vaddr)
{
    uint64_t* pte = find_pte(vaddr);
    kassert(*pte & PAGE_FLAG_PRESENT);
    kassert(*pte & PAGE_FLAG_USER_PAGE);
    *pte = 0;
    kernel::tlb_shootdown();
}

uint64_t*
kernel::page_table::find_pte(const void* _vaddr) const
{
    const uintptr_t vaddr  = (uintptr_t)_vaddr;
    kassert((vaddr & 0xFFFF800000000000UL) == 0 ||
            (vaddr & 0xFFFF800000000000UL) == 0xFFFF800000000000);

    uint64_t* page = cr3;
    size_t shift   = 39;
    for (;;)
    {
        size_t index = ((vaddr >> shift) & 0x1FF);
        uint64_t pte = page[index];
        if (!(pte & PAGE_FLAG_PRESENT))
            return &page[index];
        if (pte & PAGE_FLAG_USER_PAGE)
            return &page[index];

        page   = (uint64_t*)phys_to_virt(pte & PAGE_PADDR_MASK);
        shift -= 9;
    }
}

dma_addr64
kernel::page_table::xlate(const void* vaddr) const
{
    uint64_t* pte = find_pte(vaddr);
    kassert(*pte & PAGE_FLAG_PRESENT);
    kassert(*pte & PAGE_FLAG_USER_PAGE);

    dma_addr64 paddr = (*pte & PAGE_PADDR_MASK);
    if (paddr & 0x0000800000000000)
        paddr |= 0xFFFF000000000000;

    size_t level      = ((*pte & PAGE_LEVEL_MASK) >> 60);
    uint64_t low_mask = (0x0000007FFFFFFFFF >> (9*level));
    kassert(!(paddr & low_mask));
    paddr |= ((uintptr_t)vaddr & low_mask);

    return paddr;
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
            stack[level].entries =
                (uint64_t*)phys_to_virt(pte & PAGE_PADDR_MASK);
            stack[level].index   = -1;
            continue;
        }

        // Finally, for an unpopulated slot check if the user cares.
        if ((pte & set_mask) == set_flags)
            break;
    }
}
