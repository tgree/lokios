#ifndef __KERNEL_VECTOR_H
#define __KERNEL_VECTOR_H

#include "vector_base.h"
#include "mm/page.h"

namespace kernel
{
    template<typename T>
    using vector = vector_base<T,page_allocator>;
}

#endif /* __KERNEL_VECTOR_H */
