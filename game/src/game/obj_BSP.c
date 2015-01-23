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

/// @file game/obj_BSP.c
/// @brief Implementation of functions for the object BSP
/// @details

#include "game/obj_BSP.h"

#include "game/mesh_BSP.h"

#include "game/char.h" /** @todo Remove. */
#include "game/particle.h" /** @todo Remove. */
#include "egolib/bsp.h"

#include "game/profiles/ProfileSystem.hpp"

#include "game/ChrList.h"
#include "game/PrtList.h"

obj_BSP_t *obj_BSP_t::ctor(size_t bsp_dim, const mesh_BSP_t *mesh_bsp)
{
    if (bsp_dim < 2)
    {
        log_error("obj_BSP_t::ctor() - cannot construct an object BSP with less than 2 dimensions\n");
		return nullptr;
    }
    else if (bsp_dim > 3)
    {
        log_error("obj_BSP_t::ctor() - cannot construct an object BSP with more than than 3 dimensions\n");
		return nullptr;
    }
	if (nullptr == mesh_bsp)
	{
		return nullptr;
	}
	const BSP_tree_t *mesh_tree = &(mesh_bsp->tree);
    size_t mesh_dim = mesh_bsp->tree.dimensions;

	BLANK_STRUCT_PTR(this);

    // Construct the BSP tree.
	if (!tree.ctor(bsp_dim, mesh_tree->max_depth))
	{
		return nullptr;
	}
	// Take the minimum of the requested dimensionality and dimensionality
	// of the mesh BSP tree as the dimensionality of the resulting BSP tree.
	size_t min_dim = std::min(bsp_dim, mesh_dim);

	float bsp_size = 0.0f;
	// Find the maximum extent of the BSP tree from the size of the bounding box of the mesh tree.
    for (size_t cnt = 0; cnt < min_dim; ++cnt)
    {
        float tmp_size = std::abs( mesh_tree->bsp_bbox.maxs.ary[cnt] - mesh_tree->bsp_bbox.mins.ary[cnt] );
        bsp_size = std::max(bsp_size, tmp_size);
    }

	BSP_tree_t *obj_tree = &(tree);

    // Copy the volume from the mesh.
    for (size_t cnt = 0; cnt < min_dim; ++cnt)
    {
        // get the size
        obj_tree->bsp_bbox.mins.ary[cnt] = std::min( mesh_tree->bsp_bbox.mins.ary[cnt], mesh_tree->bbox.aabb.mins[cnt] );
        obj_tree->bsp_bbox.maxs.ary[cnt] = std::max( mesh_tree->bsp_bbox.maxs.ary[cnt], mesh_tree->bbox.aabb.maxs[cnt] );

        // make some extra space
        obj_tree->bsp_bbox.mins.ary[cnt] -= bsp_size * 0.25f;
        obj_tree->bsp_bbox.maxs.ary[cnt] += bsp_size * 0.25f;
    }

    // Calculate a "reasonable size" for all dimensions that are not in the mesh.
    for (size_t cnt = min_dim; cnt < bsp_dim; cnt++ )
    {
        obj_tree->bsp_bbox.mins.ary[cnt] = -bsp_size * 0.5f;
        obj_tree->bsp_bbox.maxs.ary[cnt] =  bsp_size * 0.5f;
    }

    if (bsp_dim > 2)
    {
        // Make some extra special space in the z direction.
        obj_tree->bsp_bbox.mins.ary[kZ] = std::min(-bsp_size, obj_tree->bsp_bbox.mins.ary[kZ]);
        obj_tree->bsp_bbox.maxs.ary[kZ] = std::max(+bsp_size, obj_tree->bsp_bbox.maxs.ary[kZ]);
    }

    // Calculate the mid positions
    for (size_t cnt = 0; cnt < bsp_dim; ++cnt)
    {
		/// @todo Use BSP_aabb_t::getCenter();
        obj_tree->bsp_bbox.mids.ary[cnt] = 0.5f * (obj_tree->bsp_bbox.mins.ary[cnt] + obj_tree->bsp_bbox.maxs.ary[cnt]);
    }

    BSP_aabb_validate(obj_tree->bsp_bbox);

    return this;
}

//--------------------------------------------------------------------------------------------
void obj_BSP_t::dtor()
{
    // Destruct the BSP tree.
	tree.dtor();
}

//--------------------------------------------------------------------------------------------
obj_BSP_t *obj_BSP_new(size_t dim, const mesh_BSP_t *mesh_bsp)
{
	EGOBOO_ASSERT(NULL != mesh_bsp);
	obj_BSP_t *self = (obj_BSP_t *)malloc(sizeof(obj_BSP_t));
	if (!self)
	{
		log_error("%s:%d: unable to allocate %zu Bytes\n",__FILE__,__LINE__,sizeof(obj_BSP_t));
		return NULL;
	}
	if (!self->ctor(dim, mesh_bsp))
	{
		free(self);
		return NULL;
	}
	return self;
}

void obj_BSP_delete(obj_BSP_t *self)
{
	EGOBOO_ASSERT(NULL != self);
	self->dtor();
	free(self);
}


size_t obj_BSP_t::collide_aabb(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *>  *collisions) const
{
    return tree.collide_aabb(aabb, test, collisions);
}


