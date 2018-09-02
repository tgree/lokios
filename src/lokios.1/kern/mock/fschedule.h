#ifndef __KERNEL_MOCK_FSCHEDULE_H
#define __KERNEL_MOCK_FSCHEDULE_H

#include "../schedule.h"

namespace kernel
{
    void fire_timer(kernel::tqe* wqe);
    void fire_work(kernel::wqe* wqe);
}

#endif /* __KERNEL_MOCK_FSCHEDULE_H */
