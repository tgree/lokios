#ifdef __cplusplus
extern "C" {
#endif

void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
size_t strlen(const char* s);
size_t strnlen(const char* s, size_t maxlen);

#ifdef __cplusplus
}
#endif
