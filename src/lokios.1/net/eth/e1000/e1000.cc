/*
 * Driver for the Intel 8254x family of Ethernet devices (also known as e1000).
 * Reference:
 *  //www.intel.com/content/dam/doc/manual/pci-pci-x-family-gbe-controllers-software-dev-manual.pdf
 */
#include "e1000.h"

e1000::dev::dev(const kernel::pci::dev* pd, const e1000::driver* owner):
    kernel::pci::dev(pd,owner)
{
}
