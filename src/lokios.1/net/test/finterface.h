#ifndef __NET_TEST_FINTERFACE_H
#define __NET_TEST_FINTERFACE_H

#include "../eth.h"
#include <tmock/tmock.h>

namespace eth
{
    struct finterface : public interface
    {
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
