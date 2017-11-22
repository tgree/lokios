.code16
.text


# Entry point from the bootsector.
.globl _mbr_entry
_mbr_entry:
    # For now we just have a common sequence to run.
    jmp     _common_entry
