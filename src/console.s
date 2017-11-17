.code16
.text


# Write a chracter to the console.
#   %al - contains the character to write
.globl _putc
_putc:
    movb    $0x0E, %ah
    movb    $0x00, %bh
    movb    $0x07, %bl
    int     $0x10
    ret


# Write a null-terminated string to the console.
#   %si - contains the address of the string to write
.globl _puts
_puts:
    movb    (%si), %al
    cmp     $0, %al
    je      0f

    push    %si
    call    _putc
    pop     %si
    inc     %si
    jmp     _puts

0:
    ret


# Write a uint32_t to the console.
#   %edx - contains the value to write
.globl _put32
_put32:
    lea     .L_put32_val, %si
    lea     .L_hextab, %bx
    jmp     .L_put_8_digits

# Write a uint16_t to the console.
#   %dx - contains the value to write
.globl _put16
_put16:
    lea     .L_put16_val, %si
    lea     .L_hextab, %bx
    jmp     .L_put_4_digits

# Write a uint8_t to the console.
#   %dl - contains the value to write
.globl _put8
_put8:
    lea     .L_put8_val, %si
    lea     .L_hextab, %bx
    jmp     .L_put_2_digits

.L_put_8_digits:
    mov     %edx, %edi
    shr     $28, %edi
    and     $0x0F, %di
    movb    (%bx,%di), %al
    movb    %al, .L_put32_val+0

    mov     %edx, %edi
    shr     $24, %edi
    and     $0x0F, %di
    movb    (%bx,%di), %al
    movb    %al, .L_put32_val+1

    mov     %edx, %edi
    shr     $20, %edi
    and     $0x0F, %di
    movb    (%bx,%di), %al
    movb    %al, .L_put32_val+2

    mov     %edx, %edi
    shr     $16, %edi
    and     $0x0F, %di
    movb    (%bx,%di), %al
    movb    %al, .L_put32_val+3

.L_put_4_digits:
    mov     %dx, %di
    shr     $12, %di
    and     $0x0F, %di
    movb    (%bx,%di), %al
    movb    %al, .L_put16_val+0

    mov     %dx, %di
    shr     $8, %di
    and     $0x0F, %di
    movb    (%bx,%di), %al
    movb    %al, .L_put16_val+1

.L_put_2_digits:
    mov     %dx, %di
    shr     $4, %di
    and     $0x0F, %di
    movb    (%bx,%di), %al
    movb    %al, .L_put8_val+0

    mov     %dx, %di
    and     $0x0F, %di
    movb    (%bx,%di), %al
    movb    %al, .L_put8_val+1

    jmp     _puts


# Write a CRLF to the console.
.globl _putCRLF
_putCRLF:
    lea     .L_putCRLF_string, %si
    jmp     _puts


# --------------------- Data Segment ---------------------
.data
.L_hextab:
    .ascii  "0123456789ABCDEF"
.L_put32_val:
    .ascii  "0000"
.L_put16_val:
    .ascii  "00"
.L_put8_val:
    .asciz  "00"
.L_putCRLF_string:
    .asciz  "\r\n"
