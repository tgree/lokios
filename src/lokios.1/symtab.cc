#include "symtab.h"
#include "console.h"

extern uint8_t _symtab_begin[];
extern uint8_t _symtab_end[];

void
kernel::init_symtab()
{
    kernel::console::printf(".symtab 0x%016lX len %lu\n",
                            (uintptr_t)_symtab_begin,
                            _symtab_end - _symtab_begin);
}
