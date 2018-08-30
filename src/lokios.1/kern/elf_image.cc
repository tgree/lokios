#include "elf_image.h"
#include "kassert.h"

using kernel::_kassert;

elf64::image::image(const void* _ehdr):
    ehdr((const Elf64_Ehdr*)_ehdr),
    shdrs(ehdr,ehdr->e_shentsize,ehdr->e_shnum,ehdr->e_shoff),
    shstrtab(ehdr,&shdrs[ehdr->e_shstrndx])
{
    for (auto& sh : shdrs)
    {
        if (sh.sh_type != SHT_SYMTAB)
            continue;

        symtab.init(ehdr,sh.sh_entsize,sh.sh_size/sh.sh_entsize,sh.sh_offset,0);
        symstrtab.init(ehdr,&shdrs[sh.sh_link]);
        break;
    }
}

const void*
elf64::image::get_vaddr(size_t offset)
{
    return (const char*)ehdr + offset;
}

const Elf64_Phdr*
elf64::image::get_phdr(size_t i)
{
    kassert(i < ehdr->e_phnum);
    return (Elf64_Phdr*)get_vaddr(ehdr->e_phoff + ehdr->e_phentsize*i);
}

const char*
elf64::image::get_section_name(size_t i)
{
    if (ehdr->e_shstrndx == SHN_UNDEF)
        return NULL;

    auto& strtab = shdrs[ehdr->e_shstrndx];
    auto& shdr   = shdrs[i];
    kassert(shdr.sh_name < strtab.sh_size);
    return (const char*)get_vaddr(strtab.sh_offset + shdr.sh_name);
}

kernel::sym_info
elf64::image::get_sym_info(const void* addr)
{
    if (!symtab)
        throw kernel::symbol_not_found_exception();

    const Elf64_Sym* closest  = NULL;
    uint64_t closest_distance = 0xFFFFFFFFFFFFFFFFUL;
    size_t i = -1;
    for (auto& s : *symtab)
    {
        ++i;
        switch (ELF64_ST_TYPE(s.st_info))
        {
            case STT_NOTYPE:
            case STT_OBJECT:
            case STT_FUNC:
            break;

            default:
                continue;
        }
        if ((uint64_t)s.st_value > (uintptr_t)addr)
            continue;

        uint64_t distance = (uintptr_t)addr - s.st_value;
        if (distance > closest_distance)
            continue;

        closest          = &s;
        closest_distance = distance;
        if (closest_distance == 0)
            break;
    }

    if (!closest)
        throw kernel::symbol_not_found_exception();

    return kernel::sym_info{closest->st_value,
                            (uintptr_t)addr - closest->st_value,
                            closest->st_size,
                            symstrtab->get_string(closest->st_name),
                            NULL,
                            0,
                           };
}
