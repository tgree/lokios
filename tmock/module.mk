TMOCK_SRCS := $(wildcard $(MODULE)/*.cc)
TMOCK_OBJ := $(patsubst $(MODULE)/%.cc,$(MODULE_BUILD_DIR)/%.o,$(TMOCK_SRCS))

-include $(TMOCK_OBJ:.o=.d)

LTMOCK := $(MODULE_BUILD_DIR)/tmock.a
$(LTMOCK): $(TMOCK_OBJ) $(MODULE)/module.mk
	$(AR) $(ARFLAGS) $@ $(TMOCK_OBJ)
