#ifndef __KERNEL_LAPIC_H
#define __KERNEL_LAPIC_H

#include "kernel/types.h"
#include "kernel/kassert.h"

namespace kernel
{
    typedef fat_register<16,uint32_t> lapic_register;

    struct lapic_registers
    {
        uint8_t         rsrv[32];                   // 0x000
        lapic_register  apic_id;                    // 0x020
        lapic_register  apic_version;               // 0x030
        uint8_t         rsrv2[64];                  // 0x040
        lapic_register  task_priority;              // 0x080
        lapic_register  arbitration_priority;       // 0x090
        lapic_register  processor_priority;         // 0x0A0
        lapic_register  eoi;                        // 0x0B0
        lapic_register  remote_read;                // 0x0C0
        lapic_register  logical_destination;        // 0x0D0
        lapic_register  destination_format;         // 0x0E0
        lapic_register  spurious_interrupt_vector;  // 0x0F0
        lapic_register  isr[8];                     // 0x100
        lapic_register  tmr[8];                     // 0x180
        lapic_register  irr[8];                     // 0x200
        lapic_register  error_status;               // 0x280
        uint8_t         rsrv3[96];                  // 0x290
        lapic_register  lvt_cmci;                   // 0x2F0
        lapic_register  icr[2];                     // 0x300
        lapic_register  lvt_timer;                  // 0x320
        lapic_register  lvt_thermal_sensor;         // 0x330
        lapic_register  lvt_perfmon_counters;       // 0x340
        lapic_register  lvt_lint0;                  // 0x350
        lapic_register  lvt_lint1;                  // 0x360
        lapic_register  lvt_error;                  // 0x370
        lapic_register  initial_count;              // 0x380
        lapic_register  current_count;              // 0x390
        uint8_t         rsrv4[64];                  // 0x3A0
        lapic_register  divide_configuration;       // 0x3E0
        uint8_t         rsrv5[16];                  // 0x3F0
    };
    KASSERT(sizeof(lapic_registers) == 0x400);

    void test_lapic();
    void init_lapic();
}

#endif /* __KERNEL_LAPIC_H */
