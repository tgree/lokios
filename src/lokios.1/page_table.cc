#include "page_table.h"
#include <string.h>

kernel::page_table::page_table()
{
    pages.emplace_back();
    cr3 = (uint64_t*)pages[0].addr;
    memset(cr3,0,PAGE_SIZE);
}

uint64_t*
kernel::page_table::alloc_pte(uint64_t* entries, uint64_t vaddr, size_t level)
{
    uint16_t index = ((vaddr >> 39) & 0x01FF);
    uint64_t* pte = &entries[index];
    if (!level)
        return pte;

    if (!(*pte & 1))
    {
        void* child = pages.emplace_back().addr;
        memset(child,0,PAGE_SIZE);
        *pte = ((uint64_t)child | 3);
    }

    return alloc_pte((uint64_t*)(*pte & 0x000000FFFFFFF000),vaddr << 9,level-1);
}

void
kernel::page_table::map_page(void* _vaddr, uint64_t paddr,
    uint64_t page_flags, page_cache_flags cache_flags, size_t level,
    uint64_t pte_flags, uint64_t offset_mask)
{
    uintptr_t vaddr = (uintptr_t)_vaddr;
    kassert((vaddr & offset_mask) == 0);
    kassert((vaddr >> 47) == 0x00000 || (vaddr >> 47) == 0x1FFFF);
    kassert((paddr & offset_mask) == 0);
    kassert(0 <= cache_flags && cache_flags <= 3);

    uint64_t* pte = alloc_pte(cr3,vaddr,level);
    kassert(!(*pte & 1));

    uint64_t entry = (paddr | (1 << 0) | pte_flags);
    if (page_flags & PAGE_FLAG_WRITEABLE)
        entry |= (1 << 1);
    if (page_flags & PAGE_FLAG_NOEXEC)
        entry |= (1UL << 63);
    entry |= (uint64_t)cache_flags << 3;
    *pte = entry;
}

void
kernel::page_table::map_2m_page(void* vaddr, uint64_t paddr,
    uint64_t page_flags, page_cache_flags cache_flags)
{
    map_page(vaddr,paddr,page_flags,cache_flags,2,(1 << 7),HPAGE_OFFSET_MASK);
}

void
kernel::page_table::map_4k_page(void* vaddr, uint64_t paddr,
    uint64_t page_flags, page_cache_flags cache_flags)
{
    map_page(vaddr,paddr,page_flags,cache_flags,3,0,PAGE_OFFSET_MASK);
}

void
kernel::page_table_iterator::operator++()
{
    while (stack[0].index != 512)
    {
        // Did we exhaust this level?
        int16_t index = ++stack[level].index;
        if (index == 512)
        {
            --level;
            continue;
        }

        // Is this slot present?
        uint64_t pte = stack[level].entries[index];
        if (!(pte & 1))
            continue;

        // Check if we found a 4K page.
        if (level == 3)
            break;

        // Check if we found a huge page.
        if (level > 0 && level < 3 && (pte & (1<<7)))
            break;

        // Recurse.
        ++level;
        stack[level].entries = (uint64_t*)(pte & 0x000000FFFFFFF000);
        stack[level].index   = -1;
    }
}

kernel::page_table_iterator::page_table_entry
kernel::page_table_iterator::operator*() const
{
    uint64_t vaddr = 0;
    if (stack[0].index >= 0x0100)
        vaddr |= 0xFFFF000000000000;
    for (int i=0; i<=level; ++i)
        vaddr |= (((uint64_t)stack[i].index) << (39 - (level*9)));

    uint64_t pte   = stack[level].entries[stack[level].index];
    uint64_t pmask = (0x000000FFFFFFF000 << (27 - (level*9))) &
                     0x000000FFFFFFF000;
    uint64_t paddr = (pte & pmask);

    size_t len = 0x0000008000000000 >> (9*level);

    uint64_t page_flags = PAGE_FLAG_READABLE;
    if (pte & (1UL << 63))
        page_flags |= PAGE_FLAG_NOEXEC;
    if (pte & (1 << 1))
        page_flags |= PAGE_FLAG_WRITEABLE;

    page_cache_flags cache_flags = (page_cache_flags)((pte >> 3) & 3);

    return page_table_entry{(void*)vaddr,paddr,len,page_flags,cache_flags};
}
