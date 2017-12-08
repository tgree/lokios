#include "kernel_args.h"
#include "console.h"
#include "kassert.h"

const kernel::kernel_args* kernel::kargs;

int
main()
{
    kernel::vga->printf("Loki is kickawesome\n");

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
}
