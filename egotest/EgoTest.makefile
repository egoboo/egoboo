# This is a template for EgoTest's handwritten backend

# Set EGOTEST_DIR to where the main egotest directory is
# Set TEST_SOURCES to your test sources

# Optional flags
# Set TEST_CXXFLAGS for compiling your tests (default $CXXFLAGS)
# Set TEST_LDFLAGS for liinking your tests (default $LDFLAGS)
# Set TEST_BINARY to the test binary (and the generated cpp file) (default ./TestMain)

ifeq ($(TEST_LDFLAGS),)
TEST_LDFLAGS := $(LDFLAGS)
endif

ifeq ($(TEST_CXXFLAGS),)
TEST_CXXFLAGS := $(CXXFLAGS)
endif

ifeq ($(TEST_BINARY),)
TEST_BINARY := ./TestMain
endif

TEST_CXXFLAGS += -I$(EGOTEST_DIR)/src

TEST_REQUIREDFILES = ${EGOTEST_DIR}/generate_main_cpp ${EGOTEST_DIR}/src/EgoTest/EgoTest_Handwritten.cpp

.PHONY: test_check_vars do_test test_clean

do_test: test_check_vars $(TEST_BINARY)
	$(TEST_BINARY)

$(TEST_BINARY): $(TEST_BINARY).o
	$(CXX) -o $@ $^ $(TEST_LDFLAGS)

$(TEST_BINARY).o: $(TEST_BINARY).cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ -c $^

$(TEST_BINARY).cpp: ${TEST_SOURCES} $(TEST_REQUIREDFILES)
	perl ${EGOTEST_DIR}/generate_main_cpp $@ ${TEST_SOURCES}

test_clean:
	rm -f $(TEST_BINARY) $(TEST_BINARY).cpp $(TEST_BINARY).o

test_check_vars:
ifeq ($(EGOTEST_DIR),)
	$(error EGOTEST_DIR is empty, this makefile will not function correctly.)
endif
	
ifeq ($(TEST_SOURCES),)
	$(warning TEST_SOURCES is empty, no tests will be compiled.)
endif
