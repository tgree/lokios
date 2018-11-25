#include "image.h"
#include "vmm.h"
#include "mm/buddy.h"
#include "k++/checksum.h"

kernel::kdlist_leaks<vmm::image> vmm::images;
static kernel::slab images_slab(sizeof(vmm::image));

static void
vmm_images_post_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // POST /vmm/images
    // This must have a header, 'Image-Name:' to tell us which image to create.
    const char* name = req->headers["Image-Name"];
    if (strlen(name) >= 31)
        throw wapi::bad_request_exception();
    if (req->body.c_str() == NULL)
        throw wapi::bad_request_exception();

    size_t len  = req->body.capacity();
    void* addr  = req->body.steal();
    auto* image = images_slab.alloc<vmm::image>(name,addr,len);
    vmm::images.push_back(&image->link);
}

static void
vmm_images_get_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // GET /vmm/images
    rsp->printf("{\r\n"
                "    images : [ ");
    for (auto& i : klist_elems(vmm::images,link))
        rsp->printf(" \"%s\",",i.wapi_node.name.c_str());
    rsp->ks.shrink();
    rsp->printf(" ]\r\n}\r\n");
}

static void
vmm_images_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    switch (req->method)
    {
        case http::METHOD_GET:
            return vmm_images_get_request(node,req,obj,rsp);

        case http::METHOD_POST:
            return vmm_images_post_request(node,req,obj,rsp);

        default:
            throw wapi::method_not_allowed_exception(req->method);
    }
}

static wapi::node images_node(&vmm::wapi_node,func_delegate(vmm_images_request),
                              METHOD_GET_MASK | METHOD_POST_MASK,"images");

static void
image_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // GET /vmm/images/foo
    auto* image = container_of(node,vmm::image,wapi_node);
    rsp->printf("{\r\n"
                "    \"name\"   : \"%s\",\r\n"
                "    \"size\"   : %zu\r\n"
                "    \"md5sum\" : \"%02x%02x%02x%02x%02x%02x%02X%02x"
                "%02x%02x%02x%02x%02x%02x%02X%02x\""
                "}\r\n",
                image->wapi_node.name.c_str(),image->len,
                image->md5[0],image->md5[1],image->md5[2],image->md5[3],
                image->md5[4],image->md5[5],image->md5[6],image->md5[7],
                image->md5[8],image->md5[9],image->md5[10],image->md5[11],
                image->md5[12],image->md5[13],image->md5[14],image->md5[15]
                );
}

vmm::image::image(const char* name, const void* addr, size_t len):
    wapi_node(&images_node,func_delegate(image_request),METHOD_GET_MASK,name),
    addr(addr),
    len(len)
{
    kernel::md5sum(addr,len,md5);
}

vmm::image::~image()
{
    kernel::buddy_free_by_len(const_cast<void*>(addr),len);
}
