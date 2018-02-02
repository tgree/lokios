#include "e1000.h"

e1000::dev::dev(const kernel::pci::dev* pd, const e1000::driver* owner):
    kernel::pci::dev(pd,owner)
{
}
