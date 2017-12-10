#ifndef __KERNEL_STATIC_VECTOR_H
#define __KERNEL_STATIC_VECTOR_H

#include "vector_base.h"
#include "hdr/compiler.h"

namespace kernel
{
    template<typename T, size_t N>
    struct static_vector : public vector_base<T>
    {
        uint8_t _storage[sizeof(T)*(N+1)] __ALIGNED__(16);

        constexpr static_vector():vector_base<T>(_storage,N) {}
        inline static_vector(size_t size):vector_base<T>(_storage,N,size) {}
        inline static_vector(const vector_base<T>& other):
            vector_base<T>(_storage,N,other) {}
        template<size_t N2>
        inline static_vector(const static_vector<T,N2>& other):
            vector_base<T>((const vector_base<T>&)other)
        {
            KASSERT(N <= N2);
        }
    };
}

#endif /* __KERNEL_STATIC_VECTOR_H */
