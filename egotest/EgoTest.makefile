# This is a template for EgoTest's handwritten backend

# Set EGOTEST_DIR to where the main egotest directory is
# Set TEST_SOURCES to your test sources

# Optional flags
# Set TEST_CXXFLAGS for compiling your tests (default $CXXFLAGS)
# Set TEST_LDFLAGS for liinking your tests (default $LDFLAGS)
# Set TEST_BINARY to the test binary (default ./TestMain)

ifeq ($(TEST_LDFLAGS),)
TEST_LDFLAGS := $(LDFLAGS)
endif

ifeq ($(TEST_CXXFLAGS),)
TEST_CXXFLAGS := $(CXXFLAGS)
endif

ifeq ($(TEST_BINARY),)
TEST_BINARY := ./TestMain
endif

TEST_GENERATED_SOURCES = $(addprefix gen/, $(TEST_SOURCES))
TEST_GENERATED_FILES = gen/TestMain.cpp $(TEST_GENERATED_SOURCES)
TEST_GENERATED_OBJECTS = $(TEST_GENERATED_FILES:.cpp=.o)

TEST_CXXFLAGS += -I$(EGOTEST_DIR)/src -DEGOTEST_USE_HANDWRITTEN -I.

TEST_REQUIREDFILES = ${EGOTEST_DIR}/generate_test_files.pl ${EGOTEST_DIR}/src/EgoTest/EgoTest_Handwritten.cpp

.PHONY: test_check_vars do_test test_clean

do_test: test_check_vars $(TEST_BINARY)
	$(TEST_BINARY)

$(TEST_BINARY): $(TEST_GENERATED_OBJECTS)
	$(CXX) -o $@ $^ $(TEST_LDFLAGS)

$(TEST_GENERATED_OBJECTS): %.o: %.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ -c $^

$(TEST_GENERATED_SOURCES): gen/TestMain.cpp

gen/TestMain.cpp: ${TEST_SOURCES} $(TEST_REQUIREDFILES)
	perl ${EGOTEST_DIR}/generate_test_files.pl cpp ${TEST_SOURCES}

test_clean:
	rm -f $(TEST_BINARY) $(TEST_GENERATED_FILES) $(TEST_GENERATED_OBJECTS)

test_check_vars:
ifeq ($(EGOTEST_DIR),)
	$(error EGOTEST_DIR is empty, this makefile will not function correctly.)
endif
	
ifeq ($(TEST_SOURCES),)
	$(warning TEST_SOURCES is empty, no tests will be compiled.)
endif
