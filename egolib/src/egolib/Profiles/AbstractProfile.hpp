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

/// @brief The base class of EnchantProfile, ObjectProfile and ParticleProfiles.
class AbstractProfile : public Ego::Core::NonCopyable
{

public:

    bool _loaded;              ///< Was the data read in?
    STRING _name;              ///< Usually the source filename.
    size_t _spawnRequestCount; ///< The number of attempted spawns.
    size_t _spawnCount;        ///< The number of successful spawns.
    
protected:

    AbstractProfile() :
        _loaded(false), _spawnRequestCount(0), _spawnCount(0)
    {
        _name[0] = '\0';
    }

    virtual ~AbstractProfile()
    {
    }

public:

    /// @todo Rename to reset.
    void init()
    {
        _loaded = false;
        _name[0] = '\0';
        _spawnRequestCount = 0;
        _spawnCount = 0;
    }

};

/// Enchants as well as particles can spawn other particles.
/// This structure describes aspects of this spawning process.
struct SpawnDescriptor
{
    Uint8 _amount;     ///< Spawn amount
    Uint16 _facingAdd; ///< Spawn in circle
    int _lpip;         ///< Spawn type ( local )
    SpawnDescriptor() :
        _amount(0), _facingAdd(0), _lpip(-1)
    {}
    /// @todo Rename to reset.
    void init()
    {
        _amount = 0;
        _facingAdd = 0;
        _lpip = -1;
    }
};

/// Enchants as well as particles can continuously spawn other particles.
struct ContinuousSpawnDescriptor : public SpawnDescriptor
{
    Uint16 _delay;     ///< Delay between to consecutive spawns.
    ContinuousSpawnDescriptor() :
        SpawnDescriptor(),
        _delay(0)
    {
    }
    /// @todo Rename to init.
    void init()
    {
        this->SpawnDescriptor::init();
        _delay = 0;
    }
};