#include "mode32.h"
#include "serial.h"
#include "a20_gate.h"
#include "e820.h"
#include "mbr.h"
#include "pxe.h"
#include "console.h"
#include "massert.h"
#include "kernel/kernel_args.h"

struct kernel_header
{
    uint32_t    num_sectors;
    uint32_t    cr3;
};

extern uint8_t _kernel_base[];
extern uint8_t _kernel_stack[];
extern uint8_t _kernel_bsp_entry[];
extern uint8_t _kernel_ap_entry[];
extern uint8_t _kernel_params[];
extern uint8_t _e820_end[];

constexpr kernel_header* kernel_base = (kernel_header*)(uint32_t)_kernel_base;

int
m32_entry(uint32_t flags)
{
    console::init();

    m32_serial_puts("Enabling A20 line.\n");
    m32_assert(a20_enable() == 0);

    int err = -1;
    switch (flags & FLAG_BOOT_TYPE_MASK)
    {
        case FLAG_BOOT_TYPE_MBR:    err = m32_mbr_entry();  break;
        case FLAG_BOOT_TYPE_PXE:    err = m32_pxe_entry();  break;
    }
    if (err)
        return err;

    m32_serial_putx(kernel_base->num_sectors);
    m32_serial_putc('\n');
    m32_serial_putx(kernel_base->cr3);
    m32_serial_putc('\n');

    e820_map* m = (e820_map*)(uint32_t)_e820_end;
    m->nentries = 0;

    e820_io io;
    io.cookie = 0;
    for (m32_e820_iter(&io); io.cookie; m32_e820_iter(&io))
    {
        m->entries[m->nentries++] = io.entry;

        m32_assert(io.sig == 0x534D4150);
        m32_serial_putu(io.entry_len);
        m32_serial_putc(' ');
        m32_serial_putx(io.entry.addr >> 32);
        m32_serial_putc(':');
        m32_serial_putx(io.entry.addr >>  0);
        m32_serial_putc(' ');
        m32_serial_putx(io.entry.len >> 32);
        m32_serial_putc(':');
        m32_serial_putx(io.entry.len >>  0);
        m32_serial_putc(' ');
        m32_serial_putx(io.entry.type);
        if (io.entry_len > 20)
        {
            m32_serial_putc(' ');
            m32_serial_putx(io.entry.rsrv);
        }
        m32_serial_putc('\n');
    }

    auto* kargs      = (kernel::kernel_args*)(uint32_t)_kernel_params;
    kargs->e820_base = (dma_addr64)_e820_end;
    kargs->vga_base  = 0x000B8000;

    m32_long_jump(kernel_base->cr3,
                  0xFFFFFFFFC0000000ULL | (uint64_t)_kernel_bsp_entry,
                  0xFFFFFFFFC0000000ULL | (uint64_t)_kernel_stack
                  );
}

void
m32_smp_entry()
{
    m32_long_jump(kernel_base->cr3,
                  0xFFFFFFFFC0000000ULL | (uint64_t)_kernel_ap_entry,
                  0xFFFFFFFFC0000000ULL | (uint64_t)_kernel_stack
                  );
}
