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

/// @file  game/Physics/ObjectPhysics.cpp
/// @brief Code for handling object physics
/// @author Johan Jansen aka Zefz
#include "ObjectPhysics.hpp"
#include "game/Entities/_Include.hpp"
#include "game/Physics/ObjectPhysics.h"
#include "game/Core/GameEngine.hpp"
#include "game/game.h" //TODO: only for latches
#include "egolib/Graphics/ModelDescriptor.hpp"

namespace Ego
{
namespace Physics
{

ObjectPhysics::ObjectPhysics() :
    _platformOffset(0.0f, 0.0f),
    _desiredVelocity(0.0f, 0.0f),
    _traction(1.0f)
{
    //ctor
}

void ObjectPhysics::keepItemsWithHolder(const std::shared_ptr<Object> &pchr)
{
    const std::shared_ptr<Object> &holder = pchr->getHolder();
    if (holder)
    {
        // Keep in hand weapons with iattached
        if ( chr_matrix_valid(pchr.get()) )
        {
            pchr->setPosition(mat_getTranslate(pchr->inst.matrix));
        }
        else
        {
            pchr->setPosition(holder->getPosition());
        }

        pchr->ori.facing_z = holder->ori.facing_z;

        // Copy this stuff ONLY if it's a weapon, not for mounts
        if (holder->getProfile()->transferBlending() && pchr->isItem())
        {

            // Items become partially invisible in hands of players
            if (holder->isPlayer() && 255 != holder->inst.alpha)
            {
                pchr->setAlpha(SEEINVISIBLE);
            }
            else
            {
                // Only if not naturally transparent
                if (255 == pchr->getProfile()->getAlpha())
                {
                    pchr->setAlpha(holder->inst.alpha);
                }
                else
                {
                    pchr->setAlpha(pchr->getProfile()->getAlpha());
                }
            }

            // Do light too
            if (holder->isPlayer() && 255 != holder->inst.light)
            {
                pchr->setLight(SEEINVISIBLE);
            }
            else
            {
                // Only if not naturally transparent
                if (255 == pchr->getProfile()->getLight())
                {
                    pchr->setLight(holder->inst.light);
                }
                else
                {
                    pchr->setLight(pchr->getProfile()->getLight());
                }
            }
        }
    }
    else
    {
        pchr->attachedto = ObjectRef::Invalid;
    }
}

void ObjectPhysics::updateMovement(const std::shared_ptr<Object> &object)
{
    //Desired velocity in scaled space [-1 , 1]
    _desiredVelocity = Vector2f(0.0f, 0.0f);

    //Can it move?
    if (object->isAlive() && object->getAttribute(Ego::Attribute::ACCELERATION) > 0.0f)  {
        _desiredVelocity[kX] = object->latch.x;
        _desiredVelocity[kY] = object->latch.y;

        // Reverse movements for daze
        if (object->daze_timer > 0) {
            _desiredVelocity[kX] = -_desiredVelocity[kX];
            _desiredVelocity[kY] = -_desiredVelocity[kY];
        }

        // Switch x and y for grog
        if (object->grog_timer > 0) {
            std::swap(_desiredVelocity[kX], _desiredVelocity[kY]);
        }

        //Update which way we are looking
        updateFacing(object);
    }

    //Is there any movement going on?
    if(_desiredVelocity.length_abs() > 0.05f) {
        const float maxSpeed = getMaxSpeed(object.get());

        //Scale [-1 , 1] to velocity of the object
        _desiredVelocity *= maxSpeed;

        //Limit to max velocity
        if(_desiredVelocity.length() > maxSpeed) {
            _desiredVelocity *= maxSpeed / _desiredVelocity.length();
        }
    }
    else {
        //Try to stand still
        _desiredVelocity.setZero();
    }

    //Determine acceleration/deceleration
    Vector2f acceleration;
    acceleration.x() = (_desiredVelocity.x() - object->vel.x()) * (4.0f / GameEngine::GAME_TARGET_UPS);
    acceleration.y() = (_desiredVelocity.y() - object->vel.y()) * (4.0f / GameEngine::GAME_TARGET_UPS);

    //How good grip do we have to add additional momentum?
    acceleration *= _traction;

    //Finally apply acceleration to velocity
    object->vel.x() += acceleration.x();
    object->vel.y() += acceleration.y();
}

void ObjectPhysics::updateHillslide(const std::shared_ptr<Object> &pchr)
{
    //This makes it hard for characters to jump uphill
    if(pchr->vel.z() > 0.0f && pchr->enviro.is_slippy && !g_meshLookupTables.twist_flat[pchr->enviro.grid_twist]) {
        pchr->vel.z() *= 0.8f;
    }

    //Only slide if we are touching the floor
    if(pchr->enviro.grounded) {

        //Can the character slide on this floor?
        if (pchr->enviro.is_slippy && !pchr->getAttachedPlatform())
        {
            //Make characters slide down hills
            if(!g_meshLookupTables.twist_flat[pchr->enviro.grid_twist]) {
                const float hillslide = Ego::Physics::g_environment.hillslide * (1.0f - pchr->enviro.zlerp) * (1.0f - _traction);
                pchr->vel.x() += g_meshLookupTables.twist_nrm[pchr->enviro.grid_twist].x() * hillslide;
                pchr->vel.y() += g_meshLookupTables.twist_nrm[pchr->enviro.grid_twist].y() * hillslide;

                //Reduce traction while we are sliding downhill
                _traction *= 0.8f;
            }
            else {
                //Flat icy floor -> reduced traction
                _traction = 1.0f - Ego::Physics::g_environment.icefriction;
            }
        }
        else {
            //Reset traction
            _traction = 1.0f;
        }
    }
}

void ObjectPhysics::updatePhysics(const std::shared_ptr<Object> &pchr)
{
    //Update physical enviroment variables first
    move_one_character_get_environment(pchr.get());

    // Keep inventory items with the carrier
    if(pchr->isInsideInventory()) {
        pchr->setPosition(_currentModule->getObjectHandler()[pchr->inwhich_inventory]->getPosition());
        return;
    }

    //Is this character being held by another character?
    if(pchr->isBeingHeld()) {
        keepItemsWithHolder(pchr);
        move_one_character_do_animation(pchr.get());
        return;
    }

    // Character's old location
    pchr->vel_old          = pchr->vel;
    pchr->ori_old.facing_z = pchr->ori.facing_z;

    chr_do_latch_button(pchr.get());

    // do friction with the floor before voluntary motion
    updateHillslide(pchr);

    updateMovement(pchr);

    updatePlatformPhysics(pchr);

    //Apply gravity
    if(!pchr->isFlying()) {
        pchr->vel.z() += pchr->enviro.zlerp * Ego::Physics::g_environment.gravity;
    }
    else {
        pchr->vel.z() += (pchr->enviro.fly_level + pchr->getAttribute(Ego::Attribute::FLY_TO_HEIGHT) - pchr->getPosZ()) * FLYDAMPEN;
    }

    updateMeshCollision(pchr);

    //Cutoff for low velocities to make them truly stop
    if(pchr->vel.length_abs() < 0.05f) {
        pchr->vel.setZero();
    }

    move_one_character_do_animation(pchr.get());
}

float ObjectPhysics::getMaxSpeed(Object *object) const
{
    // this is the maximum speed that a character could go under the v2.22 system
    float maxspeed = object->getAttribute(Ego::Attribute::ACCELERATION) * Ego::Physics::g_environment.airfriction / (1.0f - Ego::Physics::g_environment.airfriction);
    float speedBonus = 1.0f;

    //Sprint perk gives +10% movement speed if above 75% life remaining
    if(object->hasPerk(Ego::Perks::SPRINT) && object->getLife() >= object->getAttribute(Ego::Attribute::MAX_LIFE)*0.75f) {
        speedBonus += 0.1f;

        //Uninjured? (Dash perk can give another 10% extra speed)
        if(object->hasPerk(Ego::Perks::DASH) && object->getAttribute(Ego::Attribute::MAX_LIFE)-object->getLife() < 1.0f) {
            speedBonus += 0.1f;
        }
    }

    //Rally Bonus? (+10%)
    if(object->hasPerk(Ego::Perks::RALLY) && update_wld < object->getRallyDuration()) {
        speedBonus += 0.1f;
    }    

    //Increase movement by 1% per Agility above 10 (below 10 agility reduces movement speed!)
    speedBonus += (object->getAttribute(Ego::Attribute::AGILITY)-10.0f) * 0.01f;

    //Now apply speed modifiers
    maxspeed *= speedBonus;

    //Are we in water?
    if(object->isInWater(true)) {
        if(object->hasPerk(Ego::Perks::ATHLETICS)) {
            maxspeed *= 0.25f; //With athletics perk we can have three-quarters speed
        }
        else {
            maxspeed *= 0.5f; //Half speed in water
        }
    }

    //Check animation frame freeze movement
    if ( chr_instance_t::get_framefx(object->inst) & MADFX_STOP )
    {
        //Allow 50% movement while using Shield and have the Mobile Defence perk
        if(object->hasPerk(Ego::Perks::MOBILE_DEFENCE) && ACTION_IS_TYPE(object->inst.action_which, P))
        {
            maxspeed *= 0.5f;
        }
        //Allow 50% movement with Mobility perk and attacking with a weapon
        else if(object->hasPerk(Ego::Perks::MOBILITY) && object->isAttacking())
        {
            maxspeed *= 0.5f;
        }
        else
        {
            //No movement allowed
            maxspeed = 0.0f;
        }
    }

    //Check if AI has limited movement rate
    else if(!object->isPlayer())
    {
        maxspeed *= object->ai.maxSpeed;
    }

    bool sneak_mode_active = object->isStealthed();

    //Reduce speed while stealthed
    if(object->isStealthed()) {
        if(object->hasPerk(Ego::Perks::SHADE)) {
            maxspeed *= 0.75f;  //Shade allows 75% movement speed while stealthed
        }
        else if(object->hasPerk(Ego::Perks::STALKER)) {
            maxspeed *= 0.50f;  //Stalker allows 50% movement speed while stealthed
        }
        else {
            maxspeed *= 0.33f;  //Can only move at 33% speed while stealthed
        }
    }

    if ( sneak_mode_active )
    {
        // sneak mode
        object->movement_bits = CHR_MOVEMENT_BITS_SNEAK | CHR_MOVEMENT_BITS_STOP;
    }
    else
    {
        // non-sneak mode
        object->movement_bits = ( unsigned )( ~CHR_MOVEMENT_BITS_SNEAK );
    }

    return maxspeed;    
}

void ObjectPhysics::updateFacing(const std::shared_ptr<Object> &pchr)
{
    //Figure out how to turn around
    switch ( pchr->turnmode )
    {
        // Get direction from ACTUAL change in velocity
        default:
        case TURNMODE_VELOCITY:
            {
                if (_desiredVelocity.length_abs() > TURNSPD)
                {
                    if (pchr->isPlayer())
                    {
                        // Players turn quickly
                        pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing(_desiredVelocity.x(), _desiredVelocity.y()), 2 );
                    }
                    else
                    {
                        // AI turn slowly
                        pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing(_desiredVelocity.x(), _desiredVelocity.y()), 8 );
                    }
                }
            }
            break;

