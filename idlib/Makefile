# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files

IDLIB_SOURCE := $(wildcard src/IdLib/*.c)
IDLIB_SOURCE += $(wildcard src/IdLib/*/*.c)
IDLIB_CPPSRC := $(wildcard src/IdLib/*.cpp)
IDLIB_CPPSRC += $(wildcard src/IdLib/*/*.cpp)
IDLIB_CPPSRC += $(wildcard src/IdLib/*/*/*.cpp)

# remove some files
# (no files yet)

IDLIB_OBJ := ${IDLIB_SOURCE:.c=.o} ${IDLIB_CPPSRC:.cpp=.o}

INC := -Isrc

CFLAGS   += $(INC)
CXXFLAGS += $(INC)

# variables for EgoTest's makefile 

EGOTEST_DIR  := ../egotest
TEST_SOURCES := $(wildcard tests/*.cpp)
TEST_LDFLAGS := $(IDLIB_TARGET) $(LDFLAGS)

#------------------------------------
# definitions of the target projects

.PHONY: all clean

$(IDLIB_TARGET): ${IDLIB_OBJ}
	$(AR) -r $@ $^

all: $(IDLIB_TARGET)

include $(EGOTEST_DIR)/EgoTest.makefile

test: $(IDLIB_TARGET) do_test

clean: test_clean
	rm -f ${IDLIB_OBJ} $(IDLIB_TARGET)
