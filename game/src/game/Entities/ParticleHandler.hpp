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

/// @file  game/Entities/ParticleHandler.hpp
/// @brief Handler of particle entities.

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/Entities/_Include.hpp` instead)
#endif

#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"
#include "game/LockableList.hpp"
#include "game/Entities/Particle.hpp"

//--------------------------------------------------------------------------------------------
// looping macros
//--------------------------------------------------------------------------------------------

// Macros automate looping through the PrtList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so tha the length of the list will not change during the loop.

#define PRT_BEGIN_LOOP_ACTIVE(IT, PRT_BDL) \
    { \
        int IT##_internal; \
        int prt_loop_start_depth = ParticleHandler::get().getLockCount(); \
        ParticleHandler::get().lock(); \
        for(IT##_internal=0;IT##_internal<ParticleHandler::get().getUsedCount();IT##_internal++) \
        { \
            PRT_REF IT; \
            prt_bundle_t PRT_BDL; \
            IT = (PRT_REF)ParticleHandler::get().used_ref[IT##_internal]; \
            if(!ACTIVE_PRT(IT)) continue; \
            PRT_BDL.set(ParticleHandler::get().get_ptr( IT ));

#define PRT_BEGIN_LOOP_DISPLAY(IT, PRT_BDL) \
    { \
        int IT##_internal; \
        int prt_loop_start_depth = ParticleHandler::get().getLockCount(); \
        ParticleHandler::get().lock(); \
        for(IT##_internal=0;IT##_internal<ParticleHandler::get().getUsedCount();IT##_internal++) \
        { \
            PRT_REF IT; \
            prt_bundle_t PRT_BDL; \
            IT = (PRT_REF)ParticleHandler::get().used_ref[IT##_internal]; \
            if(!DISPLAY_PRT(IT)) continue; \
            PRT_BDL.set(ParticleHandler::get().get_ptr(IT));

#define PRT_END_LOOP() \
        } \
        ParticleHandler::get().unlock(); \
        EGOBOO_ASSERT(prt_loop_start_depth == ParticleHandler::get().getLockCount()); \
        ParticleHandler::get().maybeRunDeferred(); \
    }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

struct ParticleHandler : public _LockableList < prt_t, PRT_REF, INVALID_PRT_REF, PARTICLES_MAX, BSP_LEAF_PRT>
{
    ParticleHandler() :
        _LockableList(),
        _displayLimit(512)
    {
    }

    void update_used();

    /**
     * @brief
     *  Get an unused particle.
     *   If all particles are in use and @a force is @a true, get the first unimportant one.
     * @return
     *  the particle index on success, INVALID_PRT_REF on failure
     */
    PRT_REF allocate(const bool force);

    /**
     * @brief
     *  This resets all particle data and reads in global particles (e.g. money).
     */
    void reset_all();

public:

    static ParticleHandler& get();

protected:

    /**
     * @brief
     *  An display limit smaller than @a PARTICLES_MAX is an upper-bound for the number of particles rendered.
     */
    size_t _displayLimit;

public:
    /**
     * @brief
     *  Get the display limit for particles.
     * @return
     *  the display limit for particles
     */
    size_t getDisplayLimit() const
    {
        return _displayLimit;
    }
    /**
     * @brief
     *  Set the display limit for particles.
     * @param displayLimit
     *  the display limit for particles
     */
    void setDisplayLimit(size_t displayLimit)
    {
        displayLimit = Ego::Math::constrain<size_t>(displayLimit, 256, PARTICLES_MAX);
        if (_displayLimit != displayLimit)
        {
            _displayLimit = displayLimit;
        }
    }

    /**
     * @brief
     *  Spawn a particle.
     * @param position
     *  the position of the particle
     * @param facing
     *  the facing of the particle
     * @return
     *  the index of the particle on success, INVALID_PRT_REF on failure
     */
    PRT_REF spawn_one_particle(const fvec3_t& position, FACING_T facing, const PRO_REF iprofile, int pip_index,
                               const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                               const CHR_REF chr_origin, const PRT_REF prt_origin, int multispawn, const CHR_REF oldtarget);

    /**
     * @brief
     *  Spawn a particle.
     * @remark
     *  This function is slightly different than spawn_one_particle because it takes a PIP_REF rather than a pip_index
     * @return the PRT_REF of the spawned particle or INVALID_PRT_REF on failure
     */
    PRT_REF spawnOneParticle(const fvec3_t& pos, FACING_T facing, const PRO_REF iprofile, const PIP_REF ipip,
                             const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                             const CHR_REF chr_origin, const PRT_REF prt_origin = INVALID_PRT_REF,
                             const int multispawn = 0, const CHR_REF oldtarget = INVALID_CHR_REF);

    PRT_REF spawn_one_particle_global(const fvec3_t& pos, FACING_T facing, int pip_index, int multispawn);
};

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------

bool VALID_PRT_RANGE(const PRT_REF ref);
bool DEFINED_PRT(const PRT_REF ref);
bool ALLOCATED_PRT(const PRT_REF ref);
bool ACTIVE_PRT(const PRT_REF ref);
bool WAITING_PRT(const PRT_REF ref);
bool TERMINATED_PRT(const PRT_REF ref);
PRT_REF GET_REF_PPRT(const prt_t *ptr);
bool DEFINED_PPRT(const prt_t *ptr);
bool ALLOCATED_PPRT(const prt_t *ptr);
bool ACTIVE_PPRT(const prt_t *ptr);
bool WAITING_PPRT(const prt_t *ptr);
bool TERMINATED_PPRT(const prt_t *ptr);
bool INGAME_PRT_BASE(const PRT_REF ref);
bool INGAME_PPRT_BASE(const prt_t *ptr);
bool INGAME_PRT(const PRT_REF ref);
bool INGAME_PPRT(const prt_t *ptr);
bool DISPLAY_PRT(const PRT_REF IPRT);
bool DISPLAY_PPRT(const prt_t *ptr);
