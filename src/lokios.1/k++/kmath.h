#ifndef __KERNEL_MATH_H
#define __KERNEL_MATH_H

#include "kern/kassert.h"
#include <stddef.h>

namespace kernel
{
    template<typename T>
    constexpr T _min(T v)
    {
        return v;
    }
    template<typename L, typename R, typename ...Args>
    constexpr auto _min(L l, R r, Args... args)
    {
        return (l < r) ? _min(l,args...) : _min(r,args...);
    }
#define MIN kernel::_min
    KASSERT(MIN(1,2,3,4,5)   == 1);
    KASSERT(MIN(1,-2,3,4,5)  == -2);
    KASSERT(MIN(1,-2,3,4,-5) == -5);
    KASSERT(MIN((int16_t)-1,(int32_t)0x80000000) == (int32_t)0x80000000);
    KASSERT(MIN((int32_t)0x80000000,(int16_t)-1) == (int32_t)0x80000000);
    KASSERT(MIN((int32_t)0x80000000,(uint16_t)1) == (int32_t)0x80000000);
    KASSERT(MIN((int16_t)-1,(uint16_t)5) == (int16_t)-1);

    template<typename T>
    constexpr T _max(T v)
    {
        return v;
    }
    template<typename L, typename R, typename ...Args>
    constexpr auto _max(L l, R r, Args... args)
    {
        return (l > r) ? _max(l,args...) : _max(r,args...);
    }
#define MAX kernel::_max
    KASSERT(MAX(1,2,3,4,5)     == 5);
    KASSERT(MAX(1,2,3,4,-5)    == 4);
    KASSERT(MAX(1,-2,-3,-4,-5) == 1);
    KASSERT(MAX((int16_t)-1,(int32_t)0x80000000) == -1);
    KASSERT(MAX((int32_t)0x80000000,(int16_t)-1) == -1);
    KASSERT(MAX((int32_t)0x80000000,(uint16_t)1) == 1);
    KASSERT(MAX((int16_t)-1,(uint16_t)5) == 5);

    template<typename T>
    constexpr T _clamp(T lower, T val, T upper)
    {
        return (val < lower ? lower :
                val > upper ? upper :
                val);
    }
#define CLAMP kernel::_clamp
    KASSERT(CLAMP(10,5,30) == 10);
    KASSERT(CLAMP(10,10,30) == 10);
    KASSERT(CLAMP(10,15,30) == 15);
    KASSERT(CLAMP(10,20,30) == 20);
    KASSERT(CLAMP(10,25,30) == 25);
    KASSERT(CLAMP(10,30,30) == 30);
    KASSERT(CLAMP(10,35,30) == 30);

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

    constexpr int ffs(unsigned int v)
    {
        return __builtin_ffs(v) - 1;
    }

    constexpr int ffs(unsigned long v)
    {
        return __builtin_ffsl(v) - 1;
    }

    constexpr int ffs(unsigned long long v)
    {
        return __builtin_ffsll(v) - 1;
    }
    KASSERT(ffs(0x12345678U) == 3);
    KASSERT(ffs(0x12345678UL) == 3);
    KASSERT(ffs(0x1234567800000000ULL) == 35);

    constexpr int popcount(unsigned int v)
    {
        return __builtin_popcount(v);
    }
    constexpr int popcount(unsigned long v)
    {
        return __builtin_popcountl(v);
    }
    constexpr int popcount(unsigned long long v)
    {
        return __builtin_popcountll(v);
    }
    KASSERT(popcount(0x11111111U) == 8);
    KASSERT(popcount(0x33333333UL) == 16);
    KASSERT(popcount(0x3333333344444444ULL) == 24);

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

    template<typename T>
    constexpr T round_to_nearest_multiple(T v, T base)
    {
        return (T)(((v + (base/2))/base)*base);
    }
    KASSERT(round_to_nearest_multiple(12345,100) == 12300);
    KASSERT(round_to_nearest_multiple(12349,100) == 12300);
    KASSERT(round_to_nearest_multiple(12350,100) == 12400);
    KASSERT(round_to_nearest_multiple(12351,100) == 12400);
    KASSERT(round_to_nearest_multiple(12399,100) == 12400);
    KASSERT(round_to_nearest_multiple(12400,100) == 12400);
    KASSERT(round_to_nearest_multiple(12401,100) == 12400);

    template<typename T>
    constexpr T round_down_to_nearest_multiple(T v, T base)
    {
        return (T)((v/base)*base);
    }
    KASSERT(round_down_to_nearest_multiple(12300,100) == 12300);
    KASSERT(round_down_to_nearest_multiple(12301,100) == 12300);
    KASSERT(round_down_to_nearest_multiple(12345,100) == 12300);
    KASSERT(round_down_to_nearest_multiple(12399,100) == 12300);
    KASSERT(round_down_to_nearest_multiple(12400,100) == 12400);

    template<typename T>
    constexpr T round_up_to_nearest_multiple(T v, T base)
    {
        return (T)(((v + (base - 1))/base)*base);
    }
    KASSERT(round_up_to_nearest_multiple(12300,100) == 12300);
    KASSERT(round_up_to_nearest_multiple(12301,100) == 12400);
    KASSERT(round_up_to_nearest_multiple(12345,100) == 12400);
    KASSERT(round_up_to_nearest_multiple(12399,100) == 12400);
    KASSERT(round_up_to_nearest_multiple(12400,100) == 12400);

    // Find the next power-of-2 that is greater than or equal to v.  Note that
    // this will wrap back to 0 if your high bit and some other bit are set.
    constexpr unsigned int ceil_pow2(unsigned int v)
    {
        return (!v ? 0 : (is_pow2(v) ? v : (2U << ulog2(v))));
    }

    constexpr unsigned long ceil_pow2(unsigned long v)
    {
        return (!v ? 0 : (is_pow2(v) ? v : (2UL << ulog2(v))));
    }

    constexpr unsigned long long ceil_pow2(unsigned long long v)
    {
        return (!v ? 0 : (is_pow2(v) ? v : (2ULL << ulog2(v))));
    }
    KASSERT(ceil_pow2(0U) == 0);
    KASSERT(ceil_pow2(1U) == 1);
    KASSERT(ceil_pow2(2U) == 2);
    KASSERT(ceil_pow2(3U) == 4);
    KASSERT(ceil_pow2(4U) == 4);
    KASSERT(ceil_pow2(5U) == 8);
    KASSERT(ceil_pow2(7U) == 8);
    KASSERT(ceil_pow2(8U) == 8);
    KASSERT(ceil_pow2(9U) == 16);

    template<typename T>
    constexpr T ceil_div(T num, T denom)
    {
        return (T)((num + denom - 1)/denom);
    }

    template<typename T>
    constexpr T floor_div(T num, T denom)
    {
        return (T)(num/denom);
    }
}

#endif /* __KERNEL_MATH_H */
