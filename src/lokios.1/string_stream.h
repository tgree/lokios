#ifndef __KERNEL_STRING_STREAM_H
#define __KERNEL_STRING_STREAM_H

#include "char_stream.h"
#include "kassert.h"
#include <stddef.h>

namespace kernel
{
    struct string_stream : public char_stream
    {
        char*       base;
        char*       pos;
        char* const end;

        inline operator const char*() const {return base;}

        virtual void _putc(char c)
        {
            if (pos < end-1)
            {
                *pos++ = c;
                *pos   = '\0';
            }
        }

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
