MODULES := lokios.0 lokios.1
CLEAN   := bin/* lokios.mbr
NOW     := $(shell date +"%c")

I386_16_ASFLAGS := -march=i386 --32

.PHONY: all
all: bin/lokios.0 bin/lokios.1 lokios.mbr

include $(patsubst %, %/module.mk, $(MODULES))

bin/lokios.0: lokios.0/lokios.0.elf
	objcopy -O binary -S lokios.0/lokios.0.elf bin/lokios.0

bin/lokios.1: lokios.1/lokios.1.elf
	objcopy -O binary -S lokios.1/lokios.1.elf bin/lokios.1

lokios.mbr: bin/lokios.0 bin/lokios.1
	cat bin/lokios.0 bin/lokios.1 > lokios.mbr

.PHONY: clean
clean:
	rm -f $(CLEAN)
