#include "kernel_args.h"
#include "console.h"
#include "kassert.h"

const kernel::kernel_args* kernel::kargs;

__thread uint64_t tls_signature = 0x135724683579468A;

int
main()
{
    kernel::vga->printf("Loki is kickawesome\n");
    kernel::kassert(tls_signature == 0x135724683579468A);

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
