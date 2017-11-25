#include "kernel_args.h"
#include "console.h"
#include "e820.h"

const kernel_args* kargs;

int
main(const kernel_args* _kargs)
{
    kargs = _kargs;

    console_init();
    console_puts("Loki is kickawesome");
}
