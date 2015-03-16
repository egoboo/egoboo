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
            prt_bundle_t::set(&PRT_BDL, ParticleHandler::get().get_ptr( IT ));

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
            prt_bundle_t::set(&PRT_BDL, ParticleHandler::get().get_ptr(IT));

#define PRT_END_LOOP() \
        } \
        ParticleHandler::get().unlock(); \
        EGOBOO_ASSERT(prt_loop_start_depth == ParticleHandler::get().getLockCount()); \
        ParticleHandler::get().maybeRunDeferred(); \
    }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

struct ParticleHandler : public _LockableList < prt_t, PRT_REF, INVALID_PRT_REF, MAX_PRT, BSP_LEAF_PRT>
{
    ParticleHandler() :
        _LockableList(),
        _displayLimit(512)
    {
    }

    void update_used();

    /**
     * @brief
     *	Run all deferred updates if the particle list is not locked.
     */
    void maybeRunDeferred();

    /**
     * @brief
     *  Get an unused particle.
     *   If all particles are in use and @a force is @a true, get the first unimportant one.
     * @return
     *  the particle index on success, INVALID_PRT_REF on failure
     */
    PRT_REF allocate(const bool force);

    void reset_all();

public:
    static ParticleHandler& get();
    bool free_one(const PRT_REF iprt);
    bool push_free(const PRT_REF);
    void prune_used_list();
    void prune_free_list();

protected:
    /**
     * @brief
     *  An display limit smaller than @a MAX_PRT is an upper-bound for the number of particles rendered.
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
        displayLimit = Math::constrain<uint16_t>(displayLimit, 256, MAX_PRT);
        if (_displayLimit != displayLimit)
        {
            _displayLimit = displayLimit;
        }
    }
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
bool VALID_PRT_PTR(const prt_t *ptr);
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
