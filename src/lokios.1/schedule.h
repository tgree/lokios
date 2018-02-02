#ifndef __KERNEL_SCHEDULE_H
#define __KERNEL_SCHEDULE_H

#include "spinlock.h"
#include "k++/klist.h"
#include "mm/page.h"

namespace kernel
{
    struct tls_tcb;
    struct cpu;

    struct work_entry
    {
        klink       link;
        void       (*fn)(work_entry* wqe);
        uint64_t    args[6];
    };
    KASSERT(sizeof(work_entry) == 64);

    struct scheduler_table
    {
        klist<work_entry>   slots[256];
    };
    KASSERT(sizeof(scheduler_table) == PAGE_SIZE);

    struct scheduler
    {
        spinlock            remote_work_lock;
        klist<work_entry>   remote_work;

        klist<work_entry>   local_work __CACHE_ALIGNED__;

        uint64_t            tbase;
        size_t              current_slot;
        scheduler_table*    wheel;

        void schedule_local_work(work_entry* wqe);
        void schedule_remote_work(work_entry* wqe);
        void schedule_deferred_local_work(work_entry* wqe, uint64_t dt);

        void workloop();

        scheduler(uint64_t tbase = 0);
        ~scheduler();
    };

    // These will allocate/free WQEs off a locked, shared internal slab.  Use
    // of these is optional, you can define WQE objects anywhere.
    work_entry* alloc_wqe();
    void free_wqe(work_entry* wqe);
}

#endif /* __KERNEL_SCHEDULE_H */
