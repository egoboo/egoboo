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

/// @file egoboo_typedef.c
/// @brief Implementation of the support functions for egoboo's special datatypes
/// @details

#include "egoboo_typedef.h"
#include "egoboo_math.h"

#include "log.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32 ego_object_guid = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t hash_list_dtor( hash_list_t * lst );
static bool_t hash_node_dtor( hash_node_t * n );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t hash_node_dtor( hash_node_t * n )
{
    if ( NULL == n ) return bfalse;

    n->data = NULL;

    return btrue;
}

//--------------------------------------------------------------------------------------------
hash_node_t * hash_node_ctor( hash_node_t * n, void * data )
{
    if ( NULL == n ) return n;

    memset( n, 0, sizeof( hash_node_t ) );

    n->data = data;

    return n;
}

//--------------------------------------------------------------------------------------------
hash_node_t * hash_node_create( void * data )
{
    hash_node_t * n = EGOBOO_NEW( hash_node_t );
    if ( NULL == n ) return n;

    return hash_node_ctor( n, data );
}

//--------------------------------------------------------------------------------------------
bool_t hash_node_destroy( hash_node_t ** pn )
{
    bool_t retval = bfalse;

    if ( NULL == pn || NULL == *pn ) return bfalse;

    retval = hash_node_dtor( *pn );

    EGOBOO_DELETE( *pn );

    return retval;
}

//--------------------------------------------------------------------------------------------
hash_node_t * hash_node_insert_after( hash_node_t lst[], hash_node_t * n )
{
    if ( NULL == n ) return lst;
    n->next = NULL;

    if ( NULL == lst ) return n;

    n->next = n->next;
    lst->next = n;

    return lst;
}

//--------------------------------------------------------------------------------------------
hash_node_t * hash_node_insert_before( hash_node_t lst[], hash_node_t * n )
{
    if ( NULL == n ) return lst;
    n->next = NULL;

    if ( NULL == lst ) return n;

    n->next = lst;

    return n;
}

//--------------------------------------------------------------------------------------------
hash_node_t * hash_node_remove_after( hash_node_t lst[] )
{
    hash_node_t * n;

    if ( NULL == lst ) return NULL;

    n = lst->next;
    if ( NULL == n ) return lst;

    lst->next = n->next;
    n->next   = NULL;

    return lst;
}

//--------------------------------------------------------------------------------------------
hash_node_t * hash_node_remove( hash_node_t lst[] )
{
    hash_node_t * n;

    if ( NULL == lst ) return NULL;

    n = lst->next;
    if ( NULL == n ) return NULL;

    lst->next = NULL;

    return n;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t hash_list_deallocate( hash_list_t * lst )
{
    if ( NULL == lst ) return bfalse;
    if ( 0 == lst->allocated ) return btrue;

    EGOBOO_DELETE( lst->subcount );
    EGOBOO_DELETE( lst->sublist );
    lst->allocated = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_allocate( hash_list_t * lst, int size )
{
    if ( NULL == lst ) return bfalse;

    hash_list_deallocate( lst );

    lst->subcount = EGOBOO_NEW_ARY( int, size );
    if ( NULL == lst->subcount )
    {
        return bfalse;
    }

    lst->sublist = EGOBOO_NEW_ARY( hash_node_t *, size );
    if ( NULL == lst->sublist )
    {
        EGOBOO_DELETE( lst->subcount );
        return bfalse;
    }

    lst->allocated = size;

    return btrue;
}

//--------------------------------------------------------------------------------------------
hash_list_t * hash_list_ctor( hash_list_t * lst, int size )
{
    if ( NULL == lst ) return NULL;

    if ( size < 0 ) size = 256;
    hash_list_allocate( lst, size );

    return lst;
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_dtor( hash_list_t * lst )
{
    if ( NULL == lst ) return bfalse;

    hash_list_deallocate( lst );

    return btrue;
}

//--------------------------------------------------------------------------------------------
hash_list_t * hash_list_create( int size )
{
    return hash_list_ctor( EGOBOO_NEW( hash_list_t ), size );
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_destroy( hash_list_t ** plst )
{
    bool_t retval = bfalse;

    if ( NULL == plst || NULL == *plst ) return bfalse;

    retval = hash_list_dtor( *plst );

    EGOBOO_DELETE( *plst );

    return retval;
}

//--------------------------------------------------------------------------------------------
const char * undo_idsz( IDSZ idsz )
{
    /// @details ZZ@> This function takes an integer and makes a text IDSZ out of it.

    static char value_string[5] = {"NONE"};

    if ( idsz == IDSZ_NONE )
    {
        strncpy( value_string, "NONE", SDL_arraysize( value_string ) );
    }
    else
    {
        // Bad! both function return and return to global variable!
        value_string[0] = (( idsz >> 15 ) & 31 ) + 'A';
        value_string[1] = (( idsz >> 10 ) & 31 ) + 'A';
        value_string[2] = (( idsz >> 5 ) & 31 ) + 'A';
        value_string[3] = (( idsz ) & 31 ) + 'A';
        value_string[4] = 0;
    }

    return value_string;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t irect_point_inside( irect_t * prect, int   ix, int   iy )
{
    if ( NULL == prect ) return bfalse;

    if ( ix < prect->left || ix > prect->right  + 1 ) return bfalse;
    if ( iy < prect->top  || iy > prect->bottom + 1 ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t frect_point_inside( frect_t * prect, float fx, float fy )
{
    if ( NULL == prect ) return bfalse;

    if ( fx < prect->left || fx > prect->right ) return bfalse;
    if ( fy < prect->top  || fy > prect->bottom ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void latch_init( latch_t * platch )
{
    if ( NULL == platch ) return;

    platch->x = 0.0f;
    platch->y = 0.0f;
    platch->b = 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void pair_to_range( IPair pair, FRange * prange )
{
    /// @details ZZ@> convert from a pair to a range

    if ( pair.base < 0 )
    {
        log_warning( "We got a randomization error again! (Base is less than 0)\n" );
    }

    if ( pair.rand < 0 )
    {
        log_warning( "We got a randomization error again! (rand is less than 0)\n" );
    }

    if ( NULL != prange )
    {
        float fFrom, fTo;

        fFrom = FP8_TO_FLOAT( pair.base );
        fTo   = FP8_TO_FLOAT( pair.base + pair.rand );

        prange->from = MIN( fFrom, fTo );
        prange->to   = MAX( fFrom, fTo );
    }
}

//--------------------------------------------------------------------------------------------
void range_to_pair( FRange range, IPair * ppair )
{
    /// @details ZZ@> convert from a range to a pair

    if ( range.from > range.to )
    {
        log_warning( "We got a range error! (to is less than from)\n" );
    }

    if ( NULL != ppair )
    {
        float fFrom, fTo;

        fFrom = MIN( range.from, range.to );
        fTo   = MAX( range.from, range.to );

        ppair->base = FLOAT_TO_FP8( fFrom );
        ppair->rand = FLOAT_TO_FP8( fTo - fFrom );
    }
}

//--------------------------------------------------------------------------------------------
void ints_to_range( int ibase, int irand, FRange * prange )
{
    IPair pair_tmp;

    pair_tmp.base = ibase;
    pair_tmp.rand = irand;

    pair_to_range( pair_tmp, prange );
}

//--------------------------------------------------------------------------------------------
void floats_to_pair( float vmin, float vmax, IPair * ppair )
{
    FRange range_tmp;

    range_tmp.from = vmin;
    range_tmp.to   = vmax;

    range_to_pair( range_tmp, ppair );
}
