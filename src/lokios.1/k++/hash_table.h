#ifndef __KERNEL_HASH_TABLE_H
#define __KERNEL_HASH_TABLE_H

#include "hash.h"
#include "klist.h"
#include "mm/page.h"
#include "mm/buddy_allocator.h"
#include "mm/slab.h"

namespace hash
{
    struct no_such_key_exception : public kernel::message_exception
    {
        constexpr no_such_key_exception():message_exception("no such key") {}
    };

    template<typename Key, typename Value,
             size_t (&HashFunc)(const Key&) = hash::hasher<Key>::compute>
    struct table
    {
        struct dummy {};
        struct node
        {
            kernel::kdlink  link;
            Key             k;
            Value           v;

            node(const Key& k, const Value& v):k(k),v(v) {}

            template<typename ...Args>
            node(dummy, const Key& k, Args&& ...args):
                k(k),
                v(loki::forward<Args>(args)...)
            {
            }
        };

        size_t                  nelems;
        size_t                  nbins;
        kernel::kdlist<node>*   bins;
        kernel::slab            node_slab;

        size_t size() const {return nelems;}
        bool empty() const  {return size() == 0;}

        static size_t compute_slot(const Key& k, size_t nbins)
        {
            return (HashFunc(k) & (nbins-1));
        }

        static typeof(bins) alloc_bins(size_t n)
        {
            kernel::kassert(kernel::is_pow2(n));
            kernel::kassert(n >= PAGE_SIZE/sizeof(bins[0]));

            size_t order = kernel::ulog2(n*sizeof(bins[0])) - 12;
            auto* b      = (typeof(bins))kernel::buddy_alloc(order);
            return new(b) kernel::kdlist<node>[n];
        }

        static void free_bins(typeof(bins) b, size_t n)
        {
            kernel::kassert(kernel::is_pow2(n));
            kernel::kassert(n >= PAGE_SIZE/sizeof(bins[0]));

            for (size_t i=0; i<n; ++i)
                b[i].~kdlist();

            size_t order = kernel::ulog2(n*sizeof(bins[0])) - 12;
            kernel::buddy_free(b,order);
        }

        node* find_node_in_slot(const Key& k, size_t slot) const
        {
            for (auto& n : klist_elems(bins[slot],link))
            {
                if (hash::equals(n.k,k))
                    return &n;
            }
            return NULL;
        }

        node* find_node(const Key& k) const
        {
            return find_node_in_slot(k,compute_slot(k,nbins));
        }

        bool contains(const Key& k) const
        {
            return find_node(k) != NULL;
        }

        Value& insert(const Key& k, const Value& v)
        {
            size_t slot = compute_slot(k,nbins);
            kernel::kassert(!find_node_in_slot(k,slot));
            node* n = node_slab.alloc<node>(k,v);
            bins[slot].push_back(&n->link);
            ++nelems;
            return n->v;
        }

        template<typename ...Args>
        Value& emplace(const Key& k, Args&& ...args)
        {
            size_t slot = compute_slot(k,nbins);
            kernel::kassert(!find_node_in_slot(k,slot));
            node* n = node_slab.alloc<node>(dummy(),k,
                                            loki::forward<Args>(args)...);
            bins[slot].push_back(&n->link);
            ++nelems;
            return n->v;
        }

        void erase(node* n)
        {
            --nelems;
            n->link.unlink();
            node_slab.free(n);
        }

        void erase_value(Value* v)
        {
            erase(container_of(v,node,v));
        }

        bool erase(const Key& k)
        {
            node* n = find_node(k);
            if (!n)
                return false;

            erase(n);
            return true;
        }

        Value& operator[](const Key& k)
        {
            node* n = find_node(k);
            if (!n)
                throw no_such_key_exception();
            return n->v;
        }

        const Value& operator[](const Key& k) const
        {
            node* n = find_node(k);
            if (!n)
                throw no_such_key_exception();
            return n->v;
        }

        table():
            nelems(0),
            nbins(PAGE_SIZE/sizeof(bins[0])),
            bins(alloc_bins(nbins)),
            node_slab(sizeof(node))
        {
        }

        ~table()
        {
            for (size_t i=0; i<nbins; ++i)
            {
                while (!bins[i].empty())
                {
                    auto* n = klist_front(bins[i],link);
                    bins[i].pop_front();
                    node_slab.free(n);
                }
            }
            free_bins(bins,nbins);
        }
    };
}

#endif /* __KERNEL_HASH_TABLE_H */
