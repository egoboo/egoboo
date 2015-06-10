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

/// @file  game/Entities/Particle.cpp
/// @brief Particle entities.

#define GAME_ENTITIES_PRIVATE 1
#include "game/Core/GameEngine.hpp"
#include "egolib/Audio/AudioSystem.hpp"
#include "egolib/Profiles/_Include.hpp"
#include "game/game.h"
#include "game/mesh.h"
#include "game/obj_BSP.h"
#include "egolib/Graphics/mad.h"
#include "game/renderer_3d.h"
#include "game/egoboo.h"
#include "game/mesh.h"
#include "game/Entities/EnchantHandler.hpp"
#include "game/Entities/ParticleHandler.hpp"
#include "game/Entities/ObjectHandler.hpp"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define PRT_TRANS 0x80

const float buoyancy_friction = 0.2f;          // how fast does a "cloud-like" object slow down?

static const float STOPBOUNCINGPART = 10.0f;        ///< To make particles stop bouncing

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int prt_stoppedby_tests = 0;
int prt_pressure_tests = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int prt_do_end_spawn(const PRT_REF iprt);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool prt_t::free(prt_t * pprt)
{
    if (!ALLOCATED_PPRT(pprt)) return false;

    // do not allow this if you are inside a particle loop
    EGOBOO_ASSERT(0 == ParticleHandler::get().getLockCount());

    if (TERMINATED_PPRT(pprt)) return true;

    // deallocate any dynamic data

    return true;
}

//--------------------------------------------------------------------------------------------
prt_t::prt_t(PRT_REF ref) :
	_StateMachine<prt_t, PRT_REF, ParticleHandler>(BSP_LEAF_PRT, ref) {
}

void prt_t::reset() {
	this->_StateMachine<prt_t, PRT_REF, ParticleHandler>::reset();
	this->is_ghost = false;

	this->spawn_data.reset();

	this->pip_ref = INVALID_PIP_REF;
	this->profile_ref = INVALID_PRO_REF;

	this->attachedto_ref = INVALID_CHR_REF;
	this->owner_ref = INVALID_CHR_REF;
	this->target_ref = INVALID_CHR_REF;
	this->parent_ref = INVALID_PRT_REF;
	this->parent_guid = EGO_GUID_INVALID;

	this->attachedto_vrt_off = 0;
	this->type = 0;
	this->facing = 0;
	this->team = 0;


	this->_image.reset();

	this->vel_stt = fvec3_t::zero();

	PhysicsData::reset(this);

	this->offset = fvec3_t::zero();

	this->is_hidden = false;

	this->rotate = 0;
	this->rotate_add = 0;

	this->size_stt = 0;
	this->size = 0;
	this->size_add = 0;

	// "no lifetime" = "eternal"
	this->is_eternal = false;
	this->lifetime_total = (size_t)(~0);
	this->lifetime_remaining = this->lifetime_total;
	this->frames_total = (size_t)(~0);
	this->frames_remaining = this->frames_total;
	//
	this->contspawn_timer = 0;

	// bumping
	this->bump_size_stt = 0;           ///< the starting size of the particle (8.8 fixed point)
	bumper_t::reset(&this->bump_real);
	bumper_t::reset(&this->bump_padded);
	this->prt_min_cv.mins = this->prt_min_cv.maxs = oct_vec_v2_t();
	this->prt_min_cv.empty = false;
	this->prt_max_cv.mins = this->prt_max_cv.maxs = oct_vec_v2_t();
	this->prt_max_cv.empty = false;

	// damage
	this->damagetype = DamageType::DAMAGE_SLASH;
	this->damage.base = 0;
	this->damage.rand = 0;
	this->lifedrain = 0;
	this->manadrain = 0;

	// bump effects
	this->is_bumpspawn = false;

	// motion effects
	this->buoyancy = 0.0f;
	this->air_resistance = 0.0f;
	this->is_homing = false;
	this->no_gravity = false;

	// some data that needs to be copied from the particle profile
	this->endspawn_amount = 0;         ///< The number of particles to be spawned at the end
	this->endspawn_facingadd = 0;      ///< The angular spacing for the end spawn
	this->endspawn_lpip = LocalParticleProfileRef::Invalid; ///< The actual local pip that will be spawned at the end
	this->endspawn_characterstate = 0; ///< if != SPAWNNOCHARACTER, then a character is spawned on end

	this->dynalight.reset();
	this->inst.reset();
	this->enviro.reset();
}

void prt_t::config_do_ctor()
{
    this->is_ghost = false;
    
    this->spawn_data.reset();

    this->pip_ref = INVALID_PIP_REF;
    this->profile_ref = INVALID_PRO_REF;

    this->attachedto_ref = INVALID_CHR_REF;
    this->owner_ref = INVALID_CHR_REF;
    this->target_ref = INVALID_CHR_REF;
    this->parent_ref = INVALID_PRT_REF;
    this->parent_guid = EGO_GUID_INVALID;

    this->attachedto_vrt_off = 0;
    this->type = 0;
    this->facing = 0;
    this->team = 0;


    this->_image.reset();

    this->vel_stt = fvec3_t::zero();

    PhysicsData::reset(this);

    this->offset = fvec3_t::zero();

    this->is_hidden = false;

    this->rotate = 0;
    this->rotate_add = 0;

    this->size_stt = 0;
    this->size = 0;
    this->size_add = 0;

    // "no lifetime" = "eternal"
    this->is_eternal = false;
    this->lifetime_total = (size_t)(~0);
    this->lifetime_remaining = this->lifetime_total;
    this->frames_total = (size_t)(~0);
    this->frames_remaining = this->frames_total;
    //
    this->contspawn_timer = 0;

    // bumping
    this->bump_size_stt = 0;           ///< the starting size of the particle (8.8 fixed point)
    bumper_t::reset(&this->bump_real);
    bumper_t::reset(&this->bump_padded);
    this->prt_min_cv.mins = this->prt_min_cv.maxs = oct_vec_v2_t();
    this->prt_min_cv.empty = false;
    this->prt_max_cv.mins = this->prt_max_cv.maxs = oct_vec_v2_t();
    this->prt_max_cv.empty = false;

    // damage
    this->damagetype = DamageType::DAMAGE_SLASH;
    this->damage.base = 0;
    this->damage.rand = 0;
    this->lifedrain = 0;
    this->manadrain = 0;

    // bump effects
    this->is_bumpspawn = false;

    // motion effects
    this->buoyancy = 0.0f;
    this->air_resistance = 0.0f;
    this->is_homing = false;
    this->no_gravity = false;

    // some data that needs to be copied from the particle profile
    this->endspawn_amount = 0;         ///< The number of particles to be spawned at the end
    this->endspawn_facingadd = 0;      ///< The angular spacing for the end spawn
    this->endspawn_lpip = LocalParticleProfileRef::Invalid; ///< The actual local pip that will be spawned at the end
    this->endspawn_characterstate = 0; ///< if != SPAWNNOCHARACTER, then a character is spawned on end

    this->dynalight.reset();
    this->inst.reset();
    this->enviro.reset();

    // reset the base counters
	this->update_count = 0;
	this->frame_count = 0;
	this->state = State::Initializing;
}

//--------------------------------------------------------------------------------------------
prt_t::~prt_t() {
}

