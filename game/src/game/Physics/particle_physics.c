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
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/renderer_3d.h"
#include "game/egoboo.h"
#include "game/mesh.h"
#include "game/Entities/ParticleHandler.hpp"
#include "game/Entities/ObjectHandler.hpp"
#include "game/Physics/PhysicalConstants.hpp"

static const float STOPBOUNCINGPART = 10.0f;        ///< To make particles stop bouncing

prt_bundle_t *prt_bundle_t::move_one_particle_get_environment()
{
    float loc_level = 0.0f;

    if (NULL == this->_prt_ptr) return NULL;
    Ego::Particle *loc_pprt = this->_prt_ptr;
    Ego::prt_environment_t *penviro = &(loc_pprt->enviro);

    //---- character "floor" level
    penviro->floor_level = _currentModule->getMeshPointer()->getElevation(Vector2f(loc_pprt->getPosX(), loc_pprt->getPosY()));
    penviro->level = penviro->floor_level;

    //---- The actual level of the characer.
    //     Estimate platform attachment from whatever is in the onwhichplatform_ref variable from the
    //     last loop
    loc_level = penviro->floor_level;
    if (_currentModule->getObjectHandler().exists(loc_pprt->onwhichplatform_ref))
    {
        loc_level = std::max(penviro->floor_level, _currentModule->getObjectHandler().get(loc_pprt->onwhichplatform_ref)->getPosZ() + _currentModule->getObjectHandler().get(loc_pprt->onwhichplatform_ref)->chr_min_cv._maxs[OCT_Z]);
    }
    loc_pprt->setElevation(loc_level);

    //---- the "twist" of the floor
    penviro->twist = TWIST_FLAT;
    Index1D itile = Index1D::Invalid;
    if (_currentModule->getObjectHandler().exists(loc_pprt->onwhichplatform_ref))
    {
        // this only works for 1 level of attachment
        itile = _currentModule->getObjectHandler().get(loc_pprt->onwhichplatform_ref)->getTile();
    }
    else
    {
        itile = loc_pprt->getTile();
    }

    penviro->twist = _currentModule->getMeshPointer()->get_twist(itile);

    // the "watery-ness" of whatever water might be here
    penviro->is_watery = water._is_water && penviro->inwater;
    penviro->is_slippy = !penviro->is_watery && (0 != _currentModule->getMeshPointer()->test_fx(loc_pprt->getTile(), MAPFX_SLIPPY));

    //---- traction
    penviro->traction = 1.0f;
    if (loc_pprt->isHoming())
    {
        // any traction factor here
        /* traction = ??; */
    }
    else if (_currentModule->getObjectHandler().exists(loc_pprt->onwhichplatform_ref))
    {
        // in case the platform is tilted
        // unfortunately platforms are attached in the collision section
        // which occurs after the movement section.

        Vector3f platform_up;

        chr_getMatUp(_currentModule->getObjectHandler().get(loc_pprt->onwhichplatform_ref), platform_up);
        platform_up.normalize();

        penviro->traction = std::abs(platform_up[kZ]) * (1.0f - penviro->zlerp) + 0.25f * penviro->zlerp;

        if (penviro->is_slippy)
        {
            penviro->traction /= Ego::Physics::g_environment.hillslide * (1.0f - penviro->zlerp) + 1.0f * penviro->zlerp;
        }
    }
    else if (_currentModule->getMeshPointer()->grid_is_valid(loc_pprt->getTile()))
    {
        penviro->traction = std::abs(g_meshLookupTables.twist_nrm[penviro->twist][kZ]) * (1.0f - penviro->zlerp) + 0.25f * penviro->zlerp;

        if (penviro->is_slippy)
        {
            penviro->traction /= Ego::Physics::g_environment.hillslide * (1.0f - penviro->zlerp) + 1.0f * penviro->zlerp;
        }
    }

    //---- the friction of the fluid we are in
    if (penviro->is_watery)
    {
        penviro->fluid_friction_vrt = Ego::Physics::g_environment.waterfriction;
        penviro->fluid_friction_hrz = Ego::Physics::g_environment.waterfriction;
    }
    else
    {
        penviro->fluid_friction_hrz = Ego::Physics::g_environment.airfriction;       // like real-life air friction
        penviro->fluid_friction_vrt = Ego::Physics::g_environment.airfriction;
    }

    //---- friction
    penviro->friction_hrz = 1.0f;
    if (!loc_pprt->isHoming())
    {
        // Make the characters slide
        float temp_friction_xy = Ego::Physics::g_environment.noslipfriction;
        if (_currentModule->getMeshPointer()->grid_is_valid(loc_pprt->getTile()) && penviro->is_slippy)
        {
            // It's slippy all right...
            temp_friction_xy = Ego::Physics::g_environment.slippyfriction;
        }

        penviro->friction_hrz = penviro->zlerp * 1.0f + (1.0f - penviro->zlerp) * temp_friction_xy;
    }

    return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_do_floor_friction()
{
    float temp_friction_xy;
    Vector3f vup;
    Vector3f floor_acc;

    Ego::Particle *loc_pprt = this->_prt_ptr;
    Ego::prt_environment_t *penviro = &(loc_pprt->enviro);

    // if the particle is homing in on something, ignore friction
    if (loc_pprt->isHoming()) return this;

    // limit floor friction effects to solid objects
    if (SPRITE_SOLID != loc_pprt->type) return this;

    // figure out the acceleration due to the current "floor"
    floor_acc[kX] = floor_acc[kY] = floor_acc[kZ] = 0.0f;
    temp_friction_xy = 1.0f;

    const std::shared_ptr<Object> &platform = _currentModule->getObjectHandler()[loc_pprt->onwhichplatform_ref];
    if (platform)
    {
        temp_friction_xy = 1.0f - PLATFORM_STICKINESS;

        floor_acc = platform->vel - platform->vel_old;

        chr_getMatUp(platform.get(), vup);
    }
    else
    {
        //Is the floor slippery?
        if (_currentModule->getMeshPointer()->grid_is_valid(loc_pprt->getTile()) && penviro->is_slippy)
        {
            // It's slippy all right...
            temp_friction_xy = 1.0f - Ego::Physics::g_environment.slippyfriction;
        }
        else 
        {
            temp_friction_xy = 1.0f - Ego::Physics::g_environment.noslipfriction;
        }


        floor_acc = -loc_pprt->vel;

        //Is floor flat or sloped?
        if (TWIST_FLAT == penviro->twist)
        {
            vup = Vector3f(0.0f, 0.0f, 1.0f);
        }
        else
        {
            vup = g_meshLookupTables.twist_nrm[penviro->twist];
        }
    }

    // the first guess about the floor friction
    Vector3f fric_floor = floor_acc * (1.0f - penviro->zlerp) * temp_friction_xy * penviro->traction;

    // the total "friction" due to the floor
    Vector3f fric = fric_floor + penviro->acc;

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

    // test to see if the particle has any more friction left?
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
    loc_pprt->vel += fric_floor * 0.25f;

    return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_do_homing()
{
    int       ival;
    float     vlen, min_length, uncertainty;
    Vector3f  vdiff, vdither;

    if (NULL == this->_prt_ptr) return NULL;
    Ego::Particle *loc_pprt = this->_prt_ptr;
    std::shared_ptr<pip_t> loc_ppip = this->_pip_ptr;

    // is the particle a homing type?
    if (!loc_pprt->getProfile()->homing) return this;

    // the particle update function is supposed to turn homing off if the particle looses its target
    if (!loc_pprt->isHoming()) return this;

    // the loc_pprt->isHoming() variable is supposed to track the following, but it could have lost synch by this point
    if (loc_pprt->isAttached() || !loc_pprt->hasValidTarget()) return this;

    // grab a pointer to the target
    const std::shared_ptr<Object> &ptarget = loc_pprt->getTarget();

    vdiff = ptarget->getPosition() - loc_pprt->getPosition();
    vdiff[kZ] += ptarget->bump.height * 0.5f;

    min_length = 2 * 5 * 256 * (FLOAT_TO_FP8(_currentModule->getObjectHandler().get(loc_pprt->owner_ref)->getAttribute(Ego::Attribute::INTELLECT)) / (float)PERFECTBIG);

    // make a little incertainty about the target
    uncertainty = 256.0f * (1.0f - FLOAT_TO_FP8(_currentModule->getObjectHandler().get(loc_pprt->owner_ref)->getAttribute(Ego::Attribute::INTELLECT)) / (float)PERFECTBIG);

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
    if (!this->_prt_ptr->no_gravity && this->_prt_ptr->type == SPRITE_SOLID && !this->_prt_ptr->isHoming() && !this->_prt_ptr->isAttached())
    {
        this->_prt_ptr->vel[kZ] += Ego::Physics::g_environment.gravity * Ego::Physics::g_environment.airfriction;
    }
    return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_integrate_motion_attached()
{
    bool touch_a_floor, hit_a_wall;
    Vector3f nrm_total;

    if (NULL == this->_prt_ptr) return NULL;
    Ego::Particle *loc_pprt = this->_prt_ptr;
    Ego::prt_environment_t *penviro = &(loc_pprt->enviro);

    // if the particle is not still in "display mode" there is no point in going on
    if (loc_pprt->isTerminated()) return this;

    // only deal with attached particles
    if (!loc_pprt->isAttached()) return this;

    touch_a_floor = false;
    hit_a_wall = false;
    nrm_total = Vector3f::zero();

    // Move the particle
    if (loc_pprt->getPosition()[kZ] < penviro->adj_level)
    {
        touch_a_floor = true;
    }

    // handle floor collision
    if (touch_a_floor)
    {
        // Play the sound for hitting the floor [FSND]
        loc_pprt->playSound(loc_pprt->getProfile()->end_sound_floor);

        if(loc_pprt->getProfile()->end_ground)
        {
            loc_pprt->requestTerminate();
            return nullptr;
        }
    }

    // interaction with the mesh walls
    hit_a_wall = false;
    if (std::abs(loc_pprt->vel[kX]) + std::abs(loc_pprt->vel[kY]) > 0.0f)
    {
        if (EMPTY_BIT_FIELD != loc_pprt->test_wall(loc_pprt->getPosition()))
        {
            Vector2f nrm;
            float   pressure;

            // how is the character hitting the wall?
            BIT_FIELD hit_bits = loc_pprt->hit_wall(loc_pprt->getPosition(), nrm, &pressure);

            if (0 != hit_bits)
            {
                hit_a_wall = true;
            }
        }
    }

    // handle the collision
    if (hit_a_wall)
    {
        if(loc_pprt->getProfile()->end_wall || loc_pprt->getProfile()->end_bump)
        {
            loc_pprt->requestTerminate();
            return nullptr;
        }
    }

    return this;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t *prt_bundle_t::move_one_particle_integrate_motion()
{
    float ftmp;
    Vector3f nrm_total;

    if (NULL == this->_prt_ptr) return NULL;

    Ego::Particle *loc_pprt = this->_prt_ptr;
    Ego::prt_environment_t *penviro = &(loc_pprt->enviro);

    // if the particle is not still in "display mode" there is no point in going on
    if (loc_pprt->isTerminated()) return this;

    // capture the position
    Vector3f tmp_pos = loc_pprt->getPosition();

    // no point in doing this if the particle thinks it's attached
    if (loc_pprt->isAttached()) {
        return this->move_one_particle_integrate_motion_attached();
    }

    auto mesh = _currentModule->getMeshPointer();
    if (!mesh) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == mesh");
    }

    bool hit_a_floor = false;
    bool hit_a_wall = false;
    bool touch_a_floor = false;
    bool touch_a_wall = false;
    nrm_total[kX] = nrm_total[kY] = nrm_total[kZ] = 0.0f;

    // Move the particle
    ftmp = tmp_pos[kZ];
    tmp_pos[kZ] += loc_pprt->vel[kZ];
    LOG_NAN(tmp_pos[kZ]);

    //Are we touching the floor?
    if (tmp_pos[kZ] < penviro->adj_level)
    {
        Vector3f floor_nrm = Vector3f(0.0f, 0.0f, 1.0f);

        touch_a_floor = true;

        uint8_t tmp_twist = mesh->get_fan_twist(loc_pprt->getTile());

        if (TWIST_FLAT != tmp_twist)
        {
            floor_nrm = g_meshLookupTables.twist_nrm[penviro->twist];
        }

        float vel_dot = floor_nrm.dot(loc_pprt->vel);
        //if (0.0f == vel_dot)
        //{
        //    vel_perp = Vector3f::zero();
        //    vel_para = loc_pprt->vel;
        //}
        //else
        //{
        //    vel_perp = floor_nrm * vel_dot;
        //    vel_para = loc_pprt->vel - vel_perp;
        //}

        //Handle bouncing
        if (loc_pprt->vel[kZ] < -STOPBOUNCINGPART)
        {
            // the particle will bounce
            nrm_total += floor_nrm;

            // take reflection in the floor into account when computing the new level
            tmp_pos[kZ] = penviro->adj_level + (penviro->adj_level - ftmp) * loc_pprt->getProfile()->dampen + 0.1f;

            loc_pprt->vel[kZ] = -loc_pprt->vel[kZ];

            hit_a_floor = true;
        }
        else if (vel_dot > 0.0f)
        {
            // the particle is not bouncing, it is just at the wrong height
            tmp_pos[kZ] = penviro->adj_level + 0.1f;
        }
        else
        {
            // the particle is in the "stop bouncing zone"
            tmp_pos[kZ] = penviro->adj_level + 0.1f;
            loc_pprt->vel[kZ] = 0.0f;
            //loc_pprt->vel = vel_para;
        }
    }

    // handle the sounds
    if (hit_a_floor)
    {
        // Play the sound for hitting the floor [FSND]
        loc_pprt->playSound(loc_pprt->getProfile()->end_sound_floor);
    }

    // handle the collision
    if (touch_a_floor && loc_pprt->getProfile()->end_ground)
    {
        loc_pprt->requestTerminate();
        return NULL;
    }

    // interaction with the mesh walls
    hit_a_wall = false;
    if (std::abs(loc_pprt->vel[kX]) + std::abs(loc_pprt->vel[kY]) > 0.0f)
    {
        tmp_pos[kX] += loc_pprt->vel[kX];
        tmp_pos[kY] += loc_pprt->vel[kY];

        //Hitting a wall?
        if (EMPTY_BIT_FIELD != loc_pprt->test_wall(tmp_pos))
        {
            Vector2f nrm;
            float   pressure;

            // how is the character hitting the wall?
            if (EMPTY_BIT_FIELD != loc_pprt->hit_wall(tmp_pos, nrm, &pressure))
            {
                touch_a_wall = true;

                nrm_total[kX] += nrm[XX];
                nrm_total[kY] += nrm[YY];

                hit_a_wall = (Vector2f(loc_pprt->vel[kX], loc_pprt->vel[kY]).dot(nrm) < 0.0f);
            }
        }
    }

    // handle the sounds
    if (hit_a_wall)
    {
        // Play the sound for hitting the wall [WSND]
        loc_pprt->playSound(loc_pprt->getProfile()->end_sound_wall);
    }

    // handle the collision
    if (touch_a_wall)
    {
        //End particle if it hits a wall?
        if(loc_pprt->getProfile()->end_wall)
        {
            loc_pprt->requestTerminate();
            return nullptr;            
        }
    }

    // do the reflections off the walls and floors
    if (hit_a_wall || hit_a_floor)
    {

        if ((hit_a_wall && (loc_pprt->vel[kX] * nrm_total[kX] + loc_pprt->vel[kY] * nrm_total[kY]) < 0.0f) ||
            (hit_a_floor && (loc_pprt->vel[kZ] * nrm_total[kZ]) < 0.0f))
        {
            float vdot;
            Vector3f vpara, vperp;

            nrm_total.normalize();

            vdot = nrm_total.dot(loc_pprt->vel);

            vperp = nrm_total * vdot;

            vpara = loc_pprt->vel - vperp;

            // we can use the impulse to determine how much velocity to kill in the parallel direction
            // imp = vperp * (1.0f + loc_pprt->getProfile()->dampen);

            // do the reflection
            vperp *= -loc_pprt->getProfile()->dampen;

            // fake the friction, for now
            if (0.0f != nrm_total[kY] || 0.0f != nrm_total[kZ])
            {
                vpara[kX] *= loc_pprt->getProfile()->dampen;
            }

            if (0.0f != nrm_total[kX] || 0.0f != nrm_total[kZ])
            {
                vpara[kY] *= loc_pprt->getProfile()->dampen;
            }

            if (0.0f != nrm_total[kX] || 0.0f != nrm_total[kY])
            {
                vpara[kZ] *= loc_pprt->getProfile()->dampen;
            }

            // add the components back together
            loc_pprt->vel = vpara + vperp;
        }

        if (nrm_total[kZ] != 0.0f && loc_pprt->vel[kZ] < STOPBOUNCINGPART)
        {
            // this is the very last bounce
            loc_pprt->vel[kZ] = 0.0f;
            tmp_pos[kZ] = penviro->adj_level + 0.0001f;
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
    if (loc_pprt->isHoming()) {
        tmp_pos[kZ] = std::max(tmp_pos[kZ], 0.0f);
    }

    //Rotate particle to the direction we are moving
    if (loc_pprt->getProfile()->rotatetoface)
    {
        if (std::abs(loc_pprt->vel[kX]) + std::abs(loc_pprt->vel[kY]) > FLT_EPSILON)
        {
            // use velocity to find the angle
            loc_pprt->facing = vec_to_facing(loc_pprt->vel[kX], loc_pprt->vel[kY]);
        }
        else if (loc_pprt->hasValidTarget())
        {
            const std::shared_ptr<Object> &ptarget = loc_pprt->getTarget();

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
    Ego::Particle *loc_pprt = this->_prt_ptr;
    Ego::prt_environment_t *penviro = &(loc_pprt->enviro);

    if (loc_pprt->isTerminated()) return false;

    // if the particle is hidden it is frozen in time. do nothing.
    if (loc_pprt->isHidden()) return false;

    // save the acceleration from the last time-step
    penviro->acc = loc_pprt->vel - loc_pprt->vel_old;

    // determine the actual velocity for attached particles
    if (loc_pprt->isAttached())
    {
        loc_pprt->vel = loc_pprt->getPosition() - loc_pprt->getOldPosition();
    }

    // Store particle's old location
    loc_pprt->setOldPosition(loc_pprt->getPosition());
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

    // move every particle
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        if(particle->isTerminated()) continue;

        // prime the environment
        particle->enviro.ice_friction = Ego::Physics::g_environment.icefriction;

        prt_bundle_t prt_bdl = prt_bundle_t(particle.get());

        prt_bdl.move_one_particle();
    }
}

//--------------------------------------------------------------------------------------------
int spawn_bump_particles(ObjectRef character, const ParticleRef particle)
{
    /// @author ZZ
    /// @details This function is for catching characters on fire and such

    int      cnt, bs_count;
    float    x, y, z;
    FACING_T facing;
    int      amount;
    FACING_T direction;
    float    fsin, fcos;

    const std::shared_ptr<Ego::Particle> &pprt = ParticleHandler::get()[iparticle];
    if(!pprt || pprt->isTerminated()) {
        return 0;
    }

    const std::shared_ptr<pip_t> &ppip = pprt->getProfile();

    // no point in going on, is there?
    if (0 == ppip->bumpspawn._amount && !ppip->spawnenchant) return 0;
    amount = ppip->bumpspawn._amount;

    if (!_currentModule->getObjectHandler().exists(character)) return 0;
    Object *pchr = _currentModule->getObjectHandler().get(character);

    bs_count = 0;

    // Only damage if hitting from proper direction
    direction = vec_to_facing(pprt->vel[kX], pprt->vel[kY]);
    direction = ATK_BEHIND + (pchr->ori.facing_z - direction);

    // Check that direction
    if (!pchr->isInvictusDirection(direction, ppip->damfx))
    {
        // Spawn new enchantments
        if (ppip->spawnenchant) 
        {
            const std::shared_ptr<ObjectProfile> &spawnerProfile = ProfileSystem::get().getProfile(pprt->getSpawnerProfile());
            pchr->addEnchant(spawnerProfile->getEnchantRef(), pprt->getSpawnerProfile(), _currentModule->getObjectHandler()[pprt->owner_ref], Object::INVALID_OBJECT);
        }

        // Spawn particles - this has been modded to maximize the visual effect
        // on a given target. It is not the most optimal solution for lots of particles
        // spawning. Thst would probably be to make the distance calculations and then
        // to quicksort the list and choose the n closest points.
        //
        // however, it seems that the bump particles in game rarely attach more than
        // one bump particle

        //check if we resisted the attack, we could resist some of the particles or none
        for (cnt = 0; cnt < amount; cnt++)
        {
            if (Random::nextFloat() <= pchr->getDamageReduction(pprt->damagetype)) amount--;
        }

        if (amount > 0 && !pchr->getProfile()->hasResistBumpSpawn() && !pchr->invictus)
        {
            int grip_verts, vertices;
            int slot_count;

            slot_count = 0;
            if (pchr->getProfile()->isSlotValid(SLOT_LEFT)) slot_count++;
            if (pchr->getProfile()->isSlotValid(SLOT_RIGHT)) slot_count++;

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
                TURN_T   turn;

				auto vertex_occupied = std::make_unique<ParticleRef[]>(vertices);
                auto vertex_distance = std::make_unique<float[]>(vertices);

                // this could be done more easily with a quicksort....
                // but I guess it doesn't happen all the time
                float dist = (pprt->getPosition() - pchr->getPosition()).length_abs();

                // clear the occupied list
                z = pprt->getPosZ() - pchr->getPosZ();
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
                    vertex_occupied[cnt] = ParticleRef::Invalid;
                }

                // determine if some of the vertex sites are already occupied
                for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
                {
                    if(particle->isTerminated()) continue;

                    if (pchr != particle->getAttachedObject().get()) continue;

                    if (particle->attachedto_vrt_off < vertices)
                    {
                        vertex_occupied[particle->attachedto_vrt_off] = particle->getParticleID();
                    }
                }

                    // Find best vertices to attach the particles to
                    for (cnt = 0; cnt < amount; cnt++)
                    {
                        Uint32  bestdistance;
                        int     bestvertex;

                        bestvertex = 0;
                        bestdistance = 0xFFFFFFFF;         //Really high number

                        for (cnt = 0; cnt < vertices; cnt++)
                        {
                            if (ParticleRef::Invalid != vertex_occupied[cnt])
                                continue;

                            if (vertex_distance[cnt] < bestdistance)
                            {
                                bestdistance = vertex_distance[cnt];
                                bestvertex = cnt;
                            }
                        }

                        std::shared_ptr<Ego::Particle> bs_part = 
							ParticleHandler::get().spawnLocalParticle(pchr->getPosition(), pchr->ori.facing_z, pprt->getSpawnerProfile(), ppip->bumpspawn._lpip,
                                                                      character, bestvertex + 1, pprt->team, pprt->owner_ref, particle, cnt, character);

                        if (bs_part)
                        {
                            vertex_occupied[bestvertex] = bs_part->getParticleID();
                            bs_part->is_bumpspawn = true;
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
            }
        }
    }

    return bs_count;
}

prt_bundle_t::prt_bundle_t()
    : _prt_ref(), _prt_ptr(nullptr),
      _pip_ref(INVALID_PIP_REF), _pip_ptr(nullptr) {
}

prt_bundle_t::prt_bundle_t(Ego::Particle *prt)
    : _prt_ref(), _prt_ptr(nullptr),
      _pip_ref(INVALID_PIP_REF), _pip_ptr(nullptr) {
    if (!prt) {
        throw std::invalid_argument("nullptr == prt");
    }
    _prt_ptr = prt;
    _prt_ref = _prt_ptr->getParticleID();

    _pip_ref = _prt_ptr->getProfileID();
    _pip_ptr = _prt_ptr->getProfile();
}

ObjectRef prt_get_iowner(const ParticleRef iprt, int depth)
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
        return ObjectRef::Invalid;
    }

    const std::shared_ptr<Ego::Particle> &pprt = ParticleHandler::get()[iprt];
    if(pprt == nullptr || pprt->isTerminated()) {
        return ObjectRef::Invalid;
    }

    ObjectRef iowner = ObjectRef::Invalid;
    if (_currentModule->getObjectHandler().exists(pprt->owner_ref))
    {
        iowner = pprt->owner_ref;
    }
    else
    {
        // make a check for a stupid looping structure...
        // cannot be sure you could never get a loop, though

        if (!ParticleHandler::get()[pprt->parent_ref])
        {
            // make sure that a non valid parent_ref is marked as non-valid
            pprt->parent_ref = ParticleRef::Invalid;
        }
        else
        {
            // if a particle has been poofed, and another particle lives at that address,
            // it is possible that the pprt->parent_ref points to a valid particle that is
            // not the parent. Depending on how scrambled the list gets, there could actually
            // be looping structures. I have actually seen this, so don't laugh :)
            if (iprt != pprt->parent_ref)
            {
                iowner = prt_get_iowner(pprt->parent_ref, depth + 1);
            }
        }
    }

    return iowner;
}
