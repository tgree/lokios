.code16
.text

# Macro to provide a big-endian uint16_t value.
.macro .beshort val:req
.short  (\val << 8) | (\val >> 8)
.endm


# Entry point from the bootsector.
# On entry:
#   _saved_es_bx : should contain a far pointer to the PXENV+ structure
#   _saved_ss_sp : contains a far pointer to the entry stack
# The entry stack is useful because for later versions of the PXE spec it tells
# us how to find the !PXE structure.
.globl _pxe_entry
_pxe_entry:
    # Note that we have successfully started the PXE bootloader and enable both
    # the A20 gate and unreal mode.
    lea     .L_pxe_bootloader_started_text, %si
    call    _common_init
    jc      .L_pxe_entry_return_to_bios

    # If we are here, we've already found that there is a PXENV+ signature at
    # ES:BX.  Load into GS:BX (we want to keep ES = DS) and let's checksum the
    # thing.  The PXENV+ structure is 44 bytes long.
    lgs     _saved_es_bx, %bx
    push    %bx
    xor     %ecx, %ecx
    movb    %gs:8(%bx), %cl
    call    _checksum
    test    %al, %al
    jne     _pxenv_checksum_mismatch

    # If the PXENV+ struct is less than 0x0201 then we should use it.
    pop     %bx
    cmpw    $0x0201, %gs:6(%bx)
    jb      _pxenv_below_2_1

    # There is a !PXE struct pointed to by the end of the PXENV+ struct.  Start
    # by looking for the "!PXE" signature.
    lgs     %gs:40(%bx), %bx
    cmpl    $0x45585021, %gs:(%bx)
    jne     _notpxe_sig_missing

    # Now we should checksum the !PXE struct.
    push    %bx
    xor     %ecx, %ecx
    movb    %gs:4(%bx), %cl
    call    _checksum
    test    %al, %al
    jne     _notpxe_checksum_mismatch

    # The !PXE struct checks out.
    pop     %bx
    jmp     _use_notpxe_struct

    # The PXENV+ struct should be used.  We need to reload its address.
_use_pxenv_struct:
    lgs     _saved_es_bx, %bx
    movl    %gs:10(%bx), %eax
    movl    %eax, _pxe_api_far_ptr
    lea     .L_use_pxenv_struct_text, %si
    call    _puts
    jmp     _pxe_entry_found

    # The !PXE struct should be used.  It's address is in GS:BX.
_use_notpxe_struct:
    movl    %gs:16(%bx), %eax
    movl    %eax, _pxe_api_far_ptr
    lea     .L_use_notpxe_struct_text, %si
    call    _puts
    jmp     _pxe_entry_found

    # Okay, we have found an entry point.  Check it for validity, and if it is
    # good then jump to the code that will load the kernel.
_pxe_entry_found:
    movl    _pxe_api_far_ptr, %eax
    test    %eax, %eax
    je      _pxe_api_null_ptr
    jmp     _pxe_start_load

    # The PXENV+ struct was bad.  We are just going to bail out back to BIOS at
    # this point.
_pxenv_checksum_mismatch:
    mov     %eax, %edx
    call    _put32
    lea     .L_pxenv_checksum_mismatch_text, %si
    call    _puts
.L_pxe_entry_return_to_bios:
    ret

    # The PXENV+ struct is before 2.1.  This means !PXE isn't supported by this
    # version of the BIOS, so just go with PXENV+.
_pxenv_below_2_1:
    lea     .L_pxenv_below_2_1_text, %si
    call    _puts
    jmp     _use_pxenv_struct

    # The !PXE struct is missing the "!PXE" signature.  Go with the PXENV+
    # struct.
_notpxe_sig_missing:
    lea     .L_notpxe_sig_missing_text, %si
    call    _puts
    jmp     _use_pxenv_struct

    # The !PXE struct has an invalid checksum.  Go with the PXENV+ struct.
_notpxe_checksum_mismatch:
    mov     %eax, %edx
    call    _put32
    lea     .L_notpxe_checksum_mismatch_text, %si
    call    _puts
    jmp     _use_pxenv_struct

    # The PXE API we are using has a NULL pointer!  Return to BIOS.
_pxe_api_null_ptr:
    lea     .L_pxe_api_null_ptr_text, %si
    call    _puts
    ret


