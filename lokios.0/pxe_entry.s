.code16
.text


# Entry point from the bootsector.
.globl _pxe_entry
_pxe_entry:
    # Print the banners.
    jmp     _common_entry
