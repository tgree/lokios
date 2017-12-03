#ifndef __KERNEL_LIST_H
#define __KERNEL_LIST_H

#include "kassert.h"
#include "container_of.h"
#include <stddef.h>

namespace kernel
{
#define KLINK_NOT_IN_USE ((klink*)0x4444444444444444)
    struct klink
    {
        klink*  next;

        constexpr klink():next(KLINK_NOT_IN_USE) {}
        inline ~klink() {kassert(next == KLINK_NOT_IN_USE);}
    };

    template<typename T>
    struct klist
    {
        typedef T container_type;

        klink*  head;
        klink*  tail;

        constexpr bool empty() const {return head == NULL;}

        inline size_t size() const    
        {
            size_t n = 0;
            klink* l = head;
            while (l)
            {
                l = l->next;
                ++n;
            }
            return n;
        }

        inline void push_back(klink* l)
        {
            kassert(l->next == KLINK_NOT_IN_USE);
            l->next = NULL;
            if (!tail)
                head = l;
            else
                tail->next = l;
            tail = l;
        }

        inline void pop_front()
        {
            kassert(!empty());
            klink* curr = head;
            if (head == tail)
                head = tail = NULL;
            else
                head = head->next;
            curr->next = KLINK_NOT_IN_USE;
        }

        inline void pop_all()
        {
            while (!empty())
                pop_front();
        }

        constexpr klist():head(NULL),tail(NULL) {}
        inline ~klist() {kassert(head == NULL && tail == NULL);}
    };

    template<typename T, size_t OFFSET>
    struct klist_rbfl_adapter
    {
        klink* pos;

        inline T& operator*() const
        {
            return *(T*)((char*)pos - OFFSET);
        }

        inline void operator++()
        {
            pos = pos->next;
        }

        inline bool operator!=(const klist_rbfl_adapter&) const
        {
            // We only allow testing for a loop termination condition - this is
            // a RBFL adapter and not a general iterator.
            return pos != NULL;
        }

        inline klist_rbfl_adapter begin()
        {
            return *this;
        }

        inline const klist_rbfl_adapter end()
        {
            return *this;
        }

        constexpr klist_rbfl_adapter(klist<T>& kl):pos(kl.head) {}
    };

#define klist_front(q,field) \
    (q.empty() ? NULL : container_of(q.head,decltype(q)::container_type,field))

#define klist_elems(q,field) \
    kernel::klist_rbfl_adapter<decltype(q)::container_type, \
                               offsetof(decltype(q)::container_type,field)>(q)
}

#endif /* __KERNEL_LIST_H */
