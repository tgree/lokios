#include "domain.h"
#include "pci.h"

static void
pci_domain_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // GET /pci/0000
    auto* d = container_of(node,kernel::pci::domain,wapi_node);
    rsp->printf("{\r\n"
                "    \"first_bus_num\" : \"0x%02X\",\r\n"
                "    \"last_bus_num\"  : \"0x%02X\",\r\n"
                "    \"devices\"       : [",
                d->first_bus_num,d->last_bus_num);
    for (auto& pd : klist_elems(d->devices,domain_link))
    {
        rsp->printf("\r\n        { "
                    "\"slot\" : \"%02X:%u.%u\", "
                    "\"ht\" : \"0x%02X\", "
                    "\"vendor_id\" : \"0x%04X\", "
                    "\"device_id\" : \"0x%04X\", "
                    "\"driver\" : \"%s\" "
                    "},",
                    pd.bus,(pd.devfn >> 3),(pd.devfn & 7),
                    pd.config_read_8(14),pd.config_read_16(0),
                    pd.config_read_16(2),pd.owner->name);
    }
    rsp->ks.shrink();
    rsp->printf("\r\n    ]\r\n}\r\n");
}

kernel::pci::domain::domain(uint16_t id, uint8_t first_bus_num,
    uint8_t last_bus_num, config_accessor* cfg):
        id(id),
        first_bus_num(first_bus_num),
        last_bus_num(last_bus_num),
        cfg(cfg),
        wapi_node(&kernel::pci::wapi_node,func_delegate(pci_domain_request),
                  METHOD_GET_MASK,"%04X",id)
{
    wapi_node.register_child(&cfg->wapi_node);
}
