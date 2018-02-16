# Offsets of struct scheduler stuff.
.equiv scheduler_local_work_head_offset,    192
.equiv scheduler_local_work_tail_offset,    200

# Offsets of struct cpu stuff
.equiv cpu_cpu_addr_offset,     0
.equiv cpu_jiffies_offset,      8
.equiv cpu_scheduler_offset,    128
.equiv cpu_scheduler_local_work_head_offset, (cpu_scheduler_offset + scheduler_local_work_head_offset)
.equiv cpu_scheduler_local_work_tail_offset, (cpu_scheduler_offset + scheduler_local_work_tail_offset)
.equiv cpu_msix_entries_offset, 4096

# Offsets of struct work_entry stuff.
.equiv work_entry_args0_offset, 16
.equiv work_entry_args1_offset, (work_entry_args0_offset + 8)

.org 0
    jmp     _bsp_entry

.org 16
    jmp     _ap_entry


# On entry:
#   RDI - address of the kernel_args struct
#   RSP - set to the bootloader temporary stack
_bsp_entry:
    # Populate kernel::args.
    mov     %rdi, _ZN6kernel5kargsE

    # Enable SSE since the stupid compiler is generating %xmm register
    # references and I can't turn it off because then stdint.h doesn't work.
    call    _enable_sse

    # Initialize the bootstrap processor.
    jmp     init_bsp


# On entry:
#   RDI - address of the kernel_args struct
#   RSP - set to the bootloader temporary stack
_ap_entry:
    # Enable SSE (see above).
    call    _enable_sse

    # Initialize the application processor.
    jmp     init_ap


# Enable SSE.
_enable_sse:
    mov     %cr0, %rax
    and     $0xFFFB, %ax
    or      $2, %ax
    mov     %rax, %cr0
    mov     %cr4, %rax
    or      $(3 << 9), %ax
    mov     %rax, %cr4
    ret


# On entry:
#   rdi - thread* to activate
.globl _thread_jump
_thread_jump:
    # Set up a dummy interrupt frame in the thread tcb and jump to the
    # interrupt-handler exit code that will restore the state out of the thread
    # context we are trying to jump into.
    lea     26624(%rdi), %rax   # thread.tcb
    lea     27280(%rdi), %rsp   # thread.tcb.fxsa.avail
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

    # Save the FPU and SSE state.
    fxsaveq %fs:192

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

    # Restore the FPU and SSE state.
    fxrstorq    %fs:192

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


# BSP/AP tickers.  Our only task is to increment jiffies and ack the lapic
# interrupt by writing to EOI.  This ticker's only purpose is to periodically
# wake up a halted CPU so that it can service its timer wheels.
# We also use this exact same code as an entry point for IPIs used to wake up a
# halted CPU that may now have new work on its work queue.
#   Note: The KERNEL_GS_BASE contains a pointer to the local APIC
.globl _interrupt_entry_ticker
.globl _interrupt_entry_scheduler_wakeup
_interrupt_entry_ticker:
    incq    %gs:cpu_jiffies_offset
_interrupt_entry_scheduler_wakeup:
    swapgs
    movl    $0, %gs:0xB0
    swapgs
    iretq


# MSI-X vector entry.  We need to look at the cpu* to find the work_entry* that
# is going to handle this interrupt at task level.  Using that information we
# will then mask the MSI-X vector in the device.  Then we will queue up the
# work_entry* on the cpu's scheduler and finally acknowledge the interrupt in
# the local APIC.
#   The work_entry's args[] is as follows:
#       [0] - address of the MSI-X vector register on the device
.macro _define_msix_interrupt num
.global _msix_entry_\num
_msix_entry_\num :
    push    %rbx
    movq    $(cpu_msix_entries_offset + (64 * \num)), %rbx
    jmp     _msix_entry_generic
.endm

