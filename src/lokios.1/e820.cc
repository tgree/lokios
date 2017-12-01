#include "e820.h"
#include "kmath.h"
#include "console.h"
#include "sort.h"
#include "local_vector.h"
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

static void
print_e820_entries(const kernel::local_vector<kernel::e820_entry>& entries)
{
    for (const auto& e : entries)
    {
        kernel::vga.printf("0x%016lX:0x%016lX %s\n",
                           e.base,e.base + e.len - 1,
                           e820_type_lut[e.type < kernel::nelems(e820_type_lut)
                                         ? e.type : 0]);
    }
}

void
kernel::parse_e820_map(const e820_map* m)
{
    kernel::vga.printf("E820 entry count: 0x%04X\n",m->nentries);
    kernel::local_vector<const e820_entry>
        all_entries(m->entries,m->nentries,m->nentries);

    e820_entry _ram_entries[m->nentries*2];
    kernel::local_vector<e820_entry> ram_entries(_ram_entries,m->nentries*2);
    for (const e820_entry& e : all_entries)
    {
        if (e.type == E820_TYPE_RAM)
            ram_entries.push_back(e);
    }

    kernel::sort::quicksort(ram_entries);
    print_e820_entries(ram_entries);
}
