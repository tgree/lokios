#include "../schedule.h"
#include "../cpu.h"
#include <tmock/tmock.h>

kernel::cpu*
kernel::get_current_cpu()
{
    return (kernel::cpu*)mock("kernel::get_current_cpu");
}

kernel::work_entry*
kernel::alloc_wqe()
{
    return (kernel::work_entry*)mock("kernel::alloc_wqe");
}

void
kernel::free_wqe(work_entry* wqe)
{
    mock("kernel::free_wqe",wqe);
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
    mock("kernel::scheduler::schedule_local_work",wqe);
}

void
kernel::scheduler::schedule_remote_work(work_entry* wqe)
{
    mock("kernel::scheduler::schedule_remote_work",wqe);
}

void
kernel::scheduler::schedule_timer(timer_entry* wqe, uint64_t dt)
{
    mock("kernel::scheduler::schedule_timer",wqe,dt);
}

void
kernel::scheduler::cancel_timer(timer_entry* wqe)
{
    mock("kernel::scheduler::cancel_timer",wqe);
}

void
kernel::scheduler::workloop()
{
    mock("kernel::scheduler::workloop");
}