_define_msix_interrupt  128
_define_msix_interrupt  129
_define_msix_interrupt  130
_define_msix_interrupt  131
_define_msix_interrupt  132
_define_msix_interrupt  133
_define_msix_interrupt  134
_define_msix_interrupt  135
_define_msix_interrupt  136
_define_msix_interrupt  137
_define_msix_interrupt  138
_define_msix_interrupt  139
_define_msix_interrupt  140
_define_msix_interrupt  141
_define_msix_interrupt  142
_define_msix_interrupt  143
_define_msix_interrupt  144
_define_msix_interrupt  145
_define_msix_interrupt  146
_define_msix_interrupt  147
_define_msix_interrupt  148
_define_msix_interrupt  149
_define_msix_interrupt  150
_define_msix_interrupt  151
_define_msix_interrupt  152
_define_msix_interrupt  153
_define_msix_interrupt  154
_define_msix_interrupt  155
_define_msix_interrupt  156
_define_msix_interrupt  157
_define_msix_interrupt  158
_define_msix_interrupt  159
_define_msix_interrupt  160
_define_msix_interrupt  161
_define_msix_interrupt  162
_define_msix_interrupt  163
_define_msix_interrupt  164
_define_msix_interrupt  165
_define_msix_interrupt  166
_define_msix_interrupt  167
_define_msix_interrupt  168
_define_msix_interrupt  169
_define_msix_interrupt  170
_define_msix_interrupt  171
_define_msix_interrupt  172
_define_msix_interrupt  173
_define_msix_interrupt  174
_define_msix_interrupt  175
_define_msix_interrupt  176
_define_msix_interrupt  177
_define_msix_interrupt  178
_define_msix_interrupt  179
_define_msix_interrupt  180
_define_msix_interrupt  181
_define_msix_interrupt  182
_define_msix_interrupt  183
_define_msix_interrupt  184
_define_msix_interrupt  185
_define_msix_interrupt  186
_define_msix_interrupt  187
_define_msix_interrupt  188
_define_msix_interrupt  189
_define_msix_interrupt  190
_define_msix_interrupt  191
_define_msix_interrupt  192
_define_msix_interrupt  193
_define_msix_interrupt  194
_define_msix_interrupt  195
_define_msix_interrupt  196
_define_msix_interrupt  197
_define_msix_interrupt  198
_define_msix_interrupt  199
_define_msix_interrupt  200
_define_msix_interrupt  201
_define_msix_interrupt  202
_define_msix_interrupt  203
_define_msix_interrupt  204
_define_msix_interrupt  205
_define_msix_interrupt  206
_define_msix_interrupt  207
_define_msix_interrupt  208
_define_msix_interrupt  209
_define_msix_interrupt  210
_define_msix_interrupt  211
_define_msix_interrupt  212
_define_msix_interrupt  213
_define_msix_interrupt  214
_define_msix_interrupt  215
_define_msix_interrupt  216
_define_msix_interrupt  217
_define_msix_interrupt  218
_define_msix_interrupt  219
_define_msix_interrupt  220
_define_msix_interrupt  221
_define_msix_interrupt  222
_define_msix_interrupt  223
_define_msix_interrupt	224
_define_msix_interrupt  225
_define_msix_interrupt  226
_define_msix_interrupt  227
_define_msix_interrupt  228
_define_msix_interrupt  229
_define_msix_interrupt  230
_define_msix_interrupt  231
_define_msix_interrupt  232
_define_msix_interrupt  233
_define_msix_interrupt  234
_define_msix_interrupt  235
_define_msix_interrupt  236
_define_msix_interrupt  237
_define_msix_interrupt  238
_define_msix_interrupt  239
_define_msix_interrupt  240
_define_msix_interrupt  241
_define_msix_interrupt  242
_define_msix_interrupt  243
_define_msix_interrupt  244
_define_msix_interrupt  245
_define_msix_interrupt  246
_define_msix_interrupt  247
_define_msix_interrupt  248
_define_msix_interrupt  249
_define_msix_interrupt  250
_define_msix_interrupt  251
_define_msix_interrupt  252
_define_msix_interrupt  253
_define_msix_interrupt  254
_define_msix_interrupt  255

# On entry:
#   rbx      - offset to the msix_entries _work_entry*
#   orig rbx - on stack
_msix_entry_generic:
    push    %rax
    movq    %gs:cpu_cpu_addr_offset, %rax # rax = cpu*
    push    %rcx
    add     %rax, %rbx          # rbx = msix_entry*
    movq    work_entry_args1_offset(%rbx), %rcx
    swapgs
    movl    $1, (%rcx)          # mask the vector

    # Queue up our work entry.
    xorq    %rcx, %rcx
    movq    %rcx, (%rbx)        # msix_entry->link.next = NULL
    movq    cpu_scheduler_local_work_tail_offset(%rax), %rcx
    test    %rcx, %rcx
    jz      .L_null_tail
.L_non_null_tail:
    mov     %rbx, (%rcx)        # tail->next = l
    jmp     .L_set_tail
.L_null_tail:
    mov     %rbx, cpu_scheduler_local_work_head_offset(%rax)
.L_set_tail:
    mov     %rbx, cpu_scheduler_local_work_tail_offset(%rax)

    # Exit
    movl    %eax, %gs:0xB0      # EOI the interrupt
    pop     %rcx
    pop     %rax
    pop     %rbx
    swapgs
    iretq


.section .note.GNU-stack,"",%progbits
