#include "page.h"
#include "buddy.h"
#include "kern/task.h"
#include "kern/cpu.h"
#include "kern/console.h"
#include "k++/local_vector.h"

using kernel::console::printf;

void*
kernel::page_alloc()
{
    return buddy_alloc_by_len(PAGE_SIZE);
}

void
kernel::page_free(void* p)
{
    buddy_free_by_len(p,PAGE_SIZE);
}

size_t
kernel::page_count_total()
{
    return buddy_count_total();
}

size_t
kernel::page_count_free()
{
    return buddy_count_free();
}

void
kernel::page_preinit(const e820_map* m, uint64_t top_addr,
    dma_addr64 bitmap_base)
{
    // Populate the buddy allocator with the free bootloader-mapped pages that
    // come following the end of the kernel image.

    // Parse the usable RAM regions out of the E820 map and subtract the
    // unusable ones for safety.
    local_vector<region> usable_regions;
    local_vector<region> unusable_regions;
    get_e820_regions(m,usable_regions,E820_TYPE_RAM_MASK);
    get_e820_regions(m,unusable_regions,~E820_TYPE_RAM_MASK);
    regions_remove(usable_regions,unusable_regions);

    // Dump the usable regions and find the end of RAM.
    printf("E820 usable RAM regions:\n");
    dma_addr64 last_dma = usable_regions[-1].last + 1;
    for (auto& r : usable_regions)
    {
        printf("  0x%016lX - 0x%016lX\n",r.first,r.last);
        if (r.last + 1 > last_dma)
            last_dma = r.last + 1;
    }

    // Remove everything above the top bootloader address since those pages,
    // while free, aren't currently mapped.
    region_remove(usable_regions,top_addr,0xFFFFFFFFFFFFFFFF);

    // Initialize and populate the buddy allocator.
    size_t bitmap_len = buddy_init(0,last_dma,bitmap_base);

    // Remove all pages below the end of the buddy bitmap.
    dma_addr64 bitmap_end = round_up_pow2(bitmap_base+bitmap_len,PAGE_SIZE);
    region_remove(usable_regions,0,bitmap_end);

    // Add all remaining pages to the buddy allocator.
    for (auto& r : usable_regions)
    {
        uint64_t begin_pfn = ceil_div(r.first,PAGE_SIZE);
        uint64_t end_pfn   = floor_div(r.last+1,PAGE_SIZE);
        if (begin_pfn > end_pfn)
            continue;

        for (uint64_t pfn = begin_pfn; pfn != end_pfn; ++pfn)
            buddy_populate(phys_to_virt(pfn*PAGE_SIZE),0);
    }
}

void
kernel::page_init(const e820_map* m, uint64_t top_addr)
{
    // Map all of the remaining pages not initially mapped by the bootloader
    // (i.e. the pages not handled in page_preinit()).

    // Build up the list of free regions, removing everything below the top
    // bootloader address.
    local_vector<region> unusable_regions;
    local_vector<region> free_regions;
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
            buddy_populate(phys_to_virt(paddr),ulog2(npfns));

            pfn      += npfns;
            rem_pfns -= npfns;
        }
    }
}
