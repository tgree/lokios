.code16
.text

# These links are helpful:
#
#   https://stackoverflow.com/q/980999
#   https://stackoverflow.com/a/33651438
#
# We are going to use vector 8 on page 0x8000, so they claim we will start in
# the following state:
#
#   CS:IP   - 0x800:0
#
# Much like a non-smp entry, we should probably canonicalize everything first.
.globl _smp_entry
_smp_entry:
    # Disable interrupts.
    cli

    # Clear CS
    ljmp    $0, $.L_start_clear_cs
.L_start_clear_cs:

    # Clear direction so string operations move forward in memory.
    cld

    # Load the gdt.
    lgdt    _m32_gdt_desc

    # Enter protected mode - we need this for the switch to long mode.
    call _mode_switch_protected_and_write_dot_code32_after_this_line
.code32

    # Do it.
    jmp     m32_smp_entry