# Ready to start loading the kernel via PXE.
# On entry:
#   nothing.  
_pxe_start_load:
    # Okay, tell everyone how we're doing.
    lea     .L_pxe_loading_beginning_text, %si
    call    _puts

    # Start by getting the cached DHCP response we got from the server.  This
    # is going to tell us the server's IP address.
    lea     _pxe_get_cached_info_cmd, %di
    mov     $0x0071, %bx
    call    _call_pxe
    jc      _pxe_cmd_error

    # The result was filled in; notify of the IP address we will get the file
    # from.
    lea     .L_pxe_server_ip_text, %si
    call    _puts
    lgs     _pxe_get_cached_info_cmd + 6, %si
    movl    %gs:20(%si), %edx
    call    _putipv4
    call    _putCRLF

    # Do a TFTP OPEN command for our target file.
    lea     _pxe_open_cmd, %di
    mov     $0x0020, %bx
    movl    %edx, _pxe_open_cmd_server_ip
    call    _call_pxe
    jc      _pxe_cmd_error

    # Do a TFTP READ to get the first 512-byte packet.
    lea     _pxe_read_cmd, %di
    mov     $0x0022, %bx
    call    _call_pxe
    jc      _pxe_cmd_error

    # Ensure that this really was the first packet.
    mov     $0xFFFF, %bx
    mov     _pxe_read_cmd + 2, %ax
    cmp     $1, %ax
    jne     _pxe_cmd_error

    # Ensure we got a full sector.
    movw    _pxe_read_cmd + 4, %ax
    cmp     $512, %ax
    jne     .L_read_truncated

    # Get the remaining number of sectors to bounce-copy.
    movl    _pre_e820_bounce_buffer, %esi
    movl    %esi, _pxe_remaining_sectors

.L_read_loop:
    # Bounce the packet up into place.  Sadly, calling into PXE breaks unreal
    # mode.  It probably switches into protected mode since it has a protected
    # mode entry point as well, and switching would cause the CPU to then
    # update the FS limit values if PXE changed FS for some reason.
    call    _enter_unreal_mode
    mov     _pxe_sector_number, %esi    # Sector number
    mov     %esi, %edi                  # Make a copy
    shl     $9, %edi                    # Now it's an offset
    add     $_kernel_base, %edi         # Now it's an address
    inc     %esi                        # Advance the sector number
    mov     %esi, _pxe_sector_number
    lea     _pre_e820_bounce_buffer, %esi
    mov     $512/4, %ecx
    call    _unreal_memcpy

    # We copied a sector.  See if we are done.
    mov     _pxe_remaining_sectors, %esi
    dec     %esi
    je      .L_read_loop_done
    mov     %esi, _pxe_remaining_sectors

    # Read one sector into the bounce buffer.
    lea     _pxe_read_cmd, %di
    mov     $0x0022, %bx
    call    _call_pxe
    jc      _pxe_cmd_error

    # Ensure we got a full sector.
    movw    _pxe_read_cmd + 4, %dx
    cmp     $512, %dx
    jne     .L_read_truncated

    # And loop until we are done.
    jmp     .L_read_loop

.L_read_loop_done:
    # Execute the sequence that we want to use to try and shut down PXE.
    mov     _pxe_shutdown_num_ops, %cx
    lea     _pxe_shutdown_ops_cmd, %di
    lea     _pxe_shutdown_ops, %si
.L_shutdown_op_loop:
    mov     (%si), %bx
    add     $2, %si
    call    _call_pxe
    jc      _pxe_cmd_error
    loop    .L_shutdown_op_loop

    # Jump to the common entry point.
    jmp     _common_entry

.L_read_truncated:
    # We thought we had more sectors to go, but the PXE server didn't.  Print
    # an error and return to BIOS.
    lea     .L_read_truncated_text, %si
    call    _puts
    ret

    # Some PXE call failed.
    # On entry:
    #   DI  - address of command param block
    #   AX  - immediate return value
    #   BX  - the opcode
_pxe_cmd_error:
    push    %ax
    mov     %bx, %dx
    call    _put16
    mov     $':', %al
    call    _putc
    mov     $' ', %al
    call    _putc
    pop     %dx
    call    _put16
    mov     $' ', %al
    call    _putc
    movw    (%di), %dx
    call    _put16
    mov     $' ', %al
    call    _putc
    lea     .L_pxe_cmd_error_text, %si
    call    _puts
    ret

