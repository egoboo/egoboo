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
#include "game/Entities/Particle.hpp"
#include "game/Entities/particle_physics.h"

class ParticleHandler : public Id::NonCopyable
{
public:
    static ParticleHandler& get();

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
        setDisplayLimit(512);
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
    std::shared_ptr<Ego::Particle> spawnLocalParticle(const Vector3f& position, FACING_T facing, const PRO_REF iprofile, const LocalParticleProfileRef& pip_index,
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
     * @param onlyOverWater
     *  if this is set to true, then the particle will fail to spawn if the resulting position (after random offsets) does not result in
     *  a tile containing water
     * @return
     *   The Particle object that was spawned or nullptr if it failed.
     */
    std::shared_ptr<Ego::Particle> spawnParticle(const Vector3f& spawnPos, const FACING_T spawnFacing, const PRO_REF spawnProfile,
                                                 const PIP_REF particleProfile, const CHR_REF spawnAttach, Uint16 vrt_offset, const TEAM_REF spawnTeam,
                                                 const CHR_REF spawnOrigin, const PRT_REF spawnParticleOrigin = INVALID_PRT_REF, const int multispawn = 0, 
                                                 const CHR_REF spawnTarget = INVALID_CHR_REF, const bool onlyOverWater = false);

    /**
    * @brief
    *   Spawns a global particle
    * @return
    *   The Particle object that was spawned or nullptr if it failed.
    **/
    std::shared_ptr<Ego::Particle> spawnGlobalParticle(const Vector3f& spawnPos, const FACING_T spawnFacing, const LocalParticleProfileRef& pip_index,
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

    const oglx_texture_t* getLightParticleTexture();
    const oglx_texture_t* getTransparentParticleTexture();

private:
    std::shared_ptr<Ego::Particle> getFreeParticle(bool force);

    void lock();

    void unlock();

private:
    size_t _maxParticles;   ///< Maximum allowed active particles to be alive at the same time
    std::atomic<size_t> _semaphoreLock;
    std::atomic<PRT_REF> _totalParticlesSpawned;

    std::vector<std::shared_ptr<Ego::Particle>> _unusedPool;         //Particles currently unused
    std::vector<std::shared_ptr<Ego::Particle>> _activeParticles;    //List of all particles that are active ingame
    std::vector<std::shared_ptr<Ego::Particle>> _pendingParticles;   //Particles that will be added to the active list as soon as it is unlocked

    std::unordered_map<PRT_REF, std::shared_ptr<Ego::Particle>> _particleMap; //Mapping from PRT_REF to Particle

    Ego::DeferredOpenGLTexture _transparentParticleTexture;
    Ego::DeferredOpenGLTexture _lightParticleTexture;
};
