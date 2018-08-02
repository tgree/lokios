#include "symtab.h"
#include "console.h"
#include "image.h"

void
kernel::init_symtab()
{
    kernel::console::printf(".elf 0x%016lX len %lu\n",
                            (uintptr_t)elf_begin,
                            elf_end - elf_begin);
}
