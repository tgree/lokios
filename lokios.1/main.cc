#include "kernel_args.h"
#include "console.h"
#include "e820.h"

int
main()
{
    vga.printf("Loki is kickawesome\n");
    vga.printf("Hello %d '%4d' %05d '%-5d'",123,45,67,89);
}
