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
        inline operator const char*() const {return base;}
        inline size_t strlen() const {return pos-base;}
        inline size_t avail() const {return base+len-pos-1;}

        inline void clear()
        {
            pos  = base;
            *pos = '\0';
        }

        inline string_stream(char* base, size_t len):
            base(base),pos(base),len(len)
        {
            kassert(len > 0);
            *pos = '\0';
        }

        friend class ::tmock_test;
    };

    template<size_t N>
    struct fixed_string_stream : public string_stream
    {
        char    storage[N];

        inline fixed_string_stream():string_stream(storage,N) {}
    };
}

#endif /* __KERNEL_STRING_STREAM_H */
