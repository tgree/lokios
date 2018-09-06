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

template<typename T, T (*from_endian)(T), T (*to_endian)(T)>
struct endian_uint
{
    T   v;
    constexpr operator T()          {return from_endian(v);}
    constexpr operator T() const    {return from_endian(v);}
    constexpr operator T() volatile {return from_endian(v);}
    void operator=(T _v)            {v = to_endian(_v);}
    void operator=(T _v) volatile   {v = to_endian(_v);}

    inline endian_uint& operator+=(T _v)
    {
        v = to_endian((T)(from_endian(v) + _v));
        return *this;
    }

    constexpr endian_uint(T v):v(to_endian(v)) {}
    inline endian_uint() = default;
} __PACKED__;

template<typename T>
using be_uint = endian_uint<T,from_big_endian,to_big_endian>;

template<typename T>
using le_uint = endian_uint<T,from_little_endian,to_little_endian>;

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

// Convert a sequence of bytes into an appropriately-sized unsigned integer
// type.  This handles the endian conversion if necessary.
constexpr uint16_t char_code(char a, char b)
{
    return from_big_endian<uint16_t>(((uint16_t)a << 8) |
                                     ((uint16_t)b << 0));
}

constexpr uint32_t char_code(char a, char b, char c, char d)
{
    return from_big_endian<uint32_t>((uint32_t)((uint32_t)a << 24) |
                                               ((uint32_t)b << 16) |
                                               ((uint32_t)c <<  8) |
                                               ((uint32_t)d <<  0));
}

constexpr uint64_t char_code(char a, char b, char c, char d, char e, char f,
                             char g, char h)
{
    return from_big_endian<uint64_t>(((uint64_t)a << 56) |
                                     ((uint64_t)b << 48) |
                                     ((uint64_t)c << 40) |
                                     ((uint64_t)d << 32) |
                                     ((uint64_t)e << 24) |
                                     ((uint64_t)f << 16) |
                                     ((uint64_t)g <<  8) |
                                     ((uint64_t)h <<  0));
}

// Convert a string into an appropriately-sized unsigned integer type.  This
// also handles endian conversion if necessary.
constexpr uint16_t char_code(const char (&s)[3])
{
    return char_code(s[0],s[1]);
}

constexpr uint32_t char_code(const char (&s)[5])
{
    return char_code(s[0],s[1],s[2],s[3]);
}

constexpr uint64_t char_code(const char (&s)[9])
{
    return char_code(s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7]);
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
KASSERT(char_code("01")       == 0x3130);
KASSERT(char_code("0123")     == 0x33323130);
KASSERT(char_code("01234567") == 0x3736353433323130);
#else
KASSERT(char_code("01")       == 0x3031);
KASSERT(char_code("0123")     == 0x30313233);
KASSERT(char_code("01234567") == 0x3031323334353637);
#endif

#endif /* __LOKIOS_ENDIAN_H */
