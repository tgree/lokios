.code16
.text


# Hide the cursor.
.globl _hide_cursor
_hide_cursor:
    movb    $0x01, %ah
    movw    $0x2706, %cx
    int     $0x10
    ret


# Write a chracter to the console.
#   %al - contains the character to write
.globl _putc
_putc:
    movb    $0x0E, %ah
    movw    $0x0007, %bx
    int     $0x10
.L_handy_ret:
    ret


# Write a CRLF to the console.
.globl _putCRLF
_putCRLF:
    lea     .L_putCRLF_string, %si
# Write a null-terminated string to the console.
#   %si - contains the address of the string to write
# Stomps:
#   EAX, EBX
.globl _puts
_puts:
    lodsb
    cmp     $0, %al
    je      .L_handy_ret
    call    _putc
    jmp     _puts


# Write a hex value to the console
#   _put32: %edx - contains the value to write
#   _put16: %dx  - contains the value to write
#   _put8:  %dl  - contains the value to write
# Stomps:
#   EAX, EBX, ECX
.global _put32
.global _put16
.global _put8
_put32:
    mov     $8, %cx
    jmp     .L_put_loop
_put16:
    mov     $4, %cx
    rol     $16, %edx
    jmp     .L_put_loop
_put8:
    mov     $2, %cx
    rol     $24, %edx
.L_put_loop:
    rol     $4, %edx
    mov     %dl, %al
    and     $0x0F, %al
    add     $0x30, %al
    cmp     $0x3A, %al
    jl      .L_conv_complete
    add     $0x07, %al
.L_conv_complete:
    call    _putc
    loop    .L_put_loop
    ret


# Write an IPv4 address to the console.  For simplicity, we use hex.  If we run
# out of bootsector space, this can be moved out of the bootsector since only
# the PXE code prints these.
# On entry:
#   EDX - the IPv4 address
# Stomps:
#   EAX, EBX, ECX
.global _putipv4
_putipv4:
    mov     $3, %cx
.L_putipv4_loop:
    push    %cx
    call    _put8
    mov     $'.', %al
    call    _putc
    ror     $8, %edx
    pop     %cx
    loop    .L_putipv4_loop
    call    _put8
    ror     $8, %edx
    ret


# --------------------- Data Segment ---------------------
.data
.L_putCRLF_string:
    .asciz  "\r\n"
