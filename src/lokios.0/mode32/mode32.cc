#include "mode32.h"
#include "a20_gate.h"
#include "e820.h"
#include "mbr.h"
#include "pxe.h"
#include "console.h"
#include "raw_image.h"
#include "elf_image.h"
#include "massert.h"
#include "kernel/kernel_args.h"
#include <string.h>

extern uint8_t _elf_base[];
extern uint8_t _kernel_base[];
extern uint8_t _kernel_stack[];
extern uint8_t _kernel_bsp_entry[];
extern uint8_t _kernel_ap_entry[];
extern uint8_t _kernel_params[];
extern uint8_t _e820_end[];

extern "C" void _m32_long_jump(uint32_t cr3, uint64_t proc_addr,
                               uint64_t rsp) __NORETURN__;

int
m32_entry(uint32_t flags)
{
    console::init();

    console::printf("Enabling A20 line.\n");
    assert(a20_enable() == 0);

    // Nuke the ELF base in memory.
    memset(_elf_base,0,_kernel_base-_elf_base);

    // Find someone to read the image.
    image_stream* is;
    switch (flags & FLAG_BOOT_TYPE_MASK)
    {
        case FLAG_BOOT_TYPE_MBR: is = mbr_entry();  break;
        case FLAG_BOOT_TYPE_PXE: is = pxe_entry();  break;
    }
    if (!is)
        return -1;

    // Open the input stream.
    console::printf("Opening %s input stream.\n",is->name);
    int err = is->open();
    if (err)
    {
        console::printf("Error %d opening stream.\n",err);
        return err;
    }

    // Read the first sector into a local buffer.
    uint8_t buf[512];
    err = is->read(buf,1);
    if (err)
    {
        console::printf("Error %d reading first sector.\n",err);
        return err;
    }

    // Find someone to handle this.
    uintptr_t image_end;
    if (((kernel::image_header*)buf)->sig == IMAGE_HEADER_SIG)
    {
        err = process_raw_image(is,(kernel::image_header*)buf,&image_end);
        if (err)
        {
            console::printf("Error %d handling raw image.\n",err);
            return err;
        }
    }
    else if (*(uint32_t*)buf == ELF_HEADER_SIG)
    {
        err = process_elf_image(is,buf,&image_end);
        if (err)
        {
            console::printf("Error %d handling elf image.\n",err);
            return err;
        }
    }
    else
    {
        console::printf("Don't know how to handle this type of image.\n");
        return -6;
    }

    // Close the stream.
    err = is->close();
    if (err)
    {
        console::printf("Error %d closing input stream.\n",err);
        return err;
    }

    // Sanity on the header before we jump anywhere.
    auto* khdr = (kernel::image_header*)_kernel_base;
    if (khdr->sig != IMAGE_HEADER_SIG)
    {
        console::printf("Post-processing validation failed on header sig.\n");
        return -7;
    }

    // Sanity on the footer before we jump anywhere.
    auto* kftr =
        (kernel::image_footer*)((char*)khdr + 512*(khdr->num_sectors-1));
    if (kftr->sig != IMAGE_FOOTER_SIG)
    {
        console::printf("Post-processing validation failed on footer sig.\n");
        return -8;
    }

    // Initialize the E820 map.
    e820_map* m = (e820_map*)_e820_end;
    m->nentries = 0;

    // Fetch the E820 entries.
    e820_io io;
    io.cookie = 0;
    for (e820_iter(&io); io.cookie; e820_iter(&io))
    {
        m->entries[m->nentries++] = io.entry;

        assert(io.sig == 0x534D4150);
        console::printf("%u %016llX:%016llX 0x%08X",
                        io.entry_len,io.entry.addr,
                        io.entry.addr + io.entry.len - 1,io.entry.type);
        console::printf(" 0x%08X",io.entry.extended_attrs);
        console::printf("\n");
    }

    // Initialize the kernel args.
    auto* kargs       = (kernel::kernel_args*)_kernel_params;
    kargs->e820_base  = (dma_addr64)_e820_end;
    kargs->vga_base   = 0x000B8000;
    kargs->kernel_end = image_end;

    // Jump!
    _m32_long_jump(khdr->page_table_addr,
                   0xFFFFFFFFC0000000ULL | (uint64_t)_kernel_bsp_entry,
                   0xFFFFFFFFC0000000ULL | (uint64_t)_kernel_stack
                   );
}

void
m32_smp_entry()
{
    // Jump!
    auto* khdr = (kernel::image_header*)_kernel_base;
    _m32_long_jump(khdr->page_table_addr,
                   0xFFFFFFFFC0000000ULL | (uint64_t)_kernel_ap_entry,
                   0xFFFFFFFFC0000000ULL | (uint64_t)_kernel_stack
                   );
}
