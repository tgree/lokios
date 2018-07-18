#include "phy.h"

// Flags set in the tg3 driver:
//  57765_PLUS
//  5755_PLUS
//  5750_PLUS
//  5705_PLUS
//  PCI_EXPRESS
//  CPMU_PRESENT
//  TAGGED_STATUS
//
// mi_mode
//  MAC_MI_MODE_500KHZ_CONST
//
// coalesce_mode
//  HOSTCC_MODE_CLRTICK_RXBD
//  HOSTCC_MODE_CLRTICK_TXBD

using kernel::_kassert;

struct link_mode
{
    uint32_t    speed;
    uint32_t    flags;
};

static const link_mode link_mode_table[] = {
    {0,     0},
    {10,    PHY_LM_LINK_UP},
    {10,    PHY_LM_LINK_UP | PHY_LM_DUPLEX_FULL},
    {100,   PHY_LM_LINK_UP},
    {100,   PHY_LM_LINK_UP},
    {100,   PHY_LM_LINK_UP | PHY_LM_DUPLEX_FULL},
    {1000,  PHY_LM_LINK_UP},
    {1000,  PHY_LM_LINK_UP | PHY_LM_DUPLEX_FULL},
};

struct bcm57765 : public eth::phy
{
    kernel::work_entry  lm_wqe;
    kernel::work_entry* lm_cqe;

    virtual void issue_get_link_mode(kernel::work_entry* cqe)
    {
        lm_cqe = cqe;
        intf->issue_phy_read_16(0x19,&lm_wqe);
    }

    void handle_phy_completion(kernel::work_entry* wqe)
    {
        lm_cqe->args[1] = wqe->args[1];
        if (!lm_cqe->args[1])
        {
            uint32_t aux_status = wqe->args[2];
            const link_mode* lm;
            if (!(aux_status & (1<<2)))
                lm = &link_mode_table[0];
            else
                lm = &link_mode_table[(aux_status >> 8) & 7];

            lm_cqe->args[2] = lm->speed;
            lm_cqe->args[3] = lm->flags;
        }
        lm_cqe->fn(lm_cqe);
    }

    bcm57765(eth::interface* intf, eth::phy_driver* owner):
        eth::phy(intf,owner)
    {
        lm_wqe.fn      = work_delegate(handle_phy_completion);
        lm_wqe.args[0] = (uintptr_t)this;
    }
};

static uint32_t ids[] = {0x5C0D8A40};
eth::phy_id_matcher_driver<bcm57765> bcm57765_driver("BCM57765",ids);
