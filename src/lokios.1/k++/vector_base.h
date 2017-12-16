#ifndef __KERNEL_VECTOR_BASE_H
#define __KERNEL_VECTOR_BASE_H

#include "kernel/kassert.h"
#include <stddef.h>
#include <unistd.h>
#include <utility>
#include <new>

namespace kernel
{
    template<typename T>
    struct vector_base
    {
        typedef T   value_type;
        typedef T*  iterator;

        T*              _elems;
        size_t          _size;
        const size_t    _capacity;

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
            new((void*)mem) T(std::forward<T&&>(val));
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

        constexpr vector_base(void* _elems, size_t _capacity):
            _elems((T*)_elems),
            _size(0),
            _capacity(_capacity)
        {
        }

        inline vector_base(void* _elems, size_t _capacity, size_t size):
            _elems((T*)_elems),
            _size(0),
            _capacity(_capacity)
        {
            while (size--)
                emplace_back();
        }

        inline vector_base(void* _elems, size_t _capacity,
            const vector_base& other):
                _elems((T*)_elems),
                _size(0),
                _capacity(_capacity)
        {
            for (const T& e : other)
                push_back(e);
        }

        inline ~vector_base()
        {
            for (size_t i = size(); i != 0; --i)
                _elems[i-1].~T();
        }
    };
}

#endif /* __KERNEL_VECTOR_BASE_H */
