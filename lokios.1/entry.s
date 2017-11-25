# On entry:
#   RDI - address of the kernel_args struct
_entry:
    mov     %rdi, kargs
    call    init
    call    main
    hlt

.data
.globl kargs
kargs:
    .long   0
