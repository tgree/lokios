MODULES := lokios.0
CLEAN   := bin/* lokios.mbr
NOW     := $(shell date +"%c")

I386_16_ASFLAGS := -march=pentium --32
I386_32_CFLAGS := -O1 -m32 -march=pentium

.PHONY: all
all: bin/lokios.0 lokios.mbr

include $(patsubst %, %/module.mk, $(MODULES))

bin/lokios.0: lokios.0/lokios.0.elf
	objcopy -O binary -S lokios.0/lokios.0.elf bin/lokios.0

lokios.mbr: bin/lokios.0
	cat bin/lokios.0 > lokios.mbr

.PHONY: clean
clean:
	rm -f $(CLEAN)
