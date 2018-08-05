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

    # Clear DS
    xor     %ax, %ax
    mov     %ax, %ds

    # Clear direction so string operations move forward in memory.
    cld

    # Enter unreal mode - we need this for the switch to long mode.
    lgdt    _m32_gdt_desc
    call    _enter_unreal_mode

    # Enter long mode and jump to the kernel.
    movl    $_kernel_ap_entry, %esi
    jmp     _enter_long_mode
