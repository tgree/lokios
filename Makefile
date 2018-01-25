MODULES      := tmock lokios.1 lokios.0
ALL_TESTS    :=

TESTS_DIR    := tests
SRC_DIR      := src
BUILD_DIR    := build
BUILD_O_DIR  := $(BUILD_DIR)/obj
BUILD_TO_DIR := $(BUILD_DIR)/objt
LIB_DIR      := $(BUILD_DIR)/lib
BIN_DIR      := bin
TEST_RES_DIR := $(TESTS_DIR)/.results
CLEAN        := $(BUILD_DIR) $(BIN_DIR) $(TESTS_DIR)
NOW          := $(shell date +"%c")
INCLUDE_DIR  := $(BUILD_DIR)/include
HEADERS      :=

I386_16_ASFLAGS := -march=core2 --32
I386_32_CFLAGS := -O1 -m32 -march=pentium -Wall -Werror

ASFLAGS := -march=core2 --64
KERN_CXXFLAGS := -O2 -march=core2 -m64 -std=gnu++17 -Wall -Werror \
                 -Wno-invalid-offsetof -Wno-multichar -Wno-pmf-conversions \
		 -ggdb -I$(abspath $(SRC_DIR)) -I$(INCLUDE_DIR)
TEST_CXXFLAGS := -O2 -march=core2 -m64 -std=gnu++17 -Wall -Werror \
                 -Wno-invalid-offsetof -Wno-multichar -ggdb \
                 -I$(abspath $(SRC_DIR)) -I$(INCLUDE_DIR)

ARFLAGS := rc

BUILD_TEST = mkdir -p $(TESTS_DIR) && $(CXX) $(TEST_CXXFLAGS) -o $@

.PHONY: all
all: $(BIN_DIR)/lokios.mbr test
	@:

define define_test
-include $$($(1).objs:.o=.d)
$$(TESTS_DIR)/$(1): $$($(1).objs) $$($(1).libs) $$(LTMOCK) $$(MODULE_MK)
	@echo Building $$@
	@$$(BUILD_TEST) $$($(1).objs) -Wl,-\( $$($(1).libs) -Wl,-\) $$(LTMOCK)
endef

define include_module
	$(eval SUBMODULES := )
	$(eval TESTS := )
	$(eval MODULE := $(1))
	$(eval PARENT := $(2))
	$(eval MODULE_SRC_DIR := $(SRC_DIR)/$(MODULE))
	$(eval PARENT_SRC_DIR := $(SRC_DIR)/$(PARENT))
	$(eval MODULE_BUILD_DIR := $(BUILD_O_DIR)/$(MODULE))
	$(eval PARENT_BUILD_DIR := $(BUILD_O_DIR)/$(PARENT))
	$(eval MODULE_TBUILD_DIR := $(BUILD_TO_DIR)/$(MODULE))
	$(eval PARENT_TBUILD_DIR := $(BUILD_TO_DIR)/$(PARENT))
	$(eval MODULE_MK := $(MODULE_SRC_DIR)/module.mk)
	$(eval include $(MODULE_MK))
	$(eval $(foreach T,$(TESTS),$(eval $(call define_test,$(T)))))
	$(eval ALL_TESTS += $(TESTS))
	ifneq ($(strip $(SUBMODULES)),)
		$(eval $(call include_modules,$(patsubst %,$(1)/%,$(SUBMODULES)),$(1)))
	endif
endef

define include_modules
	$(eval $(foreach M,$(1),$(call include_module,$(M),$(2))))
endef

$(call include_modules,$(MODULES),)

ifeq ($(MAKECMDGOALS),test)
    $(shell rm -rf $(TEST_RES_DIR))
endif
.PHONY: test
test: $(ALL_TESTS:%=$(TEST_RES_DIR)/%.tpass)
	$(info All tests passed.)

.PHONY: headers
headers: $(INCLUDE_DIR)/kernel $(HEADERS:$(SRC_DIR)/lokios.1/%=$(INCLUDE_DIR)/%)

$(BIN_DIR)/lokios.0: $(BUILD_O_DIR)/lokios.0/lokios.0.elf
	@echo Generating $@...
	@mkdir -p $(@D)
	@objcopy -O binary -S $(BUILD_O_DIR)/lokios.0/lokios.0.elf $(BIN_DIR)/lokios.0

$(BIN_DIR)/lokios.1: $(BUILD_O_DIR)/lokios.1/lokios.1.elf
	@echo Generating $@...
	@mkdir -p $(@D)
	@objcopy -O binary -S $(BUILD_O_DIR)/lokios.1/lokios.1.elf $(BIN_DIR)/lokios.1

$(BIN_DIR)/lokios.mbr: $(BIN_DIR)/lokios.0 $(BIN_DIR)/lokios.1
	@echo Generating $@...
	@cat $(BIN_DIR)/lokios.0 $(BIN_DIR)/lokios.1 > $(BIN_DIR)/lokios.mbr

.PHONY: clean
clean:
	rm -rf $(CLEAN)

# We can't use $^ for the list of source files because that will also pull in all
# the other header dependencies generated by the .d files.
$(BUILD_O_DIR)/%.o: $(SRC_DIR)/%.cc | headers
	@echo Compiling $(SRC_DIR)/$*.cc...
	@mkdir -p $(dir $@)
	@$(CXX) -MMD -MP -MF $(BUILD_O_DIR)/$*.d -c $(KERN_CXXFLAGS) $(SRC_DIR)/$*.cc -o $@

$(BUILD_TO_DIR)/%.o: $(SRC_DIR)/%.cc | headers
	@echo Compiling $(SRC_DIR)/$*.cc...
	@mkdir -p $(dir $@)
	@$(CXX) -MMD -MP -MF $(BUILD_TO_DIR)/$*.d -c $(TEST_CXXFLAGS) $(SRC_DIR)/$*.cc -o $@

$(BUILD_O_DIR)/%.o: $(SRC_DIR)/%.s
	@echo Assembling $^...
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) -o $@ $^

$(TEST_RES_DIR)/%.tpass: $(TESTS_DIR)/%
	@mkdir -p $(TEST_RES_DIR)
	@$^ && touch $@

$(INCLUDE_DIR)/%: $(SRC_DIR)/lokios.1/%
	@mkdir -p $(dir $@)
	@ln -r -s $^ $@

$(INCLUDE_DIR)/kernel: $(SRC_DIR)/lokios.1
	@mkdir -p $(dir $@)
	@ln -r -s $^ $@
