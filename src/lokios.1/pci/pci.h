#ifndef __KERNEL_PCI_H
#define __KERNEL_PCI_H

#include "domain.h"
#include "kern/schedule.h"
#include "wapi/wapi.h"
#include "k++/obj_list.h"

namespace kernel::pci
{
    struct dev;

    struct msix_entry
    {
        volatile uint32_t    msg_addr_low;
        volatile uint32_t    msg_addr_high;
        volatile uint32_t    msg_data;
        volatile uint32_t    vector_control;
    };
    KASSERT(sizeof(msix_entry) == 16);

    struct driver
    {
        klink           link;
        const char*     name;
        klist<pci::dev> devices;

        virtual uint64_t    score(pci::dev* pd) const = 0;
        virtual pci::dev*   claim(pci::dev* pd) const = 0;
        virtual void        release(pci::dev* pd) const = 0;

        driver(const char* name);
    };

    struct dev
    {
        struct cap_list_adapter
        {
            pci::dev*   dev;
            uint8_t     pos;

            inline uint8_t operator*() const
            {
                return pos;
            }

            inline void operator++()
            {
                pos = dev->config_read_8(pos + 1);
            }

            inline bool operator!=(kernel::end_sentinel) const
            {
                return pos != 0 && pos < 254;
            }

            inline cap_list_adapter begin()
            {
                return *this;
            }

            inline kernel::end_sentinel end() const
            {
                return kernel::end_sentinel();
            }

            inline cap_list_adapter(pci::dev* dev):
                dev(dev)
            {
                if (dev->config_read_16(0x06) & 0x0010)
                    pos = dev->config_read_8(0x34);
                else
                    pos = 0;
            }
        };

        klink               domain_link;
        klink               driver_link;
        pci::domain*        domain;
        const uint8_t       bus;
        const uint8_t       devfn;
        const pci::driver*  owner;

        size_t              msix_nvecs;
        msix_entry*         msix_table;

        inline uint8_t config_read_8(uint16_t offset)
            {return domain->cfg->config_read_8(bus,devfn,offset);}
        inline uint16_t config_read_16(uint16_t offset)
            {return domain->cfg->config_read_16(bus,devfn,offset);}
        inline uint32_t config_read_32(uint16_t offset)
            {return domain->cfg->config_read_32(bus,devfn,offset);}
        inline uint64_t config_read_64(uint16_t offset)
            {return domain->cfg->config_read_64(bus,devfn,offset);}

        inline void config_write_8(uint8_t val, uint16_t offset)
            {domain->cfg->config_write_8(val,bus,devfn,offset);}
        inline void config_write_16(uint16_t val, uint16_t offset)
            {domain->cfg->config_write_16(val,bus,devfn,offset);}
        inline void config_write_32(uint32_t val, uint16_t offset)
            {domain->cfg->config_write_32(val,bus,devfn,offset);}
        inline void config_write_64(uint64_t val, uint16_t offset)
            {domain->cfg->config_write_64(val,bus,devfn,offset);}

        inline void config_set_clear_8(uint8_t s, uint8_t c, uint16_t offset)
        {
            kassert(!(s & c));
            uint8_t v = config_read_8(offset);
            config_write_8((v | s) & ~c,offset);
        }
        inline void config_set_clear_16(uint16_t s, uint16_t c, uint16_t offset)
        {
            kassert(!(s & c));
            uint16_t v = config_read_16(offset);
            config_write_16((v | s) & ~c,offset);
        }
        inline void config_set_clear_32(uint32_t s, uint32_t c, uint16_t offset)
        {
            kassert(!(s & c));
            uint32_t v = config_read_32(offset);
            config_write_32((v | s) & ~c,offset);
        }
        inline void config_set_8(uint8_t s, uint16_t offset)
            {config_set_clear_8(s,0,offset);}
        inline void config_set_16(uint16_t s, uint16_t offset)
            {config_set_clear_16(s,0,offset);}
        inline void config_set_32(uint32_t s, uint16_t offset)
            {config_set_clear_32(s,0,offset);}
        inline void config_clear_8(uint8_t c, uint16_t offset)
            {config_set_clear_8(0,c,offset);}
        inline void config_clear_16(uint16_t c, uint16_t offset)
            {config_set_clear_16(0,c,offset);}
        inline void config_clear_32(uint32_t c, uint16_t offset)
            {config_set_clear_32(0,c,offset);}

        inline uint32_t config_extract_32(uint8_t low, uint8_t high,
                                          uint16_t offset)
        {
            kassert(low <= high);
            kassert(high < 32);
            uint32_t mask = ((1 << (high+1)) - 1) ^ ((1 << low) - 1);
            return (config_read_32(offset) & mask) >> low;
        }
        inline void config_insert_32(uint32_t val, uint8_t low, uint8_t high,
                                     uint16_t offset)
        {
            kassert(low <= high);
            kassert(high < 32);
            val <<= low;
            uint32_t mask = ((1 << (high+1)) - 1) ^ ((1 << low) - 1);
            kassert(!(val & ~mask));
            config_write_32((config_read_32(offset) & ~mask) | val,offset);
        }

        inline cap_list_adapter cap_list() {return cap_list_adapter(this);}
        uint8_t find_pci_capability(uint8_t cap_id);

        // Map [offset, offset+len) of BARi.  Typically you will map the whole
        // BAR, but for some devices (virtio devices) and for MSI-X tables it
        // is also convenient to map just a subset of the BAR.
        void* map_bar(uint8_t bari, size_t len, size_t offset = 0);

        void map_msix_table();
        kernel::wqe* alloc_msix_vector(size_t vec,
                                       kernel::work_handler handler);
        void enable_msix_vector(size_t vec);
        void dump_msix_table();

        dev(pci::domain* domain, uint8_t bus, uint8_t devfn):
            domain(domain),
            bus(bus),
            devfn(devfn),
            owner(NULL),
            msix_nvecs(0),
            msix_table(NULL)
        {
        }

        dev(const dev* pd, const pci::driver* owner):
            domain(pd->domain),
            bus(pd->bus),
            devfn(pd->devfn),
            owner(owner),
            msix_nvecs(pd->msix_nvecs),
            msix_table(pd->msix_table)
        {
        }

        virtual ~dev() {}
    };

    extern obj_list<pci::domain> domains;
    extern wapi::node wapi_node;

    void init_pci();
}

#endif /* __KERNEL_PCI_H */
