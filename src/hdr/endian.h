#ifndef __LOKIOS_ENDIAN_H
#define __LOKIOS_ENDIAN_H

#include "kassert.h"
#include "compiler.h"
#include <stddef.h>
#include <stdint.h>

template<typename T> T swap_uint(T);

template<>
constexpr uint64_t swap_uint<uint64_t>(uint64_t v)
{
    return ((v << 56) & 0xFF00000000000000) |
           ((v << 40) & 0x00FF000000000000) |
           ((v << 24) & 0x0000FF0000000000) |
           ((v <<  8) & 0x000000FF00000000) |
           ((v >>  8) & 0x00000000FF000000) |
           ((v >> 24) & 0x0000000000FF0000) |
           ((v >> 40) & 0x000000000000FF00) |
           ((v >> 56) & 0x00000000000000FF);
}

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
KASSERT(to_little_endian<uint64_t>(0x1122334455667788) == 0x1122334455667788);
KASSERT(to_little_endian<uint32_t>(0x11223344)   == 0x11223344);
KASSERT(to_little_endian<uint16_t>(0x1122)       == 0x1122);
KASSERT(from_little_endian<uint32_t>(0x11223344) == 0x11223344);
KASSERT(from_little_endian<uint16_t>(0x1122)     == 0x1122);
KASSERT(to_big_endian<uint64_t>(0x1122334455667788) == 0x8877665544332211);
KASSERT(to_big_endian<uint32_t>(0x11223344)      == 0x44332211);
KASSERT(to_big_endian<uint16_t>(0x1122)          == 0x2211);
KASSERT(from_big_endian<uint32_t>(0x11223344)    == 0x44332211);
KASSERT(from_big_endian<uint16_t>(0x1122)        == 0x2211);
#else
template<typename T> constexpr T to_little_endian(T v)   {return swap_uint(v);}
template<typename T> constexpr T from_little_endian(T v) {return swap_uint(v);}
template<typename T> constexpr T to_big_endian(T v)      {return v;}
template<typename T> constexpr T from_big_endian(T v)    {return v;}
KASSERT(to_little_endian<uint64_t>(0x1122334455667788) == 0x8877665544332211);
KASSERT(to_little_endian<uint32_t>(0x11223344)   == 0x44332211);
KASSERT(to_little_endian<uint16_t>(0x1122)       == 0x2211);
KASSERT(from_little_endian<uint32_t>(0x11223344) == 0x44332211);
KASSERT(from_little_endian<uint16_t>(0x1122)     == 0x2211);
KASSERT(to_big_endian<uint64_t>(0x1122334455667788) == 0x1122334455667788);
KASSERT(to_big_endian<uint32_t>(0x11223344)      == 0x11223344);
KASSERT(to_big_endian<uint16_t>(0x1122)          == 0x1122);
KASSERT(from_big_endian<uint32_t>(0x11223344)    == 0x11223344);
KASSERT(from_big_endian<uint16_t>(0x1122)        == 0x1122);
#endif

template<typename T>
struct be_uint
{
    T   v;
    constexpr operator T()          {return to_big_endian(v);}
    constexpr operator T() const    {return to_big_endian(v);}
    constexpr operator T() volatile {return to_big_endian(v);}
    void operator=(T _v)            {v = from_big_endian(_v);}
    void operator=(T _v) volatile   {v = from_big_endian(_v);}
    constexpr be_uint(T v):v(to_big_endian(v)) {}
    inline be_uint() = default;
} __PACKED__;

template<typename T>
struct le_uint
{
    T   v;
    constexpr operator T()          {return to_little_endian(v);}
    constexpr operator T() const    {return to_little_endian(v);}
    constexpr operator T() volatile {return to_little_endian(v);}
    void operator=(T _v)            {v = from_little_endian(_v);}
    void operator=(T _v) volatile   {v = from_little_endian(_v);}
    constexpr le_uint(T v):v(to_big_endian(v)) {}
    inline le_uint() = default;
} __PACKED__;

typedef be_uint<uint64_t> be_uint64_t;
typedef be_uint<uint32_t> be_uint32_t;
typedef be_uint<uint16_t> be_uint16_t;
typedef le_uint<uint64_t> le_uint64_t;
typedef le_uint<uint32_t> le_uint32_t;
typedef le_uint<uint16_t> le_uint16_t;
KASSERT(sizeof(be_uint64_t) == sizeof(uint64_t));
KASSERT(sizeof(be_uint32_t) == sizeof(uint32_t));
KASSERT(sizeof(be_uint16_t) == sizeof(uint16_t));
KASSERT(sizeof(le_uint64_t) == sizeof(uint64_t));
KASSERT(sizeof(le_uint32_t) == sizeof(uint32_t));
KASSERT(sizeof(le_uint16_t) == sizeof(uint16_t));

#endif /* __LOKIOS_ENDIAN_H */
