.code16
.text


# Dispatch table at address 0x7E10
.word   .L_mbr_entry
.word   .L_pxe_entry

# Called from lokios.0.  On entry:
#   EAX        - address of the lokios.0 _puts function
#   ES, DS, CS - all 0
#   SS:SP      - on a valid stack with base 0:0xFC00
#
# We also stored some values on the base of the stack:
#   0xFBF8     - contains original SS:SP (to get at !PXE struct)
#   0xFBFC     - contains original ES:BX (PXENV+ addr)
.L_mbr_entry:
.L_pxe_entry:
    lea     .L_lokios_1_banner, %si
    call    *%ax
    ret


.data
.L_lokios_1_banner:
    .asciz  "lokios.1 entered\r\n"
