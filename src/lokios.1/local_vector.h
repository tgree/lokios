#ifndef __KERNEL_LOCAL_VECTOR_H
#define __KERNEL_LOCAL_VECTOR_H

#include "kassert.h"

namespace kernel
{
    template<typename T>
    struct local_vector
    {
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

        inline void insert_at_index(const T& val, size_t index)
        {
            kassert(!full());
            kassert(index < size() + 1);
            for (size_t i=size(); i != index; --i)
                _elems[i] = _elems[i-1];
            _elems[index] = val;
            ++_size;
        }

        inline void push_back(const T& val)
        {
            kassert(!full());
            _elems[_size++] = val;
        }

        inline T* begin()             {return _elems;}
        inline const T* begin() const {return _elems;}
        inline T* end()               {return _elems + _size;}
        inline const T* end() const   {return _elems + _size;}

        inline local_vector(T* _elems, size_t _capacity, size_t _size=0):
            _elems(_elems),_size(_size),_capacity(_capacity)
        {
        }
    };
}

#endif /* __KERNEL_LOCAL_VECTOR_H */
