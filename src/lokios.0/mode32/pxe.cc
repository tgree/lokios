#include "pxe.h"
#include "console.h"
#include "massert.h"
#include <string.h>

extern far_pointer _saved_es_bx;
extern uint8_t _kernel_base[];

static far_pointer _m32_pxe_entry_fp;

static uint8_t
m32_pxe_checksum(void* _p, uint32_t n)
{
    uint8_t sum = 0;
    uint8_t* p  = (uint8_t*)_p;
    while (n--)
        sum += *p++;
    return sum;
}

static int
m32_pxe_find_entry(far_pointer* fp)
{
    // Checksum the PXENV+ struct.
    auto* pep = (pxe_env_plus*)_saved_es_bx.to_addr();
    uint8_t cs = m32_pxe_checksum(pep,pep->len);
    if (cs != 0)
    {
        console::printf("PXENV+ struct has non-zero checksum (0x%02X)\n",cs);
        return -1;
    }

    // We have a valid PXENV+ struct and therefore an entry point.  Save the
    // entry point off in case !PXE doesn't pan out.
    *fp = pep->rm_entry;

    // If the version is < 2.1 then we use the PXENV+ struct.
    if (pep->version < 0x0201)
    {
        console::printf("Version below 2.1, using PXENV+\n");
        return 0;
    }

    // The version is >= 2.1.  Check for the !PXE signature.
    auto* np = (not_pxe_truncated*)pep->notpxe_ptr.to_addr();
    if (np->sig[0] != '!' || np->sig[1] != 'P' || np->sig[2] != 'X' ||
        np->sig[3] != 'E')
    {
        // No !PXE struct was found.  Use the PXENV+ one we already saved off.
        console::printf("!PXE signature missing\n");
        return 0;
    }

    // Found !PXE, checksum it.
    cs = m32_pxe_checksum(np,np->len);
    if (cs != 0)
    {
        // What to do?  We found a !PXE struct but the checksum is bogus.  It
        // could be corrupted or maybe BIOS is just broken?  In any case, we
        // won't trust it, so go with the PXENV+ entry point.
        console::printf("!PXE struct has non-zero checksum (0x%02X), using "
                        "PXENV+\n",cs);
        return 0;
    }

    // !PXE checks out, so use it.
    console::printf("Using !PXE struct\n");
    *fp = np->entry_point_sp;
    return 0;
}

static uint16_t
m32_pxe_get_cached_dhcp_ack(far_pointer* fp, uint16_t* size)
{
    pxe_get_cached_info_pb pb;
    memset(&pb,0,sizeof(pb));
    pb.packet_type = 2;

    uint16_t rc = m32_call_pxe(0x0071,&pb,_m32_pxe_entry_fp);
    if (rc)
        return rc;
    if (pb.status)
        return pb.status;

    *fp   = pb.buffer_ptr;
    *size = pb.buffer_size;
    return 0;
}

static uint16_t
m32_tftp_open(uint8_t* server_ip, const char* filename)
{
    tftp_open_pb pb;
    memset(&pb,0,sizeof(pb));
    pb.server_ip[0] = server_ip[0];
    pb.server_ip[1] = server_ip[1];
    pb.server_ip[2] = server_ip[2];
    pb.server_ip[3] = server_ip[3];
    pb.port         = 69;
    pb.packet_size  = 512;

    char* p = pb.filename;
    while (*filename)
        *p++ = *filename++;

    return m32_call_pxe(0x0020,&pb,_m32_pxe_entry_fp) ?: pb.status;
}

static uint16_t
m32_tftp_read(void* buf, uint16_t* packet_num, uint16_t* size)
{
    tftp_read_pb pb;
    memset(&pb,0,sizeof(pb));
    pb.buffer_fp = far_pointer{(uint16_t)(uint32_t)buf,0};

    uint16_t rc = m32_call_pxe(0x0022,&pb,_m32_pxe_entry_fp);
    if (rc)
        return rc;
    if (pb.status)
        return pb.status;

    *packet_num = pb.packet_num;
    *size       = pb.buffer_size;
    return 0;
}

static uint16_t
m32_pxe_generic_cmd(uint16_t opcode)
{
    pxe_generic_pb pb;
    memset(&pb,0,sizeof(pb));
    return m32_call_pxe(opcode,&pb,_m32_pxe_entry_fp) ?: pb.status;
}

static uint16_t
m32_bounce_packet(uint16_t expected_packet_num)
{
    char* kernel_base = (char*)(uint32_t)_kernel_base;
    uint16_t packet_num;
    uint16_t packet_size;
    uint8_t buf[512];
    m32_assert(!m32_tftp_read(buf,&packet_num,&packet_size));
    m32_assert(packet_num == expected_packet_num);
    m32_assert(packet_size == 512 || packet_size == 0);
    if (packet_size)
        memcpy(kernel_base + 512*(expected_packet_num-1),buf,sizeof(buf));
    return packet_size;
}

int
m32_pxe_entry()
{
    // Find the entry point.
    m32_assert(m32_pxe_find_entry(&_m32_pxe_entry_fp) == 0);

    // Get cached data.
    far_pointer dhcp_fp;
    uint16_t dhcp_size;
    uint16_t rc = m32_pxe_get_cached_dhcp_ack(&dhcp_fp,&dhcp_size);
    if (rc != 0)
    {
        console::printf("Error %u getting cached DHCP ACK\n",rc);
        return -1;
    }

    auto* siaddr = (uint8_t*)dhcp_fp.to_addr() + 20;
    console::printf("DHCP Servier IP: %u.%u.%u.%u\n",
                    siaddr[0],siaddr[1],siaddr[2],siaddr[3]);

    rc = m32_tftp_open(siaddr,"lokios.1");
    if (rc != 0)
    {
        console::printf("Error %u from TFTP_OPEN\n",rc);
        return -2;
    }

    // Handle the first sector.
    m32_assert(m32_bounce_packet(1) == 512);

    // Figure out how many sectors we need.  We've already read the first
    // sector, but there will be an extra zero-length sector from the PXE
    // server at the end to terminate the transfer.
    uint16_t expected_packet_num = 2;
    uint32_t nsectors = *(uint32_t*)(uint32_t)_kernel_base;
    while (--nsectors)
    {
        if (m32_bounce_packet(expected_packet_num++) != 512)
            return -3;
    }
    if (m32_bounce_packet(expected_packet_num) != 0)
            return -4;

    // Issue the PXE shutdown sequence.
    uint16_t shutdown_sequence[] = {
            0x0021, // TFTP Close
        //  0x0007, // UNDI Close - DELL doesn't like this error 0x6A
            0x0005, // UNDI Shutdown
            0x0076, // Stop Base
        //  0x0070, // Unload Stack - DELL doesn't like this one error 0x01
            0x0002, // UNDI Cleanup
        //  0x0015, // Stop UNDI - iPXE doesn't seem to like this one
    };
    for (uint16_t opcode : shutdown_sequence)
    {
        if (m32_pxe_generic_cmd(opcode))
            return -5;
    }

    console::printf("PXE entry complete.\n");
    return 0;
}
