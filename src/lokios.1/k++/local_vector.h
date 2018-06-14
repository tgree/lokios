#ifndef __KERNEL_LOCAL_VECTOR_H
#define __KERNEL_LOCAL_VECTOR_H

#include "vector_base.h"
#include "../mm/sbrk.h"

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

    template<typename T>
    struct sbrk_vector : public local_vector<T>
    {
        inline sbrk_vector(size_t bytelen):
            local_vector<T>((T*)phys_to_virt(sbrk(bytelen)),
                            bytelen/sizeof(T))
        {
        }
    };
}

#endif /* __KERNEL_LOCAL_VECTOR_H */
