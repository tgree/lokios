#include "tables.h"
#include "../console.h"
#include "mm/e820.h"
#include "mm/mm.h"
#include "k++/vector.h"
#include "k++/checksum.h"

using kernel::console::printf;

kernel::vector<const kernel::sdt_header*> kernel::acpi_sdts;

static kernel::rsdp_table*
rsdp_search(const void* _base, size_t len)
{
    const uint64_t* end = (const uint64_t*)((uintptr_t)_base + len);
    end = kernel::min(end,(const uint64_t*)0x00100000);
    const uint64_t* start = kernel::round_up_pow2((const uint64_t*)_base,16);
    while (start < end)
    {
        if (*start == RDSP_SIG)
            return (kernel::rsdp_table*)start;
        start += 2;
    }
    return NULL;
}

static const kernel::sdt_header*
pmap_sdt(uint64_t paddr)
{
    auto* h = (const kernel::sdt_header*)kernel::pmap_range(
                paddr,sizeof(kernel::sdt_header));
    kernel::pmap_range(paddr,h->length);
    return h;
}

template<typename T>
static void
parse_acpi_sdts(T base)
{
    const kernel::sdt_header* sdt_hdr = pmap_sdt(base);
    const T* _sdts = (const T*)(sdt_hdr + 1);
    size_t nsdts = (sdt_hdr->length - sizeof(*sdt_hdr)) / sizeof(T);
    for (size_t i=0; i<nsdts; ++i)
    {
        auto* h = pmap_sdt(_sdts[i]);
        if (kernel::checksum<uint8_t>(h,h->length) != 0)
            continue;
        kernel::acpi_sdts.emplace_back(h);
    }
}

const kernel::sdt_header*
kernel::find_acpi_table(uint32_t signature)
{
    for (auto* h : acpi_sdts)
    {
        if (h->signature == signature)
            return h;
    }
    return NULL;
}

void
kernel::init_acpi_tables(const e820_map* m)
{
    rsdp_table* rsdp = NULL;

    // Start with the BIOS EBDA area.
    if (rsdp == NULL)
    {
        const void* ebdap = (const void*)(uintptr_t)*(uint16_t*)0x40E;
        rsdp = rsdp_search(ebdap,1024);
    }

    // Now try any E820 regions.
    if (rsdp == NULL)
    {
        kernel::vector<kernel::e820_entry> e820_entries;
        get_e820_map(m,e820_entries,~E820_TYPE_BAD_RAM_MASK);
        for (auto& e : e820_entries)
        {
            if (e.base >= 0x00100000)
                continue;

            rsdp = rsdp_search((const void*)e.base,e.len);
            if (rsdp)
                break;
        }
    }

    // Hopefully we found it.
    kassert(rsdp != NULL);

    // Checksum the legacy table.
    printf("Found RSDP at 0x%016lX\n",(uint64_t)rsdp);
    kassert(checksum<uint8_t>(rsdp,sizeof(*rsdp)) == 0);
    kassert(rsdp->revision == 0 || rsdp->revision == 2);

    // Try using the extended table.
    if (rsdp->revision == 2)
    {
        const rsdp_table_2* rsdp2 = (const rsdp_table_2*)rsdp;
        kassert(checksum<uint8_t>(rsdp2,sizeof(*rsdp2)) == 0);
        if (rsdp2->xsdt_base)
        {
            parse_acpi_sdts(rsdp2->xsdt_base);
            return;
        }
    }

    // Fall back to using the legacy table.
    parse_acpi_sdts(rsdp->rsdt_base);
}
