.code16
.text


# Dispatch table at address 0x7E10
            .word   .L_mbr_entry
            .word   .L_pxe_entry
_puts_ptr:  .word   0
_put16_ptr: .word   0
_put32_ptr: .word   0

_puts:      jmpw    *_puts_ptr
_put16:     jmpw    *_put16_ptr
_put32:     jmpw    *_put32_ptr

# Called from lokios.0.  On entry:
#   AX         - address of the lokios.0 _puts function
#   BX         - address of the lokios.0 _put16 function
#   CX         - address of the lokios.0 _put32 function
#   ES, DS, CS - all 0
#   SS:SP      - on a valid stack with base 0:0xFC00
#
# We also stored some values on the base of the stack:
#   0xFBF8     - contains original SS:SP (to get at !PXE struct)
#   0xFBFC     - contains original ES:BX (PXENV+ addr)
.L_mbr_entry:
.L_pxe_entry:
    mov     %ax, _puts_ptr
    mov     %bx, _put16_ptr
    mov     %cx, _put32_ptr

    lea     .L_lokios_1_banner, %si
    call    _puts
    ret


.data
.L_lokios_1_banner:
    .asciz  "lokios.1 entered\r\n"
