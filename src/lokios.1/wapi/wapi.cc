#include "wapi.h"
#include "mm/mm.h"

using kernel::_kassert;

static const char rsp_404[] =
    "HTTP/1.1 404 Not Found\r\n"
    "\r\n"
    "404 Not Found\r\n";
static kernel::dma_alp alp_404 = {kernel::virt_to_phys(rsp_404),
                                  sizeof(rsp_404)-1};

void
wapi::send_404_response(tcp::socket* s)
{
    s->send(1,&alp_404);
    s->close_send();
}

wapi::node*
wapi::find_node_for_path(const char* path)
{
    kassert(strlen(path) <= HTTP_MAX_PATH_LEN);

    char segment[HTTP_MAX_PATH_LEN+1] = {'\0'};
    const char* src                   = path;
    char* dst                         = segment;
    wapi::node* n                     = wapi::root_node;
    for (;;)
    {
        if (*src == '\0' || *src == '/')
        {
            if (segment[0] != '\0')
                n = n->find_child(segment);
            if (!n)
                return NULL;
            if (*src == '\0')
                return n;

            ++src;
            dst = segment;
        }
        else
            *dst++ = *src++;

        *dst = '\0';
    }
}

wapi::node::node(const char* fmt, va_list ap)
{
    name.vprintf(fmt,ap);
}

wapi::node::node(const char* fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    name.vprintf(fmt,ap);
    va_end(ap);
}

void
wapi::node::register_child(wapi::node* c)
{
    kassert(find_child(c->name) == NULL);
    children.push_back(&c->link);
    c->parent = this;
}

wapi::node*
wapi::node::find_child(const char* name)
{
    for (auto& c : klist_elems(children,link))
    {
        if (!strcmp(c.name,name))
            return &c;
    }
    return NULL;
}
