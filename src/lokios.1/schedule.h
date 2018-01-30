#ifndef __KERNEL_SCHEDULE_H
#define __KERNEL_SCHEDULE_H

namespace kernel
{
    struct tls_tcb;

    void schedule_loop();
    tls_tcb* schedule_tick();
}

#endif /* __KERNEL_SCHEDULE_H */
