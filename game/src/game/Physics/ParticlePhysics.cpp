#include "game/Physics/ParticlePhysics.hpp"
#include "game/Entities/_Include.hpp"
#include "game/Physics/PhysicalConstants.hpp"
#include "game/CharacterMatrix.h"

namespace Ego
{
namespace Physics
{

ParticlePhysics::ParticlePhysics(Ego::Particle &particle) :
	_particle(particle)
{
	//ctor
}

void ParticlePhysics::updatePhysics()
{
    // if the particle is hidden it is frozen in time. do nothing.
    if (_particle.isTerminated() || _particle.isHidden()) {
    	return;
    }

    // save the acceleration from the last time-step
    _particle.enviro.acc = _particle.vel - _particle.vel_old;

    // determine the actual velocity for attached particles
    if (_particle.isAttached()) {
        _particle.vel = _particle.getPosition() - _particle.getOldPosition();
    }

    // Store particle's old location
    _particle.setOldPosition(_particle.getPosition());
    _particle.vel_old = _particle.vel;

    // what is the local environment like?
    updateEnviroment();

    // do friction with the floor before voluntary motion
    updateFloorFriction();

    updateHoming();

    //Update gravitational pull of particle (if any)
    updateGravity();

    //Different physics depending if we are attached to an Object or not
    if (_particle.isAttached()) {
        return updateAttached();
    }
    else {
	    updateMovement();
    }
}

void ParticlePhysics::updateMovement()
{
    Ego::prt_environment_t *penviro = &(_particle.enviro);

    //capture the position
    Vector3f tmp_pos = _particle.getPosition();

    auto mesh = _currentModule->getMeshPointer();

    bool hit_a_floor = false;
    bool hit_a_wall = false;
    bool touch_a_floor = false;
    bool touch_a_wall = false;

    Vector3f nrm_total;
    nrm_total.setZero();

    // Move the particle
    float ftmp = tmp_pos.z();
    tmp_pos.z() += _particle.vel.z();
    LOG_NAN(tmp_pos.z());

    //Are we touching the floor?
    if (tmp_pos.z() < penviro->adj_level)
    {
        Vector3f floor_nrm = Vector3f(0.0f, 0.0f, 1.0f);

        touch_a_floor = true;

        uint8_t tmp_twist = mesh->get_fan_twist(_particle.getTile());

        if (TWIST_FLAT != tmp_twist)
        {
            floor_nrm = g_meshLookupTables.twist_nrm[penviro->twist];
        }

        float vel_dot = floor_nrm.dot(_particle.vel);

        //Handle bouncing
        if (_particle.vel.z() < -STOPBOUNCINGPART)
        {
            // the particle will bounce
            nrm_total += floor_nrm;

            // take reflection in the floor into account when computing the new level
            tmp_pos.z() = penviro->adj_level + (penviro->adj_level - ftmp) * _particle.getProfile()->dampen + 0.1f;

            _particle.vel.z() = -_particle.vel.z();

            hit_a_floor = true;
        }
        else if (vel_dot > 0.0f)
        {
            // the particle is not bouncing, it is just at the wrong height
            tmp_pos.z() = penviro->adj_level + 0.1f;
        }
        else
        {
            // the particle is in the "stop bouncing zone"
            tmp_pos.z() = penviro->adj_level + 0.1f;
            _particle.vel.z() = 0.0f;
        }
    }

    // handle the sounds
    if (hit_a_floor)
    {
        // Play the sound for hitting the floor [FSND]
        _particle.playSound(_particle.getProfile()->end_sound_floor);
    }

    // handle the collision
    if (touch_a_floor && _particle.getProfile()->end_ground)
    {
        _particle.requestTerminate();
        return;
    }

    // interaction with the mesh walls
    hit_a_wall = false;
    if (std::abs(_particle.vel.x()) + std::abs(_particle.vel.y()) > 0.0f)
    {
        tmp_pos.x() += _particle.vel.x();
        tmp_pos.y() += _particle.vel.y();

        //Hitting a wall?
        if (EMPTY_BIT_FIELD != _particle.test_wall(tmp_pos))
        {
            Vector2f nrm;
            float   pressure;

            // how is the character hitting the wall?
            if (EMPTY_BIT_FIELD != _particle.hit_wall(tmp_pos, nrm, &pressure))
            {
                touch_a_wall = true;

                nrm_total.x() += nrm.x();
                nrm_total.y() += nrm.y();

                hit_a_wall = (Vector2f(_particle.vel.x(), _particle.vel.y()).dot(nrm) < 0.0f);
            }
        }
    }

    // handle the sounds
    if (hit_a_wall)
    {
        // Play the sound for hitting the wall [WSND]
        _particle.playSound(_particle.getProfile()->end_sound_wall);
    }

    // handle the collision
    if (touch_a_wall)
    {
        //End particle if it hits a wall?
        if(_particle.getProfile()->end_wall)
        {
            _particle.requestTerminate();
            return;            
        }
    }

    // do the reflections off the walls and floors
    if (hit_a_wall || hit_a_floor)
    {

        if ((hit_a_wall && (_particle.vel.x() * nrm_total.x() + _particle.vel.y() * nrm_total.y()) < 0.0f) ||
            (hit_a_floor && (_particle.vel.z() * nrm_total.z()) < 0.0f))
        {
            float vdot;
            Vector3f vpara, vperp;

            nrm_total.normalize();

            vdot = nrm_total.dot(_particle.vel);

            vperp = nrm_total * vdot;

            vpara = _particle.vel - vperp;

            // do the reflection
            vperp *= -_particle.getProfile()->dampen;

            // fake the friction, for now
            if (0.0f != nrm_total.y() || 0.0f != nrm_total.z())
            {
                vpara.x() *= _particle.getProfile()->dampen;
            }

            if (0.0f != nrm_total.x() || 0.0f != nrm_total.z())
            {
                vpara.y() *= _particle.getProfile()->dampen;
            }

            if (0.0f != nrm_total.x() || 0.0f != nrm_total.y())
            {
                vpara.z() *= _particle.getProfile()->dampen;
            }

            // add the components back together
            _particle.vel = vpara + vperp;
        }

        if (nrm_total.z() != 0.0f && _particle.vel.z() < STOPBOUNCINGPART)
        {
            // this is the very last bounce
            _particle.vel.z() = 0.0f;
            tmp_pos.z() = penviro->adj_level + 0.0001f;
        }

        if (hit_a_wall)
        {
            float fx, fy;

            // fix the facing
            facing_to_vec(_particle.facing, &fx, &fy);

            if (0.0f != nrm_total.x())
            {
                fx *= -1;
            }

            if (0.0f != nrm_total.y())
            {
                fy *= -1;
            }

            _particle.facing = vec_to_facing(fx, fy);
        }
    }

    //Don't fall in pits...
    if (_particle.isHoming()) {
        tmp_pos.z() = std::max(tmp_pos.z(), 0.0f);
    }

    //Rotate particle to the direction we are moving
    if (_particle.getProfile()->rotatetoface)
    {
        if (std::abs(_particle.vel.x()) + std::abs(_particle.vel.y()) > FLT_EPSILON)
        {
            // use velocity to find the angle
            _particle.facing = vec_to_facing(_particle.vel.x(), _particle.vel.y());
        }
        else if (_particle.hasValidTarget())
        {
            const std::shared_ptr<Object> &ptarget = _particle.getTarget();

            // face your target
            _particle.facing = vec_to_facing(ptarget->getPosX() - tmp_pos.x(), ptarget->getPosY() - tmp_pos.y());
        }
    }

    _particle.setPosition(tmp_pos);
}

void ParticlePhysics::updateAttached()
{
    Ego::prt_environment_t *penviro = &(_particle.enviro);

    // if the particle is not still in "display mode" there is no point in going on
    if (_particle.isTerminated()) return;

    // handle floor collision
    if (_particle.getPosition().z() < penviro->adj_level)
    {
        // Play the sound for hitting the floor [FSND]
        _particle.playSound(_particle.getProfile()->end_sound_floor);

        if(_particle.getProfile()->end_ground)
        {
            _particle.requestTerminate();
        }
    }

    // interaction with the mesh walls
    if (std::abs(_particle.vel.x()) + std::abs(_particle.vel.y()) > 0.0f)
    {
        if (EMPTY_BIT_FIELD != _particle.test_wall(_particle.getPosition()))
        {
            Vector2f nrm;
            float   pressure;

            // how is the character hitting the wall?
            BIT_FIELD hit_bits = _particle.hit_wall(_particle.getPosition(), nrm, &pressure);

		    // handle the collision
            if (0 != hit_bits)
            {
		        if(_particle.getProfile()->end_wall || _particle.getProfile()->end_bump)
		        {
		            _particle.requestTerminate();
		        }
            }
        }
    }
}

void ParticlePhysics::updateHoming()
{
    // is the particle a homing type?
    if (!_particle.getProfile()->homing) return;

    // the particle update function is supposed to turn homing off if the particle looses its target
    if (!_particle.isHoming()) return;

    // the _particle.isHoming() variable is supposed to track the following, but it could have lost synch by this point
    if (_particle.isAttached() || !_particle.hasValidTarget()) return;

    // grab a pointer to the target
    const std::shared_ptr<Object> &ptarget = _particle.getTarget();

    Vector3f vdiff = ptarget->getPosition() - _particle.getPosition();
    vdiff.z() += ptarget->bump.height * 0.5f;

    float min_length = 2 * 5 * 256 * (FLOAT_TO_FP8(_currentModule->getObjectHandler().get(_particle.owner_ref)->getAttribute(Ego::Attribute::INTELLECT)) / (float)PERFECTBIG);

    // make a little incertainty about the target
    float uncertainty = 256.0f * (1.0f - FLOAT_TO_FP8(_currentModule->getObjectHandler().get(_particle.owner_ref)->getAttribute(Ego::Attribute::INTELLECT)) / (float)PERFECTBIG);

    Vector3f vdither;
    int ival;

    ival = Random::next(std::numeric_limits<uint16_t>::max());
    vdither.x() = (((float)ival / 0x8000) - 1.0f)  * uncertainty;

    ival = Random::next(std::numeric_limits<uint16_t>::max());
    vdither.y() = (((float)ival / 0x8000) - 1.0f)  * uncertainty;

    ival = Random::next(std::numeric_limits<uint16_t>::max());
    vdither.z() = (((float)ival / 0x8000) - 1.0f)  * uncertainty;

    // take away any dithering along the direction of motion of the particle
    float vlen = _particle.vel.length_2();
    if (vlen > 0.0f)
    {
        float vdot = vdither.dot(_particle.vel) / vlen;

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

    _particle.vel = (_particle.vel + vdiff * _particle.getProfile()->homingaccel) * _particle.getProfile()->homingfriction;
}

void ParticlePhysics::updateFloorFriction()
{
    Vector3f vup;

    Ego::prt_environment_t *penviro = &(_particle.enviro);

    // if the particle is homing in on something, ignore friction
    if (_particle.isHoming()) return;

    // limit floor friction effects to solid objects
    if (SPRITE_SOLID != _particle.type) return;

    // figure out the acceleration due to the current "floor"
    Vector3f floor_acc = Vector3f::zero();
    float temp_friction_xy = 1.0f;

    const std::shared_ptr<Object> &platform = _currentModule->getObjectHandler()[_particle.onwhichplatform_ref];
    if (platform)
    {
        temp_friction_xy = 1.0f - PLATFORM_STICKINESS;

        floor_acc = platform->vel - platform->vel_old;

        chr_getMatUp(platform.get(), vup);
    }
    else
    {
        //Is the floor slippery?
        if (_currentModule->getMeshPointer()->grid_is_valid(_particle.getTile()) && penviro->is_slippy)
        {
            // It's slippy all right...
            temp_friction_xy = 1.0f - Ego::Physics::g_environment.slippyfriction;
        }
        else 
        {
            temp_friction_xy = 1.0f - Ego::Physics::g_environment.noslipfriction;
        }


        floor_acc = -_particle.vel;

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
    if (std::abs(vup.z()) > 0.9999f)
    {
        floor_acc.z() = 0.0f;
        fric.z() = 0.0f;
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
    _particle.vel += fric_floor * 0.25f;
}

void ParticlePhysics::updateGravity()
{
    //Only do world gravity for solid particles
    if (!_particle.no_gravity && _particle.type == SPRITE_SOLID && !_particle.isHoming() && !_particle.isAttached()) {
        _particle.vel.z() += Ego::Physics::g_environment.gravity * Ego::Physics::g_environment.airfriction;
    }

    //Some particles can have a special gravity field that pulls or pushes
    if(_particle.getProfile()->getGravityPull() != 0.0f) {
        float pullDistance = _particle.getProfile()->bump_size * 3.0f;
        const auto &particleTeam = _currentModule->getTeamList()[_particle.team];

        //Pull all nearby objects
        std::vector<std::shared_ptr<Object>> affectedObjects = _currentModule->getObjectHandler().findObjects(_particle.getPosX(), _particle.getPosY(), pullDistance, false);
        for(const std::shared_ptr<Object> &object : affectedObjects)
        {
            //Do not affect the object we are attached to
            if(_particle.getAttachedObject() == object) continue;

            //Allow friendly fire?
            if(!_particle.getProfile()->hateonly && !particleTeam.hatesTeam(object->getTeam())) continue;

            //Skip objects that cannot collide
            if(!object->canCollide()) continue;

            const Vector3f pull = _particle.getPosition() - object->getPosition();
            const float distance = pull.length_2();
            if(distance > 10.0f) {
                object->vel += (pull * _particle.getProfile()->getGravityPull()) * (1.0f/distance);
            }
        }

        //Pull all nearby particles
        for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
        {
            //Don't to terminated particles
            if(particle->isTerminated()) continue;

            //Skip attached particles
            if(particle->isAttached()) continue;

            //Do not affect ourselves!
            if(particle.get() == &_particle) continue;

            //Skip those that are not affected by gravity
            if(particle->no_gravity) continue;

            //Skip particles that cannot collide with anything
            if(!particle->canCollide()) continue;

            const Vector3f pull = _particle.getPosition() - particle->getPosition();
            const float distance = pull.length_2();
            if(distance > 10.0f) {
                particle->vel += (pull * _particle.getProfile()->getGravityPull()) * (1.0f/distance);
            }
        }
    }
}

void ParticlePhysics::updateEnviroment()
{
    float loc_level = 0.0f;

    Ego::prt_environment_t *penviro = &(_particle.enviro);

    const std::shared_ptr<Object>& platform = _currentModule->getObjectHandler()[_particle.onwhichplatform_ref];

    //---- character "floor" level
    penviro->floor_level = _currentModule->getMeshPointer()->getElevation(Vector2f(_particle.getPosX(), _particle.getPosY()));
    penviro->level = penviro->floor_level;

    //---- The actual level of the characer.
    //     Estimate platform attachment from whatever is in the onwhichplatform_ref variable from the
    //     last loop
    loc_level = penviro->floor_level;
    if (platform)
    {
        loc_level = std::max(penviro->floor_level, platform->getPosZ() +platform->chr_min_cv._maxs[OCT_Z]);
    }
    _particle.setElevation(loc_level);

    //---- the "twist" of the floor
    penviro->twist = TWIST_FLAT;
    Index1D itile = Index1D::Invalid;
    if (platform)
    {
        // this only works for 1 level of attachment
        itile = platform->getTile();
    }
    else
    {
        itile = _particle.getTile();
    }

    penviro->twist = _currentModule->getMeshPointer()->get_twist(itile);

    // the "watery-ness" of whatever water might be here
    penviro->is_watery = _currentModule->getWater()._is_water && penviro->inwater;
    penviro->is_slippy = !penviro->is_watery && (0 != _currentModule->getMeshPointer()->test_fx(_particle.getTile(), MAPFX_SLIPPY));

    //---- traction
    penviro->traction = 1.0f;
    if (_particle.isHoming())
    {
        // any traction factor here
        /* traction = ??; */
    }
    else if (platform)
    {
        // in case the platform is tilted
        // unfortunately platforms are attached in the collision section
        // which occurs after the movement section.

        Vector3f platform_up;

        chr_getMatUp(platform.get(), platform_up);
        platform_up.normalize();

        penviro->traction = std::abs(platform_up.z()) * (1.0f - penviro->zlerp) + 0.25f * penviro->zlerp;

        if (penviro->is_slippy)
        {
            penviro->traction /= Ego::Physics::g_environment.hillslide * (1.0f - penviro->zlerp) + 1.0f * penviro->zlerp;
        }
    }
    else if (_currentModule->getMeshPointer()->grid_is_valid(_particle.getTile()))
    {
        penviro->traction = std::abs(g_meshLookupTables.twist_nrm[penviro->twist].z()) * (1.0f - penviro->zlerp) + 0.25f * penviro->zlerp;

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
    if (!_particle.isHoming())
    {
        // Make the characters slide
        float temp_friction_xy = Ego::Physics::g_environment.noslipfriction;
        if (_currentModule->getMeshPointer()->grid_is_valid(_particle.getTile()) && penviro->is_slippy)
        {
            // It's slippy all right...
            temp_friction_xy = Ego::Physics::g_environment.slippyfriction;
        }

        penviro->friction_hrz = penviro->zlerp * 1.0f + (1.0f - penviro->zlerp) * temp_friction_xy;
    }
}

void ParticlePhysics::detachFromPlatform()
{
    // undo the attachment
    _particle.onwhichplatform_ref    = ObjectRef::Invalid;
    _particle.onwhichplatform_update = 0;
    _particle.targetplatform_ref     = ObjectRef::Invalid;
    _particle.targetplatform_level   = -1e32;

    // get the correct particle environment
    updateEnviroment();
}

} //Physics
} //Ego
