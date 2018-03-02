#ifndef __KERNEL_OBJECT_LIST_H
#define __KERNEL_OBJECT_LIST_H

#include "allocator.h"
#include "klist.h"

namespace kernel
{
    template<typename T, typename Allocator = kernel::std_new_allocator>
    struct obj_list
    {
        struct node
        {
            kdlink  link;
            T       obj;

            template<typename ...Args>
            node(Args&& ...args):obj(std::forward<Args>(args)...) {}
        };
        constexpr static size_t node_size = sizeof(node);

        struct iterator
        {
            kdlink_leaks* n;

            inline T& operator*() const {return ((node*)n)->obj;}
            inline void operator++()    {n = n->next;}
            inline void operator--()    {n = n->prev;}

            inline iterator operator+(size_t count) const
            {
                kdlink_leaks* p = n;
                while (count--)
                    p = p->next;
                return iterator(p);
            }

            inline iterator operator-(size_t count) const
            {
                kdlink_leaks* p = n;
                while (count--)
                    p = p->prev;
                return iterator(p);
            }

            inline bool operator==(const iterator& rhs) const
            {
                return n == rhs.n;
            }

            inline bool operator!=(const iterator& rhs) const
            {
                return !(*this == rhs);
            }

            constexpr iterator(kdlink_leaks* n):n(n) {}
        };

        kdlist<node> objs;
        Allocator& allocator;

        constexpr iterator begin()
        {
            return iterator(objs.first_link());
        }

        constexpr const iterator end()
        {
            return iterator(const_cast<kdlink_leaks*>(objs.sentinel_link()));
        }

        inline bool empty() const  {return objs.empty();}
        inline size_t size() const {return objs.size();}
        inline T& front()          {return klist_front(objs,link)->obj;}

        template<typename ...Args>
        inline iterator emplace(iterator pos, Args&& ...args)
        {
            node* n = allocator.template alloc<node,Args...>(
                        std::forward<Args>(args)...);
            n->link.insert_before(pos.n);
            return iterator(&n->link);
        }

        inline iterator insert(iterator pos, const T& obj)
        {
            return emplace(pos,obj);
        }

        template<typename ...Args>
        inline void emplace_back(Args&& ...args)
        {
            emplace(end(),std::forward<Args>(args)...);
        }

        inline void push_back(const T& obj)
        {
            emplace_back(obj);
        }

        inline void erase(iterator pos)
        {
            node* n = (node*)pos.n;
            n->link.unlink();
            allocator.free(n);
        }

        inline void pop_front()
        {
            kassert(!empty());
            erase(begin());
        }

        inline void pop_all()
        {
            while (!empty())
                pop_front();
        }

        obj_list():allocator(Allocator::default_allocator) {}
        obj_list(Allocator& allocator):allocator(allocator) {}
    };
}

#endif /* __KERNEL_OBJECT_LIST_H */
