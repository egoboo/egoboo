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
#include "game/Entities/Particle.hpp"

std::shared_ptr<Ego::Particle> ParticleHandler::spawnLocalParticle(const Vector3f& pos, FACING_T facing, const PRO_REF iprofile, const LocalParticleProfileRef& pip_index,
                                                                   const ObjectRef chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                                                                   const ObjectRef chr_origin, const PRT_REF prt_origin, int multispawn, const ObjectRef oldtarget)
{
    if(!ProfileSystem::get().isValidProfileID(iprofile)) {
		Log::get().debug("spawnLocalParticle() - cannot spawn particle with invalid PRO_REF %d\n", iprofile);
        return Ego::Particle::INVALID_PARTICLE;
    }

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
    if((*result).second->isTerminated() || (*result).second->getParticleID() != index) {
        _particleMap.erase(index);
        return Ego::Particle::INVALID_PARTICLE;        
    }

    //All good!
    return (*result).second;
}

std::shared_ptr<Ego::Particle> ParticleHandler::spawnGlobalParticle(const Vector3f& spawnPos, const FACING_T spawnFacing,
                                                                    const LocalParticleProfileRef& pip_index, int multispawn, const bool onlyOverWater)
{
    //Get global particle profile
    PIP_REF globalProfile = ((pip_index.get() < 0) || (pip_index.get() > MAX_PIP)) ? MAX_PIP : static_cast<PIP_REF>(pip_index.get());

    return spawnParticle(spawnPos, spawnFacing, INVALID_PRO_REF, globalProfile, ObjectRef::Invalid, GRIP_LAST, Team::TEAM_NULL,
                         ObjectRef::Invalid, INVALID_PRT_REF, multispawn, ObjectRef::Invalid, onlyOverWater);
}

std::shared_ptr<Ego::Particle> ParticleHandler::spawnParticle(const Vector3f& spawnPos, const FACING_T spawnFacing, const PRO_REF spawnProfile,
                                                              const PIP_REF particleProfile, const ObjectRef spawnAttach, Uint16 vrt_offset, const TEAM_REF spawnTeam,
                                                              const ObjectRef spawnOrigin, const PRT_REF spawnParticleOrigin, const int multispawn, const ObjectRef spawnTarget, const bool onlyOverWater)
{
    const std::shared_ptr<pip_t> &ppip = ParticleProfileSystem::get().get_ptr(particleProfile);

    if (!ppip)
    {
        const std::string spawnOriginName = _currentModule->getObjectHandler().exists(spawnOrigin) ? _currentModule->getObjectHandler()[spawnOrigin]->getName() : "INVALID";
        const std::string spawnProfileName = ProfileSystem::get().isValidProfileID(spawnProfile) ? ProfileSystem::get().getProfile(spawnProfile)->getPathname() : "INVALID";
		Log::get().debug("spawn_one_particle() - cannot spawn particle with invalid particle profile == %d, spawn origin == %" PRIuZ " (\"%s\"), spawn profile == %d (\"%s\"))\n",
                         REF_TO_INT(particleProfile), 
                         spawnOrigin.get(), spawnOriginName.c_str(),
                         REF_TO_INT(spawnProfile), spawnProfileName.c_str());

        return Ego::Particle::INVALID_PARTICLE;
    }

    // count all the requests for this particle type
    ppip->_spawnRequestCount++;

    //Try to get a free particle
    std::shared_ptr<Ego::Particle> particle = getFreeParticle(ppip->force);
    if(particle) {
        //Initialize particle and add it into the game
        if(particle->initialize(_totalParticlesSpawned++, spawnPos, spawnFacing, spawnProfile, particleProfile, spawnAttach, vrt_offset, 
                                spawnTeam, spawnOrigin, spawnParticleOrigin, multispawn, spawnTarget, onlyOverWater)) 
        {
            _pendingParticles.push_back(particle);
            _particleMap[particle->getParticleID()] = particle;
        }
        else {
            //If we failed to spawn somehow, put it back to the unused pool
            _unusedPool.push_back(particle);
        }        
    }

    if(!particle) {
        const std::string spawnOriginName = _currentModule->getObjectHandler().exists(spawnOrigin) ? _currentModule->getObjectHandler().get(spawnOrigin)->getName() : "INVALID";
        const std::string particleProfileName = LOADED_PIP(particleProfile) ? ParticleProfileSystem::get().get_ptr(particleProfile)->_name : "INVALID";
        const std::string spawnProfileName = ProfileSystem::get().isValidProfileID(spawnProfile) ? ProfileSystem::get().getProfile(spawnProfile)->getPathname().c_str() : "INVALID";
        Log::get().debug("spawn_one_particle() - cannot allocate a particle!    owner == %" PRIuZ "(\"%s\"), spawn profile == %d(\"%s\"), particle profile == %d(\"%s\")\n",
                         spawnOrigin.get(), spawnOriginName.c_str(),
                         REF_TO_INT(spawnProfile), spawnProfileName.c_str(),
                         REF_TO_INT(particleProfile), particleProfileName.c_str());        
    }

    return particle;
}

