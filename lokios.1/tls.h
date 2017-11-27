#ifndef __KERNEL_TLS_H
#define __KERNEL_TLS_H

#include "kassert.h"
#include <stdint.h>
#include <stddef.h>

struct tls_tcb
{
    tls_tcb*    self;
    uint64_t    rsrv[4];
    uint64_t    stack_guard;    // gcc has this hard-coded to offset 0x28
};
KASSERT(offsetof(tls_tcb, stack_guard) == 0x28);

#endif /* __KERNEL_TLS_H */
