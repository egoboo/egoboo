#pragma once

#pragma once
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#include "egolib/typedef.h"

/// @brief The base class of EnchantProfile, ObjectProfile and ParticleProfiles.
class AbstractProfile
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