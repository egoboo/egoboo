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

#include "../egolib/hash.h"

// this include must be the absolute last include
#include "../egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static C_BOOLEAN hash_node_dtor( hash_node_t * n );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
C_BOOLEAN hash_node_dtor( hash_node_t * n )
{
    if ( NULL == n ) return C_FALSE;

    n->data = NULL;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
hash_node_t * hash_node_ctor( hash_node_t * pn, void * data )
{
    if ( NULL == pn ) return pn;

    BLANK_STRUCT_PTR( pn )

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
C_BOOLEAN hash_node_destroy( hash_node_t ** pn )
{
    C_BOOLEAN retval = C_FALSE;

    if ( NULL == pn || NULL == *pn ) return C_FALSE;

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
    /// @author BB
    /// @details count the total number of nodes in the hash list

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

    BLANK_STRUCT_PTR( rv )

    return hash_list_ctor( rv, size );
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN hash_list_destroy( hash_list_t ** plst )
{
    C_BOOLEAN retval = C_FALSE;

    if ( NULL == plst || NULL == *plst ) return C_FALSE;

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
C_BOOLEAN hash_list_set_allocd( hash_list_t *plst, int ival )
{
    if ( NULL == plst ) return C_FALSE;

    plst->allocated = ival;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN hash_list_set_count( hash_list_t *plst, int i, int count )
{
    if ( NULL == plst || NULL == plst->subcount ) return C_FALSE;

    if ( i >= plst->allocated ) return C_FALSE;

    plst->subcount[i] = count;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN hash_list_set_node( hash_list_t *plst, int i, hash_node_t * pnode )
{
    if ( NULL == plst || NULL == plst->sublist ) return C_FALSE;

    if ( i >= plst->allocated ) return C_FALSE;

    plst->sublist[i] = pnode;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN hash_list_free( hash_list_t * lst )
{
    if ( NULL == lst ) return C_FALSE;
    if ( 0 == lst->allocated ) return C_TRUE;

    EGOBOO_DELETE_ARY( lst->subcount );
    EGOBOO_DELETE_ARY( lst->sublist );
    lst->allocated = 0;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN hash_list_alloc( hash_list_t * lst, int size )
{
    if ( NULL == lst ) return C_FALSE;

    hash_list_free( lst );

    lst->subcount = EGOBOO_NEW_ARY( int, size );
    if ( NULL == lst->subcount )
    {
        return C_FALSE;
    }

    lst->sublist = EGOBOO_NEW_ARY( hash_node_t *, size );
    if ( NULL == lst->sublist )
    {
        EGOBOO_DELETE( lst->subcount );
        return C_FALSE;
    }
    else
    {
        int cnt;
        for ( cnt = 0; cnt < size; cnt++ ) lst->sublist[cnt] = NULL;
    }

    lst->allocated = size;

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN hash_list_renew( hash_list_t * lst )
{
    /// @author BB
    /// @details renew the CoNode_t hash table.
    ///
    /// Since we are filling this list with pre-allocated CoNode_t's,
    /// there is no need to delete any of the existing pchlst->sublist elements

    int cnt;

    if ( NULL == lst ) return C_FALSE;

    for ( cnt = 0; cnt < lst->allocated; cnt++ )
    {
        lst->subcount[cnt] = 0;
        lst->sublist[cnt]  = NULL;
    }

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
hash_list_iterator_t * hash_list_iterator_ctor( hash_list_iterator_t * it )
{
    if ( NULL == it ) return NULL;

    BLANK_STRUCT_PTR( it )

    return it;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN hash_list_iterator_set_begin( hash_list_iterator_t * it, hash_list_t * hlst )
{
    int i;

    it = hash_list_iterator_ctor( it );

    if ( NULL == it || NULL == hlst ) return C_FALSE;

    // find the first non-null hash element
    for ( i = 0; i < hlst->allocated && NULL == it->pnode; i++ )
    {
        it->hash  = i;
        it->pnode = hlst->sublist[i];
    }

    return NULL != it->pnode;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN hash_list_iterator_done( hash_list_iterator_t * it, hash_list_t * hlst )
{
    if ( NULL == it || NULL == hlst ) return C_TRUE;

    // the end consition
    if ( it->hash >= hlst->allocated ) return C_TRUE;

    return C_FALSE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN hash_list_iterator_next( hash_list_iterator_t * it, hash_list_t * hlst )
{
    int i, inext;
    hash_node_t * pnext;

    if ( NULL == it || NULL == hlst ) return C_FALSE;

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
