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

/// @file egolib/hash.c
/// @brief Implementation of the support functions for hash tables
/// @details

#include "egolib/hash.h"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
hash_node_t *hash_node_insert_after( hash_node_t lst[], hash_node_t * n )
{
    if ( NULL == n ) return lst;
    n->next = NULL;

    if ( NULL == lst ) return n;

    n->next = n->next;
    lst->next = n;

    return lst;
}

hash_node_t *hash_node_insert_before( hash_node_t lst[], hash_node_t * n )
{
    if ( NULL == n ) return lst;
    n->next = NULL;

    if ( NULL == lst ) return n;

    n->next = lst;

    return n;
}

hash_node_t *hash_node_remove_after( hash_node_t lst[] )
{
    hash_node_t * n;

    if ( NULL == lst ) return NULL;

    n = lst->next;
    if ( NULL == n ) return lst;

    lst->next = n->next;
    n->next   = NULL;

    return lst;
}

hash_node_t *hash_node_remove( hash_node_t lst[] )
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
size_t hash_list_count_nodes(hash_list_t *self)
{
	if (NULL == self)
	{
		return 0;
	}
	size_t count = 0;
    for (size_t i = 0; i < self->capacity; i++)
    {
        if (NULL != self->sublist[i])
        {
            count += self->subcount[i];
        }
    }
    return count;
}

hash_node_t *hash_list_get_node(hash_list_t *self, size_t index)
{
	if (NULL == self || NULL == self->sublist)
	{
		return NULL;
	}
    return self->sublist[index];
}

bool hash_list_set_count(hash_list_t *self, size_t index, size_t count)
{
    if ( NULL == self || NULL == self->subcount ) return false;

    if ( index >= self->capacity ) return false;

    self->subcount[index] = count;

    return true;
}

bool hash_list_set_node(hash_list_t *self, size_t index, hash_node_t *node)
{
	if (NULL == self || NULL == self->sublist)
	{
		return false;
	}
	if (index >= self->capacity)
	{
		return false;
	}
    self->sublist[index] = node;
    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

hash_list_iterator_t *hash_list_iterator_t::ctor()
{
	this->hash = 0;
	this->pnode = nullptr;
    return this;
}

bool hash_list_iterator_set_begin( hash_list_iterator_t * it, hash_list_t * hlst )
{
	if (nullptr == it) return false;
	it = it->ctor();
    if ( NULL == it || NULL == hlst ) return false;

    // find the first non-null hash element
    for (size_t i = 0; i < hlst->capacity && NULL == it->pnode; i++ )
    {
        it->hash  = i;
        it->pnode = hlst->sublist[i];
    }

    return NULL != it->pnode;
}

bool hash_list_iterator_done( hash_list_iterator_t * it, hash_list_t * hlst )
{
    if ( NULL == it || NULL == hlst ) return true;

    // the end consition
    if ( it->hash >= hlst->capacity ) return true;

    return false;
}

bool hash_list_iterator_next( hash_list_iterator_t * it, hash_list_t * hlst )
{
    int i, inext;
    hash_node_t * pnext;

    if ( NULL == it || NULL == hlst ) return false;

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
        for ( i = it->hash + 1; i < hlst->capacity && NULL == pnext; i++ )
        {
            inext = i;
            pnext = hlst->sublist[i];
        }
    }

    if ( NULL == pnext )
    {
        // could not find one. set the iterator to the end condition.
        inext = hlst->capacity;
    }

    it->hash  = inext;
    it->pnode = pnext;

    return NULL != it->pnode;
}

void * hash_list_iterator_ptr( hash_list_iterator_t * it )
{
    if ( NULL == it || NULL == it->pnode ) return NULL;

    return it->pnode->data;
}
