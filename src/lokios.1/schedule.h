#ifndef __KERNEL_SCHEDULE_H
#define __KERNEL_SCHEDULE_H

#include "k++/klist.h"

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

    work_entry* alloc_wqe();
    void free_wqe(work_entry* wqe);
    void schedule_wqe(cpu* c, work_entry* wqe);
    void schedule_loop();
}

#endif /* __KERNEL_SCHEDULE_H */
