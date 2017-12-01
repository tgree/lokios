#ifndef __KERNEL_MATH_H
#define __KERNEL_MATH_H

#include <stddef.h>

namespace kernel
{
    template<typename T>
    inline T min(T l, T r)
    {
        return (l < r ? l : r);
    }

    template<typename T>
    inline T max(T l, T r)
    {
        return (l < r ? r : l);
    }

    template<typename T, size_t N>
    constexpr size_t nelems(const T (&)[N])
    {
        return N;
    }
}

#endif /* __KERNEL_MATH_H */
