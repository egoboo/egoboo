# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files

EGOTOOL_SRC    := ${wildcard src/*.c} ${wildcard src/*/*.c}
EGOTOOL_CPPSRC := ${wildcard src/*.cpp} ${wildcard src/*/*.cpp} ../../unix/main.cpp
EGOTOOL_OBJ    := ${EGOTOOL_SRC:.c=.o} ${EGOTOOL_CPPSRC:.cpp=.o}

#---------------------
# the egolib configuration

EGOLIB_L := ../../egolib/$(EGOLIB_TARGET)
IDLIB_L  := ../../idlib/$(IDLIB_TARGET)

#---------------------
# the compiler options

INC      := -Isrc -I../../egolib/src -I../../idlib/src

CFLAGS   += $(INC)
CXXFLAGS += $(INC)

#------------------------------------
# definitions of the target projects

.PHONY: all clean

$(EGOTOOL_TARGET): ${EGOTOOL_OBJ} ${EGOLIB_L} ${IDLIB_L}
	$(CXX) -o $@ $^ $(LDFLAGS)

all: $(EGOTOOL_TARGET)

clean:
	rm -f ${EGOTOOL_OBJ} $(EGOTOOL_TARGET)
