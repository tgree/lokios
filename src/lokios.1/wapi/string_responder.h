#ifndef __KERNEL_WAPI_STRING_RESPONDER_H
#define __KERNEL_WAPI_STRING_RESPONDER_H

#include "wapi.h"
#include "http/response.h"

namespace wapi
{
    typedef kernel::delegate<void(const http::request* req,
                             http::string_response* rsp,
                             void* cookie)> string_responder_delegate;

    struct string_responder_node : public wapi::node
    {
        string_responder_delegate   handler;
        void*                       cookie;
        uint64_t                    method_mask;

        virtual void handle_request(const http::request* r, tcp::socket* s);
                void send_cb(tcp::send_op*);

        string_responder_node(string_responder_delegate handler, void* cookie,
                              uint64_t method_mask, const char* fmt,
                              va_list ap);
        string_responder_node(string_responder_delegate handler, void* cookie,
                              uint64_t method_mask, const char* fmt, ...);
    };

    string_responder_node* register_string_responder(wapi::node* parent,
            string_responder_delegate handler, void* cookie,
            uint64_t method_mask, const char* fmt, ...);
}

#endif /* __KERNEL_WAPI_STRING_RESPONDER_H */
