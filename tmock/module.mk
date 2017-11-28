TMOCK_OBJ := \
    	$(MODULE)/tmock.o

CLEAN += \
	$(TMOCK_OBJ)       \
	$(TMOCK_OBJ:.o=.d) \
	$(MODULE)/tmock.a

-include $(TMOCK_OBJ:.o=.d)

$(MODULE)/tmock.a: $(TMOCK_OBJ) $(MODULE)/module.mk
	$(AR) $(ARFLAGS) $@ $(TMOCK_OBJ)