size_t obj_BSP_t::collide_frustum(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *>  *collisions) const
{
    return tree.collide_frustum(frustum, test, collisions);
}

/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool chr_BSP_can_collide(BSP_leaf_t * pchr_leaf)
{
    chr_t * pchr;

    bool can_be_reaffirmed;
    bool can_grab_money;
    bool can_use_platforms;
    bool can_collide;

    bool requires_chr_chr;
    bool requires_chr_prt;

    // make sure we have a character leaf
    if ( NULL == pchr_leaf || NULL == pchr_leaf->data || BSP_LEAF_CHR != pchr_leaf->data_type )
    {
        return false;
    }
    pchr = ( chr_t * )( pchr_leaf->data );

    if ( !ACTIVE_PCHR( pchr ) ) return false;

    // no interactions with hidden objects
    if ( pchr->is_hidden ) return false;

    // no interactions with packed objects
    if ( VALID_CHR_RANGE( pchr->inwhich_inventory ) ) return false;

    // generic flags for character interaction
    can_be_reaffirmed = ( pchr->reaffirm_damagetype < DAMAGE_COUNT );
    can_grab_money    = pchr->cangrabmoney;
    can_use_platforms = pchr->canuseplatforms;
    can_collide       = ( 0 != pchr->bump_stt.size ) && ( INVALID_CHR_REF == pchr->attachedto );

    // conditions for normal chr-chr interaction
    // platform tests are done elsewhere
    requires_chr_chr = can_collide /* || can_use_platforms */;

    // conditions for chr-prt interaction
    requires_chr_prt = can_be_reaffirmed /* || can_grab_money */;

    // even if an object does not interact with other characters,
    // it must still be inserted if it might interact with a particle
    if ( !requires_chr_chr && !requires_chr_prt ) return false;

    if ( oct_bb_empty( &( pchr->chr_max_cv ) ) ) return false;

    return true;
}

/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool prt_BSP_can_collide(BSP_leaf_t * pprt_leaf)
{
    prt_t * pprt;
    pip_t * ppip;

    // Each one of these tests allows one MORE reason to include the particle, not one less.
    // Removed bump particles. We have another loop that can detect these, and there
    // is no reason to fill up the BSP with particles like coins.
    bool       has_enchant;
    bool       does_damage;
    bool       does_status_effect;
    bool       does_special_effect;
    bool       can_push;
    bool       has_bump;

    // make sure we have a character leaf
    if ( NULL == pprt_leaf || NULL == pprt_leaf->data || BSP_LEAF_PRT != pprt_leaf->data_type )
    {
        return false;
    }
    pprt = ( prt_t * )( pprt_leaf->data );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return false;
    ppip = PipStack.get_ptr( pprt->pip_ref );

    // is the particle in-game?
    if ( !_INGAME_PPRT_BASE( pprt ) || pprt->is_hidden || pprt->is_ghost ) return false;

    // Make this optional? Is there any reason to fail if the particle has no profile reference?
    has_enchant = false;
    if ( ppip->spawnenchant )
    {
        has_enchant = LOADED_EVE( _profileSystem.pro_get_ieve(pprt->profile_ref) );
    }

    // any possible damage?
    does_damage         = ( ABS( pprt->damage.base ) + ABS( pprt->damage.rand ) ) > 0;

    // the other possible status effects
    // do not require damage
    does_status_effect  = ( 0 != ppip->grog_time ) || ( 0 != ppip->daze_time ) || ( 0 != ppip->lifedrain ) || ( 0 != ppip->manadrain );

    // these are not implemented yet
    does_special_effect = ppip->cause_pancake || ppip->cause_roll;

    // according to v1.0, only particles that cause damage can push
    can_push            = does_damage && ppip->allowpush;
    
    /// @todo this is a stopgap solution, figure out if this is the correct place or
    ///       we need to fix the loop in fill_interaction_list instead
    has_bump            = ppip->bump_height && ppip->bump_size;

    // particles with no effect
    if ( !can_push && !has_enchant && !does_damage && !does_status_effect && !does_special_effect && !has_bump ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool chr_BSP_is_visible(BSP_leaf_t * pchr_leaf)
{
    // make sure we have a character leaf
    if (NULL == pchr_leaf || NULL == pchr_leaf->data || BSP_LEAF_CHR != pchr_leaf->data_type)
    {
        return false;
    }
	chr_t *pchr = (chr_t *)(pchr_leaf->data);

    if (!ACTIVE_PCHR(pchr)) return false;

    // no interactions with hidden objects
    if (pchr->is_hidden) return false;

    // no interactions with packed objects
    if (VALID_CHR_RANGE(pchr->inwhich_inventory)) return false;

    return true;
}

/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool prt_BSP_is_visible(BSP_leaf_t * pprt_leaf)
{
    // make sure we have a character leaf
    if (NULL == pprt_leaf || NULL == pprt_leaf->data || BSP_LEAF_PRT != pprt_leaf->data_type)
    {
        return false;
    }
	prt_t *pprt = (prt_t *)(pprt_leaf->data);

    // is the particle in-game?
    if (!_INGAME_PPRT_BASE(pprt) || pprt->is_hidden) return false;

    // zero sized particles are not visible
    if (0 == pprt->size)
    {
        return false;
    }
    else if (pprt->inst.valid && pprt->inst.size <= 0.0f)
    {
        return false;
    }

    return true;
}
