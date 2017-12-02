#ifndef __KERNEL_LOCAL_VECTOR_H
#define __KERNEL_LOCAL_VECTOR_H

#include "kassert.h"
#include <stddef.h>

namespace kernel
{
    template<typename T>
    struct local_vector
    {
        typedef T   value_type;
        typedef T*  iterator;

        T*              _elems;
        size_t          _size;
        const size_t    _capacity;

                  size_t    size() const     {return _size;}
                  bool      empty() const    {return size() == 0;}
        constexpr size_t    capacity() const {return _capacity;}
                  bool      full() const     {return size() == capacity();}

        inline T& operator[](size_t index)
        {
            kassert(index < size());
            return _elems[index];
        }

        inline void push_back(const T& val)
        {
            kassert(!full());
            _elems[_size++] = val;
        }

        inline void insert(iterator pos, const T& val)
        {
            kassert(!full());
            kassert(pos <= end());
            for (iterator i = end(); i != pos; --i)
                *i = *(i-1);
            *pos = val;
            ++_size;
        }

        inline iterator erase(iterator at)
        {
            kassert(at < end());
            for (iterator i = at; i + 1 != end(); ++i)
                *i = *(i+1);
            --_size;
            return at;
        }

        inline iterator begin()             {return _elems;}
        inline const iterator begin() const {return _elems;}
        inline iterator end()               {return _elems + _size;}
        inline const iterator end() const   {return _elems + _size;}

        inline local_vector(T* _elems, size_t _capacity, size_t _size=0):
            _elems(_elems),_size(_size),_capacity(_capacity)
        {
        }
    };
}

#endif /* __KERNEL_LOCAL_VECTOR_H */
