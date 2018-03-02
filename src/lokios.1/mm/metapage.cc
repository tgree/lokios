#include "metapage.h"
#include "../task.h"
#include "../console.h"
#include "k++/vector.h"

using kernel::console::printf;

static size_t max_pfn = 0;
kernel::metapage* kernel::metapage_table = (metapage*)0xFFFF900000000000;

static const char* metatype_table[] = {"None", "RAM ", "IO  ", "Rsrv"};

static dma_addr64
region_alloc_2m(kernel::vector<kernel::region>& free_regions)
{
    for (auto& r : free_regions)
    {
        uint64_t first = kernel::round_up_pow2(r.first,HPAGE_SIZE);
        if (first + HPAGE_SIZE <= r.last)
        {
            kernel::region_remove(free_regions,first,first+HPAGE_SIZE-1);
            return first;
        }
    }

    return 0;
}

void
kernel::metapage_init(const e820_map* m, uint64_t top_addr)
{
    // Get the list of non-RAM regions.
    vector<region> non_ram_regions;
    get_e820_regions(m,non_ram_regions,~E820_TYPE_RAM_MASK);

    // Get the list of all RAM regions and subtract the non-RAM ones in case
    // there is overlap.
    vector<region> ram_regions;
    get_e820_regions(m,ram_regions,E820_TYPE_RAM_MASK);
    regions_remove(ram_regions,non_ram_regions);

    // Find the highest address.
    dma_addr64 highest_addr = 0;
    for (auto& r : ram_regions)
        highest_addr = max(r.last+1,highest_addr);

    // Compute the number of pfns we need to track.
    size_t npfns = ceil_div(highest_addr,PAGE_SIZE);

    // Initialize the list of used RAM to include the kernel RAM.
    vector<region> used_regions;
    used_regions.push_back(region{0,top_addr-1});

    // Get the list of free regions by subtracting the used RAM from all RAM.
    vector<region> free_regions(ram_regions);
    regions_remove(free_regions,used_regions);

    // Build the metatable.
    auto* mpe = metapage_table;
    for (; max_pfn < npfns;)
    {
        // Map the next 2M of the metatable.
        dma_addr64 paddr = region_alloc_2m(free_regions);
        kernel_task->pt.map_2m_page(mpe,paddr,PAGE_FLAGS_DATA);
        used_regions.push_back(region{paddr,paddr+HPAGE_SIZE-1});

        // We just used a 2M page out of the available free regions.  We need
        // to mark those pfns as in-use if they already have been populated in
        // the metatable.
        for (size_t i = 0; i < HPAGE_SIZE/sizeof(metapage); ++i)
        {
            if (paddr/PAGE_SIZE + i < max_pfn)
                metapage_table[paddr/PAGE_SIZE + i].in_use = true;
        }

        // Mark all newly-present PFNs in the metatable.
        for (size_t pfn = max_pfn; pfn < HPAGE_SIZE/sizeof(metapage); ++pfn)
        {
            mpe->link_pfn = 0;
            mpe->rsrv     = 0;
            if (!regions_contains(ram_regions,pfn*PAGE_SIZE))
            {
                mpe->in_use = true;
                mpe->type   = MPTYPE_NONE;
            }
            else
            {
                if (regions_contains(used_regions,pfn*PAGE_SIZE))
                    mpe->in_use = true;
                else
                    mpe->in_use = false;
                mpe->type = MPTYPE_RAM;
            }
            ++max_pfn;
            ++mpe;
        }
    }

    // Dump the metatable.
    auto* prev = metapage_table;
    for (size_t i=1; i<max_pfn; ++i)
    {
        mpe = &metapage_table[i];
        if (mpe->in_use == prev->in_use &&
            mpe->type   == prev->type)
        {
            continue;
        }

        printf("0x%016lX - 0x%016lX: %s %s\n",
               (prev - metapage_table)*PAGE_SIZE,
               (mpe  - metapage_table)*PAGE_SIZE-1,
               metatype_table[prev->type],prev->in_use ? "in_use" : "free");

        prev = mpe;
    }
}
