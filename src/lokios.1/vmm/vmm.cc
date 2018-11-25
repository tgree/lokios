#include "vmm.h"
#include "image.h"
#include "mm/slab.h"

void
vmm::init_vmm()
{
}

static void
vmm_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // GET /vmm
    rsp->printf("{\r\n"
                "    \"image_count\" : %zu\r\n"
                "}\r\n",
                vmm::images.size());
}

wapi::global_node vmm::wapi_node(&wapi::root_node,func_delegate(vmm_request),
                                 METHOD_GET_MASK,"vmm");
