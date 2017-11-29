MODULES   := tmock lokios.1 lokios.0
TESTS     :=

TESTS_DIR := tests
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin
TESTRES   := $(TESTS_DIR)/.results
CLEAN     := $(BUILD_DIR) $(BIN_DIR) $(TESTS_DIR) lokios.mbr
NOW       := $(shell date +"%c")

I386_16_ASFLAGS := -march=core2 --32
I386_32_CFLAGS := -O1 -m32 -march=pentium -Wall -Werror

X86_64_ASFLAGS := -march=core2 --64
CXXFLAGS := -O2 -march=core2 -m64 -std=gnu++14 -Wall -Werror \
            -I$(abspath $(SRC_DIR))

ARFLAGS := rc

BUILD_TEST = mkdir -p $(TESTS_DIR) && $(CXX) $(CXXFLAGS) -o $@

.PHONY: all
all: lokios.mbr test
	@:

define include_module
	$(eval SUBMODULES := )
	$(eval MODULE := $(1))
	$(eval PARENT := $(2))
	$(eval MODULE_SRC_DIR := $(SRC_DIR)/$(MODULE))
	$(eval PARENT_SRC_DIR := $(SRC_DIR)/$(PARENT))
	$(eval MODULE_BUILD_DIR := $(BUILD_DIR)/$(MODULE))
	$(eval PARENT_BUILD_DIR := $(BUILD_DIR)/$(PARENT))
	$(eval MODULE_MK := $(MODULE_SRC_DIR)/module.mk)
	$(eval include $(MODULE_MK))
	ifneq ($(strip $(SUBMODULES)),)
		$(eval $(call include_modules,$(patsubst %,$(1)/%,$(SUBMODULES)),$(1)))
	endif
endef

define include_modules
	$(eval $(foreach M,$(1),$(call include_module,$(M),$(2))))
endef

$(call include_modules,$(MODULES),)

.PHONY: test
test: $(TESTS:$(TESTS_DIR)/%=$(TESTRES)/%.tpass)
	$(info All tests passed.)

$(BIN_DIR)/lokios.0: $(BUILD_DIR)/lokios.0/lokios.0.elf
	@mkdir -p $(@D)
	objcopy -O binary -S $(BUILD_DIR)/lokios.0/lokios.0.elf $(BIN_DIR)/lokios.0

$(BIN_DIR)/lokios.1: $(BUILD_DIR)/lokios.1/lokios.1.elf
	@mkdir -p $(@D)
	objcopy -O binary -S $(BUILD_DIR)/lokios.1/lokios.1.elf $(BIN_DIR)/lokios.1

lokios.mbr: $(BIN_DIR)/lokios.0 $(BIN_DIR)/lokios.1
	cat $(BIN_DIR)/lokios.0 $(BIN_DIR)/lokios.1 > lokios.mbr

.PHONY: clean
clean:
	rm -rf $(CLEAN)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cc
	@mkdir -p $(dir $@)
	$(CXX) -MMD -MP -MF $(BUILD_DIR)/$*.d -c $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $^

$(TESTRES)/%.tpass: $(TESTS_DIR)/%
	@mkdir -p $(TESTRES)
	@$^ && touch $@
