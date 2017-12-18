#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H

#include <stdint.h>

namespace kernel
{
    struct tls_tcb;

    typedef tls_tcb* (*interrupt_handler)(uint64_t selector,
                                          uint64_t error_code);

    void register_handler(uint64_t selector, interrupt_handler handler);
    void unregister_handler(uint64_t selector);

    void init_interrupts();
}

#endif /* __KERNEL_INTERRUPT_H */
