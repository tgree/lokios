.code16
.text


.global _enter_unreal_mode
_enter_unreal_mode:
    # Load the GDT.
    lgdt    _gdt_unreal_16_desc

    # Enable protected mode.
    mov     %cr0, %eax
    or      $(1<<0), %eax
    mov     %eax, %cr0

    # Wonky.
    jmp     .L_386_486_bug
.L_386_486_bug:

    # Load FS and GS with unreal mode.  The $8 below is the offset to the
    # descriptor in the GDT.  This loads the "hidden part" of the FS and GS
    # registers with the limits from the GDT entry.
    mov     $8, %bx
    mov     %bx, %fs
    mov     %bx, %gs

    # Leave protected mode.
    and     $~(1<<0), %eax
    mov     %eax, %cr0

    # Now we reset FS and GS back to 0 so that the linear address is calculated
    # from a 0 base.  The key here is that in real mode there are no segment
    # descriptors so the processor doesn't go and reload the "hidden part" of
    # the registers - but it keeps the 4G limit that's already in there!  So we
    # can now use expressions like:
    #
    #   mov     %dx, %fs:(%eax)
    #
    # To access any 32-bit address (note the used of %eax instead of %ax,
    # specifying a 32-bit register offset from FS-base).
    mov     $0, %bx
    mov     %bx, %fs
    mov     %bx, %gs

    # FS and GS are now in unreal mode.
    ret
