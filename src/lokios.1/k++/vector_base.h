#ifndef __KERNEL_VECTOR_BASE_H
#define __KERNEL_VECTOR_BASE_H

#include "allocator.h"
#include "kernel/kassert.h"
#include "hdr/types.h"
#include <stddef.h>
#include <unistd.h>
#include <new>

namespace kernel
{
    template<typename T, typename Allocator>
    struct vector_base
    {
        typedef T   value_type;
        typedef T*  iterator;

        Allocator               _allocator;
        raii_block<Allocator>   _block;
        T*                      _elems;
        size_t                  _size;
        const size_t            _capacity;

        inline size_t size() const     {return _size;}
        inline bool   empty() const    {return size() == 0;}
        inline size_t capacity() const {return _capacity;}
        inline bool   full() const     {return size() == capacity();}

        inline T& operator[](ssize_t index) const
        {
            if (index >= 0)
            {
                // The range [0, size-1] contains all elements.
                kassert(index < (ssize_t)size());
                return _elems[index];
            }
            else
            {
                // The range [-size, -1] contains all elements.
                kassert(-index <= (ssize_t)size());
                return _elems[size() + index];
            }
        }

        inline void push_back(const T& val)
        {
            kassert(!full());
            T* mem = &_elems[size()];
            new((void*)mem) T(val);
            ++_size;
        }

        inline void push_back(T&& val)
        {
            kassert(!full());
            T* mem = &_elems[size()];
            new((void*)mem) T(loki::forward<T&&>(val));
            ++_size;
        }

        inline void pop_back()
        {
            kassert(!empty());
            _elems[size() - 1].~T();
            --_size;
        }

        inline void _make_slot(const iterator pos)
        {
            kassert(!full());
            kassert(pos <= end());
            for (iterator i = end(); i != pos; --i)
            {
                new((void*)i) T(std::move(*(i-1)));
                (i-1)->~T();
            }
        }

        template<typename ...Args>
        void emplace(const iterator pos, Args&& ...args)
        {
            _make_slot(pos);
            new((void*)pos) T(loki::forward<Args>(args)...);
            ++_size;
        }

        template<typename ...Args>
        T& emplace_back(Args&& ...args)
        {
            kassert(!full());
            T* t = &_elems[size()];
            t = new((void*)t) T(loki::forward<Args>(args)...);
            ++_size;
            return *t;
        }

        iterator insert(const iterator pos, const T& val)
        {
            _make_slot(pos);
            new((void*)pos) T(val);
            ++_size;
            return pos;
        }

        iterator erase(iterator pos)
        {
            kassert(pos < end());
            pos->~T();
            for (iterator i = pos; i+1 != end(); ++i)
            {
                new((void*)i) T(*(i+1));
                (i+1)->~T();
            }
            --_size;
            return pos;
        }

        inline iterator begin()             {return _elems;}
        inline const iterator begin() const {return _elems;}
        inline iterator end()               {return _elems + _size;}
        inline const iterator end() const   {return _elems + _size;}

        inline vector_base(size_t size):
            _block(_allocator),
            _elems((T*)_block.addr),
            _size(0),
            _capacity(_allocator.len/sizeof(T))
        {
            while (size--)
                emplace_back();
        }

        inline vector_base():
            _block(_allocator),
            _elems((T*)_block.addr),
            _size(0),
            _capacity(_allocator.len/sizeof(T))
        {
        }

        inline vector_base(const vector_base& other):
            _block(_allocator),
            _elems((T*)_block.addr),
            _size(0),
            _capacity(_allocator.len/sizeof(T))
        {
            for (const T& e : other)
                push_back(e);
        }

        inline vector_base(vector_base&& other):
            _block(std::move(other._block)),
            _elems((T*)_block.addr),
            _size(other._size),
            _capacity(other._capacity)
        {
            kassert(_allocator.movable);
            other._elems    = (T*)other._block.addr;
            other._size     = 0;
        }

        inline ~vector_base()
        {
            for (size_t i = size(); i != 0; --i)
                _elems[i-1].~T();
        }
    };
}

#endif /* __KERNEL_VECTOR_BASE_H */
