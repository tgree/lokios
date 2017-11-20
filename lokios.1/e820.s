.code16
.text


# Decode INT 15h E820 memory map.
# On exit:
#   CF        - set if we failed, cleared if we succeeded
#   _e820_end - pointer past the last valid entry in the E820 array
#   _e820_map - array of 24-byte E820 entries
.globl _E820_get_list
_E820_get_list:
    lea     _e820_map, %di
    xor     %ebx, %ebx
    mov     $0x534D4150, %edx
    mov     $0x0000E820, %eax
    mov     $0x00000024, %ecx
    int     $0x15

    jc      .L_E820_error_exit
    cmp     $0x534D4150, %eax
    jne     .L_E820_error_exit
    cmp     $0, %ebx
    je      .L_E820_error_exit
    cmp     $24, %cl
    ja      .L_E820_error_exit

.L_E820_loop:
    add     $24, %di
    cmp     $_e820_last_max_entry, %di
    ja      .L_E820_error_exit
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
    movw    %di, _e820_end
    clc
    ret

.L_E820_error_exit:
    stc
    ret


# Dump the contents of the E820 list after it has been decoded.
.globl _E820_print_list
_E820_print_list:
    lea     .L_e820_dump_banner, %si
    call    _puts
    lea     _e820_map, %si
.L_E820_dump_loop:
    cmp     _e820_end, %si
    jae     .L_E820_dump_done
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
.L_e820_dump_banner:
    .asciz  "E820 memory map:\r\n"