        // Get direction from the DESIRED change in velocity
        case TURNMODE_WATCH:
            {
                if (_desiredVelocity.length_abs() > WATCHMIN )
                {
                    pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing(_desiredVelocity.x(), _desiredVelocity.y()), 8 );
                }
            }
            break;

        // Face the target
        case TURNMODE_WATCHTARGET:
            {
                if ( pchr->getObjRef() != pchr->ai.getTarget() )
                {
                    pchr->ori.facing_z = static_cast<int>(pchr->ori.facing_z) + terp_dir( pchr->ori.facing_z, vec_to_facing( _currentModule->getObjectHandler().get(pchr->ai.getTarget())->getPosX() - pchr->getPosX() , _currentModule->getObjectHandler().get(pchr->ai.getTarget())->getPosY() - pchr->getPosY() ), 8 );
                }
            }
            break;

        // Otherwise make it spin
        case TURNMODE_SPIN:
            {
                pchr->ori.facing_z += SPINRATE;
            }
            break;
    }
}

void ObjectPhysics::detachFromPlatform(Object* pchr)
{
    // adjust the platform weight, if necessary
    if(pchr->getAttachedPlatform()) {
        pchr->getAttachedPlatform()->holdingweight -= pchr->phys.weight;
    }

    // undo the attachment
    pchr->onwhichplatform_ref    = ObjectRef::Invalid;
    pchr->onwhichplatform_update = 0;
    pchr->targetplatform_ref     = ObjectRef::Invalid;
    pchr->targetplatform_level   = -1e32;
    _platformOffset.setZero();

    // update the character-platform properties
    move_one_character_get_environment(pchr);

    // update the character jumping
    pchr->jumpready = pchr->enviro.grounded;
    if ( pchr->jumpready ) {
        pchr->jumpnumber = pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS);
    }
}

