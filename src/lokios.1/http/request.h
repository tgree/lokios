#ifndef __KERNEL_HTTP_REQUEST_H
#define __KERNEL_HTTP_REQUEST_H

#include "http.h"
#include "kern/kstring.h"
#include "k++/hash_table.h"

namespace http
{
    struct header_invalid_exception : public http::exception {};
    struct unrecognized_method_exception : public http::exception {};

    struct request
    {
        enum
        {
            PARSING_HEADER,
            PARSING_BODY,
            DONE,
        } state;
        uint16_t                                version = 0;
        http::method                            method = http::METHOD_UNKNOWN;
        const char*                             request_target = NULL;
        size_t                                  rem_content_length;
        hash::table<const char*,const char*>    headers;
        kernel::string                          header_table;
        kernel::string                          body;

        // Check if we are done.
        inline bool is_done() const {return state == DONE;}

        // Check if we should keep the socket alive.
        inline bool should_keepalive() const
        {
            auto* e = headers.find_node("Connection");
            return (e ? !strcmp(e->v,"keep-alive") : version >= 0x11);
        }

        // Reset the request so it can be reused.
        void    reset();

        // Used to feed more data into the parser.  Returns the number of
        // characters consumed.
        size_t  parse(const char* p, size_t len);
        void    parse_header();
        size_t  read_more_header(const char* p, size_t len);
        size_t  read_more_body(const char* p, size_t len);
    };
}

#endif /* __KERNEL_HTTP_REQUEST_H */
