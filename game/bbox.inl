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

/// @file bbox.inl
/// @brief
/// @details

#include "bbox.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static INLINE bool_t oct_vec_ctor( oct_vec_t ovec, const fvec3_base_t pos );
static INLINE bool_t oct_vec_self_clear( oct_vec_t * ovec );
static INLINE bool_t oct_vec_add_fvec3( const oct_vec_t osrc, const fvec3_base_t fvec, oct_vec_t odst );
static INLINE bool_t oct_vec_self_add_fvec3( oct_vec_t osrc, const fvec3_base_t fvec );

static INLINE oct_bb_t * oct_bb_ctor( oct_bb_t * pobb );
static INLINE egoboo_rv oct_bb_set_bumper( oct_bb_t * pobb, const bumper_t src );
static INLINE egoboo_rv oct_bb_copy( oct_bb_t * pdst, const oct_bb_t * psrc );
static INLINE egoboo_rv oct_bb_validate( oct_bb_t * pobb );
static INLINE bool_t oct_bb_empty_raw( const oct_bb_t * pbb );
static INLINE bool_t oct_bb_empty( const oct_bb_t * pbb );
static INLINE egoboo_rv  oct_bb_set_ovec( oct_bb_t * pobb, const oct_vec_t ovec );
static INLINE oct_bb_t * oct_bb_ctor_index( oct_bb_t * pobb, int index );
static INLINE egoboo_rv oct_bb_copy_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index );
static INLINE egoboo_rv oct_bb_validate_index( oct_bb_t * pobb, int index );
static INLINE bool_t oct_bb_empty_index_raw( const oct_bb_t * pbb, int index );
static INLINE bool_t oct_bb_empty_index( const oct_bb_t * pbb, int index );
static INLINE egoboo_rv oct_bb_union_index( const oct_bb_t * psrc1, const oct_bb_t  * psrc2, oct_bb_t * pdst, int index );
static INLINE egoboo_rv oct_bb_intersection_index( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst, int index );
static INLINE egoboo_rv oct_bb_self_union_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index );
static INLINE egoboo_rv oct_bb_self_intersection_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index );
static INLINE egoboo_rv oct_bb_union( const oct_bb_t * psrc1, const oct_bb_t  * psrc2, oct_bb_t * pdst );
static INLINE egoboo_rv oct_bb_intersection( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst );
static INLINE egoboo_rv oct_bb_self_union( oct_bb_t * pdst, const oct_bb_t * psrc );
static INLINE egoboo_rv oct_bb_self_intersection( oct_bb_t * pdst, const oct_bb_t * psrc );
static INLINE egoboo_rv oct_bb_add_fvec3( const oct_bb_t * psrc, const fvec3_base_t vec, oct_bb_t * pdst );
static INLINE egoboo_rv oct_bb_self_add_fvec3( oct_bb_t * pdst, const fvec3_base_t vec );
static INLINE egoboo_rv oct_bb_add_ovec( const oct_bb_t * psrc, const oct_vec_t ovec, oct_bb_t * pdst );
static INLINE egoboo_rv oct_bb_self_add_ovec( oct_bb_t * pdst, const oct_vec_t ovec );
static INLINE egoboo_rv oct_bb_self_sum_ovec( oct_bb_t * pdst, const oct_vec_t ovec );
static INLINE egoboo_rv oct_bb_self_grow( oct_bb_t * pdst, const oct_vec_t ovec );
static INLINE bool_t oct_bb_point_inside( const oct_bb_t * pobb, const oct_vec_t ovec );
static INLINE bool_t oct_bb_lhs_contains_rhs( const oct_bb_t * plhs, const oct_bb_t * prhs );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static INLINE bool_t oct_vec_ctor( oct_vec_t ovec, const fvec3_base_t pos )
{
    if ( NULL == ovec ) return bfalse;

    ovec[OCT_X ] =  pos[kX];
    ovec[OCT_Y ] =  pos[kY];
    ovec[OCT_Z ] =  pos[kZ];
    ovec[OCT_XY] =  pos[kX] + pos[kY];
    ovec[OCT_YX] = -pos[kX] + pos[kY];

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t oct_vec_self_clear( oct_vec_t * ovec )
{
    int cnt;

    if ( NULL == ovec ) return bfalse;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        ( *ovec )[cnt] = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t oct_vec_add_fvec3( const oct_vec_t osrc, const fvec3_base_t fvec, oct_vec_t odst )
{
    if ( NULL == odst ) return bfalse;

    oct_vec_ctor( odst, fvec );

    if ( NULL != osrc )
    {
        int cnt;

        for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
        {
            odst[cnt] += osrc[cnt];
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t oct_vec_self_add_fvec3( oct_vec_t osrc, const fvec3_base_t fvec )
{
    int cnt;
    oct_vec_t otmp;

    if ( NULL == osrc ) return bfalse;

    oct_vec_ctor( otmp, fvec );

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        osrc[cnt] += otmp[cnt];
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE oct_bb_t * oct_bb_ctor( oct_bb_t * pobb )
{
    if ( NULL == pobb ) return NULL;

    memset( pobb, 0, sizeof( *pobb ) );

    pobb->empty = btrue;

    return pobb;
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_set_bumper( oct_bb_t * pobb, const bumper_t src )
{
    if ( NULL == pobb ) return rv_error;

    pobb->mins[OCT_X] = -src.size;
    pobb->maxs[OCT_X] =  src.size;

    pobb->mins[OCT_Y] = -src.size;
    pobb->maxs[OCT_Y] =  src.size;

    pobb->mins[OCT_XY] = -src.size_big;
    pobb->maxs[OCT_XY] =  src.size_big;

    pobb->mins[OCT_YX] = -src.size_big;
    pobb->maxs[OCT_YX] =  src.size_big;

    pobb->mins[OCT_Z] = -src.height;
    pobb->maxs[OCT_Z] =  src.height;

    return oct_bb_validate( pobb );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_copy( oct_bb_t * pdst, const oct_bb_t * psrc )
{
    if ( NULL == pdst ) return rv_error;

    if ( NULL == psrc || psrc->empty )
    {
        oct_bb_ctor( pdst );
        return rv_success;
    }

    memmove( pdst, psrc, sizeof( *pdst ) );

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_validate( oct_bb_t * pobb )
{
    if ( NULL == pobb ) return rv_error;

    pobb->empty = oct_bb_empty_raw( pobb );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t oct_bb_empty_raw( const oct_bb_t * pbb )
{
    int cnt;
    bool_t rv = bfalse;

    for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
    {
        if ( pbb->mins[cnt] >= pbb->maxs[cnt] )
        {
            rv = btrue;
            break;
        }
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t oct_bb_empty( const oct_bb_t * pbb )
{
    if ( NULL == pbb || pbb->empty ) return btrue;

    return oct_bb_empty_raw( pbb );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv  oct_bb_set_ovec( oct_bb_t * pobb, const oct_vec_t ovec )
{
    int cnt;

    if ( NULL == pobb ) return rv_error;

    if ( NULL == ovec )
    {
        oct_bb_ctor( pobb ) ;
        return rv_fail;
    }

    // copy the data over
    for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
    {
        pobb->mins[cnt] = pobb->maxs[cnt] = ovec[cnt];
    }

    // this is true by the definition of this function
    pobb->empty = btrue;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE oct_bb_t * oct_bb_ctor_index( oct_bb_t * pobb, int index )
{
    if ( NULL == pobb ) return NULL;

    if ( index >= 0 && index < OCT_COUNT )
    {
        pobb->mins[index] = pobb->maxs[index] = 0.0f;
        pobb->empty = btrue;
    }

    return pobb;
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_copy_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index )
{
    if ( NULL == pdst ) return rv_error;

    if ( NULL == psrc || psrc->empty )
    {
        oct_bb_ctor_index( pdst, index );
        return rv_success;
    }

    pdst->mins[index] = psrc->mins[index];
    pdst->maxs[index] = psrc->maxs[index];

    return oct_bb_validate_index( pdst, index );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_validate_index( oct_bb_t * pobb, int index )
{
    if ( NULL == pobb ) return rv_error;

    if ( oct_bb_empty_index( pobb, index ) )
    {
        pobb->empty = btrue;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t oct_bb_empty_index_raw( const oct_bb_t * pbb, int index )
{
    return pbb->mins[index] >= pbb->maxs[index];
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t oct_bb_empty_index( const oct_bb_t * pbb, int index )
{
    if ( NULL == pbb || pbb->empty ) return btrue;

    if ( index < 0 || index >= OCT_COUNT ) return btrue;

    return oct_bb_empty_index_raw( pbb, index );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_union_index( const oct_bb_t * psrc1, const oct_bb_t  * psrc2, oct_bb_t * pdst, int index )
{
    /// @details BB@> find the union of two oct_bb_t

    bool_t src1_empty, src2_empty;

    if ( NULL == pdst ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    src1_empty = ( NULL == psrc1 || psrc1->empty );
    src2_empty = ( NULL == psrc2 || psrc2->empty );

    if ( src1_empty && src2_empty )
    {
        oct_bb_ctor_index( pdst, index );
        return rv_fail;
    }
    else if ( src2_empty )
    {
        oct_bb_copy_index( pdst, psrc1, index );
        return rv_success;
    }
    else if ( src1_empty )
    {
        oct_bb_copy_index( pdst, psrc2, index );
        return rv_success;
    }

    // no simple case, do the hard work

    pdst->mins[index]  = MIN( psrc1->mins[index],  psrc2->mins[index] );
    pdst->maxs[index]  = MAX( psrc1->maxs[index],  psrc2->maxs[index] );

    return oct_bb_validate_index( pdst, index );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_intersection_index( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst, int index )
{
    /// @details BB@> find the intersection of two oct_bb_t

    bool_t src1_empty, src2_empty;

    if ( NULL == pdst ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    src1_empty = ( NULL == psrc1 || psrc1->empty );
    src2_empty = ( NULL == psrc2 || psrc2->empty );

    if ( src1_empty && src2_empty )
    {
        oct_bb_ctor_index( pdst, index );
        return rv_success;
    }

    // no simple case. do the hard work

    pdst->mins[index]  = MAX( psrc1->mins[index],  psrc2->mins[index] );
    pdst->maxs[index]  = MIN( psrc1->maxs[index],  psrc2->maxs[index] );

    return oct_bb_validate_index( pdst, index );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv  oct_bb_self_union_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index )
{
    /// @details BB@> find the union of two oct_bb_t

    bool_t dst_empty, src_empty;

    if ( NULL == pdst ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    dst_empty = pdst->empty;
    src_empty = ( NULL == psrc || psrc->empty );

    if ( src_empty )
    {
        // !!!! DO NOTHING !!!!
        return rv_success;
    }
    else if ( dst_empty )
    {
        oct_bb_copy_index( pdst, psrc, index );
        return rv_success;
    }

    // no simple case, do the hard work

    pdst->mins[index]  = MIN( pdst->mins[index],  psrc->mins[index] );
    pdst->maxs[index]  = MAX( pdst->maxs[index],  psrc->maxs[index] );

    return oct_bb_validate_index( pdst, index );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_self_intersection_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index )
{
    /// @details BB@> find the intersection of two oct_bb_t

    bool_t dst_empty, src_empty;

    if ( NULL == pdst ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    dst_empty = pdst->empty;
    src_empty = ( NULL == psrc || psrc->empty );

    if ( dst_empty && src_empty )
    {
        oct_bb_ctor_index( pdst, index );
        return rv_fail;
    }

    // no simple case. do the hard work

    pdst->mins[index]  = MAX( pdst->mins[index],  psrc->mins[index] );
    pdst->maxs[index]  = MIN( pdst->maxs[index],  psrc->maxs[index] );

    return oct_bb_validate_index( pdst, index );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_union( const oct_bb_t * psrc1, const oct_bb_t  * psrc2, oct_bb_t * pdst )
{
    /// @details BB@> find the union of two oct_bb_t

    bool_t src1_null, src2_null;
    int cnt;

    if ( NULL == pdst ) return rv_error;

    src1_null = ( NULL == psrc1 );
    src2_null = ( NULL == psrc2 );

    if ( src1_null && src2_null )
    {
        oct_bb_ctor( pdst );
        return rv_fail;
    }
    else if ( src2_null )
    {
        return oct_bb_copy( pdst, psrc1 );
    }
    else if ( src1_null )
    {
        return oct_bb_copy( pdst, psrc2 );
    }

    // no simple case, do the hard work
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt]  = MIN( psrc1->mins[cnt],  psrc2->mins[cnt] );
        pdst->maxs[cnt]  = MAX( psrc1->maxs[cnt],  psrc2->maxs[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_intersection( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst )
{
    /// @details BB@> find the intersection of two oct_bb_t

    bool_t src1_empty, src2_empty;
    int cnt;

    if ( NULL == pdst ) return rv_error;

    src1_empty = ( NULL == psrc1 || psrc1->empty );
    src2_empty = ( NULL == psrc2 || psrc2->empty );

    if ( src1_empty && src2_empty )
    {
        oct_bb_ctor( pdst );
        return rv_fail;
    }

    // no simple case. do the hard work
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt]  = MAX( psrc1->mins[cnt],  psrc2->mins[cnt] );
        pdst->maxs[cnt]  = MIN( psrc1->maxs[cnt],  psrc2->maxs[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_self_union( oct_bb_t * pdst, const oct_bb_t * psrc )
{
    /// @details BB@> find the union of two oct_bb_t

    bool_t dst_null, src_null;
    int cnt;

    dst_null = ( NULL == pdst );

    if ( dst_null ) return rv_error;

    src_null = ( NULL == psrc );

    if ( src_null )
    {
        // !!!! DO NOTHING !!!!
        return rv_success;
    }

    // no simple case, do the hard work
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt]  = MIN( pdst->mins[cnt],  psrc->mins[cnt] );
        pdst->maxs[cnt]  = MAX( pdst->maxs[cnt],  psrc->maxs[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_self_intersection( oct_bb_t * pdst, const oct_bb_t * psrc )
{
    /// @details BB@> find the intersection of two oct_bb_t

    bool_t dst_empty, src_empty;
    int cnt;

    if ( NULL == pdst ) return rv_error;

    dst_empty = pdst->empty;
    src_empty = ( NULL == psrc || psrc->empty );

    if ( dst_empty && src_empty )
    {
        oct_bb_ctor( pdst );
        return rv_fail;
    }

    // no simple case. do the hard work
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt]  = MAX( pdst->mins[cnt],  psrc->mins[cnt] );
        pdst->maxs[cnt]  = MIN( pdst->maxs[cnt],  psrc->maxs[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_add_fvec3( const oct_bb_t * psrc, const fvec3_base_t vec, oct_bb_t * pdst )
{
    /// @details BB@> shift the bounding box by the vector vec

    if ( NULL == pdst ) return rv_error;

    if ( NULL == psrc )
    {
        oct_bb_ctor( pdst );
    }
    else
    {
        oct_bb_copy( pdst, psrc );
    }

    pdst->mins[OCT_X]  += vec[kX];
    pdst->maxs[OCT_X]  += vec[kX];

    pdst->mins[OCT_Y]  += vec[kY];
    pdst->maxs[OCT_Y]  += vec[kY];

    pdst->mins[OCT_XY] += vec[kX] + vec[kY];
    pdst->maxs[OCT_XY] += vec[kX] + vec[kY];

    pdst->mins[OCT_YX] += -vec[kX] + vec[kY];
    pdst->maxs[OCT_YX] += -vec[kX] + vec[kY];

    pdst->mins[OCT_Z]  += vec[kZ];
    pdst->maxs[OCT_Z]  += vec[kZ];

    return rv_success;
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_self_add_fvec3( oct_bb_t * pdst, const fvec3_base_t vec )
{
    /// @details BB@> shift the bounding box by the vector vec

    if ( NULL == pdst ) return rv_error;

    if ( NULL == vec ) return rv_success;

    pdst->mins[OCT_X]  += vec[kX];
    pdst->maxs[OCT_X]  += vec[kX];

    pdst->mins[OCT_Y]  += vec[kY];
    pdst->maxs[OCT_Y]  += vec[kY];

    pdst->mins[OCT_XY] += vec[kX] + vec[kY];
    pdst->maxs[OCT_XY] += vec[kX] + vec[kY];

    pdst->mins[OCT_YX] += -vec[kX] + vec[kY];
    pdst->maxs[OCT_YX] += -vec[kX] + vec[kY];

    pdst->mins[OCT_Z]  += vec[kZ];
    pdst->maxs[OCT_Z]  += vec[kZ];

    return rv_success;
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_add_ovec( const oct_bb_t * psrc, const oct_vec_t ovec, oct_bb_t * pdst )
{
    /// @details BB@> shift the bounding box by the vector ovec

    int cnt;

    if ( NULL == pdst ) return rv_error;

    if ( NULL == psrc )
    {
        oct_bb_ctor( pdst );
    }
    else
    {
        oct_bb_copy( pdst, psrc );
    }

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt] += ovec[cnt];
        pdst->maxs[cnt] += ovec[cnt];
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv oct_bb_self_add_ovec( oct_bb_t * pdst, const oct_vec_t ovec )
{
    /// @details BB@> shift the bounding box by the vector ovec

    int cnt;

    if ( NULL == pdst ) return rv_error;

    if ( NULL == ovec ) return rv_success;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt] += ovec[cnt];
        pdst->maxs[cnt] += ovec[cnt];
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv  oct_bb_self_sum_ovec( oct_bb_t * pdst, const oct_vec_t ovec )
{
    int cnt;

    if ( NULL == pdst ) return rv_fail;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt] = MIN( pdst->mins[cnt], ovec[cnt] );
        pdst->maxs[cnt] = MAX( pdst->maxs[cnt], ovec[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
static INLINE egoboo_rv  oct_bb_self_grow( oct_bb_t * pdst, const oct_vec_t ovec )
{
    int cnt;

    if ( NULL == pdst ) return rv_error;
    if ( NULL == ovec ) return rv_error;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt] = pdst->mins[cnt] - ABS( ovec[cnt] );
        pdst->maxs[cnt] = pdst->maxs[cnt] + ABS( ovec[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t oct_bb_point_inside( const oct_bb_t * pobb, const oct_vec_t ovec )
{
    int cnt;

    if ( NULL == pobb || pobb->empty ) return bfalse;

    if ( NULL == ovec ) return bfalse;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        if ( ovec[cnt] < pobb->mins[cnt] ) return bfalse;
        if ( ovec[cnt] > pobb->maxs[cnt] ) return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t oct_bb_lhs_contains_rhs( const oct_bb_t * plhs, const oct_bb_t * prhs )
{
    int cnt;

    if ( NULL == plhs || plhs->empty ) return bfalse;

    if ( NULL == prhs ) return bfalse;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        if ( prhs->maxs[cnt] > plhs->maxs[cnt] ) return bfalse;
        if ( prhs->mins[cnt] < plhs->mins[cnt] ) return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t oct_bb_get_mids( const oct_bb_t * pbb, oct_vec_t mids )
{
    if ( NULL == pbb || NULL == mids ) return bfalse;

    if ( oct_bb_empty( pbb ) )
    {
        memmove( mids, pbb->maxs, sizeof( oct_vec_t ) );
    }
    else
    {
        int cnt;

        for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
        {
            mids[cnt] = 0.5f * ( pbb->mins[cnt] + pbb->maxs[cnt] );
        }
    }

    return btrue;
}