bool ObjectPhysics::attachToPlatform(const std::shared_ptr<Object> &object, const std::shared_ptr<Object> &platform)
{
    // check if they can be connected
    if(!object->canuseplatforms || object->isFlying() || !platform->platform) {
        return false;
    }

    // do the attachment
    object->onwhichplatform_ref    = platform->getObjRef();
    object->onwhichplatform_update = update_wld;
    object->targetplatform_ref     = ObjectRef::Invalid;

    _platformOffset.x() = object->getPosX() - platform->getPosX();
    _platformOffset.y() = object->getPosY() - platform->getPosY();

    //Make sure the object is now on top of the platform
    const float platformElevation = platform->getPosZ() + platform->chr_min_cv._maxs[OCT_Z];
    if(object->getPosZ() < platformElevation) {
        object->setPosition(object->getPosX(), object->getPosY(), platformElevation);
    }

    // update the character's relationship to the ground
    object->enviro.level     = std::max(object->enviro.floor_level, platformElevation);
    object->enviro.zlerp     = (object->getPosZ() - object->enviro.level) / PLATTOLERANCE;
    object->enviro.zlerp     = Ego::Math::constrain(object->enviro.zlerp, 0.0f, 1.0f);
    object->enviro.grounded  = !object->isFlying() && ( object->enviro.zlerp < 0.25f );

    object->enviro.fly_level = std::max(object->enviro.fly_level, object->enviro.level);
    if (object->enviro.fly_level < 0) object->enviro.fly_level = 0;  // fly above pits...

    // add the weight to the platform
    platform->holdingweight += object->phys.weight;

    // update the character jumping
    if (object->enviro.grounded)
    {
        object->jumpready = true;
        object->jumpnumber = object->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS);
    }

    // tell the platform that we bumped into it
    // this is necessary for key buttons to work properly, for instance
    ai_state_t::set_bumplast(platform->ai, object->getObjRef());

    return true;
}

