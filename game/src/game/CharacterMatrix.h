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

/// @file game/CharacterMatrix.h

#pragma once

#include "game/egoboo_typedef.h"

//Forward declarations
class Object;

//Function prototypes
bool    chr_matrix_valid( const Object * pchr );
egolib_rv chr_update_matrix( Object * pchr, bool update_size );
bool set_weapongrip( const CHR_REF iitem, const CHR_REF iholder, uint16_t vrt_off );
