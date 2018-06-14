#ifndef __KERNEL_LOCAL_VECTOR_H
#define __KERNEL_LOCAL_VECTOR_H

#include "vector_base.h"
#include "allocator.h"
#include "../mm/sbrk.h"

namespace kernel
{
    template<typename T>
    using local_vector = vector_base<T,static_allocator<4096>>;
    template<typename T>
    using sbrk_vector = vector_base<T,sbrk_allocator<4096>>;
}

#endif /* __KERNEL_LOCAL_VECTOR_H */
