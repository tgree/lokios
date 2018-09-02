#ifndef __KERNEL_SCHEDULE_H
#define __KERNEL_SCHEDULE_H

#include "spinlock.h"
#include "k++/klist.h"
#include "k++/heap.h"
#include "k++/vector.h"
#include "mm/page.h"

namespace kernel
{
    struct tls_tcb;
    struct cpu;
    struct wqe;
    struct tqe;

    typedef void (*work_handler)(kernel::wqe* wqe);

    struct wqe
    {
        klink           link;
        work_handler    fn;
        uint64_t        args[6];

        inline bool is_armed() const
        {
            return link.nextu != KLINK_NOT_IN_USE;
        }

        wqe() = default;
        wqe(work_handler fn, uint64_t arg0):fn(fn),args{arg0} {}
    };
    KASSERT(sizeof(wqe) == 64);
    KASSERT(offsetof(wqe,args) == 16);

    typedef void (*timer_handler)(kernel::tqe* wqe);

    struct tqe
    {
        kdlink          link;
        timer_handler   fn;
        uint64_t        texpiry;
        size_t          pos = -1;
        uint64_t        args[3];

        inline bool is_armed() const
        {
            return link.nextu != KLINK_NOT_IN_USE || pos != (size_t)-1;
        }

        tqe() = default;
        tqe(timer_handler fn, uint64_t arg0):fn(fn),args{arg0} {}
    };
    KASSERT(sizeof(tqe) == 64);

    struct tqe_less
    {
        constexpr bool operator()(const tqe* lhs, const tqe* rhs) const
        {
            return lhs->texpiry < rhs->texpiry;
        }
    };

    struct scheduler_table
    {
        kdlist<tqe> slots[256];
    };
    KASSERT(sizeof(scheduler_table) == PAGE_SIZE);

    struct scheduler
    {
        spinlock                        remote_work_lock;
        klist<wqe>                      remote_work;

        klist<wqe>                      local_work __CACHE_ALIGNED__;

        uint64_t                        tbase;
        size_t                          current_slot;
        scheduler_table*                wheel;
        heap<vector<tqe*>, tqe_less>    overflow_heap;

        void schedule_local_work(kernel::wqe* wqe);
        void schedule_remote_work(kernel::wqe* wqe);
        void schedule_timer(kernel::tqe* wqe, uint64_t dt10ms);
        void schedule_timer_ms(kernel::tqe* wqe, uint64_t dtms)
        {
            schedule_timer(wqe,(dtms+9)/10);
        }
        void schedule_timer_sec(kernel::tqe* wqe, uint64_t secs)
        {
            schedule_timer(wqe,secs*100);
        }
        void cancel_timer(kernel::tqe* wqe);

        void workloop();

        scheduler(uint64_t tbase = 0);
        ~scheduler();
    };
    KASSERT(offsetof(scheduler,local_work) == 192);
    KASSERT(offsetof(scheduler,local_work.head) == 192);
    KASSERT(offsetof(scheduler,local_work.tail) == 200);

    // Delegation of work to a member function.  For this to work you must
    // populate wqe->args[0] with a T*.
    template<typename T, typename WQE, void (T::*Handler)(WQE*)>
    struct _work_delegate
    {
        static void handler(WQE* wqe)
        {
            (((T*)wqe->args[0])->*Handler)(wqe);
        }
    };
#define work_delegate(fn) \
    kernel::_work_delegate< \
        loki::remove_reference_t<decltype(*this)>, \
        kernel::wqe, \
        &loki::remove_reference_t<decltype(*this)>::fn>::handler
#define timer_delegate(fn) \
    kernel::_work_delegate< \
        loki::remove_reference_t<decltype(*this)>, \
        kernel::tqe, \
        &loki::remove_reference_t<decltype(*this)>::fn>::handler

#define method_tqe(fn) \
    kernel::tqe(timer_delegate(fn),(uint64_t)this)
#define method_wqe(fn) \
    kernel::wqe(work_delegate(fn),(uint64_t)this)

    // These will allocate/free WQEs off a locked, shared internal slab.  Use
    // of these is optional, you can define WQE objects anywhere.
    kernel::wqe* alloc_wqe();
    void free_wqe(kernel::wqe* wqe);
}

template<>
inline void notify_moved<kernel::tqe*>(kernel::tqe*& wqe, size_t new_pos)
{
    wqe->pos = new_pos;
}

#endif /* __KERNEL_SCHEDULE_H */
