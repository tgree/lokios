.code16
.text


# Starting point.  These are the first instructions executed after BIOS.
# --------
# The PXE spec says that this is the state upon entry here:
# - CS:IP must contain the value 0:7C00h.
# - ES:BX must contain the address of the PXENV+ structure.
# - SS:[SP+4] must contain the segment:offset address of the !PXE structure.
# - SS:SP is to contain the address of the beginning of the unused portion of
#   the PXEservices stack.  There must be at least 1.5KB of free stack space
#   for the NBP.
# --------
# On entry from ipxe, the actual register state is as follows:
#
#   CR0 - 0x00000012
#   CS  - 0x0000
#   SS  - 0x9CB4
#   DS  - 0x9CB4
#   ES  - 0x9C28
#   FS  - 0x9CB4
#   GS  - 0x9CB4
#   
# We are in real-addressing mode and memory addresses are generated using
# simple math on the segment registers rather than going through a descriptor
# table:
#
#   linear address = ((segment register << 4) + offset)
#
# The segment register is typically selected implicitly based on the
# instruction being executed; code fetches always use CS, stack fetches use
# SS and data fetches use DS unless you explicitly specify one of ES, FS or GS.
#
# Here, we are going to force CS to be 0 via a far jmp (just in case it
# actually wasn't 0 out of BIOS - apparently some BIOSes foolishly far jmp to
# 0x7c0:0 instead of 0:0x7c00) and we are also going to force DS to 0 so that
# data addresses are consistent with code addresses.  We are going to leave ES
# alone since we may wish to access ES:BX for PXENV+ at some point.
#
# We aren't going to touch the stack for now, which is living up in some BIOS
# memory area, but eventually we should move it down into our memory.
# Apparently some BIOSes also require you to be on this stack when you call
# into PXE routines and we'll surely need to be on it if we ever return back to
# BIOS.
.globl _start
_start:
    # Save everything.
    pushfw
    pushaw
    pushw   %ss
    pushw   %ds
    pushw   %es
    pushw   %fs
    pushw   %gs

    # Clear DS manually then clear CS by doing a far call to the target.
    xor     %ax, %ax
    mov     %ax, %ds
    lcall   $0, $_lokios_start

.if 0
    # Return to BIOS.
    pop     %gs
    pop     %fs
    pop     %es
    pop     %ds
    pop     %ss
    popaw
    popfw
    xor     %ax, %ax
    lret
.else
    # Loop forever.
_forever:
    hlt
    jmp     _forever
.endif
