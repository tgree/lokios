#include "schedule.h"
#include "cpu.h"
#include "thread.h"
#include "task.h"
#include "console.h"
#include "pmtimer.h"
#include "mm/slab.h"
#include "interrupts/lapic.h"

static kernel::spinlock wqe_slab_lock;
static kernel::slab wqe_slab(sizeof(kernel::work_entry));

kernel::work_entry*
kernel::alloc_wqe()
{
    with (wqe_slab_lock)
    {
        return wqe_slab.alloc<kernel::work_entry>();
    }
}

void
kernel::free_wqe(work_entry* wqe)
{
    with (wqe_slab_lock)
    {
        wqe_slab.free(wqe);
    }
}

void
kernel::schedule_wqe(cpu* c, work_entry* wqe)
{
    if (c == get_current_cpu())
    {
        c->work_queue.push_back(&wqe->link);
        return;
    }

    with (c->work_queue_lock)
    {
        c->work_queue.push_back(&wqe->link);
    }
    kernel::send_schedule_wakeup_ipi(c->apic_id);
}

void
kernel::schedule_loop()
{
    cpu* c = get_current_cpu();
    for (;;)
    {
        asm ("cli;");

        while (c->work_queue.empty())
            asm ("sti; hlt; cli;");

        klist<work_entry> wq;
        with (c->work_queue_lock)
        {
            wq.append(c->work_queue);
        }

        asm ("sti;");

        while (!wq.empty())
        {
            work_entry* wqe = klist_front(wq,link);
            wq.pop_front();
            wqe->fn(wqe);
        }
    }
}
