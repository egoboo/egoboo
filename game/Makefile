# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files

EGO_SRC    := ${wildcard src/game/*.c} ${wildcard src/game/ai/*.c}
EGO_CPPSRC := ${wildcard src/game/*.cpp} ${wildcard src/game/*/*.cpp} ../unix/main.cpp
EGO_CPPSRC := $(filter-out src/game/PhysicalConstants.cpp, $(EGO_CPPSRC))
EGO_OBJ    := ${EGO_SRC:.c=.o} ${EGO_CPPSRC:.cpp=.o}

#---------------------
# the egolib configuration

EGOLIB_L := ../egolib/$(EGOLIB_TARGET)
IDLIB_L  := ../idlib/$(IDLIB_TARGET)

#---------------------
# the compiler options

INC      := -Isrc -I../egolib/src -I../idlib/src

CFLAGS   += $(INC)
CXXFLAGS += $(INC)

#------------------------------------
# definitions of the target projects

.PHONY: all clean

$(EGO_TARGET): ${EGO_OBJ} ${EGOLIB_L} ${IDLIB_L}
	$(CXX) -o $@ $^ $(LDFLAGS)

all: $(EGO_TARGET)

clean:
	rm -f ${EGO_OBJ} $(EGO_TARGET)
