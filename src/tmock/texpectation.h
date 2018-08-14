#ifndef __TMOCK_EXPECTATION_H
#define __TMOCK_EXPECTATION_H

#include <hdr/variadic_macros.h>
#include <hdr/compiler.h>
#include <stdint.h>
#include <stddef.h>

namespace tmock
{
    struct targ
    {
        const char* name;
        uintptr_t   value;
    };

    struct constraint
    {
        enum
        {
            ARGUMENT,
            RETURN_VALUE,
            CAPTURE,
        } type;

        union
        {
            targ want_arg;

            struct
            {
                uintptr_t value;
            } return_value;

            struct
            {
                const char* name;
                uintptr_t* dst;
            } capture_arg;
        };
    };

    struct expectation
    {
        expectation*    next;
        const char*     fname;
        bool            armed;
        size_t          nconstraints;
        constraint      constraints[];
    };

    struct call
    {
        size_t  nargs;
        targ    call_args[];

        const targ* find_arg(const char* name) const;
    };

    void _expect(expectation* e);
    uintptr_t _mock_call(const char* fname, const call* e);
    void cleanup_expectations();
}

// Construct an expectation without dynamically allocating memory.  Each
// expectation defines a static global associated with the expectation.  The
// global will be added to a linked list of expectations when you execute your
// texpect() line, arming the expectation.  The expectation will be dequeued
// and disarmed when the expected function call is invoked.  This means you
// can't arm the same texpect() multiple times, say in a loop, if it hasn't
// actually fired yet.
#define want(name,val) \
    tmock::constraint{tmock::constraint::ARGUMENT, \
                      {.want_arg = {#name,(uintptr_t)val}}}
#define returns(val) \
    tmock::constraint{tmock::constraint::RETURN_VALUE, \
                      {.return_value = {(uintptr_t)val}}}
#define capture(name,dst) \
    tmock::constraint{tmock::constraint::CAPTURE, \
                      {.capture_arg = {#name,dst}}}
#define texpect(fname,...) \
    do { \
        constexpr size_t nc = sizeof((tmock::constraint[]){__VA_ARGS__})/ \
                              sizeof(tmock::constraint); \
        static struct { \
            tmock::expectation* next; \
            const char* _fname; \
            bool armed; \
            size_t nconstraints; \
            tmock::constraint constraints[nc]; \
        } e = {NULL,fname,false,nc,{__VA_ARGS__}}; \
        tmock::_expect((tmock::expectation*)&e); \
    } while(false)

#define TEXPECT(fname,...) \
    __attribute__((constructor))                    \
    static void _tmock_static_expect ## __LINE__()  \
    {                                               \
        texpect(fname,__VA_ARGS__);                 \
    }

// Construct a mock function call without dynamically allocating memory.
#define _MOCK_ARG(_arg) tmock::targ{#_arg, (uintptr_t)_arg},
#define mock(fname,...) \
    ({ \
        struct { \
            size_t nargs; \
            tmock::targ call_args[VA_NARGS(__VA_ARGS__)]; \
        } c = {VA_NARGS(__VA_ARGS__), {VA_APPLY(_MOCK_ARG,__VA_ARGS__)}}; \
        tmock::_mock_call(fname,(tmock::call*)&c); \
    })

#endif /* __TMOCK_EXPECTATION_H */
