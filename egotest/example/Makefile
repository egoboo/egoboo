
LIB_SOURCES := $(wildcard src/*.cpp)
LIB_OBJECTS := $(LIB_SOURCES:.cpp=.o)

LIB_NAME := libegotestexample.a

CXXFLAGS += -std=c++11

EGOTEST_DIR := ..

TEST_SOURCES := $(wildcard tests/*.cpp)

TEST_CXXFLAGS := $(CXXFLAGS) -Itests -Isrc

TEST_LDFLAGS := $(LIB_NAME) $(LDFLAGS)

.PHONY: clean test

$(LIB_NAME): ${LIB_OBJECTS}
	ar -r $@ $^

include $(EGOTEST_DIR)/EgoTest.makefile

test: $(LIB_NAME) do_test

clean: test_clean
	rm -f ${LIB_OBJECTS} $(LIB_NAME)