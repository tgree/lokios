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

    template<typename T>
    constexpr bool is_pow2(T v)
    {
        return (v != 0) && ((v & (v-1)) == 0);
    }

    constexpr unsigned int ulog2(unsigned int v)
    {
        return __builtin_ffs(v) - 1;
    }

    constexpr unsigned int ulog2(unsigned long v)
    {
        return __builtin_ffsl(v) - 1;
    }

    constexpr unsigned int ulog2(unsigned long long v)
    {
        return __builtin_ffsll(v) - 1;
    }
}

#endif /* __KERNEL_MATH_H */
