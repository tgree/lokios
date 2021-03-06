#ifndef __KERNEL_WAPI_WAPI_H
#define __KERNEL_WAPI_WAPI_H

#include "http/request.h"
#include "http/response.h"
#include "http/json.h"
#include "k++/klist.h"
#include "k++/string_stream.h"

namespace wapi
{
    struct node;

    struct response_exception {};
    struct not_found_exception : public response_exception {};
    struct bad_request_exception : public response_exception {};
    struct method_not_allowed_exception : public response_exception
    {
        uint64_t method_mask;
        constexpr method_not_allowed_exception(uint64_t method_mask):
            method_mask(method_mask) {}
    };

    typedef kernel::delegate<void(wapi::node*, http::request*, json::object*,
                                  http::response*)> delegate;
    struct node
    {
        kernel::kdlink                  link;
        wapi::node*                     parent = NULL;
        kernel::kdlist<wapi::node>      children;
        kernel::fixed_string_stream<32> name;
        const uint64_t                  method_mask;
        wapi::delegate                  handler;

        void        register_child(wapi::node* c);
        wapi::node* find_child(const char* name, size_t len);

        void        deregister();

        node(wapi::delegate handler, uint64_t method_mask,
             const char* fmt, ...);

        // Use with caution: the parent node must already have been initialized
        // and this isn't necessarily guaranteed with cross-compilation unit
        // globals!
        node(node* parent, wapi::delegate handler, uint64_t method_mask,
             const char* fmt, ...);

    protected:
        // Records the parent but doesn't link us up.  For use by global_node.
        node(node* parent, wapi::delegate handler, uint64_t method_mask);
    };

    struct global_node : public node
    {
        kernel::klink   glink;

        global_node(node* parent, wapi::delegate handler, uint64_t method_mask,
                    const char* fmt, ...);
    };

    extern wapi::node root_node;

    wapi::node* find_node_for_path(const char* path);
    void link_gnodes();
}

#endif /* __KERNEL_WAPI_WAPI_H */
