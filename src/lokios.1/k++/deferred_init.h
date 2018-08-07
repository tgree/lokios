#ifndef __KERNEL_DEFERRED_INIT_H
#define __KERNEL_DEFERRED_INIT_H

#include "hdr/compiler.h"
#include "hdr/types.h"
#include "kernel/kassert.h"
#include <new>

namespace kernel
{
    template<typename T, bool auto_destroy = true>
    struct deferred_init
    {
        union
        {
            char    storage[1];
            T       obj;
        };
        bool    inited;

        inline operator bool() {return inited;}

        inline operator T*()
        {
            kassert(inited);
            return &obj;
        }

        inline T* operator->()
        {
            kassert(inited);
            return &obj;
        }

        inline T& operator*()
        {
            kassert(inited);
            return obj;
        }

        template<typename ...Args>
        inline void init(Args&& ...args)
        {
            kassert(!inited);
            inited = true;
            new(&obj) T(loki::forward<Args>(args)...);
        }

        inline void destroy()
        {
            kassert(inited);
            inited = false;
            obj.~T();
        }

        constexpr deferred_init():
            storage{},
            inited(false)
        {
        }

        ~deferred_init()
        {
            if (auto_destroy && inited)
                destroy();
        }
    };

    // Don't call the destructor; useful for actual globals.
    template<typename T>
    using deferred_global = deferred_init<T,false>;
}

#endif /* __KERNEL_DEFERRED_INIT_H */
