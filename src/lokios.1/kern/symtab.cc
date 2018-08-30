#include "symtab.h"
#include "elf_image.h"
#include "console.h"
#include "k++/deferred_init.h"

using kernel::console::printf;

extern uint8_t _virt_elf_header[];
static kernel::deferred_init<elf64::image> elf;

kernel::sym_info
kernel::get_sym_info(void* addr)
{
    if (!elf || !elf->symtab)
        throw symbol_not_found_exception();
    return elf->get_sym_info(addr);
}

void
kernel::init_symtab()
{
    if (*(uint32_t*)_virt_elf_header == 0x464C457F)
        elf.init(_virt_elf_header);
}
