#ifndef __KERNEL_RING_H
#define __KERNEL_RING_H

#include "kmath.h"
#include "hdr/types.h"
#include <stddef.h>
#include <new>

namespace kernel
{
    template<typename T>
    struct ring
    {
        T* const        elems;
        const size_t    nelems;
        const size_t    size_mask;
        size_t          producer;
        size_t          consumer;

        constexpr size_t size() const {return producer - consumer;}
        constexpr bool full() const   {return size() == nelems;}
        constexpr bool empty() const  {return size() == 0;}

        inline T& operator[](size_t index) const
        {
            kassert(index < size());
            return elems[(consumer + index) & size_mask];
        }

        template<typename ...Args>
        void emplace_back(Args&& ...args)
        {
            kassert(!full());
            T* t = &elems[producer & size_mask];
            new((void*)t) T(loki::forward<Args>(args)...);
            ++producer;
        }

        void pop_front(size_t n = 1)
        {
            for (size_t i=0; i<n; ++i)
                (*this)[i].~T();
            consumer += n;
        }

        ring(T* elems, size_t nelems):
            elems(elems),
            nelems(nelems),
            size_mask(nelems-1),
            producer(0),
            consumer(0)
        {
            kassert(is_pow2(nelems));
        }

        template<size_t N>
        ring(T (&elems)[N]):ring(elems,N) {}

        ~ring()
        {
            while (!empty())
                pop_front();
        }
    };
}

#endif /* __KERNEL_RING_H */
