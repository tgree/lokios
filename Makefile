MODULES := lokios.0
CLEAN   := bin/*
NOW     := $(shell date +"%c")

I386_16_ASFLAGS := -march=i386 --32

.PHONY: all
all: bin/lokios.0

include $(patsubst %, %/module.mk, $(MODULES))

bin/lokios.0: lokios.0/lokios.0.elf
	objcopy -O binary -S lokios.0/lokios.0.elf bin/lokios.0

.PHONY: clean
clean:
	rm -f $(CLEAN)
