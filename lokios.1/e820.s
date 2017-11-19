.code16
.text


# Decode INT 15h E820 memory map and store it at _heap_start.
# On entry:
#   ES:DI - address in which to store the list
# On exit:
#   ES:DI - points to just past the end of the list
.globl _E820_get_list
_E820_get_list:
    xor     %ebx, %ebx
    mov     $0x534D4150, %edx
    mov     $0x0000E820, %eax
    mov     $0x00000024, %ecx
    int     $0x15

    jc      .L_E820_unsupported
    cmp     $0x534D4150, %eax
    jne     .L_E820_unsupported
    cmp     $0, %ebx
    je      .L_E820_unsupported
    cmp     $24, %cl
    jg      .L_E820_unsupported

.L_E820_loop:
    add     $24, %di
    cmp     $(_heap_end - 24), %di
    jg      .L_E820_out_of_memory
    mov     $0x0000E820, %eax
    mov     $0x00000024, %ecx
    int     $0x15
    jc      .L_E820_done_carry_set
    cmp     $0, %ebx
    je      .L_E820_done_ebx_zero
    jmp     .L_E820_loop
.L_E820_done_ebx_zero:
    add     $24, %di
.L_E820_done_carry_set:
    movw    $0x4E4F, 16(%di)
    movw    $0x4E4F, 18(%di)
    clc
    ret

.L_E820_unsupported:
    lea     .L_e820_unsupported_text, %si
    jmp     .L_E820_error_exit
.L_E820_out_of_memory:
    lea     .L_e820_oom_text, %si
.L_E820_error_exit:
    call    _puts
    stc
    ret


# Dump the contents of the E820 list after it has been decoded.
# On entry:
#   SI - start of table
.globl _E820_print_list
_E820_print_list:
    push    %si
    lea     .L_e820_dump_banner, %si
    call    _puts
    pop     %si
.L_E820_dump_loop:
    cmpw    $0x4E4F, 16(%si)
    je      .L_E820_dump_done
    mov     4(%si), %edx
    call    _put32
    mov     0(%si), %edx
    call    _put32
    mov     $' ', %al
    call    _putc
    mov     12(%si), %edx
    call    _put32
    mov     8(%si), %edx
    call    _put32
    mov     $' ', %al
    call    _putc
    mov     16(%si), %edx
    call    _put32
    mov     $'\r', %al
    call    _putc
    mov     $'\n', %al
    call    _putc
    add     $24, %si
    jmp     .L_E820_dump_loop
.L_E820_dump_done:
    ret


.data
.L_e820_unsupported_text:
    .asciz  "INT15h E820 call not supported\r\n"
.L_e820_oom_text:
    .asciz  "INT15h E820 used all available heap\r\n"
.L_e820_dump_banner:
    .asciz  "E820 memory map:\r\n"
