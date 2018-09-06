#include "net.h"
#include "interface.h"

kernel::kdlist<net::interface> net::interfaces;

void
net::register_interface(net::interface* intf)
{
    net::interfaces.push_back(&intf->link);
}

void
net::deregister_interface(net::interface* intf)
{
    intf->link.unlink();
}
