#include "net/net.h"
#include "net/dhcp/dhcpc.h"
#include "interface.h"

struct _eth_observer : public net::observer
{
    virtual void intf_activated(net::interface* intf) override
    {
    }

    virtual void intf_link_up(net::interface* intf) override
    {
        auto* eintf = dynamic_cast<eth::interface*>(intf);
        if (!eintf)
            return;

        // Post a dhcp request.
        // TODO: Wait a random time from 0-10 seconds before starting dhcpc.
        eintf->dhcpc->start();
    }

    virtual void intf_link_down(net::interface* intf) override
    {
    }

    virtual void intf_deactivated(net::interface* intf) override
    {
    }
};

_eth_observer eth_observer;
