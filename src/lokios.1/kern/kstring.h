#ifndef __KERNEL_KSTRING_H
#define __KERNEL_KSTRING_H

#include "k++/string_stream.h"
#include "k++/strings.h"
#include "mm/buddy_allocator.h"
#include "hdr/chartype.h"

namespace kernel
{
    class string : public string_stream
    {
        void grow(size_t n)
        {
            if (len)
            {
                size_t old_order = buddy_order_for_len(len);
                size_t new_order = buddy_order_for_len(strlen() + n + 1);
                char* p          = (char*)buddy_alloc(new_order);
                kassert(p);
                memcpy(p,base,strlen()+1);
                buddy_free(base,old_order);

                pos  = p + (pos - base);
                base = p;
                len  = (PAGE_SIZE << new_order);
            }
            else
            {
                size_t order = buddy_order_for_len(n + 1);
                base = pos = (char*)buddy_alloc(order);
                len = (PAGE_SIZE << order);
            }
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
            string_stream((char*)buddy_alloc(0),PAGE_SIZE)
        {
            va_list ap;
            va_start(ap,fmt);
            vprintf(fmt,ap);
            va_end(ap);
        }
        inline ~string()
        {
            if (len)
            {
                size_t order = buddy_order_for_len(len);
                buddy_free(base,order);
            }
        }

        friend class ::tmock_test;
    };
}

#endif /* __KERNEL_KSTRING_H */
