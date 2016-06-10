# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files
CARTMAN_SRC := ${wildcard src/cartman/*.c}
CARTMAN_SRC += ${wildcard src/cartman/*/*.c}
CARTMAN_SRC_CPP := ../unix/main.cpp
CARTMAN_SRC_CPP += ${wildcard src/cartman/*.cpp}
CARTMAN_SRC_CPP += ${wildcard src/cartman/*/*.cpp}
CARTMAN_OBJ := ${CARTMAN_SRC:.c=.o} ${CARTMAN_SRC_CPP:.cpp=.o}

#---------------------
# the egolib configuration

EGOLIB_L := ../egolib/$(EGOLIB_TARGET)
IDLIB_L  := ../idlib/$(IDLIB_TARGET)

#---------------------
# the compiler options

override CXXFLAGS += $(EGO_CXXFLAGS) -Isrc -I../egolib/src -I../idlib/src
override LDFLAGS += $(EGO_LDFLAGS)

#------------------------------------
# definitions of the target projects

.PHONY: all clean

all: $(CARTMAN_TARGET)

$(CARTMAN_TARGET): ${CARTMAN_OBJ} ${EGOLIB_L} ${IDLIB_L}
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CXX) -x c++ $(CXXFLAGS) -o $@ -c $^

clean:
	rm -f ${CARTMAN_OBJ} $(CARTMAN_TARGET)
