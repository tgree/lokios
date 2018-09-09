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
             auto HashFunc = hash::hasher<Key>::compute>
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

        struct rbfl_adapter
        {
            table&                  t;
            size_t                  bin;
            kernel::kdlink_leaks*   pos;

            inline node& operator*() const
            {
                return *container_of(pos,node,link);
            }

            inline void operator++()
            {
                kernel::kassert(pos != (kernel::kdlink_leaks*)this);
                pos = pos->next;
                while (pos == t.bins[bin].sentinel_link())
                {
                    if (++bin == t.nbins)
                    {
                        // We just need a unique address that's guaranteed to
                        // not be NULL and not be on any queue.  A pointer to
                        // our iterator will do just fine.
                        pos = (kernel::kdlink_leaks*)this;
                        break;
                    }

                    pos = t.bins[bin].first_link();
                }
            }

            inline bool operator!=(kernel::end_sentinel) const
            {
                return pos != (kernel::kdlink_leaks*)this;
            }

            rbfl_adapter(table& t):
                t(t),
                bin(0)
            {
                pos = t.bins[0].first_link();
                if (pos == t.bins[0].sentinel_link())
                    ++(*this);
            }
        };

        size_t                  nelems;
        size_t                  nbins;
        kernel::kdlist<node>*   bins;
        kernel::kdlist<node>    unlinked_nodes;
        kernel::slab            node_slab;

        size_t size() const {return nelems;}
        bool empty() const  {return size() == 0;}

        rbfl_adapter begin()
        {
            return rbfl_adapter(*this);
        }

        inline kernel::end_sentinel end() const
        {
            return kernel::end_sentinel();
        }

        static size_t compute_slot(const Key& k, size_t nbins)
        {
            return (HashFunc(k) & (nbins-1));
        }

        static typeof(bins) alloc_bins(size_t n)
        {
            kernel::kassert(kernel::is_pow2(n));
            kernel::kassert(n >= PAGE_SIZE/sizeof(bins[0]));

            size_t len = n*sizeof(bins[0]);
            auto* b    = (typeof(bins))kernel::buddy_alloc_by_len(len);
            return new(b) kernel::kdlist<node>[n];
        }

        static void free_bins(typeof(bins) b, size_t n)
        {
            kernel::kassert(kernel::is_pow2(n));
            kernel::kassert(n >= PAGE_SIZE/sizeof(bins[0]));

            for (size_t i=0; i<n; ++i)
                b[i].~kdlist();

            kernel::buddy_free_by_len(b,n*sizeof(bins[0]));
        }

        static void bin_transfer(typeof(bins) old_bins, size_t old_nbins,
                                 typeof(bins) new_bins, size_t new_nbins)
        {
            for (size_t i=0; i<old_nbins; ++i)
            {
                while (!old_bins[i].empty())
                {
                    auto* n = klist_front(old_bins[i],link);
                    old_bins[i].pop_front();
                    new_bins[compute_slot(n->k,new_nbins)].push_back(&n->link);
                }
            }
        }

        void grow()
        {
            ++nelems;
            if (nelems < 6*nbins)
                return;

            try
            {
                auto* old_bins = bins;
                bins           = alloc_bins(nbins*2);
                nbins         *= 2;
                bin_transfer(old_bins,nbins/2,bins,nbins);
                free_bins(old_bins,nbins/2);
            }
            catch (kernel::buddy_allocator_oom_exception&)
            {
            }
        }

        void shrink()
        {
            --nelems;
            if (nelems >= 2*nbins || nbins <= PAGE_SIZE/sizeof(bins[0]))
                return;

            try
            {
                auto* old_bins = bins;
                bins           = alloc_bins(nbins/2);
                nbins         /= 2;
                bin_transfer(old_bins,nbins*2,bins,nbins);
                free_bins(old_bins,nbins*2);
            }
            catch (kernel::buddy_allocator_oom_exception&)
            {
            }
        }

        void clear()
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
            nelems = 0;
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

        Value& append_to_slot(node* n, size_t slot)
        {
            bins[slot].push_back(&n->link);
            grow();
            return n->v;
        }

        Value& insert(const Key& k, const Value& v)
        {
            size_t slot = compute_slot(k,nbins);
            kernel::kassert(!find_node_in_slot(k,slot));
            return append_to_slot(node_slab.alloc<node>(k,v),slot);
        }

        template<typename ...Args>
        Value& emplace(const Key& k, Args&& ...args)
        {
            size_t slot = compute_slot(k,nbins);
            kernel::kassert(!find_node_in_slot(k,slot));
            return append_to_slot(
                node_slab.alloc<node>(dummy(),k,loki::forward<Args>(args)...),
                slot);
        }

        void erase(node* n)
        {
            shrink();
            n->link.unlink();
            node_slab.free(n);
        }

        void unlink(node* n)
        {
            // Unlink the node from the hash bucket, but don't delete the
            // slab element yet.  This could be useful for a client that
            // doesn't want the element to appear in the hash table anymore but
            // still exist for some period of time - say when a TCP socket
            // transitions to TCP_CLOSED but we aren't quite done with it yet.
            // Unlinking the socket would free up the TCP port number for
            // reuse since the hash slot would become available for a new
            // socket.
            n->link.unlink();
            unlinked_nodes.push_back(&n->link);
        }

        void erase_value(Value* v)
        {
            erase(container_of(v,node,v));
        }

        void unlink_value(Value* v)
        {
            unlink(container_of(v,node,v));
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
            clear();
            free_bins(bins,nbins);
        }
    };
}

#endif /* __KERNEL_HASH_TABLE_H */
