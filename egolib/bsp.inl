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

/// @file egolib/bsp.inl
/// @brief
/// @details

#include "egolib/bsp.h"

#include "egolib/platform.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static INLINE ego_bool BSP_leaf_valid( BSP_leaf_t * L );

static INLINE ego_bool BSP_aabb_empty( const BSP_aabb_t * psrc );
static INLINE ego_bool BSP_aabb_invalidate( BSP_aabb_t * psrc );
static INLINE ego_bool BSP_aabb_self_clear( BSP_aabb_t * psrc );

static INLINE ego_bool BSP_aabb_overlap_with_BSP_aabb( const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr );
static INLINE ego_bool BSP_aabb_contains_BSP_aabb( const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr );

static INLINE ego_bool BSP_aabb_overlap_with_aabb( const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr );
static INLINE ego_bool BSP_aabb_contains_aabb( const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE ego_bool BSP_leaf_valid( BSP_leaf_t * L )
{
    if ( NULL == L ) return ego_false;

    if ( NULL == L->data ) return ego_false;
    if ( L->data_type < 0 ) return ego_false;

    return ego_true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE ego_bool BSP_aabb_empty( const BSP_aabb_t * psrc )
{
    Uint32 cnt;

    if ( NULL == psrc || 0 == psrc->dim  || !psrc->valid ) return ego_true;

    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        if ( psrc->maxs.ary[cnt] <= psrc->mins.ary[cnt] )
            return ego_true;
    }

    return ego_false;
}

//--------------------------------------------------------------------------------------------
static INLINE ego_bool BSP_aabb_invalidate( BSP_aabb_t * psrc )
{
    if ( NULL == psrc ) return ego_false;

    // set it to valid
    psrc->valid = ego_false;

    return ego_true;
}

//--------------------------------------------------------------------------------------------
static INLINE ego_bool BSP_aabb_self_clear( BSP_aabb_t * psrc )
{
    /// @author BB
    /// @details Return this bounding box to an empty state.

    Uint32 cnt;

    if ( NULL == psrc ) return ego_false;

    if ( psrc->dim <= 0 || NULL == psrc->mins.ary || NULL == psrc->mids.ary || NULL == psrc->maxs.ary )
    {
        BSP_aabb_invalidate( psrc );
        return ego_false;
    }

    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        psrc->mins.ary[cnt] = psrc->mids.ary[cnt] = psrc->maxs.ary[cnt] = 0.0f;
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE ego_bool BSP_aabb_overlap_with_BSP_aabb( const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr )
{
    /// @author BB
    /// @details Do lhs_ptr and rhs_ptr overlap? If rhs_ptr has less dimensions
    ///               than lhs_ptr, just check the lowest common dimensions.

    size_t cnt, min_dim;

    const float * rhs_mins, * rhs_maxs, * lhs_mins, * lhs_maxs;

    if ( NULL == lhs_ptr || !lhs_ptr->valid ) return ego_false;
    if ( NULL == rhs_ptr || !rhs_ptr->valid ) return ego_false;

    min_dim = MIN( rhs_ptr->dim, lhs_ptr->dim );
    if ( 0 == min_dim ) return ego_false;

    // the optomizer is supposed to do this stuff all by itself,
    // but isn't
    rhs_mins = rhs_ptr->mins.ary;
    rhs_maxs = rhs_ptr->maxs.ary;
    lhs_mins = lhs_ptr->mins.ary;
    lhs_maxs = lhs_ptr->maxs.ary;

    for ( cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++ )
    {
        if (( *rhs_maxs ) < ( *lhs_mins ) ) return ego_false;
        if (( *rhs_mins ) > ( *lhs_maxs ) ) return ego_false;
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
static INLINE ego_bool BSP_aabb_contains_BSP_aabb( const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr )
{
    /// @author BB
    /// @details Is rhs_ptr contained within lhs_ptr? If rhs_ptr has less dimensions
    ///               than lhs_ptr, just check the lowest common dimensions.

    size_t cnt, min_dim;

    const float * rhs_mins, * rhs_maxs, * lhs_mins, * lhs_maxs;

    if ( NULL == lhs_ptr || !lhs_ptr->valid ) return ego_false;
    if ( NULL == rhs_ptr || !rhs_ptr->valid ) return ego_false;

    min_dim = MIN( rhs_ptr->dim, lhs_ptr->dim );
    if ( 0 == min_dim ) return ego_false;

    // the optomizer is supposed to do this stuff all by itself,
    // but isn't
    rhs_mins = rhs_ptr->mins.ary;
    rhs_maxs = rhs_ptr->maxs.ary;
    lhs_mins = lhs_ptr->mins.ary;
    lhs_maxs = lhs_ptr->maxs.ary;

    for ( cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++ )
    {
        if (( *rhs_maxs ) > ( *lhs_maxs ) ) return ego_false;
        if (( *rhs_mins ) < ( *lhs_mins ) ) return ego_false;
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE ego_bool BSP_aabb_overlap_with_aabb( const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr )
{
    /// @author BB
    /// @details Do lhs_ptr and rhs_ptr overlap? If rhs_ptr has less dimensions
    ///               than lhs_ptr, just check the lowest common dimensions.

    size_t cnt, min_dim;

    const float * rhs_mins, * rhs_maxs, * lhs_mins, * lhs_maxs;

    if ( NULL == lhs_ptr || !lhs_ptr->valid ) return ego_false;
    if ( NULL == rhs_ptr /* || !rhs_ptr->valid */ ) return ego_false;

    min_dim = MIN( 3 /* rhs_ptr->dim */, lhs_ptr->dim );
    if ( 0 == min_dim ) return ego_false;

    // the optomizer is supposed to do this stuff all by itself,
    // but isn't
    rhs_mins = rhs_ptr->mins + 0;
    rhs_maxs = rhs_ptr->maxs + 0;
    lhs_mins = lhs_ptr->mins.ary;
    lhs_maxs = lhs_ptr->maxs.ary;

    for ( cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++ )
    {
        if (( *rhs_maxs ) < ( *lhs_mins ) ) return ego_false;
        if (( *rhs_mins ) > ( *lhs_maxs ) ) return ego_false;
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
static INLINE ego_bool BSP_aabb_contains_aabb( const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr )
{
    /// @author BB
    /// @details Is rhs_ptr contained within lhs_ptr? If rhs_ptr has less dimensions
    ///               than lhs_ptr, just check the lowest common dimensions.

    size_t cnt, min_dim;

    const float * rhs_mins, * rhs_maxs, * lhs_mins, * lhs_maxs;

    if ( NULL == lhs_ptr || !lhs_ptr->valid ) return ego_false;
    if ( NULL == rhs_ptr /* || !rhs_ptr->valid */ ) return ego_false;

    min_dim = MIN( 3 /* rhs_ptr->dim */, lhs_ptr->dim );
    if ( 0 == min_dim ) return ego_false;

    // the optomizer is supposed to do this stuff all by itself,
    // but isn't
    rhs_mins = rhs_ptr->mins + 0;
    rhs_maxs = rhs_ptr->maxs + 0;
    lhs_mins = lhs_ptr->mins.ary;
    lhs_maxs = lhs_ptr->maxs.ary;

    for ( cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++ )
    {
        if (( *rhs_maxs ) > ( *lhs_maxs ) ) return ego_false;
        if (( *rhs_mins ) < ( *lhs_mins ) ) return ego_false;
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif
