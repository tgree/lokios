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
    mov     %eax, _putc_ptr
    mov     %ebx, _put16_ptr

    lea     .L_lokios_1_banner, %si
    call    _puts
    lea     _heap_start+2, %di
    call    _E820_get_list
    mov     %si, _heap_start
    jc      .L_exit_to_bios
    lea     _heap_start+2, %si
    call    _E820_print_list

.L_exit_to_bios:
    ret


.data
.L_lokios_1_banner:
    .asciz  "lokios.1 entered\r\n"
