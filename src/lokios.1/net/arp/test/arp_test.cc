#include "../arp.h"
#include "net/eth/traits.h"
#include <tmock/tmock.h>

typedef arp::service<eth::net_traits,ipv4::net_traits> ipv4_arp_service;
KASSERT(sizeof(ipv4_arp_service::ll_arp_frame) == 42);

TMOCK_MAIN();
