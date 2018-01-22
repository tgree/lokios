#ifndef __KERNEL_TASK_H
#define __KERNEL_TASK_H

#include "thread.h"
#include "spinlock.h"

namespace kernel
{
    struct task
    {
        page_table      pt;

        spinlock        threads_lock;
        klist<thread>   threads;

        void spawn_thread(void (*entry_fn)());
        void spawn_and_jump_into_thread(void (*entry_fn)());
    };

    extern task* kernel_task;

    void init_kernel_task();
}

#endif /* __KERNEL_TASK_H */
