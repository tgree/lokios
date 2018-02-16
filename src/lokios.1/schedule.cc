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

kernel::scheduler::scheduler(uint64_t tbase):
    tbase(tbase),
    current_slot(0)
{
    wheel = new scheduler_table;
}

kernel::scheduler::~scheduler()
{
    delete wheel;
}

void
kernel::scheduler::schedule_local_work(work_entry* wqe)
{
    // The currently-executing CPU is the local CPU.
    kassert(get_current_cpu() == container_of(this,cpu,scheduler));
    local_work.push_back(&wqe->link);
}

void
kernel::scheduler::schedule_remote_work(work_entry* wqe)
{
    // The currently-executing CPU is a remote CPU trying to schedule work on
    // us.
    with (remote_work_lock)
    {
        remote_work.push_back(&wqe->link);
    }
    kernel::send_schedule_wakeup_ipi(container_of(this,cpu,scheduler)->apic_id);
}

void
kernel::scheduler::schedule_deferred_local_work(timer_entry* wqe, uint64_t dt)
{
    // The currently-executing CPU is the local CPU and in non-interrupt
    // context.
    kassert(get_current_cpu() == container_of(this,cpu,scheduler));
    kassert(dt != 0);
    wqe->texpiry = tbase + current_slot + dt + 1;
    if (dt < kernel::nelems(wheel->slots) - 1)
    {
        size_t slot = wqe->texpiry % kernel::nelems(wheel->slots);
        wheel->slots[slot].push_back(&wqe->link);
        wqe->pos = -1;
    }
    else
        overflow_heap.insert(wqe);
}

void
kernel::scheduler::workloop()
{
    cpu* c = get_current_cpu();
    kassert(container_of(this,cpu,scheduler) == c);

    for (;;)
    {
        // This list of work we are going to do this iteration.
        klist<work_entry> wq;
        kdlist<timer_entry> tq;

        // Disable interrupts, check for work and then halt if there is no work
        // to do presently.
        asm ("cli;");
        while (remote_work.empty() &&
               local_work.empty() &&
               c->jiffies == tbase + current_slot)
        {
            // sti; hlt; is an atomic sequence even though it is two
            // instructions.  We enable interrupts while the CPU is halted; an
            // interrupt is what's going to cause the hlt instruction to
            // complete after which point we will re-disable interrupts and
            // check for work.
            asm ("sti; hlt; cli;");
        }

        // Build the list of work we are going to do.  The local_work queue can
        // be modified by a device interrupt on the local CPU, so we need to
        // move it before we reenable interrupts.
        wq.append(local_work);
        asm ("sti;");

        // Compute the new current slot and append all queues up to and
        // including the new current slot.
        while (c->jiffies != tbase + current_slot)
        {
            if (++current_slot == kernel::nelems(wheel->slots))
            {
                current_slot = 0;
                tbase       += kernel::nelems(wheel->slots);
            }
            tq.append(wheel->slots[current_slot]);
        }

        // Pop any elements from the heap that have expired.
        while (!overflow_heap.empty() &&
               c->jiffies >= overflow_heap.front()->texpiry)
        {
            tq.push_back(&overflow_heap.front()->link);
            overflow_heap.pop_front();
        }

        // If there is work to do on the remote queue, add it to the list.
        if (!remote_work.empty())
        {
            with (remote_work_lock)
            {
                wq.append(remote_work);
            }
        }

        // Process all work elements.
        while (!wq.empty())
        {
            work_entry* wqe = klist_front(wq,link);
            wq.pop_front();
            wqe->fn(wqe);
        }

        // Process all timers.
        while (!tq.empty())
        {
            timer_entry* wqe = klist_front(tq,link);
            tq.pop_front();
            wqe->fn(wqe);
        }
    }
}
