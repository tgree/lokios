#ifndef __KERNEL_ARGS_H
#define __KERNEL_ARGS_H

#include <stdint.h>

struct kernel_args
{
    uint64_t    e820_base;
    uint64_t    vga_base;
};

extern const kernel_args* kargs;

#endif /* __KERNEL_ARGS_H */
