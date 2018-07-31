#ifndef __KERNEL_DEFERRED_GLOBAL_H
#define __KERNEL_DEFERRED_GLOBAL_H

#include "hdr/compiler.h"
#include "hdr/types.h"

namespace kernel
{
    template<typename T>
    struct deferred_global
    {
        bool    inited;
        char    storage[sizeof(T)] __ALIGNED__(alignof(T));

        inline operator bool() {return inited;}

        inline operator T&()
        {
            kassert(inited);
            return *(T*)storage;
        }

        inline T* operator->()
        {
            kassert(inited);
            return (T*)storage;
        }

        template<typename ...Args>
        inline void init(Args&& ...args)
        {
            kassert(!inited);
            inited = true;
            new(storage) T(loki::forward<Args>(args)...);
        }

        inline void destroy()
        {
            kassert(inited);
            inited = false;
            ((T*)storage)->~T();
        }

        constexpr deferred_global():
            inited(false),
            storage{}
        {
        }
    };
}

#endif /* __KERNEL_DEFERRED_GLOBAL_H */
