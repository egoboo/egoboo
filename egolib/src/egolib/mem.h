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

/// @file egolib/mem.h
/// @details Allocation, reallocation, deallocation and manipulation of memory blocks and array memory blocks

#pragma once

#include "egolib/platform.h"

/**
 * @brief
 *	Allocate a memory block of @a sz Bytes.
 * @param sz
 *	the size, in Bytes, of the memory block
 * @return
 *	a pointer to the memory block on success, @a NULL on failure
 * @remark
 *	@a 0 is a valid value for @a sz.
 *  This corresponds to C++ semantics where <tt>new T[0]</tt> for some type @a T returns a non-null pointer.
 */
void *EgoNew(size_t sz);

/**
 * @brief
 *	Allocate an array memory block of @a sz*esz Bytes.
 * @param sz the number of elements in the array
 * @param esz the size, in Bytes, of an element of the array
 * @return a pointer to the memory block on success, @a NULL on failure
 * @remark
 *	@a 0 is a valid value for @a sz and/or @a esz.
 *  This corresponds to C++ semantics where <tt>new T[0]</tt> for some type @a T returns a non-null pointer.
 */
void *EgoNew(size_t sz, size_t esz);

#define EGOBOO_NEW(TYPE) (TYPE *)EgoNew(1, sizeof(TYPE))
#define EGOBOO_NEW_ARY(TYPE,COUNT) (TYPE *)EgoNew(COUNT, sizeof(TYPE))

#define EGOBOO_DELETE(PTR) if(nullptr != (PTR)) { free(PTR); (PTR) = nullptr; }
#define EGOBOO_DELETE_ARY(PTR) if(nullptr != (PTR)) { free(PTR); (PTR) = nullptr; }
