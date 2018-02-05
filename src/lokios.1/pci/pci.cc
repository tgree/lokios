#include "pci.h"
#include "iocfg.h"
#include "mcfg.h"
#include "acpi/tables.h"
#include "mm/mm.h"
#include "k++/sort.h"

kernel::vector<kernel::pci::domain> kernel::pci::domains;
static kernel::klist<kernel::pci::driver> drivers;

kernel::pci::driver::driver(const char* name):
    name(name)
{
    drivers.push_back(&link);
}

uint8_t
kernel::pci::dev::find_pci_capability(uint8_t cap_id)
{
    for (uint8_t pos : cap_list())
    {
        if (config_read_8(pos) == cap_id)
            return pos;
    }
    return 0;
}

void*
kernel::pci::dev::map_bar(uint8_t bari, size_t len, size_t offset)
{
    kassert(bari < 6);
    uint32_t barl = config_read_32(0x10 + bari*4);
    uint64_t baru = 0;
    kassert((barl & 1) == 0); // IO BARs not supported
    if ((barl & 0x6) == 4)
        baru = config_read_32(0x10 + bari*4 + 4);
    return iomap_range(((baru << 32) | (barl & 0xFFFFFFF0)) + offset,len);
}

void
kernel::pci::init_pci()
{
    // Parse and MCFG entries first.
    const sdt_header* mcfgh = find_acpi_table(MCFG_SIG);
    if (mcfgh)
    {
        auto* mcfgt = (const mcfg_table*)mcfgh;
        size_t entries_len = (mcfgh->length - sizeof(*mcfgt));
        kassert(entries_len % sizeof(mcfg_entry) == 0);
        size_t nentries = entries_len/sizeof(mcfg_entry);
        for (size_t i=0; i<nentries; ++i)
        {
            const mcfg_entry* e = &mcfgt->entries[i];
            auto* mca = new mem_config_accessor(e->addr,e->start_bus_num,
                                                e->end_bus_num);
            domains.emplace_back(domain{e->domain,mca});
        }
    }

    // If we didn't find domain 0 yet, add it using the IO config accessor.
    sort::quicksort(domains);
    if (domains.empty() || domains[0].id != 0)
    {
        domains.emplace(domains.begin(),
                        domain{0,new io_config_accessor(0xCF8,0xCFC)});
    }

    // Probe all domains.
    for (auto& d : domains)
    {
        for (uint16_t bus=0; bus<256; ++bus)
        {
            for (uint8_t dev=0; dev<32; ++dev)
            {
                uint8_t hdrtype = d.cfg->config_read_8(bus,(dev << 3),14);
                if (hdrtype == 0xFF)
                    continue;

                uint8_t maxfns = (hdrtype & 0x80) ? 8 : 1;
                for (uint8_t fn=0; fn<maxfns; ++fn)
                {
                    uint8_t devfn = ((dev << 3) | fn);
                    pci::dev pd(&d,bus,devfn);
                    if (pd.config_read_16(0) == 0xFFFF)
                        continue;

                    pci::driver* driver = NULL;
                    uint64_t best_score = 0;
                    for (auto& drv : klist_elems(drivers,link))
                    {
                        uint64_t score = drv.score(&pd);
                        if (score > best_score)
                        {
                            driver     = &drv;
                            best_score = score;
                        }
                    }

                    if (!driver)
                        continue;

                    driver->devices.push_back(&driver->claim(&pd)->link);
                }
            }
        }
    }
}
