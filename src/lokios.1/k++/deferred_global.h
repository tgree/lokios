#ifndef __KERNEL_DEFERRED_GLOBAL_H
#define __KERNEL_DEFERRED_GLOBAL_H

#include "hdr/compiler.h"
#include "hdr/types.h"
#include "kernel/kassert.h"
#include <new>

namespace kernel
{
    template<typename T, bool auto_destroy = false>
    struct deferred_global
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

        constexpr deferred_global():
            storage{},
            inited(false)
        {
        }

        ~deferred_global()
        {
            if (auto_destroy && inited)
                destroy();
        }
    };
}

#endif /* __KERNEL_DEFERRED_GLOBAL_H */
