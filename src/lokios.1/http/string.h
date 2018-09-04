#ifndef __KERNEL_HTTP_STRING_H
#define __KERNEL_HTTP_STRING_H

#include "k++/hash.h"
#include "kern/kassert.h"

namespace http
{
    template<size_t MaxLen, typename TooLongException>
    struct string
    {
        char        val[MaxLen+1];
        uint16_t    len = 0;

        inline operator const char*() const {return val;}
        inline const char* c_str() const {return val;}
        inline size_t size() const {return len;}

        inline void clear()
        {
            len = 0;
        }

        inline void append(char c)
        {
            if (len == MaxLen)
                throw TooLongException();
            val[len++] = c;
            val[len]   = '\0';
        }

        inline void shrink(uint16_t n)
        {
            kernel::kassert(n <= len);
            len     -= n;
            val[len] = '\0';
        }

        inline char& operator[](int32_t pos)
        {
            if (pos >= 0)
            {
                kernel::kassert(pos < len);
                return val[pos];
            }
            else
            {
                kernel::kassert(-pos <= len);
                return val[len + pos];
            }
        }

        inline string() = default;
        inline string(const char* k):
            len(strlen(k))
        {
            if (len > MaxLen)
                throw TooLongException();
            strcpy(val,k);
        }
    };
}

namespace hash
{
    template<size_t MaxLen, typename TooLongException>
    struct hasher<http::string<MaxLen,TooLongException>>
    {
        static inline size_t compute(
            const http::string<MaxLen,TooLongException>& hs)
        {
            return hash::hasher<const char*>::compute(hs.val);
        }
    };
}

#endif /* __KERNEL_HTTP_STRING_H */
