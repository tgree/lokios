#include "dhcp.h"

using kernel::_kassert;

void
dhcp::message::format_discover(uint32_t _xid, const eth::addr& src_mac)
{
    op                  = 1;
    htype               = 1;
    hlen                = 6;
    hops                = 0;
    xid                 = _xid;
    secs                = 0;
    flags               = 0x8000;
    ciaddr              = ipv4::addr{0,0,0,0};
    yiaddr              = ipv4::addr{0,0,0,0};
    siaddr              = ipv4::addr{0,0,0,0};
    giaddr              = ipv4::addr{0,0,0,0};
    *(eth::addr*)chaddr = src_mac;

    magic = DHCP_OPTIONS_MAGIC;
    options[0] = 0xFF;

    append_message_type(DHCP_DISCOVER);
}

void
dhcp::message::format_request(uint32_t _xid, const eth::addr& src_mac,
    const ipv4::addr& req_addr, const ipv4::addr& server_id,
    const ipv4::addr& ci_addr)
{
    op                  = 1;
    htype               = 1;
    hlen                = 6;
    hops                = 0;
    xid                 = _xid;
    secs                = 0;
    flags               = 0x8000;
    ciaddr              = ci_addr;
    yiaddr              = ipv4::addr{0,0,0,0};
    siaddr              = ipv4::addr{0,0,0,0};
    giaddr              = ipv4::addr{0,0,0,0};
    *(eth::addr*)chaddr = src_mac;

    magic = DHCP_OPTIONS_MAGIC;
    options[0] = 0xFF;
    memset(options+1,0,sizeof(options)-1);

    append_message_type(DHCP_REQUEST);
    append_requested_ip(req_addr);
    if (server_id != ipv4::addr{0,0,0,0})
        append_server_id(server_id);
}

void
dhcp::message::append_option(uint8_t tag, uint8_t len, const void* data)
{
    uint8_t* p = options;
    while (*p != 0xFF)
    {
        if (p == 0)
        {
            ++p;
            continue;
        }
        else
            p += 2 + p[1];
    }
    kassert(p + 3 + len <= options + kernel::nelems(options));

    p[0]       = tag;
    p[1]       = len;
    p[2 + len] = 0xFF;
    memcpy(p + 2,data,len);
}

const dhcp::option*
dhcp::message::find_option(uint8_t tag) const
{
    const uint8_t* p = options;
    while (*p != 0xFF)
    {
        if (p == 0)
        {
            ++p;
            continue;
        }
        else if (*p == tag)
            return (dhcp::option*)p;
        else
            p += 2 + p[1];
    }
    return NULL;
}
