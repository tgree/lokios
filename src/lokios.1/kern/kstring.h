#ifndef __KERNEL_KSTRING_H
#define __KERNEL_KSTRING_H

#include "k++/string_stream.h"
#include "k++/strings.h"
#include "mm/buddy_allocator.h"
#include "hdr/chartype.h"

#define MIN_KSTRING_CAPACITY    4096UL

namespace kernel
{
    class string : public string_stream
    {
        void grow(size_t n)
        {
            size_t new_strsize = strlen() + n + 1;
            size_t new_len = ceil_pow2(MAX(MIN_KSTRING_CAPACITY,new_strsize));

            if (len)
            {
                char* p = (char*)buddy_alloc_by_len(new_len);
                kassert(p);
                memcpy(p,base,strlen()+1);
                buddy_free_by_len(base,len);

                pos  = p + (pos - base);
                base = p;
            }
            else
                base = pos = (char*)buddy_alloc_by_len(new_len);

            len = new_len;
        }

        virtual void _putc(char c)
        {
            if (!avail())
                grow(1);
            string_stream::_putc(c);
        }

    public:
        size_t avail_after(char* p) const
        {
            kassert(base <= p && p <= pos);
            return pos - p;
        }

        void append(const char* s, size_t n)
        {
            if (n > avail())
                grow(n);
            memcpy(pos,s,n);
            pos += n;
            *pos = '\0';
        }
        inline void append(const char* s)
        {
            append(s,::strlen(s));
        }

        inline void operator+=(const char* s)
        {
            append(s);
        }
        inline void operator+=(const string_stream& ss)
        {
            append(ss,ss.strlen());
        }

        bool ends_with(const char* s) const
        {
            size_t s_len = ::strlen(s);
            if (s_len > strlen())
                return false;

            const char* s1 = s + s_len - 1;
            const char* s2 = pos - 1;
            while (s_len--)
            {
                if (*s1-- != *s2--)
                    return false;
            }
            return true;
        }

        char* begin() const {return base;}
        char* end() const {return pos;}
        char* find(char c, char* p) const
        {
            kassert(base <= p && p <= pos);
            while (p != pos && *p != c)
                ++p;
            return p;
        }
        char* find(char c) const
        {
            return find(c,base);
        }

        uint64_t to_u64() const
        {
            const char* p = base;
            if (p == pos)
                throw not_a_number_exception();

            uint64_t val = 0;
            while (p != pos)
            {
                if (!kisdigit(*p))
                    throw not_a_number_exception();
                val *= 10;
                val += *p - '0';
                ++p;
            }
            return val;
        }

        inline string():
            string_stream(NULL,0)
        {
        }
        inline string(const char* fmt, ...):
            string_stream((char*)buddy_alloc_by_len(MIN_KSTRING_CAPACITY),
                          MIN_KSTRING_CAPACITY)
        {
            va_list ap;
            va_start(ap,fmt);
            vprintf(fmt,ap);
            va_end(ap);
        }
        inline ~string()
        {
            if (len)
                buddy_free_by_len(base,len);
        }

        friend class ::tmock_test;
    };
}

#endif /* __KERNEL_KSTRING_H */
