#ifndef __KERN_NET_SYSLOGGER_H
#define __KERN_NET_SYSLOGGER_H

#include "eth.h"
#include "udp.h"
#include "../schedule.h"

namespace net
{
    struct syslog_pkt
    {
        eth::header     llhdr;
        ipv4::header    iphdr;
        udp::header     uhdr;
        char            payload[16];
    };

    struct syslogger
    {
        eth::interface*     intf;
        eth::addr           gw_mac;
        eth::tx_op          send_op;
        syslog_pkt          pkt;
        kernel::work_entry  arp_cqe;

        void post_message();
        void start();

        void handle_arp_completion(kernel::work_entry* wqe);
        void handle_tx_send_comp();

        syslogger(eth::interface* intf);
    };
}

#endif /* __KERN_NET_SYSLOGGER_H */
