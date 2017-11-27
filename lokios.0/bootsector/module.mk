BOOTSECTOR_OBJ = \
	$(MODULE)/disk.o    \
	$(MODULE)/console.o \
	$(MODULE)/entry.o
CLEAN += $(BOOTSECTOR_OBJ) $(MODULE)/bootsector.a

$(BOOTSECTOR_OBJ): ASFLAGS := $(I386_16_ASFLAGS)
$(BOOTSECTOR_OBJ): CFLAGS := $(I386_32_CFLAGS)
$(MODULE)/bootsector.a: $(BOOTSECTOR_OBJ)
	$(AR) $(ARFLAGS) $@ $^
