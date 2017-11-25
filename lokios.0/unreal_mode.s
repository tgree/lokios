.code16
.text


# Switch in to unreal mode.
# Stomps:
#   EAX, EBX
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

    # Load FS with unreal mode.  The $8 below is the offset to the descriptor
    # in the GDT.  This loads the "hidden part" of the FS register with the
    # limits from the GDT entry.
    mov     $8, %bx
    mov     %bx, %fs

    # Leave protected mode.
    and     $~(1<<0), %eax
    mov     %eax, %cr0

    # Now we reset FS back to 0 so that the linear address is calculated from a
    # 0 base.  The key here is that in real mode there are no segment
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

    # FS is now in unreal mode.
    ret


# Do a memcpy.
#   ESI - 32-bit source address
#   EDI - 32-bit destination address
#   ECX - number of 32-bit words to copy
# On exit:
#   ESI, EDI - incremented to the end of their respective buffers
# Stomped:
#   E[XHL], ECX
.globl _unreal_memcpy
_unreal_memcpy:
    cmp     %ecx, 0
    je      .L_unreal_memcpy_done

.L_unreal_memcpy_loop:
    movl    %fs:(%esi), %eax
    movl    %eax, %fs:(%edi)
    add     $4, %esi
    add     $4, %edi
    dec     %ecx
    jne     .L_unreal_memcpy_loop

.L_unreal_memcpy_done:
    ret
