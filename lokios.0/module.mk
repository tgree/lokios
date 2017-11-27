SUBMODULES := bootsector

LOKIOS_0_OBJ := \
    	$(MODULE)/bootsector/bootsector.a \
	$(MODULE)/common_entry.o          \
	$(MODULE)/pxe_entry.o             \
	$(MODULE)/mbr_entry.o             \
	$(MODULE)/e820.o                  \
	$(MODULE)/a20_gate.o              \
	$(MODULE)/unreal_mode.o           \
	$(MODULE)/long_mode.o
TIMESTAMP_S := $(MODULE)/bootsector/timestamp.s
TIMESTAMP_O := $(MODULE)/bootsector/timestamp.o
CLEAN += \
	$(LOKIOS_0_OBJ)       		 \
	$(MODULE)/lokios.0.elf 		 \
	$(TIMESTAMP_S)			 \
	$(TIMESTAMP_O)

$(LOKIOS_0_OBJ): ASFLAGS := $(I386_16_ASFLAGS)
$(LOKIOS_0_OBJ): CFLAGS := $(I386_32_CFLAGS)

$(MODULE)/lokios.0.elf: LDLD := $(MODULE)/lokios.0.ld
$(MODULE)/lokios.0.elf: $(LOKIOS_0_OBJ) $(MODULE)/lokios.0.ld $(MODULE)/module.mk
	@echo '.data' > $(TIMESTAMP_S)
	@echo '.globl _BUILD_TIME' >> $(TIMESTAMP_S)
	@echo '_BUILD_TIME: .asciz "$(NOW)\\r\\n"' >> $(TIMESTAMP_S)
	@$(AS) $(I386_16_ASFLAGS) -o $(TIMESTAMP_O) $(TIMESTAMP_S)
	ld -melf_i386 -T $(LDLD) -o $@ $(LOKIOS_0_OBJ) $(TIMESTAMP_O)
