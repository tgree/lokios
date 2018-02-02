#ifndef __KERNEL_PCI_H
#define __KERNEL_PCI_H

#include "domain.h"
#include "k++/vector.h"

namespace kernel::pci
{
    struct dev;

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
        klink               link;
        pci::domain*        domain;
        const uint8_t       bus;
        const uint8_t       devfn;
        const pci::driver*  owner;

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

        dev(pci::domain* domain, uint8_t bus, uint8_t devfn):
            domain(domain),
            bus(bus),
            devfn(devfn),
            owner(NULL)
        {
        }
        virtual ~dev() {}

    protected:
        dev(const dev* pd, const pci::driver* owner):
            domain(pd->domain),
            bus(pd->bus),
            devfn(pd->devfn),
            owner(owner)
        {
        }
    };

    extern vector<pci::domain> domains;

    void init_pci();
}

#endif /* __KERNEL_PCI_H */
