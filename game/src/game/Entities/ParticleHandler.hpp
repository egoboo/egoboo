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
#include "game/Entities/OldParticle.hpp"

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
            IT = (PRT_REF)ParticleHandler::get().used_ref[IT##_internal]; \
            if(!ACTIVE_PRT(IT)) continue; \
			prt_bundle_t PRT_BDL(ParticleHandler::get().get_ptr( IT ));

#define PRT_BEGIN_LOOP_DISPLAY(IT, PRT_BDL) \
    { \
        int IT##_internal; \
        int prt_loop_start_depth = ParticleHandler::get().getLockCount(); \
        ParticleHandler::get().lock(); \
        for(IT##_internal=0;IT##_internal<ParticleHandler::get().getUsedCount();IT##_internal++) \
        { \
            PRT_REF IT; \
            IT = (PRT_REF)ParticleHandler::get().used_ref[IT##_internal]; \
            if(!DISPLAY_PRT(IT)) continue; \
            prt_bundle_t PRT_BDL(ParticleHandler::get().get_ptr(IT));

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
        _maxParticles(0)
    {
        setDisplayLimit(512);
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

public:

    static ParticleHandler& get();

public:
    /**
    * @brief
    *   Updates all particles and free particles that have been marked as terminated
    **/
    void updateAllParticles();

    /**
     * @brief
     *  Get the display limit for particles.
     * @return
     *  the display limit for particles
     */
    size_t getDisplayLimit() const;

    /**
     * @brief
     *  Set the display limit for particles.
     * @param displayLimit
     *  the display limit for particles
     */
    void setDisplayLimit(size_t displayLimit);

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
    PRT_REF spawn_one_particle(const fvec3_t& position, FACING_T facing, const PRO_REF iprofile, const LocalParticleProfileRef& pip_index,
                               const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                               const CHR_REF chr_origin, const PRT_REF prt_origin, int multispawn, const CHR_REF oldtarget);
    /**
     * @brief Return a pointer object for the specifiec PRT_REF.
     * @return a pointer object for the specified PRT_REF.
     *         Return nullptr object if PRT_REF was not found.
     */
    const std::shared_ptr<Ego::Particle>& operator[] (const PRT_REF index);

    /**
     * @brief
     *  Spawn a particle and add it into the game
     * @param spawnPos
     *  the position of the particle
     * @param spawnFacing
     *  the facing (direction) of the particle
     * @param spawnProfile
     *  the Object that spawned this particle (INVALID_CHR_REF for a global particle), e.g a Weapon
     * @param particleProfile
     *  the profile of the Particle
     * @param spawnAttach
     *  if this value is != INVALID_CHR_REF, then the particle will spawn attached to that Object
     * @param vrt_offset
     *  determines which vertex this particle will attach to when spawnAttach is used
     * @param spawnTeam
     *  on which team is this particle?
     * @param spawnOrigin
     *  the Object who is the owner of this Particle (e.g the holder of a Weapon)
     * @param spawnParticleOrigin
     *  set to the PRT_REF if this Particle was spawned by another Particle
     * @param multispawn
     *  used for bulk spawn (spawning many particles at the same time). This is the index number of the particle in the bulk spawn
     * @param spawnTarget
     *  set the particle target to this on spawn
     * @return
     *   The Particle object that was spawned or nullptr if it failed.
     */
    std::shared_ptr<Ego::Particle> spawnParticle(const fvec3_t& spawnPos, const FACING_T spawnFacing, const PRO_REF spawnProfile,
                            const PIP_REF particleProfile, const CHR_REF spawnAttach, Uint16 vrt_offset, const TEAM_REF spawnTeam,
                            const CHR_REF spawnOrigin, const PRT_REF spawnParticleOrigin = INVALID_PRT_REF, const int multispawn = 0, 
                            const CHR_REF spawnTarget = INVALID_CHR_REF);

    /**
    * @brief
    *   Spawns a global particle
    * @return
    *   The Particle object that was spawned or nullptr if it failed.
    **/
    std::shared_ptr<Ego::Particle> spawnGlobalParticle(const fvec3_t& spawnPos, const FACING_T spawnFacing, const LocalParticleProfileRef& pip_index, int multispawn);

private:
    std::shared_ptr<Ego::Particle> getFreeParticle(bool force);

    //TODO: REMOVE
    PRT_REF spawnOneParticle(const fvec3_t& pos, FACING_T facing, const PRO_REF iprofile, const PIP_REF ipip,
                                              const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                                              const CHR_REF chr_origin, const PRT_REF prt_origin, const int multispawn, const CHR_REF oldtarget);

private:
    size_t _maxParticles;   ///< Maximum allowed active particles to be alive at the same time

    std::vector<std::shared_ptr<Ego::Particle>> _unusedPool;         //Particles currently unused
    std::vector<std::shared_ptr<Ego::Particle>> _activeParticles;    //List of all particles that are active ingame

    std::unordered_map<PRT_REF, std::shared_ptr<Ego::Particle>> _particleMap; //Mapping from PRT_REF to Particle
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
