#ifndef __KERNEL_PCI_WAPI_H
#define __KERNEL_PCI_WAPI_H

#include "wapi/string_responder.h"

namespace kernel::pci
{
    struct domain;

    extern wapi::string_responder_node wapi_node;

    void wapi_register(kernel::pci::domain* d);
    void wapi_register();
}

#endif /* __KERNEL_PCI_WAPI_H */
