#include "kernel_args.h"
#include "console.h"
#include "e820.h"
#include "assert.h"

int
main()
{
    vga.printf("Loki is kickawesome\n");

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
