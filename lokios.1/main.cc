#include "kernel_args.h"
#include "console.h"
#include "e820.h"

int
main()
{
    console_init();
    console_puts("Loki is kickawesome");
}
