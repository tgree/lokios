#include "task.h"

extern "C" void _thread_jump(kernel::thread* t) __attribute__((noreturn));

static kernel::task* kernel_task;

void
kernel::init_kernel_task(void (*entry_fn)())
{
    // Create a task object.
    kernel_task = new task;

    // Clone the bootloader memory map.
    for (auto e : kernel::page_table_leaf_iterator((uint64_t*)mfcr3()))
        kernel_task->pt.map_page(e.vaddr,e.get_paddr(),e.get_len(),e.pte);
    kernel_task->pt.activate();

    // Create the thread for this task and jump into it.
    thread* t = new((thread_id)0,kernel_task) thread(entry_fn);
    kernel_task->threads.push_back(&t->tcb.link);
    _thread_jump(t);
}
