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

/// @file  game/Entities/Particle.h
/// @brief Old C code for Particle entities that isnt ported yet.

#pragma once

#include "game/egoboo_typedef.h"
#include "game/graphic_prt.h"
#include "game/physics.h"
#include "egolib/_math.h"
#include "egolib/bbox.h"
#include "game/Entities/Common.hpp"
#include "egolib/Graphics/Animation2D.hpp"
#include "game/char.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Particles
#define SPAWNNOCHARACTER                255         ///< For particles that spawn characters...

/**
 * @brief
 *  Convenient access to a prt ref and prt as well as pip ref and pip.
 */
struct prt_bundle_t
{
    ParticleRef _prt_ref;
    Ego::Particle *_prt_ptr;

    PIP_REF _pip_ref;
    std::shared_ptr<ParticleProfile> _pip_ptr;

    prt_bundle_t();
    prt_bundle_t(Ego::Particle *prt);

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
    /// A helper method to compute the next valid position of this particle.
    /// Collisions with the mesh are included in this step.
    prt_bundle_t *move_one_particle_integrate_motion();
    /// @brief
    /// A helper method to compute the next valid position of this particle.
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
    /// Helper method to compute the friction of this particle with the water.
    //prt_bundle_t *move_one_particle_do_fluid_friction();

    void move_one_particle_update_gravity_pull();
    
public:
    /// @brief
    /// Helper method to get all of the information about the particle's environment
    /// (like friction, etc.) that will be necessary for the other move_one_particle_*()
    /// functions to work
    prt_bundle_t *move_one_particle_get_environment();
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// function prototypes
//--------------------------------------------------------------------------------------------
void move_all_particles();

int  spawn_bump_particles(ObjectRef objectRef, const ParticleRef particle);
ObjectRef prt_get_iowner(const ParticleRef iprt, int depth);
