# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files

EGO_SRC    := ${wildcard src/game/*.c} ${wildcard src/game/*/*.c} ${wildcard src/game/*/*.c}
EGO_CPPSRC := ${wildcard src/game/*.cpp} ${wildcard src/game/*/*.cpp} ${wildcard src/game/*/*/*.cpp} 
EGO_CPPSRC += ../unix/main.cpp
EGO_OBJ    := ${EGO_SRC:.c=.o} ${EGO_CPPSRC:.cpp=.o}

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

all: $(EGO_TARGET)

$(EGO_TARGET): ${EGO_OBJ} ${EGOLIB_L} ${IDLIB_L}
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CXX) -x c++ $(CXXFLAGS) -o $@ -c $^

clean:
	rm -f ${EGO_OBJ} $(EGO_TARGET)
