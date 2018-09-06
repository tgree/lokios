#ifndef __KERNEL_NET_ETH_MOCK_FINTERFACE_H
#define __KERNEL_NET_ETH_MOCK_FINTERFACE_H

#include "../interface.h"
#include <tmock/tmock.h>

namespace eth
{
    struct finterface : public interface
    {
        virtual void issue_phy_read_16(uint8_t offset, kernel::wqe* cqe)
        {
            mock("eth::interface::issue_phy_read_16",offset,cqe);
        }

        virtual void issue_phy_write_16(uint16_t v, uint8_t offset,
                                        kernel::wqe* cqe)
        {
            mock("eth::interface::issue_phy_write_16",v,offset,cqe);
        }

        virtual void post_tx_frame(net::tx_op* op)
        {
            mock("eth::interface::post_tx_frame",op);
        }

        virtual void post_rx_pages(kernel::klist<net::rx_page>& pages)
        {
            mock("eth::interface::post_rx_pages",&pages);
        }

        virtual void handle_wapi_request(wapi::node* node, http::request* req,
                                         json::object* obj, http::response* rsp)
        {
            mock("eth::interface::handle_wapi_request",node,req,obj,rsp);
        }

        finterface(const eth::addr& hw_mac):interface(hw_mac,10,10,1500,1500) {}
    };
}

#endif /* __KERNEL_NET_ETH_MOCK_FINTERFACE_H */
