#ifndef __KERNEL_MOCK_FSCHEDULE_H
#define __KERNEL_MOCK_FSCHEDULE_H

#include "../schedule.h"

namespace kernel
{
    void fire_timer(timer_entry* wqe);
    void fire_work(kernel::wqe* wqe);
}

#endif /* __KERNEL_MOCK_FSCHEDULE_H */
