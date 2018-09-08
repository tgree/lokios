#include "pci.h"
#include "iocfg.h"
#include "mcfg.h"
#include "acpi/tables.h"
#include "mm/mm.h"
#include "k++/sort.h"
#include "kern/console.h"
#include "kern/cpu.h"

using kernel::console::printf;

kernel::obj_list<kernel::pci::domain> kernel::pci::domains;
static kernel::klist<kernel::pci::driver> drivers;

static void
pci_dev_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    auto* pd = container_of(node,kernel::pci::dev,wapi_node);
    uint8_t ht = pd->config_read_8(14);
    rsp->printf("{\r\n"
                "    \"driver\"                : \"%s\",\r\n"
                "    \"slot\"                  : \"%02X:%u.%u\",\r\n"
                "    \"vendor_id\"             : \"0x%04X\",\r\n"
                "    \"device_id\"             : \"0x%04X\",\r\n"
                "    \"command\"               : \"0x%04X\",\r\n"
                "    \"status\"                : \"0x%04X\",\r\n"
                "    \"revision_id\"           : \"0x%02X\",\r\n"
                "    \"class_code\"            : \"0x%06X\",\r\n"
                "    \"cacheline_size\"        : \"0x%02X\",\r\n"
                "    \"latency_timer\"         : \"0x%02X\",\r\n"
                "    \"header_type\"           : \"0x%02X\",\r\n"
                "    \"bist\"                  : \"0x%02X\",\r\n",
                pd->owner->name,pd->bus,(pd->devfn >> 3),(pd->devfn & 7),
                pd->config_read_16(0),pd->config_read_16(2),
                pd->config_read_16(4),pd->config_read_16(6),
                pd->config_read_8(8),(pd->config_read_32(8) >> 8),
                pd->config_read_8(12),pd->config_read_8(13),
                ht,pd->config_read_8(15));
    switch (ht & 0x7F)
    {
        case 0x00:
            // Device.
            rsp->printf("    \"bars\"                  : [ "
                        "\"0x%08X\", \"0x%08X\", \"0x%08X\", \"0x%08X\", "
                        "\"0x%08X\", \"0x%08X\" ],\r\n"
                        "    \"cardbus_cis_ptr\"       : \"0x%08X\",\r\n"
                        "    \"subsystem_vendor_id\"   : \"0x%04X\",\r\n"
                        "    \"subsystem_id\"          : \"0x%04X\",\r\n"
                        "    \"expansion_rom_bar\"     : \"0x%08X\",\r\n"
                        "    \"capabilities_ptr\"      : \"0x%02X\",\r\n"
                        "    \"interrupt_line\"        : \"0x%02X\",\r\n"
                        "    \"interrupt_pin\"         : \"0x%02X\",\r\n"
                        "    \"min_gnt\"               : \"0x%02X\",\r\n"
                        "    \"max_lat\"               : \"0x%02X\"\r\n",
                        pd->config_read_32(16),pd->config_read_32(20),
                        pd->config_read_32(24),pd->config_read_32(28),
                        pd->config_read_32(32),pd->config_read_32(36),
                        pd->config_read_32(40),pd->config_read_16(44),
                        pd->config_read_16(46),pd->config_read_32(48),
                        pd->config_read_8(52),pd->config_read_8(60),
                        pd->config_read_8(61),pd->config_read_8(62),
                        pd->config_read_8(63));
        break;

        case 0x01:
            // PCI-to-PCI bridge.
            rsp->printf("    \"bars\"                  : [ "
                        "\"0x%08X\", \"0x%08X\" ],\r\n"
                        "    \"primary_bus_num\"       : \"0x%02X\",\r\n"
                        "    \"secondary_bus_num\"     : \"0x%02X\",\r\n"
                        "    \"subordinate_bus_num\"   : \"0x%02X\",\r\n"
                        "    \"secondary_lat_timer\"   : \"0x%02X\",\r\n"
                        "    \"io_base\"               : \"0x%02X\",\r\n"
                        "    \"io_limit\"              : \"0x%02X\",\r\n"
                        "    \"secondary_status\"      : \"0x%04X\",\r\n"
                        "    \"memory_base\"           : \"0x%04X\",\r\n"
                        "    \"memory_limit\"          : \"0x%04X\",\r\n"
                        "    \"prefetch_memory_base\"  : \"0x%04X\",\r\n"
                        "    \"prefetch_memory_limit\" : \"0x%04X\",\r\n"
                        "    \"prefetch_base_upper\"   : \"0x%08X\",\r\n"
                        "    \"prefetch_limit_upper\"  : \"0x%08X\",\r\n"
                        "    \"io_base_upper\"         : \"0x%04X\",\r\n"
                        "    \"io_limit_upper\"        : \"0x%04X\",\r\n"
                        "    \"capability_ptr\"        : \"0x%02X\",\r\n"
                        "    \"expansion_rom_bar\"     : \"0x%08X\",\r\n"
                        "    \"interrupt_line\"        : \"0x%02X\",\r\n"
                        "    \"interrupt_pin\"         : \"0x%02X\",\r\n"
                        "    \"bridge_control\"        : \"0x%04X\"\r\n",
                        pd->config_read_32(16),pd->config_read_32(20),
                        pd->config_read_8(24),pd->config_read_8(25),
                        pd->config_read_8(26),pd->config_read_8(27),
                        pd->config_read_8(28),pd->config_read_8(29),
                        pd->config_read_16(30),pd->config_read_16(32),
                        pd->config_read_16(34),pd->config_read_16(36),
                        pd->config_read_16(38),pd->config_read_32(40),
                        pd->config_read_32(44),pd->config_read_16(48),
                        pd->config_read_16(50),pd->config_read_8(52),
                        pd->config_read_32(56),pd->config_read_8(60),
                        pd->config_read_8(61),pd->config_read_16(62));
        break;

        default:
            rsp->printf("    \"more_stuff\" : \"header-type-0x%02X\"\r\n",
                        (ht & 0x7F));
        break;
    }
    rsp->printf("}\r\n");
}

