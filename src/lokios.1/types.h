#ifndef __KERNEL_TYPES_H
#define __KERNEL_TYPES_H

#include <stddef.h>

namespace kernel
{
    struct non_copyable
    {
        void operator=(const non_copyable&) = delete;

        non_copyable() = default;
        non_copyable(const non_copyable&) = delete;
    };

    template<size_t Width, typename T>
    struct fat_register
    {
        volatile T  data;
        const char  padding[Width - sizeof(T)];

        void operator=(T v)    {data = v;}
        operator volatile T&() {return data;}
    };
}

#endif /* __KERNEL_TYPES_H */
