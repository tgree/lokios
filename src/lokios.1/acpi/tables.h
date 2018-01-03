#ifndef __KERNEL_ACPI_TABLES_H
#define __KERNEL_ACPI_TABLES_H

#include "hdr/compiler.h"
#include "kernel/kassert.h"
#include "k++/vector.h"
#include <stdint.h>

namespace kernel
{
    struct e820_map;

    struct gen_addr
    {
        uint8_t     addr_space_id;
        uint8_t     register_bit_width;
        uint8_t     register_bit_offset;
        uint8_t     access_size;
        uint64_t    addr;
    } __PACKED__;
    KASSERT(sizeof(gen_addr) == 12);

#define RDSP_SIG 0x2052545020445352UL
    struct rsdp_table
    {
        uint64_t    signature;
        uint8_t     checksum;
        char        oem_id[6];
        uint8_t     revision;
        uint32_t    rsdt_base;
    } __PACKED__;
    KASSERT(sizeof(rsdp_table) == 20);

    struct rsdp_table_2
    {
        uint64_t    signature;
        uint8_t     checksum;
        char        oem_id[6];
        uint8_t     revision;
        uint32_t    rsdt_base;

        uint32_t    length;
        uint64_t    xsdt_base;
        uint8_t     xchecksum;
        uint8_t     rsrv[3];
    } __PACKED__;
    KASSERT(sizeof(rsdp_table_2) == 36);

    struct sdt_header
    {
        uint32_t    signature;
        uint32_t    length;
        uint8_t     revision;
        uint8_t     checksum;
        char        oem_id[6];
        char        oem_table_id[8];
        uint32_t    oem_revision;
        uint32_t    creator_id;
        uint32_t    creator_revision;
    } __PACKED__;
    KASSERT(sizeof(sdt_header) == 36);

#define FADT_SIG 0x50434146
    struct fadt_table
    {
        sdt_header  hdr;
        uint32_t    firmware_ctrl;
        uint32_t    dsdt;
        uint8_t     rsrv1;
        uint8_t     preferred_pm_profile;
        uint16_t    sci_int;
        uint32_t    sci_cmd;
        uint8_t     acpi_enable;
        uint8_t     acpi_disable;
        uint8_t     s4bios_req;
        uint8_t     pstate_cnt;
        uint32_t    pm1a_evt_blk;
        uint32_t    pm1b_evt_blk;
        uint32_t    pm1a_cnt_blk;
        uint32_t    pm1b_cnt_blk;
        uint32_t    pm2_cnt_blk;
        uint32_t    pm_tmr_blk;
        uint32_t    gpe0_blk;
        uint32_t    gpe1_blk;
        uint8_t     pm1_evt_len;
        uint8_t     pm1_cnt_len;
        uint8_t     pm2_cnt_len;
        uint8_t     pm_tmr_len;
        uint8_t     gpe0_blk_len;
        uint8_t     gpe1_blk_len;
        uint8_t     gpe1_base;
        uint8_t     cst_cnt;
        uint16_t    p_lvl2_lat;
        uint16_t    p_lvl3_lat;
        uint16_t    flush_size;
        uint16_t    flush_stride;
        uint8_t     duty_offset;
        uint8_t     duty_width;
        uint8_t     day_alrm;
        uint8_t     mon_alrm;
        uint8_t     century;
        uint16_t    iapc_boot_arch;
        uint8_t     rsrv2;
        uint32_t    flags;
        gen_addr    reset_reg;
        uint8_t     reset_value;
        uint8_t     rsrv3[3];
        uint64_t    x_firmware_ctrl;
        uint64_t    x_dsdt;
        gen_addr    x_pm1a_evt_blk;
        gen_addr    x_pm1b_evt_blk;
        gen_addr    x_pm1a_cnt_blk;
        gen_addr    x_pm1b_cnt_blk;
        gen_addr    x_pm2_cnt_blk;
        gen_addr    x_pm_tmr_blk;
        gen_addr    x_gpe0_blk;
        gen_addr    x_gpe1_blk;
        gen_addr    sleep_control_reg;
        gen_addr    sleep_status_reg;
    } __PACKED__;
    KASSERT(sizeof(fadt_table) == 268);

#define MCFG_SIG 0x4746434DU
    struct mcfg_entry
    {
        uint64_t    addr;
        uint16_t    domain;
        uint8_t     start_bus_num;
        uint8_t     end_bus_num;
        uint32_t    rsrv;
    } __PACKED__;
    KASSERT(sizeof(mcfg_entry) == 16);

