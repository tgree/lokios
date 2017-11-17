OBJS = lokios.o console.o entry.o
ASFLAGS = -march=i386 --32

lokios: lokios.raw
	cp lokios.raw lokios

lokios.raw: lokios.elf
	objcopy -O binary -S lokios.elf lokios.raw

lokios.elf: $(OBJS) lokios.ld
	ld -melf_i386 -T lokios.ld -o lokios.elf $(OBJS)

.PHONY: clean
clean:
	rm -f lokios.elf lokios.raw lokios *.o
