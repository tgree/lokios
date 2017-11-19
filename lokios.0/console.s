.code16
.text


# Write a chracter to the console.
#   %al - contains the character to write
.globl _putc
_putc:
    movb    $0x0E, %ah
    movw    $0x0007, %bx
    int     $0x10
    ret


# Write a null-terminated string to the console.
#   %si - contains the address of the string to write
.globl _puts
_puts:
    lodsb
    cmp     $0, %al
    je      0f

    call    _putc
    jmp     _puts

0:
    ret


# Write a hex value to the console
#   _put32: %edx - contains the value to write
#   _put16: %dx  - contains the value to write
.global _put32
.global _put16
_put32:
    mov     $8, %cx
    jmp     .L_put_loop
_put16:
    mov     $4, %cx
    rol     $16, %edx
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


# Write a CRLF to the console.
.globl _putCRLF
_putCRLF:
    lea     .L_putCRLF_string, %si
    jmp     _puts


# --------------------- Data Segment ---------------------
.data
.L_putCRLF_string:
    .asciz  "\r\n"
