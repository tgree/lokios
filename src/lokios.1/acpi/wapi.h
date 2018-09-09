#ifndef __KERNEL_ACPI_WAPI_H
#define __KERNEL_ACPI_WAPI_H

#include "tables.h"
#include "wapi/wapi.h"

namespace kernel
{
    extern wapi::global_node acpi_node;

    void init_acpi_wapi();
}

#endif /* __KERNEL_ACPI_WAPI_H */