void prt_t::config_do_dtor() {
    // destruct/free any allocated data
    prt_t::free(this);

    // Destroy the base object.
    // Sets the state to ego_object_terminated automatically.
    this->terminate();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void prt_play_sound(const prt_t& prt, Sint8 sound)
{
    if (!DEFINED_PPRT(&prt)) return;
    if (ProfileSystem::get().isValidProfileID(prt.profile_ref))
    {
        /// @todo MH: Someone explain to me why this is an object profile?
        const auto profile = ProfileSystem::get().getProfile(prt.profile_ref);
        AudioSystem::get().playSound(prt.pos, profile->getSoundID(sound));
    }
    else if (sound >= 0 && sound < GSND_COUNT)
    {
        GlobalSound globalSound = static_cast<GlobalSound>(sound);
        AudioSystem::get().playSound(prt.pos, AudioSystem::get().getGlobalSound(globalSound));
    }
}
void prt_play_sound(const PRT_REF particle, Sint8 sound)
{
    if (ParticleHandler::get().isValidRef(particle)) {
        prt_t *pprt = ParticleHandler::get().get_ptr(particle);
        return prt_play_sound(*pprt, sound);
    }
}

//--------------------------------------------------------------------------------------------
void end_one_particle_now(const PRT_REF iprt)
{
    prt_t *pprt = ParticleHandler::get().get_ptr(iprt);
    pprt->requestTerminate();
}

//--------------------------------------------------------------------------------------------
void end_one_particle_in_game(const PRT_REF particle)
{
    // does the particle have valid data?
    if (DEFINED_PRT(particle))
    {
        prt_t *pprt = ParticleHandler::get().get_ptr(particle);
        std::shared_ptr<pip_t> ppip = pprt->get_ppip();

        // The object is waiting to be killed, so do all of the end of life care for the particle.
        prt_do_end_spawn(particle);

        if (SPAWNNOCHARACTER != pprt->endspawn_characterstate)
        {
            CHR_REF child = spawn_one_character(pprt->getPosition(), pprt->profile_ref, pprt->team, 0, pprt->facing, NULL, INVALID_CHR_REF);
            if (_currentModule->getObjectHandler().exists(child))
            {
                Object *pchild = _currentModule->getObjectHandler().get(child);

                chr_set_ai_state(pchild, pprt->endspawn_characterstate);
                pchild->ai.owner = pprt->owner_ref;
            }
        }

        // Play end sound.
        if (nullptr != ppip)
        {
            prt_play_sound(particle, ppip->end_sound);
        }
    }

    return end_one_particle_now(particle);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

prt_t *prt_t::config_do_init()
{
    prt_t *pprt = this;
    const int INFINITE_UPDATES = std::numeric_limits<int>::max();

    int     velocity;
    fvec3_t vel;
    float   tvel;
    int     offsetfacing = 0, newrand;
    fvec3_t tmp_pos;
    TURN_T  turn;
    float   loc_spdlimit;
    int     prt_life_frames_updates, prt_anim_frames_updates;
    bool  prt_life_infinite, prt_anim_infinite;

    FACING_T loc_facing;
    CHR_REF loc_chr_origin;

    if (NULL == pprt) return NULL;
    prt_spawn_data_t *pdata = &(pprt->spawn_data);
    PRT_REF iprt = GET_REF_PPRT(pprt);

    // Convert from local pdata->ipip to global pdata->ipip
    if (!LOADED_PIP(pdata->ipip))
    {
        log_debug("spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
            REF_TO_INT(pdata->ipip), REF_TO_INT(pdata->chr_origin), _currentModule->getObjectHandler().exists(pdata->chr_origin) ? _currentModule->getObjectHandler().get(pdata->chr_origin)->Name : "INVALID",
            REF_TO_INT(pdata->iprofile), ProfileSystem::get().isValidProfileID(pdata->iprofile) ? ProfileSystem::get().getProfile(pdata->iprofile)->getPathname().c_str() : "INVALID");

        return nullptr;
    }
    std::shared_ptr<pip_t> ppip = PipStack.get_ptr(pdata->ipip);

    // let the object be activated
    if (pprt->isAllocated() && !pprt->kill_me && Ego::Entity::State::Invalid != pprt->state)
    {
		pprt->_name = ppip->_name;
        pprt->state = Ego::Entity::State::Active;
    }

    // make some local copies of the spawn data
    loc_facing = pdata->facing;

    // Save a version of the position for local use.
    // In cpp, will be passed by reference, so we do not want to alter the
    // components of the original vector.
    tmp_pos = pdata->pos;

    // try to get an idea of who our owner is even if we are
    // given bogus info
    loc_chr_origin = pdata->chr_origin;
    if (!_currentModule->getObjectHandler().exists(pdata->chr_origin) && DEFINED_PRT(pdata->prt_origin))
    {
        loc_chr_origin = prt_get_iowner(pdata->prt_origin, 0);
    }

    pprt->pip_ref = pdata->ipip;
    pprt->profile_ref = pdata->iprofile;
    pprt->team = pdata->team;
    pprt->owner_ref = loc_chr_origin;
    pprt->parent_ref = pdata->prt_origin;
    pprt->parent_guid = ALLOCATED_PRT(pdata->prt_origin) ? ParticleHandler::get().get_ptr(pdata->prt_origin)->getGUID() : EGO_GUID_INVALID;
    pprt->damagetype = ppip->damageType;
    pprt->lifedrain = ppip->lifeDrain;
    pprt->manadrain = ppip->manaDrain;

    // Lighting and sound
    pprt->dynalight = ppip->dynalight;
    pprt->dynalight.on = false;
    if (0 == pdata->multispawn)
    {
        pprt->dynalight.on = ppip->dynalight.mode;
        if (DYNA_MODE_LOCAL == ppip->dynalight.mode)
        {
            pprt->dynalight.on = DYNA_MODE_OFF;
        }
    }

    // Set character attachments ( pdata->chr_attach == INVALID_CHR_REF means none )
    pprt->attachedto_ref = pdata->chr_attach;
    pprt->attachedto_vrt_off = pdata->vrt_offset;

    // Correct loc_facing
    loc_facing = loc_facing + ppip->facing_pair.base;

    // Targeting...
    vel[kZ] = 0;

    pprt->offset[kZ] = generate_irand_pair(ppip->spacing_vrt_pair) - (ppip->spacing_vrt_pair.rand / 2);
    tmp_pos[kZ] += pprt->offset[kZ];
    velocity = generate_irand_pair(ppip->vel_hrz_pair);
    pprt->target_ref = pdata->oldtarget;
    if (ppip->newtargetonspawn)
    {
        if (ppip->targetcaster)
        {
            // Set the target to the caster
            pprt->target_ref = loc_chr_origin;
        }
        else
        {
            const int PERFECT_AIM = 45 * 256;   // 45 dex is perfect aim

            // Find a target
            pprt->target_ref = prt_find_target(pdata->pos, loc_facing, pdata->ipip, pdata->team, loc_chr_origin, pdata->oldtarget);
            if (_currentModule->getObjectHandler().exists(pprt->target_ref) && !ppip->homing)
            {
                /// @note ZF@> ?What does this do?!
                /// @note BB@> glouseangle is the angle found in prt_find_target()
                loc_facing -= glouseangle;
            }

            // Correct loc_facing for dexterity...
            offsetfacing = 0;
            if (_currentModule->getObjectHandler().get(loc_chr_origin)->dexterity < PERFECT_AIM)
            {
                // Correct loc_facing for randomness
                offsetfacing = generate_irand_pair(ppip->facing_pair) - (ppip->facing_pair.base + ppip->facing_pair.rand / 2);
                offsetfacing = (offsetfacing * (PERFECT_AIM - _currentModule->getObjectHandler().get(loc_chr_origin)->dexterity)) / PERFECT_AIM;
            }

            if (0.0f != ppip->zaimspd)
            {
                if (_currentModule->getObjectHandler().exists(pprt->target_ref))
                {
                    // These aren't velocities...  This is to do aiming on the Z axis
                    if (velocity > 0)
                    {
                        vel[kX] = _currentModule->getObjectHandler().get(pprt->target_ref)->getPosX() - pdata->pos[kX];
                        vel[kY] = _currentModule->getObjectHandler().get(pprt->target_ref)->getPosY() - pdata->pos[kY];
                        tvel = std::sqrt(vel[kX] * vel[kX] + vel[kY] * vel[kY]) / velocity;  // This is the number of steps...
                        if (tvel > 0.0f)
                        {
                            // This is the vel[kZ] alteration
                            vel[kZ] = (_currentModule->getObjectHandler().get(pprt->target_ref)->getPosZ() + (_currentModule->getObjectHandler().get(pprt->target_ref)->bump.height * 0.5f) - tmp_pos[kZ]) / tvel;
                        }
                    }
                }
                else
                {
                    vel[kZ] = 0.5f * ppip->zaimspd;
                }

                vel[kZ] = CLIP(vel[kZ], -0.5f * ppip->zaimspd, ppip->zaimspd);
            }
        }

        // Does it go away?
        if (!_currentModule->getObjectHandler().exists(pprt->target_ref) && ppip->needtarget)
        {
            end_one_particle_in_game(iprt);
            return NULL;
        }

        // Start on top of target
        if (_currentModule->getObjectHandler().exists(pprt->target_ref) && ppip->startontarget)
        {
            tmp_pos[kX] = _currentModule->getObjectHandler().get(pprt->target_ref)->getPosX();
            tmp_pos[kY] = _currentModule->getObjectHandler().get(pprt->target_ref)->getPosY();
        }
    }
    else
    {
        // Correct loc_facing for randomness
        offsetfacing = generate_irand_pair(ppip->facing_pair) - (ppip->facing_pair.base + ppip->facing_pair.rand / 2);
    }
    loc_facing = loc_facing + offsetfacing;
    pprt->facing = loc_facing;

    // this is actually pointing in the opposite direction?
    turn = TO_TURN(loc_facing);

    // Location data from arguments
    newrand = generate_irand_pair(ppip->spacing_hrz_pair);
    pprt->offset[kX] = -turntocos[turn] * newrand;
    pprt->offset[kY] = -turntosin[turn] * newrand;

    tmp_pos[kX] += pprt->offset[kX];
    tmp_pos[kY] += pprt->offset[kY];

    tmp_pos[kX] = CLIP(tmp_pos[kX], 0.0f, PMesh->gmem.edge_x - 2.0f);
    tmp_pos[kY] = CLIP(tmp_pos[kY], 0.0f, PMesh->gmem.edge_y - 2.0f);

    pprt->setPosition(tmp_pos);
    pprt->pos_old = tmp_pos;
    pprt->pos_stt = tmp_pos;

    // Velocity data
    vel[kX] = -turntocos[turn] * velocity;
    vel[kY] = -turntosin[turn] * velocity;
    vel[kZ] += generate_irand_pair(ppip->vel_vrt_pair) - (ppip->vel_vrt_pair.rand / 2);
    pprt->vel = pprt->vel_old = pprt->vel_stt = vel;

    // Template values
    pprt->bump_size_stt = ppip->bump_size;
    pprt->type = ppip->type;

    // Image data
    pprt->rotate = (FACING_T)generate_irand_pair(ppip->rotate_pair);
    pprt->rotate_add = ppip->rotate_add;

    pprt->size_stt = ppip->size_base;
    pprt->size_add = ppip->size_add;

    pprt->_image._start = (ppip->image_stt)*EGO_ANIMATION_MULTIPLIER;
    pprt->_image._add = generate_irand_pair(ppip->image_add);
    pprt->_image._count = (ppip->image_max)*EGO_ANIMATION_MULTIPLIER;

    // a particle can EITHER end_lastframe or end_time.
    // if it ends after the last frame, end_time tells you the number of cycles through
    // the animation
    prt_anim_frames_updates = 0;
    prt_anim_infinite = false;
    if (ppip->end_lastframe)
    {
        if (0 == pprt->_image._add)
        {
            prt_anim_frames_updates = INFINITE_UPDATES;
            prt_anim_infinite = true;
        }
        else
        {
            prt_anim_frames_updates = pprt->_image.getUpdateCount();

            if (ppip->end_time > 0)
            {
                // Part time is used to give number of cycles
                prt_anim_frames_updates *= ppip->end_time;
            }
        }
    }
    else
    {
        // no end to the frames
        prt_anim_frames_updates = INFINITE_UPDATES;
        prt_anim_infinite = true;
    }
    prt_anim_frames_updates = std::max(1, prt_anim_frames_updates);

    // estimate the number of frames
    prt_life_frames_updates = 0;
    prt_life_infinite = false;
    if (ppip->end_lastframe)
    {
        // for end last frame, the lifetime is given by the
        prt_life_frames_updates = prt_anim_frames_updates;
        prt_life_infinite = prt_anim_infinite;
    }
    else if (ppip->end_time <= 0)
    {
        // zero or negative lifetime == infinite lifetime
        prt_life_frames_updates = INFINITE_UPDATES;
        prt_life_infinite = true;
    }
    else
    {
        prt_life_frames_updates = ppip->end_time;
    }
    prt_life_frames_updates = std::max(1, prt_life_frames_updates);

    // set lifetime counter
    if (prt_life_infinite)
    {
        pprt->lifetime_total = (size_t)(~0);
        pprt->is_eternal = true;
    }
    else
    {
        // the lifetime is really supposed tp be in terms of frames, but
        // to keep the number of updates stable, the frames could lag.
        // sooo... we just rescale the prt_life_frames_updates so that it will work with the
        // updates and cross our fingers
        pprt->lifetime_total = std::ceil((float)prt_life_frames_updates * (float)GameEngine::GAME_TARGET_UPS / (float)GameEngine::GAME_TARGET_FPS);
    }
    // make the particle exists for AT LEAST one update
    pprt->lifetime_total = std::max((size_t)1, pprt->lifetime_total);
    pprt->lifetime_remaining = pprt->lifetime_total;

    // set the frame counters
    // make the particle display AT LEAST one frame, regardless of how many updates
    // it has or when someone requests for it to terminate
    pprt->frames_total = std::max(1, prt_anim_frames_updates);
    pprt->lifetime_remaining = pprt->lifetime_total;

    // Damage stuff
    range_to_pair(ppip->damage, &(pprt->damage));

    // Spawning data
    pprt->contspawn_timer = ppip->contspawn._delay;
    if (0 != pprt->contspawn_timer)
    {
        pprt->contspawn_timer = 1;
        if (_currentModule->getObjectHandler().exists(pprt->attachedto_ref))
        {
            pprt->contspawn_timer++; // Because attachment takes an update before it happens
        }
    }

    // the end-spawn data. determine the
    pprt->endspawn_amount = ppip->endspawn._amount;
    pprt->endspawn_facingadd = ppip->endspawn._facingAdd;
    pprt->endspawn_lpip = ppip->endspawn._lpip;

    // set up the particle transparency
    pprt->inst.alpha = 0xFF;
    switch (pprt->inst.type)
    {
    case SPRITE_SOLID: break;
    case SPRITE_ALPHA: pprt->inst.alpha = PRT_TRANS; break;
    case SPRITE_LIGHT: break;
    }

    // is the spawn location safe?
    fvec2_t nrm;
    if (0 == pprt->hit_wall(tmp_pos, nrm, nullptr, nullptr))
    {
        pprt->safe_pos = tmp_pos;
        pprt->safe_valid = true;
        pprt->safe_grid = pprt->getTile();
    }

    // get an initial value for the is_homing variable
    pprt->is_homing = ppip->homing && !_currentModule->getObjectHandler().exists(pprt->attachedto_ref);

    //enable or disable gravity
    pprt->no_gravity = ppip->ignore_gravity;

    // estimate some parameters for buoyancy and air resistance
    loc_spdlimit = ppip->spdlimit;

    {
        const float buoyancy_min = 0.0f;
        const float buoyancy_max = 2.0f * std::abs(Physics::g_environment.gravity);
        const float air_resistance_min = 0.0f;
        const float air_resistance_max = 1.0f;

        // find the buoyancy, assuming that the air_resistance of the particle
        // is equal to air_friction at standard gravity
        pprt->buoyancy = -loc_spdlimit * (1.0f - Physics::g_environment.airfriction) - Physics::g_environment.gravity;
        pprt->buoyancy = CLIP(pprt->buoyancy, buoyancy_min, buoyancy_max);

        // reduce the buoyancy if the particle falls
        if (loc_spdlimit > 0.0f) pprt->buoyancy *= 0.5f;

        // determine if there is any left-over air resistance
        if (std::abs(loc_spdlimit) > 0.0001f)
        {
            pprt->air_resistance = 1.0f - (pprt->buoyancy + Physics::g_environment.gravity) / -loc_spdlimit;
            pprt->air_resistance = CLIP(pprt->air_resistance, air_resistance_min, air_resistance_max);

            pprt->air_resistance /= Physics::g_environment.airfriction;
            pprt->air_resistance = CLIP(pprt->air_resistance, 0.0f, 1.0f);
        }
        else
        {
            pprt->air_resistance = 0.0f;
        }
    }

    pprt->endspawn_characterstate = SPAWNNOCHARACTER;

    pprt->set_size(ppip->size_base);

#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)

    // some code to track all allocated particles, where they came from, how long they are going to last,
    // what they are being used for...
    log_debug( "spawn_one_particle() - spawned a particle %d\n"
        "\tupdate == %d, last update == %d, frame == %d, minimum frame == %d\n"
        "\towner == %d(\"%s\")\n"
        "\tpip == %d(\"%s\")\n"
        "\t\t%s"
        "\tprofile == %d(\"%s\")\n"
        "\n",
        iprt,
        update_wld, pprt->lifetime, game_frame_all, pprt->safe_time,
        loc_chr_origin, _currentModule->getObjectHandler().exists( loc_chr_origin ) ? _currentModule->getObjectHandler().get(loc_chr_origin)->Name : "INVALID",
        pdata->ipip, ( NULL != ppip ) ? ppip->name : "INVALID", ( NULL != ppip ) ? ppip->comment : "",
        pdata->iprofile, ProfileSystem::get().isValidProfileID(pdata->iprofile) ? ProList.lst[pdata->iprofile].name : "INVALID");
#endif

    if (INVALID_CHR_REF != pprt->attachedto_ref)
    {
        prt_bundle_t prt_bdl(pprt);
		attach_one_particle(&prt_bdl);
    }

    // Sound effect
    prt_play_sound(iprt, ppip->soundspawn);

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t *prt_t::config_do_active()
{
    /* There is not inherent reason to change this state. */
    return this;
}

//--------------------------------------------------------------------------------------------
void prt_t::config_do_deinit()
{
    // Go to next state.
    this->state = Ego::Entity::State::Destructing;
    this->on = false;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD prt_t::hit_wall(fvec2_t& nrm, float *pressure, mesh_wall_data_t *data)
{
    if (!DEFINED_PPRT(this))
    {
        return EMPTY_BIT_FIELD;
    }
    if (!LOADED_PIP(this->pip_ref))
    {
        return EMPTY_BIT_FIELD;
    }
    return hit_wall(this->getPosition(), nrm, pressure, data);
}

BIT_FIELD prt_t::hit_wall(const fvec3_t& pos, fvec2_t& nrm, float *pressure, mesh_wall_data_t *data)
{
    if (!DEFINED_PPRT(this))
    {
        return EMPTY_BIT_FIELD;
    }
    if (!LOADED_PIP(this->pip_ref))
    {
        return EMPTY_BIT_FIELD;
    }
    std::shared_ptr<pip_t> ppip = PipStack.get_ptr(this->pip_ref);

    BIT_FIELD stoppedby = MAPFX_IMPASS;
    if (0 != ppip->bump_money) SET_BIT(stoppedby, MAPFX_WALL);

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    BIT_FIELD  result = ego_mesh_hit_wall(PMesh, pos, 0.0f, stoppedby, nrm, pressure, data);
    prt_stoppedby_tests += mesh_mpdfx_tests;
    prt_pressure_tests += mesh_pressure_tests;

    return result;
}

BIT_FIELD prt_t::test_wall(mesh_wall_data_t *data)
{
    if (!ACTIVE_PPRT(this))
    {
        return EMPTY_BIT_FIELD;
    }
    if (!LOADED_PIP(this->pip_ref))
    {
        return EMPTY_BIT_FIELD;
    }
    return test_wall(this->getPosition(), data);
}

BIT_FIELD prt_t::test_wall(const fvec3_t& pos, mesh_wall_data_t *data)
{
    if (!ACTIVE_PPRT(this))
    {
        return EMPTY_BIT_FIELD;
    }
    if (!LOADED_PIP(this->pip_ref))
    {
        return EMPTY_BIT_FIELD;
    }
    std::shared_ptr<pip_t> pip = PipStack.get_ptr(this->pip_ref);

    BIT_FIELD  stoppedby = MAPFX_IMPASS;
    if (0 != pip->bump_money) SET_BIT(stoppedby, MAPFX_WALL);

    // Do the wall test.
    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    BIT_FIELD result = ego_mesh_test_wall(PMesh, pos, 0.0f, stoppedby, data);
    prt_stoppedby_tests += mesh_mpdfx_tests;
    prt_pressure_tests += mesh_pressure_tests;

    return result;
}

//--------------------------------------------------------------------------------------------
void update_all_particles()
{
    /// @author BB
    /// @details main loop for updating particles. Do not use the
    ///               PRT_BEGIN_LOOP_* macro.
    ///               Converted all the update functions to the prt_run_config() paradigm.

    // Activate any particles might have been generated last update in an in-active state
    for (PRT_REF ref = 0; ref < ParticleHandler::get().getCount(); ++ref)
    {
        if (!ALLOCATED_PRT(ref)) continue;

		prt_bundle_t prt_bdl(ParticleHandler::get().get_ptr(ref));
        prt_bdl.update();
    }
}

//--------------------------------------------------------------------------------------------
void prt_t::set_level(const float level)
{
    float loc_height;

    if (!DISPLAY_PPRT(this)) return;

    this->enviro.level = level;

    loc_height = this->get_scale() * std::max(FP8_TO_FLOAT(this->size), this->offset[kZ] * 0.5f);

    this->enviro.adj_level = this->enviro.level;
    this->enviro.adj_floor = this->enviro.floor_level;

    this->enviro.adj_level += loc_height;
    this->enviro.adj_floor += loc_height;

    // set the zlerp after we have done everything to the particle's level we care to
    this->enviro.zlerp = (this->pos[kZ] - this->enviro.adj_level) / PLATTOLERANCE;
    this->enviro.zlerp = CLIP(this->enviro.zlerp, 0.0f, 1.0f);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_get_environment()
{
    float loc_level = 0.0f;

    if (NULL == this->_prt_ptr) return NULL;
    prt_t *loc_pprt = this->_prt_ptr;
    prt_environment_t *penviro = &(loc_pprt->enviro);

    //---- character "floor" level
    penviro->floor_level = ego_mesh_t::get_level(PMesh, PointWorld(loc_pprt->pos[kX], loc_pprt->pos[kY]));
    penviro->level = penviro->floor_level;

    //---- The actual level of the characer.
    //     Estimate platform attachment from whatever is in the onwhichplatform_ref variable from the
    //     last loop
    loc_level = penviro->floor_level;
    if (_currentModule->getObjectHandler().exists(loc_pprt->onwhichplatform_ref))
    {
        loc_level = std::max(penviro->floor_level, _currentModule->getObjectHandler().get(loc_pprt->onwhichplatform_ref)->getPosZ() + _currentModule->getObjectHandler().get(loc_pprt->onwhichplatform_ref)->chr_min_cv.maxs[OCT_Z]);
    }
    loc_pprt->set_level(loc_level);

    //---- the "twist" of the floor
    penviro->twist = TWIST_FLAT;
    TileIndex itile = TileIndex::Invalid;
    if (_currentModule->getObjectHandler().exists(loc_pprt->onwhichplatform_ref))
    {
        // this only works for 1 level of attachment
        itile = _currentModule->getObjectHandler().get(loc_pprt->onwhichplatform_ref)->getTile();
    }
    else
    {
        itile = loc_pprt->getTile();
    }

    penviro->twist = ego_mesh_get_twist(PMesh, itile);

    // the "watery-ness" of whatever water might be here
    penviro->is_watery = water.is_water && penviro->inwater;
    penviro->is_slippy = !penviro->is_watery && (0 != ego_mesh_t::test_fx(PMesh, loc_pprt->getTile(), MAPFX_SLIPPY));

    //---- traction
    penviro->traction = 1.0f;
    if (loc_pprt->is_homing)
    {
        // any traction factor here
        /* traction = ??; */
    }
    else if (_currentModule->getObjectHandler().exists(loc_pprt->onwhichplatform_ref))
    {
        // in case the platform is tilted
        // unfortunately platforms are attached in the collision section
        // which occurs after the movement section.

        fvec3_t platform_up;

        chr_getMatUp(_currentModule->getObjectHandler().get(loc_pprt->onwhichplatform_ref), platform_up);
        platform_up.normalize();

        penviro->traction = std::abs(platform_up[kZ]) * (1.0f - penviro->zlerp) + 0.25f * penviro->zlerp;

        if (penviro->is_slippy)
        {
            penviro->traction /= Physics::g_environment.hillslide * (1.0f - penviro->zlerp) + 1.0f * penviro->zlerp;
        }
    }
    else if (ego_mesh_t::grid_is_valid(PMesh, loc_pprt->getTile()))
    {
        penviro->traction = std::abs(map_twist_nrm[penviro->twist][kZ]) * (1.0f - penviro->zlerp) + 0.25f * penviro->zlerp;

        if (penviro->is_slippy)
        {
            penviro->traction /= Physics::g_environment.hillslide * (1.0f - penviro->zlerp) + 1.0f * penviro->zlerp;
        }
    }

    //---- the friction of the fluid we are in
    if (penviro->is_watery)
    {
        penviro->fluid_friction_vrt = Physics::g_environment.waterfriction;
        penviro->fluid_friction_hrz = Physics::g_environment.waterfriction;
    }
    else
    {
        penviro->fluid_friction_hrz = penviro->air_friction;       // like real-life air friction
        penviro->fluid_friction_vrt = penviro->air_friction;
    }

    //---- friction
    penviro->friction_hrz = 1.0f;
    if (!loc_pprt->is_homing)
    {
        // Make the characters slide
        float temp_friction_xy = Physics::g_environment.noslipfriction;
        if (ego_mesh_t::grid_is_valid(PMesh, loc_pprt->getTile()) && penviro->is_slippy)
        {
            // It's slippy all right...
            temp_friction_xy = Physics::g_environment.slippyfriction;
        }

        penviro->friction_hrz = penviro->zlerp * 1.0f + (1.0f - penviro->zlerp) * temp_friction_xy;
    }

    return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_do_fluid_friction()
{
    fvec3_t fluid_acc;

    prt_t *loc_pprt = this->_prt_ptr;
	std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;
    prt_environment_t *loc_penviro = &(loc_pprt->enviro);

    // if the particle is a homing-type particle, ignore friction
	if (loc_ppip->homing) return this;

    // Light isn't affected by fluid velocity
	if (SPRITE_LIGHT == loc_pprt->type) return this;

    // assume no acceleration
    fluid_acc = fvec3_t::zero();

    // get the speed relative to the fluid
    if (loc_pprt->enviro.inwater)
    {
        fluid_acc = Physics::g_environment.waterspeed - loc_pprt->vel;
    }
    else
    {
        fluid_acc = Physics::g_environment.windspeed - loc_pprt->vel;
    }

    // get the fluid friction
    if (loc_pprt->buoyancy > 0.0f)
    {
        // this is a buoyant particle, like smoke

        float loc_buoyancy_friction = Physics::g_environment.airfriction * loc_pprt->air_resistance;

        if (loc_pprt->enviro.inwater)
        {
            float water_friction = std::pow(loc_buoyancy_friction, 2.0f);

            fluid_acc *= 1.0f - water_friction;
        }
        else
        {
            fluid_acc *= 1.0f - loc_buoyancy_friction;
        }
    }
    else
    {
        // this is a normal particle, like a mushroom

        if (loc_pprt->enviro.inwater)
        {
            fluid_acc[kX] *= 1.0f - loc_penviro->fluid_friction_hrz * loc_pprt->air_resistance;
            fluid_acc[kY] *= 1.0f - loc_penviro->fluid_friction_hrz * loc_pprt->air_resistance;
            fluid_acc[kZ] *= 1.0f - loc_penviro->fluid_friction_vrt * loc_pprt->air_resistance;
        }
        else
        {
            fluid_acc[kX] *= 1.0f - loc_penviro->fluid_friction_hrz * loc_pprt->air_resistance;
            fluid_acc[kY] *= 1.0f - loc_penviro->fluid_friction_hrz * loc_pprt->air_resistance;
            fluid_acc[kZ] *= 1.0f - loc_penviro->fluid_friction_vrt * loc_pprt->air_resistance;
        }
    }

    // apply the fluid friction
    loc_pprt->vel += fluid_acc;

	return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_do_floor_friction()
{
    float temp_friction_xy;
    fvec3_t   vup, fric, fric_floor;
    fvec3_t   floor_acc;

    if (NULL == this->_prt_ptr) return NULL;
    prt_t *loc_pprt = this->_prt_ptr;
    prt_environment_t *penviro = &(loc_pprt->enviro);

    // if the particle is homing in on something, ignore friction
    if (loc_pprt->is_homing) return this;

    // limit floor friction effects to solid objects
    if (SPRITE_SOLID != loc_pprt->type)  return this;

    // figure out the acceleration due to the current "floor"
    floor_acc[kX] = floor_acc[kY] = floor_acc[kZ] = 0.0f;
    temp_friction_xy = 1.0f;
    if (_currentModule->getObjectHandler().exists(loc_pprt->onwhichplatform_ref))
    {
        Object * pplat = _currentModule->getObjectHandler().get(loc_pprt->onwhichplatform_ref);

        temp_friction_xy = PLATFORM_STICKINESS;

        floor_acc = pplat->vel - pplat->vel_old;

        chr_getMatUp(pplat, vup);
    }
    else
    {
        temp_friction_xy = 0.5f;
        floor_acc = -loc_pprt->vel;

        if (TWIST_FLAT == penviro->twist)
        {
            vup = fvec3_t(0, 0, 1);
        }
        else
        {
            vup = map_twist_nrm[penviro->twist];
        }
    }

    // the first guess about the floor friction
    fric_floor = floor_acc * (1.0f - penviro->zlerp) * (1.0f - temp_friction_xy) * penviro->traction;

    // the total "friction" due to the floor
    fric = fric_floor + penviro->acc;

    //---- limit the friction to whatever is horizontal to the mesh
    if (std::abs(vup[kZ]) > 0.9999f)
    {
        floor_acc[kZ] = 0.0f;
        fric[kZ] = 0.0f;
    }
    else
    {
        float ftmp = floor_acc.dot(vup);
        floor_acc -= vup * ftmp;

        ftmp = fric.dot(vup);
        fric -= vup * ftmp;
    }

    // test to see if the player has any more friction left?
    penviro->is_slipping = fric.length_abs() > penviro->friction_hrz;

    if (penviro->is_slipping)
    {
        penviro->traction *= 0.5f;
        temp_friction_xy = std::sqrt(temp_friction_xy);

        fric_floor = floor_acc * ((1.0f - penviro->zlerp) * (1.0f - temp_friction_xy) * penviro->traction);
        float ftmp = fric_floor.dot(vup);
        fric_floor -= vup * ftmp;
    }

    // Apply the floor friction
    loc_pprt->vel += fric_floor*0.25f;

    return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_do_homing()
{
    int       ival;
    float     vlen, min_length, uncertainty;
    fvec3_t   vdiff, vdither;

    if (NULL == this->_prt_ptr) return NULL;
	prt_t *loc_pprt = this->_prt_ptr;
	std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;

    // is the particle a homing type?
	if (!loc_ppip->homing) return this;

    // the particle update function is supposed to turn homing off if the particle looses its target
	if (!loc_pprt->is_homing) return this;

    // the loc_pprt->is_homing variable is supposed to track the following, but it could have lost synch by this point
	if (_currentModule->getObjectHandler().exists(loc_pprt->attachedto_ref) || !_currentModule->getObjectHandler().exists(loc_pprt->target_ref)) return this;

    // grab a pointer to the target
    Object *ptarget = _currentModule->getObjectHandler().get(loc_pprt->target_ref);

    vdiff = ptarget->getPosition() - loc_pprt->getPosition();
    vdiff[kZ] += ptarget->bump.height * 0.5f;

    min_length = 2 * 5 * 256 * (_currentModule->getObjectHandler().get(loc_pprt->owner_ref)->wisdom / (float)PERFECTBIG);

    // make a little incertainty about the target
    uncertainty = 256.0f * (1.0f - _currentModule->getObjectHandler().get(loc_pprt->owner_ref)->intelligence / (float)PERFECTBIG);

    ival = Random::next(std::numeric_limits<uint16_t>::max());
    vdither[kX] = (((float)ival / 0x8000) - 1.0f)  * uncertainty;

    ival = Random::next(std::numeric_limits<uint16_t>::max());
    vdither[kY] = (((float)ival / 0x8000) - 1.0f)  * uncertainty;

    ival = Random::next(std::numeric_limits<uint16_t>::max());
    vdither[kZ] = (((float)ival / 0x8000) - 1.0f)  * uncertainty;

    // take away any dithering along the direction of motion of the particle
    vlen = loc_pprt->vel.length_2();
    if (vlen > 0.0f)
    {
        float vdot = vdither.dot(loc_pprt->vel) / vlen;

        vdither -= vdiff * (vdot/vlen);
    }

    // add in the dithering
    vdiff += vdither;

    // Make sure that vdiff doesn't ever get too small.
    // That just makes the particle slooooowww down when it approaches the target.
    // Do a real kludge here. this should be a lot faster than a square root, but ...
    vlen = vdiff.length_abs();
    if (vlen > FLT_EPSILON)
    {
        float factor = min_length / vlen;

        vdiff *= factor;
    }

    loc_pprt->vel = (loc_pprt->vel + vdiff * loc_ppip->homingaccel) * loc_ppip->homingfriction;

	return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::updateParticleSimpleGravity()
{
    //Only do gravity for solid particles
    if (!this->_prt_ptr->no_gravity && this->_prt_ptr->type == SPRITE_SOLID && !this->_prt_ptr->is_homing  && !_currentModule->getObjectHandler().exists(this->_prt_ptr->attachedto_ref))
    {
        this->_prt_ptr->vel[kZ] += Physics::g_environment.gravity 
                                  * Physics::g_environment.airfriction;
    }
    return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_do_z_motion()
{
    float loc_zlerp, tmp_buoyancy, loc_buoyancy;

    fvec3_t z_motion_acc;

    if (NULL == this->_prt_ptr) return NULL;
    prt_t *loc_pprt = this->_prt_ptr;
    prt_environment_t *penviro = &(loc_pprt->enviro);

    /// @note ZF@> We really can't do gravity for Light! A lot of magical effects and attacks in the game depend on being able
    ///            to move forward in a straight line without being dragged down into the dust!
    /// @note BB@> however, the fireball particle is light, and without gravity it will never bounce on the
    ///            ground as it is supposed to
    /// @note ZF@> I will try to fix this by adding a new  no_gravity expansion for particles
    if (loc_pprt->no_gravity || /* loc_pprt->type == SPRITE_LIGHT || */ loc_pprt->is_homing || _currentModule->getObjectHandler().exists(loc_pprt->attachedto_ref)) return this;

    loc_zlerp = CLIP(penviro->zlerp, 0.0f, 1.0f);

    z_motion_acc = fvec3_t::zero();

    // in higher gravity environments, buoyancy is larger
    tmp_buoyancy = loc_pprt->buoyancy * std::abs(Physics::g_environment.gravity);

    // handle bouyancy near the ground
    if (loc_zlerp >= 1.0f)
    {
        loc_buoyancy = tmp_buoyancy;
    }
    else if (loc_zlerp <= 0.0f)
    {
        loc_buoyancy = 0.0f;
    }
    else
    {
        // Do particle buoyancy. This is kinda BS the way it is calculated
        loc_buoyancy = 0.0f;
        if (tmp_buoyancy + Physics::g_environment.gravity < 0.0f)
        {
            // the particle cannot hold itself up

            // loc_zacc = ( tmp_buoyancy + gravity ) * loc_zlerp;
            loc_buoyancy = tmp_buoyancy * loc_zlerp;
        }
        else
        {
            // the particle is floating, make the normal force cancel gravity, only

            // loc_zacc = tmp_buoyancy + gravity * loc_zlerp;
            loc_buoyancy = tmp_buoyancy;
        }
    }

    // do the buoyancy calculation
    z_motion_acc[kZ] += loc_buoyancy;

    // do gravity
    if (penviro->is_slippy && (TWIST_FLAT != penviro->twist) && loc_zlerp < 1.0f)
    {
        // Hills make particles slide:

        // Gravity parallel to the mesh.
        fvec3_t gpara = map_twist_vel[penviro->twist];

        // Gravity perpendicular to the mesh.
        fvec3_t gperp = -gpara;
        gperp[kZ] += Physics::g_environment.gravity;

        z_motion_acc += gpara * (1.0f - loc_zlerp) + gperp * loc_zlerp;
    }
    else
    {
        z_motion_acc[kZ] += loc_zlerp * Physics::g_environment.gravity;
    }

    loc_pprt->vel += z_motion_acc;

    return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_integrate_motion_attached()
{
    float loc_level;
    bool touch_a_floor, hit_a_wall, needs_test;
    fvec3_t nrm_total;
    fvec3_t tmp_pos;

    if (NULL == this->_prt_ptr) return NULL;
    prt_t *loc_pprt = this->_prt_ptr;
    PRT_REF loc_iprt = this->_prt_ref;
    std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;
    prt_environment_t *penviro = &(loc_pprt->enviro);

    // if the particle is not still in "display mode" there is no point in going on
    if (!DISPLAY_PPRT(loc_pprt)) return this;

    // capture the particle position
    tmp_pos = loc_pprt->getPosition();

    // only deal with attached particles
    if (INVALID_CHR_REF == loc_pprt->attachedto_ref) return this;

    touch_a_floor = false;
    hit_a_wall = false;
    nrm_total = fvec3_t::zero();

    loc_level = penviro->adj_level;

    // Move the particle
    if (tmp_pos[kZ] < loc_level)
    {
        touch_a_floor = true;
    }

    if (touch_a_floor)
    {
        // Play the sound for hitting the floor [FSND]
        prt_play_sound(loc_iprt, loc_ppip->end_sound_floor);
    }

    // handle the collision
    if (touch_a_floor && loc_ppip->end_ground)
    {
        end_one_particle_in_game(this->_prt_ref);
        return nullptr;
    }

    // interaction with the mesh walls
    hit_a_wall = false;
    needs_test = false;
    if (std::abs(loc_pprt->vel[kX]) + std::abs(loc_pprt->vel[kY]) > 0.0f)
    {
        mesh_wall_data_t wdata;

        if (EMPTY_BIT_FIELD != loc_pprt->test_wall(tmp_pos, &wdata))
        {
            fvec2_t nrm;
            float   pressure;

            // how is the character hitting the wall?
            BIT_FIELD hit_bits = loc_pprt->hit_wall(tmp_pos, nrm, &pressure, &wdata);

            if (0 != hit_bits)
            {
                hit_a_wall = true;
            }
        }
    }

    // handle the sounds
    if (hit_a_wall)
    {
        // Play the sound for hitting the floor [FSND]
        prt_play_sound(loc_iprt, loc_ppip->end_sound_wall);
    }

    // handle the collision
    if (hit_a_wall && (loc_ppip->end_wall || loc_ppip->end_bump))
    {
        end_one_particle_in_game(this->_prt_ref);
        return nullptr;
    }

    loc_pprt->setPosition(tmp_pos);

    return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_integrate_motion()
{
    float ftmp, loc_level;
    bool hit_a_floor, hit_a_wall, needs_test;
    bool touch_a_floor, touch_a_wall;
    fvec3_t nrm_total;
    fvec3_t tmp_pos;

    if (NULL == this->_prt_ptr) return NULL;
    prt_t *loc_pprt = this->_prt_ptr;
    PRT_REF loc_iprt = this->_prt_ref;
    std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;
    prt_environment_t *penviro = &(loc_pprt->enviro);

    // if the particle is not still in "display mode" there is no point in going on
    if (!DISPLAY_PPRT(loc_pprt)) return this;

    // capture the position
    tmp_pos = loc_pprt->getPosition();

    // no point in doing this if the particle thinks it's attached
    if (INVALID_CHR_REF != loc_pprt->attachedto_ref)
    {
        return this->move_one_particle_integrate_motion_attached();
    }

    hit_a_floor = false;
    hit_a_wall = false;
    touch_a_floor = false;
    touch_a_wall = false;
    nrm_total[kX] = nrm_total[kY] = nrm_total[kZ] = 0.0f;

    loc_level = penviro->adj_level;

    // Move the particle
    ftmp = tmp_pos[kZ];
    tmp_pos[kZ] += loc_pprt->vel[kZ];
    LOG_NAN(tmp_pos[kZ]);
    if (tmp_pos[kZ] < loc_level)
    {
        fvec3_t floor_nrm = fvec3_t(0.0f, 0.0f, 1.0f);
        float vel_dot;
        fvec3_t vel_perp, vel_para;

        touch_a_floor = true;

        uint8_t tmp_twist = cartman_get_fan_twist(PMesh, loc_pprt->getTile());

        if (TWIST_FLAT != tmp_twist)
        {
            floor_nrm = map_twist_nrm[penviro->twist];
        }

        vel_dot = floor_nrm.dot(loc_pprt->vel);
        if (0.0f == vel_dot)
        {
            vel_perp = fvec3_t::zero();
            vel_para = loc_pprt->vel;
        }
        else
        {
            vel_perp = floor_nrm * vel_dot;
            vel_para = loc_pprt->vel - vel_perp;
        }

        if (loc_pprt->vel[kZ] < -STOPBOUNCINGPART)
        {
            // the particle will bounce
            nrm_total += floor_nrm;

            // take reflection in the floor into account when computing the new level
            tmp_pos[kZ] = loc_level + (loc_level - ftmp) * loc_ppip->dampen + 0.1f;

            loc_pprt->vel[kZ] = -loc_pprt->vel[kZ];

            hit_a_floor = true;
        }
        else if (vel_dot > 0.0f)
        {
            // the particle is not bouncing, it is just at the wrong height
            tmp_pos[kZ] = loc_level + 0.1f;
        }
        else
        {
            // the particle is in the "stop bouncing zone"
            tmp_pos[kZ] = loc_level + 0.1f;
            //loc_pprt->vel = vel_para;
        }
    }

    // handle the sounds
    if (hit_a_floor)
    {
        // Play the sound for hitting the floor [FSND]
        prt_play_sound(loc_iprt, loc_ppip->end_sound_floor);
    }

    // handle the collision
    if (touch_a_floor && loc_ppip->end_ground)
    {
        end_one_particle_in_game(this->_prt_ref);
        return NULL;
    }

    // interaction with the mesh walls
    hit_a_wall = false;
    needs_test = false;
    if (std::abs(loc_pprt->vel[kX]) + std::abs(loc_pprt->vel[kY]) > 0.0f)
    {
        mesh_wall_data_t wdata;

        tmp_pos[kX] += loc_pprt->vel[kX];
        tmp_pos[kY] += loc_pprt->vel[kY];

        //Hitting a wall?
        if (EMPTY_BIT_FIELD != loc_pprt->test_wall(tmp_pos, &wdata))
        {
            fvec2_t nrm;
            float   pressure;

            // how is the character hitting the wall?
            if (EMPTY_BIT_FIELD != loc_pprt->hit_wall(tmp_pos, nrm, &pressure, &wdata))
            {
                touch_a_wall = true;

                nrm_total[kX] += nrm[XX];
                nrm_total[kY] += nrm[YY];

                hit_a_wall = (fvec2_t(loc_pprt->vel[kX], loc_pprt->vel[kY]).dot(nrm) < 0.0f);
            }
        }
    }

    // handle the sounds
    if (hit_a_wall)
    {
        // Play the sound for hitting the wall [WSND]
        prt_play_sound(loc_iprt, loc_ppip->end_sound_wall);
    }

    // handle the collision
    if (touch_a_wall && loc_ppip->end_wall)
    {
        end_one_particle_in_game(this->_prt_ref);
        return nullptr;
    }

    // do the reflections off the walls and floors
    if (hit_a_wall || hit_a_floor)
    {

        if ((hit_a_wall && (loc_pprt->vel[kX] * nrm_total[kX] + loc_pprt->vel[kY] * nrm_total[kY]) < 0.0f) ||
            (hit_a_floor && (loc_pprt->vel[kZ] * nrm_total[kZ]) < 0.0f))
        {
            float vdot;
            fvec3_t   vpara, vperp;

            nrm_total.normalize();

            vdot = nrm_total.dot(loc_pprt->vel);

            vperp = nrm_total * vdot;

            vpara = loc_pprt->vel - vperp;

            // we can use the impulse to determine how much velocity to kill in the parallel direction
            // imp = vperp * (1.0f + loc_ppip->dampen);

            // do the reflection
            vperp *= -loc_ppip->dampen;

            // fake the friction, for now
            if (0.0f != nrm_total[kY] || 0.0f != nrm_total[kZ])
            {
                vpara[kX] *= loc_ppip->dampen;
            }

            if (0.0f != nrm_total[kX] || 0.0f != nrm_total[kZ])
            {
                vpara[kY] *= loc_ppip->dampen;
            }

            if (0.0f != nrm_total[kX] || 0.0f != nrm_total[kY])
            {
                vpara[kZ] *= loc_ppip->dampen;
            }

            // add the components back together
            loc_pprt->vel = vpara + vperp;
        }

        if (nrm_total[kZ] != 0.0f && loc_pprt->vel[kZ] < STOPBOUNCINGPART)
        {
            // this is the very last bounce
            loc_pprt->vel[kZ] = 0.0f;
            tmp_pos[kZ] = loc_level + 0.0001f;
        }

        if (hit_a_wall)
        {
            float fx, fy;

            // fix the facing
            facing_to_vec(loc_pprt->facing, &fx, &fy);

            if (0.0f != nrm_total[kX])
            {
                fx *= -1;
            }

            if (0.0f != nrm_total[kY])
            {
                fy *= -1;
            }

            loc_pprt->facing = vec_to_facing(fx, fy);
        }
    }

    //Don't fall in pits...
    if (loc_pprt->is_homing) tmp_pos[kZ] = std::max(tmp_pos[kZ], 0.0f);

    //Rotate particle to the direction we are moving
    if (loc_ppip->rotatetoface)
    {
        if (std::abs(loc_pprt->vel[kX]) + std::abs(loc_pprt->vel[kY]) > FLT_EPSILON)
        {
            // use velocity to find the angle
            loc_pprt->facing = vec_to_facing(loc_pprt->vel[kX], loc_pprt->vel[kY]);
        }
        else if (_currentModule->getObjectHandler().exists(loc_pprt->target_ref))
        {
            Object *ptarget = _currentModule->getObjectHandler().get(loc_pprt->target_ref);

            // face your target
            loc_pprt->facing = vec_to_facing(ptarget->getPosX() - tmp_pos[kX], ptarget->getPosY() - tmp_pos[kY]);
        }
    }

    loc_pprt->setPosition(tmp_pos);

    return this;
}

//--------------------------------------------------------------------------------------------
bool prt_bundle_t::move_one_particle()
{
    if (NULL == this->_prt_ptr) return false;
	prt_t *loc_pprt = this->_prt_ptr;
    prt_environment_t *penviro = &(loc_pprt->enviro);

    if (!DISPLAY_PPRT(loc_pprt)) return false;

    // if the particle is hidden it is frozen in time. do nothing.
    if (loc_pprt->is_hidden) return false;

    // save the acceleration from the last time-step
    penviro->acc = loc_pprt->vel - loc_pprt->vel_old;

    // determine the actual velocity for attached particles
    if (_currentModule->getObjectHandler().exists(loc_pprt->attachedto_ref))
    {
        loc_pprt->vel = loc_pprt->getPosition() - loc_pprt->pos_old;
    }

    // Particle's old location
    loc_pprt->pos_old = loc_pprt->getPosition();
    loc_pprt->vel_old = loc_pprt->vel;

    // what is the local environment like?
	if (!this->move_one_particle_get_environment()) return false;
	if (!this->_prt_ptr) return false;

    //ZF> Disabled, this doesn't really work yet
    // wind, current, and other fluid friction effects
    //if (!this->move_one_particle_do_fluid_friction()) return false;
    //if (!this->prt_ptr) return false;

    // do friction with the floor before voluntary motion
	if (!this->move_one_particle_do_floor_friction()) return false;
    if (!this->_prt_ptr) return false;

	if (!this->move_one_particle_do_homing()) return false;
    if (!this->_prt_ptr) return false;

    //ZF> Dirty hack using VERY simple gravity calculation
	//MH> Unify this.
	if (!this->updateParticleSimpleGravity()) return false;
    //if (!this->move_one_particle_do_z_motion()) return false;
    if (!this->_prt_ptr) return false;

	if (!this->move_one_particle_integrate_motion()) return false;
    if (!this->_prt_ptr) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
void move_all_particles()
{
    /// @author ZZ
    /// @details This is the particle physics function

    prt_stoppedby_tests = 0;

    // move every particle
    PRT_BEGIN_LOOP_DISPLAY(cnt, prt_bdl)
    {
        // prime the environment
        prt_bdl._prt_ptr->enviro.air_friction = Physics::g_environment.airfriction;
        prt_bdl._prt_ptr->enviro.ice_friction = Physics::g_environment.icefriction;

        prt_bdl.move_one_particle();
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
int spawn_bump_particles(const CHR_REF character, const PRT_REF particle)
{
    /// @author ZZ
    /// @details This function is for catching characters on fire and such

    int      cnt, bs_count;
    float    x, y, z;
    FACING_T facing;
    int      amount;
    FACING_T direction;
    float    fsin, fcos;

    if (!INGAME_PRT(particle)) return 0;
    prt_t *pprt = ParticleHandler::get().get_ptr(particle);

    if (!LOADED_PIP(pprt->pip_ref)) return 0;
    std::shared_ptr<pip_t> ppip = PipStack.get_ptr(pprt->pip_ref);

    // no point in going on, is there?
    if (0 == ppip->bumpspawn._amount && !ppip->spawnenchant) return 0;
    amount = ppip->bumpspawn._amount;

    if (!_currentModule->getObjectHandler().exists(character)) return 0;
    Object *pchr = _currentModule->getObjectHandler().get(character);

    mad_t *pmad = chr_get_pmad(character);
    if (NULL == pmad) return 0;

    const std::shared_ptr<ObjectProfile> &profile = ProfileSystem::get().getProfile(pchr->profile_ref);

    bs_count = 0;

    // Only damage if hitting from proper direction
    direction = vec_to_facing(pprt->vel[kX], pprt->vel[kY]);
    direction = ATK_BEHIND + (pchr->ori.facing_z - direction);

    // Check that direction
    if (!is_invictus_direction(direction, character, ppip->damfx))
    {
        IPair loc_rand(0, 100);
        int damage_resistance;

        // Spawn new enchantments
        if (ppip->spawnenchant)
        {
            EnchantHandler::get().spawn_one_enchant(pprt->owner_ref, character, INVALID_CHR_REF, INVALID_ENC_REF, pprt->profile_ref);
        }

        // Spawn particles - this has been modded to maximize the visual effect
        // on a given target. It is not the most optimal solution for lots of particles
        // spawning. Thst would probably be to make the distance calculations and then
        // to quicksort the list and choose the n closest points.
        //
        // however, it seems that the bump particles in game rarely attach more than
        // one bump particle

        //check if we resisted the attack, we could resist some of the particles or none
        damage_resistance = (pprt->damagetype >= DAMAGE_COUNT) ? 0 : pchr->damage_resistance[pprt->damagetype] * 100;
        for (cnt = 0; cnt < amount; cnt++)
        {
            if (generate_irand_pair(loc_rand) <= damage_resistance) amount--;
        }

        if (amount > 0 && !profile->hasResistBumpSpawn() && !pchr->invictus)
        {
            int grip_verts, vertices;
            int slot_count;

            slot_count = 0;
            if (profile->isSlotValid(SLOT_LEFT)) slot_count++;
            if (profile->isSlotValid(SLOT_RIGHT)) slot_count++;

            if (0 == slot_count)
            {
                grip_verts = 1;  // always at least 1?
            }
            else
            {
                grip_verts = GRIP_VERTS * slot_count;
            }

            vertices = (int)pchr->inst.vrt_count - (int)grip_verts;
            vertices = std::max(0, vertices);

            if (vertices != 0)
            {
                PRT_REF *vertex_occupied;
                float   *vertex_distance;
                float    dist;
                TURN_T   turn;

                vertex_occupied = EGOBOO_NEW_ARY(PRT_REF, vertices);
                vertex_distance = EGOBOO_NEW_ARY(float, vertices);

                // this could be done more easily with a quicksort....
                // but I guess it doesn't happen all the time
                dist = (pprt->getPosition() - pchr->getPosition()).length_abs();

                // clear the occupied list
                z = pprt->pos[kZ] - pchr->getPosition()[kZ];
                facing = pprt->facing - pchr->ori.facing_z;
                turn = TO_TURN(facing);
                fsin = turntosin[turn];
                fcos = turntocos[turn];
                x = dist * fcos;
                y = dist * fsin;

                // prepare the array values
                for (cnt = 0; cnt < vertices; cnt++)
                {
                    dist = std::abs(x - pchr->inst.vrt_lst[vertices - cnt - 1].pos[XX])
                         + std::abs(y - pchr->inst.vrt_lst[vertices - cnt - 1].pos[YY])
                         + std::abs(z - pchr->inst.vrt_lst[vertices - cnt - 1].pos[ZZ]);

                    vertex_distance[cnt] = dist;
                    vertex_occupied[cnt] = INVALID_PRT_REF;
                }

                // determine if some of the vertex sites are already occupied
                PRT_BEGIN_LOOP_ACTIVE(iprt, prt_bdl)
                {
                    if (character != prt_bdl._prt_ptr->attachedto_ref) continue;

                    if (prt_bdl._prt_ptr->attachedto_vrt_off < vertices)
                    {
                        vertex_occupied[prt_bdl._prt_ptr->attachedto_vrt_off] = prt_bdl._prt_ref;
                    }
                }
                PRT_END_LOOP()

                    // Find best vertices to attach the particles to
                    for (cnt = 0; cnt < amount; cnt++)
                    {
                        PRT_REF bs_part;
                        Uint32  bestdistance;
                        int     bestvertex;

                        bestvertex = 0;
                        bestdistance = 0xFFFFFFFF;         //Really high number

                        for (cnt = 0; cnt < vertices; cnt++)
                        {
                            if (INVALID_PRT_REF != vertex_occupied[cnt])
                                continue;

                            if (vertex_distance[cnt] < bestdistance)
                            {
                                bestdistance = vertex_distance[cnt];
                                bestvertex = cnt;
                            }
                        }

                        bs_part = ParticleHandler::get().spawn_one_particle(pchr->getPosition(), pchr->ori.facing_z, pprt->profile_ref, ppip->bumpspawn._lpip,
                                                                            character, bestvertex + 1, pprt->team, pprt->owner_ref, particle, cnt, character);

                        if (DEFINED_PRT(bs_part))
                        {
                            vertex_occupied[bestvertex] = bs_part;
                            ParticleHandler::get().get_ptr(bs_part)->is_bumpspawn = true;
                            bs_count++;
                        }
                    }
                //}
                //else
                //{
                //    // Multiple particles are attached to character
                //    for ( cnt = 0; cnt < amount; cnt++ )
                //    {
                //        int irand = Random::next(std::numeric_limits<uint16_t>::max());

                //        bs_part = spawn_one_particle( pchr->pos, pchr->ori.facing_z, pprt->profile_ref, ppip->bumpspawn_lpip.get(),
                //                                      character, irand % vertices, pprt->team, pprt->owner_ref, particle, cnt, character );

                //        if( DEFINED_PRT(bs_part) )
                //        {
                //            PrtList.lst[bs_part].is_bumpspawn = true;
                //            bs_count++;
                //        }
                //    }
                //}

                EGOBOO_DELETE_ARY(vertex_occupied);
                EGOBOO_DELETE_ARY(vertex_distance);
            }
        }
    }

    return bs_count;
}

//--------------------------------------------------------------------------------------------
bool prt_is_over_water(const PRT_REF ref)
{
    /// @author ZZ
    /// @details This function returns true if the particle is over a water tile

    if (!ALLOCATED_PRT(ref)) return false;

    prt_t *prt = ParticleHandler::get().get_ptr(ref);
    TileIndex fan = ego_mesh_t::get_grid(PMesh, PointWorld(prt->pos[kX], prt->pos[kY]));
    if (ego_mesh_t::grid_is_valid(PMesh, fan))
    {
        if (0 != ego_mesh_t::test_fx(PMesh, fan, MAPFX_WATER))  return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------
void prt_t::requestTerminate()
{
    if (!ALLOCATED_PPRT(this) || TERMINATED_PPRT(this))
    {
        return;
    }

    bool is_visible = this->size > 0 && !this->is_hidden && this->inst.alpha > 0.0f;

    if (is_visible && 0 == this->frame_count)
    {
        // turn the particle into a ghost
        this->is_ghost = true;
    }
    else
    {
        // the particle has already been seen or is not visible, so just
        // terminate it, as normal
		this->POBJ_REQUEST_TERMINATE();
    }
}

//--------------------------------------------------------------------------------------------

bool prt_t::update_safe_raw(prt_t * pprt)
{
    bool retval = false;

    BIT_FIELD hit_a_wall;
    float  pressure;

    if (!ALLOCATED_PPRT(pprt)) return false;
    fvec2_t nrm;
    hit_a_wall = pprt->hit_wall(nrm, &pressure, nullptr);
    if ((0 == hit_a_wall) && (0.0f == pressure))
    {
        pprt->safe_valid = true;
        pprt->safe_pos = pprt->getPosition();
        pprt->safe_time = update_wld;
        pprt->safe_grid = ego_mesh_t::get_grid(PMesh, PointWorld(pprt->pos[kX], pprt->pos[kY])).getI();

        retval = true;
    }

    return retval;
}

bool prt_t::update_safe(prt_t * pprt, bool force)
{
    bool retval = false;
    bool needs_update = false;

    if (!ALLOCATED_PPRT(pprt)) return false;

    if (force || !pprt->safe_valid)
    {
        needs_update = true;
    }
    else
    {
        TileIndex new_grid = ego_mesh_t::get_grid(PMesh, PointWorld(pprt->pos[kX], pprt->pos[kY]));

        if (TileIndex::Invalid == new_grid)
        {
            if (std::abs(pprt->pos[kX] - pprt->safe_pos[kX]) > GRID_FSIZE ||
                std::abs(pprt->pos[kY] - pprt->safe_pos[kY]) > GRID_FSIZE)
            {
                needs_update = true;
            }
        }
        else if (new_grid != pprt->safe_grid)
        {
            needs_update = true;
        }
    }

    if (needs_update)
    {
        retval = prt_t::update_safe_raw(pprt);
    }

    return retval;
}

PIP_REF prt_t::get_ipip() const
{
    if (!DEFINED_PPRT(this)) return INVALID_PIP_REF;
    if (!LOADED_PIP(this->pip_ref)) return INVALID_PIP_REF;
    return this->pip_ref;
}

bool prt_t::setPosition(const fvec3_t& position)
{
    if (!ALLOCATED_PPRT(this))
    {
        return false;
    }
    EGO_DEBUG_VALIDATE(position);
    /// Has our position changed?
    if (position != this->pos)
    {
        this->pos = position;

        this->_tile = ego_mesh_t::get_grid(PMesh, PointWorld(this->pos[kX], this->pos[kY])).getI();
        this->_block = ego_mesh_t::get_block(PMesh, PointWorld(this->pos[kX], this->pos[kY])).getI();

        // Update whether the current particle position is safe.
        prt_t::update_safe(this, false);

        // Update the breadcrumb list (does not exist for particles).
        /*
        prt_update_breadcrumb(this, false);
        */

        return true;
    }
    return false;
}

std::shared_ptr<pip_t> prt_t::get_ppip() const
{
    PIP_REF pipRef = get_ipip();
    if (pipRef == INVALID_PIP_REF) return nullptr;
    return PipStack.get_ptr(pipRef);
}

float prt_t::get_scale() const
{
    float scale = 0.25f;

    if (!DEFINED_PPRT(this)) return scale;

    // set some particle dependent properties
    switch (this->type)
    {
        case SPRITE_SOLID: scale *= 0.9384f; break;
        case SPRITE_ALPHA: scale *= 0.9353f; break;
        case SPRITE_LIGHT: scale *= 1.5912f; break;
    }

    return scale;
}

bool prt_t::set_size(int size)
{
    if (!DEFINED_PPRT(this)) return false;

    if (!LOADED_PIP(this->pip_ref)) return false;
    std::shared_ptr<pip_t> ppip = PipStack.get_ptr(this->pip_ref);

    // set the graphical size
    this->size = size;

    // set the bumper size, if available
    if (0 == this->bump_size_stt)
    {
        // make the particle non-interacting if the initial bumper size was 0
        this->bump_real.size = 0;
        this->bump_padded.size = 0;
    }
    else
    {
        float real_size = FP8_TO_FLOAT(size) * this->get_scale();

        if (0.0f == this->bump_real.size || 0.0f == size)
        {
            // just set the size, assuming a spherical particle
            this->bump_real.size = real_size;
            this->bump_real.size_big = real_size * Ego::Math::sqrtTwo<float>();
            this->bump_real.height = real_size;
        }
        else
        {
            float mag = real_size / this->bump_real.size;

            // resize all dimensions equally
            this->bump_real.size *= mag;
            this->bump_real.size_big *= mag;
            this->bump_real.height *= mag;
        }

        // make sure that the virtual bumper size is at least as big as what is in the pip file
        this->bump_padded.size = std::max(this->bump_real.size, ((float)ppip->bump_size));
        this->bump_padded.size_big = std::max(this->bump_real.size_big, ((float)ppip->bump_size) * Ego::Math::sqrtTwo<float>());
        this->bump_padded.height = std::max(this->bump_real.height, ((float)ppip->bump_height));
    }

    // set the real size of the particle
    this->prt_min_cv.assign(this->bump_real);

    // use the padded bumper to figure out the chr_max_cv
    this->prt_max_cv.assign(this->bump_padded);

    return true;
}

//--------------------------------------------------------------------------------------------

prt_bundle_t *prt_bundle_t::do_bump_damage()
{
    // apply damage from  attatched bump particles (about once a second)
	if (!this->_prt_ptr) {
		return this;
	}
    prt_t *loc_pprt = this->_prt_ptr;
    std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;

    // this is often set to zero when the particle hits something
    int max_damage = std::abs(loc_pprt->damage.base) + std::abs(loc_pprt->damage.rand);

    // wait until the right time
    Uint32 update_count = update_wld + loc_pprt->getGUID();
    if (0 != (update_count & 31)) return this;

    // we must be attached to something
    if (!_currentModule->getObjectHandler().exists(loc_pprt->attachedto_ref)) return this;

    CHR_REF ichr = loc_pprt->attachedto_ref;
    Object *loc_pchr = _currentModule->getObjectHandler().get(loc_pprt->attachedto_ref);

    // find out who is holding the owner of this object
    CHR_REF iholder = chr_get_lowest_attachment(ichr, true);
    if (INVALID_CHR_REF == iholder) iholder = ichr;

    // do nothing if you are attached to your owner
    if ((INVALID_CHR_REF != loc_pprt->owner_ref) && (iholder == loc_pprt->owner_ref || ichr == loc_pprt->owner_ref)) return this;

    const std::shared_ptr<Object> &character = _currentModule->getObjectHandler()[ichr];

    //---- only do damage in certain cases:

    // 1) the particle has the DAMFX_ARRO bit
    bool skewered_by_arrow = HAS_SOME_BITS(loc_ppip->damfx, DAMFX_ARRO);

    // 2) the character is vulnerable to this damage type
    bool has_vulnie = chr_has_vulnie(GET_INDEX_PCHR(loc_pchr), loc_pprt->profile_ref);

    // 3) the character is "lit on fire" by the particle damage type
    bool is_immolated_by = (loc_pprt->damagetype < DAMAGE_COUNT && loc_pchr->reaffirm_damagetype == loc_pprt->damagetype);

    // 4) the character has no protection to the particle
    bool no_protection_from = (0 != max_damage) && (loc_pprt->damagetype < DAMAGE_COUNT) && (0 == loc_pchr->damage_resistance[loc_pprt->damagetype]);

    if (!skewered_by_arrow && !has_vulnie && !is_immolated_by && !no_protection_from)
    {
        return this;
    }

    IPair local_damage;
    if (has_vulnie || is_immolated_by)
    {
        // the damage is the maximum damage over and over again until the particle dies
        range_to_pair(loc_ppip->damage, &local_damage);
    }
    else if (no_protection_from)
    {
        // take a portion of whatever damage remains
        local_damage = loc_pprt->damage;
    }
    else
    {
        range_to_pair(loc_ppip->damage, &local_damage);

        local_damage.base /= 2;
        local_damage.rand /= 2;

        // distribute 1/2 of the maximum damage over the particle's lifetime
        if (!loc_pprt->is_eternal)
        {
            // how many 32 update cycles will this particle live through?
            int cycles = loc_pprt->lifetime_total / 32;

            if (cycles > 1)
            {
                local_damage.base /= cycles;
                local_damage.rand /= cycles;
            }
        }
    }

    //---- special effects
    if (loc_ppip->allowpush && 0 == loc_ppip->vel_hrz_pair.base)
    {
        // Make character limp
        character->vel[kX] *= 0.5f;
        character->vel[kY] *= 0.5f;
    }

    //---- do the damage
    int actual_damage = character->damage(ATK_BEHIND, local_damage, static_cast<DamageType>(loc_pprt->damagetype), loc_pprt->team,
        _currentModule->getObjectHandler()[loc_pprt->owner_ref], loc_ppip->damfx, false);

    // adjust any remaining particle damage
    if (loc_pprt->damage.base > 0)
    {
        loc_pprt->damage.base -= actual_damage;
        loc_pprt->damage.base = std::max(0, loc_pprt->damage.base);

        // properly scale the random amount
        loc_pprt->damage.rand = std::abs(loc_ppip->damage.to - loc_ppip->damage.from) * loc_pprt->damage.base / loc_ppip->damage.from;
    }

    return this;
}

int prt_bundle_t::do_contspawn()
{
    Uint8 spawn_count = 0;

    if (NULL == this->_prt_ptr) return spawn_count;
    prt_t *loc_pprt = this->_prt_ptr;
    std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;

    if (loc_ppip->contspawn._amount <= 0 || LocalParticleProfileRef::Invalid == loc_ppip->contspawn._lpip)
    {
        return spawn_count;
    }

    if (loc_pprt->contspawn_timer > 0) return spawn_count;

    // reset the spawn timer
    loc_pprt->contspawn_timer = loc_ppip->contspawn._delay;

    FACING_T facing = loc_pprt->facing;
    for (Uint8 tnc = 0; tnc < loc_ppip->contspawn._amount; tnc++)
    {
        PRT_REF prt_child = ParticleHandler::get().spawn_one_particle(loc_pprt->getPosition(), facing, loc_pprt->profile_ref, loc_ppip->contspawn._lpip,
                                                                      INVALID_CHR_REF, GRIP_LAST, loc_pprt->team, loc_pprt->owner_ref, this->_prt_ref, tnc, loc_pprt->target_ref);

        if (DEFINED_PRT(prt_child))
        {
            // Inherit velocities from the particle we were spawned from, but only if it wasn't attached to something

            /// @note ZF@> I have disabled this at the moment. This is what caused the erratic particle movement for the Adventurer Torch
            /// @note BB@> taking out the test works, though  I should have checked vs. loc_pprt->attached_ref, anyway,
            ///            since we already specified that the particle is not attached in the function call :P
            /// @note ZF@> I have again disabled this. Is this really needed? It wasn't implemented before and causes
            ///            many, many, many issues with all particles around the game.
            //if( !_currentModule->getObjectHandler().exists( loc_pprt->attachedto_ref ) )
            /*{
            PrtList.lst[prt_child].vel[kX] += loc_pprt->vel[kX];
            PrtList.lst[prt_child].vel[kY] += loc_pprt->vel[kY];
            PrtList.lst[prt_child].vel[kZ] += loc_pprt->vel[kZ];
            }*/

            //Keep count of how many were actually spawned
            spawn_count++;
        }

        facing += loc_ppip->contspawn._facingAdd;
    }

    return spawn_count;
}

prt_bundle_t *prt_bundle_t::update_do_water()
{
    if (NULL == this->_prt_ptr) return NULL;

    bool inwater = (this->_prt_ptr->pos[kZ] < water.surface_level)
                && (0 != ego_mesh_t::test_fx(PMesh, this->_prt_ptr->getTile(), MAPFX_WATER));

    if (inwater && water.is_water && this->_pip_ptr->end_water)
    {
        // Check for disaffirming character
        if (_currentModule->getObjectHandler().exists(this->_prt_ptr->attachedto_ref) && this->_prt_ptr->owner_ref == this->_prt_ptr->attachedto_ref)
        {
            // Disaffirm the whole character
            disaffirm_attached_particles(this->_prt_ptr->attachedto_ref);
        }
        else
        {
            // destroy the particle
            end_one_particle_in_game(this->_prt_ref);
            return NULL;
        }
    }
    else if (inwater)
    {
        bool  spawn_valid = false;
        LocalParticleProfileRef global_pip_index;
        fvec3_t vtmp = fvec3_t(this->_prt_ptr->pos[kX], this->_prt_ptr->pos[kY], water.surface_level);

        if (INVALID_CHR_REF == this->_prt_ptr->owner_ref && (PIP_SPLASH == this->_prt_ptr->pip_ref || PIP_RIPPLE == this->_prt_ptr->pip_ref))
        {
            /* do not spawn anything for a splash or a ripple */
            spawn_valid = false;
        }
        else
        {
            if (!this->_prt_ptr->enviro.inwater)
            {
                if (SPRITE_SOLID == this->_prt_ptr->type)
                {
                    global_pip_index = LocalParticleProfileRef(PIP_SPLASH);
                }
                else
                {
                    global_pip_index = LocalParticleProfileRef(PIP_RIPPLE);
                }
                spawn_valid = true;
            }
            else
            {
                if (SPRITE_SOLID == this->_prt_ptr->type && !_currentModule->getObjectHandler().exists(this->_prt_ptr->attachedto_ref))
                {
                    // only spawn ripples if you are touching the water surface!
                    if (this->_prt_ptr->pos[kZ] + this->_prt_ptr->bump_real.height > water.surface_level && this->_prt_ptr->pos[kZ] - this->_prt_ptr->bump_real.height < water.surface_level)
                    {
                        int ripand = ~((~RIPPLEAND) << 1);
                        if (0 == ((update_wld + this->_prt_ptr->getGUID()) & ripand))
                        {

                            spawn_valid = true;
                            global_pip_index = LocalParticleProfileRef(PIP_RIPPLE);
                        }
                    }
                }
            }
        }

        if (spawn_valid)
        {
            // Splash for particles is just a ripple
            ParticleHandler::get().spawn_one_particle_global(vtmp, 0, global_pip_index, 0);
        }

        this->_prt_ptr->enviro.inwater = true;
    }
    else
    {
        this->_prt_ptr->enviro.inwater = false;
    }

    return this;
}

prt_bundle_t * prt_bundle_t::update_animation()
{
    /// animate the particle
    if (NULL == this->_prt_ptr) return NULL;
    prt_t *loc_pprt = this->_prt_ptr;
    std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;

    bool image_overflow = false;
    long image_overflow_amount = 0;
    if (loc_pprt->_image._offset >= loc_pprt->_image._count)
    {
        // how did the image get here?
        image_overflow = true;

        // cast the integers to larger type to make sure there are no overflows
        image_overflow_amount = (long)loc_pprt->_image._offset + (long)loc_pprt->_image._add - (long)loc_pprt->_image._count;
    }
    else
    {
        // the image is in the correct range
        if ((loc_pprt->_image._count - loc_pprt->_image._offset) > loc_pprt->_image._add)
        {
            // the image will not overflow this update
            loc_pprt->_image._offset = loc_pprt->_image._offset + loc_pprt->_image._add;
        }
        else
        {
            image_overflow = true;
            // cast the integers to larger type to make sure there are no overflows
            image_overflow_amount = (long)loc_pprt->_image._offset + (long)loc_pprt->_image._add - (long)loc_pprt->_image._count;
        }
    }

    // what do you do about an image overflow?
    if (image_overflow)
    {
        if (loc_ppip->end_lastframe /*&& loc_ppip->end_time > 0*/) //ZF> I don't think the second statement is needed
        {
            // freeze it at the last frame
            loc_pprt->_image._offset = std::max(0, loc_pprt->_image._count - 1);
        }
        else
        {
            // the animation is looped. set the value to image_overflow_amount
            // so that we get the exact number of image updates called for
            loc_pprt->_image._offset = image_overflow_amount;
        }
    }

    // rotate the particle
    loc_pprt->rotate += loc_pprt->rotate_add;

    // update the particle size
    if (0 != loc_pprt->size_add)
    {
        int size_new;

        // resize the paricle
        size_new = loc_pprt->size + loc_pprt->size_add;
        size_new = CLIP(size_new, 0, 0xFFFF);

        loc_pprt->set_size(size_new);
    }

    // spin the particle
    loc_pprt->facing += loc_ppip->facingadd;

    // frames_remaining refers to the number of animation updates, not the
    // number of frames displayed
    if (loc_pprt->frames_remaining > 0)
    {
        loc_pprt->frames_remaining--;
    }

    // the animation has terminated
    if (loc_ppip->end_lastframe && 0 == loc_pprt->frames_remaining)
    {
        end_one_particle_in_game(this->_prt_ref);
    }

    return this;
}

prt_bundle_t * prt_bundle_t::update_dynalight()
{
    if (NULL == this->_prt_ptr) return NULL;
    prt_t *loc_pprt = this->_prt_ptr;
    std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;

    // Change dyna light values
    if (loc_pprt->dynalight.level > 0)
    {
        loc_pprt->dynalight.level += loc_ppip->dynalight.level_add;
        if (loc_pprt->dynalight.level < 0) loc_pprt->dynalight.level = 0;
    }
    else if (loc_pprt->dynalight.level < 0)
    {
        // try to guess what should happen for negative lighting
        loc_pprt->dynalight.level += loc_ppip->dynalight.level_add;
        if (loc_pprt->dynalight.level > 0) loc_pprt->dynalight.level = 0;
    }
    else
    {
        loc_pprt->dynalight.level += loc_ppip->dynalight.level_add;
    }

    loc_pprt->dynalight.falloff += loc_ppip->dynalight.falloff_add;

    return this;
}

prt_bundle_t * prt_bundle_t::update_timers()
{
    if (NULL == this->_prt_ptr) return NULL;
    prt_t *loc_pprt = this->_prt_ptr;

    // down the remaining lifetime of the particle
    if (loc_pprt->lifetime_remaining > 0)
    {
        loc_pprt->lifetime_remaining--;
    }

    // down the continuous spawn timer
    if (loc_pprt->contspawn_timer > 0)
    {
        loc_pprt->contspawn_timer--;
    }

    return this;
}

prt_bundle_t * prt_bundle_t::update_ingame()
{
    if (NULL == this->_prt_ptr) return NULL;
    prt_t *loc_pprt = this->_prt_ptr;
    std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;

    loc_pprt->is_hidden = false;
    // If the object to which the particle is attached to exists, ...
    if (_currentModule->getObjectHandler().exists(loc_pprt->attachedto_ref))
    {
        // ... the particle inherits the property of being hidden or not from that object.
        loc_pprt->is_hidden = _currentModule->getObjectHandler().get(loc_pprt->attachedto_ref)->is_hidden;
    }
    // Clear out the attachment if the character does not exist.
    else if (!_currentModule->getObjectHandler().exists(loc_pprt->attachedto_ref))
    {
        loc_pprt->attachedto_ref = INVALID_CHR_REF;
    }

    // If the particle is hidden, there is nothing else to do.
    if (loc_pprt->is_hidden) return this;

    // Determine if a "homing" particle still has something to "home":
    // If its homing (according to its profile), is not attached to an object (yet),
    // and a target exists, then the particle will "home" that target.
    loc_pprt->is_homing = loc_ppip->homing && !_currentModule->getObjectHandler().exists(loc_pprt->attachedto_ref)
                       && _currentModule->getObjectHandler().exists(loc_pprt->target_ref);

    // Update the particle interaction with water.
    /// @todo This might end the particle, however, the test via the return value *sucks*.
    if (!update_do_water()) return nullptr;

    // The following functions should not be done the first time through the update loop.
    if (0 == update_wld) return this;

    // Update the particle animation.
    /// @todo This might end the particle, however, the test via the return value *sucks*.
    if (!update_animation()) return nullptr;
    if (NULL == this->_prt_ptr) return NULL;

    if (!update_dynalight()) return NULL;
    if (NULL == this->_prt_ptr) return NULL;

    if (!update_timers()) return NULL;
    if (NULL == this->_prt_ptr) return NULL;

    do_contspawn();
    if (NULL == this->_prt_ptr) return NULL;

    if (!this->do_bump_damage()) return NULL;
    if (NULL == this->_prt_ptr) return NULL;

	loc_pprt->update_count++;

    // If the particle is done updating, remove it from the game, but do not kill it
	if (!loc_pprt->is_eternal && (loc_pprt->update_count > 0 && 0 == loc_pprt->lifetime_remaining))
    {
        end_one_particle_in_game(this->_prt_ref);
    }

    return this;
}

prt_bundle_t *prt_bundle_t::update_ghost()
{
	if (!this->_prt_ptr) {
		return nullptr;
	}
	prt_t *loc_pprt = this->_prt_ptr;
    std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;

    // is this the right function?
	if (!loc_pprt->is_ghost) {
		return this;
	}

    // is the prt visible
    bool prt_visible = (loc_pprt->size > 0) && (loc_pprt->inst.alpha > 0) && !loc_pprt->is_hidden;

    // are we done?
	if (!prt_visible || loc_pprt->frame_count > 0) {
        loc_pprt->requestTerminate();
        return nullptr;
    }

    // clear out the attachment if the character doesn't exist at all
    if (!_currentModule->getObjectHandler().exists(loc_pprt->attachedto_ref))
    {
        loc_pprt->attachedto_ref = INVALID_CHR_REF;
    }

    // determine whether the pbdl_prt->prt_ref is hidden
    loc_pprt->is_hidden = false;
    if (_currentModule->getObjectHandler().exists(loc_pprt->attachedto_ref)) {
        loc_pprt->is_hidden = _currentModule->getObjectHandler().get(loc_pprt->attachedto_ref)->is_hidden;
    }

    loc_pprt->is_homing = loc_ppip->homing && !_currentModule->getObjectHandler().exists(loc_pprt->attachedto_ref) && _currentModule->getObjectHandler().exists(loc_pprt->target_ref);

    // the following functions should not be done the first time through the update loop
    if (0 == update_wld) return this;

    if (!update_animation()) return nullptr;
    if (!loc_pprt) return nullptr;

	if (!update_dynalight()) return nullptr;
	if (!loc_pprt) return nullptr;

    if (!loc_pprt->is_hidden) {
		loc_pprt->update_count++;
    }

    return this;
}

prt_bundle_t * prt_bundle_t::update()
{
    prt_t *loc_pprt = this->_prt_ptr;
	if (!loc_pprt) {
		return nullptr;
	}
	// do the next step in the particle configuration
    if (!loc_pprt->run_config()) {
		*this = prt_bundle_t();
		return nullptr;
    }

    // if the bundle is no longer valid, return
    if (NULL == this->_prt_ptr || NULL == this->_pip_ptr) return this;

    // if the particle is no longer allocated, return
    if (!ALLOCATED_PPRT(this->_prt_ptr)) return this;

    // handle different particle states differently
    if (loc_pprt->is_ghost) {
        // the particle is not on
        return update_ghost();
    } else {
        // the particle is on
        return update_ingame();
    }
}

prt_bundle_t::prt_bundle_t()
	: _prt_ref(INVALID_PRT_REF), _prt_ptr(nullptr),
	  _pip_ref(INVALID_PIP_REF), _pip_ptr(nullptr) {
}

prt_bundle_t::prt_bundle_t(prt_t *prt)
	: _prt_ref(INVALID_PRT_REF), _prt_ptr(nullptr),
      _pip_ref(INVALID_PIP_REF), _pip_ptr(nullptr) {
	if (!prt) {
		throw std::invalid_argument("nullptr == prt");
	}
	_prt_ptr = prt;
	_prt_ref = GET_REF_PPRT(_prt_ptr);

	_pip_ref = _prt_ptr->pip_ref;
	_pip_ptr = PipStack.get_ptr(_pip_ref);
}

//--------------------------------------------------------------------------------------------

int prt_do_end_spawn(const PRT_REF iprt)
{
    int endspawn_count = 0;

    if (!ALLOCATED_PRT(iprt)) return endspawn_count;

    prt_t *pprt = ParticleHandler::get().get_ptr(iprt);

    // Spawn new particles if time for old one is up
    if (pprt->endspawn_amount > 0 /*&& ProfileSystem::get().isValidProfileID(pprt->profile_ref)*/ && LocalParticleProfileRef::Invalid != pprt->endspawn_lpip)
    {
        FACING_T facing = pprt->facing;
        for (Uint8 tnc = 0; tnc < pprt->endspawn_amount; tnc++)
        {
            PRT_REF spawned_prt = ParticleHandler::get().spawn_one_particle(pprt->pos_old, facing, pprt->profile_ref, pprt->endspawn_lpip,
                                                                            INVALID_CHR_REF, GRIP_LAST, pprt->team, prt_get_iowner(iprt, 0),
                                                                            iprt, tnc, pprt->target_ref);

            if (DEFINED_PRT(spawned_prt))
            {
                endspawn_count++;
            }

            facing += pprt->endspawn_facingadd;
        }

        // we have already spawned these particles, so set this amount to
        // zero in case we are not actually calling end_one_particle_in_game()
        // this time around.
        pprt->endspawn_amount = 0;
    }

    return endspawn_count;
}

void cleanup_all_particles()
{
    // do end-of-life care for particles. Must iterate over all particles since the
    // number of particles could change inside this list
    for (PRT_REF iprt = 0; iprt < ParticleHandler::get().getCount(); iprt++)
    {
        prt_t *pprt = ParticleHandler::get().get_ptr(iprt);

		if (!pprt->FLAG_ALLOCATED_PBASE()) continue;

		if (pprt->TERMINATED_PBASE())
        {
            // now that the object is in the "killed" state,
            // actually put it back into the free store
            ParticleHandler::get().free_one(GET_REF_PPRT(pprt));
        }
		else if (pprt->STATE_WAITING_PBASE())
        {
            // do everything to end the particle in-game (spawn secondary particles,
            // play end sound, etc.) amd mark it with kill_me
            end_one_particle_in_game(iprt);
        }
    }
}

void bump_all_particles_update_counters()
{
    for (PRT_REF cnt = 0; cnt < ParticleHandler::get().getCount(); cnt++)
    {
        prt_t *prt = ParticleHandler::get().get_ptr(cnt);
		if (!prt->ACTIVE_PBASE()) continue;
		prt->update_count++;
    }
}

std::shared_ptr<pip_t> prt_get_ppip(const PRT_REF ref)
{
    if (!ParticleHandler::get().isValidRef(ref)) return nullptr;
    return ParticleHandler::get().get_ptr(ref)->get_ppip();
}

CHR_REF prt_get_iowner(const PRT_REF iprt, int depth)
{
    /// @author BB
    /// @details A helper function for determining the owner of a paricle
    ///
    ///      @details There could be a possibility that a particle exists that was spawned by
    ///      another particle, but has lost contact with its original spawner. For instance
    ///      If an explosion particle bounces off of something with MISSILE_DEFLECT or
    ///      MISSILE_REFLECT, which subsequently dies before the particle...
    ///
    ///      That is actually pretty far fetched, but at some point it might make sense to
    ///      spawn particles just keeping track of the spawner (whether particle or character)
    ///      and working backward to any potential owner using this function. ;)
    ///
    /// @note this function should be completely trivial for anything other than
    ///       damage particles created by an explosion

    // be careful because this can be recursive
    if (depth > (int)ParticleHandler::get().getCount() - (int)ParticleHandler::get().getFreeCount())
    {
        return INVALID_CHR_REF;
    }

    if (!DEFINED_PRT(iprt)) return INVALID_CHR_REF;
    prt_t *pprt = ParticleHandler::get().get_ptr(iprt);

    CHR_REF iowner = INVALID_CHR_REF;
    if (_currentModule->getObjectHandler().exists(pprt->owner_ref))
    {
        iowner = pprt->owner_ref;
    }
    else
    {
        // make a check for a stupid looping structure...
        // cannot be sure you could never get a loop, though

        if (!ALLOCATED_PRT(pprt->parent_ref))
        {
            // make sure that a non valid parent_ref is marked as non-valid
            pprt->parent_ref = INVALID_PRT_REF;
            pprt->parent_guid = EGO_GUID_INVALID;
        }
        else
        {
            // if a particle has been poofed, and another particle lives at that address,
            // it is possible that the pprt->parent_ref points to a valid particle that is
            // not the parent. Depending on how scrambled the list gets, there could actually
            // be looping structures. I have actually seen this, so don't laugh :)

            if (ParticleHandler::get().get_ptr(pprt->parent_ref)->getGUID() == pprt->parent_guid)
            {
                if (iprt != pprt->parent_ref)
                {
                    iowner = prt_get_iowner(pprt->parent_ref, depth + 1);
                }
            }
            else
            {
                // the parent particle doesn't exist anymore
                // fix the reference
                pprt->parent_ref = INVALID_PRT_REF;
                pprt->parent_guid = EGO_GUID_INVALID;
            }
        }
    }

    return iowner;
}
