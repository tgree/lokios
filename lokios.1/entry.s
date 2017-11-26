# On entry:
#   RDI - address of the kernel_args struct
_entry:
    # Save off kargs.
    mov     %rdi, kargs

    # Call the _init stuff.
    call    init

    # Call main.
    call    main

    # STOP.
    hlt

.data
.globl kargs
kargs:
    .long   0
