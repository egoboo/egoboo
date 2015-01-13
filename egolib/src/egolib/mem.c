//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/mem.c
/// @details Allocation, reallocation, deallocation and manipulation of memory blocks and array memory blocks

#include "egolib/mem.h"

#include <cstdlib>

void *EgoNew(size_t sz) {
	if (sz == 0) {
		return malloc(1);    // circumvent implementation defined behaviour
	} else {
		return malloc(sz);
	}
}

void *EgoNew(size_t sz, size_t esz) {
	if (sz == 0 || esz == 0) {
		return calloc(1, 1); // circumvent implementation defined behaviour
	} else {
		return calloc(sz, esz);
	}
}
