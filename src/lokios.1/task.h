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
}

#endif /* __KERNEL_TASK_H */
