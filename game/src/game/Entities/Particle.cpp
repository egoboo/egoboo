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

namespace Ego
{

Particle::Particle(PRT_REF ref) :
    _particleID(ref),
    _attachedTo(INVALID_CHR_REF),
    _particleProfileID(INVALID_PIP_REF),
    _particleProfile(nullptr),
    _isTerminated(true),
    _target(INVALID_CHR_REF),
    _isHoming(false)
{
    reset();
}

void Particle::reset() 
{
    _isTerminated = true;
    is_ghost = false;

    _particleProfileID = INVALID_PRO_REF;
    _particleProfile = nullptr;

    _attachedTo = INVALID_CHR_REF;
    owner_ref = INVALID_CHR_REF;
    _target = INVALID_CHR_REF;
    parent_ref = INVALID_PRT_REF;

    attachedto_vrt_off = 0;
    type = 0;
    facing = 0;
    team = 0;

    _image.reset();

    vel_stt = fvec3_t::zero();

    PhysicsData::reset(this);

    offset = fvec3_t::zero();

    rotate = 0;
    rotate_add = 0;

    size_stt = 0;
    size = 0;
    size_add = 0;

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

    // some data that needs to be copied from the particle profile
    endspawn_amount = 0;         ///< The number of particles to be spawned at the end
    endspawn_facingadd = 0;      ///< The angular spacing for the end spawn
    endspawn_lpip = LocalParticleProfileRef::Invalid; ///< The actual local pip that will be spawned at the end
    endspawn_characterstate = 0; ///< if != SPAWNNOCHARACTER, then a character is spawned on end

    dynalight.reset();
    inst.reset();
    enviro.reset();
}


bool Particle::isAttached() const
{
    return _currentModule->getObjectHandler().exists(_attachedTo);
}

bool Particle::setPosition(const fvec3_t& position)
{
    EGO_DEBUG_VALIDATE(position);

    /// Has our position changed?
    if (position != this->pos)
    {
        this->pos = position;

        this->_tile = ego_mesh_t::get_grid(_currentModule->getMeshPointer(), PointWorld(this->pos[kX], this->pos[kY])).getI();
        this->_block = ego_mesh_t::get_block(_currentModule->getMeshPointer(), PointWorld(this->pos[kX], this->pos[kY])).getI();

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
            float mag = realSize / this->bump_real.size;

            // resize all dimensions equally
            bump_real.size *= mag;
            bump_real.size_big *= mag;
            bump_real.height *= mag;
        }

        // make sure that the virtual bumper size is at least as big as what is in the pip file
        bump_padded.size     = std::max<float>(bump_real.size, getProfile()->bump_size);
        bump_padded.size_big = std::max<float>(bump_real.size_big, getProfile()->bump_size) * Ego::Math::sqrtTwo<float>();
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

    float loc_height = getScale() * std::max(FP8_TO_FLOAT(size), offset[kZ] * 0.5f);

    enviro.adj_level = enviro.level;
    enviro.adj_floor = enviro.floor_level;

    enviro.adj_level += loc_height;
    enviro.adj_floor += loc_height;

    // set the zlerp after we have done everything to the particle's level we care to
    enviro.zlerp = (pos[kZ] - enviro.adj_level) / PLATTOLERANCE;
    enviro.zlerp = Ego::Math::constrain(enviro.zlerp, 0.0f, 1.0f);
}

void Particle::load(PIP_REF profile)
{
    _particleProfileID = profile;
    _particleProfile = PipStack.get_ptr(_particleProfileID);
    if(!_particleProfile) {
        throw std::invalid_argument("profile == invalid");
    }
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
    return _currentModule->getObjectHandler()[_target] != nullptr;
}

void Particle::update()
{
    //Should never happen
    if(_isTerminated) {
        return;
    }

    // If the particle is hidden, there is nothing else to do.
    if(isHidden()) {
        return;
    }

    // Determine if a "homing" particle still has something to "home":
    // If its homing (according to its profile), is not attached to an object (yet),
    // and a target exists, then the particle will "home" that target.
    _isHoming = getProfile()->homing && !isAttached() && hasValidTarget();

    // Update the particle interaction with water.
    /// @todo This might end the particle, however, the test via the return value *sucks*.
    //if (!update_do_water()) return nullptr;

    // Update the particle animation.
    /// @todo This might end the particle, however, the test via the return value *sucks*.
    //if (!update_animation()) return nullptr;

    //if (!update_dynalight()) return NULL;

    //if (!update_timers()) return NULL;

    //do_contspawn();

    //if (!this->do_bump_damage()) return NULL;

    // If the particle is done updating, remove it from the game, but do not kill it
    if (!is_eternal && 0 == lifetime_remaining)
    {
        requestTerminate();
    }

}

} //Ego
