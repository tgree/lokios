.code16
.text

.equiv END_WITH_HALT, 0


# Starting point.  These are the first instructions executed after BIOS and we
# are in the wild wild west.  Since we want to be able to handle either an MBR
# or a PXE boot, we need to work with the lowest common denominator until we
# get more information about the state of the system.  With PXE we at least get
# a stack, but with MBR we don't even get that.  The state is something like
# this:
#
#   MBR:
#       CR0       - 0x60000010 (via bochs)
#       CS:IP     - 0:0x7C00 or possibly 0x7C0:0
#       DL        - contains the drive number in an MBR boot
#
#   PXE:
#       CR0       - 0x00000012 (via ipxe)
#       CS:IP     - supposed to be 0:0x7C00
#       ES:BX     - contains the PXENV+ struct
#       SS:[SP+4] - contains the segment:offset address of the !PXE struct
#       SS:SP     - contains 1.5K of stack
#
# Apparently there is no guarantee via MBR that there is any sort of usable
# stack.  So, we are going to preserve DL, ES:BX, SS:SP and then switch to our
# own stack.
# 
# osdev.org says that 0x07E00-0x7FFFF is guaranteed free for use.
# The PXE spec says that the max safe size of an image laoded at 0x7C00 is 32K,
# implying that only 0x07E00-0x0x0FC00 is guaranteed free for use.  Since PXE
# is the lowest common denominator here, we are going to relocate our stack to
# 0xFC00.
.globl _start
_start:
    # Disable interrupts.
    cli

    # Clear CS
    ljmp    $0, $.L_start_clear_cs
.L_start_clear_cs:

    # Save SS:SP.
    movw    %sp, %cs:.L_saved_ss_sp
    movw    %ss, %sp
    movw    %sp, %cs:.L_saved_ss_sp+2

    # Load 0:FC00 into SS:SP.
    lss     %cs:.L_loader_ss_sp, %sp

    # Save all args.
    push    %bx
    push    %es
    mov     %cs:.L_saved_ss_sp+2, %bx
    push    %bx
    mov     %cs:.L_saved_ss_sp, %bx
    push    %bx
    pushfw
    pushaw
    pushw   %ds

    # Clear DS.
    xor     %ax, %ax
    mov     %ax, %ds

    # Clear direction so string operations move forward in memory.
    cld

    # Check if we can see a 'PXENV+' signature at ES:BX and dispatch to the
    # correct entry point.  We also set ES = 0 on the way out so that it maps
    # to the same segment as DS.
    lea     .L_pxenv_str, %si
    mov     %bx, %di
    mov     $6, %cx
    repe cmpsb
    mov     %ax, %es
    je      .L_pxe_start
.L_mbr_start:
    call    _dispatch_mbr
    jmp     .L_done
.L_pxe_start:
    call    _dispatch_pxe
.L_done:

.if END_WITH_HALT
    # Loop forever.
.L_forever:
    hlt
    jmp     .L_forever
.else
    # Return to BIOS.
    pop     %ds
    popaw
    xor     %ax, %ax    # Return 0 to PXE BIOS.
    popfw
    les     %ss:0xFBFC, %bx
    lss     %ss:0xFBF8, %sp
    lret
.endif

.data
.L_pxenv_str:
    .ascii  "PXENV+"
.L_saved_ss_sp:
    .word   0       # SP
    .word   0       # SS
.L_loader_ss_sp:
    .word   0xFC00  # SP
    .word   0       # SS
