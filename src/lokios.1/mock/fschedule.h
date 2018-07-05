#ifndef __KERNEL_MOCK_FSCHEDULE_H
#define __KERNEL_MOCK_FSCHEDULE_H

#include "../schedule.h"

namespace kernel
{
    void fire_timer(timer_entry* wqe);
    void fire_work(work_entry* wqe);
}

#endif /* __KERNEL_MOCK_FSCHEDULE_H */