void ObjectPhysics::updatePlatformPhysics(const std::shared_ptr<Object> &object)
{
    const std::shared_ptr<Object> &platform = object->getAttachedPlatform();
    if(!platform) {
        return;
    }

    // grab the pre-computed zlerp value, and map it to our needs
    float lerp_z = 1.0f - object->enviro.zlerp;

    // if your velocity is going up much faster than the
    // platform, there is no need to suck you to the level of the platform
    // this was one of the things preventing you from jumping from platforms
    // properly
    if(std::abs(object->vel.z() - platform->vel.z()) / 5.0f <= PLATFORM_STICKINESS) {
        object->vel.z() += (platform->vel.z()  - object->vel.z()) * lerp_z;
    }

    // determine the rotation rates
    int16_t rot_b = object->ori.facing_z - object->ori_old.facing_z;
    int16_t rot_a = platform->ori.facing_z - platform->ori_old.facing_z;
    object->ori.facing_z += (rot_a - rot_b) * PLATFORM_STICKINESS;    

    //Allows movement on the platform
    _platformOffset.x() += object->vel.x();
    _platformOffset.y() += object->vel.y();

    //Inherit position of platform with given offsets
    float zCorrection = (object->enviro.level - object->getPosZ()) * 0.125f * lerp_z;
    object->setPosition(platform->getPosX() + _platformOffset.x(), platform->getPosY() + _platformOffset.y(), object->getPosZ() + zCorrection);
}

