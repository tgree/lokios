#ifndef __KERNEL_ELF_IMAGE_H
#define __KERNEL_ELF_IMAGE_H

#include "symtab.h"
#include "console.h"
#include "k++/deferred_init.h"
#include <elf.h>

namespace elf64
{
    struct str_table_range_exception : public kernel::message_exception
    {
        str_table_range_exception():
            message_exception("String table index out of range.") {}
    };

    struct table_range_exception : public kernel::message_exception
    {
        table_range_exception():
            message_exception("Table index out of range.") {}
    };

    struct str_table
    {
        const char*     base;
        const size_t    len;

        const char* get_string(size_t offset) const
        {
            if (offset >= len)
            {
                kernel::console::printf("offset %zu >= table len %zu\n",
                                        offset,len);
                throw str_table_range_exception();
            }
            return base + offset;
        }

        constexpr str_table(const Elf64_Ehdr* ehdr, const Elf64_Shdr* shdr):
            base((const char*)ehdr + shdr->sh_offset),
            len(shdr->sh_size)
        {
        }
    };

    template<typename T>
    struct obj_table
    {
        const size_t    stride;
        const size_t    count;
        const T* const  _begin;
        const T* const  _end;

        struct iterator
        {
            const T*        obj;
            const size_t    stride;

            const T& operator*() const {return *obj;}

            iterator operator++()
            {
                obj = (const T*)((char*)obj + stride);
                return *this;
            }

            bool operator!=(const iterator& rhs) const
            {
                return obj != rhs.obj;
            }

            constexpr iterator(const T* obj, size_t stride):
                obj(obj),
                stride(stride)
            {
            }
        };

        inline const T& operator[](size_t n) const
        {
            if (n >= count)
                throw table_range_exception();
            return *(const T*)((uintptr_t)_begin + n*stride);
        }

        iterator begin() const {return iterator(_begin,stride);}
        iterator end() const   {return iterator(_end,stride);}

        constexpr obj_table(const Elf64_Ehdr* ehdr, size_t stride,
                            size_t count, size_t offset, size_t skip = 0):
            stride(stride),
            count(count - skip),
            _begin((const T*)((uintptr_t)ehdr + offset + skip*stride)),
            _end((const T*)((uintptr_t)_begin + stride*count))
        {
        }
    };

    struct image
    {
        const Elf64_Ehdr* const                     ehdr;
        const obj_table<Elf64_Shdr>                 shdrs;
        const str_table                             shstrtab;
        kernel::deferred_init<obj_table<Elf64_Sym>> symtab;
        kernel::deferred_init<str_table>            symstrtab;

        const void* get_vaddr(size_t offset);
        const Elf64_Phdr* get_phdr(size_t i);
        const char* get_section_name(size_t i);
        kernel::sym_info get_sym_info(const void* addr);

        image(const void* ehdr);
    };
}

#endif /* __KERNEL_ELF_IMAGE_H */
