#include "wapi.h"
#include "dev/cmos.h"
#include "platform/platform.h"

static void
root_get(http::response* rsp)
{
    auto dt = kernel::read_cmos_date_time();
    rsp->printf("{\r\n"
                "    \"loki\"  : \"1.0\",\r\n"
                "    \"date\"  : \"%4lu-%02lu-%02lu %02lu:%02lu:%02lu\",\r\n"
                "    \"state\" : \"running\"\r\n"
                "}\r\n",
                dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second);
}

static void
root_set_state_stopped(tcp::send_op*)
{
    kernel::exit_guest(1);
}

static void
root_post(json::object& obj, http::response* rsp)
{
    bool should_stop = false;

    for (auto& n : obj)
    {
        if (!strcmp(n.k,"state"))
        {
            if (!strcmp(n.v,"stopped"))
                should_stop = true;
            else if (strcmp(n.v,"running"))
                throw wapi::bad_request_exception();
        }
        else
            throw wapi::bad_request_exception();
    }

    if (should_stop)
        rsp->send_comp_delegate = func_delegate(root_set_state_stopped);
}

static void
root_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    if (req->method == http::METHOD_POST)
        root_post(*obj,rsp);
    else
        root_get(rsp);
}

wapi::node wapi::root_node(func_delegate(root_request),
                           METHOD_GET_MASK | METHOD_POST_MASK,"");
