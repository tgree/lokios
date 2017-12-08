.section .init
    popq    %rbp
    ret

.section .fini
    popq    %rbp
    ret


.section .note.GNU-stack,"",%progbits
