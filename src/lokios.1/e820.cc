#include "e820.h"
#include "kmath.h"
#include "console.h"
#include "sort.h"
#include "local_vector.h"
#include "region_set.h"
#include "page.h"
#include <string.h>

void
kernel::parse_e820_map(const e820_map* m)
{
    // Build a RAM region vector.
    region _ram_regions[m->nentries];
    local_vector<region> ram_regions(_ram_regions,m->nentries);
    get_e820_regions(m,ram_regions,E820_TYPE_RAM_MASK);

    // Build a non-RAM region vector.
    region _nonram_regions[m->nentries];
    local_vector<region> nonram_regions(_nonram_regions,m->nentries);
    get_e820_regions(m,nonram_regions,~E820_TYPE_RAM_MASK);

    // Remove the non-RAM regions from the RAM regions in case BIOS gave us
    // some inconsistent overlapping stuff.
    for (const auto& e : nonram_regions)
        region_remove(ram_regions,e);

    // Hand the RAM regions over to the page component to build the page lists
    // and possibly remove more regions.
    page_init(ram_regions);

    // Dump the final result.
    vga->printf("Reduced RAM regions:\n");
    for (const auto& r : ram_regions)
        vga->printf("0x%016lX:0x%016lX\n",r.first,r.last);
}
