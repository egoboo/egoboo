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
		static hash_node_t *ctor(hash_node_t *self, void * data);
		static void dtor(hash_node_t *self);
    };

    hash_node_t *hash_node_create( void * data );
    bool         hash_node_destroy( hash_node_t ** );

    hash_node_t *hash_node_insert_after( hash_node_t lst[], hash_node_t * n );
    hash_node_t *hash_node_insert_before( hash_node_t lst[], hash_node_t * n );
    hash_node_t *hash_node_remove_after( hash_node_t lst[] );
    hash_node_t *hash_node_remove( hash_node_t lst[] );

//--------------------------------------------------------------------------------------------
    struct hash_list_t
    {
		/**
		 * @brief
		 *	The capacity of this hash list i.e. the number of elements in the array pointed to by @a sublist.
		 */
		size_t capacity;
		/**
		 * @brief
		 *	The number of entries in each bucket.
		 */
        int *subcount;
		/**
		 * @brief
		 *	A pointer to an array of @a capacity buckets.
		 */
        hash_node_t **sublist;
		/**
		 * @brief
		 *	Construct this hash list.
		 * @param initialCapacity
		 *	the initial capacity of the hash list
		 */
		hash_list_t(size_t initialCapacity)
		{
			subcount = EGOBOO_NEW_ARY(int, initialCapacity);
			if (!subcount)
			{
				throw std::bad_alloc();
			}
			sublist = EGOBOO_NEW_ARY(hash_node_t *, initialCapacity);
			if (!sublist)
			{
				EGOBOO_DELETE(subcount);
				subcount = nullptr;
				throw std::bad_alloc();
			}
			else
			{
				for (size_t i = 0, n = initialCapacity; i < n; ++i)
				{
					subcount[i] = 0;
					sublist[i] = nullptr;
				}
			}
			capacity = initialCapacity;
		}
		/**
		 * @brief
		 *	Destruct this hash list.
		 */
		virtual ~hash_list_t()
		{
			EGOBOO_DELETE_ARY(subcount);
			subcount = nullptr;
			EGOBOO_DELETE_ARY(sublist);
			sublist = nullptr;
			capacity = 0;
		}
#if 0
		/**
		 * @brief
		 *	Deallocate the data of a hash list.
		 * @param self
		 *	a pointer the hash list
		 * @return
		 *	@a true on success, @a false on failure
		 * @post
		 *	<tt>self->capacity = 0</tt>, <tt>self->subcount = nullptr</tt>, <tt>self->sublist = nullptr</tt>
		 */
		static bool dealloc(hash_list_t *self)
		{
			if (nullptr == self)
			{
				return false;
			}
			if (0 == self->capacity)
			{
				return true;
			}
			EGOBOO_DELETE_ARY(self->subcount);
			self->subcount = nullptr;
			EGOBOO_DELETE_ARY(self->sublist);
			self->sublist = nullptr;
			self->capacity = 0;

			return true;
		}
		static bool alloc(hash_list_t *self, size_t capacity)
		{
			if (nullptr == self)
			{
				return false;
			}
			// Ensure subcount and sublist are null pointers and capacity is zero.
			dealloc(self);

			self->subcount = EGOBOO_NEW_ARY(int, capacity);
			if (!self->subcount)
			{
				return false;
			}
			self->sublist = EGOBOO_NEW_ARY(hash_node_t *, capacity);
			if (!self->sublist)
			{
				EGOBOO_DELETE(self->subcount);
				self->subcount = nullptr;
				return false;
			}
			else
			{
				for (size_t i = 0, n = capacity; i < n; ++i)
				{
					self->sublist[i] = nullptr;
				}
			}
			self->capacity = capacity;
			return true;
		}
#endif
		/**
		 * @brief
		 *	Remove all entries from this hash list.
		 */
		void clear()
		{
			for (size_t i = 0, n = capacity; i < n; ++i)
			{
				subcount[i] = 0;
				sublist[i] = nullptr;
			}

		}
		/**
		 * @brief
		 *	Get the size of this hash list.
		 * @return
		 *	the size of this hash list
		 */
		size_t getSize() const
		{
			size_t size = 0;
			for (size_t i = 0, n = capacity; i < n; ++i)
			{
				size += subcount[i];
			}
			return size;
		}
		/**
		 * @brief
		 *	Get the capacity of this hash list.
		 * @return
		 *	the capacity of this hash list
		 */
		size_t getCapacity() const
		{
			return capacity;
		}
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
#if 0
    int hash_list_get_allocd(hash_list_t *self);
#endif
    size_t hash_list_get_count(hash_list_t *self, size_t index);
    hash_node_t *hash_list_get_node(hash_list_t *self, size_t index);

    bool hash_list_set_count(hash_list_t *self, size_t index, size_t count);
    bool hash_list_set_node(hash_list_t *self, size_t index, hash_node_t *node);

    bool hash_list_insert_unique( hash_list_t * phash, hash_node_t * pnode );

//--------------------------------------------------------------------------------------------

/// An iterator element for traversing the hash_list_t
    struct hash_list_iterator_t
    {
        int hash;
        hash_node_t * pnode;
		hash_list_iterator_t *ctor();
    };

    void *hash_list_iterator_ptr(hash_list_iterator_t * it);
    bool hash_list_iterator_set_begin( hash_list_iterator_t * it, hash_list_t * hlst );
    bool hash_list_iterator_done( hash_list_iterator_t * it, hash_list_t * hlst );
    bool hash_list_iterator_next( hash_list_iterator_t * it, hash_list_t * hlst );