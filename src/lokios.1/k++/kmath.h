#ifndef __KERNEL_MATH_H
#define __KERNEL_MATH_H

#include "kernel/kassert.h"
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
        return sizeof(v)*8 - __builtin_clz(v) - 1;
    }

    constexpr unsigned int ulog2(unsigned long v)
    {
        return sizeof(v)*8 - __builtin_clzl(v) - 1;
    }

    constexpr unsigned int ulog2(unsigned long long v)
    {
        return sizeof(v)*8 - __builtin_clzll(v) - 1;
    }

    template<typename T>
    constexpr T round_up_pow2(T v, uint64_t p2)
    {
        kassert(is_pow2(p2));
        return (T)(((uint64_t)v + p2 - 1) & ~(p2 - 1));
    }

    template<typename T>
    constexpr T round_down_pow2(T v, uint64_t p2)
    {
        kassert(is_pow2(p2));
        return (T)(((uint64_t)v) & ~(p2 - 1));
    }
}

#endif /* __KERNEL_MATH_H */
