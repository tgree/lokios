#ifndef __KERNEL_HEAP_H
#define __KERNEL_HEAP_H

#include "sort.h"
#include "functional.h"

namespace kernel
{
    template<typename Container, typename Compare>
    struct heap
    {
        typedef typename Container::value_type value_type;

        Container c;

        inline bool is_heap() const
        {
            return heap_sort::is_heap(kernel::begin(c),kernel::end(c),
                                      Compare());
        }

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
            heap_sort::insert(kernel::begin(c),kernel::end(c),Compare());
        }

        inline void pop_front()
        {
            heap_sort::remove(kernel::begin(c),kernel::end(c),kernel::begin(c),
                              Compare());
            c.pop_back();
        }

        inline void remove(size_t index)
        {
            auto iter = kernel::begin(c) + index;
            heap_sort::remove(kernel::begin(c),kernel::end(c),iter,Compare());
            c.pop_back();
        }
    };

    template<typename Container>
    using max_heap = heap<Container,
                          kernel::greater<typename Container::value_type>>;

    template<typename Container>
    using min_heap = heap<Container,
                          kernel::less<typename Container::value_type>>;
}

#endif /* __KERNEL_HEAP_H */
