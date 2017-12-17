#ifndef __KERNEL_PMTIMER_H
#define __KERNEL_PMTIMER_H

#include <stdint.h>

namespace kernel::pmtimer
{
    void wait_us(uint64_t us);
    void init();
}

#endif /* __KERNEL_PMTIMER_H */
