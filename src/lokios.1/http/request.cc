#include "request.h"

using kernel::_kassert;

static uint64_t
to_u64(const char* p)
{
    if (*p == '\0')
        throw kernel::not_a_number_exception();

    uint64_t val = 0;
    while (*p)
    {
        if (!kisdigit(*p))
            throw kernel::not_a_number_exception();
        val *= 10;
        val += *p - '0';
        ++p;
    }
    return val;
}

size_t
http::request::parse(const char* start, size_t rem)
{
    size_t len;
    size_t total = 0;
    while (rem && !is_done())
    {
        switch (state)
        {
            case PARSING_HEADER:
                len    = read_more_header(start,rem);
                start += len;
                rem   -= len;
            break;

            case PARSING_BODY:
                len    = read_more_body(start,rem);
                start += len;
                rem   -= len;
            break;

            case DONE:
            break;
        }
    }
    return total;
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

size_t
http::request::read_more_header(const char* start, size_t rem)
{
    const char* p   = start;
    const char* end = start + rem;
    while (p != end && state == PARSING_HEADER)
    {
        const char* base = p;
        while (p != end && *p++ != '\n')
            ;

        header_table.append(base,p-base);
        if (!header_table.ends_with("\r\n\r\n"))
            continue;

        parse_header();

        if (headers.contains("Transfer-Encoding"))
            throw header_invalid_exception();

        auto* n = headers.find_node("Content-Length");
        if (n)
        {
            rem_content_length = to_u64(n->v);
            state              = PARSING_BODY;
        }
        else
            state = DONE;
    }

    return p - start;
}

size_t
http::request::read_more_body(const char* p, size_t len)
{
    // We can't continue reading into the same string we used for the header
    // because reading the body could cause the string to expand to a higher-
    // order buddy allocation which would move it in memory and invalidate all
    // the hash table pointers to header key/value substrings.
    len = MIN(rem_content_length,len);
    body.append(p,len);
    rem_content_length -= len;
    if (!rem_content_length)
        state = DONE;
    return len;
}
