#include "wapi.h"
#include "domain.h"
#include "pci.h"
#include "wapi/string_responder.h"

static void
pci_request(const http::request* req, http::string_response* rsp, void* cookie);

wapi::string_responder_node
    kernel::pci::wapi_node(func_delegate(pci_request),NULL,METHOD_GET_MASK,
                           "pci");

static void
pci_request(const http::request* req, http::string_response* rsp, void* cookie)
{
    // GET /pci
    rsp->printf("HTTP/1.1 200 OK\r\n\r\nDomains:\r\n");
    for (auto& d : kernel::pci::domains)
        rsp->printf("%04X\r\n",d.id);
}

static void
pci_domain_request(const http::request* req, http::string_response* rsp,
    void* cookie)
{
    // GET /pci/0000
    auto* d = (kernel::pci::domain*)cookie;
    rsp->printf("HTTP/1.1 200 OK\r\n\r\nDevices:\r\n");
    for (auto& pd : klist_elems(d->devices,domain_link))
    {
        rsp->printf("%04X:%02X:%u.%u %04X %04X %s\r\n",
                    pd.domain->id,pd.bus,(pd.devfn >> 3),(pd.devfn & 7),
                    pd.config_read_16(0),pd.config_read_16(2),pd.owner->name);
    }
}

void
kernel::pci::wapi_register(kernel::pci::domain* d)
{
    wapi::register_string_responder(&kernel::pci::wapi_node,
                                    func_delegate(pci_domain_request),d,
                                    METHOD_GET_MASK,"%04X",d->id);
}

void
kernel::pci::wapi_register()
{
    wapi::root_node->register_child(&kernel::pci::wapi_node);
}
