#include "pci.h"
#include "iocfg.h"
#include "mcfg.h"
#include "acpi/tables.h"
#include "kernel/console.h"
#include "k++/sort.h"

using kernel::console::printf;

kernel::vector<kernel::pci::domain> kernel::pci::domains;

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
        printf("Probing PCI domain %04X...\n",d.id);
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
                    uint16_t vendor_id = d.cfg->config_read_16(bus,devfn,0);
                    if (vendor_id == 0xFFFF)
                        continue;

                    uint16_t device_id = d.cfg->config_read_16(bus,devfn,2);
                    printf("%02X:%02X.%u: %04X %04X\n",
                           bus,dev,fn,vendor_id,device_id);
                }
            }
        }
    }
}
