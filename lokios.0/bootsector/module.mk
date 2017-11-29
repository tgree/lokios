BOOTSECTOR_OBJ := \
	$(MODULE)/disk.o    \
	$(MODULE)/console.o \
	$(MODULE)/entry.o
CLEAN += $(BOOTSECTOR_OBJ) $(MODULE)/bootsector.a

$(BOOTSECTOR_OBJ): ASFLAGS := $(I386_16_ASFLAGS)
$(MODULE)/bootsector.a: $(BOOTSECTOR_OBJ) $(MODULE)/module.mk
	$(AR) $(ARFLAGS) $@ $(BOOTSECTOR_OBJ)
