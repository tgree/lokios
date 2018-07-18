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

static const eth::link_mode link_mode_table[] = {
    {0,     0},
    {10,    0},
    {10,    PHY_LM_DUPLEX_FULL},
    {100,   0},
    {100,   0},
    {100,   PHY_LM_DUPLEX_FULL},
    {1000,  0},
    {1000,  PHY_LM_DUPLEX_FULL},
};

struct bcm57765 : public eth::phy
{
    virtual const eth::link_mode get_link_mode()
    {
        uint32_t aux_status = phy_read_16(0x19);
        if (!(aux_status & (1<<2)))
            return eth::link_mode{0,0};

        eth::link_mode lm = link_mode_table[(aux_status >> 8) & 7];
        lm.flags |= PHY_LM_LINK_UP;
        return lm;
    }

    bcm57765(eth::interface* intf, eth::phy_driver* owner):
        eth::phy(intf,owner)
    {
    }
};

static uint32_t ids[] = {0x5C0D8A40};
eth::phy_id_matcher_driver<bcm57765> bcm57765_driver("BCM57765",ids);
