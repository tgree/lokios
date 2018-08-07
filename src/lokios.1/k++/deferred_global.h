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
        char    storage[sizeof(T)] __ALIGNED__(alignof(T));
        bool    inited;

        inline operator bool() {return inited;}

        inline operator T*()
        {
            kassert(inited);
            return (T*)storage;
        }

        inline T* operator->()
        {
            kassert(inited);
            return (T*)storage;
        }

        inline T& operator*()
        {
            kassert(inited);
            return *(T*)storage;
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
