.code16
.text


# Dispatch table at address 0x7E10
            .word   .L_mbr_entry
            .word   .L_pxe_entry
_putc_ptr:  .word   0
_puts_ptr:  .word   0
_put16_ptr: .word   0
_put32_ptr: .word   0

.globl _putc
.globl _puts
.globl _put16
.globl _put32
_putc:      jmpw    *_putc_ptr
_puts:      jmpw    *_puts_ptr
_put16:     jmpw    *_put16_ptr
_put32:     jmpw    *_put32_ptr

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
    lea     _heap_start+2, %di
    call    _E820_get_list
    jc      .L_E820_get_list_failed
    mov     %si, _heap_start
    lea     _heap_start+2, %si
    call    _E820_print_list

    # Enable the A20 gate so we can use all of RAM.
    call    _A20_enable
    jc      .L_A20_enable_failed

    # Success.
    lea     .L_lokios_os_ready, %si
    call    _puts
    jmp     .L_exit_to_bios

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
.L_lokios_os_ready:
    .asciz  "Ready to enter long mode.\r\n"
