.code16
.text


# Called from one of the PXE or MBR entry points.  On entry:
#   ES, DS, CS - all 0
#   SS:SP      - on a valid stack with base 0:0xFC00
.globl _common_entry
_common_entry:
    # Parse the memory map with E820.
    call    _E820_get_list
    jc      .L_E820_get_list_failed
    call    _E820_print_list

    # Enable the A20 gate so we can use all of RAM.
    call    _A20_enable
    jc      .L_A20_enable_failed

    # Enter long mode - point of no return so we jmp there.
    jmp     _enter_long_mode

.L_E820_get_list_failed:
    lea     .L_E820_failure_text, %si
    call    _puts
    jmp     .L_exit_to_bios
.L_A20_enable_failed:
    lea     .L_A20_failure_text, %si
    call    _puts
    jmp     .L_exit_to_bios
.L_exit_to_bios:
    ret


.data
.L_E820_failure_text:
    .asciz  "E820 memory probe failed.\r\n"
.L_A20_failure_text:
    .asciz  "A20 gate enable failed.\r\n"
