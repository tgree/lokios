#ifndef __KERNEL_TIME_H
#define __KERNEL_TIME_H

#include "cpu.h"

namespace kernel
{
#ifndef BUILDING_UNITTEST
    static inline uint64_t get_jiffies()
    {
        if (cpus.size())
            return cpus[0]->jiffies;
        return 0;
    }
#else
    uint64_t get_jiffies();
#endif
}

#endif /* __KERNEL_TIME_H */
