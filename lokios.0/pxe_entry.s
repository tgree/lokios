.code16
.text


# Entry point from the bootsector.
.globl _pxe_entry
_pxe_entry:
    # Print the banners.
    lea     _pxe_text, %si
    call    _print_banners
    jmp     _common_entry


.data
_pxe_text:
    .asciz  "PXE "
