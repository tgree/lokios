#ifndef __KERNEL_HTTP_HTTP_H
#define __KERNEL_HTTP_HTTP_H

#include "hdr/chartype.h"
#include <stddef.h>

namespace http
{
    struct exception {};

    enum method
    {
        METHOD_GET,
        METHOD_HEAD,
        METHOD_POST,
        METHOD_PUT,
        METHOD_DELETE,
        METHOD_CONNECT,
        METHOD_OPTIONS,
        METHOD_TRACE,

        METHOD_UNKNOWN,
    };
    static constexpr size_t NUM_METHODS = METHOD_UNKNOWN;
#define METHOD_GET_MASK     (1<<http::METHOD_GET)
#define METHOD_HEAD_MASK    (1<<http::METHOD_HEAD)
#define METHOD_POST_MASK    (1<<http::METHOD_POST)
#define METHOD_PUT_MASK     (1<<http::METHOD_PUT)
#define METHOD_DELETE_MASK  (1<<http::METHOD_DELETE)
#define METHOD_CONNECT_MASK (1<<http::METHOD_CONNECT)
#define METHOD_OPTIONS_MASK (1<<http::METHOD_OPTIONS)
#define METHOD_TRACE_MASK   (1<<http::METHOD_TRACE)

    const char* get_method_name(http::method m);
    method get_method(const char* name);

    CHARTYPE_CHECKER(istchar,0x57FFFFFFC7FFFFFE,0x03FF6CFA00000000);
}

#endif /* __KERNEL_HTTP_HTTP_H */
