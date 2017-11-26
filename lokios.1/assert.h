#ifndef __KERNEL_ASSERT_H
#define __KERNEL_ASSERT_H

#include <stdlib.h>

void
aborts(const char* s);

#define ASSERT(exp) static_assert(exp, #exp)

#endif /* __KERNEL_ASSERT_H */
