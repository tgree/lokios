#include "page.h"
#include "buddy_allocator.h"
#include "kern/task.h"
#include "kern/cpu.h"
#include "kern/console.h"
#include "k++/local_vector.h"

using kernel::console::printf;

void*
kernel::page_alloc()
{
    return buddy_alloc(0);
}

void
kernel::page_free(void* p)
{
    buddy_free(p,0);
}

size_t
kernel::page_count_free()
{
    return buddy_count_free();
}

void
kernel::page_preinit(const e820_map* m, uint64_t top_addr)
{
    // Populate the buddy allocator with the free bootloader-mapped pages that
    // come following the sbrk region.

    // Parse the usable RAM regions out of the E820 map and subtract the
    // unusable ones for safety.
    local_vector<region> usable_regions;
    local_vector<region> unusable_regions;
    get_e820_regions(m,usable_regions,E820_TYPE_RAM_MASK);
    get_e820_regions(m,unusable_regions,~E820_TYPE_RAM_MASK);
    regions_remove(usable_regions,unusable_regions);

    // Remove everything below the sbrk limit.  This includes all of the
    // kernel image, all of BIOS stuff in low memory and anything that gets
    // allocated out of the sbrk pool.
    region_remove(usable_regions,0,(uintptr_t)get_sbrk_limit()-1);

    // What we have now is the working set of free pages for use by the kernel,
    // following the sbrk region.  Record this for later.
    dma_addr64 first_dma = usable_regions[0].first;
    dma_addr64 last_dma  = usable_regions[-1].last + 1;

    // Dump the usable regions and ensure they are sorted.
    printf("E820 usable RAM regions:\n");
    bool sorted   = true;
    uint64_t prev = 0;
    for (auto& r : usable_regions)
    {
        printf("  0x%016lX - 0x%016lX\n",r.first,r.last);
        if (prev >= r.first)
            sorted = false;
        prev = r.last;
    }
    kassert(sorted);

    // Remove everything above the top bootloader address since those pages,
    // while free, aren't currently mapped.
    region_remove(usable_regions,top_addr,0xFFFFFFFFFFFFFFFF);

    // We are kind of relying on the fact that sbrk() can allocate a
    // contiguous range without there being any holes in the memory map.
    // Basically that means that after removing the top and bottom regions
    // above that we should have been reduced to a single range.
    kassert(usable_regions.size() == 1);
    kassert(usable_regions[0].first == (uintptr_t)get_sbrk_limit());
    kassert(usable_regions[0].last == top_addr - 1);

    // Initialize and populate the buddy allocator.
    uint64_t begin_pfn = ceil_div(usable_regions[0].first,PAGE_SIZE);
    uint64_t end_pfn   = floor_div(usable_regions[0].last+1,PAGE_SIZE);
    kassert(begin_pfn < end_pfn);
    buddy_init(first_dma,last_dma - first_dma);
    for (uint64_t pfn = begin_pfn; pfn != end_pfn; ++pfn)
        buddy_free(phys_to_virt(pfn*PAGE_SIZE),0);
}

void
kernel::page_init(const e820_map* m, uint64_t top_addr)
{
    // Map all of the remaining pages not initially mapped by the bootloader
    // (i.e. the pages not handled in page_preinit()).

    // Build up the list of free regions, removing everything below the top
    // bootloader address.
    vector<region> unusable_regions;
    vector<region> free_regions;
    get_e820_regions(m,free_regions,E820_TYPE_RAM_MASK);
    get_e820_regions(m,unusable_regions,~E820_TYPE_RAM_MASK);
    regions_remove(free_regions,unusable_regions);
    region_remove(free_regions,0,top_addr-1);

    // Now we are going to map all of the remaining free RAM into the
    // 0xFFFF800000000000 area.
    bool gp = (get_current_cpu()->flags & CPU_FLAG_PAGESIZE_1G);
    for (auto& r : free_regions)
    {
        uint64_t begin_pfn = ceil_div(r.first,PAGE_SIZE);
        uint64_t end_pfn   = floor_div(r.last+1,PAGE_SIZE);
        if (begin_pfn > end_pfn)
            continue;

        // Map pages.
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
            buddy_free(phys_to_virt(paddr),ulog2(npfns));

            pfn      += npfns;
            rem_pfns -= npfns;
        }
    }
}
