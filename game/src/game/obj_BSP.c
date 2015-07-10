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
#include "egolib/bsp.h"
#include "game/Entities/_Include.hpp"

obj_BSP_t::Parameters::Parameters(size_t dim, const mesh_BSP_t *meshBSP)
{
	if (dim < ALLOWED_DIM_MIN)
	{
		log_error("%s:%d: specified dimensionality %" PRIuZ " is smaller than allowed minimum dimensionality %" PRIuZ "\n", \
			      __FILE__, __LINE__, dim, obj_BSP_t::Parameters::ALLOWED_DIM_MIN);
		throw std::domain_error("dimensionality out of range");
	}
	else if (dim > ALLOWED_DIM_MAX)
	{
		log_error("%s:%d: specified dimensionality %" PRIuZ " is greater than allowed maximum dimensionality %" PRIuZ "\n", \
			      __FILE__, __LINE__, dim, BSP_tree_t::Parameters::ALLOWED_DIM_MAX);
		throw std::domain_error("dimensionality out of range");
	}
	if (!meshBSP)
	{
		log_error("%s:%d: no mesh BSP tree provided\n", __FILE__, __LINE__);
		throw std::invalid_argument("no mesh BSP tree provided");
	}
	// Take the minimum of the requested dimensionality and the dimensionality
	// of the mesh BSP tree as the dimensionality of the object BSP tree.
	_dim = dim;
	_maxDepth = meshBSP->getParameters().getMaxDepth();
	_meshBSP = meshBSP;
}

obj_BSP_t::obj_BSP_t(const Parameters& parameters) :
	BSP_tree_t(BSP_tree_t::Parameters(parameters._dim, parameters._maxDepth)),
	count(0)
{
	size_t minDim = std::min(parameters._dim, parameters._meshBSP->getParameters().getDim());
	// Use the information for the dimensions the object and the mesh BSP share.
	for (size_t i = 0; i < minDim; ++i)
	{
		bsp_bbox.min()[i] = parameters._meshBSP->getBoundingBox().min()[i];
		bsp_bbox.mid()[i] = parameters._meshBSP->getBoundingBox().mid()[i];
		bsp_bbox.max()[i] = parameters._meshBSP->getBoundingBox().max()[i];
	}
	if (parameters._dim > minDim)
	{
		// For dimensions in the object BSP but not in the mesh BSP,
		// use the minima and the maxima of existing dimensions ...
		float min = bsp_bbox.min()[0], max = bsp_bbox.max()[0];
		for (size_t i = 1; i < minDim; ++i)
		{
			min = std::min(bsp_bbox.min()[i], min);
			max = std::max(bsp_bbox.max()[i], max);
		}
		for (size_t i = minDim; i < parameters._dim; ++i)
		{
			bsp_bbox.min()[i] = min;
			bsp_bbox.max()[i] = max;
			bsp_bbox.mid()[i] = 0.5f * (min + max);
		}
	}
}

//--------------------------------------------------------------------------------------------
obj_BSP_t::~obj_BSP_t()
{
	// Set the count to zero.
	count = 0;
}

/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool chr_BSP_can_collide(BSP_leaf_t * pchr_leaf)
{
    Object * pchr;

    bool can_be_reaffirmed;
    bool can_grab_money;
    bool can_use_platforms;
    bool can_collide;

    bool requires_chr_chr;
    bool requires_chr_prt;

    // make sure we have a character leaf
    if ( NULL == pchr_leaf || NULL == pchr_leaf->_data || BSP_LEAF_CHR != pchr_leaf->_type )
    {
        return false;
    }
    pchr = ( Object * )( pchr_leaf->_data );

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

    if (oct_bb_empty(pchr->chr_max_cv)) return false;

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
    if (!pprt_leaf || !pprt_leaf->_data || BSP_LEAF_PRT != pprt_leaf->_type )
    {
        return false;
    }

    Ego::Particle* pprt = static_cast<Ego::Particle *>(pprt_leaf->_data);
    const std::shared_ptr<pip_t> &ppip = pprt->getProfile();

    // is the particle in-game?
    if ( pprt->isTerminated() || pprt->isHidden() ) return false;

    // Make this optional? Is there any reason to fail if the particle has no profile reference?
    has_enchant = false;
    if ( ppip->spawnenchant )
    {
        has_enchant = LOADED_EVE(ProfileSystem::get().pro_get_ieve(pprt->getSpawnerProfile()));
    }

    // any possible damage?
    does_damage         = (std::abs(pprt->damage.base) + std::abs(pprt->damage.rand)) > 0;

    // the other possible status effects
    // do not require damage
    does_status_effect  = ( 0 != ppip->grogTime ) || ( 0 != ppip->dazeTime ) || ( 0 != ppip->lifeDrain ) || ( 0 != ppip->manaDrain );

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
    if (NULL == pchr_leaf || NULL == pchr_leaf->_data || BSP_LEAF_CHR != pchr_leaf->_type)
    {
        return false;
    }
	Object *pchr = (Object *)(pchr_leaf->_data);

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
    if (NULL == pprt_leaf || NULL == pprt_leaf->_data || BSP_LEAF_PRT != pprt_leaf->_type)
    {
        return false;
    }
	Ego::Particle *pprt = (Ego::Particle *)(pprt_leaf->_data);

    // is the particle in-game?
    if (pprt == nullptr || pprt->isTerminated() || pprt->isHidden()) return false;

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
