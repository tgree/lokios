#ifndef __KERNEL_STRING_STREAM_H
#define __KERNEL_STRING_STREAM_H

#include "char_stream.h"
#include "hdr/kassert.h"
#include <stddef.h>

class tmock_test;

namespace kernel
{
    class string_stream : public char_stream<noop_lock>
    {
    protected:
        char*   base;
        char*   pos;
        size_t  len;

        virtual void _putc(char c)
        {
            if (avail())
            {
                *pos++ = c;
                *pos   = '\0';
            }
        }

    public:
        inline char* c_str() {return base;}
        inline const char* c_str() const {return base;}
        inline operator const char*() const {return c_str();}
        inline size_t strlen() const {return pos-base;}
        inline size_t avail() const {return len ? base+len-pos-1 : 0;}
        inline size_t capacity() const {return len;}
        inline bool empty() const {return strlen() == 0;}

        inline void shrink()
        {
            kassert(!empty());
            *--pos = '\0';
        }

        inline void clear()
        {
            pos = base;
            if (len)
                *pos = '\0';
        }

        inline string_stream(char* base, size_t len):
            base(base),pos(base),len(len)
        {
            if (len)
                *pos = '\0';
        }
        inline string_stream(char* base, size_t len, char* pos):
            base(base),pos(pos),len(len)
        {
        }

        template<size_t N>
        inline string_stream(char (&base)[N]):
            base(base),pos(base),len(N)
        {
            if (len)
                *pos = '\0';
        }

        friend class ::tmock_test;
    };

    template<size_t N>
    struct fixed_string_stream : public string_stream
    {
        char    storage[N];

        inline fixed_string_stream():string_stream(storage,N) {}
        inline fixed_string_stream(const char* fmt, ...):
            string_stream(storage,N)
        {
            va_list ap;
            va_start(ap,fmt);
            vprintf(fmt,ap);
            va_end(ap);
        }
    };

    template<size_t N>
    char __PRINTF__(2,3) (&ksprintf(char (&dst)[N], const char* fmt, ...))[N]
    {
        va_list ap;
        va_start(ap,fmt);
        string_stream(dst).vprintf(fmt,ap);
        va_end(ap);
        return dst;
    }
}

#endif /* __KERNEL_STRING_STREAM_H */
