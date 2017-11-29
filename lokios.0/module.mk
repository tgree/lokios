SUBMODULES := bootsector

LOKIOS_0_OBJ := \
    	$(MODULE_BUILD_DIR)/bootsector/bootsector.a \
	$(MODULE_BUILD_DIR)/common_entry.o          \
	$(MODULE_BUILD_DIR)/pxe_entry.o             \
	$(MODULE_BUILD_DIR)/mbr_entry.o             \
	$(MODULE_BUILD_DIR)/e820.o                  \
	$(MODULE_BUILD_DIR)/a20_gate.o              \
	$(MODULE_BUILD_DIR)/unreal_mode.o           \
	$(MODULE_BUILD_DIR)/long_mode.o
TIMESTAMP_S := $(MODULE_BUILD_DIR)/bootsector/timestamp.s
TIMESTAMP_O := $(MODULE_BUILD_DIR)/bootsector/timestamp.o

$(LOKIOS_0_OBJ): ASFLAGS := $(I386_16_ASFLAGS)
$(LOKIOS_0_OBJ): CFLAGS := $(I386_32_CFLAGS)

$(MODULE_BUILD_DIR)/lokios.0.elf: LDLD := $(MODULE)/lokios.0.ld
$(MODULE_BUILD_DIR)/lokios.0.elf: $(LOKIOS_0_OBJ) $(MODULE)/lokios.0.ld $(MODULE)/module.mk
	@echo '.data' > $(TIMESTAMP_S)
	@echo '.globl _BUILD_TIME' >> $(TIMESTAMP_S)
	@echo '_BUILD_TIME: .asciz "$(NOW)\\r\\n"' >> $(TIMESTAMP_S)
	@$(AS) $(I386_16_ASFLAGS) -o $(TIMESTAMP_O) $(TIMESTAMP_S)
	ld -melf_i386 -T $(LDLD) -o $@ $(LOKIOS_0_OBJ) $(TIMESTAMP_O)
