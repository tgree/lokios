#ifndef __KERNEL_STRINGS_H
#define __KERNEL_STRINGS_H

#include "hdr/chartype.h"

namespace kernel
{
    struct not_a_number_exception {};

    template<typename T>
    T str_to(const char* p)
    {
        if (*p == '\0')
            throw not_a_number_exception();

        T val = 0;
        while (*p)
        {
            if (!kisdigit(*p))
                throw not_a_number_exception();
            val *= 10;
            val += *p - '0';
            ++p;
        }
        return val;
    }
}

#endif /* __KERNEL_STRINGS_H */
