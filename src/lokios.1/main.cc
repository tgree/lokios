#include "kernel_args.h"
#include "console.h"
#include "page_table.h"
#include "x86.h"
#include "kassert.h"

extern uint8_t _tdata_begin[];
extern uint8_t _tdata_end[];
extern uint8_t _tbss_begin[];
extern uint8_t _tbss_end[];
extern uint8_t _tbss_size[];

const kernel::kernel_args* kernel::kargs;

int
main()
{
    kernel::vga->printf("Loki is kickawesome\n");
    kernel::vga->printf("_tdata_begin = 0x%016lX\n",(uintptr_t)_tdata_begin);
    kernel::vga->printf("_tdata_end   = 0x%016lX\n",(uintptr_t)_tdata_end);
    kernel::vga->printf("_tbss_begin  = 0x%016lX\n",(uintptr_t)_tbss_begin);
    kernel::vga->printf("_tbss_end    = 0x%016lX\n",(uintptr_t)_tbss_end);
    kernel::vga->printf("_tbss_size   = 0x%016lX\n",(uintptr_t)_tbss_size);

    kernel::page_table pt;
    for (auto e : kernel::page_table_leaf_iterator((uint64_t*)mfcr3()))
        pt.map_page(e.vaddr,e.get_paddr(),e.get_len(),e.pte);
    pt.activate();

    try
    {
        throw kernel::vga;
        kernel::panic("throw failed");
    }
    catch (int)
    {
        kernel::panic("caught int");
    }
    catch (kernel::console* c)
    {
        kernel::vga->printf("caught console*\n");
    }
    catch (...)
    {
        kernel::panic("caught ...");
    }

    kernel::halt();
}
