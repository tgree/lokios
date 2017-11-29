.code16
.text


# Identity-map the first 2M of RAM using a single hugepage.  Interrupts should
# be disabled.
.global  _enter_long_mode
_enter_long_mode:
    # Load the IDT.
    lidt    _idt_64_desc

    # Enable PAE.
    mov     %cr4, %eax
    or      $(1<<5), %eax
    mov     %eax, %cr4

    # Set CR3 to point at the first level page table we defined in the linker
    # file.  This page table identity-maps the first 2M of RAM.
    xor     %eax, %eax
    lea     _page_table, %ax
    mov     %eax, %cr3

    # Set EFER.LME.
    mov     $0xC0000080, %ecx
    rdmsr
    or      $(1<<8), %eax
    wrmsr

    # Enable paging and protected mode.
    mov     %cr0, %eax
    or      $(1<<31) | (1<<0), %eax
    mov     %eax, %cr0

    # OK, we are now in compatibility mode.  We need to load the GDT and use it
    # to jump into our code segment.  The $8 below is the offset to the code
    # descriptor in the GDT.
    lgdt    _gdt_64_desc
    ljmp    $8, $_into_long_mode

.code64
_into_long_mode:
    # Set up the kernel parameters in a write-protected page just past the
    # top of the stack.
    xor     %rdi, %rdi
    movl    $_kernel_params, %edi
    xor     %rax, %rax
    mov     $_e820_end, %eax
    mov     %rax, 0(%rdi)
    mov     $0x000B8000, %eax
    mov     %rax, 8(%rdi)

    # Enable write protection.  It seems we have to do this after the long jump
    # above or things break.
    mov     %cr0, %rax
    or      $(1<<16), %rax
    mov     %rax, %cr0

    # Zero out the kernel TLS page.
    mov     $0x4E4F4E4F4E4F4E4F, %rax
    #xor     %rax, %rax
    mov     $_kernel_tls_page, %esi
    mov     $4096/8, %ecx
.L_zero_tls_loop:
    mov     %rax, (%esi)
    add     $8, %esi
    loop    .L_zero_tls_loop

    # Set MSR_FS_BASE to point at the TLS area.
    mov     $0xC0000100, %ecx
    mov     $_kernel_tls, %eax
    wrmsr
    mov     %rax, 0(%rax)

    # Jump into the kernel after switching to the new stack.
    xor     %rsp, %rsp
    movl    $_kernel_stack, %esp
    xor     %rax, %rax
    movl    $_kernel_entry, %eax
    jmp     *%rax