#ifndef __KERNEL_HTTP_REQUEST_H
#define __KERNEL_HTTP_REQUEST_H

#include "http.h"
#include "http/string.h"
#include "net/interface.h"
#include "k++/hash_table.h"
#include "hdr/chartype.h"

namespace http
{
    struct header_invalid_exception : public http::exception {};
    struct unrecognized_method_exception : public http::exception {};
    struct header_path_too_long_exception : public http::exception {};
    struct header_key_too_long_exception : public http::exception {};
    struct header_val_too_long_exception : public http::exception {};

    using request_target_string = http::string<HTTP_MAX_PATH_LEN,
                                               header_path_too_long_exception>;
    using header_key_string = http::string<HTTP_MAX_HEADER_KEY_LEN,
                                           header_key_too_long_exception>;
    using header_val_string = http::string<HTTP_MAX_HEADER_VAL_LEN,
                                           header_val_too_long_exception>;

    struct request
    {
        enum
        {
            // The request line.
            PARSING_METHOD,
            PARSING_REQUEST_TARGET,
            PARSING_HTTP_VERSION_HTTP_SLASH,
            PARSING_HTTP_VERSION_MAJOR,
            PARSING_HTTP_VERSION_DOT,
            PARSING_HTTP_VERSION_MINOR,
            PARSING_REQUEST_LINE_CR,
            PARSING_REQUEST_LINE_LF,

            // Waiting to see if we will parse a header or the final CRLF.
            PARSING_WAIT_NEXT_HEADER,

            // Parsing a header line.
            PARSING_HEADER_KEY,
            PARSING_HEADER_VAL,
            PARSING_HEADER_LF,

            // Parsing the final LF.
            PARSING_FINAL_LF,

            PARSING_DONE,
        } state;

        // Final state.
        http::method                                        method;
        uint8_t                                             http_slash_pos = 0;
        uint16_t                                            version = 0;
        request_target_string                               request_target;
        hash::table<header_key_string,header_val_string>    headers;

        // Intermediate parsing state.
        header_key_string   parsing_key;
        header_val_string*  parsing_val;

        // State checkers.
        bool    is_done() const {return state == PARSING_DONE;}

        // Used to feed more data into the parser.  Return 0 if this was the
        // last character in the header.
        int     parse(char c);

        request();
    };
}

#endif /* __KERNEL_HTTP_REQUEST_H */
