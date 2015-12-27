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

#pragma once

#pragma once
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#include "egolib/typedef.h"
#include "egolib/Logic/Damage.hpp"
#include "egolib/Profiles/LocalParticleProfileRef.hpp"

/**
 * @brief
 *  The base class of EnchantProfile, ObjectProfile and ParticleProfiles.
 */
class AbstractProfile : public Id::NonCopyable
{

public:
    /**
     * @brief
     *  The name of the profile, usually the source pathname.
     */
    std::string _name;
    /**
     * @brief
     *  The number of attempted spawns.
     */
    size_t _spawnRequestCount;
    /**
     * @brief
     *  The number of successful spawns.
     */
    size_t _spawnCount;
    
protected:

    /**
     * @brief
     *  Construct this abstract profile.
     * @remark
     *  Intentionally protected.
     */
    AbstractProfile() :
        _name("*NONE*"), 
        _spawnRequestCount(0), 
        _spawnCount(0)
    {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Destruct this abstract profile.
     * @remark
     *  Intentionally protected.
     */
    virtual ~AbstractProfile()
    {
        /* Intentionally empty. */
    }

public:

    /**
     * @brief
     *  Get the name of this profile.
     * @return
     *  the name of this profile.
     */
    inline const std::string& getName() const {
        return _name;
    }
};

/// Enchants as well as particles can spawn other particles.
/// This structure describes aspects of this spawning process.
struct SpawnDescriptor
{
    uint8_t _amount;                 ///< Spawn amount
    
    uint16_t _facingAdd;             ///< Spawn in circle
    
    LocalParticleProfileRef _lpip; ///< Spawn type ( local )
    
    SpawnDescriptor() :
        _amount(0), _facingAdd(0), _lpip()
    {}
    
    void reset()
    {
        _amount = 0;
        _facingAdd = 0;
        _lpip = LocalParticleProfileRef::Invalid;
    }
};

/// Enchants as well as particles can continuously spawn other particles.
struct ContinuousSpawnDescriptor : public SpawnDescriptor
{
    uint16_t _delay;     ///< Delay between to consecutive spawns.
    ContinuousSpawnDescriptor() :
        SpawnDescriptor(),
        _delay(0)
    {
    }

    void reset()
    {
        this->SpawnDescriptor::reset();
        _delay = 0;
    }
};
