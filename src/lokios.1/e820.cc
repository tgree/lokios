#include "e820.h"
#include "kmath.h"
#include "console.h"
#include "sort.h"
#include "local_vector.h"
#include "region_set.h"
#include "page.h"
#include <string.h>

static const char* e820_type_lut[] =
{
    "reserved",
    "1 - RAM",
    "2 - reserved",
    "3 - ACPI reclaimable",
    "4 - ACPI NVS",
    "5 - bad RAM",
};

namespace kernel
{
    struct e820_vector : public local_vector<e820_entry>
    {
        void print(char_stream* cs)
        {
            for (const auto& e : *this)
            {
                const char* type =
                    e820_type_lut[e.type < nelems(e820_type_lut) ? e.type : 0];
                cs->printf("0x%016lX:0x%016lX %s\n",
                           e.base,e.base + e.len - 1,type);
            }
        }

        e820_vector(const e820_map* m, e820_entry* storage, size_t capacity,
                    uint64_t mask):
            local_vector(storage,capacity)
        {
            get_e820_map(m,*this,mask);
            sort::quicksort(*this);
        }
    };
}

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
