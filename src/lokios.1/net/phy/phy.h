#ifndef __KERNEL_NET_PHY_PHY_H
#define __KERNEL_NET_PHY_PHY_H

#include "../eth.h"

namespace eth
{
    struct phy;

    // Ethernet PHY driver.  When we talk about phy_ids here, we mean the
    // concatenation:
    //
    //  OUI[24:3] | Model[5:0] | Revision[3:0]
    //
    // which isn't the same order as the PHY ID 1 and 2 registers.
    // Driver types that can recognize and instantiate PHY subclasses.
    struct phy_driver
    {
        kernel::klink           link;
        const char* const       name;
        kernel::klist<eth::phy> phys;

        static eth::phy*  probe(interface* intf);

        virtual uint64_t  score(interface* intf, uint32_t phy_id) = 0;
        virtual eth::phy* alloc(interface* intf, uint32_t phy_id) = 0;
        virtual void      release(eth::phy* p) = 0;

        phy_driver(const char* name);
    };

    template<typename Phy>
    struct phy_id_matcher_driver : public phy_driver
    {
        const uint32_t  id_mask;
        const size_t    nids;
        const uint32_t* ids;

        virtual uint64_t score(eth::interface* intf, uint32_t phy_id)
        {
            for (size_t i=0; i<nids; ++i)
            {
                if (ids[i] == (phy_id & id_mask))
                    return 100;
            }
            return 0;
        }

        virtual eth::phy* alloc(eth::interface* intf, uint32_t phy_id)
        {
            return new Phy(intf,this);
        }

        virtual void release(eth::phy* p)
        {
            delete p;
        }

        template<typename T, size_t N>
        phy_id_matcher_driver(const char* name, T (&ids)[N],
                              uint32_t id_mask = 0xFFFFFFFF):
            phy_driver(name),
            id_mask(id_mask),
            nids(N),
            ids(ids)
        {
        }
    };

#define PHY_LM_LINK_UP      0x00000001
#define PHY_LM_DUPLEX_FULL  0x00000002
    struct link_mode
    {
        uint32_t    speed;
        uint32_t    flags;
    };

    // Ethernet PHY.
    struct phy
    {
        kernel::klink           link;
        eth::interface* const   intf;
        phy_driver* const       owner;

        // Register helpers.
        inline uint16_t phy_read_16(uint8_t o)
            {return intf->phy_read_16(o);}
        inline void phy_write_16(uint16_t v, uint8_t o)
            {intf->phy_write_16(v,0);}
        inline void phy_set_clear_16(uint16_t s, uint16_t c, uint8_t offset)
        {
            kernel::kassert(!(s & c));
            phy_write_16((phy_read_16(offset) | s) & ~c,offset);
        }
        inline void phy_set_16(uint16_t s, uint8_t offset)
        {
            phy_set_clear_16(s,0,offset);
        }
        inline void phy_clear_16(uint16_t c, uint8_t offset)
        {
            phy_set_clear_16(0,c,offset);
        }
        inline void phy_insert_32(uint16_t val, uint8_t low, uint8_t high,
                                  uint8_t offset)
        {
            kernel::kassert(low <= high);
            kernel::kassert(high < 16);
            val <<= low;
            uint16_t mask = ((1 << (high+1)) - 1) ^ ((1 << low) - 1);
            kernel::kassert(!(val & ~mask));
            phy_write_16((phy_read_16(offset) & ~mask) | val,offset);
        }

        // Required methods.
        virtual const link_mode get_link_mode() = 0;

        // PHY methods.
        void    dump_regs();
        void    reset();
        void    start_autonegotiation();

        phy(eth::interface* intf, phy_driver* owner);
        virtual ~phy();
    };
}

#endif /* __KERNEL_NET_PHY_H */
