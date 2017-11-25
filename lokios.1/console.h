#ifndef __KERNEL_CONSOLE_H
#define __KERNEL_CONSOLE_H

void
console_init();

void
console_putc(char c);

void
console_puts(const char* s);

#endif /* __KERNEL_CONSOLE_H */
