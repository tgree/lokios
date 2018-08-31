#ifndef __KERNEL_HASH_H
#define __KERNEL_HASH_H

#include <stddef.h>
#include <string.h>
#include <stdint.h>

namespace hash
{
    // Anything trivially-convertible to a size_t can be used as a hash.
    template<typename Key>
    struct hasher
    {
        static inline size_t compute(const Key& k)
        {
            return size_t(k);
        }
    };

    // If it's a pointer to something, we'll assume natural alignment.  Here,
    // we're assuming that objects of type T won't be overlapping each other,
    // so the low-order address bits used to select a hash slot won't be very
    // unique.
    template<typename T>
    struct hasher<T*>
    {
        static inline size_t compute(const T* ptr)
        {
            return size_t(ptr)/sizeof(T);
        }
    };

    // A string hash function.  See:
    //  https://en.wikipedia.org/wiki/Jenkins_hash_function
    template<>
    struct hasher<const char*>
    {
        static inline size_t compute(const char* s)
        {
            size_t h = 0;
            while (*s)
            {
                h += *s++;
                h += (h << 10);
                h ^= (h >>  6);
            }
            h += (h <<  3);
            h ^= (h >> 11);
            h += (h << 15);
            return h;
        }
    };

    template<typename T>
    inline size_t compute(const T& v)
    {
        return hasher<T>::compute(v);
    }

    inline size_t compute(const char* v)
    {
        return hasher<const char*>::compute(v);
    }

    inline size_t compute(char* v)
    {
        return hash::compute((const char*)v);
    }

    template<typename Key>
    inline bool equals(const Key& lhs, const Key& rhs)
    {
        return lhs == rhs;
    }

    inline bool equals(const char* lhs, const char* rhs)
    {
        return !strcmp(lhs,rhs);
    }
}

#endif /* __KERNEL_HASH_H */
