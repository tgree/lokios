#include "dhcp.h"

using kernel::_kassert;

void
dhcp::format_discover(dhcp::message* msg, uint32_t xid,
    const eth::addr& src_mac)
{
    msg->op                  = 1;
    msg->htype               = 1;
    msg->hlen                = 6;
    msg->hops                = 0;
    msg->xid                 = xid;
    msg->secs                = 0;
    msg->flags               = 0x8000;
    msg->ciaddr              = ipv4::addr{0,0,0,0};
    msg->yiaddr              = ipv4::addr{0,0,0,0};
    msg->siaddr              = ipv4::addr{0,0,0,0};
    msg->giaddr              = ipv4::addr{0,0,0,0};
    *(eth::addr*)msg->chaddr = src_mac;

    msg->magic = DHCP_OPTIONS_MAGIC;
    msg->options[0] = 0xFF;

    append_message_type(msg,DHCP_DISCOVER);
}

void
dhcp::format_request(dhcp::message* msg, uint32_t xid,
    const eth::addr& src_mac, const ipv4::addr& req_addr,
    const ipv4::addr& server_id)
{
    msg->op                  = 1;
    msg->htype               = 1;
    msg->hlen                = 6;
    msg->hops                = 0;
    msg->xid                 = xid;
    msg->secs                = 0;
    msg->flags               = 0x8000;
    msg->ciaddr              = ipv4::addr{0,0,0,0};
    msg->yiaddr              = ipv4::addr{0,0,0,0};
    msg->siaddr              = ipv4::addr{0,0,0,0};
    msg->giaddr              = ipv4::addr{0,0,0,0};
    *(eth::addr*)msg->chaddr = src_mac;

    msg->magic = DHCP_OPTIONS_MAGIC;
    msg->options[0] = 0xFF;

    append_message_type(msg,DHCP_REQUEST);
    append_requested_ip(msg,req_addr);
    append_server_id(msg,server_id);
}

void
dhcp::append_option(dhcp::message* msg, uint8_t tag, uint8_t len,
    const void* data)
{
    uint8_t* p = msg->options;
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
    kassert(p + 3 + len <= msg->options + kernel::nelems(msg->options));

    p[0]       = tag;
    p[1]       = len;
    p[2 + len] = 0xFF;
    memcpy(p + 2,data,len);
}

dhcp::option*
dhcp::find_option(dhcp::message* msg, uint8_t tag)
{
    uint8_t* p = msg->options;
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
