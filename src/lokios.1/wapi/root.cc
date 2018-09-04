#include "wapi.h"
#include "mm/mm.h"

using kernel::_kassert;

#define CANNED_ALP(str) {kernel::virt_to_phys(str),sizeof(str)-1}

static const char rsp_ver[] =
    "HTTP/1.1 200 OK\r\n"
    "\r\n"
    "LokiOS 1.0";
static kernel::dma_alp alp_ver = CANNED_ALP(rsp_ver);

struct wapi_root_node : public wapi::node
{
    virtual void handle_request(const http::request* r, tcp::socket* s)
    {
        s->send(1,&alp_ver);
        s->close_send();
    }

    using wapi::node::node;
};

static wapi_root_node _root_node("");
wapi::node* wapi::root_node = &_root_node;
