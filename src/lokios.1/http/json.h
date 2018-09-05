#ifndef __KERNEL_HTTP_JSON_H
#define __KERNEL_HTTP_JSON_H

#include "k++/hash_table.h"

namespace json
{
    struct exception {};
    struct syntax_error_exception : public exception {};

    typedef hash::table<const char*,const char*> object;

    // Parse a JSON object (dict).  This is destructive to the input string
    // since we replace trailing string quotes with null-terminators so that
    // we can build up an in-place string table.
    size_t parse_object(char* str, json::object* obj);

    // Parse sub-types.
    size_t parse_whitespace(char* s);
    size_t parse_members(char* start, char* s, json::object* obj);
    size_t parse_member(char* s, json::object* obj);
    size_t parse_element(char* s, char** v);
    size_t parse_string(char* s);
    size_t parse_characters(char* start, char* s);
    size_t parse_character(char* s);
}

#endif /* __KERNEL_HTTP_JSON_H */
