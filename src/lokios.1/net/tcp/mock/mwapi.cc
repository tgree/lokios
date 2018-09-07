#include "../tcp.h"
#include <tmock/tmock.h>

void
tcp::handle_wapi_request(wapi::node* n, http::request* req, json::object* obj,
    http::response* rsp)
{
    mock("tcp::handle_wapi_request",n,req,obj,rsp);
}
