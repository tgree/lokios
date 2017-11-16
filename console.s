.code16
.text
.globl _putc
_putc:
    # Write a chracter to the console.
    #   %al - contains the character to write
    movb    $0x0E, %ah
    movb    $0x00, %bh
    movb    $0x07, %bl
    int     $0x10
    ret
