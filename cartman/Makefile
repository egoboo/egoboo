# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files
CARTMAN_SRC := ${wildcard src/cartman/*.c}
CARTMAN_OBJ := ${CARTMAN_SRC:.c=.o}

#---------------------
# the egolib configuration

EGOLIB_L := ../egolib/$(EGOLIB_TARGET)

#---------------------
# the compiler options

INC      := -Isrc -I../egolib/src

CFLAGS   += $(INC)
CXXFLAGS += $(INC)
LDFLAGS  += ${EGOLIB_L}

#------------------------------------
# definitions of the target projects

.PHONY: all clean

all: $(CARTMAN_TARGET)

$(CARTMAN_TARGET): ${CARTMAN_OBJ} ${EGOLIB_L}
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f ${CARTMAN_OBJ} $(CARTMAN_TARGET)
