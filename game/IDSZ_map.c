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

/// @file IDSZ_map.c
/// @brief

#include "IDSZ_map.h"
#include "log.h"

#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void idsz_map_init( IDSZ_node_t idsz_map[], const size_t idsz_map_len )
{
    size_t i;

    if ( NULL == idsz_map ) return;

    for ( i = 0; i < idsz_map_len; i++ )
    {
        idsz_map[i].id = IDSZ_NONE;
        idsz_map[i].level = IDSZ_NOT_FOUND;
    }
}

//--------------------------------------------------------------------------------------------
egoboo_rv idsz_map_add( IDSZ_node_t idsz_map[], const size_t idsz_map_len, const IDSZ idsz, const int level )
{
    /// @details ZF> Adds a single IDSZ with the specified level to the map. If it already exists
    ///              in the map, the higher of the two level values will be used.

    egoboo_rv rv = rv_error;
    size_t    i, key = 0;

    if ( NULL == idsz_map ) return rv_error;

    // we must allow the function to add any level of IDSZ, otherwize we cannot
    // add beaten quests, skills with negative levels, etc.
    if ( IDSZ_NONE == idsz ) return rv_fail;

    for ( i = 0; i < idsz_map_len; i++ )
    {
        key = ( idsz + i ) % idsz_map_len;

        // found an empty spot?
        if ( IDSZ_NONE == idsz_map[key].id ) break;

        // found a matching idsz?
        if ( idsz == idsz_map[key].id )
        {
            // But only if the new idsz level is "better" than the previous one
            if (( level > 0 && idsz_map[key].level > level ) ||
                ( level < 0 && idsz_map[key].level < level ) )
            {
                rv = rv_fail;
            }
            else
            {
                rv = rv_success;
                idsz_map[key].level = level;
            }

            break;
        }
    }

    //Trying to add a idsz to a full idsz list?
    if ( idsz_map_len == i )
    {
        log_warning( "idsz_map_add() - Failed to add [%s] to an IDSZ_map. Consider increasing idsz_map_len (currently %i)\n", undo_idsz( idsz ), idsz_map_len );

        rv = rv_fail;
    }
    else if ( IDSZ_NONE == idsz_map[key].id )
    {
        //Found an empty place in the list. Simply append the new idsz to idsz_map

        //Add the new idsz
        idsz_map[key].id    = idsz;
        idsz_map[key].level = level;

        rv = rv_success;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
IDSZ_node_t* idsz_map_get( const IDSZ_node_t idsz_map[], const size_t idsz_map_len, const IDSZ idsz )
{
    /// @details ZF> This function returns a pointer to the IDSZ_node_t from the IDSZ specified
    ///              or NULL if it wasn't found in the map.

    int i;
    size_t key;
    IDSZ_node_t* found_node = NULL;

    if ( NULL == idsz_map || IDSZ_NONE == idsz ) return NULL;

    // iterate the map
    for ( i = 0; i < idsz_map_len; i++ )
    {
        key = ( idsz + i ) % idsz_map_len;

        // found an empty spot, it's not in here
        if ( IDSZ_NONE == idsz_map[key].id ) break;

        // found a matching idsz?
        if ( idsz == idsz_map[key].id )
        {
            found_node = ( IDSZ_node_t * ) & idsz_map[key];
        }
    }

    return found_node;
}

//--------------------------------------------------------------------------------------------
IDSZ_node_t* idsz_map_iterate( const IDSZ_node_t idsz_map[], const size_t idsz_map_len, int *iterator_ptr )
{
    /// @details ZF> This function iterates through a map containing any number of IDSZ_node_t
    ///              Returns NULL if there are no more elements to iterate.

    int step = 0;

    if ( NULL == idsz_map || NULL == iterator_ptr ) return NULL;

    // alias the variable
    step = *iterator_ptr;

    // Reached the end of the list without finding a matching idsz
    if ( step < 0  || ( size_t )step >= idsz_map_len ) return NULL;

    // Keep looking until we actually find one
    while ( IDSZ_NONE == idsz_map[step].id )
    {
        step++;
        if (( size_t )step >= idsz_map_len ) return NULL;
    }

    // Increment the iterator for the next iteration
    *iterator_ptr = step     + 1;

    // Return the next element we found from the map
    return ( IDSZ_node_t * )( idsz_map + step );
}

//--------------------------------------------------------------------------------------------
egoboo_rv idsz_map_copy( const IDSZ_node_t map_src[], const size_t src_len, IDSZ_node_t map_dst[] )
{
    ///@details ZF@> This function copies one set of IDSZ map to another IDSZ map (exact)

    if ( map_src == NULL || map_dst == NULL || 0 == src_len ) return rv_error;

    // memcpy() is probably a lot more efficient than copying each element individually
    memmove( map_dst, map_src, sizeof( IDSZ_node_t ) * src_len );

    return rv_success;
}
