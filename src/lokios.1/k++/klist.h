#ifndef __KERNEL_LIST_H
#define __KERNEL_LIST_H

#include "kernel_iterator.h"
#include "kernel/kassert.h"
#include <hdr/container_of.h>
#include <stddef.h>

namespace kernel
{
#define KLINK_NOT_IN_USE 0x4444444444444444U
    struct klink
    {
        union
        {
            uintptr_t   nextu;
            klink*      next;
        };

        constexpr klink():nextu(KLINK_NOT_IN_USE) {}
        inline ~klink() {kassert(nextu == KLINK_NOT_IN_USE);}
    };

    template<typename T>
    struct klist_leaks
    {
        typedef T elem_type;
        typedef klink link_type;

        klink*  head;
        klink*  tail;

        inline void clear() {head = tail = NULL;}
        constexpr bool empty() const {return head == NULL;}

        constexpr klink* first_link() const {return head;}
        constexpr const klink* sentinel_link() const {return NULL;}

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
            kassert(l->nextu == KLINK_NOT_IN_USE);
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
            curr->nextu = KLINK_NOT_IN_USE;
        }

        inline void pop_all()
        {
            while (!empty())
                pop_front();
        }

        inline void append(klist_leaks& other)
        {
            if (other.empty())
                return;

            if (tail)
                tail->next = other.head;
            else
                head = other.head;

            tail       = other.tail;
            other.head = NULL;
            other.tail = NULL;
        }

        constexpr klist_leaks():head(NULL),tail(NULL) {}
    };

    template<typename T>
    struct klist : public klist_leaks<T>
    {
        constexpr klist():klist_leaks<T>() {}
        inline ~klist() {kassert(this->head == NULL && this->tail == NULL);}
    };

    struct kdlink_leaks
    {
        union
        {
            struct
            {
                uintptr_t   nextu;
                uintptr_t   prevu;
            };
            struct
            {
                kdlink_leaks*   next;
                kdlink_leaks*   prev;
            };
        };

        inline void unlink()
        {
            prev->next = next;
            next->prev = prev;
        }

        constexpr kdlink_leaks():
            nextu(KLINK_NOT_IN_USE),
            prevu(KLINK_NOT_IN_USE)
        {
        }
        constexpr kdlink_leaks(kdlink_leaks* next,kdlink_leaks* prev):
            next(next),
            prev(prev)
        {
        }
    };

    struct kdlink : public kdlink_leaks
    {
        inline void unlink()
        {
            kdlink_leaks::unlink();
            nextu = prevu = KLINK_NOT_IN_USE;
        }

        constexpr kdlink():kdlink_leaks() {}
        inline ~kdlink()
        {
            kassert(nextu == KLINK_NOT_IN_USE && prevu == KLINK_NOT_IN_USE);
        }
    };

    template<typename T>
    struct kdlist_leaks
    {
        typedef T elem_type;
        typedef kdlink_leaks link_type;

        kdlink_leaks    sentinel;

        inline void clear() {sentinel.next = sentinel.prev = &sentinel;}
        constexpr bool empty() const {return sentinel.next == &sentinel;}

        constexpr kdlink_leaks* first_link() const {return sentinel.next;}
        constexpr const kdlink_leaks* sentinel_link() const {return &sentinel;}

        inline size_t size() const
        {
            size_t n = 0;
            auto* l  = sentinel.next;
            while (l != &sentinel)
            {
                l = l->next;
                ++n;
            }
            return n;
        }

        inline void push_back(kdlink_leaks* l)
        {
            kassert(l->nextu == KLINK_NOT_IN_USE);
            kassert(l->prevu == KLINK_NOT_IN_USE);
            l->next             = &sentinel;
            l->prev             = sentinel.prev;
            sentinel.prev->next = l;
            sentinel.prev       = l;
        }

        inline void pop_front()
        {
            kassert(!empty());
            auto* l = sentinel.next;
            l->next->prev = &sentinel;
            sentinel.next = l->next;
            l->nextu      = KLINK_NOT_IN_USE;
            l->prevu      = KLINK_NOT_IN_USE;
        }

        inline void pop_all()
        {
            while (!empty())
                pop_front();
        }

        inline void append(kdlist_leaks& other)
        {
            if (other.empty())
                return;

            sentinel.prev->next       = other.sentinel.next;
            other.sentinel.next->prev = sentinel.prev;
            other.sentinel.prev->next = &sentinel;
            sentinel.prev             = other.sentinel.prev;
            other.sentinel.next       = &other.sentinel;
            other.sentinel.prev       = &other.sentinel;
        }

        constexpr kdlist_leaks():sentinel(&sentinel,&sentinel) {}
    };

    template<typename T>
    struct kdlist : public kdlist_leaks<T>
    {
        constexpr kdlist():kdlist_leaks<T>() {}
        inline ~kdlist()
        {
            kassert(this->sentinel.next == &this->sentinel &&
                    this->sentinel.prev == &this->sentinel);
        }
    };

    template<typename L, size_t OFFSET>
    struct klist_rbfl_adapter
    {
        typedef typename L::elem_type T;

        typename L::link_type* pos;
        const typename L::link_type* sentinel;

        inline T& operator*() const
        {
            return *(T*)((char*)pos - OFFSET);
        }

        inline void operator++()
        {
            pos = pos->next;
        }

        inline bool operator!=(kernel::end_sentinel) const
        {
            // We only allow testing for a loop termination condition - this is
            // a RBFL adapter and not a general iterator.
            return pos != sentinel;
        }

        inline klist_rbfl_adapter begin()
        {
            return *this;
        }

        inline kernel::end_sentinel end() const
        {
            return kernel::end_sentinel();
        }

        constexpr klist_rbfl_adapter(L& kl):
            pos(kl.first_link()),
            sentinel(kl.sentinel_link())
        {}
    };

#define klist_front(q,field) \
    (q.empty() ? NULL : \
     container_of(q.first_link(),typename decltype(q)::elem_type,field))

#define klist_elems(q,field) \
    kernel::klist_rbfl_adapter<decltype(q), \
                       offsetof(typename decltype(q)::elem_type,field)>(q)
}

#endif /* __KERNEL_LIST_H */
