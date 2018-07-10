#ifndef __KERNEL_STRING_STREAM_H
#define __KERNEL_STRING_STREAM_H

#include "char_stream.h"
#include "kernel/kassert.h"
#include <stddef.h>

namespace kernel
{
    class string_stream : public char_stream<noop_lock>
    {
        char*       base;
        char*       pos;
        char* const end;

        virtual void _putc(char c)
        {
            if (pos < end-1)
            {
                *pos++ = c;
                *pos   = '\0';
            }
        }

    public:
        inline operator const char*() const {return base;}
        inline size_t strlen() {return pos-base;}

        inline string_stream(char* base, size_t len):
            base(base),pos(base),end(base+len)
        {
            kassert(len > 0);
            *pos = '\0';
        }
    };

    template<size_t N>
    struct fixed_string_stream : public string_stream
    {
        char    storage[N];

        inline fixed_string_stream():string_stream(storage,N) {}
    };
}

#endif /* __KERNEL_STRING_STREAM_H */
