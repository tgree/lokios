#include "schedule.h"
#include "cpu.h"
#include "thread.h"
#include "task.h"
#include "console.h"

void
kernel::schedule_loop()
{
    for (;;)
        asm ("hlt;");
}

kernel::tls_tcb*
kernel::schedule_tick()
{
    cpu* c    = get_current_cpu();
    thread* t = get_current_thread();

    with (kernel_task->threads_lock)
    {
        if (t != c->schedule_thread)
        {
            t->tcb.link.unlink();
            kernel_task->runnable_threads.push_back(&t->tcb.link);
        }

        if (kernel_task->runnable_threads.empty())
            t = c->schedule_thread;
        else
        {
            t = klist_front(kernel_task->runnable_threads,tcb.link);
            t->tcb.link.unlink();
            kernel_task->running_threads.push_back(&t->tcb.link);
        }
    }

    return &t->tcb;
}
