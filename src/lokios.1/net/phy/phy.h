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

        static void       issue_probe(interface* intf, kernel::work_entry* cqe);

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

    // Flags for get_link_mode.
#define PHY_LM_LINK_UP      0x00000001
#define PHY_LM_DUPLEX_FULL  0x00000002

    // Ethernet PHY.
    struct phy
    {
        kernel::klink           link;
        eth::interface* const   intf;
        phy_driver* const       owner;

        // PHY methods.
        //  All method return an error code in args[1].
        //  get_link_mode returns speed in args[2] and flags in args[3].
                void issue_reset(kernel::work_entry* wqe);
                void issue_start_autonegotiation(kernel::work_entry* wqe);
        virtual void issue_get_link_mode(kernel::work_entry* wqe) = 0;

        phy(eth::interface* intf, phy_driver* owner);
        virtual ~phy();
    };

    // Helper base class for PHY drivers that need to perform transactions.
    struct phy_state_machine
    {
        eth::interface*     intf;
        kernel::work_entry  phy_wqe;
        kernel::work_entry* cqe;

        void complete()
        {
            cqe->fn(cqe);
            delete this;
        }

        void handle_phy_completion(kernel::work_entry* wqe)
        {
            cqe->args[1] = wqe->args[1];
            if (wqe->args[1])
                complete();
            else
                handle_phy_success(wqe);
        }

        virtual void handle_phy_success(kernel::work_entry* wqe) = 0;

        phy_state_machine(eth::interface* intf, kernel::work_entry* cqe):
            intf(intf),
            cqe(cqe)
        {
            phy_wqe.fn      = work_delegate(handle_phy_completion);
            phy_wqe.args[0] = (uintptr_t)this;
        }
        virtual ~phy_state_machine()
        {
        }
    };
}

#endif /* __KERNEL_NET_PHY_H */
