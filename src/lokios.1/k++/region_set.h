#ifndef __KERNEL_REGION_SET_H
#define __KERNEL_REGION_SET_H

#include "sort.h"
#include "kmath.h"
#include "kernel/kassert.h"
#include <stdint.h>

namespace kernel
{
    // A region containing bytes in the range [first, last].
    struct region
    {
        uint64_t    first;
        uint64_t    last;
    };

    // Coalesce overlapping regions in the sorted range.
    template<typename C>
    void region_compress(C& c)
    {
        auto i = c.begin();
        while (i != c.end())
        {
            auto j = i + 1;
            while (j != c.end() && j->first <= i->last)
            {
                i->last = j->last;
                c.erase(j);
                j = i + 1;
            }
            ++i;
        }
    }

    template<typename C>
    void region_add(C& c, typename C::value_type& r)
    {
        auto pos = sort::find_insertion_point(r,c.begin(),c.end());
        c.insert(pos,r);
        region_compress(c);
    }

    template<typename C>
    void region_remove(C& c, uint64_t first, uint64_t last)
    {
        auto i = c.begin();
        while (i != c.end())
        {
            region intersection = {max(first,i->first),min(last,i->last)};
            if (intersection.first > i->last || intersection.last < i->first)
                ++i;
            else if (intersection.first > i->first &&
                     intersection.last == i->last)
            {
                i->last = intersection.first - 1;
                ++i;
            }
            else if (intersection.first > i->first &&
                     intersection.last < i->last)
            {
                // Sadly this invalidates our iterator because we may need to
                // grow the container.
                region front = {i->first,intersection.first - 1};
                i->first = intersection.last + 1;
                i = c.insert(i,front) + 2;
            }
            else if (intersection.first == i->first &&
                     intersection.last < i->last)
            {
                i->first = intersection.last + 1;
                ++i;
            }
            else
            {
                kassert(intersection.first == i->first);
                kassert(intersection.last == i->last);
                i = c.erase(i);
            }
        }
    }

    template<typename C>
    void region_remove(C& c, const region& r)
    {
        region_remove(c,r.first,r.last);
    }

    template<typename C>
    void regions_remove(C& c, const C& c2)
    {
        for (auto& e : c2)
            region_remove(c,e);
    }

    template<typename C>
    bool regions_contains(C& c, uint64_t addr)
    {
        for (auto& r : c)
        {
            if (r.first <= addr && addr <= r.last)
                return true;
        }
        return false;
    }
}

inline bool operator==(const kernel::region& l,
                       const kernel::region& r)
{
    return l.first == r.first && l.last == r.last;
}

inline bool operator!=(const kernel::region& l,
                       const kernel::region& r)
{
    return !(l == r);
}

inline bool operator<(const kernel::region& l,
                      const kernel::region& r)
{
    if (l.first < r.first)
        return true;
    if (l.first > r.first)
        return false;

    return l.last < r.last;
}

#endif /* __KERNEL_REGION_SET_H */
