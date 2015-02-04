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

/// @file  game/bsp.c
/// @brief Global mesh, character and particle BSPs.
#include "game/bsp.h"
#include "game/char.h"
#include "game/mesh.h"
#include "game/particle.h"
#include "game/game.h"
#include "game/char.h" /** @todo Remove. */
#include "game/particle.h" /** @todo Remove. */
#include "game/PrtList.h" /** @todo Remove. */
#include "game/module/ObjectHandler.hpp" /** @todo Remove. */

//--------------------------------------------------------------------------------------------

static bool _mesh_BSP_system_initialized = false;

/**
 * @brief
 *	Global BSP for the mesh.
 */
static mesh_BSP_t *mesh_BSP_root = NULL;

mesh_BSP_t *getMeshBSP()
{
	EGOBOO_ASSERT(true == _mesh_BSP_system_initialized && NULL != mesh_BSP_root);
	return mesh_BSP_root;
}

bool mesh_BSP_system_started()
{
	return _mesh_BSP_system_initialized;
}

bool mesh_BSP_system_begin(ego_mesh_t *mesh)
{
	EGOBOO_ASSERT(NULL != mesh);

	// If the system is already started, do a reboot.
	if (_mesh_BSP_system_initialized)
	{
		mesh_BSP_system_end();
	}

	// Start the system using the given mesh.
	mesh_BSP_root = mesh_BSP_new(mesh);
	if (!mesh_BSP_root)
	{
		return false;
	}
	// Let the code know that everything is initialized.
	_mesh_BSP_system_initialized = true;
	return true;
}

void mesh_BSP_system_end()
{
	if (_mesh_BSP_system_initialized)
	{
		mesh_BSP_delete(mesh_BSP_root);
		mesh_BSP_root = NULL;
	}
	_mesh_BSP_system_initialized = false;
}

//--------------------------------------------------------------------------------------------

static bool _obj_BSP_system_initialized = false;

/**
 * @brief
 *	Global BSP for the characters.
 */
static obj_BSP_t *chr_BSP_root = NULL;

/**
 * @brief
 *	Global BSP for the particles.
 */
static obj_BSP_t *prt_BSP_root = NULL;

obj_BSP_t *getChrBSP()
{
	EGOBOO_ASSERT(true == _obj_BSP_system_initialized && NULL != chr_BSP_root);
	return chr_BSP_root;
}

obj_BSP_t *getPrtBSP()
{
	EGOBOO_ASSERT(true == _obj_BSP_system_initialized && NULL != prt_BSP_root);
	return prt_BSP_root;
}

bool obj_BSP_system_begin(mesh_BSP_t *mesh_bsp)
{
	if (_obj_BSP_system_initialized)
	{
		obj_BSP_system_end();
	}

	// use 2D BSPs for the moment
	chr_BSP_root = obj_BSP_new(2,mesh_bsp);
	if (!chr_BSP_root)
	{
		return false;
	}
	prt_BSP_root = obj_BSP_new(2, mesh_bsp);
	if (!prt_BSP_root)
	{
		obj_BSP_delete(chr_BSP_root);
		chr_BSP_root = NULL;
		return false;
	}
	// Let the code know that everything is initialized.
	_obj_BSP_system_initialized = true;
	return true;
}

void obj_BSP_system_end()
{
	/// @author BB
	/// @details initialize the obj_BSP list and load up some intialization files
	///     necessary for the the obj_BSP loading code to work

	if (_obj_BSP_system_initialized)
	{
		// delete the object BSP data
		obj_BSP_delete(chr_BSP_root);
		chr_BSP_root = NULL;
		obj_BSP_delete(prt_BSP_root);
		prt_BSP_root = NULL;

		_obj_BSP_system_initialized = false;
	}
}

bool obj_BSP_system_started()
{
	return _obj_BSP_system_initialized;
}