void ObjectPhysics::updateMeshCollision(const std::shared_ptr<Object> &pchr)
{
    Vector3f tmp_pos = pchr->getPosition();;

    // interaction with the mesh
    //if ( std::abs( pchr->vel[kZ] ) > 0.0f )
    {
        const float floorElevation = pchr->enviro.floor_level + RAISE;

        tmp_pos.z() += pchr->vel.z();
        if (tmp_pos.z() <= floorElevation)
        {
            //We have hit the ground
            if(!pchr->isFlying()) {
                pchr->enviro.grounded = true;
            }

            if (std::abs(pchr->vel.z()) < Ego::Physics::STOP_BOUNCING)
            {
                pchr->vel.z() = 0.0f;
                tmp_pos.z() = floorElevation;
            }
            else
            {
                //Make it bounce!
                if (pchr->vel.z() < 0.0f)
                {
                    float diff = floorElevation - tmp_pos.z();

                    pchr->vel.z() *= -pchr->phys.bumpdampen;
                    diff          *= -pchr->phys.bumpdampen;

                    tmp_pos.z() = std::max(tmp_pos.z() + diff, floorElevation);
                }
                else
                {
                    tmp_pos.z() = floorElevation;
                }
            }
        }
    }

    // fixes to the z-position
    if (pchr->isFlying())
    {
        // Don't fall in pits...
        if (tmp_pos.z() < 0.0f) {
            tmp_pos.z() = 0.0f;
        }
    }


    //if (std::abs(pchr->vel[kX]) + std::abs(pchr->vel[kY]) > 0.0f)
    {
        float old_x = tmp_pos.x();
        float old_y = tmp_pos.y();

        float new_x = old_x + pchr->vel.x();
        float new_y = old_y + pchr->vel.y();

        tmp_pos.x() = new_x;
        tmp_pos.y() = new_y;

        //Wall collision?
        if ( EMPTY_BIT_FIELD != pchr->test_wall( tmp_pos ) )
        {            
            Vector2f nrm;
            float   pressure;

            pchr->hit_wall(tmp_pos, nrm, &pressure );

            // how is the character hitting the wall?
            if (pressure > 0.0f)
            {
                tmp_pos.x() -= pchr->vel.x();
                tmp_pos.y() -= pchr->vel.y();

                const float bumpdampen = std::max(0.1f, 1.0f-pchr->phys.bumpdampen);

                //Bounce velocity of normal
                Vector2f velocity = Vector2f(pchr->vel.x(), pchr->vel.y());
                velocity.x() -= 2.0f * (nrm.dot(velocity) * nrm.x());
                velocity.y() -= 2.0f * (nrm.dot(velocity) * nrm.y());

                pchr->vel.x() = pchr->vel.x() * bumpdampen + velocity.x()*(1.0f-bumpdampen);
                pchr->vel.y() = pchr->vel.y() * bumpdampen + velocity.y()*(1.0f-bumpdampen);

                //Add additional pressure perpendicular from wall depending on how far inside wall we are
                float displacement = Vector2f(pchr->getSafePosition().x()-tmp_pos.x(), pchr->getSafePosition().y()-tmp_pos.y()).length();
                if(displacement > MAX_DISPLACEMENT_XY) {
                    displacement = MAX_DISPLACEMENT_XY;
                }
                pchr->vel.x() += displacement * bumpdampen * pressure * nrm.x();
                pchr->vel.y() += displacement * bumpdampen * pressure * nrm.y();

                //Apply correction
                tmp_pos.x() += pchr->vel.x();
                tmp_pos.y() += pchr->vel.y();
            }
        }
    }

    pchr->setPosition(tmp_pos);

    // Characters with sticky butts lie on the surface of the mesh
    if(pchr->getProfile()->hasStickyButt() || !pchr->isAlive()) {
        float fkeep = (7.0f + pchr->enviro.zlerp) / 8.0f;
        float fnew  = (1.0f - pchr->enviro.zlerp) / 8.0f;

        if (fnew > 0) {
            pchr->ori.map_twist_facing_x = pchr->ori.map_twist_facing_x * fkeep + g_meshLookupTables.twist_facing_x[pchr->enviro.grid_twist] * fnew;
            pchr->ori.map_twist_facing_y = pchr->ori.map_twist_facing_y * fkeep + g_meshLookupTables.twist_facing_y[pchr->enviro.grid_twist] * fnew;
        }
    }
}

const Vector2f& ObjectPhysics::getDesiredVelocity() const
{
    return _desiredVelocity;
}

} //Physics
} //Ego
