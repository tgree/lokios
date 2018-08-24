#ifndef __KERNEL_EARLY_TASK_H
#define __KERNEL_EARLY_TASK_H

namespace kernel
{
    void init_early_task_bsp();
    void init_early_task_ap();
    void early_task_release_tss();
}

#endif /* __KERNEL_EARLY_TASK_H */
