SUBMODULES :=
HEADERS +=

VIRTIO_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
VIRTIO_OBJ := $(VIRTIO_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(VIRTIO_OBJ:.o=.d)

$(LIB_DIR)/virtio_net.a: $(VIRTIO_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(VIRTIO_OBJ)
