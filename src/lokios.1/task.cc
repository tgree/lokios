#include "task.h"

kernel::task* kernel::kernel_task;

void
kernel::task::spawn_thread(void (*entry_fn)())
{
    with (threads_lock)
    {
        thread* t = new thread(entry_fn);
        runnable_threads.push_back(&t->tcb.link);
    }
}

void
kernel::init_kernel_task()
{
    // Create a task object.
    kernel_task = new task;

    // Clone the bootloader memory map.
    for (auto e : kernel::page_table_leaf_iterator((uint64_t*)mfcr3()))
        kernel_task->pt.map_page(e.vaddr,e.get_paddr(),e.get_len(),e.pte);
    kernel_task->pt.activate();
}
