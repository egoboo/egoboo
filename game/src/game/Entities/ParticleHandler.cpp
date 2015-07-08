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

/// @file  game/Entities/ParticleHandler.cpp
/// @brief Handler of particle entities.

#define GAME_ENTITIES_PRIVATE 1
#include "game/Entities/ParticleHandler.hpp"
#include "game/egoboo_object.h"
#include "game/Entities/Particle.hpp"


static ParticleHandler PrtList;

ParticleHandler& ParticleHandler::get()
{
    return PrtList;
}

//--------------------------------------------------------------------------------------------
#if 0
PRT_REF ParticleHandler::allocate(const bool force)
{
    // Return INVALID_PRT_REF if we can't find one.
    PRT_REF iprt = INVALID_PRT_REF;

    if (0 == freeCount)
    {
        if (force)
        {
            PRT_REF found        = INVALID_PRT_REF;
            size_t  min_life     = std::numeric_limits<size_t>::max();
            PRT_REF min_life_idx = INVALID_PRT_REF;
            size_t  min_anim     = std::numeric_limits<size_t>::max();
            PRT_REF min_anim_idx = INVALID_PRT_REF;

            // Gotta find one, so go through the list and replace a unimportant one.
            for (iprt = 0; iprt < getCount(); iprt++)
            {
                // Is this an invalid particle? The particle allocation count is messed up! :(
                if (!DEFINED_PRT(iprt))
                {
                    found = iprt;
                    break;
                }
                prt_t *pprt = get_ptr(iprt);

                // Does it have a valid profile?
                if (!LOADED_PIP(pprt->pip_ref))
                {
                    found = iprt;
                    end_one_particle_in_game(iprt);
                    break;
                }

                // Do not bump another.
                bool was_forced = TO_C_BOOL(PipStack.get_ptr(pprt->pip_ref)->force);

                if (WAITING_PRT(iprt))
                {
                    // If the particle has been "terminated" but is still waiting around,
                    // bump it to the front of the list.

                    size_t min_time = std::min( pprt->lifetime_remaining, pprt->frames_remaining );

                    if ( min_time < std::max( min_life, min_anim ) )
                    {
                        min_life     = pprt->lifetime_remaining;
                        min_life_idx = iprt;

                        min_anim     = pprt->frames_remaining;
                        min_anim_idx = iprt;
                    }
                }
                else if (!was_forced)
                {
                    // If the particle has not yet died, let choose the worst one.
                    if ( pprt->lifetime_remaining < min_life )
                    {
                        min_life     = pprt->lifetime_remaining;
                        min_life_idx = iprt;
                    }

                    if ( pprt->frames_remaining < min_anim )
                    {
                        min_anim     = pprt->frames_remaining;
                        min_anim_idx = iprt;
                    }
                }
            }

            if (isValidRef(found))
            {
                // Found a "bad" particle.
                iprt = found;
            }
            else if (isValidRef(min_anim_idx))
            {
                // Found a "terminated" particle.
                iprt = min_anim_idx;
            }
            else if (isValidRef(min_life_idx))
            {
                // Found a particle that closest to death.
                iprt = min_life_idx;
            }
            else
            {
                // Found nothing. This should only happen if all the particles are forced.
                iprt = INVALID_PRT_REF;
            }
        }
    }
    else
    {
        if (freeCount > (getCount() / 4) || force)
        {
            iprt = pop_free();
        }
    }

    // If we have found a a particle ...
    if (isValidRef(iprt))
    {
        // Make sure it is terminated.
        // If the particle is already being used, make sure to destroy the old one.
        if (DEFINED_PRT(iprt))
        {
            end_one_particle_in_game(iprt);
        }

        // Allocate the new one.
        get_ptr(iprt)->allocate(REF_TO_INT(iprt));
    }

    if (ALLOCATED_PRT(iprt))
    {
        // construct the new structure
        get_ptr(iprt)->config_construct(100);
    }

    return iprt;
}
#endif

std::shared_ptr<Ego::Particle> ParticleHandler::spawnLocalParticle(const fvec3_t& pos, FACING_T facing, const PRO_REF iprofile, const LocalParticleProfileRef& pip_index,
                                            const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                                            const CHR_REF chr_origin, const PRT_REF prt_origin, int multispawn, const CHR_REF oldtarget)
{
    //Local character pip
    PIP_REF ipip = ProfileSystem::get().getProfile(iprofile)->getParticleProfile(pip_index); 

    return spawnParticle(pos, facing, iprofile, ipip, chr_attach, vrt_offset, team, chr_origin, prt_origin, multispawn, oldtarget);
}

const std::shared_ptr<Ego::Particle>& ParticleHandler::operator[] (const PRT_REF index)
{
    auto result = _particleMap.find(index);
    
    //Does the PRT_REF not exist?
    if(result == _particleMap.end()) {
        return Ego::Particle::INVALID_PARTICLE;
    }

    //Check if particle was marked as terminated
    if((*result).second->isTerminated()) {
        return Ego::Particle::INVALID_PARTICLE;        
    }

    //All good!
    return (*result).second;
}

