/*
 * JSON parser.
 * Useful reference:
 *
 *  http://www.json.org
 */
#include "json.h"
#include "hdr/chartype.h"
#include <stdio.h>

size_t
json::parse_object(char* s, json::object* obj)
{
    // object:
    //  '{' ws '}'
    //  '{' members '}'
    char* start = s;

    if (*s != '{')
        throw syntax_error_exception();
    ++s;

    s += parse_whitespace(s);

    if (*s == '}')
        return s + 1 - start;

    s += parse_members(s,s,obj);

    if (*s != '}')
        throw syntax_error_exception();

    return s + 1 - start;
}

size_t
json::parse_whitespace(char* s)
{
    // whitespace:
    //  ""
    //  ' '
    //  '\t'
    //  '\r'
    //  '\n'
    char* start = s;

    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
        ++s;

    return s - start;
}

size_t
json::parse_members(char* start, char* s, json::object* obj)
{
    // members:
    //  member
    //  member ',' members
    s += parse_member(s,obj);
    if (*s != ',')
        return s - start;
    return parse_members(start,s + 1,obj);
}

size_t
json::parse_member(char* s, json::object* obj)
{
    // member:
    //  ws string ws ':' element
    char* start = s;
    s += parse_whitespace(s);
    char* key = s + 1;
    s += parse_string(s);
    s += parse_whitespace(s);
    if (*s != ':')
        throw syntax_error_exception();
    ++s;
    char* val;
    s += parse_element(s,&val);
    obj->emplace(key,val);
    return s - start;
}

size_t
json::parse_element(char* s, char** v)
{
    // element:
    //  ws value ws
    // Note: we only support string types for the value.
    char* start = s;
    s += parse_whitespace(s);
    *v = s + 1;
    s += parse_string(s);
    s += parse_whitespace(s);
    return s - start;
}

size_t
json::parse_string(char* s)
{
    // string:
    //  '"' characters '"'
    char* start = s;
    if (*s != '"')
        throw syntax_error_exception();
    ++s;

    s += parse_characters(s,s);
    if (*s != '"')
        throw syntax_error_exception();
    *s = '\0';
    ++s;

    return s - start;
}

size_t
json::parse_characters(char* start, char* s)
{
    // characters:
    //  ""
    //  character characters
    size_t len = parse_character(s);
    if (!len)
        return s - start;
    return parse_characters(start,s + len);
}

size_t
json::parse_character(char* s)
{
    // character:
    //  '0020' . '10ffff' - '"' - '\'
    //  '\' escape
    // escape:
    //  '"'
    //  '\'
    //  '/'
    //  'b'
    //  'n'
    //  'r'
    //  't'
    //  'u' hex hex hex hex
    if (*s < 0x20 || *s == '"')
        return 0;
    if (*s == '\\')
    {
        switch (*++s)
        {
            case '"':
            case '\\':
            case '/':
            case 'b':
            case 'n':
            case 'r':
            case 't':
                return 2;
            break;

            case 'u':
                if (!kisxdigit(s[0]) || !kisxdigit(s[1]) ||
                    !kisxdigit(s[2]) || !kisxdigit(s[3]))
                {
                    throw syntax_error_exception();
                }
                return 5;
            break;

            default:
                return 0;
            break;
        }
    }
    return 1;
}
