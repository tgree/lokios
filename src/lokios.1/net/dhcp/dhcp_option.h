#ifndef __KERNEL_NET_DHCP_OPTION_H
#define __KERNEL_NET_DHCP_OPTION_H

#include "net/ip.h"
#include "kernel/cxx_exception.h"
#include "kernel/libc.h"
#include <stdint.h>
#include <cxxabi.h>
#include <typeinfo>

namespace dhcp
{
    struct option
    {
        uint8_t tag;
        uint8_t len;
        uint8_t data[];
    };

    struct option_exception : public kernel::message_exception
    {
        const uint8_t tag;
        const char* type;
        inline option_exception(const char* msg, uint8_t tag, const char* tn):
            kernel::message_exception(msg),
            tag(tag)
        {
            int status;
            type = abi::__cxa_demangle(tn,0,0,&status);
        }
        ~option_exception()
        {
            free((void*)type);
        }
    };

    struct missing_option_exception : public option_exception
    {
        inline missing_option_exception(uint8_t tag, const char* tn):
            option_exception("missing",tag,tn) {}
    };

    struct invalid_option_exception : public option_exception
    {
        inline invalid_option_exception(const char* msg, uint8_t tag,
                                        const char* tn):
            option_exception(msg,tag,tn) {}
    };

    template<uint8_t Tag>
    struct option_parser
    {
        static constexpr uint8_t tag = Tag;
        [[noreturn]] inline void throw_invalid(const char* msg)
        {
            throw invalid_option_exception(msg,tag,typeid(*this).name());
        }
        virtual ~option_parser() {}
    };

    template<uint8_t Tag, typename T>
    struct simple_option_parser : public option_parser<Tag>
    {
        T parse_option(const option* o)
        {
            if (o->len != sizeof(T))
                this->throw_invalid("invalid length");
            return *(const T*)o->data;
        }
    };

    enum message_type
    {
        DHCP_DISCOVER   = 1,
        DHCP_OFFER      = 2,
        DHCP_REQUEST    = 3,
        DHCP_DECLINE    = 4,
        DHCP_ACK        = 5,
        DHCP_NAK        = 6,
        DHCP_RELEASE    = 7,
        DHCP_INFORM     = 8,
    };

    struct message_type_option : public option_parser<53>
    {
        message_type parse_option(const option* o)
        {
            if (o->len != 1)
                throw_invalid("invalid length");
            switch (auto mt = (message_type)o->data[0]; mt)
            {
                case DHCP_DISCOVER:
                case DHCP_OFFER:
                case DHCP_REQUEST:
                case DHCP_DECLINE:
                case DHCP_ACK:
                case DHCP_NAK:
                case DHCP_RELEASE:
                case DHCP_INFORM:
                    return mt;
                break;
            }
            throw_invalid("invalid content");
        }
    };

    template<uint8_t Tag>
    struct address_option : public simple_option_parser<Tag,ipv4::addr> {};
    struct server_id_option : public address_option<54> {};
    struct subnet_mask_option : public address_option<1> {};
    struct router_option : public address_option<3> {};
    struct dns_option : public address_option<6> {};

    struct lease_option : public simple_option_parser<51,be_uint32_t> {};
    struct t1_option : public simple_option_parser<58,be_uint32_t> {};
    struct t2_option : public simple_option_parser<59,be_uint32_t> {};
}

#endif /* __KERNEL_NET_DHCP_OPTION_H */
