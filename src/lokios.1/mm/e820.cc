#include "e820.h"
#include "k++/string_stream.h"

static const char* e820_type_map[] = {
    "Unknown (0)",
    "RAM",
    "Reserved",
    "ACPI Reclaimable",
    "ACPI NVS",
    "Bad RAM",
};

const char*
kernel::get_e820_type_string(e820_type t, char (&buf)[22])
{
    if (t >= NELEMS(e820_type_map))
        return ksprintf(buf,"Unknown (%u)",t);
    return e820_type_map[t];
}
