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

/// @file egolib/game/Entities/ParticleHandler.hpp
/// @brief Handler of particle entities.

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/Entities/_Include.hpp` instead)
#endif

#include "egolib/game/egoboo.h"
#include "egolib/Entities/Particle.hpp"

class ParticleHandler : public id::singleton<ParticleHandler>
{
public:

    /**
    * @brief A completely recursive loop safe container for accessing instances of in-game objects
    **/
    class ParticleIterator
    {
    public:

        inline std::vector<std::shared_ptr<Ego::Particle>>::const_iterator cbegin() const 
        {
            return ParticleHandler::get()._activeParticles.cbegin();
        }

        inline std::vector<std::shared_ptr<Ego::Particle>>::const_iterator cend() const 
        {
            return ParticleHandler::get()._activeParticles.cend();
        }

        inline std::vector<std::shared_ptr<Ego::Particle>>::iterator begin()
        {
            return ParticleHandler::get()._activeParticles.begin();
        }

        inline std::vector<std::shared_ptr<Ego::Particle>>::iterator end()
        {
            return ParticleHandler::get()._activeParticles.end();
        }   

        ~ParticleIterator()
        {
            //Free the ParticleHandler lock
            ParticleHandler::get().unlock();
        }

        // Copy constructor
        ParticleIterator(const ParticleIterator &other)
        {
            ParticleHandler::get().lock();
        }
        
        // Disable copy assignment operator
        ParticleIterator& operator=(const ParticleIterator&) = delete;
    
    private:
        ParticleIterator()
        {
            // Ensure the ParticleHandler is locked as long as we are in existance.
            ParticleHandler::get().lock();
        }

        friend class ParticleHandler;
    };

public:
    ParticleHandler() :
        _maxParticles(0),
        _semaphoreLock(0),
        _totalParticlesSpawned(0),
        _unusedPool(),
        _activeParticles(),
        _particleMap(),
        
        _transparentParticleTexture("mp_data/globalparticles/particle_trans"),
        _lightParticleTexture("mp_data/globalparticles/particle_light")
    {
		setDisplayLimit(egoboo_config_t::get().graphic_simultaneousParticles_max.getValue());
    }

    /**
    * @brief
    *   Return a safe iterator that guarantees no changes will be made to the list
    *   as long as the iterator is alive
    **/
    ParticleIterator iterator() const { return ParticleIterator(); }

    /**
    * @brief
    *   Updates all particles and free particles that have been marked as terminated
    **/
    void updateAllParticles();

    void download(egoboo_config_t& cfg);

    void upload(egoboo_config_t& cfg);

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
    *   Resets and clears the particle handler, freeing all allocated Particle memory from the game
    **/
    void clear();

    /**
     * @brief
     *  Same as spawnParticle() except that it uses LocalParticleProfileRef instead of a PIP_REF
     *  Ideally we would like to remove this function as it is simply a wrapper
     */
    std::shared_ptr<Ego::Particle> spawnLocalParticle
        (
            const Vector3f& position,
            const Facing& facing,
            ObjectProfileRef iprofile,
            const LocalParticleProfileRef& pip_index,
            const ObjectRef chr_attach,
            uint16_t vrt_offset,
            const TEAM_REF team,
            const ObjectRef chr_origin,
            const ParticleRef prt_origin,
            int multispawn,
            const ObjectRef oldtarget
        );
    /**
     * @brief Get a pointer to the particle for a specified particle reference.
     * @return a pointer to the referenced particle if it was found, the null pointer otherwise
     */
    const std::shared_ptr<Ego::Particle>& operator[] (const ParticleRef index);

    /**
     * @brief
     *  Spawn a particle and add it into the game
     * @param spawnPos
     *  the position of the particle
     * @param spawnFacing
     *  the facing (direction) of the particle
     * @param spawnProfile
     *  the Object that spawned this particle (ObjectRef::Invalid for a global particle), e.g a Weapon
     * @param particleProfile
     *  the profile of the Particle
     * @param spawnAttach
     *  if this value is != ObjectRef::Invalid, then the particle will spawn attached to that Object
     * @param vrt_offset
     *  determines which vertex this particle will attach to when spawnAttach is used
     * @param spawnTeam
     *  on which team is this particle?
     * @param spawnOrigin
     *  the Object who is the owner of this Particle (e.g the holder of a Weapon)
     * @param spawnParticleOrigin
     *  set to the particle reference of the particle which spawned this particle
     * @param multispawn
     *  used for bulk spawn (spawning many particles at the same time). This is the index number of the particle in the bulk spawn
     * @param spawnTarget
     *  set the particle target to this on spawn
     * @param onlyOverWater
     *  if this is set to true, then the particle will fail to spawn if the resulting position (after random offsets) does not result in
     *  a tile containing water
     * @return
     *   The Particle object that was spawned or nullptr if it failed.
     */
    std::shared_ptr<Ego::Particle> spawnParticle(const Vector3f& spawnPos, const Facing& spawnFacing, const ObjectProfileRef spawnProfile,
                                                 const PIP_REF particleProfile, const ObjectRef spawnAttach, uint16_t vrt_offset, const TEAM_REF spawnTeam,
                                                 const ObjectRef spawnOrigin, const ParticleRef spawnParticleOrigin = ParticleRef::Invalid, const int multispawn = 0,
                                                 const ObjectRef spawnTarget = ObjectRef::Invalid, const bool onlyOverWater = false);

    /**
    * @brief
    *   Spawns a global particle
    * @return
    *   The Particle object that was spawned or nullptr if it failed.
    **/
    std::shared_ptr<Ego::Particle> spawnGlobalParticle(const Vector3f& spawnPos, const Facing& spawnFacing, const LocalParticleProfileRef& pip_index,
                                                       int multispawn, const bool onlyOverWater = false);

    /**
    * @brief
    *   Get number of particles that have been allocated for use
    **/
    size_t getCount() const {return _activeParticles.size() + _pendingParticles.size();}

    /**
    * @brief
    *   Get number of unused particles
    **/
    size_t getFreeCount() const { return std::min(_maxParticles, _maxParticles - getCount()); }

    std::shared_ptr<const Ego::Texture> getLightParticleTexture();
    std::shared_ptr<const Ego::Texture> getTransparentParticleTexture();

    void spawnPoof(const std::shared_ptr<Object> &object);

    void spawnDefencePing(const std::shared_ptr<Object> &object, const std::shared_ptr<Object> &attacker);

private:
    std::shared_ptr<Ego::Particle> getFreeParticle(bool force);

    void lock();

    void unlock();

private:
    static constexpr uint8_t DEFENDTIME = 24;   ///< Invincibility time after blocking an attack

    size_t _maxParticles;   ///< Maximum allowed active particles to be alive at the same time
    std::atomic<size_t> _semaphoreLock;
    std::atomic<size_t> _totalParticlesSpawned;

    std::vector<std::shared_ptr<Ego::Particle>> _unusedPool;         //Particles currently unused
    std::vector<std::shared_ptr<Ego::Particle>> _activeParticles;    //List of all particles that are active ingame
    std::vector<std::shared_ptr<Ego::Particle>> _pendingParticles;   //Particles that will be added to the active list as soon as it is unlocked

    std::unordered_map<ParticleRef, std::shared_ptr<Ego::Particle>> _particleMap; //Mapping from PRT_REF to Particle

    Ego::DeferredTexture _transparentParticleTexture;
    Ego::DeferredTexture _lightParticleTexture;
};
