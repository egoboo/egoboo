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

/// @file hash.h
/// @details Implementation of the "efficient" hash node storage.

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a hash type for "efficiently" storing data
struct s_hash_node
{
    struct s_hash_node * next;

    void * data;
};
typedef struct s_hash_node hash_node_t;

hash_node_t * hash_node_create( void * data );
bool_t        hash_node_destroy( hash_node_t ** );
hash_node_t * hash_node_ctor( hash_node_t * n, void * data );
hash_node_t * hash_node_insert_after( hash_node_t lst[], hash_node_t * n );
hash_node_t * hash_node_insert_before( hash_node_t lst[], hash_node_t * n );
hash_node_t * hash_node_remove_after( hash_node_t lst[] );
hash_node_t * hash_node_remove( hash_node_t lst[] );

//--------------------------------------------------------------------------------------------
struct s_hash_list
{
    int            allocated;
    int         *  subcount;
    hash_node_t ** sublist;
};
typedef struct s_hash_list hash_list_t;

hash_list_t * hash_list_create( int size );
bool_t        hash_list_destroy( hash_list_t ** );
hash_list_t * hash_list_ctor( hash_list_t * lst, int size );
hash_list_t * hash_list_dtor( hash_list_t * lst );
bool_t        hash_list_free( hash_list_t * lst );
bool_t        hash_list_alloc( hash_list_t * lst, int size );
bool_t        hash_list_renew( hash_list_t * lst );

size_t        hash_list_count_nodes( hash_list_t *plst );
int           hash_list_get_allocd( hash_list_t *plst );
size_t        hash_list_get_count( hash_list_t *plst, int i );
hash_node_t * hash_list_get_node( hash_list_t *plst, int i );

bool_t        hash_list_set_allocd( hash_list_t *plst,        int );
bool_t        hash_list_set_count( hash_list_t *plst, int i, int );
bool_t        hash_list_set_node( hash_list_t *plst, int i, hash_node_t * );

bool_t        hash_list_insert_unique( hash_list_t * phash, hash_node_t * pnode );

//--------------------------------------------------------------------------------------------

/// An iterator element for traversing the hash_list_t
struct s_hash_list_iterator
{
    int           hash;
    hash_node_t * pnode;
};

typedef struct s_hash_list_iterator hash_list_iterator_t;

hash_list_iterator_t * hash_list_iterator_ctor( hash_list_iterator_t * it );
void                 * hash_list_iterator_ptr( hash_list_iterator_t * it );
bool_t                 hash_list_iterator_set_begin( hash_list_iterator_t * it, hash_list_t * hlst );
bool_t                 hash_list_iterator_done( hash_list_iterator_t * it, hash_list_t * hlst );
bool_t                 hash_list_iterator_next( hash_list_iterator_t * it, hash_list_t * hlst );
