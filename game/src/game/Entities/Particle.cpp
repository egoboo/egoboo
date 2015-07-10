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
/// @author Johan Jansen aka Zefz

#define GAME_ENTITIES_PRIVATE 1

#include "Particle.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/Module/Module.hpp"
#include "game/Entities/_Include.hpp"
#include "game/game.h"
#include "game/Entities/particle.h"

namespace Ego
{
const std::shared_ptr<Particle> Particle::INVALID_PARTICLE = nullptr;

Particle::Particle() :
    _particleID(INVALID_PRT_REF),
    _bspLeaf(this, BSP_LEAF_PRT, INVALID_PRT_REF),
    _attachedTo(INVALID_CHR_REF),
    _particleProfileID(INVALID_PIP_REF),
    _particleProfile(nullptr),
    _isTerminated(true),
    _target(INVALID_CHR_REF),
    _spawnerProfile(INVALID_CHR_REF),
    _isHoming(false)
{
    reset(INVALID_PRT_REF);
}

void Particle::reset(PRT_REF ref) 
{
    //We are terminated until we are initialized()
    _isTerminated = true;

    _particleID = ref;
    frame_count = 0;
    _bspLeaf = BSP_leaf_t(this, BSP_LEAF_PRT, ref); //because we have a new ref

    _particleProfileID = INVALID_PIP_REF;
    _particleProfile = nullptr;

    _attachedTo = INVALID_CHR_REF;
    owner_ref = INVALID_CHR_REF;
    _target = INVALID_CHR_REF;
    parent_ref = INVALID_PRT_REF;
    _spawnerProfile = INVALID_PRO_REF,

    attachedto_vrt_off = 0;
    type = SPRITE_LIGHT;
    facing = 0;
    team = 0;

    vel_stt = fvec3_t::zero();
    offset = fvec3_t::zero();

    PhysicsData::reset(this);

    rotate = 0;
    rotate_add = 0;

    size_stt = 0;
    size = 0;
    size_add = 0;

    _image.reset();

    // "no lifetime" = "eternal"
    is_eternal = false;
    lifetime_total = std::numeric_limits<size_t>::max();
    lifetime_remaining = lifetime_total;
    frames_total = std::numeric_limits<size_t>::max();
    frames_remaining = frames_total;

    contspawn_timer = 0;

    // bumping
    bump_size_stt = 0;           ///< the starting size of the particle (8.8 fixed point)
    bumper_t::reset(&bump_real);
    bumper_t::reset(&bump_padded);
    prt_min_cv = oct_bb_t(oct_vec_v2_t());
    prt_max_cv = oct_bb_t(oct_vec_v2_t());

    // damage
    damagetype = DamageType::DAMAGE_SLASH;
    damage.base = 0;
    damage.rand = 0;
    lifedrain = 0;
    manadrain = 0;

    // bump effects
    is_bumpspawn = false;

    // motion effects
    buoyancy = 0.0f;
    air_resistance = 0.0f;
    _isHoming = false;
    no_gravity = false;

    ///if != SPAWNNOCHARACTER, then a character is spawned on end
    endspawn_characterstate = SPAWNNOCHARACTER; 

    dynalight.reset();
    inst.reset();
    enviro.reset();
}


bool Particle::isAttached() const
{
    return _currentModule->getObjectHandler().exists(_attachedTo);
}

const std::shared_ptr<Object>& Particle::getAttachedObject() const
{
    return _currentModule->getObjectHandler()[_attachedTo];
}


bool Particle::setPosition(const fvec3_t& position)
{
    EGO_DEBUG_VALIDATE(position);

    /// Has our position changed?
    if (position != this->pos)
    {
        this->pos = position;

        _tile = ego_mesh_t::get_grid(_currentModule->getMeshPointer(), PointWorld(this->pos[kX], this->pos[kY])).getI();
        _block = ego_mesh_t::get_block(_currentModule->getMeshPointer(), PointWorld(this->pos[kX], this->pos[kY])).getI();

        // Update whether the current particle position is safe.
        updateSafe(false);

        return true;
    }
    return false;
}

bool Particle::updateSafeRaw()
{
    bool retval = false;

    BIT_FIELD hit_a_wall;
    float  pressure;

    fvec2_t nrm;
    hit_a_wall = hit_wall(nrm, &pressure, nullptr);
    if ((0 == hit_a_wall) && (0.0f == pressure))
    {
        safe_valid = true;
        safe_pos = getPosition();
        safe_time = update_wld;
        safe_grid = ego_mesh_t::get_grid(_currentModule->getMeshPointer(), PointWorld(pos[kX], pos[kY])).getI();

        retval = true;
    }

    return retval;
}

bool Particle::updateSafe(bool force)
{
    bool retval = false;
    bool needs_update = false;

    if (force || !safe_valid)
    {
        needs_update = true;
    }
    else
    {
        TileIndex new_grid = ego_mesh_t::get_grid(_currentModule->getMeshPointer(), PointWorld(pos[kX], pos[kY]));

        if (TileIndex::Invalid == new_grid)
        {
            if (std::abs(pos[kX] - safe_pos[kX]) > GRID_FSIZE ||
                std::abs(pos[kY] - safe_pos[kY]) > GRID_FSIZE)
            {
                needs_update = true;
            }
        }
        else if (new_grid != safe_grid)
        {
            needs_update = true;
        }
    }

    if (needs_update)
    {
        retval = updateSafeRaw();
    }

    return retval;
}

BIT_FIELD Particle::hit_wall(fvec2_t& nrm, float *pressure, mesh_wall_data_t *data)
{
    return hit_wall(getPosition(), nrm, pressure, data);
}

BIT_FIELD Particle::hit_wall(const fvec3_t& pos, fvec2_t& nrm, float *pressure, mesh_wall_data_t *data)
{
    BIT_FIELD stoppedby = MAPFX_IMPASS;
    if (0 != getProfile()->bump_money) SET_BIT(stoppedby, MAPFX_WALL);

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    BIT_FIELD  result = ego_mesh_hit_wall(_currentModule->getMeshPointer(), pos, 0.0f, stoppedby, nrm, pressure, data);

    return result;
}

BIT_FIELD Particle::test_wall(mesh_wall_data_t *data)
{
    return test_wall(getPosition(), data);
}

BIT_FIELD Particle::test_wall(const fvec3_t& pos, mesh_wall_data_t *data)
{
    BIT_FIELD  stoppedby = MAPFX_IMPASS;
    if (0 != getProfile()->bump_money) SET_BIT(stoppedby, MAPFX_WALL);

    // Do the wall test.
    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    BIT_FIELD result = ego_mesh_test_wall(_currentModule->getMeshPointer(), pos, 0.0f, stoppedby, data);

    return result;
}

const std::shared_ptr<pip_t>& Particle::getProfile() const
{
    return _particleProfile;
}

float Particle::getScale() const
{
    float scale = 0.25f;

    // set some particle dependent properties
    switch (type)
    {
        case SPRITE_SOLID: scale *= 0.9384f; break;
        case SPRITE_ALPHA: scale *= 0.9353f; break;
        case SPRITE_LIGHT: scale *= 1.5912f; break;
    }

    return scale;
}

void Particle::setSize(int size)
{
    // set the graphical size
    this->size = size;

    // set the bumper size, if available
    if (0 == bump_size_stt)
    {
        // make the particle non-interacting if the initial bumper size was 0
        bump_real.size = 0;
        bump_padded.size = 0;
    }
    else
    {
        const float realSize = FP8_TO_FLOAT(size) * getScale();

        if (0.0f == bump_real.size || 0.0f == size)
        {
            // just set the size, assuming a spherical particle
            bump_real.size = realSize;
            bump_real.size_big = realSize * Ego::Math::sqrtTwo<float>();
            bump_real.height = realSize;
        }
        else
        {
            float mag = realSize / bump_real.size;

            // resize all dimensions equally
            bump_real.size *= mag;
            bump_real.size_big *= mag;
            bump_real.height *= mag;
        }

        // make sure that the virtual bumper size is at least as big as what is in the pip file
        bump_padded.size     = std::max<float>(bump_real.size, getProfile()->bump_size);
        bump_padded.size_big = std::max<float>(bump_real.size_big, getProfile()->bump_size * Ego::Math::sqrtTwo<float>());
        bump_padded.height   = std::max<float>(bump_real.height, getProfile()->bump_height);
    }

    // set the real size of the particle
    prt_min_cv.assign(bump_real);

    // use the padded bumper to figure out the chr_max_cv
    prt_max_cv.assign(bump_padded);
}

void Particle::requestTerminate()
{
    _isTerminated = true;
}

void Particle::setElevation(const float level)
{
    enviro.level = level;

    float loc_height = bump_real.height * 0.5f;

    enviro.adj_level = enviro.level;
    enviro.adj_floor = enviro.floor_level;

    enviro.adj_level += loc_height;
    enviro.adj_floor += loc_height;

    // set the zlerp after we have done everything to the particle's level we care to
    enviro.zlerp = (pos[kZ] - enviro.adj_level) / PLATTOLERANCE;
    enviro.zlerp = Ego::Math::constrain(enviro.zlerp, 0.0f, 1.0f);
}

bool Particle::isHidden() const
{
    const std::shared_ptr<Object>& attachedToObject = _currentModule->getObjectHandler()[_attachedTo]; 

    if(!attachedToObject) {
        return false;
    }

    return attachedToObject->is_hidden;
}

bool Particle::hasValidTarget() const
{
    return getTarget() != nullptr;
}

bool Particle::isTerminated() const
{
    return _isTerminated;
}

PIP_REF Particle::getProfileID() const
{
    return _particleProfileID;
}

void Particle::update()
{
    //Should never happen
    if(isTerminated()) {
        return;
    }

    // If the particle is hidden, there is nothing else to do.
    if(isHidden()) {
        return;
    }

    //Clear invalid attachements incase Object have been removed from the game
    if(!isAttached()) {
        _attachedTo = INVALID_CHR_REF;
    }

    // Determine if a "homing" particle still has something to "home":
    // If its homing (according to its profile), is not attached to an object (yet),
    // and a target exists, then the particle will "home" that target.
    if(_isHoming) {
        _isHoming = !isAttached() && hasValidTarget();
    }

    updateDynamicLighting();

    // Update the particle animation.
    updateAnimation();
    if(isTerminated()) {
        return; //destroyed by end of animation
    }


    // Update the particle interaction with water.
    updateWater();
    if(isTerminated()) {
        return; //destroyed by water
    }

    //Spawn other particles
    updateContinuousSpawning();

    //Damage whomever we are attached to
    updateAttachedDamage();

    // down the remaining lifetime of the particle
    if (!is_eternal)
    {
        if (lifetime_remaining > 0) {
            lifetime_remaining--;
        }
        else {
            //end of life
            requestTerminate();
        }
    }
}

void Particle::updateWater()
{
    bool inwater = (pos[kZ] < water._surface_level) && isOverWater();

    if (inwater && water._is_water && getProfile()->end_water)
    {
        // Check for disaffirming character
        if (isAttached() && owner_ref == _attachedTo)
        {
            // Disaffirm the whole character
            disaffirm_attached_particles(_attachedTo);
        }
        else
        {
            // destroy the particle
            requestTerminate();
            return;
        }
    }
    else if (inwater)
    {
        bool  spawn_valid = false;
        LocalParticleProfileRef global_pip_index;
        fvec3_t vtmp = fvec3_t(pos[kX], pos[kY], water._surface_level);

        if (INVALID_CHR_REF == owner_ref && (PIP_SPLASH == getProfileID() || PIP_RIPPLE == getProfileID()))
        {
            /* do not spawn anything for a splash or a ripple */
            spawn_valid = false;
        }
        else
        {
            if (!enviro.inwater)
            {
                if (SPRITE_SOLID == type)
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
                if (SPRITE_SOLID == type && !isAttached())
                {
                    // only spawn ripples if you are touching the water surface!
                    if (pos[kZ] + bump_real.height > water._surface_level && pos[kZ] - bump_real.height < water._surface_level)
                    {
                        int ripand = ~((~RIPPLEAND) << 1);
                        if (0 == ((update_wld + _particleID) & ripand))
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
            ParticleHandler::get().spawnGlobalParticle(vtmp, 0, global_pip_index, 0);
        }

        enviro.inwater = true;
    }
    else
    {
        enviro.inwater = false;
    }
}

void Particle::updateAnimation()
{
    /// animate the particle

    bool image_overflow = false;
    long image_overflow_amount = 0;
    if (_image._offset >= _image._count)
    {
        // how did the image get here?
        image_overflow = true;

        // cast the integers to larger type to make sure there are no overflows
        image_overflow_amount = (long)_image._offset + (long)_image._add - (long)_image._count;
    }
    else
    {
        // the image is in the correct range
        if ((_image._count - _image._offset) > _image._add)
        {
            // the image will not overflow this update
            _image._offset = _image._offset + _image._add;
        }
        else
        {
            image_overflow = true;
            // cast the integers to larger type to make sure there are no overflows
            image_overflow_amount = (long)_image._offset + (long)_image._add - (long)_image._count;
        }
    }

    // what do you do about an image overflow?
    if (image_overflow)
    {
        if (getProfile()->end_lastframe /*&& getProfile()->end_time > 0*/) //ZF> I don't think the second statement is needed
        {
            // freeze it at the last frame
            _image._offset = std::max(0, _image._count - 1);
        }
        else
        {
            // the animation is looped. set the value to image_overflow_amount
            // so that we get the exact number of image updates called for
            _image._offset = image_overflow_amount;
        }
    }

    // rotate the particle
    rotate += rotate_add;

    // update the particle size
    if (0 != size_add)
    {
        int size_new;

        // resize the paricle
        size_new = size + size_add;
        size_new = Ego::Math::constrain(size_new, 0, 0xFFFF);

        setSize(size_new);
    }

    // spin the particle
    facing += getProfile()->facingadd;

    // frames_remaining refers to the number of animation updates, not the
    // number of frames displayed
    if (frames_remaining > 0)
    {
        frames_remaining--;
    }

    // the animation has terminated
    if (getProfile()->end_lastframe && 0 == frames_remaining)
    {
        //End of life
        requestTerminate();
    }
}

void Particle::updateDynamicLighting()
{
    // Change dyna light values
    if (dynalight.level > 0)
    {
        dynalight.level += getProfile()->dynalight.level_add;
        if (dynalight.level < 0) dynalight.level = 0;
    }
    else if (dynalight.level < 0)
    {
        // try to guess what should happen for negative lighting
        dynalight.level += getProfile()->dynalight.level_add;
        if (dynalight.level > 0) dynalight.level = 0;
    }
    else
    {
        dynalight.level += getProfile()->dynalight.level_add;
    }

    dynalight.falloff += getProfile()->dynalight.falloff_add;
}

size_t Particle::updateContinuousSpawning()
{
    size_t spawn_count = 0;

    if (getProfile()->contspawn._amount <= 0 || LocalParticleProfileRef::Invalid == getProfile()->contspawn._lpip)
    {
        return spawn_count;
    }

    //Are we ready to spawn yet?
    if (contspawn_timer > 0) {
        contspawn_timer--;
        return spawn_count;
    }

    // reset the spawn timer
    contspawn_timer = getProfile()->contspawn._delay;

    FACING_T facing = this->facing;
    for (size_t tnc = 0; tnc < getProfile()->contspawn._amount; tnc++)
    {
        std::shared_ptr<Ego::Particle> prt_child = ParticleHandler::get().spawnLocalParticle(getPosition(), facing, _spawnerProfile, getProfile()->contspawn._lpip,
                                                                      INVALID_CHR_REF, GRIP_LAST, team, owner_ref, _particleID, tnc, _target);

        if (prt_child)
        {
            //Keep count of how many were actually spawned
            spawn_count++;
        }

        facing += getProfile()->contspawn._facingAdd;
    }

    return spawn_count;
}

void Particle::updateAttachedDamage()
{
    // this is often set to zero when the particle hits something
    int max_damage = std::abs(damage.base) + std::abs(damage.rand);

    // wait until the right time
    Uint32 update_count = update_wld + _particleID;
    if (0 != (update_count & 31)) return;

    // we must be attached to something
    if (!isAttached()) return;

    const std::shared_ptr<Object> &attachedObject = getAttachedObject();

    // find out who is holding the owner of this object
    CHR_REF iholder = chr_get_lowest_attachment(attachedObject->getCharacterID(), true);
    if (INVALID_CHR_REF == iholder) iholder = attachedObject->getCharacterID();

    // do nothing if you are attached to your owner
    if ((INVALID_CHR_REF != owner_ref) && (iholder == owner_ref || attachedObject->getCharacterID() == owner_ref)) return;

    //---- only do damage in certain cases:

    // 1) the particle has the DAMFX_ARRO bit
    bool skewered_by_arrow = HAS_SOME_BITS(getProfile()->damfx, DAMFX_ARRO);

    // 2) the character is vulnerable to this damage type
    bool has_vulnie = chr_has_vulnie(attachedObject->getCharacterID(), _spawnerProfile);

    // 3) the character is "lit on fire" by the particle damage type
    bool is_immolated_by = (damagetype < DAMAGE_COUNT && attachedObject->reaffirm_damagetype == damagetype);

    // 4) the character has no protection to the particle
    bool no_protection_from = (0 != max_damage) && (damagetype < DAMAGE_COUNT) && (0.0f <= attachedObject->getDamageReduction(damagetype));

    if (!skewered_by_arrow && !has_vulnie && !is_immolated_by && !no_protection_from)
    {
        return;
    }

    IPair local_damage;
    if (has_vulnie || is_immolated_by)
    {
        // the damage is the maximum damage over and over again until the particle dies
        range_to_pair(getProfile()->damage, &local_damage);
    }
    else if (no_protection_from)
    {
        // take a portion of whatever damage remains
        local_damage = damage;
    }
    else
    {
        range_to_pair(getProfile()->damage, &local_damage);

        local_damage.base /= 2;
        local_damage.rand /= 2;

        // distribute 1/2 of the maximum damage over the particle's lifetime
        if (!is_eternal)
        {
            // how many 32 update cycles will this particle live through?
            int cycles = lifetime_total / 32;

            if (cycles > 1)
            {
                local_damage.base /= cycles;
                local_damage.rand /= cycles;
            }
        }
    }

    //---- special effects
    if (getProfile()->allowpush && 0 == getProfile()->vel_hrz_pair.base)
    {
        // Make character limp
        attachedObject->vel[kX] *= 0.5f;
        attachedObject->vel[kY] *= 0.5f;
    }

    //---- do the damage
    int actual_damage = attachedObject->damage(ATK_BEHIND, local_damage, static_cast<DamageType>(damagetype), team,
        _currentModule->getObjectHandler()[owner_ref], getProfile()->damfx, false);

    // adjust any remaining particle damage
    if (damage.base > 0)
    {
        damage.base -= actual_damage;
        damage.base = std::max(0, damage.base);

        // properly scale the random amount
        damage.rand = std::abs(getProfile()->damage.to - getProfile()->damage.from) * damage.base / getProfile()->damage.from;
    }
}

void Particle::destroy()
{
    if(_particleID == INVALID_PRT_REF) {
        throw std::logic_error("tried to destroy() Particle that was already destroyed");
    }

    if(!isTerminated()) {
        throw std::logic_error("tried to destroy() Particle that was not terminated");
    }

    //This is no longer a valid particle
    _particleID = INVALID_PRT_REF;

    // Spawn new particles if time for old one is up
    if (getProfile()->endspawn._amount > 0 && LocalParticleProfileRef::Invalid != getProfile()->endspawn._lpip)
    {
        FACING_T facing = this->facing;
        for (size_t tnc = 0; tnc < getProfile()->endspawn._amount; tnc++)
        {
            if(_spawnerProfile == INVALID_PRO_REF)
            {
                //Global particle
                ParticleHandler::get().spawnGlobalParticle(pos_old, facing, getProfile()->endspawn._lpip, tnc);
            }
            else
            {
                //Local particle
                ParticleHandler::get().spawnLocalParticle(pos_old, facing, _spawnerProfile, getProfile()->endspawn._lpip,
                                                        INVALID_CHR_REF, GRIP_LAST, team, owner_ref,
                                                        _particleID, tnc, _target);
            }

            facing += getProfile()->endspawn._facingAdd;
        }
    }

    //Spawn an Object on particle end? (happens through a special script function)
    if (SPAWNNOCHARACTER != endspawn_characterstate)
    {
        CHR_REF child = spawn_one_character(getPosition(), _spawnerProfile, team, 0, facing, NULL, INVALID_CHR_REF);
        if (_currentModule->getObjectHandler().exists(child))
        {
            Object *pchild = _currentModule->getObjectHandler().get(child);

            chr_set_ai_state(pchild, endspawn_characterstate);
            pchild->ai.owner = owner_ref;
        }
    }

    // Play end sound
    playSound(getProfile()->end_sound);
}

void Particle::playSound(int8_t sound)
{
    //Invalid sound?
    if(sound < 0) {
        return;
    }

    //If we were spawned by an Object, then use that Object's sound pool
    const std::shared_ptr<ObjectProfile> &profile = ProfileSystem::get().getProfile(_spawnerProfile);
    if (profile) {
        AudioSystem::get().playSound(pos, profile->getSoundID(sound));
    }

    //Else we are a global particle and use global particle sounds
    else if (sound >= 0 && sound < GSND_COUNT)
    {
        GlobalSound globalSound = static_cast<GlobalSound>(sound);
        AudioSystem::get().playSound(pos, AudioSystem::get().getGlobalSound(globalSound));
    }
}

bool Particle::initialize(const PRT_REF particleID, const fvec3_t& spawnPos, const FACING_T spawnFacing, const PRO_REF spawnProfile,
                        const PIP_REF particleProfile, const CHR_REF spawnAttach, uint16_t vrt_offset, const TEAM_REF spawnTeam,
                        const CHR_REF spawnOrigin, const PRT_REF spawnParticleOrigin, const int multispawn, const CHR_REF spawnTarget,
                        const bool onlyOverWater)
{
    const int INFINITE_UPDATES = std::numeric_limits<int>::max();

    fvec3_t vel;
    int     offsetfacing = 0, newrand;

    //if(!isTerminated()) {
    //    throw std::logic_error("Tried to spawn an existing particle that was not terminated");
    //}

    //Clear any old data first
    reset(particleID);

    //Load particle profile
    _spawnerProfile = spawnProfile;
    _particleProfileID = particleProfile;
    assert(PipStack.isLoaded(_particleProfileID));
    _particleProfile = PipStack.get_ptr(_particleProfileID);
    assert(_particleProfile != nullptr); //"Tried to spawn particle with invalid PIP_REF"

    team = spawnTeam;
    parent_ref = spawnParticleOrigin;
    damagetype = getProfile()->damageType;
    lifedrain = getProfile()->lifeDrain;
    manadrain = getProfile()->manaDrain;

    //Mark particle as no longer terminated
    _isTerminated = false;

    // Save a version of the position for local use.
    // In cpp, will be passed by reference, so we do not want to alter the
    // components of the original vector.
    fvec3_t tmp_pos = spawnPos;
    FACING_T loc_facing = spawnFacing;

    // try to get an idea of who our owner is even if we are
    // given bogus info
    CHR_REF loc_chr_origin = spawnOrigin;
    if (!_currentModule->getObjectHandler().exists(spawnOrigin) && ParticleHandler::get()[spawnParticleOrigin])
    {
        loc_chr_origin = prt_get_iowner(spawnParticleOrigin, 0);
    }
    owner_ref = loc_chr_origin;

    // Lighting and sound
    dynalight = getProfile()->dynalight;
    dynalight.on = false;
    if (0 == multispawn)
    {
        dynalight.on = getProfile()->dynalight.mode;
        if (DYNA_MODE_LOCAL == getProfile()->dynalight.mode)
        {
            dynalight.on = DYNA_MODE_OFF;
        }
    }

    // Set character attachments ( pdata->chr_attach == INVALID_CHR_REF means none )
    _attachedTo = spawnAttach;
    attachedto_vrt_off = vrt_offset;

    // Correct loc_facing
    loc_facing = loc_facing + getProfile()->facing_pair.base;

    // Targeting...
    vel[kZ] = 0;

    offset[kZ] = generate_irand_pair(getProfile()->spacing_vrt_pair) - (getProfile()->spacing_vrt_pair.rand / 2);
    tmp_pos[kZ] += offset[kZ];
    const int velocity = generate_irand_pair(getProfile()->vel_hrz_pair);

    //Set target
    _target = spawnTarget;
    if (getProfile()->newtargetonspawn)
    {
        if (getProfile()->targetcaster)
        {
            // Set the target to the caster
            _target = owner_ref;
        }
        else
        {
            const int PERFECT_AIM = 45.0f;   // 45 dex is perfect aim
            const float attackerAgility = _currentModule->getObjectHandler().get(owner_ref)->getAttribute(Ego::Attribute::AGILITY);

            // Find a target
            FACING_T targetAngle;
            _target = prt_find_target(spawnPos, loc_facing, _particleProfileID, spawnTeam, owner_ref, spawnTarget, &targetAngle);
            const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[_target];

            if (target && !getProfile()->homing)
            {
                //Correct angle to new target
                loc_facing -= targetAngle;
            }

            //Agility determines how good we aim towards the target
            offsetfacing = 0;
            if ( attackerAgility < PERFECT_AIM)
            {
                //Add some random error (Apply 50% error at 10 Agility)
                float aimError = 0.5f;

                //Increase aim error by 5% for each Agility below 10 (up to a max of 100% error at 0 Agility)
                if(attackerAgility < 10.0f) {
                    aimError += 0.5f - (attackerAgility*0.05f);
                }
                else {
                    //Agility reduces aim error (convering towards 0% error at 45 Agility)
                    aimError -= (0.5f/PERFECT_AIM) * attackerAgility;
                }

                offsetfacing = Random::next(getProfile()->facing_pair.rand) - (getProfile()->facing_pair.rand / 2);
                offsetfacing *= aimError;
            }

            if (0.0f != getProfile()->zaimspd)
            {
                if (target)
                {
                    // These aren't velocities...  This is to do aiming on the Z axis
                    if (velocity > 0)
                    {
                        vel[kX] = target->getPosX() - spawnPos[kX];
                        vel[kY] = target->getPosY() - spawnPos[kY];
                        float distance = std::sqrt(vel[kX] * vel[kX] + vel[kY] * vel[kY]) / velocity;  // This is the number of steps...
                        if (distance > 0.0f)
                        {
                            // This is the vel[kZ] alteration
                            vel[kZ] = (target->getPosZ() + (target->bump.height * 0.5f) - tmp_pos[kZ]) / distance;
                        }
                    }
                }
                else
                {
                    vel[kZ] = 0.5f * getProfile()->zaimspd;
                }

                vel[kZ] = Ego::Math::constrain(vel[kZ], -0.5f * getProfile()->zaimspd, getProfile()->zaimspd);
            }
        }

        const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[_target];

        // Does it go away?
        if (!target && getProfile()->needtarget)
        {
            requestTerminate();
            return false;
        }

        // Start on top of target
        if (target && getProfile()->startontarget)
        {
            tmp_pos[kX] = target->getPosX();
            tmp_pos[kY] = target->getPosY();
        }
    }
    else
    {
        // Correct loc_facing for randomness
        offsetfacing = generate_irand_pair(getProfile()->facing_pair) - (getProfile()->facing_pair.base + getProfile()->facing_pair.rand / 2);
    }
    loc_facing = loc_facing + offsetfacing;
    facing = loc_facing;

    // this is actually pointing in the opposite direction?
    TURN_T turn = TO_TURN(loc_facing);

    // Location data from arguments
    newrand = generate_irand_pair(getProfile()->spacing_hrz_pair);
    offset[kX] = -turntocos[turn] * newrand;
    offset[kY] = -turntosin[turn] * newrand;

    tmp_pos[kX] += offset[kX];
    tmp_pos[kY] += offset[kY];

    tmp_pos[kX] = CLIP(tmp_pos[kX], 0.0f, _currentModule->getMeshPointer()->gmem.edge_x - 2.0f);
    tmp_pos[kY] = CLIP(tmp_pos[kY], 0.0f, _currentModule->getMeshPointer()->gmem.edge_y - 2.0f);

    setPosition(tmp_pos);
    pos_old = tmp_pos;
    pos_stt = tmp_pos;

    //Can this particle only spawn over water?
    if(onlyOverWater && !isOverWater()) {
        return false;
    }

    // Velocity data
    vel[kX] = -turntocos[turn] * velocity;
    vel[kY] = -turntosin[turn] * velocity;
    vel[kZ] += generate_irand_pair(getProfile()->vel_vrt_pair) - (getProfile()->vel_vrt_pair.rand / 2);
    this->vel = vel_old = vel_stt = vel;

    // Template values
    bump_size_stt = getProfile()->bump_size;
    type = getProfile()->type;

    // Image data
    rotate = (FACING_T)generate_irand_pair(getProfile()->rotate_pair);
    rotate_add = getProfile()->rotate_add;

    size_stt = getProfile()->size_base;
    size_add = getProfile()->size_add;

    _image._start = (getProfile()->image_stt)*EGO_ANIMATION_MULTIPLIER;
    _image._add = generate_irand_pair(getProfile()->image_add);
    _image._count = (getProfile()->image_max)*EGO_ANIMATION_MULTIPLIER;

    // a particle can EITHER end_lastframe or end_time.
    // if it ends after the last frame, end_time tells you the number of cycles through
    // the animation
    int prt_anim_frames_updates = 0;
    bool prt_anim_infinite = false;
    if (getProfile()->end_lastframe)
    {
        if (0 == _image._add)
        {
            prt_anim_frames_updates = INFINITE_UPDATES;
            prt_anim_infinite = true;
        }
        else
        {
            prt_anim_frames_updates = _image.getUpdateCount();

            if (getProfile()->end_time > 0)
            {
                // Part time is used to give number of cycles
                prt_anim_frames_updates *= getProfile()->end_time;
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
    int prt_life_frames_updates = 0;
    bool prt_life_infinite = false;
    if (getProfile()->end_lastframe)
    {
        // for end last frame, the lifetime is given by the number of animation frames
        prt_life_frames_updates = prt_anim_frames_updates;
        prt_life_infinite = prt_anim_infinite;
    }
    else if (getProfile()->end_time <= 0)
    {
        // zero or negative lifetime == infinite lifetime
        prt_life_frames_updates = INFINITE_UPDATES;
        prt_life_infinite = true;
    }
    else
    {
        prt_life_frames_updates = getProfile()->end_time;
    }
    prt_life_frames_updates = std::max(1, prt_life_frames_updates);

    // set lifetime counter
    if (prt_life_infinite)
    {
        lifetime_total = std::numeric_limits<size_t>::max();
        is_eternal = true;
    }
    else
    {
        // the lifetime is really supposed to be in terms of frames, but
        // to keep the number of updates stable, the frames could lag.
        // sooo... we just rescale the prt_life_frames_updates so that it will work with the
        // updates and cross our fingers
        //lifetime_total = std::ceil((float)prt_life_frames_updates * (float)GameEngine::GAME_TARGET_UPS / (float)GameEngine::GAME_TARGET_FPS);
        lifetime_total = prt_life_frames_updates;
    }

    // make the particle exists for AT LEAST one update
    lifetime_total = std::max<size_t>(1, lifetime_total);
    lifetime_remaining = lifetime_total;

    // set the frame counters
    // make the particle display AT LEAST one frame, regardless of how many updates
    // it has or when someone requests for it to terminate
    frames_total = std::max(1, prt_anim_frames_updates);
    frames_remaining = frames_total;

    // Damage stuff
    range_to_pair(getProfile()->damage, &(damage));

    // Spawning data
    if (0 != getProfile()->contspawn._delay)
    {
        contspawn_timer = 1;

        // Because attachment takes an update before it happens
        if (isAttached()) {
            contspawn_timer++;
        }
    }

    // set up the particle transparency
    inst.alpha = 0xFF;
    switch (inst.type)
    {
        case SPRITE_SOLID: break;
        case SPRITE_ALPHA: inst.alpha = 0x80; break;    //#define PRT_TRANS 0x80
        case SPRITE_LIGHT: break;
    }

    // is the spawn location safe?
    fvec2_t nrm;
    if (0 == hit_wall(tmp_pos, nrm, nullptr, nullptr))
    {
        safe_pos = tmp_pos;
        safe_valid = true;
        safe_grid = getTile();
    }

    // get an initial value for the _isHoming variable
    _isHoming = getProfile()->homing && !isAttached();

    //enable or disable gravity
    no_gravity = getProfile()->ignore_gravity;

    // estimate some parameters for buoyancy and air resistance
    {
        const float buoyancy_min = 0.0f;
        const float buoyancy_max = 2.0f * std::abs(Physics::g_environment.gravity);
        const float air_resistance_min = 0.0f;
        const float air_resistance_max = 1.0f;

        // find the buoyancy, assuming that the air_resistance of the particle
        // is equal to air_friction at standard gravity
        buoyancy = -getProfile()->spdlimit * (1.0f - Physics::g_environment.airfriction) - Physics::g_environment.gravity;
        buoyancy = CLIP(buoyancy, buoyancy_min, buoyancy_max);

        // reduce the buoyancy if the particle falls
        if (getProfile()->spdlimit > 0.0f) buoyancy *= 0.5f;

        // determine if there is any left-over air resistance
        if (std::abs(getProfile()->spdlimit) > 0.0001f)
        {
            air_resistance = 1.0f - (buoyancy + Physics::g_environment.gravity) / -getProfile()->spdlimit;
            air_resistance = CLIP(air_resistance, air_resistance_min, air_resistance_max);

            air_resistance /= Physics::g_environment.airfriction;
            air_resistance = CLIP(air_resistance, 0.0f, 1.0f);
        }
        else
        {
            air_resistance = 0.0f;
        }
    }

    endspawn_characterstate = SPAWNNOCHARACTER;

    //Set starting size
    setSize(getProfile()->size_base);

#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)

    // some code to track all allocated particles, where they came from, how long they are going to last,
    // what they are being used for...
    log_debug( "spawn_one_particle() - spawned a particle %d\n"
        "\tupdate == %d, remaining life == %d\n"
        "\towner == %d (\"%s\")\n"
        "\tparticleProfile == %d(\"%s\")\n"
        "\t\t%s"
        "\tobjectProfile == %d(\"%s\")\n"
        "\n",
        _particleID,
        update_wld, static_cast<int>(lifetime_remaining),
        loc_chr_origin, _currentModule->getObjectHandler().exists( loc_chr_origin ) ? _currentModule->getObjectHandler().get(loc_chr_origin)->Name : "INVALID",
        _particleProfileID, getProfile()->getName().c_str(), 
        getProfile()->comment,
        _spawnerProfile, ProfileSystem::get().isValidProfileID(_spawnerProfile) ? ProfileSystem::get().getProfile(_spawnerProfile)->getPathname().c_str() : "INVALID");
#endif

    //Attach ourselves to an Object if needed
    if (INVALID_CHR_REF != _attachedTo)
    {
        attach(_attachedTo);
    }

    //Spawn sound effect
    playSound(getProfile()->soundspawn);

    return true;
}

bool Particle::attach(const CHR_REF attach)
{
    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[attach];
    if(!pchr) {
        return false;
    }

    _attachedTo = attach;

    if(!placeAtVertex(pchr, attachedto_vrt_off)) {
        return false;
    }

    // Correct facing so swords knock characters in the right direction...
    if ( HAS_SOME_BITS( getProfile()->damfx, DAMFX_TURN ) )
    {
        facing = pchr->ori.facing_z;
    }

    return true;
}

bool Particle::placeAtVertex(const std::shared_ptr<Object> &object, int vertex_offset)
{
    int     vertex;
    fvec4_t point[1], nupoint[1];

    // Check validity of attachment
    if (object->isInsideInventory()) {
        requestTerminate();
        return false;
    }

    // Do we have a matrix???
    if ( !chr_matrix_valid(object.get()) )
    {
        chr_update_matrix(object.get(), true);
    }

    // Do we have a matrix???
    if ( chr_matrix_valid(object.get()) )
    {
        // Transform the weapon vertex_offset from model to world space

        if ( vertex_offset == GRIP_ORIGIN )
        {
            fvec3_t tmp_pos;
            tmp_pos[kX] = object->inst.matrix( 0, 3 );
            tmp_pos[kY] = object->inst.matrix( 1, 3 );
            tmp_pos[kZ] = object->inst.matrix( 2, 3 );

            setPosition(tmp_pos);

            return true;
        }

        if(vertex_offset > object->inst.vrt_count) {
            throw std::invalid_argument("Particle::placeAtVertex() =  vertex_offset > object->inst.vrt_count");
        }

        vertex = object->inst.vrt_count - vertex_offset;

        // do the automatic update
        chr_instance_t::update_vertices(object->inst, vertex, vertex, false );

        // Calculate vertex_offset point locations with linear interpolation and other silly things
        point[0][kX] = object->inst.vrt_lst[vertex].pos[XX];
        point[0][kY] = object->inst.vrt_lst[vertex].pos[YY];
        point[0][kZ] = object->inst.vrt_lst[vertex].pos[ZZ];
        point[0][kW] = 1.0f;

        // Do the transform
        object->inst.matrix.transform(point, nupoint, 1);

        setPosition(fvec3_t(nupoint[0][kX],nupoint[0][kY],nupoint[0][kZ]));
    }
    else
    {
        // No matrix, so just wing it...
        setPosition(object->getPosition());
    }

    return true;
}

const std::shared_ptr<Object>& Particle::getTarget() const
{
    return _currentModule->getObjectHandler()[_target];
}

bool Particle::isOverWater() const
{
    return (0 != ego_mesh_t::test_fx(_currentModule->getMeshPointer(), getTile(), MAPFX_WATER));
}

void Particle::setTarget(const CHR_REF target)
{
    _target = target;
}

PRT_REF Particle::getParticleID() const 
{
    return _particleID;
}

void Particle::setHoming(bool homing)
{
    _isHoming = homing;
}

} //Ego
