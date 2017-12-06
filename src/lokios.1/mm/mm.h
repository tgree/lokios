#ifndef __KERNEL_MM_H
#define __KERNEL_MM_H

#include "e820.h"

namespace kernel
{
    void init_mm(const e820_map* m);
}

#endif /* __KERNEL_MM_H */
