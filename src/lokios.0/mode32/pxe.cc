#include "pxe.h"
#include "console.h"
#include "massert.h"
#include "kernel/image.h"
#include <string.h>

extern far_pointer _saved_es_bx;
extern uint8_t _kernel_base[];

static far_pointer pxe_entry_fp;
static uint8_t server_ip[4];

extern "C" uint16_t _call_pxe(uint16_t opcode, void* pb, far_pointer proc_ptr);

static uint8_t
pxe_checksum(void* _p, uint32_t n)
{
    uint8_t sum = 0;
    uint8_t* p  = (uint8_t*)_p;
    while (n--)
        sum += *p++;
    return sum;
}

static int
pxe_find_entry(far_pointer* fp)
{
    // Checksum the PXENV+ struct.
    auto* pep = (pxe_env_plus*)_saved_es_bx.to_addr();
    uint8_t cs = pxe_checksum(pep,pep->len);
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
    cs = pxe_checksum(np,np->len);
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
pxe_generic_cmd(uint16_t opcode)
{
    pxe_generic_pb pb;
    memset(&pb,0,sizeof(pb));
    return _call_pxe(opcode,&pb,pxe_entry_fp) ?: pb.status;
}

static uint16_t
pxe_get_cached_dhcp_ack(far_pointer* fp, uint16_t* size)
{
    pxe_get_cached_info_pb pb;
    memset(&pb,0,sizeof(pb));
    pb.packet_type = 2;

    uint16_t rc = _call_pxe(0x0071,&pb,pxe_entry_fp);
    if (rc)
        return rc;
    if (pb.status)
        return pb.status;

    *fp   = pb.buffer_ptr;
    *size = pb.buffer_size;
    return 0;
}

static uint16_t
tftp_open(uint8_t* server_ip, const char* filename)
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

    return _call_pxe(0x0020,&pb,pxe_entry_fp) ?: pb.status;
}

static uint16_t
tftp_read(void* buf, uint16_t* packet_num, uint16_t* size)
{
    tftp_read_pb pb;
    memset(&pb,0,sizeof(pb));
    pb.buffer_fp = far_pointer{(uint16_t)(uint32_t)buf,0};

    uint16_t rc = _call_pxe(0x0022,&pb,pxe_entry_fp);
    if (rc)
        return rc;
    if (pb.status)
        return pb.status;

    *packet_num = pb.packet_num;
    *size       = pb.buffer_size;
    return 0;
}

static uint16_t
tftp_close()
{
    return pxe_generic_cmd(0x0021);
}

struct pxe_image_stream : public image_stream
{
    uint16_t expected_packet_num;

    virtual int open() override
    {
        // Reset to the first packet.
        expected_packet_num = 1;

        // Open the TFTP stream.
        return tftp_open(server_ip,"lokios.1.elf");
    }

    virtual int read(void* dst, uint16_t nsectors) override
    {
        // Read all the sectors from the TFTP stream.
        uint8_t buf[512];
        auto* pos            = (char*)dst;
        uint16_t packet_size = sizeof(buf);
        uint16_t packet_num;
        while (nsectors--)
        {
            int err = tftp_read(buf,&packet_num,&packet_size);
            if (err)
                return err;
            if (packet_num != expected_packet_num)
                return -1;
            if (packet_size == 0)
                return -2;
            if (packet_size != 512 && nsectors)
                return -3;
            if (packet_size > sizeof(buf))
                return -4;

            memcpy(pos,buf,packet_size);
            ++expected_packet_num;
            pos += packet_size;
        }

        // Zap the end of the last sector if it was short.  Since we're
        // streaming a file there's no guarantee that it's a multiple of the
        // sector size.
        memset(pos,0xEE,sizeof(buf)-packet_size);

        return 0;
    }

    virtual int close() override
    {
        // Close the TFTP stream.
        int err = tftp_close();
        if (err)
            return err;

        // Issue the PXE shutdown sequence.
        uint16_t shutdown_sequence[] = {
            //  0x0007, // UNDI Close - DELL doesn't like this error 0x6A
                0x0005, // UNDI Shutdown
                0x0076, // Stop Base
            //  0x0070, // Unload Stack - DELL doesn't like this one error 0x01
                0x0002, // UNDI Cleanup
            //  0x0015, // Stop UNDI - iPXE doesn't seem to like this one
        };
        for (uint16_t opcode : shutdown_sequence)
        {
            if (pxe_generic_cmd(opcode))
                return -5;
        }
        return 0;
    }

    constexpr pxe_image_stream():
        image_stream("PXE"),
        expected_packet_num(0)
    {
    }
};

static pxe_image_stream pxe_stream;

image_stream*
pxe_entry()
{
    // Find the entry point.
    int err = pxe_find_entry(&pxe_entry_fp);
    if (err)
    {
        console::printf("Error %d finding PXE entry point.\n",err);
        return NULL;
    }

    // Get cached data to find the server address.
    far_pointer dhcp_fp;
    uint16_t dhcp_size;
    uint16_t rc = pxe_get_cached_dhcp_ack(&dhcp_fp,&dhcp_size);
    if (rc != 0)
    {
        console::printf("Error %u getting cached DHCP ACK\n",rc);
        return NULL;
    }
    memcpy(server_ip,(uint8_t*)dhcp_fp.to_addr() + 20,sizeof(server_ip));
    console::printf("DHCP Server IP: %u.%u.%u.%u\n",
                    server_ip[0],server_ip[1],server_ip[2],server_ip[3]);

    return &pxe_stream;
}
