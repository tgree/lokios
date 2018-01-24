#ifndef __KERNEL_ITERATOR_H
#define __KERNEL_ITERATOR_H

#include <stddef.h>

namespace kernel
{
    template<typename C>
    inline auto begin(C& c) -> decltype(c.begin())
    {
        return c.begin();
    }

    template<typename C>
    inline auto end(C& c) -> decltype(c.end())
    {
        return c.end();
    }

    template<typename T, size_t N>
    inline constexpr T* begin(T (&arr)[N])
    {
        return arr;
    }

    template<typename T, size_t N>
    inline constexpr T* end(T (&arr)[N])
    {
        return arr + N;
    }

    struct end_sentinel {};
}

#endif /* __KERNEL_ITERATOR_H */
