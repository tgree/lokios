#include "page.h"
#include "sbrk.h"
#include "local_vector.h"
#include "console.h"
#include <new>

static kernel::klist<kernel::page> free_page_list;

// Given a list of regions in some container, add them to the free page list.
template<typename C>
static void
populate_pages(const C& c)
{
    for (const kernel::region& r : c)
    {
        uintptr_t begin = r.first;
        uintptr_t end   = r.last + 1;
        if (begin & PAGE_OFFSET_MASK)
            begin = (begin + PAGE_OFFSET_MASK) & PAGE_PFN_MASK;
        if (end & PAGE_OFFSET_MASK)
            end &= PAGE_PFN_MASK;
        if (end <= begin)
            continue;

        uint64_t begin_pfn = begin/PAGE_SIZE;
        uint64_t end_pfn   = end/PAGE_SIZE;
        for (size_t pfn = begin_pfn; pfn != end_pfn; ++pfn)
        {
            void* paddr = (void*)(pfn*PAGE_SIZE);
            kernel::page* p = new(paddr) kernel::page;
            free_page_list.push_back(&p->link);
        }
    }
}

void
kernel::page_preinit(const e820_map* m, uint64_t top_addr)
{
    // Find out how much RAM the bootloader mapped.
    vga->printf("End of mapped RAM: 0x%016lX\n",top_addr);

    // Leave at least 1M of space in sbrk for early libsupc++ malloc() calls.
    // We round this up to a 2M hugepage boundary.
    uintptr_t sbrk_limit = (uintptr_t)sbrk(0) + 1024*1024;
    sbrk_limit = (sbrk_limit + 0x001FFFFF) & ~0x001FFFFFUL;
    kassert(sbrk_limit < top_addr);
    set_sbrk_limit((void*)sbrk_limit);

    // We need at least another 2M huge page to initially populate the free
    // page list.
    kassert(sbrk_limit + 2*1024*1024 <= top_addr);

    // Parse the usable RAM regions out of the E820 map.
    region _usable_regions[m->nentries];
    local_vector<region> usable_regions(_usable_regions,m->nentries);
    get_e820_regions(m,usable_regions,E820_TYPE_RAM_MASK);

    // Parse the unusable regions out of the E820 map.
    region _unusable_regions[m->nentries];
    local_vector<region> unusable_regions(_unusable_regions,m->nentries);
    get_e820_regions(m,unusable_regions,~E820_TYPE_RAM_MASK);

    // Remove all unusable regions in case BIOS gave us overlap.
    regions_remove(usable_regions,unusable_regions);

    // Remove everything above the top address.
    region_remove(usable_regions,top_addr,0xFFFFFFFFFFFFFFFF);

    // Remove everything below the sbrk limit.  This includes all of the
    // kernel image, all of BIOS stuff in low memory and anything that gets
    // allocated out of the sbrk pool.
    region_remove(usable_regions,0,(uintptr_t)get_sbrk_limit()-1);

    // Populate the free page list.
    populate_pages(usable_regions);

    // Walk the page list and tell everyone about it.
    size_t free_pages = free_page_list.size();
    vga->printf("Free mapped RAM: %zuMB (%zu 4K pages)\n",
                free_pages/256,free_pages);
}
