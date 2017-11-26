MODULES := lokios.0 lokios.1
CLEAN   := bin/* lokios.mbr
NOW     := $(shell date +"%c")

I386_16_ASFLAGS := -march=core2 --32
I386_32_CFLAGS := -O1 -m32 -march=pentium -Wall -Werror

X86_64_ASFLAGS := -march=core2 --64
X86_64_CXXFLAGS := -O2 -march=core2 -m64 -std=gnu++14 -Wall -Werror

.PHONY: all
all: bin/lokios.0 lokios.mbr

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

%.o: %.cc
	$(CXX) -MMD -MP -MF $*.d -c $(CXXFLAGS) $*.cc -o $*.o
