#ifndef __KERNEL_HEAP_H
#define __KERNEL_HEAP_H

#include "sort.h"

namespace kernel
{
    template<typename Container>
    struct heap
    {
        typedef typename Container::value_type value_type;

        Container c;

        inline bool empty() const
        {
            return kernel::begin(c) == kernel::end(c);
        }

        inline value_type front() const
        {
            return *kernel::begin(c);
        }

        inline void insert(value_type v)
        {
            c.push_back(v);
            heap_sort::insert(kernel::begin(c),kernel::end(c));
        }

        inline void pop_front()
        {
            heap_sort::remove(kernel::begin(c),kernel::end(c),kernel::begin(c));
            c.pop_back();
        }

        inline void remove(size_t index)
        {
            auto iter = kernel::begin(c) + index;
            heap_sort::remove(kernel::begin(c),kernel::end(c),iter);
            c.pop_back();
        }
    };
}

#endif /* __KERNEL_HEAP_H */
