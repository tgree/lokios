MODULES := tmock lokios.1 lokios.0
TESTS   :=
TESTDIR := test
BUILD_DIR := build
TESTRES := $(TESTDIR)/.results
CLEAN   := build bin test lokios.mbr
NOW     := $(shell date +"%c")

I386_16_ASFLAGS := -march=core2 --32
I386_32_CFLAGS := -O1 -m32 -march=pentium -Wall -Werror

X86_64_ASFLAGS := -march=core2 --64
CXXFLAGS := -O2 -march=core2 -m64 -std=gnu++14 -Wall -Werror \
            -I$(abspath $(CURDIR))

ARFLAGS := rc

BUILD_TEST = mkdir -p $(TESTDIR) && $(CXX) $(CXXFLAGS) -o $@

.PHONY: all
all: $(MODULES) lokios.mbr test
	@:

define include_module
	$(eval SUBMODULES := )
	$(eval MODULE := $(1))
	$(eval PARENT := $(2))
	$(eval MODULE_BUILD_DIR := $(BUILD_DIR)/$(MODULE))
	$(eval PARENT_BUILD_DIR := $(BUILD_DIR)/$(PARENT))
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
test: $(TESTS:$(TESTDIR)/%=$(TESTRES)/%.tpass)
	$(info All tests passed.)

bin/lokios.0: $(BUILD_DIR)/lokios.0/lokios.0.elf
	@mkdir -p $(@D)
	objcopy -O binary -S build/lokios.0/lokios.0.elf bin/lokios.0

bin/lokios.1: $(BUILD_DIR)/lokios.1/lokios.1.elf
	@mkdir -p $(@D)
	objcopy -O binary -S build/lokios.1/lokios.1.elf bin/lokios.1

lokios.mbr: bin/lokios.0 bin/lokios.1
	cat bin/lokios.0 bin/lokios.1 > lokios.mbr

.PHONY: clean
clean:
	rm -rf $(CLEAN)

$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) -MMD -MP -MF $(BUILD_DIR)/$*.d -c $(CXXFLAGS) $*.cc -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $^

$(TESTRES)/%.tpass: $(TESTDIR)/%
	@mkdir -p $(TESTRES)
	@$^ && touch $@
