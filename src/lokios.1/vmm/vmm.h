#ifndef __KERNEL_VMM_VMM_H
#define __KERNEL_VMM_VMM_H

#include "wapi/wapi.h"

namespace vmm
{
    extern wapi::global_node wapi_node;

    void init_vmm();
}

#endif /* __KERNEL_VMM_VMM_H */
