#include "qemu.h"
#include "kernel/x86.h"

kernel::qemu_platform::qemu_platform():
    kernel::platform("QEMU")
{
}

void
kernel::qemu_platform::_exit_guest(int status)
{
    // When invoking qemu with the option "-device isa-debug-exit", we get a
    // special device at port 0x501 that can be used to terminate qemu with a
    // given exit code.  The qemu exit status will be (status << 1) | 1, so we
    // can't actually get it to exit with a 0 exit code, but this is still
    // pretty good for scripting.
    outb(status,0x501);
}