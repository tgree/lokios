#ifndef __KERNEL_ASSERT_H
#define __KERNEL_ASSERT_H

#include <stdint.h>

void
vgawrite(uint8_t x, uint8_t y, const char* s,
         uint16_t cflags = 0x4F00) noexcept;

void
halt() noexcept __attribute__((noreturn));

void
panic(const char* s = "") noexcept __attribute__((noreturn));

#define kassert(exp) if (!(exp)) panic(#exp)
#define KASSERT(exp) static_assert(exp, #exp)

#endif /* __KERNEL_ASSERT_H */
