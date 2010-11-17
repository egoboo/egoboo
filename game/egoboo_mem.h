#pragma once

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

/// @file egoboo_mem.h
/// @details Macros to control allocation and deallocation of memory

#include <memory.h>
#include <string.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define EGOBOO_NEW( TYPE ) (TYPE *)calloc(1, sizeof(TYPE))
#define EGOBOO_NEW_ARY( TYPE, COUNT ) (TYPE *)calloc(COUNT, sizeof(TYPE))
#define EGOBOO_DELETE(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }
#define EGOBOO_DELETE_ARY(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define egoboo_mem_h
