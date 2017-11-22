.code16
.text


# Dispatch table at address 0x7E10
            .word   .L_mbr_entry
            .word   .L_pxe_entry
_putc_ptr:  .word   0
_puts_ptr:  .word   0
_put16_ptr: .word   0
_put32_ptr: .word   0

# Called from lokios.0.  On entry:
#   EAX        - _puts | _putc
#   EBX        - _put32 | _put16
#   ES, DS, CS - all 0
#   SS:SP      - on a valid stack with base 0:0xFC00
#
# We also stored some values on the base of the stack:
#   0xFBF8     - contains original SS:SP (to get at !PXE struct)
#   0xFBFC     - contains original ES:BX (PXENV+ addr)
.L_mbr_entry:
.L_pxe_entry:
    # Save the function pointers.
    mov     %eax, _putc_ptr
    mov     %ebx, _put16_ptr

    # Print the second-stage banner.
    lea     .L_lokios_1_banner, %si
    call    _puts

    # Parse the memory map with E820.
    call    _E820_get_list
    jc      .L_E820_get_list_failed
    call    _E820_print_list

    # Enter unreal mode.  This will allow FS- and GS-based offsets to addresses
    # within the first 32-bits of memory.
    call    _enter_unreal_mode

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
.L_lokios_1_banner:
    .asciz  "lokios.1 entered\r\n"
.L_E820_failure_text:
    .asciz  "E820 memory probe failed.\r\n"
.L_A20_failure_text:
    .asciz  "A20 gate enable failed.\r\n"
