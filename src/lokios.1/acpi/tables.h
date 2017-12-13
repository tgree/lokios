#ifndef __KERNEL_ACPI_TABLES_H
#define __KERNEL_ACPI_TABLES_H

#include "hdr/compiler.h"
#include "kernel/kassert.h"
#include "k++/vector.h"
#include <stdint.h>

namespace kernel
{
    struct e820_map;

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

#define MCFG_SIG 0x4746434DU
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
    };
    KASSERT(sizeof(sdt_header) == 36);

    extern vector<const sdt_header*> acpi_sdts;

    const sdt_header* find_acpi_table(uint32_t signature);
    void init_acpi_tables(const e820_map* m);
}

#endif /* __KERNEL_ACPI_TABLES_H */
