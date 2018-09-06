#include "domain.h"
#include "pci.h"

static void
pci_domain_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // GET /pci/0000
    auto* d = container_of(node,kernel::pci::domain,wapi_node);
    rsp->printf("{\r\n"
                "    \"devices\" : [");
    for (auto& pd : klist_elems(d->devices,domain_link))
    {
        rsp->printf("\r\n        { "
                    "\"slot\" : \"%02X:%u.%u\", "
                    "\"vendor_id\" : \"%04X\", "
                    "\"device_id\" : \"%04X\", "
                    "\"driver\" : \"%s\" "
                    "},",
                    pd.bus,(pd.devfn >> 3),(pd.devfn & 7),
                    pd.config_read_16(0),pd.config_read_16(2),pd.owner->name);
    }
    rsp->ks.shrink();
    rsp->printf("\r\n    ]\r\n}\r\n");
}

kernel::pci::domain::domain(uint16_t id, config_accessor* cfg):
    id(id),
    cfg(cfg),
    wapi_node(func_delegate(pci_domain_request),METHOD_GET_MASK,"%04X",id)
{
    wapi_node.register_child(&cfg->wapi_node);
    kernel::pci::wapi_node.register_child(&wapi_node);
}
