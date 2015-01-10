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

/// @file egolib/bsp_leaf.h
/// @brief

#pragma once

#include "egolib/bv.h"
#include "egolib/geometry.h"
#if 0
#include "egolib/typedef.h"
#endif

struct BSP_leaf_t;
struct BSP_leaf_list_t;
struct egolib_frustum_t;

typedef bool (BSP_leaf_test_t)(BSP_leaf_t *);

enum bsp_leaf_type_e
{
	BSP_LEAF_TYPE_NONE = -1,
	BSP_LEAF_TYPE_CHR,
	BSP_LEAF_TYPE_ENC,
	BSP_LEAF_TYPE_PRT,
	BSP_LEAF_TYPE_TILE
};

struct BSP_leaf_t
{
	bool inserted;
	BSP_leaf_t *next;
	bsp_leaf_type_e data_type;
	void *data;
	size_t index;
	bv_t bbox;
};

DECLARE_DYNAMIC_ARY(BSP_leaf_ary, BSP_leaf_t)
DECLARE_DYNAMIC_ARY(BSP_leaf_pary, BSP_leaf_t *)

BSP_leaf_t *BSP_leaf_ctor(BSP_leaf_t *self, void *data, bsp_leaf_type_e type, int index);
BSP_leaf_t *BSP_leaf_dtor(BSP_leaf_t *self);
bool BSP_leaf_clear(BSP_leaf_t *self);
bool BSP_leaf_remove_link(BSP_leaf_t *self);
bool BSP_leaf_copy(BSP_leaf_t *self, const BSP_leaf_t *other);

//--------------------------------------------------------------------------------------------

struct BSP_leaf_list_t
{
	size_t count;
	BSP_leaf_t *lst;
	bv_t bbox;
};

BSP_leaf_list_t *BSP_leaf_list_ctor(BSP_leaf_list_t *self);
BSP_leaf_list_t *BSP_leaf_list_dtor(BSP_leaf_list_t *self);
BSP_leaf_list_t *BSP_leaf_list_clear(BSP_leaf_list_t *self);

bool BSP_leaf_list_alloc(BSP_leaf_list_t *self);
bool BSP_leaf_list_dealloc(BSP_leaf_list_t *self);
bool BSP_leaf_list_reset(BSP_leaf_list_t *self);

bool BSP_leaf_list_collide_aabb(const BSP_leaf_list_t *self, const aabb_t *aabb, BSP_leaf_test_t *test, BSP_leaf_pary_t *colst);
bool BSP_leaf_list_collide_frustum(const BSP_leaf_list_t *self, const egolib_frustum_t *frustum, BSP_leaf_test_t *test, BSP_leaf_pary_t *colst);



/**
* @brief
*	Insert a leaf in a leaf list, making sure there are no duplicates.
* @param self
*	the leaf list
* @param leaf
*	the leaf
* @warning
*	If the leaf is already an element of another leaf list.
* @remark
*	Duplicates would cause loops in the list making it impossible to traverse it properly.
* @author
*	BB
*/
bool BSP_leaf_list_push_front(BSP_leaf_list_t *self, BSP_leaf_t *leaf);

/**
* @brief
*	Remove and return the frontmost leaf from a leaf list.
* @param self
*	the leaf list
* @return
*	the fronstmost leaf in the leaf list if the leaf list is not empty, @a NULL otherwise
*/
BSP_leaf_t *BSP_leaf_list_pop_front(BSP_leaf_list_t *self);

#define EMPTY_BSP_LEAF_LIST(LL) ( (NULL == (LL)) || (NULL == (LL)->lst) || (0 == (LL)->count) )

bool BSP_leaf_valid(BSP_leaf_t *self);