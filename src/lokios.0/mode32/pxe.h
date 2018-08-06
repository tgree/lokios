#ifndef __MODE32_PXE_H
#define __MODE32_PXE_H

#include "far_pointer.h"
#include "hdr/endian.h"

struct __PACKED__ pxe_env_plus
{
    char        sig[6];
    uint16_t    version;
    uint8_t     len;
    uint8_t     checksum;
    far_pointer rm_entry;
    uint32_t    pm_offset;
    uint16_t    pm_selector;
    uint16_t    stack_seg;
    uint16_t    stack_size;
    uint16_t    bc_codeseg;
    uint16_t    bc_codesize;
    uint16_t    bc_dataseg;
    uint16_t    bc_datasize;
    uint16_t    bc_undidataseg;
    uint16_t    bc_undidatasize;
    uint16_t    bc_undicodeseg;
    uint16_t    bc_undicodesize;
    far_pointer notpxe_ptr;
};
KASSERT(sizeof(pxe_env_plus) == 44);

// We use a truncated version of the !PXE struct since we only car about the
// far pointers at the top of the struct.  The full struct has a variable-
// length descriptor table at the end that we don't care to deal with; during
// checksumming we'll do the right thing and checksum according to the len
// field.
struct __PACKED__ not_pxe_truncated
{
    char            sig[4];
    uint8_t         len;
    uint8_t         checksum;
    uint8_t         revision;
    uint8_t         rsrv1;
    far_pointer     undi_rom_id_ptr;
    far_pointer     base_rom_id_ptr;
    far_pointer     entry_point_sp;
    far_pointer     entry_point_esp;
    far_pointer     status_callout_proc;
};
KASSERT(sizeof(not_pxe_truncated) == 28);

struct __PACKED__ pxe_get_cached_info_pb
{
    uint16_t    status;
    uint16_t    packet_type;
    uint16_t    buffer_size;
    far_pointer buffer_ptr;
    uint16_t    buffer_limit;
};
KASSERT(sizeof(pxe_get_cached_info_pb) == 12);

struct __PACKED__ tftp_open_pb
{
    uint16_t    status;
    uint8_t     server_ip[4];
    uint8_t     gw_ip[4];
    char        filename[128];
    be_uint16_t port;
    uint16_t    packet_size;
};
KASSERT(sizeof(tftp_open_pb) == 142);

struct __PACKED__ tftp_read_pb
{
    uint16_t    status;
    uint16_t    packet_num;
    uint16_t    buffer_size;
    far_pointer buffer_fp;
};
KASSERT(sizeof(tftp_read_pb) == 10);

struct __PACKED__ pxe_generic_pb
{
    uint16_t    status;
    uint8_t     data[10];
};

int pxe_entry();

#endif /* __MODE32_PXE_H */
