TESTS += $(TESTDIR)/char_stream_test

CHAR_STREAM_TEST_OBJS := \
	$(MODULE)/char_stream_test.o \
	$(PARENT)/char_stream.o

CLEAN += \
	 $(CHAR_STREAM_TEST_OBJS) 	\
	 $(CHAR_STREAM_TEST_OBJS:.o=.d)

-include $(CHAR_STREAM_TEST_OBJS:.o=.d)

$(TESTDIR)/char_stream_test: $(CHAR_STREAM_TEST_OBJS) $(MODULE)/module.mk
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $(CHAR_STREAM_TEST_OBJS)
	@$@ || touch -c -m -t 197701010000 $@
