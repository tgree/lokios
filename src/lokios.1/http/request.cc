#include "request.h"

using kernel::_kassert;

size_t
http::request::parse(const char* start, size_t rem)
{
    kassert(!is_done());

    const char* p = start;
    const char* end = start + rem;
    while (p != end)
    {
        const char* base = p;
        while (p != end && *p++ != '\n')
            ;

        size_t len = p - base;
        header_table.append(base,len);
        if (header_table.ends_with("\r\n\r\n"))
        {
            parse_header();
            done = true;
            break;
        }
    }

    return p - start;
}

void
http::request::parse_header()
{
    // Parse the method field.
    char* start = header_table.begin();
    char* p     = header_table.find(' ',start);
    if (p == header_table.end())
        throw header_invalid_exception();
    *p = '\0';
    method = get_method(start);
    if (method == METHOD_UNKNOWN)
        throw unrecognized_method_exception();

    // Parse the target field.
    start = p + 1;
    p     = header_table.find(' ',start);
    if (p == header_table.end())
        throw header_invalid_exception();
    *p = '\0';
    request_target = start;

    // Parse the HTTP version.
    start = p + 1;
    if (header_table.avail_after(start) < 10)
        throw header_invalid_exception();
    if (strncmp(start,"HTTP/",5))
        throw header_invalid_exception();
    if (!kisdigit(start[5]) || start[6] != '.' || !kisdigit(start[7]) ||
        start[8] != '\r' || start[9] != '\n')
    {
        throw header_invalid_exception();
    }
    version = ((start[5] - '0') << 4) |
              ((start[7] - '0') << 0);

    // Parse headers.
    start += 10;
    for (;;)
    {
        if (header_table.avail_after(start) < 2)
            throw header_invalid_exception();

        if (start[0] == '\r' && start[1] == '\n')
            break;

        const char* key = start;
        p = header_table.find(':',start);
        if (p == header_table.end())
            throw header_invalid_exception();
        *p = '\0';
        ++p;

        while (kisblank(*p) && p != header_table.end())
            ++p;
        if (p == header_table.end())
            throw header_invalid_exception();
        const char* val = p;
        p = header_table.find('\n',p);
        if (p[-1] != '\r')
            throw header_invalid_exception();
        p[-1] = '\0';

        headers.emplace(key,val);

        start = p + 1;
    }
}
