#ifndef __KERNEL_ASSERT_H
#define __KERNEL_ASSERT_H

#include <stdlib.h>

void
aborts(const char* s) noexcept __attribute__((noreturn));

#define ASSERT(exp) static_assert(exp, #exp)

#endif /* __KERNEL_ASSERT_H */
