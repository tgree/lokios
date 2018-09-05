#include "http.h"
#include "hdr/types.h"
#include <string.h>

struct method_table_entry
{
    const char*     name;
    http::method    method;
};

static method_table_entry method_table[] = {
    {"GET",     http::METHOD_GET},
    {"HEAD",    http::METHOD_HEAD},
    {"POST",    http::METHOD_POST},
    {"PUT",     http::METHOD_PUT},
    {"DELETE",  http::METHOD_DELETE},
    {"CONNECT", http::METHOD_CONNECT},
    {"OPTIONS", http::METHOD_OPTIONS},
    {"TRACE",   http::METHOD_TRACE},
};

http::method
http::get_method(const char* key)
{
    for (auto& mte : method_table)
    {
        if (!strcmp(key,mte.name))
            return mte.method;
    }
    return http::METHOD_UNKNOWN;
}

const char*
http::get_method_name(http::method m)
{
    if (m < NELEMS(method_table))
        return method_table[m].name;
    return "UNKNOWN";
}
