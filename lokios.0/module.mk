LOKIOS_0_OBJ := \
	lokios.0/bootsector/dispatcher.o \
	lokios.0/bootsector/console.o    \
	lokios.0/bootsector/entry.o      \
	lokios.0/common_entry.o          \
	lokios.0/pxe_entry.o             \
	lokios.0/mbr_entry.o             \
	lokios.0/e820.o                  \
	lokios.0/a20_gate.o              \
	lokios.0/unreal_mode.o           \
	lokios.0/long_mode.o             \
	lokios.0/start_c.o
CLEAN += \
	$(LOKIOS_0_OBJ)       		 \
	lokios.0/lokios.0.elf 		 \
	lokios.0/bootsector/timestamp.s  \
	lokios.0/bootsector/timestamp.o

$(LOKIOS_0_OBJ): ASFLAGS := $(I386_16_ASFLAGS)
$(LOKIOS_0_OBJ): CFLAGS := $(I386_32_CFLAGS)
lokios.0/lokios.0.elf: $(LOKIOS_0_OBJ) lokios.0/lokios.0.ld lokios.0/module.mk
	@echo '.data' > lokios.0/bootsector/timestamp.s
	@echo '.globl _BUILD_TIME' >> lokios.0/bootsector/timestamp.s
	@echo '_BUILD_TIME: .asciz "$(NOW)\\r\\n"' >> lokios.0/bootsector/timestamp.s
	@$(AS) $(I386_16_ASFLAGS) -o lokios.0/bootsector/timestamp.o lokios.0/bootsector/timestamp.s
	ld -melf_i386 -T lokios.0/lokios.0.ld -o lokios.0/lokios.0.elf $(LOKIOS_0_OBJ) lokios.0/bootsector/timestamp.o
