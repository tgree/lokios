.code16
.text
.globl _start
_start:
    # Start by nuking the DS and ES registers.  These don't necessarily point
    # to the same region as the CS register or something, so without doing this
    # we would need to prefix all our memory accesses with %cs:.
    xor     %ax, %ax
    mov     %ax, %ds
    mov     %ax, %es

    # Print our banner.
    lea     _loki_os_banner, %si
    call    _puts

    mov     $0xAA, %dl
    call    _put8
    call    _putCRLF
    mov     $0xBBCC, %dx
    call    _put16
    call    _putCRLF
    mov     $0x12345678, %edx
    call    _put32
    call    _putCRLF

0:
    hlt
    jmp     0b

.data
.globl _loki_os_banner
_loki_os_banner:
    .ascii  "Loki OS\r\n"
    .asciz  "Copyright (c) 2017 by Terry Greeniaus.  All rights reserved.\r\n"
_crlf:
    .asciz  "\r\n"
