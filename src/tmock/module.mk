TMOCK_SRCS := $(wildcard $(MODULE_SRC_DIR)/*.cc)
TMOCK_OBJ := $(patsubst $(MODULE_SRC_DIR)/%.cc,$(MODULE_BUILD_DIR)/%.o,$(TMOCK_SRCS))

-include $(TMOCK_OBJ:.o=.d)

LTMOCK := $(MODULE_BUILD_DIR)/tmock.a
$(LTMOCK): $(TMOCK_OBJ) $(MODULE_MK)
	$(AR) $(ARFLAGS) $@ $(TMOCK_OBJ)
