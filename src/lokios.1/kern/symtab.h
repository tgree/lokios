#ifndef __KERNEL_SYMTAB_H
#define __KERNEL_SYMTAB_H

#include "cxx_exception.h"
#include <stdint.h>
#include <stddef.h>

namespace kernel
{
    struct sym_info
    {
        uintptr_t   addr;
        size_t      offset;
        size_t      size;
        const char* name;
        const char* file;
        size_t      line;
    };

    struct symbol_not_found_exception : public message_exception
    {
        constexpr symbol_not_found_exception():
            message_exception("Symbol not found.") {}
    };

    sym_info get_sym_info(void* addr);

    void init_symtab();
}

#endif /* __KERNEL_SYMTAB_H */
