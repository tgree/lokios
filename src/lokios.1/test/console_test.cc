#include "../vga.h"
#include "../kernel_args.h"
#include "../kassert.h"
#include "tmock/tmock.h"

static uint16_t conbuf[80*25];
static const kernel::kernel_args args = {0,(uintptr_t)conbuf};
const kernel::kernel_args* kernel::kargs = &args;

void
kernel::panic(const char* s) noexcept
{
    tmock::abort(s);
}

class tmock_test
{
    TMOCK_TEST(test_screen_initialized)
    {
        for (uint16_t v : conbuf)
            tmock::assert_equiv(v,0x1F20U);
    }

    TMOCK_TEST(test_putc)
    {
        tmock::assert_equiv(conbuf[0],0x1F20U);
        kernel::console::_putc('T');
        tmock::assert_equiv(conbuf[0],0x1F00U | 'T');
    }
};

int
main(int argc, const char* argv[])
{
    kernel::init_vga_console();
    return tmock::run_tests(argc,argv);
}
