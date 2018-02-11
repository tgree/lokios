#ifndef __KERNEL_TYPES_H
#define __KERNEL_TYPES_H

#include "kassert.h"
#include <stddef.h>
#include <stdint.h>

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

template<typename T> T swap_uint(T);

template<>
constexpr uint32_t swap_uint<uint32_t>(uint32_t v)
{
    return (v << 24) | ((v << 8) & 0x00FF0000) |
           ((v >> 8) & 0x0000FF00) | (v >> 24);
}

template<>
constexpr uint16_t swap_uint<uint16_t>(uint16_t v)
{
    return (v << 8) | (v >> 8);
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
template<typename T> constexpr T to_little_endian(T v)   {return v;}
template<typename T> constexpr T from_little_endian(T v) {return v;}
template<typename T> constexpr T to_big_endian(T v)      {return swap_uint(v);}
template<typename T> constexpr T from_big_endian(T v)    {return swap_uint(v);}
KASSERT(to_little_endian<uint32_t>(0x11223344)   == 0x11223344);
KASSERT(to_little_endian<uint16_t>(0x1122)       == 0x1122);
KASSERT(from_little_endian<uint32_t>(0x11223344) == 0x11223344);
KASSERT(from_little_endian<uint16_t>(0x1122)     == 0x1122);
KASSERT(to_big_endian<uint32_t>(0x11223344)      == 0x44332211);
KASSERT(to_big_endian<uint16_t>(0x1122)          == 0x2211);
KASSERT(from_big_endian<uint32_t>(0x11223344)    == 0x44332211);
KASSERT(from_big_endian<uint16_t>(0x1122)        == 0x2211);
#else
template<typename T> constexpr T to_little_endian(T v)   {return swap_uint(v);}
template<typename T> constexpr T from_little_endian(T v) {return swap_uint(v);}
template<typename T> constexpr T to_big_endian(T v)      {return v;}
template<typename T> constexpr T from_big_endian(T v)    {return v;}
KASSERT(to_little_endian<uint32_t>(0x11223344)   == 0x44332211);
KASSERT(to_little_endian<uint16_t>(0x1122)       == 0x2211);
KASSERT(from_little_endian<uint32_t>(0x11223344) == 0x44332211);
KASSERT(from_little_endian<uint16_t>(0x1122)     == 0x2211);
KASSERT(to_big_endian<uint32_t>(0x11223344)      == 0x11223344);
KASSERT(to_big_endian<uint16_t>(0x1122)          == 0x1122);
KASSERT(from_big_endian<uint32_t>(0x11223344)    == 0x11223344);
KASSERT(from_big_endian<uint16_t>(0x1122)        == 0x1122);
#endif

template<typename T>
struct be_uint
{
    T   v;
    constexpr operator T() {return to_big_endian(v);}
    void operator=(T _v)   {v = from_big_endian(_v);}
};

template<typename T>
struct le_uint
{
    T   v;
    constexpr operator T() {return to_little_endian(v);}
    void operator=(T _v)   {v = from_little_endian(_v);}
};

typedef be_uint<uint32_t> be_uint32_t;
typedef be_uint<uint16_t> be_uint16_t;
typedef le_uint<uint32_t> le_uint32_t;
typedef le_uint<uint16_t> le_uint16_t;

#endif /* __KERNEL_TYPES_H */
