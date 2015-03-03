# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files

EGOLIB_SOURCE := $(wildcard src/egolib/*.c)
EGOLIB_SOURCE += $(wildcard src/egolib/*/*.c)
EGOLIB_CPPSRC := $(wildcard src/egolib/*.cpp)
EGOLIB_CPPSRC += $(wildcard src/egolib/*/*.cpp)
EGOLIB_CPPSRC += $(wildcard src/egolib/*/*/*.cpp)

# remove some files
EGOLIB_SOURCE := $(filter-out src/egolib/bsp_leaf.c, $(EGOLIB_SOURCE))
EGOLIB_SOURCE := $(filter-out src/egolib/platform/file_win.c, $(EGOLIB_SOURCE))
EGOLIB_SOURCE := $(filter-out src/egolib/platform/sys_win32.c, $(EGOLIB_SOURCE))

EGOLIB_OBJ := ${EGOLIB_SOURCE:.c=.o} ${EGOLIB_CPPSRC:.cpp=.o}

INC := -Isrc -I../idlib/src -I../game/src

CFLAGS   += $(INC)
CXXFLAGS += $(INC)

# variables for EgoTest's makefile 

EGOTEST_DIR  := ../egotest
TEST_SOURCES := $(wildcard tests/*.cpp)
TEST_LDFLAGS := $(EGOLIB_TARGET) ../idlib/$(IDLIB_TARGET)  $(LDFLAGS)

#------------------------------------
# definitions of the target projects

.PHONY: all clean

$(EGOLIB_TARGET): ${EGOLIB_OBJ}
	$(AR) -r $@ $^

all: $(EGOLIB_TARGET)

include $(EGOTEST_DIR)/EgoTest.makefile

test: $(EGOLIB_TARGET) do_test

clean: test_clean
	rm -f ${EGOLIB_OBJ} $(EGOLIB_TARGET)
