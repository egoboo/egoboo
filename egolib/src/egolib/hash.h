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

/// @file    egolib/hash.h
/// @details Implementation of the "efficient" hash node storage.

#pragma once

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct hash_node_t;
    struct hash_list_t;
    struct hash_list_iterator_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

	/**
	 * @brief
	 *	A node in a hash list.
	 */
    struct hash_node_t
    {
        hash_node_t *next;
        void *data;
    };

    hash_node_t *hash_node_create( void * data );
    bool         hash_node_destroy( hash_node_t ** );
    hash_node_t *hash_node_ctor(hash_node_t *self, void * data);
    hash_node_t *hash_node_insert_after( hash_node_t lst[], hash_node_t * n );
    hash_node_t *hash_node_insert_before( hash_node_t lst[], hash_node_t * n );
    hash_node_t *hash_node_remove_after( hash_node_t lst[] );
    hash_node_t *hash_node_remove( hash_node_t lst[] );

//--------------------------------------------------------------------------------------------
    struct hash_list_t
    {
		size_t capacity; /**< @brief The capacity of the hash list. */
        int *subcount;
        hash_node_t **sublist;
    };

	/**
	 * @brief
	 *	Heap-allocate and construct a hash list.
	 * @param capacity
	 *	the initial capacity of the hash list
	 * @return
	 *	a pointer to the hash list on success, @a NULL on failure
	 */
    hash_list_t *hash_list_create(size_t capacity);
	/**
	 * @brief
	 *	Destruct and heap-deallocate a hash list.
	 * @param self
	 *	the hash list
	 */
	void hash_list_destroy(hash_list_t *self);
	/**
	 * @brief
	 *	Construct a hash list.
	 * @param self
	 *	the hash list
	 * @param capacity
	 *	the initial capacity of the hash list
	 * @return
	 *	the hash list on success, @ NULL on failure
	 */
    hash_list_t *hash_list_ctor(hash_list_t *self, size_t capacity);
	/**
	 * @brief
	 *	Destruct a hash list.
	 * @param self
	 *	the hash list
	 */
    void hash_list_dtor(hash_list_t *self);
    bool hash_list_free(hash_list_t *self);
    bool hash_list_alloc(hash_list_t *self, size_t capacity);
	/// @author BB
	/// @details renew the CoNode_t hash table.
	///
	/// Since we are filling this list with pre-allocated CoNode_t's,
	/// there is no need to delete any of the existing pchlst->sublist elements
    bool hash_list_renew(hash_list_t *self);

	/**
	 * @brief
	 *	Count the total number of nodes in the hash list.
	 * @param self
	 *	the hash list
	 * @return
	 *	the number of nodes
	 */
    size_t hash_list_count_nodes(hash_list_t *self);
    int hash_list_get_allocd(hash_list_t *self);
    size_t hash_list_get_count(hash_list_t *self, size_t index);
    hash_node_t *hash_list_get_node(hash_list_t *self, size_t index);

    bool hash_list_set_count(hash_list_t *self, size_t index, size_t count);
    bool hash_list_set_node(hash_list_t *self, size_t index, hash_node_t *node);

    bool hash_list_insert_unique( hash_list_t * phash, hash_node_t * pnode );

//--------------------------------------------------------------------------------------------

/// An iterator element for traversing the hash_list_t
    struct hash_list_iterator_t
    {
        int           hash;
        hash_node_t * pnode;
    };

    hash_list_iterator_t * hash_list_iterator_ctor( hash_list_iterator_t * it );
    void                 * hash_list_iterator_ptr( hash_list_iterator_t * it );
    bool                 hash_list_iterator_set_begin( hash_list_iterator_t * it, hash_list_t * hlst );
    bool                 hash_list_iterator_done( hash_list_iterator_t * it, hash_list_t * hlst );
    bool                 hash_list_iterator_next( hash_list_iterator_t * it, hash_list_t * hlst );