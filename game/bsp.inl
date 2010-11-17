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

/// @file bsp.inl
/// @brief
/// @details

#include "bsp.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static INLINE bool_t BSP_aabb_overlap( const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr );
static INLINE bool_t BSP_aabb_lhs_contains_rhs( const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE bool_t BSP_aabb_overlap( const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr )
{
    /// @details BB@> Do lhs_ptr and rhs_ptr overlap? If rhs_ptr has less dimensions
    ///               than lhs_ptr, just check the lowest common dimensions.

    size_t cnt, min_dim;

    float * rhs_mins, * rhs_maxs, * lhs_mins, * lhs_maxs;

    if ( NULL == lhs_ptr || !lhs_ptr->valid ) return bfalse;
    if ( NULL == rhs_ptr || !rhs_ptr->valid ) return bfalse;

    min_dim = MIN( rhs_ptr->dim, lhs_ptr->dim );
    if ( 0 == min_dim ) return bfalse;

    // the optomizer is supposed to do this stuff all by itself,
    // but isn't
    rhs_mins = rhs_ptr->mins.ary;
    rhs_maxs = rhs_ptr->maxs.ary;
    lhs_mins = lhs_ptr->mins.ary;
    lhs_maxs = lhs_ptr->maxs.ary;

    for ( cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++ )
    {
        if (( *rhs_maxs ) < ( *lhs_mins ) ) return bfalse;
        if (( *rhs_mins ) > ( *lhs_maxs ) ) return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t BSP_aabb_lhs_contains_rhs( const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr )
{
    /// @details BB@> Is rhs_ptr contained within lhs_ptr? If rhs_ptr has less dimensions
    ///               than lhs_ptr, just check the lowest common dimensions.

    size_t cnt, min_dim;

    float * rhs_mins, * rhs_maxs, * lhs_mins, * lhs_maxs;

    if ( NULL == lhs_ptr || !lhs_ptr->valid ) return bfalse;
    if ( NULL == rhs_ptr || !rhs_ptr->valid ) return bfalse;

    min_dim = MIN( rhs_ptr->dim, lhs_ptr->dim );
    if ( 0 == min_dim ) return bfalse;

    // the optomizer is supposed to do this stuff all by itself,
    // but isn't
    rhs_mins = rhs_ptr->mins.ary;
    rhs_maxs = rhs_ptr->maxs.ary;
    lhs_mins = lhs_ptr->mins.ary;
    lhs_maxs = lhs_ptr->maxs.ary;

    for ( cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++ )
    {
        if (( *rhs_maxs ) > ( *lhs_maxs ) ) return bfalse;
        if (( *rhs_mins ) < ( *lhs_mins ) ) return bfalse;
    }

    return btrue;
}

