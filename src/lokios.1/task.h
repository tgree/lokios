#ifndef __KERNEL_TASK_H
#define __KERNEL_TASK_H

#include "thread.h"

namespace kernel
{
    struct task
    {
        page_table      pt;
        klist<thread>   threads;
    };

    void init_kernel_task(void (*entry_fn)());
}

#endif /* __KERNEL_TASK_H */
