#include "wapi.h"

using kernel::_kassert;

wapi::node*
wapi::find_node_for_path(const char* path)
{
    const char* start = path;
    const char* p     = start;
    wapi::node* n     = &wapi::root_node;
    for (;;)
    {
        if (*p == '\0' || *p == '/')
        {
            if (p != start)
                n = n->find_child(start,p-start);
            if (!n)
                return NULL;
            if (*p == '\0')
                return n;

            start = p + 1;
        }
        ++p;
    }
}

wapi::node::node(wapi::delegate handler, uint64_t method_mask,
    const char* fmt, ...):
        method_mask(method_mask),
        handler(handler)
{
    va_list ap;
    va_start(ap,fmt);
    name.vprintf(fmt,ap);
    va_end(ap);
}

void
wapi::node::register_child(wapi::node* c)
{
    kassert(find_child(c->name,c->name.strlen()) == NULL);
    children.push_back(&c->link);
    c->parent = this;
}

wapi::node*
wapi::node::find_child(const char* name, size_t len)
{
    for (auto& c : klist_elems(children,link))
    {
        if (c.name.strlen() == len && !strncmp(c.name,name,len))
            return &c;
    }
    return NULL;
}

void
wapi::node::deregister()
{
    if (parent)
    {
        link.unlink();
        parent = NULL;
    }
}
