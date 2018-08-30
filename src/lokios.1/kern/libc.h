#ifndef __KERNEL_LIBC_H
#define __KERNEL_LIBC_H

#include <stddef.h>

extern "C"
{
    void* malloc(size_t size) noexcept;
    void free(void* p) noexcept;
    void abort(void) noexcept;
}

#endif /* __KERNEL_LIBC_H */
