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
    # Set up a garbage interrupt frame on the stack and jump to the interrupt-
    # handler exit code that will restore the state out of the thread context
    # we are trying to jump into.
    lea     26624(%rdi), %rax
    sub     $40, %rsp
    jmp     .L_interrupt_exit


# On entry:
#   We are running on the ISR stack.
#   We need to save everything!
.globl _interrupt_entry_no_error_code
.globl _interrupt_entry_with_error_code
_interrupt_entry_no_error_code:
    sub     $8, %rsp
_interrupt_entry_with_error_code:
    # Save the general-purpose registers.
    mov     %rax, %fs:72
    mov     %rbx, %fs:80
    mov     %rcx, %fs:88
    mov     %rdx, %fs:96
    mov     %rdi, %fs:104
    mov     %rsi, %fs:112
    mov     %rbp, %fs:120
    mov     %r8,  %fs:128
    mov     %r9,  %fs:136
    mov     %r10, %fs:144
    mov     %r11, %fs:152
    mov     %r12, %fs:160
    mov     %r13, %fs:168
    mov     %r14, %fs:176
    mov     %r15, %fs:184

    # Save the registers that were pushed on the stack.
    mov     8(%rsp), %rsi
    mov     16(%rsp), %ax
    mov     24(%rsp), %rbx
    mov     32(%rsp), %rcx
    mov     40(%rsp), %dx
    mov     %rsi, %fs:56
    mov     %ax,  %fs:32
    mov     %rbx, %fs:24
    mov     %rcx, %fs:64
    mov     %dx,  %fs:34

    # Load a vector number, the error code and then call the interrupt handler.
    # The interrupt handler returns the new FS value in RAX.
    mov     $32, %rdi
    pop     %rsi
    call    interrupt_entry

    # Load the new FS.
.L_interrupt_exit:
    mov     $0xC0000100, %ecx
    mov     %rax, %rdx
    shr     $32, %rdx
    wrmsr

    # Update the stack values that will be used in iretq with the new thread.
    xor     %rdx, %rdx
    xor     %rax, %rax
    mov     %fs:56, %rsi
    mov     %fs:32, %dx
    mov     %fs:24, %rbx
    mov     %fs:64, %rcx
    mov     %fs:34, %ax
    mov     %rsi, 0(%rsp)
    mov     %rdx, 8(%rsp)
    mov     %rbx, 16(%rsp)
    mov     %rcx, 24(%rsp)
    mov     %rax, 32(%rsp)

    # Restore the general-purpose registers.
    mov     %fs:72,  %rax 
    mov     %fs:80,  %rbx 
    mov     %fs:88,  %rcx 
    mov     %fs:96,  %rdx 
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

    # We're done!
    iretq


.section .note.GNU-stack,"",%progbits