std::shared_ptr<Ego::Particle> ParticleHandler::getFreeParticle(bool force)
{
    std::shared_ptr<Ego::Particle> particle = Ego::Particle::INVALID_PARTICLE;

    //Reserve last 25% of free particle for FORCE spawn particles
    if(!force && getFreeCount() < _maxParticles/4) {
        return particle;
    }

    //Is this a high priority particle? If so, replace a less important particle
    if(getCount() >= _maxParticles && force) {
        bool spaceFreed = false;

        //Find a pending particle first
        for(size_t i = 0; i < _pendingParticles.size(); ++i) {

            //Do not replace other critical particles!
            if(_pendingParticles[i]->getProfile()->force) {
                continue;
            }

            //Just remove an unimportant particle that hasnt been activated yet
            _pendingParticles[i]->requestTerminate();
            spaceFreed = true;
            break;
        }

        //Nothing cleared? Search active particles then
        if(!spaceFreed) {
            for(size_t i = 0; i < _activeParticles.size(); ++i) {

                //Do not replace other critical particles!
                if(_activeParticles[i]->getProfile()->force) {
                    continue;
                }

                //Ignore terminated particles, we want to free something else
                if(_activeParticles[i]->isTerminated()) {
                    continue;
                }

                //Remove the first one found (oldest in the list)
                _activeParticles[i]->requestTerminate();
                break;
            }
        }
    }

    //If we have no free particles in the memory pool but we are allowed to allocate new memory
    if(_unusedPool.empty() && getCount() < _maxParticles) {
        return std::make_shared<Ego::Particle>();
    }

    //Get a free, unused particle from the particle pool
    if (!_unusedPool.empty())
    {
        //Retrieve particle from the pool
        particle = _unusedPool.back();
        _unusedPool.pop_back();
    }

    return particle;
}

size_t ParticleHandler::getDisplayLimit() const
{
    return _maxParticles;
}

void ParticleHandler::setDisplayLimit(size_t displayLimit)
{
    _maxParticles = Ego::Math::constrain<size_t>(displayLimit, 256, PARTICLES_MAX);
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

            //Play end sound, trigger end spawn, etc.
            particle->destroy();

            //Free to be used by another instance again
            _unusedPool.push_back(particle);
            _particleMap.erase(particle->getParticleID());

            return true;
        };

        //Remove dead particles from the active list and add them to the free pool
        _activeParticles.erase(std::remove_if(_activeParticles.begin(), _activeParticles.end(), condition), _activeParticles.end());

        //Add new particles that are pending to be added
        _activeParticles.insert(_activeParticles.end(), _pendingParticles.begin(), _pendingParticles.end());
        _pendingParticles.clear();
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

void ParticleHandler::clear()
{
    if(_semaphoreLock != 0) {
        throw std::logic_error("Calling ParticleHandler::clear() while locked");
    }

    _pendingParticles.clear();
    _activeParticles.clear();
    _unusedPool.clear();
    _particleMap.clear();
    _totalParticlesSpawned = 0;
}

const oglx_texture_t* ParticleHandler::getLightParticleTexture()
{
    return _lightParticleTexture.get_ptr();
}

const oglx_texture_t* ParticleHandler::getTransparentParticleTexture()
{
    return _transparentParticleTexture.get_ptr();
}

void ParticleHandler::spawnPoof(const std::shared_ptr<Object> &object)
{
    FACING_T facing_z = object->ori.facing_z;
    for (int cnt = 0; cnt < object->getProfile()->getParticlePoofAmount(); cnt++)
    {
        ParticleHandler::get().spawnParticle(object->getOldPosition(), facing_z, object->getProfile()->getSlotNumber(), object->getProfile()->getParticlePoofProfile(),
                                             ObjectRef::Invalid, GRIP_LAST, object->team, object->ai.owner, INVALID_PRT_REF, cnt);

        facing_z += object->getProfile()->getParticlePoofFacingAdd();
    }
}

void ParticleHandler::spawnDefencePing(const std::shared_ptr<Object> &object, const std::shared_ptr<Object> &attacker)
{
    if (0 != object->damage_timer) return;

    spawnGlobalParticle(object->getPosition(), object->ori.facing_z, LocalParticleProfileRef(PIP_DEFEND), 0);

    object->damage_timer = DEFENDTIME;
    SET_BIT(object->ai.alert, ALERTIF_BLOCKED);

    // For the ones attacking a shield
    if(attacker != nullptr && !attacker->isTerminated()) {
        object->ai.setLastAttacker(attacker->getObjRef());
    }
    else {
        object->ai.setLastAttacker(ObjectRef::Invalid);
    }
}
