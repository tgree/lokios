#ifndef __KERNEL_ALLOCATOR_H
#define __KERNEL_ALLOCATOR_H

#include <utility>

namespace kernel
{
    struct std_new_allocator
    {
        static std_new_allocator default_allocator;

        template<typename T, typename ...Args>
        inline T* alloc(Args&& ...args)
        {
            return new T(std::forward<Args>(args)...);
        }

        template<typename T>
        inline void free(T* obj)
        {
            delete obj;
        }
    };
}

#endif /* __KERNEL_ALLOCATOR_H */
