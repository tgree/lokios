#ifndef __NET_TEST_FINTERFACE_H
#define __NET_TEST_FINTERFACE_H

#include "../eth/eth.h"
#include <tmock/tmock.h>

namespace eth
{
    struct finterface : public interface
    {
        virtual void issue_phy_read_16(uint8_t offset, kernel::work_entry* cqe)
        {
            mock("eth::interface::issue_phy_read_16",offset,cqe);
        }

        virtual void issue_phy_write_16(uint16_t v, uint8_t offset,
                                        kernel::work_entry* cqe)
        {
            mock("eth::interface::issue_phy_write_16",v,offset,cqe);
        }

        virtual void post_tx_frame(eth::tx_op* op)
        {
            mock("eth::interface::post_tx_frame",op);
        }

        virtual void post_rx_pages(kernel::klist<eth::rx_page>& pages)
        {
            mock("eth::interface::post_rx_pages",&pages);
        }

        finterface(const eth::addr& hw_mac):interface(hw_mac,10,10) {}
    };
}

#endif /* __NET_TEST_FINTERFACE_H */
