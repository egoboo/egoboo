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

/// @file hash.c
/// @brief Implementation of the support functions for hash tables
/// @details

#include "hash.h"

#include "egoboo_mem.h"

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
hash_node_t * hash_node_ctor( hash_node_t * pn, void * data )
{
    if ( NULL == pn ) return pn;

    memset( pn, 0, sizeof( *pn ) );

    pn->data = data;

    return pn;
}

//--------------------------------------------------------------------------------------------
hash_node_t * hash_node_create( void * data )
{
    hash_node_t * n = EGOBOO_NEW( hash_node_t );

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
    hash_list_t * rv = EGOBOO_NEW( hash_list_t );

    return hash_list_ctor( rv, size );
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