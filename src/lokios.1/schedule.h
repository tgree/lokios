#ifndef __KERNEL_SCHEDULE_H
#define __KERNEL_SCHEDULE_H

#include "spinlock.h"
#include "k++/klist.h"
#include "mm/page.h"
#include <type_traits>

namespace kernel
{
    struct tls_tcb;
    struct cpu;
    struct work_entry;

    typedef void (*work_handler)(work_entry* wqe);

    struct work_entry
    {
        klink           link;
        work_handler    fn;
        uint64_t        args[6];
    };
    KASSERT(sizeof(work_entry) == 64);
    KASSERT(offsetof(work_entry,args) == 16);

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
        void schedule_deferred_local_work(work_entry* wqe, uint64_t dt10ms);

        void workloop();

        scheduler(uint64_t tbase = 0);
        ~scheduler();
    };
    KASSERT(offsetof(scheduler,local_work) == 192);
    KASSERT(offsetof(scheduler,local_work.head) == 192);
    KASSERT(offsetof(scheduler,local_work.tail) == 200);

    // Delegation of work to a member function.  For this to work you must
    // populate wqe->args[0] with a T*.
    template<typename T, void (T::*Handler)()>
    struct _work_delegate
    {
        static void handler(kernel::work_entry* wqe)
        {
            (((T*)wqe->args[0])->*Handler)();
        }
    };
#define work_delegate(fn) \
    kernel::_work_delegate< \
        std::remove_reference_t<decltype(*this)>, \
        &std::remove_reference_t<decltype(*this)>::fn>::handler

    // These will allocate/free WQEs off a locked, shared internal slab.  Use
    // of these is optional, you can define WQE objects anywhere.
    work_entry* alloc_wqe();
    void free_wqe(work_entry* wqe);
}

#endif /* __KERNEL_SCHEDULE_H */