//--------------------------------------------------------------------------------------------
bool prt_BSP_insert(prt_bundle_t * pbdl_prt)
{
	/// @author BB
	/// @details insert a particle's BSP_leaf_t into the BSP_tree_t

	bool       retval;
	BSP_leaf_t * pleaf;
	BSP_tree_t * ptree;

	prt_t *loc_pprt;

	oct_bb_t tmp_oct;

	if (NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr) return false;
	loc_pprt = pbdl_prt->prt_ptr;
	ptree = &(prt_BSP_root->tree);

	// is the particle in-game?
	if (!_INGAME_PPRT_BASE(loc_pprt) || loc_pprt->is_hidden || loc_pprt->is_ghost) return false;

	// heal the leaf if necessary
	pleaf = POBJ_GET_PLEAF(loc_pprt);
	if (loc_pprt != (prt_t *)(pleaf->data))
	{
		// some kind of error. re-initialize the data.
		pleaf->data = loc_pprt;
		pleaf->index = _GET_INDEX_PPRT(loc_pprt);
		pleaf->data_type = BSP_LEAF_PRT;
	};

	// use the object velocity to figure out where the volume that the object will occupy during this
	// update
	phys_expand_prt_bb(loc_pprt, 0.0f, 1.0f, &tmp_oct);

	// convert the bounding box
	bv_from_oct_bb(&(pleaf->bbox), &tmp_oct);

	retval = BSP_tree_t::insert_leaf(ptree, pleaf);
	if (retval)
	{
		prt_BSP_root->count++;
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool chr_BSP_clear()
{
	// unlink all the BSP nodes
	BSP_tree_t::clear_rec(&(chr_BSP_root->tree));
	chr_BSP_root->count = 0;

	// unlink all used character nodes
	for(const std::shared_ptr<GameObject> &object : _gameObjects.iterator())
	{
		BSP_leaf_t::remove_link(&object->bsp_leaf);
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool prt_BSP_clear()
{
	// unlink all the BSP nodes
	BSP_tree_t::clear_rec(&(prt_BSP_root->tree));
	prt_BSP_root->count = 0;

	// unlink all used particle nodes
	for (PRT_REF iprt = 0; iprt < maxparticles; iprt++)
	{
		BSP_leaf_t::remove_link(POBJ_GET_PLEAF(PrtList.lst + iprt));
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool chr_BSP_insert(GameObject * pchr)
{
	/// @author BB
	/// @details insert a character's BSP_leaf_t into the BSP_tree_t

	bool       retval;
	BSP_leaf_t * pleaf;
	BSP_tree_t * ptree;

	if (!ACTIVE_PCHR(pchr)) return false;

	ptree = &(chr_BSP_root->tree);

	// no interactions with hidden objects
	if (pchr->is_hidden) return false;

	// heal the leaf if it needs it
	pleaf = &pchr->bsp_leaf;
	if (pchr != (GameObject *)(pleaf->data))
	{
		// some kind of error. re-initialize the data.
		pleaf->data = pchr;
		pleaf->index = GET_INDEX_PCHR(pchr);
		pleaf->data_type = BSP_LEAF_CHR;
	};

	// do the insert
	retval = false;
	if (!oct_bb_empty(&(pchr->chr_max_cv)))
	{
		oct_bb_t tmp_oct;

		// use the object velocity to figure out where the volume that the object will occupy during this
		// update
		phys_expand_chr_bb(pchr, 0.0f, 1.0f, &tmp_oct);

		// convert the bounding box
		bv_from_oct_bb(&(pleaf->bbox), &tmp_oct);

		// insert the leaf
		retval = BSP_tree_t::insert_leaf(ptree, pleaf);
	}

	if (retval)
	{
		chr_BSP_root->count++;
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool chr_BSP_fill()
{
	// insert the characters
	chr_BSP_root->count = 0;
	for(const std::shared_ptr<GameObject> &pchr : _gameObjects.iterator())
	{
		// reset a couple of things here
		pchr->holdingweight = 0;
		pchr->onwhichplatform_ref = INVALID_CHR_REF;
		pchr->targetplatform_ref = INVALID_CHR_REF;
		pchr->targetplatform_level = -1e32;

		// try to insert the character
		chr_BSP_insert(pchr.get());
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool prt_BSP_fill()
{
	// insert the particles
	prt_BSP_root->count = 0;
	PRT_BEGIN_LOOP_ACTIVE(iprt, prt_bdl)
	{
		// reset a couple of things here
		prt_bdl.prt_ptr->onwhichplatform_ref = INVALID_CHR_REF;
		prt_bdl.prt_ptr->targetplatform_ref = INVALID_CHR_REF;
		prt_bdl.prt_ptr->targetplatform_level = -1e32;

		// try to insert the particle
		prt_BSP_insert(&prt_bdl);
	}
	PRT_END_LOOP()

	return true;
}