    struct mcfg_table
    {
        sdt_header  hdr;
        uint8_t     rsrv[8];
        mcfg_entry  entries[];
    } __PACKED__;
    KASSERT(sizeof(mcfg_table) == 44);

#define MADT_SIG 0x43495041
    enum madt_record_type : uint8_t
    {
        MADT_TYPE_LAPIC                         = 0,
        MADT_TYPE_IOAPIC                        = 1,
        MADT_TYPE_INTERRUPT_OVERRIDE            = 2,
        MADT_TYPE_NMI_SOURCE                    = 3,
        MADT_TYPE_LAPIC_NMI                     = 4,
        MADT_TYPE_LAPIC_ADDRESS_OVERRIDE        = 5,
        MADT_TYPE_IOSAPIC                       = 6,
        MADT_TYPE_LSAPIC                        = 7,
        MADT_TYPE_PLATFORM_INTERRUPT_SOURCES    = 8,
        MADT_TYPE_Lx2APIC                       = 9,
        MADT_TYPE_Lx2APIC_NMI                   = 10,
        MADT_TYPE_GIC                           = 11,
        MADT_TYPE_GICD                          = 12,
    };

    struct madt_record
    {
        madt_record_type    type;
        uint8_t             len;

        union
        {
            struct
            {
                uint8_t     acpi_processor_id;
                uint8_t     apic_id;
                uint32_t    flags;
            } __PACKED__ type0;

            struct
            {
                uint8_t     io_apic_id;
                uint8_t     rsrv;
                uint32_t    io_apic_addr;
                uint32_t    interrupt_base;
            } __PACKED__ type1;

            struct
            {
                uint8_t     bus_source;
                uint8_t     irq_source;
                uint32_t    interrupt_num;
                uint16_t    flags;
            } __PACKED__ type2;

            struct
            {
                uint8_t     processor;
                uint16_t    flags;
                uint8_t     lint_num;
            } __PACKED__ type4;

            struct
            {
                uint16_t    rsrv;
                uint64_t    local_apic_addr;
            } __PACKED__ type5;
        };
    } __PACKED__;

    struct madt_iterator
    {
        const madt_record* r;

        inline const madt_record& operator*() const
        {
            return *r;
        }

        inline void operator++()
        {
            r = (const madt_record*)((uintptr_t)r + r->len);
        }

        inline bool operator!=(const madt_iterator& end) const
        {
            if (r >= end.r)
                return false;
            if ((uintptr_t)r + offsetof(madt_record,len) +
                    sizeof(r->len) > (uintptr_t)end.r)
            {
                return false;
            }
            return (uintptr_t)r + r->len <= (uintptr_t)end.r;
        }

        constexpr madt_iterator(const madt_record* r):r(r) {}
    };

    struct madt_table
    {
        sdt_header  hdr;
        uint32_t    local_apic_addr;
        uint32_t    flags;
        madt_record records[];

        inline madt_iterator begin() const
        {
            return madt_iterator(records);
        }

        inline madt_iterator end() const
        {
            return madt_iterator((madt_record*)((uintptr_t)&hdr + hdr.length));
        }
    };

    extern vector<const sdt_header*> acpi_sdts;

    const sdt_header* find_acpi_table(uint32_t signature);
    void init_acpi_tables(const e820_map* m);
}

#endif /* __KERNEL_ACPI_TABLES_H */
