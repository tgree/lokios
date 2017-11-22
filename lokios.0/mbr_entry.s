.code16
.text


# Entry point from the bootsector.
.globl _mbr_entry
_mbr_entry:
    # No banners to print, already done by bootsector code prior to attempting
    # to load the rest of the image.
    jmp     _common_entry
