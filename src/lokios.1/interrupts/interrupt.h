#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H

#include <stdint.h>

namespace kernel
{
    struct tls_tcb;

    // List of predefined interrupt numbers.
#define INTN_LAPIC_SPURIOUS 127
#define INTN_INT126_TEST    126
#define INTN_LAPIC_SELFTEST 125
#define INTN_LAPIC_10MS     124
#define INTN_LAPIC_WAKEUP   123
#define INTN_LAPIC_TLB      122

    typedef tls_tcb* (*interrupt_handler)(uint64_t selector,
                                          uint64_t error_code);

    void register_handler(uint64_t selector, interrupt_handler handler);
    void unregister_handler(uint64_t selector);

    const char* get_exception_name(uint8_t vec_num);

    void init_interrupts();
    void init_cpu_interrupts();
    void init_cpu_device_interrupts();
}

#endif /* __KERNEL_INTERRUPT_H */
