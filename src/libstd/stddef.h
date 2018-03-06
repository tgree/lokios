typedef unsigned long size_t;
typedef long ptrdiff_t;

#ifdef __cplusplus
namespace std
{
    typedef ::size_t size_t;
    typedef ::ptrdiff_t ptrdiff_t;
}
#endif

#define NULL 0

#define offsetof(type, member) __builtin_offsetof(type,member)
