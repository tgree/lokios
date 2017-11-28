TMOCK_SRCS := $(wildcard $(MODULE)/*.cc)
TMOCK_OBJ := $(patsubst %.cc,%.o,$(TMOCK_SRCS))

CLEAN += \
	$(TMOCK_OBJ)       \
	$(TMOCK_OBJ:.o=.d) \
	$(MODULE)/tmock.a

-include $(TMOCK_OBJ:.o=.d)

$(LTMOCK): $(TMOCK_OBJ) $(MODULE)/module.mk
	$(AR) $(ARFLAGS) $@ $(TMOCK_OBJ)
