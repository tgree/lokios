#include "kernel_args.h"
#include "console.h"
#include "e820.h"
#include "assert.h"

extern uint8_t _tdata_begin[];
extern uint8_t _tdata_end[];
extern uint8_t _tbss_begin[];
extern uint8_t _tbss_end[];
extern uint8_t _tbss_size[];

int
main()
{
    vga.printf("Loki is kickawesome\n");
    vga.printf("_tdata_begin = %ld\n",(uintptr_t)_tdata_begin);
    vga.printf("_tdata_end   = %ld\n",(uintptr_t)_tdata_end);
    vga.printf("_tbss_begin  = %ld\n",(uintptr_t)_tbss_begin);
    vga.printf("_tbss_end    = %ld\n",(uintptr_t)_tbss_end);
    vga.printf("_tbss_size   = %ld\n",(uintptr_t)_tbss_size);

    try
    {
        throw &vga;
        aborts("throw failed");
    }
    catch (int)
    {
        aborts("caught int");
    }
    catch (console* c)
    {
        vga.printf("caught console*\n");
    }
    catch (...)
    {
        aborts("caught ...");
    }

    vga.printf("Kernel halting.\n");
}
