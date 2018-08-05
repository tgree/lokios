.code16
.text


# Write a null-terminated string to the console.
#   %si - contains the address of the string to write
# Stomps:
#   EAX, EBX
.globl _puts
_puts:
    lodsb
    cmp     $0, %al
    je      .L_handy_ret
    movb    $0x0E, %ah
    movw    $0x0007, %bx
    int     $0x10
    jmp     _puts
.L_handy_ret:
    ret