kernel::pci::driver::driver(const char* name):
    name(name)
{
    drivers.push_back(&link);
}

kernel::pci::dev::dev(pci::domain* domain, uint8_t bus, uint8_t devfn):
    domain(domain),
    bus(bus),
    devfn(devfn),
    owner(NULL),
    wapi_node(func_delegate(pci_dev_request),METHOD_GET_MASK,"%02X:%u.%u",
              bus,(devfn >> 3),(devfn & 7)),
    msix_nvecs(0),
    msix_table(NULL)
{
}

kernel::pci::dev::dev(const dev* pd, const pci::driver* owner):
    domain(pd->domain),
    bus(pd->bus),
    devfn(pd->devfn),
    owner(owner),
    wapi_node(func_delegate(pci_dev_request),METHOD_GET_MASK,"%02X:%u.%u",
              bus,(devfn >> 3),(devfn & 7)),
    msix_nvecs(pd->msix_nvecs),
    msix_table(pd->msix_table)
{
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
kernel::pci::dev::map_msix_table()
{
    // Find the MSI-X capability.
    uint8_t msix_pos = find_pci_capability(0x11);
    kassert(msix_pos != 0);
    uint16_t msix_control = config_read_16(msix_pos + 2);
    uint32_t msix_table_offset = config_read_32(msix_pos + 4);

    // Map the table.
    msix_nvecs      = (msix_control & 0x7FF) + 1;
    uint8_t bar     = (msix_table_offset & 7);
    uint64_t offset = (msix_table_offset & ~7UL);
    msix_table = (msix_entry*)map_bar(bar,msix_nvecs*sizeof(msix_entry),offset);

    // Mask all interrupts.
    for (size_t i=0; i<msix_nvecs; ++i)
        msix_table[i].vector_control = 0x00000001;

    // Enable MSI-X.
    config_write_16((config_read_16(msix_pos + 2) & 0x3FF) | 0x8000,
                    msix_pos + 2);
}

kernel::wqe*
kernel::pci::dev::alloc_msix_vector(size_t vec, kernel::work_handler handler)
{
    if (!msix_table)
        map_msix_table();

    kassert(msix_table != 0);
    kassert(vec < msix_nvecs);

    cpu* c            = get_current_cpu();
    kernel::wqe* wqe  = c->alloc_msix_interrupt();
    msix_entry* me    = &msix_table[vec];
    me->msg_addr_high = 0;
    me->msg_addr_low  = 0xFEE00000;
    me->msg_data      = wqe - c->msix_entries;
    wqe->fn           = handler;
    wqe->args[0]      = (uintptr_t)this;
    wqe->args[1]      = (uintptr_t)&me->vector_control;
    return wqe;
}

void
kernel::pci::dev::enable_msix_vector(size_t vec)
{
    kassert(msix_table != 0);
    kassert(vec < msix_nvecs);
    msix_table[vec].vector_control = 0;
}

void
kernel::pci::dev::dump_msix_table()
{
    for (size_t i=0; i<msix_nvecs; ++i)
    {
        printf("msix %zu: 0x%08X%08X 0x%08X 0x%08X\n",
               i,
               msix_table[i].msg_addr_high,
               msix_table[i].msg_addr_low,
               msix_table[i].msg_data,
               msix_table[i].vector_control);
    }
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
            auto iter = domains.begin();
            for (; iter != domains.end() && iter->id < e->domain; ++iter)
                ;
            domains.emplace(iter,e->domain,e->start_bus_num,e->end_bus_num,mca);
        }
    }

    // If we didn't find domain 0 yet, add it using the IO config accessor.
    if (domains.empty() || domains.front().id != 0)
    {
        domains.emplace(domains.begin(),
                        0,0,255,new io_config_accessor(0xCF8,0xCFC));
    }

    // Probe all domains.
    for (auto& d : domains)
    {
        for (uint16_t bus=d.first_bus_num; bus<(uint16_t)d.last_bus_num + 1;
             ++bus)
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

                    auto* pdp = driver->claim(&pd);
                    d.devices.push_back(&pdp->domain_link);
                    driver->devices.push_back(&pdp->driver_link);
                    d.wapi_node.register_child(&pdp->wapi_node);
                }
            }
        }
    }

    // Register WAPI nodes.
    wapi::root_node.register_child(&kernel::pci::wapi_node);
}

static void
pci_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // GET /pci
    rsp->printf("{\r\n"
                "    \"domains\" : [");
    for (auto& d : kernel::pci::domains)
        rsp->printf(" \"0x%04X\",",d.id);
    rsp->ks.shrink();
    rsp->printf(" ]\r\n}\r\n");
}

wapi::node kernel::pci::wapi_node(func_delegate(pci_request),METHOD_GET_MASK,
                                  "pci");
