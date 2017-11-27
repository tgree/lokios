MODULES := lokios.1 lokios.0
CLEAN   := bin/* lokios.mbr
NOW     := $(shell date +"%c")

I386_16_ASFLAGS := -march=core2 --32
I386_32_CFLAGS := -O1 -m32 -march=pentium -Wall -Werror

X86_64_ASFLAGS := -march=core2 --64
X86_64_CXXFLAGS := -O2 -march=core2 -m64 -std=gnu++14 -Wall -Werror

ARFLAGS := rc

.PHONY: all
all: bin/lokios.0 lokios.mbr
	@:

define include_module
	$(info $(0) $(1) $(2))
	$(eval SUBMODULES := )
	$(eval MODULE := $(1))
	$(eval PARENT := $(2))
	$(eval include $(MODULE)/module.mk)
	ifneq ($(strip $(SUBMODULES)),)
		$(eval $(call include_modules,$(patsubst %,$(1)/%,$(SUBMODULES)),$(1)))
	endif
endef

define include_modules
	$(eval $(foreach M,$(1),$(call include_module,$(M),$(2))))
endef

$(call include_modules,$(MODULES),)

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
