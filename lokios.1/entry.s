# On entry:
#   RDI - address of the kernel_args struct
_entry:
    # Save off kargs.
    mov     %rdi, kargs

    # Clear the TLS BSS.
    mov     $_tbss_size, %ecx
    test    %ecx, %ecx
    je      .L_zero_loop_done
    mov     $-1, %rbx
    xor     %rax, %rax
.L_zero_loop:
    movb    %al, %fs:(%rbx)
    dec     %rbx
    loop    .L_zero_loop
.L_zero_loop_done:

    # Copy in the TLS TDATA.
    lea     _tdata_begin, %esi
    mov     $_tdata_size, %rcx
    test    %rcx, %rcx
    je      .L_copy_loop_done
    xor     %rbx, %rbx
    sub     $_tbss_size, %rbx
    sub     %rcx, %rbx # rbx = -_tbss_size - _tdata_size
.L_copy_loop:
    movb    (%esi), %al
    movb    %al, %fs:(%rbx)
    inc     %esi
    inc     %rbx
    loop    .L_copy_loop
.L_copy_loop_done:

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
