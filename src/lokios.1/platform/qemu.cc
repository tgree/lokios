#include "qemu.h"
#include "hdr/x86.h"

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

void
kernel::qemu_platform::_reboot()
{
    // QEMU supports the 0xCF9 register defined in the PIIX3 spec:
    //
    //  https://lists.gnu.org/archive/html/qemu-devel/2013-01/msg02324.html
    //
    // It doesn't seem to be documented anywhere in the QEMU man pages though.
    // I'm using the same sequence that the ACPI tables in my MBA say to use
    // and it seems to work fine under QEMU as well.
    outb(0x06,0xCF9);
}
