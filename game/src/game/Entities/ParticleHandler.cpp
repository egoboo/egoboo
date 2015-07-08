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

//--------------------------------------------------------------------------------------------

bool VALID_PRT_RANGE(const PRT_REF ref)
{
    return ref < ParticleHandler::get().getDisplayLimit();
}

bool DEFINED_PRT(const PRT_REF ref)
{
    return ParticleHandler::get()[ref] != nullptr;
}

bool ALLOCATED_PRT(const PRT_REF ref)
{
    return ParticleHandler::get().ALLOCATED(ref);
}

bool ACTIVE_PRT(const PRT_REF ref)
{
    return ParticleHandler::get().ACTIVE(ref);
}

bool WAITING_PRT(const PRT_REF ref)
{
    return ParticleHandler::get().WAITING(ref);
}

bool TERMINATED_PRT(const PRT_REF ref)
{
    return ParticleHandler::get().TERMINATED(ref);
}

PRT_REF GET_REF_PPRT(const prt_t *ptr)
{
	return LAMBDA(!ptr, INVALID_PRT_REF, ptr->GET_REF_POBJ(INVALID_PRT_REF));
}

//--------------------------------------------------------------------------------------------

bool DEFINED_PPRT(const prt_t *ptr)
{
    return ParticleHandler::get().DEFINED(ptr);
}

bool ALLOCATED_PPRT(const prt_t *ptr)
{
    return ParticleHandler::get().ALLOCATED(ptr);
}

bool ACTIVE_PPRT(const prt_t *ptr)
{
    return ParticleHandler::get().ACTIVE(ptr);
}

bool WAITING_PPRT(const prt_t *ptr)
{
    return ParticleHandler::get().WAITING(ptr);
}

bool TERMINATED_PPRT(const prt_t *ptr)
{
    return ParticleHandler::get().TERMINATED(ptr);
}

bool INGAME_PRT_BASE(const PRT_REF ref)
{
    return ParticleHandler::get().INGAME_BASE(ref);
}

bool INGAME_PPRT_BASE(const prt_t *ptr)
{
    return ParticleHandler::get().INGAME_BASE(ptr);
}

bool DISPLAY_PRT(const PRT_REF ref)
{
    return INGAME_PRT_BASE(ref);
}

bool DISPLAY_PPRT(const prt_t *ptr)
{
    return INGAME_PPRT_BASE(ptr);
}

bool INGAME_PRT(const PRT_REF ref)
{
    return LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_PRT(ref), INGAME_PRT_BASE(ref) && (!ParticleHandler::get().get_ptr(ref)->is_ghost));
}

bool INGAME_PPRT(const prt_t *ptr)
{
    return LAMBDA(Ego::Entities::spawnDepth > 0, INGAME_PPRT_BASE(ptr), DISPLAY_PPRT(ptr) && (!(ptr)->is_ghost));
}

//--------------------------------------------------------------------------------------------

static ParticleHandler PrtList;

ParticleHandler& ParticleHandler::get()
{
    return PrtList;
}

//--------------------------------------------------------------------------------------------
void ParticleHandler::update_used()
{
    prune_used_list();
    prune_free_list();

    // Go through the object list to see if there are any dangling objects.
    for (PRT_REF ref = 0; ref < getCount(); ++ref)
    {
        if (!isValidRef(ref)) continue;
        prt_t *x = get_ptr(ref);
		if (!x->ALLOCATED_PBASE()) continue;

        if (DISPLAY_PPRT(x))
        {
            if (!x->in_used_list)
            {
                push_used(ref);
            }
        }
		else if (!x->DEFINED_BASE_RAW()) // We can use DEFINED_BASE_RAW as the reference is valid.
        {
            if (!x->in_free_list)
            {
                add_free_ref(ref);
            }
        }
    }

    // Blank out the unused elements of the used list.
    for (size_t i = getUsedCount(); i < getCount(); ++i)
    {
        used_ref[i] = INVALID_PRT_REF;
    }

    // Blank out the unused elements of the free list.
    for (size_t i = getFreeCount(); i < getCount(); ++i)
    {
        free_ref[i] = INVALID_PRT_REF;
    }
}

//--------------------------------------------------------------------------------------------
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

