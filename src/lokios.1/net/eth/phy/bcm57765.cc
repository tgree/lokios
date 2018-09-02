#include "phy.h"
#include "net/eth/interface.h"

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

struct linkmode_getter : public eth::phy_state_machine
{
    enum
    {
        WAIT_READ_AUX_STAT,
        WAIT_READ_EXT_CTRL,
        WAIT_READ_ADVERTISE,
        WAIT_READ_LPA,
    } state;

    void start(kernel::wqe* wqe)
    {
        kassert(!cqe);
        cqe = wqe;
        intf->issue_phy_read_16(0x19,&phy_wqe);
        state = WAIT_READ_AUX_STAT;
    }

    virtual void handle_phy_success(kernel::wqe* wqe)
    {
        const link_mode* lm;

        switch (state)
        {
            case WAIT_READ_AUX_STAT:
                if (!(wqe->args[2] & (1<<2)))
                    lm = &link_mode_table[0];
                else
                    lm = &link_mode_table[(wqe->args[2] >> 8) & 7];

                cqe->args[2] = lm->speed;
                cqe->args[3] = lm->flags;
                intf->issue_phy_read_16(0x10,&phy_wqe);
                state = WAIT_READ_EXT_CTRL;
            break;

            case WAIT_READ_EXT_CTRL:
                cqe->args[4] = (wqe->args[2] << 16);
                intf->issue_phy_read_16(4,&phy_wqe);
                state = WAIT_READ_ADVERTISE;
            break;

            case WAIT_READ_ADVERTISE:
                cqe->args[4] |= wqe->args[2];
                intf->issue_phy_read_16(5,&phy_wqe);
                state = WAIT_READ_LPA;
            break;

            case WAIT_READ_LPA:
                cqe->args[5] = wqe->args[2];
                complete();
            break;
        }
    }

    linkmode_getter(eth::interface* intf):
        phy_state_machine(intf,NULL)
    {
    }
};

struct bcm57765 : public eth::phy
{
    linkmode_getter     lmg;

    virtual void issue_get_link_mode(kernel::wqe* cqe)
    {
        lmg.start(cqe);
    }

    bcm57765(eth::interface* intf, eth::phy_driver* owner):
        eth::phy(intf,owner),
        lmg(intf)
    {
    }
};

static uint32_t ids[] = {0x5C0D8A40};
eth::phy_id_matcher_driver<bcm57765> bcm57765_driver("BCM57765",ids);
