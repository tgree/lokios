# On entry:
#   RDI - address of the kernel_args struct
_entry:
    call    main
    hlt
