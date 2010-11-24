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

/// @file collision.h

#include "egoboo_typedef.h"

#include "hash.h"
#include "bbox.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_obj_BSP;
struct s_chr;
struct s_prt;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// element for storing pair-wise "collision" data
/// @note this does not use the "standard" method of inheritance from hash_node_t, where an
/// instance of hash_node_t is embedded inside CoNode_t as CoNode_t::base or something.
/// Instead, a separate lists of free hash_nodes and free CoNodes are kept, and they are
/// associated through the hash_node_t::data pointer when the hash node is added to the
/// hash_list_t
struct s_CoNode
{
    // the "colliding" objects
    CHR_REF chra;
    PRT_REF prta;

    // the "collided with" objects
    CHR_REF chrb;
    PRT_REF prtb;
    Uint32  tileb;

    // some information about the estimated collision
    float    tmin, tmax;
    oct_bb_t cv;
};
typedef struct s_CoNode CoNode_t;

CoNode_t * CoNode_ctor( CoNode_t * );
Uint8      CoNode_generate_hash( CoNode_t * coll );
int        CoNode_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------
// a template-like definition of a dynamically allocated array of CoNode_t elements
DECLARE_DYNAMIC_ARY( CoNode_ary, CoNode_t );

//--------------------------------------------------------------------------------------------
// a template-like definition of a dynamically allocated array of hash_node_t elements
DECLARE_DYNAMIC_ARY( HashNode_ary, hash_node_t );

//--------------------------------------------------------------------------------------------

/// a useful re-typing of the CHashList_t, in case we need to add more variables or functionality later
typedef hash_list_t CHashList_t;

CHashList_t * CHashList_ctor( CHashList_t * pchlst, int size );
CHashList_t * CHashList_dtor( CHashList_t * pchlst );
bool_t        CHashList_insert_unique( CHashList_t * pchlst, CoNode_t * pdata, CoNode_ary_t * cdata, HashNode_ary_t * hnlst );

CHashList_t * CHashList_get_Instance( int size );

//--------------------------------------------------------------------------------------------
extern int CHashList_inserted;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// global functions

bool_t collision_system_begin();
void   collision_system_end();

void bump_all_objects();