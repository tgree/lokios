OBJS := lokios.o console.o entry.o
ASFLAGS := -march=i386 --32
NOW := $(shell date +"%c")

lokios.0: lokios.elf
	objcopy -O binary -S lokios.elf lokios.0

lokios.elf: $(OBJS) lokios.ld
	echo '.data' > timestamp.s
	echo '.globl _BUILD_TIME' >> timestamp.s
	echo '_BUILD_TIME: .asciz "$(NOW)"' >> timestamp.s
	$(AS) $(ASFLAGS) -o timestamp.o timestamp.s
	ld -melf_i386 -T lokios.ld -o lokios.elf $(OBJS) timestamp.o

.PHONY: clean
clean:
	rm -f lokios.elf lokios.0 *.o timestamp.s
