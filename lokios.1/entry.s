.code16
.text


# Called from lokios.0.  On entry:
#   EAX   - address of the lokios.0 _puts function
#   SS:SP - on a valid stack with base 0xFE00
.globl _start
_start:
    lea     .L_lokios_1_banner, %si
    call    *%ax
    hlt
    ret


.data
.L_lokios_1_banner:
    .asciz  "lokios.1 entered"
