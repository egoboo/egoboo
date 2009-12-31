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

//--------------------------------------------------------------------------------------------
struct s_hash_list
{
    int            allocated;
    int         *  subcount;
    hash_node_t ** sublist;
};
typedef struct s_hash_list hash_list_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
hash_node_t * hash_node_create( void * data );
bool_t        hash_node_destroy( hash_node_t ** );
hash_node_t * hash_node_insert_after( hash_node_t lst[], hash_node_t * n );
hash_node_t * hash_node_insert_before( hash_node_t lst[], hash_node_t * n );
hash_node_t * hash_node_remove_after( hash_node_t lst[] );
hash_node_t * hash_node_remove( hash_node_t lst[] );

hash_list_t * hash_list_create( int size );
bool_t        hash_list_destroy( hash_list_t ** );

hash_node_t * hash_node_ctor( hash_node_t * n, void * data );
hash_list_t * hash_list_ctor( hash_list_t * lst, int size );