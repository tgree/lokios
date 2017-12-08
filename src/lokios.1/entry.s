# On entry:
#   RDI - address of the kernel_args struct
_entry:
    # Populate kernel::args.
    mov     %rdi, _ZN6kernel5kargsE

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
    jmp     init


# On entry:
#   rdi - thread* to activate
.globl _thread_jump
_thread_jump:
    # Load MSR_FS_BASE with the thread's FS value.
    mov     $0xC0000100, %ecx
    lea     26624(%rdi), %rax
    mov     %rax, %rdx
    shr     $32, %rdx
    wrmsr

    # Load the stack and push RIP onto the stack.
    mov     %fs:56, %rax
    mov     %fs:64, %rsp
    push    %rax

    # Load everything else.
    mov     %fs:72, %rax
    mov     %fs:80, %rbx
    mov     %fs:88, %rcx
    mov     %fs:96, %rdx
    mov     %fs:104, %rdi
    mov     %fs:112, %rsi
    mov     %fs:120, %rbp
    mov     %fs:128, %r8
    mov     %fs:136, %r9
    mov     %fs:144, %r10
    mov     %fs:152, %r11
    mov     %fs:160, %r12
    mov     %fs:168, %r13
    mov     %fs:176, %r14
    mov     %fs:184, %r15

    # Pop the RIP off the stack and return.
    ret


.section .note.GNU-stack,"",%progbits
