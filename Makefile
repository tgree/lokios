lokios: lokios.raw
	cp lokios.raw lokios

lokios.raw: lokios.elf
	objcopy -O binary -S lokios.elf lokios.raw

lokios.elf: lokios.o
	ld -melf_i386 -T lokios.ld -o lokios.elf lokios.o

lokios.o: lokios.S
	as -march=i386 --32 -o lokios.o lokios.S

.PHONY: clean
clean:
	rm -f lokios.elf lokios.raw lokios *.o
