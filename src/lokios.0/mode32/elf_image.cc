#include "elf_image.h"
#include "console.h"
#include "kernel/image.h"
#include "k++/kmath.h"
#include <elf.h>
#include <string.h>

using kernel::max;
using kernel::round_up_pow2;

extern uint8_t _elf_base[];
extern uint8_t _kernel_base[];
extern uint8_t _kernel_bsp_entry[];

int
process_elf_image(image_stream* is, void* sector0, uintptr_t* image_end)
{
    console::printf("Processing elf kernel image.\n");

    // Copy the first sector into place.
    auto* ehdr = (Elf64_Ehdr*)_elf_base;
    memcpy(ehdr,sector0,512);

    // Validate it.
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        console::printf("ELF image isn't 64-bit.\n");
        return -1;
    }
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        console::printf("ELF image isn't little-endian two's complement.\n");
        return -2;
    }
    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT)
    {
        console::printf("ELF ident image verion %u doesn't match current "
                        "version %u.\n",ehdr->e_ident[EI_VERSION],EV_CURRENT);
        return -3;
    }
    if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    {
        console::printf("ELF OS ABI %u isn't SYSV %u.\n",
                        ehdr->e_ident[EI_OSABI],ELFOSABI_SYSV);
        return -4;
    }
    if (ehdr->e_version != EV_CURRENT)
    {
        console::printf("ELF image verion %u doesn't match current version "
                        "%u.\n",ehdr->e_ident[EI_VERSION],EV_CURRENT);
        return -5;
    }
    if (ehdr->e_type != ET_EXEC)
    {
        console::printf("ELF image not an executable.\n");
        return -6;
    }
    if (ehdr->e_machine != EM_X86_64)
    {
        console::printf("ELF image not x86-64.\n");
        return -7;
    }
    if ((ehdr->e_entry & 0x3FFFFFFF) != (uint32_t)_kernel_bsp_entry)
    {
        console::printf("ELF image has bad entry point.\n");
        return -8;
    }

    // Find a lower bound on the file length by finding the ending offsets of
    // all the section and program header tables.
    uint64_t pos      = 512;
    uint64_t ehdr_end = ehdr->e_ehsize;
    uint64_t ph_end   = ehdr->e_phoff + ehdr->e_phentsize*ehdr->e_phnum;
    uint64_t sh_end   = ehdr->e_shoff + ehdr->e_shentsize*ehdr->e_shnum;
    uint64_t limit    = max(ehdr_end,max(ph_end,sh_end));
    limit             = round_up_pow2(limit,512);
    console::printf("Headers limit: 0x%08llX\n",limit);

    // Read in enough to cover all the tables.
    uint32_t nsectors = (limit - pos)/512;
    int err = is->read((char*)ehdr + pos,nsectors);
    if (err)
    {
        console::printf("Error %d reading %u sectors from position 0x%08llX\n",
                        err,nsectors,pos);
        return -8;
    }
    pos = limit;

    // Iterate over all the program headers looking for the highest limit.
    auto* phdr = (Elf64_Phdr*)((char*)ehdr + ehdr->e_phoff);
    for (size_t i=0; i<ehdr->e_phnum; ++i)
    {
        if (phdr->p_filesz < phdr->p_memsz)
        {
            console::printf("Program header %u had filesize %llu smaller than "
                            "memsize %llu.\n",i,phdr->p_filesz,phdr->p_memsz);
            return -9;
        }
        limit = max(limit,phdr->p_offset + phdr->p_filesz);
    }

    // Iterate over all the section headers looking for the highest limit.
    auto* shdr = (Elf64_Shdr*)((char*)ehdr + ehdr->e_shoff);
    for (size_t i=0; i<ehdr->e_shnum; ++i)
    {
        limit = max(limit,shdr->sh_offset + shdr->sh_size);
        shdr  = (Elf64_Shdr*)((char*)shdr + ehdr->e_shentsize);
    }

    // Read the rest of the sectors.
    limit = round_up_pow2(limit,512);
    console::printf("Final limit: 0x%08llX\n",limit);
    nsectors = (limit - pos)/512;
    err = is->read((char*)ehdr + pos,nsectors);
    if (err)
    {
        console::printf("Error %d reading %u sectors from position 0x%08llX\n",
                        err,nsectors,pos);
        return -9;
    }
    pos = limit;

    // Set the image end.
    *image_end = (uintptr_t)ehdr + pos;

    // Looks like success.
    return 0;
}
