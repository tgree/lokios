#include "fschedule.h"
#include "../cpu.h"
#include <tmock/tmock.h>

kernel::cpu*
kernel::get_current_cpu()
{
    return NULL;
}

void
kernel::fire_timer(timer_entry* wqe)
{
    kassert(wqe->is_armed());
    wqe->pos = -1;
    kassert(!wqe->is_armed());
    wqe->fn(wqe);
}

void
kernel::fire_work(work_entry* wqe)
{
    wqe->fn(wqe);
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
}

void
kernel::scheduler::schedule_remote_work(work_entry* wqe)
{
}

void
kernel::scheduler::schedule_timer(timer_entry* wqe, uint64_t dt)
{
    kassert(!wqe->is_armed());
    wqe->pos = 0;
    kassert(wqe->is_armed());
}

void
kernel::scheduler::cancel_timer(timer_entry* wqe)
{
    kassert(wqe->is_armed());
    wqe->pos = -1;
    kassert(!wqe->is_armed());
}
