#include "texpectation.h"
#include "tmock.h"
#include <string.h>

static tmock::expectation* head;
static tmock::expectation* tail;

const tmock::targ*
tmock::call::find_arg(const char* name) const
{
    for (size_t i=0; i<nargs; ++i)
    {
        auto* a = &call_args[i];
        if (!strcmp(name,a->name))
            return a;
    }
    return NULL;
}

void
tmock::_expect(expectation* e)
{
    TASSERT(!e->armed);
    e->next = NULL;
    if (tail)
        tail->next = e;
    else
        head = e;
    tail = e;
    e->armed = true;
}

uintptr_t
tmock::_mock_call(const char* fname, const call* mc)
{
    if (head == NULL)
    {
        printf("Unexpected call (expected nothing): %s\n",fname);
        ::abort();
    }
    else if(head->fname != fname)
    {
        printf("Unexpected call (expected %s): %s\n",head->fname,fname);
        ::abort();
    }
    
    uintptr_t rv = 0;
    for (size_t i=0; i<head->nconstraints; ++i)
    {
        const constraint* c = &head->constraints[i];
        switch (c->type)
        {
            case constraint::ARGUMENT:
            {
                auto* pc = mc->find_arg(c->want_arg.name);
                TASSERT(pc != NULL);
                TASSERT(c->want_arg.value == pc->value);
            }
            break;

            case constraint::RETURN_VALUE:
                rv = c->return_value.value;
            break;

            case constraint::CAPTURE:
            {
                auto* pc = mc->find_arg(c->capture_arg.name);
                TASSERT(pc != NULL);
                *c->capture_arg.dst = pc->value;
            }
            break;
        }
    }

    head->armed = false;
    head = head->next;
    if (!head)
        tail = NULL;

    return rv;
}

void
tmock::cleanup_expectations()
{
    TASSERT(head == NULL);
}
