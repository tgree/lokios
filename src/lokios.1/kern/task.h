#ifndef __KERNEL_TASK_H
#define __KERNEL_TASK_H

#include "thread.h"
#include "spinlock.h"
#include "mm/page_table.h"

namespace kernel
{
    struct task
    {
        page_table      pt;

        spinlock        threads_lock;
        kdlist<thread>  runnable_threads;
        kdlist<thread>  running_threads;

        void spawn_thread(void (*entry_fn)());
    };

    extern task* kernel_task;

    void init_kernel_task();
}

#endif /* __KERNEL_TASK_H */