# On entry:
#   ES:DI - address of the command buffer
#   BX    - opcode
# On exit:
#   AX    - contains the immediate return code
#   BX    - the opcode that was used
#   CF    - set in case of error
# Stomps:
#   EAX, EBX
_call_pxe:
    push    %ds
    push    %di
    push    %bx

    lcall   *_pxe_api_far_ptr

    # Error checking and set/clear the CF bit.
    test    %ax, %ax
    jne     .L_call_pxe_failed
    movw    (%di), %bx
    test    %bx, %bx
    jne     .L_call_pxe_failed

    pop     %bx
    add     $4, %sp
    clc
    jmp     .L_call_pxe_done

.L_call_pxe_failed:
    pop     %bx
    add     $4, %sp
    stc

.L_call_pxe_done:
    ret


# On entry:
#   GS:BX   - address to checksum
#   ECX     - number of bytes to checksum
# On exit:
#   AL      - checksum value
_checksum:
    mov     $0, %eax
.L_checksum_loop:
    addb    %gs:(%bx), %al
    inc     %bx
    loop    .L_checksum_loop
    ret


# Dump memory.
# On entry:
#   GS:BX   - base address of memory
#   ECX     - number of 32-bit words to dump
_dump_mem:
    mov     %bx, %si
    mov     %cx, %di
.L_dump_mem_loop:
    mov     %gs:(%si), %edx
    call    _put32
    mov     $' ', %al
    call    _putc
    add     $4, %si
    dec     %di
    jne     .L_dump_mem_loop
    ret


.data
.L_pxe_bootloader_started_text:
    .asciz  "PXE bootloader started\r\n"
.L_pxenv_below_2_1_text:
    .asciz   "PXENV+ struct is less than 2.1, !PXE not supported"
.L_notpxe_sig_missing_text:
    .asciz   "!PXE signature is missing"
.L_pxenv_checksum_mismatch_text:
    .asciz  " <- PXENV+ struct has invalid checksum\r\n"
.L_notpxe_checksum_mismatch_text:
    .asciz  " <- !PXE struct has invalid checksum\r\n"
.L_pxe_cmd_error_text:
    .asciz  " PXE Command Error\r\n"
.L_read_truncated_text:
    .asciz  "PXE stream EOF before full image downloaded\r\n"
.L_use_pxenv_struct_text:
    .asciz  "Using PXENV+ struct\r\n"
.L_use_notpxe_struct_text:
    .asciz  "Using !PXE struct\r\n"
.L_pxe_api_null_ptr_text:
    .asciz  "Chosen PXE API has a NULL real mode API entry pointer!\r\n"
.L_pxe_loading_beginning_text:
    .asciz  "Fetching kernel via PXE...\r\n"
.L_pxe_server_ip_text:
    .asciz  "PXE server IP: "
_pxe_api_far_ptr:
    .long   0
_pxe_get_cached_info_cmd:
    .short  0
    .short  2   # PXENV_PACKET_TYPE_DHCP_ACK
    .short  0
    .long   0
    .short  0

_pxe_open_cmd:
    .short      0
_pxe_open_cmd_server_ip:
    .long       0   # Server IP Address
    .long       0   # Gateway IP Address
_pxe_open_cmd_filename:
    .asciz      "lokios.1"
    .zero       (128 + _pxe_open_cmd_filename - .)
    .beshort    69  # UDP Port
    .short      512 # Packet size

_pxe_read_cmd:
    .short  0
    .short  0
    .short  0
    .short  _pre_e820_bounce_buffer
    .short  0

# It's really not clear what we have to do to shut this thing down.  That
# Stop UNDI command is particularly troublesome with iPXE on the Macbook Air
# because it ends up causing the system to reset shortly after we get into our
# kernel.  Hopefully we shut down enough things here that this doesn't matter.
_pxe_shutdown_ops_cmd:
    .short  0
    .zero   10
_pxe_shutdown_ops:
    .short  0x0021  # TFTP Close
    .short  0x0007  # UNDI Close
    .short  0x0005  # UNDI Shutdown
    .short  0x0076  # Stop Base
    .short  0x0070  # Unload Stack
    .short  0x0002  # UNDI Cleanup
#   .short  0x0015  # Stop UNDI
_pxe_shutdown_num_ops:
    .short  (_pxe_shutdown_num_ops - _pxe_shutdown_ops) / 2

_pxe_sector_number:
    .long   0
_pxe_remaining_sectors:
    .long   0
