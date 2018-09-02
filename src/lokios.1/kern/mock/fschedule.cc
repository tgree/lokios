#include "fschedule.h"
#include "../cpu.h"
#include <tmock/tmock.h>

kernel::cpu*
kernel::get_current_cpu()
{
    return NULL;
}

void
kernel::fire_timer(kernel::tqe* wqe)
{
    kassert(wqe->is_armed());
    wqe->pos = -1;
    kassert(!wqe->is_armed());
    wqe->fn(wqe);
}

void
kernel::fire_work(kernel::wqe* wqe)
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
kernel::scheduler::schedule_local_work(kernel::wqe* wqe)
{
}

void
kernel::scheduler::schedule_remote_work(kernel::wqe* wqe)
{
}

void
kernel::scheduler::schedule_timer(kernel::tqe* wqe, uint64_t dt)
{
    kassert(!wqe->is_armed());
    wqe->pos = 0;
    kassert(wqe->is_armed());
}

void
kernel::scheduler::cancel_timer(kernel::tqe* wqe)
{
    kassert(wqe->is_armed());
    wqe->pos = -1;
    kassert(!wqe->is_armed());
}
