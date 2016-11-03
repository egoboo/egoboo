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

#include "game/egoboo.h"

//Forward declarations
class Object;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Bits that tell you which variables to look at
enum matrix_cache_type_t
{
    MAT_UNKNOWN   = 0,
    MAT_CHARACTER = ( 1 << 0 ),
    MAT_WEAPON    = ( 1 << 1 )
};


/// the data necessary to cache the last values required to create the character matrix
struct matrix_cache_t : public Id::Equatable<matrix_cache_t>
{
    matrix_cache_t() :
        valid(false),
        matrix_valid(false),
        type_bits(MAT_UNKNOWN),
        rotate(Facing(0), Facing(0), Facing(0)),
        pos(),
        grip_chr(),
        grip_slot(SLOT_LEFT),
        grip_verts(),
        grip_scale(),
        self_scale()
    {
        grip_verts.fill(0xFFFF);
    }

    bool valid;    // is the cache data valid?

    // is the matrix data valid?
    bool matrix_valid;

    // how was the matrix made?
    int type_bits;

    //---- MAT_CHARACTER data

    // the "Euler" rotation angles in 16-bit form
    EulerFacing rotate;

    // the translate vector
    Vector3f   pos;

    //---- MAT_WEAPON data

    ObjectRef grip_chr;                   ///< != ObjectRef::Invalid if character is a held weapon
    slot_t  grip_slot;                  ///< SLOT_LEFT or SLOT_RIGHT
    std::array<uint16_t, GRIP_VERTS> grip_verts;     ///< Vertices which describe the weapon grip
    Vector3f grip_scale;

    //---- data used for both

    // the body fixed scaling
    Vector3f  self_scale;

    /**
     * Get if this matrix cache is valid.
     * @return @a true if this matrix cache is valid, @a false otherwise
     * @remark A matrix cache is valid if the cache is valid and the matrix is valid.
     */
    bool isValid() const;

	// CRTP
    bool equalTo(const matrix_cache_t& other) const;
};

//Function prototypes
bool    chr_matrix_valid( const Object * pchr );
egolib_rv chr_update_matrix( Object * pchr, bool update_size );
bool set_weapongrip( const ObjectRef iitem, const ObjectRef iholder, uint16_t vrt_off );
bool chr_getMatUp(Object *pchr, Vector3f& up);
void make_one_character_matrix( const ObjectRef object_ref );
bool chr_calc_grip_cv( Object * pmount, int grip_offset, oct_bb_t * grip_cv_ptr, const bool shift_origin );
