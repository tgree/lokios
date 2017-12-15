#ifndef __KERNEL_PCI_H
#define __KERNEL_PCI_H

#include "domain.h"
#include "k++/vector.h"

namespace kernel::pci
{
    extern vector<pci::domain> domains;

    void init_pci();
}

#endif /* __KERNEL_PCI_H */
