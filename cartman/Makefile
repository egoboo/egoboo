# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files
CARTMAN_SRC := ${wildcard src/cartman/*.c}
CARTMAN_OBJ := ${CARTMAN_SRC:.c=.o}

#---------------------
# the egolib configuration

EGOLIB_L := ../egolib/$(EGOLIB_TARGET)
IDLIB_L  := ../idlib/$(IDLIB_TARGET)

#---------------------
# the compiler options

INC      := -Isrc -I../egolib/src -I../idlib/src

CFLAGS   += $(INC)
CXXFLAGS += $(INC)
LDFLAGS  := ${EGOLIB_L} ${IDLIB_L} $(LDFLAGS)

#------------------------------------
# definitions of the target projects

.PHONY: all clean

$(CARTMAN_TARGET): ${CARTMAN_OBJ}
	$(CXX) -o $@ $^ $(LDFLAGS)

all: $(CARTMAN_TARGET)

clean:
	rm -f ${CARTMAN_OBJ} $(CARTMAN_TARGET)
