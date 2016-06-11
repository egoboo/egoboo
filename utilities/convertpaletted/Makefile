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

override CXXFLAGS += $(EGO_CXXFLAGS) -Isrc -I../../egolib/src -I../../idlib/src
override LDFLAGS += $(EGO_LDFLAGS)

#------------------------------------
# definitions of the target projects

.PHONY: all clean

all: $(EGOTOOL_TARGET)

$(EGOTOOL_TARGET): ${EGOTOOL_OBJ} ${EGOLIB_L} ${IDLIB_L}
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CXX) -x c++ $(CXXFLAGS) -o $@ -c $^

clean:
	rm -f ${EGOTOOL_OBJ} $(EGOTOOL_TARGET)
