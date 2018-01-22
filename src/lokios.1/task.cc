#include "task.h"

extern "C" void _thread_jump(kernel::thread* t) __attribute__((noreturn));

kernel::task* kernel::kernel_task;

void
kernel::task::spawn_thread(void (*entry_fn)())
{
    // TODO: Find a free thread id!
    thread* t = new((thread_id)1,this) thread(entry_fn);
    threads.push_back(&t->tcb.link);
}

void
kernel::task::spawn_and_jump_into_thread(void (*entry_fn)())
{
    kassert(threads.empty());
    spawn_thread(entry_fn);
    _thread_jump(klist_front(threads,tcb.link));
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
