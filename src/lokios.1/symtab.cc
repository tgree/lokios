#include "symtab.h"

kernel::sym_info
kernel::get_sym_info(void* addr)
{
    throw symbol_not_found_exception();
}

void
kernel::init_symtab()
{
}
