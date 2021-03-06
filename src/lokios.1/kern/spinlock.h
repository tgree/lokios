#ifndef _KERNEL_SPINLOCK_H
#define _KERNEL_SPINLOCK_H

#include "thread.h"
#include "k++/with.h"
#include "hdr/x86.h"

namespace kernel
{
    struct spinlock
    {
        union
        {
            struct
            {
                uint8_t     val;
                thread_id   owner;
            };
            uint8_t     sector[128];
        } __ALIGNED__(128);

        template<void (&Pause)(void) = cpu_pause>
        inline void acquire()
        {
            kassert(owner != get_thread_id());
            while (__atomic_exchange_n(&val,1,__ATOMIC_ACQUIRE))
            {
                do
                {
                    Pause();
                }
                while (val);
            }
            owner = get_thread_id();
        }

        inline void release()
        {
            kassert(val == 1 && owner == get_thread_id());
            owner = 0;
            __atomic_store_n(&val,0,__ATOMIC_RELEASE);
        }

        constexpr spinlock():val(0),owner(0) {}
        inline ~spinlock() {kassert(val == 0);}
    };
    KASSERT(sizeof(spinlock) == 128);

    struct noop_lock
    {
        inline void acquire() {}
        inline void release() {}
    };
}

#endif /* _KERNEL_SPINLOCK_H */
