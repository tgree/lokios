#include "../socket.h"
#include <tmock/tmock.h>

tcp::socket::~socket()
{
    mock("tcp::socket::~socket");
}
