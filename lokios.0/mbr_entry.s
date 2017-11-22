.code16
.text


# Entry point from the bootsector.
.globl _mbr_entry
_mbr_entry:
    # Note that we have successfully started the MBR bootloader.
    lea     .L_mbr_bootloader_started_text, %si
    call    _puts

    # Jump to the common entry point.
    jmp     _common_entry


.data
.L_mbr_bootloader_started_text:
    .asciz  "MBR bootloader started\r\n"
