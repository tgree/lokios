# On entry:
#   RDI - address of the kernel_args struct
_entry:
    # Populate kernel::args.
    mov     %rdi, _ZN6kernel5kargsE

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

    # Enable SSE since the stupid compiler is generating %xmm register
    # references and I can't turn it off because then stdint.h doesn't work.
    mov     %cr0, %rax
    and     $0xFFFB, %ax
    or      $2, %ax
    mov     %rax, %cr0
    mov     %cr4, %rax
    or      $(3 << 9), %ax
    mov     %rax, %cr4

    # Call the _init stuff.
    call    init

    # Call main.
    call    main

    # STOP.
    hlt
