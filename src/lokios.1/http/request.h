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
        http::method                            method = http::METHOD_UNKNOWN;
        uint16_t                                version = 0;
        bool                                    done = false;
        const char*                             request_target = NULL;
        hash::table<const char*,const char*>    headers;
        kernel::string                          header_table;
        kernel::string                          body;

        // Check if we are done.
        inline bool is_done() const {return done;}

        // Used to feed more data into the parser.  Returns the number of
        // characters consumed.
        size_t  parse(const char* p, size_t len);
        void    parse_header();
    };
}

#endif /* __KERNEL_HTTP_REQUEST_H */
