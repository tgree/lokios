.text
.globl _start
_start:
    movb    $0x0E, %ah
    movb    $'L',  %al
    movb    $0x00, %bh
    movb    $0x07, %bl
    int     $0x10

    movb    $0x0E, %ah
    movb    $'o',  %al
    movb    $0x00, %bh
    movb    $0x07, %bl
    int     $0x10

    movb    $0x0E, %ah
    movb    $'k',  %al
    movb    $0x00, %bh
    movb    $0x07, %bl
    int     $0x10

    movb    $0x0E, %ah
    movb    $'i',  %al
    movb    $0x00, %bh
    movb    $0x07, %bl
    int     $0x10

    movb    $0x0E, %ah
    movb    $'O',  %al
    movb    $0x00, %bh
    movb    $0x07, %bl
    int     $0x10

    movb    $0x0E, %ah
    movb    $'S',  %al
    movb    $0x00, %bh
    movb    $0x07, %bl
    int     $0x10

_forever:
    jmp     _forever

.data
    .ascii  "Loki OS!\n"
    .align  4096
