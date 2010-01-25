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
hash_list_t * hash_list_ctor( hash_list_t * lst, int hash_size )
{
    if ( NULL == lst ) return NULL;

    if ( hash_size < 0 ) hash_size = 256;

    hash_list_alloc( lst, hash_size );

    return lst;
}

//--------------------------------------------------------------------------------------------
hash_list_t * hash_list_dtor( hash_list_t * lst )
{
    if ( NULL == lst ) return NULL;

    hash_list_free( lst );

    return lst;
}

//--------------------------------------------------------------------------------------------
size_t hash_list_count_nodes( hash_list_t *plst )
{
    /// @details BB@> count the total number of nodes in the hash list

    int    i;
    size_t count = 0;

    if ( NULL == plst ) return 0;

    for ( i = 0; i < plst->allocated; i++ )
    {
        if ( NULL != plst->sublist[i] )
        {
            count += plst->subcount[i];
        }
    }

    return count;
}

//--------------------------------------------------------------------------------------------
hash_list_t * hash_list_create( int size )
{
    hash_list_t * rv = EGOBOO_NEW( hash_list_t );
    if ( NULL == rv ) return NULL;

    memset( rv, 0, sizeof( *rv ) );

    return hash_list_ctor( rv, size );
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_destroy( hash_list_t ** plst )
{
    bool_t retval = bfalse;

    if ( NULL == plst || NULL == *plst ) return bfalse;

    retval = ( NULL != hash_list_dtor( *plst ) );

    EGOBOO_DELETE( *plst );

    return retval;
}

//--------------------------------------------------------------------------------------------
int hash_list_get_allocd( hash_list_t *plst )
{
    if ( NULL == plst ) return 0;

    return plst->allocated;
}

//--------------------------------------------------------------------------------------------
size_t hash_list_get_count( hash_list_t *plst, int i )
{
    if ( NULL == plst || NULL == plst->subcount ) return 0;

    return plst->subcount[i];
}

//--------------------------------------------------------------------------------------------
hash_node_t *  hash_list_get_node( hash_list_t *plst, int i )
{
    if ( NULL == plst || NULL == plst->sublist ) return NULL;

    return plst->sublist[i];
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_set_allocd( hash_list_t *plst, int ival )
{
    if ( NULL == plst ) return bfalse;

    plst->allocated = ival;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_set_count( hash_list_t *plst, int i, int count )
{
    if ( NULL == plst || NULL == plst->subcount ) return bfalse;

    if ( i >= plst->allocated ) return bfalse;

    plst->subcount[i] = count;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_set_node( hash_list_t *plst, int i, hash_node_t * pnode )
{
    if ( NULL == plst || NULL == plst->sublist ) return bfalse;

    if ( i >= plst->allocated ) return bfalse;

    plst->sublist[i] = pnode;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_free( hash_list_t * lst )
{
    if ( NULL == lst ) return bfalse;
    if ( 0 == lst->allocated ) return btrue;

    EGOBOO_DELETE_ARY( lst->subcount );
    EGOBOO_DELETE_ARY( lst->sublist );
    lst->allocated = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_alloc( hash_list_t * lst, int size )
{
    if ( NULL == lst ) return bfalse;

    hash_list_free( lst );

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
    else
    {
        int cnt;
        for ( cnt = 0; cnt < size; cnt++ ) lst->sublist[cnt] = NULL;
    }

    lst->allocated = size;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_renew( hash_list_t * lst )
{
    /// @details BB@> renew the CoNode_t hash table.
    ///
    /// Since we are filling this list with pre-allocated CoNode_t's,
    /// there is no need to delete any of the existing pchlst->sublist elements

    int cnt;

    if ( NULL == lst ) return bfalse;

    for ( cnt = 0; cnt < lst->allocated; cnt++ )
    {
        lst->subcount[cnt] = 0;
        lst->sublist[cnt]  = NULL;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
hash_list_iterator_t * hash_list_iterator_ctor( hash_list_iterator_t * it )
{
    if ( NULL == it ) return NULL;

    memset( it, 0, sizeof( *it ) );

    return it;
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_iterator_set_begin( hash_list_iterator_t * it, hash_list_t * hlst )
{
    int i;

    it = hash_list_iterator_ctor( it );

    if ( NULL == it || NULL == hlst ) return bfalse;

    // find the first non-null hash element
    for ( i = 0; i < hlst->allocated && NULL == it->pnode; i++ )
    {
        it->hash  = i;
        it->pnode = hlst->sublist[i];
    }

    return NULL != it->pnode;
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_iterator_done( hash_list_iterator_t * it, hash_list_t * hlst )
{
    if ( NULL == it || NULL == hlst ) return btrue;

    // the end consition
    if ( it->hash >= hlst->allocated ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t hash_list_iterator_next( hash_list_iterator_t * it, hash_list_t * hlst )
{
    int i, inext;
    hash_node_t * pnext;

    if ( NULL == it || NULL == hlst ) return bfalse;

    inext = it->hash;
    pnext = NULL;
    if ( NULL != it->pnode )
    {
        // try jumping to the next element
        pnext = it->pnode->next;
    }

    if ( NULL == pnext )
    {
        // find the next non-null hash element
        for ( i = it->hash + 1; i < hlst->allocated && NULL == pnext; i++ )
        {
            inext = i;
            pnext = hlst->sublist[i];
        }
    }

    if ( NULL == pnext )
    {
        // could not find one. set the iterator to the end condition.
        inext = hlst->allocated;
    }

    it->hash  = inext;
    it->pnode = pnext;

    return NULL != it->pnode;
}

//--------------------------------------------------------------------------------------------
void * hash_list_iterator_ptr( hash_list_iterator_t * it )
{
    if ( NULL == it || NULL == it->pnode ) return NULL;

    return it->pnode->data;
}
