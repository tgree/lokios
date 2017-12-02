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
#define E820_TYPE_RAM_MASK              (1 << E820_TYPE_RAM)
#define E820_TYPE_ACPI_RECLAIMABLE_MASK (1 << E820_TYPE_RECLAIMABLE)
#define E820_TYPE_ACPI_NVS_MASK         (1 << E820_TYPE_ACPI_NVS)
#define E820_TYPE_BAD_RAM_MASK          (1 << E820_TYPE_BAD_RAM)
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
            const e820_entry* e = m->entries;
            for (size_t i=0; i<m->nentries; ++i, ++e)
            {
                if (mask & (1 << e->type))
                    push_back(*e);
            }
            sort::quicksort(*this);
        }
    };
}

void
kernel::parse_e820_map(const e820_map* m)
{
    // Build a vector of just RAM.
    e820_entry _ram_entries[m->nentries];
    e820_vector ram_entries(m,_ram_entries,m->nentries,E820_TYPE_RAM_MASK);

    // Build a region vector, ensuring no zero-length regions.
    region _ram_regions[m->nentries];
    local_vector<region> ram_regions(_ram_regions,m->nentries);
    for (const auto& e : ram_entries)
    {
        if (e.len)
            ram_regions.push_back(region{e.base,e.base + e.len - 1});
    }

    // Build a vector of non-RAM stuff.
    size_t nnonram_entries = m->nentries - ram_entries.size();
    e820_entry _nonram_entries[nnonram_entries];
    e820_vector nonram_entries(m,_nonram_entries,nnonram_entries,
                               ~E820_TYPE_RAM_MASK);
    kassert(nonram_entries.size() == nnonram_entries);

    // Dump the tables.
    vga->printf("Non-RAM entries:\n");
    nonram_entries.print(vga);
    vga->printf("RAM entries:\n");
    ram_entries.print(vga);
    vga->printf("E820 entry count: %d\n",m->nentries);

    // Remove the non-RAM regions from the RAM regions in case BIOS gave us
    // some inconsistent overlapping stuff.
    for (const auto& e : nonram_entries)
        region_remove(ram_regions,e.base,e.base + e.len - 1);

    // Hand the RAM regions over to the page component to build the page lists
    // and possibly remove more regions.
    page_init(ram_regions);

    // Dump the final result.
    vga->printf("Reduced RAM regions:\n");
    for (const auto& r : ram_regions)
        vga->printf("0x%016lX:0x%016lX\n",r.first,r.last);
}
