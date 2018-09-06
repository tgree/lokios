#include "../response.h"
#include <tmock/tmock.h>

void
http::response::printf(const char* fmt, ...)
{
    mock("http::response::printf",fmt);
}

void
http::response::headerf(const char* fmt, ...)
{
    mock("http::response::headerf",fmt);
}

tcp::send_op*
http::response::send(tcp::socket* s)
{
    return (tcp::send_op*)mock("http::response::send",s);
}

tcp::send_op*
http::response::send_error(tcp::socket* s)
{
    return (tcp::send_op*)mock("http::response::send_error",s);
}
