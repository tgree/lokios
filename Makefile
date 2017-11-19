MODULES := lokios.0
CLEAN   := bin/*
ASFLAGS := -march=i386 --32
NOW     := $(shell date +"%c")

.PHONY: all
all: bin/lokios.0

include $(patsubst %, %/module.mk, $(MODULES))

bin/lokios.0: lokios.0/lokios.0.elf
	objcopy -O binary -S lokios.0/lokios.0.elf bin/lokios.0

.PHONY: clean
clean:
	rm -f $(CLEAN)
