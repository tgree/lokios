#ifndef __KERNEL_WAPI_WAPI_H
#define __KERNEL_WAPI_WAPI_H

#include "http/request.h"
#include "net/interface.h"
#include "k++/klist.h"
#include "k++/string_stream.h"

namespace wapi
{
    class node
    {
        kernel::kdlink              link;
        wapi::node*                 parent = NULL;
        kernel::kdlist<wapi::node>  children;

    protected:
        kernel::fixed_string_stream<32> name;

        node() = default; // Initialize name yourself.

    public:
        void        register_child(wapi::node* c);
        wapi::node* find_child(const char* name, size_t len);

        virtual void handle_request(const http::request* r, tcp::socket* s) = 0;

        node(const char* fmt, va_list ap);
        node(const char* fmt, ...);
    };

    // Some canned responses.
    void send_404_response(tcp::socket* s);

    wapi::node* find_node_for_path(const char* path);

    extern wapi::node* root_node;
}

#endif /* __KERNEL_WAPI_WAPI_H */
