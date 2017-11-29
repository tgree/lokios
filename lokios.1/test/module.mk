TESTS += $(TESTDIR)/char_stream_test \
	 $(TESTDIR)/console_test

CHAR_STREAM_TEST_OBJS := \
	$(MODULE)/char_stream_test.o \
	$(PARENT)/char_stream.o

CONSOLE_TEST_OBJS := \
    	$(MODULE)/console_test.o \
	$(PARENT)/console.o \
	$(PARENT)/char_stream.o

CLEAN += \
	 $(CHAR_STREAM_TEST_OBJS) 	\
	 $(CHAR_STREAM_TEST_OBJS:.o=.d)

-include $(CHAR_STREAM_TEST_OBJS:.o=.d)
-include $(CONSOLE_TEST_OBJS:.o=.d)

$(TESTDIR)/char_stream_test: $(CHAR_STREAM_TEST_OBJS) $(LTMOCK) $(MODULE)/module.mk
	$(BUILD_TEST) $(CHAR_STREAM_TEST_OBJS) $(LTMOCK)

$(TESTDIR)/console_test: $(CONSOLE_TEST_OBJS) $(LTMOCK) $(MODULE)/module.mk
	$(BUILD_TEST) $(CONSOLE_TEST_OBJS) $(LTMOCK)
