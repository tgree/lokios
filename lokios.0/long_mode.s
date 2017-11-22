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

.code32
_into_long_mode:
    movl    $_kernel_base, %eax
    jmp     *%eax
