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
    for (auto e : kernel::page_table_iterator((uint64_t*)mfcr3()))
    {
        switch (e.len)
        {
            case 0x00200000:
                pt.map_2m_page(e.vaddr,e.paddr,e.page_flags,e.cache_flags);
            break;

            case 0x00001000:
                pt.map_4k_page(e.vaddr,e.paddr,e.page_flags,e.cache_flags);
            break;

            default:
                kernel::panic("Unsupported page size!");
            break;
        }
    }
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
