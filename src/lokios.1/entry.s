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
.macro _define_interrupt num
.global _interrupt_entry_\num
_interrupt_entry_\num :
    push    $0      # Dummy error code
    push    $\num
    jmp     _interrupt_entry_common
.endm

.macro _define_interrupt_errcode num
.global _interrupt_entry_\num
_interrupt_entry_\num :
    push    $\num
    jmp     _interrupt_entry_common
.endm

_define_interrupt           0
_define_interrupt           1
_define_interrupt           2
_define_interrupt           3
_define_interrupt           4
_define_interrupt           5
_define_interrupt           6
_define_interrupt           7
_define_interrupt_errcode   8
_define_interrupt           9
_define_interrupt_errcode   10
_define_interrupt_errcode   11
_define_interrupt_errcode   12
_define_interrupt_errcode   13
_define_interrupt_errcode   14
_define_interrupt           15
_define_interrupt           16
_define_interrupt           17
_define_interrupt           18
_define_interrupt           19
_define_interrupt           20
_define_interrupt           21
_define_interrupt           22
_define_interrupt           23
_define_interrupt           24
_define_interrupt           25
_define_interrupt           26
_define_interrupt           27
_define_interrupt           28
_define_interrupt           29
_define_interrupt           30
_define_interrupt           31
_define_interrupt           32
_define_interrupt           33
_define_interrupt           34
_define_interrupt           35
_define_interrupt           36
_define_interrupt           37
_define_interrupt           38
_define_interrupt           39
_define_interrupt           40
_define_interrupt           41
_define_interrupt           42
_define_interrupt           43
_define_interrupt           44
_define_interrupt           45
_define_interrupt           46
_define_interrupt           47
_define_interrupt           48
_define_interrupt           49
_define_interrupt           50
_define_interrupt           51
_define_interrupt           52
_define_interrupt           53
_define_interrupt           54
_define_interrupt           55
_define_interrupt           56
_define_interrupt           57
_define_interrupt           58
_define_interrupt           59
_define_interrupt           60
_define_interrupt           61
_define_interrupt           62
_define_interrupt           63
_define_interrupt           64
_define_interrupt           65
_define_interrupt           66
_define_interrupt           67
_define_interrupt           68
_define_interrupt           69
_define_interrupt           70
_define_interrupt           71
_define_interrupt           72
_define_interrupt           73
_define_interrupt           74
_define_interrupt           75
_define_interrupt           76
_define_interrupt           77
_define_interrupt           78
_define_interrupt           79
_define_interrupt           80
_define_interrupt           81
_define_interrupt           82
_define_interrupt           83
_define_interrupt           84
_define_interrupt           85
_define_interrupt           86
_define_interrupt           87
_define_interrupt           88
_define_interrupt           89
_define_interrupt           90
_define_interrupt           91
_define_interrupt           92
_define_interrupt           93
_define_interrupt           94
_define_interrupt           95
_define_interrupt           96
_define_interrupt           97
_define_interrupt           98
_define_interrupt           99
_define_interrupt           100
_define_interrupt           101
_define_interrupt           102
_define_interrupt           103
_define_interrupt           104
_define_interrupt           105
_define_interrupt           106
_define_interrupt           107
_define_interrupt           108
_define_interrupt           109
_define_interrupt           110
_define_interrupt           111
_define_interrupt           112
_define_interrupt           113
_define_interrupt           114
_define_interrupt           115
_define_interrupt           116
_define_interrupt           117
_define_interrupt           118
_define_interrupt           119
_define_interrupt           120
_define_interrupt           121
_define_interrupt           122
_define_interrupt           123
_define_interrupt           124
_define_interrupt           125
_define_interrupt           126
_define_interrupt           127

_interrupt_entry_common:
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
    mov     16(%rsp), %rsi
    mov     24(%rsp), %ax
    mov     32(%rsp), %rbx
    mov     40(%rsp), %rcx
    mov     48(%rsp), %dx
    mov     %rsi, %fs:56
    mov     %ax,  %fs:32
    mov     %rbx, %fs:24
    mov     %rcx, %fs:64
    mov     %dx,  %fs:34

    # Load a vector number, the error code and then call the interrupt handler.
    # The interrupt handler returns the new FS value in RAX.  With the error
    # code still pushed onto the stack, the stack will be at 16-byte alignment.
    # We require 16-byte alignment prior to a call instruction, so we don't
    # pop the error code until later.
    pop     %rdi
    mov     0(%rsp), %rsi
    mov     %rsp, %rdx
    mov     _interrupt_handlers(,%rdi,8), %rax
    call    *%rax
    pop     %rsi

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
.globl _interrupt_entry_noop
_interrupt_entry_noop:
    iretq


.section .note.GNU-stack,"",%progbits
