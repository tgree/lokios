#ifndef __KERNEL_LOCAL_VECTOR_H
#define __KERNEL_LOCAL_VECTOR_H

#include "vector_base.h"

namespace kernel
{
    template<typename T>
    struct local_vector : public vector_base<T>
    {
        constexpr local_vector(T* _elems, size_t _capacity):
            vector_base<T>(_elems,_capacity) {}
        inline local_vector(T* _elems, size_t _capacity, size_t _size):
            vector_base<T>(_elems,_capacity,_size) {}
    };
}

#endif /* __KERNEL_LOCAL_VECTOR_H */
