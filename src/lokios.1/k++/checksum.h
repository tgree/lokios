#ifndef __KERNEL_CHECKSUM_H
#define __KERNEL_CHECKSUM_H

#include "kern/kassert.h"

namespace kernel
{
    template<typename T>
    T checksum(const void* _base, size_t len)
    {
        kassert((len % sizeof(T)) == 0);
        const T* p = (const T*)_base;
        T sum = 0;
        while (len--)
            sum += *p++;
        return sum;
    }
}

#endif /* __KERNEL_CHECKSUM_H */
