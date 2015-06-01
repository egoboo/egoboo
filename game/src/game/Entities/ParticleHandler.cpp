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
    return ParticleHandler::get().isValidRef(ref);
}

bool DEFINED_PRT(const PRT_REF ref)
{
    return ParticleHandler::get().DEFINED(ref);
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
		if (!POBJ_GET_PBASE(x)->ALLOCATED_PBASE()) continue;

        if (DISPLAY_PPRT(x))
        {
            if (!x->obj_base.in_used_list)
            {
                push_used(ref);
            }
        }
		else if (!POBJ_GET_PBASE(x)->DEFINED_BASE_RAW()) // We can use DEFINED_BASE_RAW as the reference is valid.
        {
            if (!x->obj_base.in_free_list)
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
        get_ptr(iprt)->obj_base.allocate(REF_TO_INT(iprt));
    }

    if (ALLOCATED_PRT(iprt))
    {
        // construct the new structure
        get_ptr(iprt)->config_construct(100);
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
void ParticleHandler::reset_all()
{
    const char *loadpath;

    PipStack.reset();

    // Load in the standard global particles ( the coins for example )
    loadpath = "mp_data/1money.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_COIN1 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/5money.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_COIN5 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/25money.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_COIN25 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/100money.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_COIN100 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/200money.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_GEM200 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/500money.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_GEM500 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/1000money.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_GEM1000 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/2000money.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_GEM2000 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }
#if 0
    // Load module specific information
    loadpath = "mp_data/weather4.txt";
    if (INVALID_PIP_REF == PipStack.load_one(loadpath, (PIP_REF)PIP_WEATHER)) 
    {
        /*log_error("Data file was not found! (\"%s\")\n", loadpath);*/
    }

    loadpath = "mp_data/weather5.txt";
    if (INVALID_PIP_REF == PipStack.load_one(loadpath, (PIP_REF)PIP_WEATHER_FINISH))
    {
        /*log_error("Data file was not found! (\"%s\")\n", loadpath);*/
    }
#endif

    loadpath = "mp_data/splash.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_SPLASH ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/ripple.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_RIPPLE ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    // This is also global...
    loadpath = "mp_data/defend.txt";
    if ( INVALID_PIP_REF == PipStack.load_one( loadpath, ( PIP_REF )PIP_DEFEND ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }
}

PRT_REF ParticleHandler::spawnOneParticle(const fvec3_t& pos, FACING_T facing, const PRO_REF iprofile, const PIP_REF ipip,
                                          const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                                          const CHR_REF chr_origin, const PRT_REF prt_origin, const int multispawn, const CHR_REF oldtarget)
{
    if (!LOADED_PIP(ipip))
    {
        log_debug("spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
                  REF_TO_INT(ipip), REF_TO_INT(chr_origin), _gameObjects.exists(chr_origin) ? _gameObjects.get(chr_origin)->Name : "INVALID",
                  REF_TO_INT(iprofile), ProfileSystem::get().isValidProfileID(iprofile) ? ProfileSystem::get().getProfile(iprofile)->getPathname().c_str() : "INVALID");

        return INVALID_PRT_REF;
    }
    pip_t *ppip = PipStack.get_ptr(ipip);

    // count all the requests for this particle type
    ppip->_spawnRequestCount++;

    PRT_REF iprt = ParticleHandler::get().allocate(ppip->force);
    if (!DEFINED_PRT(iprt))
    {
        log_debug("spawn_one_particle() - cannot allocate a particle owner == %d(\"%s\"), pip == %d(\"%s\"), profile == %d(\"%s\")\n",
                  chr_origin, _gameObjects.exists(chr_origin) ? _gameObjects.get(chr_origin)->Name : "INVALID",
                  ipip, LOADED_PIP(ipip) ? PipStack.get_ptr(ipip)->_name.c_str() : "INVALID",
                  iprofile, ProfileSystem::get().isValidProfileID(iprofile) ? ProfileSystem::get().getProfile(iprofile)->getPathname().c_str() : "INVALID");

        return INVALID_PRT_REF;
    }
    prt_t *pprt = ParticleHandler::get().get_ptr(iprt);

    if ((pprt)->obj_base.isAllocated())
    {
        if (!(pprt)->obj_base.spawning)
        {
            (pprt)->obj_base.spawning = true;
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

PRT_REF ParticleHandler::spawn_one_particle_global(const fvec3_t& pos, FACING_T facing, const LocalParticleProfileRef& pip_index, int multispawn)
{
    return spawn_one_particle(pos, facing, INVALID_PRO_REF, pip_index, INVALID_CHR_REF, GRIP_LAST,
                              (TEAM_REF)TEAM_NULL, INVALID_CHR_REF, INVALID_PRT_REF, multispawn, INVALID_CHR_REF);
}
