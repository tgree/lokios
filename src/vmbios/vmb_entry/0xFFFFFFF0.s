.code16
.text

.globl _entry
.globl _vmb_main
_entry:
    # We assume that the ROM will be mapped at 0xFFFF0000 and at 0x000F0000 and
    # branch straight to the start of 0x000F0000 copy.
    ljmp $0xF000, $0x0000
