#include "../wapi.h"
#include <tmock/tmock.h>

using kernel::_kassert;

wapi::node*
wapi::find_node_for_path(const char* path)
{
    return (wapi::node*)mock("wapi::find_node_for_path",path);
}

wapi::node::node(wapi::delegate handler, uint64_t method_mask,
    const char* fmt, ...):
        method_mask(method_mask),
        handler(handler)
{
    // Note: we don't fill in the name field since that would require pulling
    // in all of the k++ printf support.
}

wapi::node::node(wapi::node* parent, wapi::delegate handler,
    uint64_t method_mask, const char* fmt, ...):
        parent(parent),
        method_mask(method_mask),
        handler(handler)
{
    // Note: we don't fill in the name field since that would require pulling
    // in all of the k++ printf support.
}

void
wapi::node::register_child(wapi::node* c)
{
    mock("wapi::node::register_child",c);
}

wapi::node*
wapi::node::find_child(const char* name, size_t len)
{
    return (wapi::node*)mock("wapi::node::find_child",name,len);
}

void
wapi::node::deregister()
{
    mock("wapi::node::deregister");
}

static void
root_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    mock("root_request",node,req,obj,rsp);
}

wapi::node wapi::root_node(func_delegate(root_request),
                           METHOD_GET_MASK | METHOD_POST_MASK,"");