PRT_REF ParticleHandler::spawnOneParticle(const fvec3_t& pos, FACING_T facing, const PRO_REF iprofile, const PIP_REF ipip,
                                          const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                                          const CHR_REF chr_origin, const PRT_REF prt_origin, const int multispawn, const CHR_REF oldtarget)
{
    if (!LOADED_PIP(ipip))
    {
        log_debug("spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
                  REF_TO_INT(ipip), REF_TO_INT(chr_origin), _currentModule->getObjectHandler().exists(chr_origin) ? _currentModule->getObjectHandler().get(chr_origin)->Name : "INVALID",
                  REF_TO_INT(iprofile), ProfileSystem::get().isValidProfileID(iprofile) ? ProfileSystem::get().getProfile(iprofile)->getPathname().c_str() : "INVALID");

        return INVALID_PRT_REF;
    }
    std::shared_ptr<pip_t> ppip = PipStack.get_ptr(ipip);

    // count all the requests for this particle type
    ppip->_spawnRequestCount++;

    PRT_REF iprt = ParticleHandler::get().allocate(ppip->force);
    if (!DEFINED_PRT(iprt))
    {
        log_debug("spawn_one_particle() - cannot allocate a particle owner == %d(\"%s\"), pip == %d(\"%s\"), profile == %d(\"%s\")\n",
                  chr_origin, _currentModule->getObjectHandler().exists(chr_origin) ? _currentModule->getObjectHandler().get(chr_origin)->Name : "INVALID",
                  ipip, LOADED_PIP(ipip) ? PipStack.get_ptr(ipip)->_name.c_str() : "INVALID",
                  iprofile, ProfileSystem::get().isValidProfileID(iprofile) ? ProfileSystem::get().getProfile(iprofile)->getPathname().c_str() : "INVALID");

        return INVALID_PRT_REF;
    }
    prt_t *pprt = ParticleHandler::get().get_ptr(iprt);

    if (pprt->isAllocated())
    {
        if (!pprt->spawning)
        {
            pprt->spawning = true;
            Ego::Entities::spawnDepth++;
        }
    }

    pprt->spawn_data.pos = pos;

    pprt->spawn_data.facing = facing;
    pprt->spawn_data.iprofile = iprofile;
    pprt->spawn_data.ipip = ipip;

    pprt->spawn_data.chr_attach = chr_attach;
    pprt->spawn_data.vrt_offset = vrt_offset;
    pprt->spawn_data.team = team;

    pprt->spawn_data.chr_origin = chr_origin;
    pprt->spawn_data.prt_origin = prt_origin;
    pprt->spawn_data.multispawn = multispawn;
    pprt->spawn_data.oldtarget = oldtarget;

    // actually force the character to spawn
    // count all the successful spawns of this particle
    if (pprt->config_activate(100))
    {
		pprt->POBJ_END_SPAWN();
        ppip->_spawnCount++;
    }

    return iprt;
}

PRT_REF ParticleHandler::spawn_one_particle(const fvec3_t& pos, FACING_T facing, const PRO_REF iprofile, const LocalParticleProfileRef& pip_index,
                                            const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                                            const CHR_REF chr_origin, const PRT_REF prt_origin, int multispawn, const CHR_REF oldtarget)
{
    PIP_REF ipip = INVALID_PIP_REF;

    if (!ProfileSystem::get().isValidProfileID(iprofile))
    {
        // check for a global pip
        ipip = ((pip_index.get() < 0) || (pip_index.get() > MAX_PIP)) ? MAX_PIP : static_cast<PIP_REF>(pip_index.get());
    }
    else
    {
        //Local character pip
        ipip = ProfileSystem::get().getProfile(iprofile)->getParticleProfile(pip_index);
    }
    return spawnOneParticle(pos, facing, iprofile, ipip, chr_attach, vrt_offset, team, chr_origin, prt_origin,
                            multispawn, oldtarget);
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

void ParticleHandler::updateAllParticles()
{
    //Update every active particle
    for(const std::shared_ptr<Ego::Particle> &particle : _activeParticles)
    {
        if(particle->isTerminated()) {
            continue;
        }

        particle->update();
    }

    auto condition = [this](const std::shared_ptr<Ego::Particle> &particle) 
    {
        if(!particle->isTerminated()) {
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
