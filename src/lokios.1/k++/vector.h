#ifndef __KERNEL_VECTOR_H
#define __KERNEL_VECTOR_H

#include "vector_base.h"
#include "mm/page.h"

namespace kernel
{
    template<typename T>
    struct vector : public vector_base<T>
    {
        page_raii   _page;

        inline vector() try:
            vector_base<T>(page_alloc(),PAGE_SIZE/sizeof(T)),
            _page(this->_elems)
        {
        }
        catch (...)
        {
            page_free(this->_elems);
        }

        inline vector(size_t size) try:
            vector_base<T>(page_alloc(),PAGE_SIZE/sizeof(T),size),
            _page(this->_elems)
        {
        }
        catch (...)
        {
            page_free(this->_elems);
        }

        inline vector(const vector_base<T>& other) try:
            vector_base<T>(page_alloc(),PAGE_SIZE/sizeof(T),other),
            _page(this->_elems)
        {
        }
        catch (...)
        {
            page_free(this->_elems);
        }

        inline vector(const vector<T>& other):
            vector((const vector_base<T>&)other) {}

        inline vector(vector&& other):
            vector_base<T>(other._elems,other._capacity)
        {
            this->_size = other._size;

            void* tmp        = _page.addr;
            _page.addr       = other._page.addr;
            other._page.addr = tmp;
            other._elems     = (T*)tmp;
            other._size      = 0;
        }
    };
}

#endif /* __KERNEL_VECTOR_H */
