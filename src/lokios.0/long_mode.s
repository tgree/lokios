.code16
.text


# Switch to long mode and jump into the kernel.  Interrupts should be disabled.
#   On entry:
#       ESI - kernel address to jump to
.global  _enter_long_mode
_enter_long_mode:
    # Set the PAT MSR to: 
    #   WB WT WC UC WB WT WC UC
    # Note that this makes the PTE PAT bit into a "don't care" bit.
    mov     $0x277, %ecx
    mov     $0x00010406, %edx
    mov     $0x00010406, %eax
    wrmsr

    # Load the IDT.
    lidt    _idt_64_desc

    # Enable PAE.
    mov     %cr4, %eax
    or      $(1<<5), %eax
    mov     %eax, %cr4

    # Set CR3 to point at the first level page table we defined in the linker
    # file.  This page table identity-maps the first 8M of RAM.
    xor     %eax, %eax
    lea     _page_table, %ax
    mov     %eax, %cr3

    # Set EFER.LME and EFER.NXE.
    mov     $0xC0000080, %ecx
    rdmsr
    or      $(1<<8) | (1 << 11), %eax
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

    # Switch to the new stack.
    xor     %rsp, %rsp
    movl    $_kernel_stack, %esp
    or      $0xFFFFFFFFC0000000, %rsp

    # Allocate some space at the top of the stack for a temporary TLS area
    # until kernel_task gets spawned.  We move this into MSR_FS_BASE.
    mov     $0xC0000100, %ecx
    sub     $64, %rsp
    mov     %rsp, %rax
    mov     %rsp, %rdx
    shr     $32, %rdx
    wrmsr
    mov     %rax, 0(%rax)

    # Jump into the kernel.
    mov      %esi, %eax
    or       $0xFFFFFFFFC0000000, %rax
    jmp     *%rax
