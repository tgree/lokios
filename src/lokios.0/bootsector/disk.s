.code16
.text

# Read from disk into low memory.  If you want to move the output into a higher
# address you'll have to do it yourself through a bounce buffer.  Apparently
# some BIOSes can't read more than 127 sectors so you may want to throttle
# here anyways.
# On entry:
#   DL  - drive number
#   EBX - first sector low-order 32-bits
#   CX  - sector count
#   SI  - destination address
# On exit:
#   CF - set on failure, cleared on success
# Stomped:
#   EA[XHL], SI
.globl _disk_read
_disk_read:
    pop     %ax
    pushl   $0
    pushl   %ebx
    pushw   $0
    push    %si
    push    %cx
    push    $0x0010
    mov     %sp, %si
    push    %ax
    mov     $0x42, %ah
    int     $0x13
    ret     $16
