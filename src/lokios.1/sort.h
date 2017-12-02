#ifndef __KERNEL_SORT_H
#define __KERNEL_SORT_H

#include <iterator>

namespace kernel
{
    namespace sort
    {
        template<typename RandomAccessIterator>
        RandomAccessIterator partition(RandomAccessIterator begin,
                                       RandomAccessIterator end)
        {
            auto pivoti = end - 1;
            auto pivot  = *pivoti;
            auto i      = begin;
            for (auto j = begin; j != pivoti; ++j)
            {
                if (*j < pivot)
                {
                    auto tmp = *i;
                    *i       = *j;
                    *j       = tmp;
                    ++i;
                }
            }
            if (pivot < *i)
            {
                *pivoti = *i;
                *i      = pivot;
            }

            return i;
        }

        template<typename RandomAccessIterator>
        void quicksort(RandomAccessIterator begin, RandomAccessIterator end)
        {
            // TODO: Turn this compare into an == and then we can quicksort
            // anything that allows iterator increment, decrement and
            // assignment.
            if (begin >= end)
                return;

            RandomAccessIterator pivot = partition(begin,end);
            quicksort(begin,pivot);
            quicksort(pivot + 1,end);
        }

        template<typename T>
        inline void quicksort(T& c)
        {
            quicksort(std::begin(c),std::end(c));
        }

        // Given a sorted range, find the first entry we are smaller than.
        // TODO: Make this a binary search.
        template<typename T, typename RAI>
        RAI find_insertion_point(const T& e, RAI begin, RAI end)
        {
            while (begin != end && *begin < e)
                ++begin;
            return begin;
        }
    }
}

#endif /* __KERNEL_SORT_H */