std::shared_ptr<Ego::Particle> ParticleHandler::spawnGlobalParticle(const fvec3_t& spawnPos, const FACING_T spawnFacing, const LocalParticleProfileRef& pip_index, int multispawn)
{
    //Get global particle profile
    PIP_REF globalProfile = ((pip_index.get() < 0) || (pip_index.get() > MAX_PIP)) ? MAX_PIP : static_cast<PIP_REF>(pip_index.get());

    return spawnParticle(spawnPos, spawnFacing, INVALID_PRO_REF, globalProfile, INVALID_CHR_REF, GRIP_LAST, Team::TEAM_NULL, 
        INVALID_CHR_REF, INVALID_PRT_REF, multispawn, INVALID_CHR_REF);
}

std::shared_ptr<Ego::Particle> ParticleHandler::spawnParticle(const fvec3_t& spawnPos, const FACING_T spawnFacing, const PRO_REF spawnProfile, 
                        const PIP_REF particleProfile, const CHR_REF spawnAttach, Uint16 vrt_offset, const TEAM_REF spawnTeam,
                        const CHR_REF spawnOrigin, const PRT_REF spawnParticleOrigin, const int multispawn, const CHR_REF spawnTarget)
{
    const std::shared_ptr<pip_t> &ppip = PipStack.get_ptr(spawnProfile);

    if (!ppip)
    {
        log_debug("spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
                  REF_TO_INT(spawnProfile), REF_TO_INT(spawnOrigin), _currentModule->getObjectHandler().exists(spawnOrigin) ? _currentModule->getObjectHandler().get(spawnOrigin)->Name : "INVALID",
                  REF_TO_INT(particleProfile), ProfileSystem::get().isValidProfileID(particleProfile) ? ProfileSystem::get().getProfile(particleProfile)->getPathname().c_str() : "INVALID");

        return Ego::Particle::INVALID_PARTICLE;
    }

    // count all the requests for this particle type
    ppip->_spawnRequestCount++;

    //Try to get a free particle
    std::shared_ptr<Ego::Particle> particle = getFreeParticle(ppip->force);

    //Initialize particle and add it into the game
    if(particle->initialize(spawnPos, spawnFacing, spawnProfile, particleProfile, spawnAttach, vrt_offset, spawnTeam, 
        spawnOrigin, spawnParticleOrigin, multispawn, spawnTarget)) {
        _activeParticles.push_back(particle);
    }
    else {
        //If we failed to spawn somehow, put it back to the unused pool
        _unusedPool.push_back(particle);
    }

    if(!particle) {
        log_debug("spawn_one_particle() - cannot allocate a particle!    owner == %d(\"%s\"), pip == %d(\"%s\"), profile == %d(\"%s\")\n",
                  spawnOrigin, _currentModule->getObjectHandler().exists(spawnOrigin) ? _currentModule->getObjectHandler().get(spawnOrigin)->Name : "INVALID",
                  spawnProfile, LOADED_PIP(spawnProfile) ? PipStack.get_ptr(spawnProfile)->_name.c_str() : "INVALID",
                  particleProfile, ProfileSystem::get().isValidProfileID(particleProfile) ? ProfileSystem::get().getProfile(particleProfile)->getPathname().c_str() : "INVALID");        
    }

    return particle;
}

std::shared_ptr<Ego::Particle> ParticleHandler::getFreeParticle(bool force)
{
    std::shared_ptr<Ego::Particle> particle = Ego::Particle::INVALID_PARTICLE;

    //TODO: Implement FORCE spawn priority

    //Not allowed to spawn more?
    if(_activeParticles.size() >= _maxParticles) {
        return particle;
    }

    //Get a free, unused particle from the particle pool
    if (!_unusedPool.empty())
    {
        particle = _unusedPool.back();
        _unusedPool.pop_back();

        //Clear any old memory
        particle->reset();
    }

    return particle;
}

size_t ParticleHandler::getDisplayLimit() const
{
    return _maxParticles;
}

void ParticleHandler::setDisplayLimit(size_t displayLimit)
{
    displayLimit = Ego::Math::constrain<size_t>(displayLimit, 256, PARTICLES_MAX);

    //Allocate more particle memory if required
    for(PRT_REF i = _maxParticles; i < displayLimit; ++i) {
        std::shared_ptr<Ego::Particle> particle = std::make_shared<Ego::Particle>(i);
        _unusedPool.push_back(particle);
        _particleMap[i] = particle;
    }
}

void ParticleHandler::lock()
{
    _semaphoreLock++;
}

void ParticleHandler::unlock()
{
    if(_semaphoreLock == 0) {
        throw std::logic_error("ParticleHandler::unlock() without prior lock()");
    }
    _semaphoreLock--;

    //All locks disengaged?
    if(_semaphoreLock == 0) {
        auto condition = [this](const std::shared_ptr<Ego::Particle> &particle) 
        {
            if(!particle->isTerminated()) {
                return false;
            }

            if (particle->getBSPLeaf().isInList()) {
                return false;
            }

            //Play end sound, trigger end spawn, etc.
            particle->destroy();

            //Free to be used by another instance again
            _unusedPool.push_back(particle);

            return true;
        };

        //Remove dead particles from the active list and add them to the free pool
        _activeParticles.erase(std::remove_if(_activeParticles.begin(), _activeParticles.end(), condition), _activeParticles.end());
    }
}

void ParticleHandler::updateAllParticles()
{
    //Update every active particle
    for(const std::shared_ptr<Ego::Particle> &particle : iterator())
    {
        if(particle->isTerminated()) {
            continue;
        }

        particle->update();
    }
}
