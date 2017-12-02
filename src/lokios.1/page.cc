#include "page.h"
#include "sbrk.h"
#include "console.h"
#include <new>

static kernel::meta_page* pages;
static size_t npages;
static kernel::klist<kernel::page> free_page_list;

void
kernel::page_init(local_vector<region>& ram)
{
    // Remove everything that we haven't mapped - currently lokios.0 only maps
    // the first 64M of RAM for us.  TODO: We should actually save off the
    // full memory map...
    region_remove(ram,0x04000000,0xFFFFFFFFFFFFFFFF);

    // Find the actual top of RAM.
    void* top_of_ram = (char*)ram[ram.size() - 1].last + 1;

    // Allocate memory for the meta page array.
    npages = (uintptr_t)top_of_ram/4096;
    pages = (meta_page*)sbrk(npages * sizeof(meta_page));

    // Leave at least 1M of space in sbrk for early libsupc++ malloc() calls.
    // We round this up to a 2M hugepage boundary.
    uintptr_t sbrk_limit = (uintptr_t)sbrk(0) + 1024*1024;
    sbrk_limit = (sbrk_limit + 0x001FFFFF) & ~0x001FFFFFUL;
    set_sbrk_limit((void*)sbrk_limit);

    // Remove everything below the sbrk limit.  This includes all of the
    // kernel image, all of BIOS stuff in low memory, the meta page array and
    // anything that gets allocated out of the sbrk pool.
    region_remove(ram,0,(uintptr_t)get_sbrk_limit()-1);

    // Start by marking all pages as reserved.
    for (size_t i=0; i<npages; ++i)
        pages[i].flags = PAGE_FLAG_RESERVED;

    // Now find all unused RAM pages and mark them as available.
    for (const region& r : ram)
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
            page* p = new(paddr) page;
            free_page_list.push_back(&p->link);
            pages[pfn].flags = 0;
        }
    }

    // Walk the page list and tell everyone about it.
    size_t free_pages = free_page_list.size();
    vga->printf("Free RAM: %zuMB (%zu 4K pages)\n",free_pages/256,free_pages);
}
