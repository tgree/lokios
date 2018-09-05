#ifndef __KERNEL_SORT_H
#define __KERNEL_SORT_H

#include "kernel_iterator.h"
#include "kern/kassert.h"
#include "functional.h"

template<typename T>
inline void notify_moved(T& t, size_t new_pos) {}

template<typename RandomAccessIterator>
inline void swap_contents(RandomAccessIterator p1, RandomAccessIterator p2)
{
    auto t = *p1;
    *p1    = *p2;
    *p2    = t;
}

template<typename RandomAccessIterator>
inline void swap_notifying(RandomAccessIterator begin, RandomAccessIterator p1,
                           RandomAccessIterator p2)
{
    swap_contents(p1,p2);
    notify_moved(*p1,p1-begin);
    notify_moved(*p2,p2-begin);
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

    template<typename RandomAccessIterator, typename Compare>
    void sift_down(RandomAccessIterator begin,
                   RandomAccessIterator end,
                   RandomAccessIterator pos,
                   Compare cmp)
    {
        while (has_left_child(begin,end,pos))
        {
            auto child = left_child_iter(begin,pos);
            auto swap  = pos;

            if (cmp(*child,*swap))
                swap = child;
            if (child + 1 < end && cmp(*(child + 1),*swap))
                swap = child + 1;
            if (swap == pos)
                return;

            swap_notifying(begin,pos,swap);
            pos = swap;
        }
    }

    template<typename RandomAccessIterator, typename Compare>
    void sift_up(RandomAccessIterator begin,
                 RandomAccessIterator pos,
                 Compare cmp)
    {
        while (pos != begin)
        {
            auto parent = parent_iter(begin,pos);
            if (!cmp(*pos,*parent))
                break;

            swap_notifying(begin,pos,parent);
            pos = parent;
        }
    }

    template<typename RandomAccessIterator, typename Compare>
    void insert(RandomAccessIterator begin,
                RandomAccessIterator end,
                Compare cmp)
    {
        // Note: the value to be inserted has already been pushed to the back
        // of the container, so end points just past the newly-inserted value.
        notify_moved(*(end-1),end-begin-1);
        sift_up(begin,end-1,cmp);
    }

    template<typename RandomAccessIterator, typename Compare>
    void remove(RandomAccessIterator begin,
                RandomAccessIterator end,
                RandomAccessIterator pos,
                Compare cmp)
    {
        // This moves the node in pos to the end of the container while
        // maintaining the heap property on rest of the nodes.  To actually
        // pop it you'll need to shrink the container by one element.
        kernel::kassert(begin != end);
        swap_notifying(begin,pos,end-1);
        notify_moved(*(end-1),-1);
        if (pos == begin || !cmp(*parent_iter(begin,pos),*pos))
            sift_down(begin,end-1,pos,cmp);
        else
            sift_up(begin,pos,cmp);
    }

    template<typename RandomAccessIterator,
     typename Compare = kernel::greater<decltype(**(RandomAccessIterator*)0)>>
    void heapify(RandomAccessIterator begin,
                 RandomAccessIterator end,
                 Compare cmp = Compare())
    {
        if (end - begin < 2)
            return;

        auto pos = parent_iter(begin,end - 1);
        do
        {
            sift_down(begin,end,pos,cmp);
        } while (pos-- != begin);
    }

    template<typename RandomAccessIterator,
     typename Compare = kernel::greater<decltype(**(RandomAccessIterator*)0)>>
    bool is_heap(RandomAccessIterator begin,
                 RandomAccessIterator end,
                 Compare cmp = Compare())
    {
        if (begin == end)
            return true;

        while (--end != begin)
        {
            if (cmp(*end,*parent_iter(begin,end)))
                return false;
        }
        return true;
    }

    template<typename RandomAccessIterator,
     typename Compare = kernel::greater<decltype(**(RandomAccessIterator*)0)>>
    void heapsort(RandomAccessIterator begin,
                  RandomAccessIterator end,
                  Compare cmp = Compare())
    {
        if (begin == end)
            return;

        heapify(begin,end,cmp);
        auto pos = end - 1;
        while (pos != begin)
        {
            swap_notifying(begin,begin,pos);
            sift_down(begin,--pos,begin,cmp);
        }
    }
}

namespace merge_sort
{
    template<typename Container, typename LessEqual>
    void merge(Container& c, Container& left, Container& right,
               LessEqual le)
    {
        while (!left.empty() && !right.empty())
        {
            if (le(left.front(),right.front()))
            {
                auto e = left.front();
                left.pop_front();
                c.push_back(e);
            }
            else
            {
                auto e = right.front();
                right.pop_front();
                c.push_back(e);
            }
        }
        if (!left.empty())
            c.append(left);
        if (!right.empty())
            c.append(right);
    }

    template<typename Container, typename LessEqual>
    void mergesort(Container& c, LessEqual le)
    {
        size_t len = c.size();
        if (len <= 1)
            return;

        Container left;
        Container right;
        for (size_t i=0; i<len/2; ++i)
        {
            auto e = c.front();
            c.pop_front();
            left.push_back(e);
        }
        right.append(c);

        mergesort(left,le);
        mergesort(right,le);
        merge(c,left,right,le);
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

    template<typename C, typename LessEqual>
    inline void mergesort(C& c, LessEqual le = LessEqual())
    {
        merge_sort::mergesort(c,le);
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
