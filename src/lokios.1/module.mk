SUBMODULES := kern mm acpi interrupts dev pci platform k++ net http wapi vmm

CRTBEGIN_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtbeginS.o)
CRTEND_OBJ   := $(shell $(CC) $(CFLAGS) -print-file-name=crtendS.o)
LGCC_EH_OBJ  := $(shell $(CC) $(CFLAGS) -print-file-name=libgcc_eh.a)

LOKIOS_1_OBJ := 
LOKIOS_1_DRIVERS := e1000 virtio_net bcm57762
LOKIOS_1_CONSTRUCTORS :=
LOKIOS_1_LIB := kern.a mm.a acpi.a interrupts.a dev.a pci.a platform.a k++.a \
                dhcp.a arp.a net.a eth.a eth_phys.a tcp.a udp.a http.a \
                wapi.a vmm.a $(LOKIOS_1_DRIVERS:%=%.a)
LOKIOS_1_WEAK := libsupc++.a

$(MODULE_BUILD_DIR)/lokios.1.elf: LDM  := $(MODULE_BUILD_DIR)/lokios.1.map
$(MODULE_BUILD_DIR)/lokios.1.elf: LDLD := $(MODULE_SRC_DIR)/lokios.1.ld
$(MODULE_BUILD_DIR)/lokios.1.elf: \
    $(LOKIOS_1_WEAK:%=$(LIB_DIR)/%) \
    $(LOKIOS_1_LIB:%=$(LIB_DIR)/%) \
    $(MODULE_SRC_DIR)/lokios.1.ld \
    $(MODULE_MK)
	@echo Linking $@...
	ld -melf_x86_64 -Map=$(LDM) -T $(LDLD) $(LOKIOS_1_DRIVERS:%=--require-defined=%_driver) $(LOKIOS_1_CONSTRUCTORS:%=--require-defined=%) -o $@ \
	    	$(CRTBEGIN_OBJ) \
		$(LOKIOS_1_OBJ) \
		-\(             \
		$(LOKIOS_1_LIB:%=$(LIB_DIR)/%) \
		-\)             \
		$(LOKIOS_1_WEAK:%=$(LIB_DIR)/%) \
		$(LGCC_EH_OBJ)  \
		$(CRTEND_OBJ)
