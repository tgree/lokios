.code16
.text


# Entry point from the bootsector.
.globl _pxe_entry
_pxe_entry:
    # Note that we have successfully started the PXE bootloader.
    lea     .L_pxe_bootloader_started_text, %si
    call    _puts

    # Jump to the common entry point.
    jmp     _common_entry


.data
.L_pxe_bootloader_started_text:
    .asciz  "PXE bootloader started\r\n"
