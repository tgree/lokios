#include "page.h"
#include "mm.h"
#include "sbrk.h"
#include "../spinlock.h"
#include "../task.h"
#include "../cpu.h"
#include "k++/local_vector.h"
#include <new>

struct page
{
    kernel::klink   link;
    char            data[4084];
};
KASSERT(sizeof(page) == PAGE_SIZE);

static kernel::spinlock     free_page_lock;
static kernel::klist<page>  free_page_list;

void*
kernel::page_alloc()
{
    page* p;

    with (free_page_lock)
    {
        kassert(!free_page_list.empty());
        p = klist_front(free_page_list,link);
        free_page_list.pop_front();
    }

    p->~page();
    return p;
}

void
kernel::page_free(void* _p)
{
    kassert(((uintptr_t)_p & PAGE_OFFSET_MASK) == 0);
    page* p = new(_p) page;
    with (free_page_lock)
    {
        free_page_list.push_front(&p->link);
    }
}

static void
page_add(void* _p)
{
    kernel::kassert(((uintptr_t)_p & PAGE_OFFSET_MASK) == 0);
    page* p = new(_p) page;
    with (free_page_lock)
    {
        free_page_list.push_back(&p->link);
    }
}

size_t
kernel::page_count_free()
{
    with (free_page_lock)
    {
        return free_page_list.size();
    }
}

// Given a list of regions in some container, add them to the free page list.
template<typename C>
static void
populate_pages(const C& c, kernel::klist<page>& fpl)
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
            void* _p = kernel::phys_to_virt(pfn*PAGE_SIZE);
            kernel::kassert(((uintptr_t)_p & PAGE_OFFSET_MASK) == 0);
            page* p = new(_p) page;
            fpl.push_back(&p->link);
        }
    }
}

void
kernel::page_preinit(const e820_map* m, uint64_t top_addr)
{
    // Find the initial sbrk position.
    dma_addr64 initial_sbrk = sbrk(0);

    // Leave at least 1M of space in sbrk for early libsupc++ malloc() calls.
    // We round this up to a 2M hugepage boundary.
    dma_addr64 sbrk_limit = initial_sbrk + 1024*1024;
    sbrk_limit = (sbrk_limit + 0x001FFFFF) & ~0x001FFFFFUL;
    kassert(sbrk_limit < top_addr);
    set_sbrk_limit(sbrk_limit);

    // We need 8K to back our vector<> objects.
    kassert(sbrk_limit + 2*4096 <= top_addr);

    // Parse the usable RAM regions out of the E820 map.
    sbrk_vector<region> usable_regions(4096);
    get_e820_regions(m,usable_regions,E820_TYPE_RAM_MASK);

    // Parse the unusable regions out of the E820 map.
    sbrk_vector<region> unusable_regions(4096);
    get_e820_regions(m,unusable_regions,~E820_TYPE_RAM_MASK);

    // Remove all unusable regions in case BIOS gave us overlap.
    regions_remove(usable_regions,unusable_regions);

    // Remove everything above the top address.
    region_remove(usable_regions,top_addr,0xFFFFFFFFFFFFFFFF);

    // Remove everything below the sbrk limit.  This includes all of the
    // kernel image, all of BIOS stuff in low memory and anything that gets
    // allocated out of the sbrk pool.
    region_remove(usable_regions,0,(uintptr_t)get_sbrk_limit()-1);

    // We are kind of relying on the fact that sbrk() can allocate a
    // contiguous range without there being any holes in the memory map.
    // Basically that means that after removing the top and bottom regions
    // above that we should have been reduced to a single range.
    kassert(usable_regions.size() == 1);
    kassert(usable_regions[0].first == (uintptr_t)get_sbrk_limit());
    kassert(usable_regions[0].last == top_addr - 1);

    // Populate the free page list.
    populate_pages(usable_regions,free_page_list);

    // Reset sbrk since we are done with the two pages backing our local
    // vectors.
    kassert(sbrk(0) == initial_sbrk + 8192);
    set_sbrk(initial_sbrk);
}

void
kernel::page_init(const e820_map* m, uint64_t top_addr)
{
    // Build up the list of free regions.
    vector<region> unusable_regions;
    vector<region> free_regions;
    get_e820_regions(m,free_regions,E820_TYPE_RAM_MASK);
    get_e820_regions(m,unusable_regions,~E820_TYPE_RAM_MASK);
    regions_remove(free_regions,unusable_regions);
    region_remove(free_regions,0,top_addr-1);

    // Now we are going to map all of the free RAM into the 0xFFFF800000000000
    // area.
    bool gp = (get_current_cpu()->flags & CPU_FLAG_PAGESIZE_1G);
    for (auto& r : free_regions)
    {
        // Map pages.
        uint64_t begin_pfn = ceil_div(r.first,PAGE_SIZE);
        uint64_t end_pfn   = floor_div(r.last+1,PAGE_SIZE);
        uint64_t rem_pfns  = end_pfn - begin_pfn;
        for (uint64_t pfn = begin_pfn; rem_pfns;)
        {
            uint64_t paddr    = pfn*PAGE_SIZE;
            uint64_t vaddr    = 0xFFFF800000000000UL | paddr;
            kassert((paddr & 0xFFFF800000000000UL) == 0);

            size_t npfns;
            if (gp && !(paddr & GPAGE_OFFSET_MASK) && rem_pfns >= 512*512)
            {
                kernel_task->pt.map_1g_page((void*)vaddr,paddr,PAGE_FLAGS_DATA);
                npfns = 512*512;
            }
            else if (!(paddr & HPAGE_OFFSET_MASK) && rem_pfns >= 512)
            {
                kernel_task->pt.map_2m_page((void*)vaddr,paddr,PAGE_FLAGS_DATA);
                npfns = 512;
            }
            else
            {
                kernel_task->pt.map_4k_page((void*)vaddr,paddr,PAGE_FLAGS_DATA);
                npfns = 1;
            }

            page* p = (page*)vaddr;
            for (size_t i=0; i<npfns; ++i)
                page_add(p++);

            pfn      += npfns;
            rem_pfns -= npfns;
        }
    }
}
