MODULES := lokios.1 lokios.0 tmock
TESTS   :=
TESTDIR := test
CLEAN   := bin test lokios.mbr
NOW     := $(shell date +"%c")

I386_16_ASFLAGS := -march=core2 --32
I386_32_CFLAGS := -O1 -m32 -march=pentium -Wall -Werror

X86_64_ASFLAGS := -march=core2 --64
CXXFLAGS := -O2 -march=core2 -m64 -std=gnu++14 -Wall -Werror \
            -I$(abspath $(CURDIR))

ARFLAGS := rc

.PHONY: all
all: $(MODULES) lokios.mbr test
	@:

define include_module
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

.PHONY: test
test: $(TESTS)

bin/lokios.0: lokios.0/lokios.0.elf
	@mkdir -p $(@D)
	objcopy -O binary -S lokios.0/lokios.0.elf bin/lokios.0

bin/lokios.1: lokios.1/lokios.1.elf
	@mkdir -p $(@D)
	objcopy -O binary -S lokios.1/lokios.1.elf bin/lokios.1

lokios.mbr: bin/lokios.0 bin/lokios.1
	cat bin/lokios.0 bin/lokios.1 > lokios.mbr

.PHONY: clean
clean:
	rm -rf $(CLEAN)

%.o: %.cc
	$(CXX) -MMD -MP -MF $*.d -c $(CXXFLAGS) $*.cc -o $*.o
