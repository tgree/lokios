.code16
.text
.globl _start
_start:
    movb    $'L',  %al
    call    _putc
    movb    $'o',  %al
    call    _putc
    movb    $'k',  %al
    call    _putc
    movb    $'i',  %al
    call    _putc
    movb    $'O',  %al
    call    _putc
    movb    $'S',  %al
    call    _putc

_forever:
    jmp     _forever

.data
    .ascii  "Loki OS!\n"
    .align  4096
