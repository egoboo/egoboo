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

/// @file  game/Entities/Particle.hpp
/// @brief Old C code for Particle entities that isnt ported yet.

#pragma once

#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"
#include "game/graphic_prt.h"
#include "game/physics.h"
#include "egolib/_math.h"
#include "egolib/bbox.h"
#include "game/Entities/Common.hpp"
#include "egolib/Graphics/Animation2D.hpp"
#include "game/char.h"

// Forward declarations.
struct mesh_wall_data_t;
struct ParticleHandler;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Particles
#define MAXPARTICLEIMAGE                256         ///< Number of particle images ( frames )
#define SPAWNNOCHARACTER                255         ///< For particles that spawn characters...

/**
 * @brief
 *  Convenient access to a prt ref and prt as well as pip ref and pip.
 */
struct prt_bundle_t
{
    PRT_REF _prt_ref;
    Ego::Particle *_prt_ptr;

    PIP_REF _pip_ref;
    std::shared_ptr<pip_t> _pip_ptr;

	prt_bundle_t();
	prt_bundle_t(Ego::Particle *prt);

	prt_bundle_t *do_bump_damage();
    prt_bundle_t *update();
    /**
     * @brief
     *  Spawn new particles if continually spawning
     * @return
     *  the number of new particles spawned
     */
    int do_contspawn();
	/// @brief
	/// The master method to compute a particle's motion.
    bool move_one_particle();
private:
	/// @brief
	///	A helper method to compute the next valid position of this particle.
	/// Collisions with the mesh are included in this step.
    prt_bundle_t *move_one_particle_integrate_motion();
	/// @brief
	///	A helper method to compute the next valid position of this particle.
	/// Collisions with the mesh are included in this step.
    prt_bundle_t *move_one_particle_integrate_motion_attached();
	/// @brief
	/// A helper method to compute gravitational acceleration and buoyancy of this particle.
	prt_bundle_t *updateParticleSimpleGravity();
    //prt_bundle_t *move_one_particle_do_z_motion();
    prt_bundle_t *move_one_particle_do_homing();
	/// @brief
	/// Helper method to compute the friction of this particle with the floor.
    prt_bundle_t *move_one_particle_do_floor_friction();
	/// @brief
	///	Helper method to compute the friction of this particle with the water.
    //prt_bundle_t *move_one_particle_do_fluid_friction();
public:
	/// @brief
	/// Helper method to get all of the information about the particle's environment
	/// (like friction, etc.) that will be necessary for the other move_one_particle_*()
	/// functions to work
    prt_bundle_t *move_one_particle_get_environment();
private:
    /**
     * @brief
     *  Update the animation of this particle.
     * @return
     *  a pointer to this particle bundle if
     *  - the bundle holds a particle and
     *  - this particle was not ended by this function,
     *  a null pointer otherwise
     */
    prt_bundle_t *update_animation();
    /**
     * @brief
     *  Handle the particle ?.
     * @return
     *  a pointer to this particle bundle if
     *  - the bundle holds a particle and
     *  - this particle was not ended by this function,
     *  a null pointer otherwise
     * @remark
     *  This can not end the particle at least for now.
     * @todo
     *  Figure out what this crap is doing.
     */
    prt_bundle_t *update_dynalight();
    /**
     * @brief
     *  Update the lifetime timers of this particle.
     * @return
     *  a pointer to this particle bundle if the bundle holds a particle, a null pointer otherwise
     */
    prt_bundle_t *update_timers();
    /// @details update everything about a particle that does not depend on collisions
    ///               or interactions with characters
    prt_bundle_t *update_ingame();

    /**
     * @brief
     *  Handle the particle interaction with water
     * @return
     *  a pointer to this particle bundle if
     *  - the bundle holds a particle and
     *  - this particle was not ended by this function,
     *  a null pointer otherwise
     */
    prt_bundle_t *update_do_water();
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// function prototypes
//--------------------------------------------------------------------------------------------
void move_all_particles();

CHR_REF prt_get_iowner(const PRT_REF iprt, int depth);
