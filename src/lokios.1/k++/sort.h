#ifndef __KERNEL_SORT_H
#define __KERNEL_SORT_H

#include "kernel_iterator.h"
#include "kernel/kassert.h"

template<typename RandomAccessIterator>
inline void swap_contents(RandomAccessIterator p1, RandomAccessIterator p2)
{
    auto t = *p1;
    *p1    = *p2;
    *p2    = t;
}

namespace quick_sort
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
                swap_contents(i,j);
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

}

namespace heap_sort
{
    template<typename RandomAccessIterator>
    constexpr RandomAccessIterator parent_iter(RandomAccessIterator begin,
                                               RandomAccessIterator pos)
    {
        kernel::kassert(pos > begin);
        return begin + (pos - begin - 1)/2;
    }

    template<typename RandomAccessIterator>
    constexpr bool has_left_child(RandomAccessIterator begin,
                                  RandomAccessIterator end,
                                  RandomAccessIterator pos)
    {
        return (2*(pos - begin) + 1) < (end - begin);
    }

    template<typename RandomAccessIterator>
    constexpr RandomAccessIterator left_child_iter(RandomAccessIterator begin,
                                                   RandomAccessIterator pos)
    {
        return begin + (2*(pos - begin) + 1);
    }

    template<typename RandomAccessIterator>
    constexpr bool has_right_child(RandomAccessIterator begin,
                                   RandomAccessIterator end,
                                   RandomAccessIterator pos)
    {
        return (2*(pos - begin) + 2) < (end - begin);
    }
                             
    template<typename RandomAccessIterator>
    constexpr RandomAccessIterator right_child_iter(RandomAccessIterator begin,
                                                    RandomAccessIterator pos)
    {
        return begin + (2*(pos - begin) + 2);
    }

    template<typename RandomAccessIterator>
    void sift_down(RandomAccessIterator begin,
                   RandomAccessIterator end,
                   RandomAccessIterator pos)
    {
        while (has_left_child(begin,end,pos))
        {
            auto child = left_child_iter(begin,pos);
            auto swap  = pos;

            if (*swap < *child)
                swap = child;
            if (child + 1 < end && *swap < *(child + 1))
                swap = child + 1;
            if (swap == pos)
                return;

            swap_contents(pos,swap);
            pos = swap;
        }
    }

    template<typename RandomAccessIterator>
    void heapify(RandomAccessIterator begin,
                 RandomAccessIterator end)
    {
        if (end - begin < 2)
            return;

        auto pos = parent_iter(begin,end - 1);
        do
        {
            sift_down(begin,end,pos);
        } while (pos-- != begin);
    }

    template<typename RandomAccessIterator>
    bool is_max_heap(RandomAccessIterator begin,
                     RandomAccessIterator end)
    {
        if (begin == end)
            return true;

        while (--end != begin)
        {
            if (*parent_iter(begin,end) < *end)
                return false;
        }
        return true;
    }

    template<typename RandomAccessIterator>
    void heapsort(RandomAccessIterator begin,
                  RandomAccessIterator end)
    {
        if (begin == end)
            return;

        heapify(begin,end);
        auto pos = end - 1;
        while (pos != begin)
        {
            swap_contents(begin,pos);
            sift_down(begin,--pos,begin);
        }
    }
}

namespace kernel::sort
{
    template<typename T>
    inline void quicksort(T& c)
    {
        quick_sort::quicksort(kernel::begin(c),kernel::end(c));
    }

    template<typename T>
    inline void heapsort(T& c)
    {
        heap_sort::heapsort(kernel::begin(c),kernel::end(c));
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

#endif /* __KERNEL_SORT_H */
