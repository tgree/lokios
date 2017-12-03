#ifndef __KERNEL_VECTOR_H
#define __KERNEL_VECTOR_H

#include "page.h"
#include <utility>
#include <new>

namespace kernel
{
    template<typename T>
    struct vector
    {
        typedef T   value_type;
        typedef T*  iterator;

        page_raii               _page;
        T*                      _elems;
        size_t                  _size;
        static constexpr size_t _capacity = PAGE_SIZE/sizeof(T);

        inline size_t size() const     {return _size;}
        inline bool   empty() const    {return size() == 0;}
        inline size_t capacity() const {return _capacity;}
        inline bool   full() const     {return size() == capacity();}

        inline T& operator[](size_t index) const
        {
            kassert(index < size());
            return _elems[index];
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
            new((void*)mem) T(std::forward<T&&>(val));
            ++_size;
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
            new((void*)pos) T(std::forward<Args>(args)...);
            ++_size;
        }

        template<typename ...Args>
        T& emplace_back(Args&& ...args)
        {
            kassert(!full());
            T* t = &_elems[size()];
            t = new((void*)t) T(std::forward<Args>(args)...);
            ++_size;
            return *t;
        }

        void insert(const iterator pos, const T& val)
        {
            _make_slot(pos);
            new((void*)pos) T(val);
            ++_size;
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

        inline vector():
            _elems((T*)_page.addr),
            _size(0)
        {
        }

        inline vector(size_t size):
            _elems((T*)_page.addr),
            _size(0)
        {
            while (size--)
                emplace_back();
        }

        inline vector(vector&& other):
            _elems(other._elems),
            _size(other._size)
        {
            void* tmp        = _page.addr;
            _page.addr       = other._page.addr;
            other._page.addr = tmp;
            other._elems     = (T*)other._page.addr;
            other._size      = 0;
        }

        inline vector(const vector& other):
            _elems((T*)_page.addr),
            _size(0)
        {
            for (const T& e : other)
                push_back(e);
        }

        inline ~vector()
        {
            for (size_t i = size(); i != 0; --i)
                _elems[i-1].~T();
        }
    };
}

#endif /* __KERNEL_VECTOR_H */
