#include "../vga.h"
#include "../kassert.h"
#include "../mm/mm.h"
#include "tmock/tmock.h"

static uint16_t conbuf[80*25];

void
kernel::panic(const char* s) noexcept
{
    tmock::abort(s);
}

void*
kernel::phys_to_virt(dma_addr64 p)
{
    return (void*)p;
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
    kernel::init_vga_console((dma_addr64)conbuf);
    return tmock::run_tests(argc,argv);
}
