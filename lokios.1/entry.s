.code16
.text


# Dispatch table at address 0x7E10
.word   .L_mbr_entry
.word   .L_pxe_entry

# Called from lokios.0.  On entry:
#   EAX   - address of the lokios.0 _puts function
#   SS:SP - on a valid stack with base 0xFE00
.L_mbr_entry:
.L_pxe_entry:
    lea     .L_lokios_1_banner, %si
    call    *%ax
    ret


.data
.L_lokios_1_banner:
    .asciz  "lokios.1 entered\r\n"
