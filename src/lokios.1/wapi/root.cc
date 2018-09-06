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
root_set_state_rebooting(tcp::send_op*)
{
    kernel::reboot_guest();
}

static void
root_post(json::object& obj, http::response* rsp)
{
    enum
    {
        ACTION_NONE,
        ACTION_STOP,
        ACTION_REBOOT,
    } action = ACTION_NONE;

    for (auto& n : obj)
    {
        if (!strcmp(n.k,"state"))
        {
            if (!strcmp(n.v,"stopped"))
                action = ACTION_STOP;
            else if (!strcmp(n.v,"rebooting"))
                action = ACTION_REBOOT;
            else if (strcmp(n.v,"running"))
                throw wapi::bad_request_exception();
        }
        else
            throw wapi::bad_request_exception();
    }

    switch (action)
    {
        case ACTION_NONE:
        break;

        case ACTION_STOP:
            rsp->send_comp_delegate = func_delegate(root_set_state_stopped);
        break;

        case ACTION_REBOOT:
            rsp->send_comp_delegate = func_delegate(root_set_state_rebooting);
        break;
    }
}

static void
root_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    if (req->method == http::METHOD_POST)
    {
        if (!obj)
            throw wapi::bad_request_exception();
        root_post(*obj,rsp);
    }
    else
        root_get(rsp);
}

wapi::node wapi::root_node(func_delegate(root_request),
                           METHOD_GET_MASK | METHOD_POST_MASK,"");
