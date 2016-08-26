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
#include "game/Core/GameEngine.hpp"
#include "game/Shop.hpp"
#include "game/CharacterMatrix.h"
#include "game/ObjectAnimation.h"

namespace Ego
{
namespace Physics
{

ObjectPhysics::ObjectPhysics(Object &object) :
    _object(object),
    _platformOffset(0.0f, 0.0f),
    _desiredVelocity(0.0f, 0.0f),
    _traction(1.0f),
    _groundElevation(0.0f),
    _aabb2D()
{
    //ctor
}

void ObjectPhysics::keepItemsWithHolder()
{
    const std::shared_ptr<Object> &holder = _object.getHolder();
    if (holder)
    {
        // Keep in hand weapons with iattached
        if ( chr_matrix_valid(&_object) )
        {
            _object.setPosition(mat_getTranslate(_object.inst.matrix));
        }
        else
        {
            _object.setPosition(holder->getPosition());
        }

        _object.ori.facing_z = holder->ori.facing_z;

        // Copy this stuff ONLY if it's a weapon, not for mounts
        if (holder->getProfile()->transferBlending() && _object.isItem())
        {

            // Items become partially invisible in hands of players
            if (holder->isPlayer() && 255 != holder->inst.alpha)
            {
                _object.setAlpha(SEEINVISIBLE);
            }
            else
            {
                // Only if not naturally transparent
                if (255 == _object.getProfile()->getAlpha())
                {
                    _object.setAlpha(holder->inst.alpha);
                }
                else
                {
                    _object.setAlpha(_object.getProfile()->getAlpha());
                }
            }

            // Do light too
            if (holder->isPlayer() && 255 != holder->inst.light)
            {
                _object.setLight(SEEINVISIBLE);
            }
            else
            {
                // Only if not naturally transparent
                if (255 == _object.getProfile()->getLight())
                {
                    _object.setLight(holder->inst.light);
                }
                else
                {
                    _object.setLight(_object.getProfile()->getLight());
                }
            }
        }
    }
    else
    {
        _object.attachedto = ObjectRef::Invalid;
    }
}

void ObjectPhysics::setDesiredVelocity(const Vector2f &velocity)
{
    _desiredVelocity = velocity;

    //Constrain desired velocity between -1.0f and 1.0f
    if (_desiredVelocity.length() > 1.0f) {
        _desiredVelocity *= (1.0f / _desiredVelocity.length());
    }
}

void ObjectPhysics::updateMovement()
{
    //Can it move?
    if (_object.isAlive() && _object.getAttribute(Ego::Attribute::ACCELERATION) > 0.0f)  {
 
        // Reverse movements for daze
        if (_object.daze_timer > 0) {
            _desiredVelocity.x() = -_desiredVelocity.x();
            _desiredVelocity.y() = -_desiredVelocity.y();
        }

        // Switch x and y for grog
        if (_object.grog_timer > 0) {
            std::swap(_desiredVelocity.x(), _desiredVelocity.y());
        }

        //Update which way we are looking
        updateFacing();
    }
    else {
        //Immobile object
        _desiredVelocity.setZero();
    }

    //Is there any movement going on?
    Vector2f velocitySetpoint;
    if(_desiredVelocity.length_abs() > 0.05f) {
        const float maxSpeed = getMaxSpeed();

        //Scale [-1 , 1] to velocity of the object
        velocitySetpoint = _desiredVelocity * maxSpeed;

        //Limit to max velocity
        if(velocitySetpoint.length() > maxSpeed) {
            velocitySetpoint *= maxSpeed / velocitySetpoint.length();
        }
    }
    else {
        //Try to stand still
        velocitySetpoint.setZero();
    }

    //Determine acceleration/deceleration
    Vector2f acceleration;
    acceleration.x() = (velocitySetpoint.x() - _object.vel.x()) * (4.0f / GameEngine::GAME_TARGET_UPS);
    acceleration.y() = (velocitySetpoint.y() - _object.vel.y()) * (4.0f / GameEngine::GAME_TARGET_UPS);

    //How good grip do we have to add additional momentum?
    acceleration *= _traction;

    //Finally apply acceleration to velocity
    _object.vel.x() += acceleration.x();
    _object.vel.y() += acceleration.y();
}

void ObjectPhysics::updateHillslide()
{
    const uint8_t floorTwist = _currentModule->getMeshPointer()->get_twist(_object.getTile());

    //This makes it hard for characters to jump uphill
    if(_object.vel.z() > 0.0f && floorIsSlippy() && !g_meshLookupTables.twist_flat[floorTwist]) {
        _object.vel.z() *= 0.8f;
    }

    //Only slide if we are touching the floor
    if(isTouchingGround()) {

        //Can the character slide on this floor?
        if (floorIsSlippy() && !_object.getAttachedPlatform())
        {
            //Make characters slide down hills
            if(!g_meshLookupTables.twist_flat[floorTwist]) {
                const float hillslide = Ego::Physics::g_environment.hillslide * (1.0f - getLerpZ()) * (1.0f - _traction);
                _object.vel.x() += g_meshLookupTables.twist_nrm[floorTwist].x() * hillslide;
                _object.vel.y() += g_meshLookupTables.twist_nrm[floorTwist].y() * hillslide;

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

float ObjectPhysics::getLerpZ() const
{
    return Ego::Math::constrain((_object.getPosZ() - _groundElevation) / PLATTOLERANCE, 0.0f, 1.0f);
}

void ObjectPhysics::updateVelocityZ()
{
    //Flying?
    if(_object.isFlying()) {
        float flyLevel = std::max(0.0f, _groundElevation);
        _object.vel.z() += (flyLevel + _object.getAttribute(Ego::Attribute::FLY_TO_HEIGHT) - _object.getPosZ()) * FLYDAMPEN;
        _object.jumpready = false;
        return;
    }

    //Apply gravity
    _object.vel.z() += getLerpZ() * Ego::Physics::g_environment.gravity;

    // Do ground hits
    if(isTouchingGround()) {
        _object.jumpready = true;

        if (_object.vel.z() < -Ego::Physics::STOP_BOUNCING && _object.hitready) {
            SET_BIT(_object.ai.alert, ALERTIF_HITGROUND);
            _object.hitready = false;
        }

        if (0 == _object.jump_timer) {
            // Reset jumping on flat areas of slippiness
            if(!floorIsSlippy() || g_meshLookupTables.twist_flat[_currentModule->getMeshPointer()->get_twist(_object.getTile())]) {
                _object.jumpnumber = _object.getAttribute(Ego::Attribute::NUMBER_OF_JUMPS);                
            }
        }
    }
    else {
        _object.jumpready = false;
    }
}

void ObjectPhysics::updatePhysics()
{
    // Keep inventory items with the carrier
    if(_object.isInsideInventory()) {
        _object.setPosition(_currentModule->getObjectHandler()[_object.inwhich_inventory]->getPosition());
        return;
    }

    // Character's old location
    _object.vel_old          = _object.vel;
    _object.ori_old.facing_z = _object.ori.facing_z;

    //Is this character being held by another character?
    if(_object.isBeingHeld()) {
        keepItemsWithHolder();
        return;
    }

    //Generate velocity from sliding on hills
    updateHillslide();

    //Generate velocity from user input (or AI script)
    updateMovement();

    //Keep us on the platform we are standing on
    updatePlatformPhysics();

    //Generate Z velocity (jumping, gravity, flight, etc.)
    updateVelocityZ();

    //Handle collision with the floor and walls
    updateMeshCollision();

    //Cutoff for low velocities to make them truly stop
    if(_object.vel.length_abs() < 0.05f) {
        _object.vel = Vector3f::zero();
    }

    //Recalculate the altitude of the ground beneath our feet
    //Apperantly this function is quite expensive so cache the result every update
    _groundElevation = recalculateGroundElevation();
}

float ObjectPhysics::recalculateGroundElevation()
{
    //Standing on a platform?
    const std::shared_ptr<Object> &platform = _object.getAttachedPlatform();
    if (platform) {
        return platform->getPosZ() + platform->chr_min_cv._maxs[OCT_Z];
    }

    //Walking on water?
    if(_object.isOnWaterTile() && _object.getAttribute(Ego::Attribute::WALK_ON_WATER) > 0) {
        return _currentModule->getMeshPointer()->getElevation(Vector2f(_object.getPosX(), _object.getPosY()), true);
    }

    //Standing on regular ground
    return _currentModule->getMeshPointer()->getElevation(Vector2f(_object.getPosX(), _object.getPosY()), false);
}

float ObjectPhysics::getMaxSpeed() const
{
    // this is the maximum speed that a character could go under the v2.22 system
    float maxspeed = _object.getAttribute(Ego::Attribute::ACCELERATION) * Ego::Physics::g_environment.airfriction / (1.0f - Ego::Physics::g_environment.airfriction);
    float speedBonus = 1.0f;

    //Sprint perk gives +10% movement speed if above 75% life remaining
    if(_object.hasPerk(Ego::Perks::SPRINT) && _object.getLife() >= _object.getAttribute(Ego::Attribute::MAX_LIFE)*0.75f) {
        speedBonus += 0.1f;

        //Uninjured? (Dash perk can give another 10% extra speed)
        if(_object.hasPerk(Ego::Perks::DASH) && _object.getAttribute(Ego::Attribute::MAX_LIFE)-_object.getLife() < 1.0f) {
            speedBonus += 0.1f;
        }
    }

    //Rally Bonus? (+10%)
    if(_object.hasPerk(Ego::Perks::RALLY) && _gameEngine->getCurrentUpdateFrame() < _object.getRallyDuration()) {
        speedBonus += 0.1f;
    }    

    //Increase movement by 1% per Agility above 10 (below 10 agility reduces movement speed!)
    speedBonus += (_object.getAttribute(Ego::Attribute::AGILITY)-10.0f) * 0.01f;

    //Now apply speed modifiers
    maxspeed *= speedBonus;

    //Are we in water?
    if(_object.isSubmerged() && _currentModule->getWater()._is_water) {
        if(_object.hasPerk(Ego::Perks::ATHLETICS)) {
            maxspeed *= 0.25f; //With athletics perk we can have three-quarters speed
        }
        else {
            maxspeed *= 0.5f; //Half speed in water
        }
    }

    //Check animation frame freeze movement
    if ( chr_instance_t::get_framefx(_object.inst) & MADFX_STOP )
    {
        //Allow 50% movement while using Shield and have the Mobile Defence perk
        if(_object.hasPerk(Ego::Perks::MOBILE_DEFENCE) && ACTION_IS_TYPE(_object.inst.actionState.action_which, P))
        {
            maxspeed *= 0.5f;
        }
        //Allow 50% movement with Mobility perk and attacking with a weapon
        else if(_object.hasPerk(Ego::Perks::MOBILITY) && _object.isAttacking())
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
    else if(!_object.isPlayer())
    {
        maxspeed *= _object.ai.maxSpeed;
    }

    bool sneak_mode_active = _object.isStealthed();

    //Reduce speed while stealthed
    if(_object.isStealthed()) {
        if(_object.hasPerk(Ego::Perks::SHADE)) {
            maxspeed *= 0.75f;  //Shade allows 75% movement speed while stealthed
        }
        else if(_object.hasPerk(Ego::Perks::STALKER)) {
            maxspeed *= 0.50f;  //Stalker allows 50% movement speed while stealthed
        }
        else {
            maxspeed *= 0.33f;  //Can only move at 33% speed while stealthed
        }
    }

    if ( sneak_mode_active )
    {
        // sneak mode
        _object.movement_bits = CHR_MOVEMENT_BITS_SNEAK | CHR_MOVEMENT_BITS_STOP;
    }
    else
    {
        // non-sneak mode
        _object.movement_bits = ( unsigned )( ~CHR_MOVEMENT_BITS_SNEAK );
    }

    return maxspeed;    
}

void ObjectPhysics::updateFacing()
{
    //Figure out how to turn around
    switch ( _object.turnmode )
    {
        // Get direction from ACTUAL change in velocity
        default:
        case TURNMODE_VELOCITY:
            {
                if (_desiredVelocity.length_abs() > TURNSPD)
                {
                    //Every Agility increases turn speed by 2%
                    const float turnSpeed = std::max(2.0f, 8.0f * (1.0f - _object.getAttribute(Ego::Attribute::AGILITY) / 50.0f)); //turn delay is 8.0f -2% per Agility
                    _object.ori.facing_z = Facing(FACING_T(rotate(_object.ori.facing_z, vec_to_facing(_desiredVelocity.x(), _desiredVelocity.y()), turnSpeed)));
                }
            }
            break;

        // Get direction from the DESIRED change in velocity
        case TURNMODE_WATCH:
            {
                if (_desiredVelocity.length_abs() > WATCHMIN )
                {
                    _object.ori.facing_z = Facing(FACING_T(rotate(_object.ori.facing_z, vec_to_facing(_desiredVelocity.x(), _desiredVelocity.y()), 8.0f)));
                }
            }
            break;

        // Face the target
        case TURNMODE_WATCHTARGET:
            {
                //Only proceed if we have a valid AI target that is not ourselves
                std::shared_ptr<Object> aiTarget = _currentModule->getObjectHandler()[_object.ai.getTarget()];
                if (aiTarget != nullptr && aiTarget->getObjRef() != _object.getObjRef())
                {
                    _object.ori.facing_z = Facing(FACING_T(rotate(_object.ori.facing_z, vec_to_facing(aiTarget->getPosX() - _object.getPosX(), aiTarget->getPosY() - _object.getPosY()), 8.0f)));
                }
            }
            break;

        // Otherwise make it spin
        case TURNMODE_SPIN:
            {
                _object.ori.facing_z += Facing(SPINRATE);
            }
            break;
    }
}

void ObjectPhysics::detachFromPlatform()
{
    // adjust the platform weight, if necessary
    if(_object.getAttachedPlatform()) {
        _object.getAttachedPlatform()->holdingweight -= _object.phys.weight;
    }

    // undo the attachment
    _object.onwhichplatform_ref    = ObjectRef::Invalid;
    _object.onwhichplatform_update = 0;
    _object.targetplatform_ref     = ObjectRef::Invalid;
    _object.targetplatform_level   = -1e32;
    _platformOffset = Vector2f::zero();
}

bool ObjectPhysics::attachToPlatform(const std::shared_ptr<Object> &platform)
{
    // check if they can be connected
    if(!_object.canuseplatforms || _object.isFlying() || !platform->platform || platform.get() == &_object) {
        return false;
    }

    // do the attachment
    _object.onwhichplatform_ref    = platform->getObjRef();
    _object.onwhichplatform_update = _gameEngine->getCurrentUpdateFrame();
    _object.targetplatform_ref     = ObjectRef::Invalid;

    _platformOffset.x() = _object.getPosX() - platform->getPosX();
    _platformOffset.y() = _object.getPosY() - platform->getPosY();

    //Make sure the object is now on top of the platform
    const float platformElevation = platform->getPosZ() + platform->chr_min_cv._maxs[OCT_Z];
    if(_object.getPosZ() < platformElevation) {
        _object.setPosition(_object.getPosX(), _object.getPosY(), platformElevation);
    }

    // add the weight to the platform
    platform->holdingweight += _object.phys.weight;

    // update the character jumping
    _object.jumpready = true;
    _object.jumpnumber = _object.getAttribute(Ego::Attribute::NUMBER_OF_JUMPS);

    // tell the platform that we bumped into it
    // this is necessary for key buttons to work properly, for instance
    ai_state_t::set_bumplast(platform->ai, _object.getObjRef());

    return true;
}

void ObjectPhysics::updatePlatformPhysics()
{
    const std::shared_ptr<Object> &platform = _object.getAttachedPlatform();
    if(!platform) {
        return;
    }

    // grab the pre-computed zlerp value, and map it to our needs
    float lerp_z = 1.0f - getLerpZ();

    // if your velocity is going up much faster than the
    // platform, there is no need to suck you to the level of the platform
    // this was one of the things preventing you from jumping from platforms
    // properly
    if(std::abs(_object.vel.z() - platform->vel.z()) / 5.0f <= PLATFORM_STICKINESS) {
        _object.vel.z() += (platform->vel.z()  - _object.vel.z()) * lerp_z;
    }

    // determine the rotation rates
    int16_t rot_b = FACING_T(_object.ori.facing_z) - FACING_T(_object.ori_old.facing_z);
    int16_t rot_a = FACING_T(platform->ori.facing_z) - FACING_T(platform->ori_old.facing_z);
    _object.ori.facing_z += Facing(int16_t((rot_a - rot_b) * PLATFORM_STICKINESS));    

    //Allows movement on the platform
    _platformOffset.x() += _object.vel.x();
    _platformOffset.y() += _object.vel.y();

    //Inherit position of platform with given offsets
    float zCorrection = (_groundElevation - _object.getPosZ()) * 0.125f * lerp_z;
    _object.setPosition(platform->getPosX() + _platformOffset.x(), platform->getPosY() + _platformOffset.y(), _object.getPosZ() + zCorrection);
}

void ObjectPhysics::updateMeshCollision()
{
    Vector3f tmp_pos = _object.getPosition();;

    // interaction with the mesh
    //if ( std::abs( _object.vel.z() ) > 0.0f )
    {
        //Make everything "float" a little above the mesh
        const float floorElevation = _groundElevation + 10.0f;

        tmp_pos.z() += _object.vel.z();

        //Hit the floor?
        if (tmp_pos.z() <= floorElevation)
        {
            tmp_pos.z() = floorElevation;

            //Stop bouncing when below threshold
            if (std::abs(_object.vel.z()) < Ego::Physics::STOP_BOUNCING)
            {
                _object.vel.z() = 0.0f;
            }

            //Make it bounce!
            else if (_object.vel.z() < 0.0f)
            {                
                _object.vel.z() = -_object.vel.z() * _object.getProfile()->getBounciness();
            }
        }
    }

    // fixes to the z-position
    if (_object.isFlying())
    {
        // Don't fall in pits...
        if (tmp_pos.z() < 0.0f) {
            tmp_pos.z() = 0.0f;
        }
    }


    //if (std::abs(_object.vel.x()) + std::abs(_object.vel.y()) > 0.0f)
    {
        float old_x = tmp_pos.x();
        float old_y = tmp_pos.y();

        float new_x = old_x + _object.vel.x();
        float new_y = old_y + _object.vel.y();

        tmp_pos.x() = new_x;
        tmp_pos.y() = new_y;

        //Wall collision?
        if ( EMPTY_BIT_FIELD != _object.test_wall( tmp_pos ) )
        {            
            Vector2f nrm;
            float   pressure;

            _object.hit_wall(tmp_pos, nrm, &pressure );

            // how is the character hitting the wall?
            if (pressure > 0.0f)
            {
                tmp_pos.x() -= _object.vel.x();
                tmp_pos.y() -= _object.vel.y();

                const float bumpdampen = std::max(0.1f, 1.0f-_object.phys.bumpdampen);

                //Bounce velocity of normal
                Vector2f velocity = Vector2f(_object.vel.x(), _object.vel.y());
                velocity.x() -= 2.0f * (nrm.dot(velocity) * nrm.x());
                velocity.y() -= 2.0f * (nrm.dot(velocity) * nrm.y());

                _object.vel.x() = _object.vel.x() * bumpdampen + velocity.x()*(1.0f-bumpdampen);
                _object.vel.y() = _object.vel.y() * bumpdampen + velocity.y()*(1.0f-bumpdampen);

                //Add additional pressure perpendicular from wall depending on how far inside wall we are
                float displacement = Vector2f(_object.getSafePosition().x()-tmp_pos.x(), _object.getSafePosition().y()-tmp_pos.y()).length();
                if(displacement > MAX_DISPLACEMENT_XY) {
                    displacement = MAX_DISPLACEMENT_XY;
                }
                _object.vel.x() += displacement * bumpdampen * pressure * nrm.x();
                _object.vel.y() += displacement * bumpdampen * pressure * nrm.y();

                //Apply correction
                tmp_pos.x() += _object.vel.x();
                tmp_pos.y() += _object.vel.y();
            }
        }
    }

    _object.setPosition(tmp_pos);

    // Characters with sticky butts lie on the surface of the mesh
    if(_object.getProfile()->hasStickyButt() || !_object.isAlive()) {
        float fkeep = (7.0f + getLerpZ()) / 8.0f;
        float fnew  = (1.0f - getLerpZ()) / 8.0f;

        if (fnew > 0) {
            const uint8_t floorTwist = _currentModule->getMeshPointer()->get_twist(_object.getTile());
            _object.ori.map_twist_facing_x = Facing(FACING_T(_object.ori.map_twist_facing_x * fkeep + g_meshLookupTables.twist_facing_x[floorTwist] * fnew));
            _object.ori.map_twist_facing_y = Facing(FACING_T(_object.ori.map_twist_facing_y * fkeep + g_meshLookupTables.twist_facing_y[floorTwist] * fnew));
        }
    }
}

const Vector2f& ObjectPhysics::getDesiredVelocity() const
{
    return _desiredVelocity;
}

float ObjectPhysics::getMass() const
{
    if ( Ego::Physics::CHR_INFINITE_WEIGHT == _object.phys.weight )
    {
        return -static_cast<float>(Ego::Physics::CHR_INFINITE_WEIGHT);
    }
    else if ( 0.0f == _object.phys.bumpdampen )
    {
        return -static_cast<float>(Ego::Physics::CHR_INFINITE_WEIGHT);
    }
    else
    {
        return _object.phys.weight / _object.phys.bumpdampen;
    }    
}

bool ObjectPhysics::grabStuff(grip_offset_t grip_off, bool grab_people)
{
    //Max search distance in quad tree relative to object position
    static constexpr float MAX_SEARCH_DIST = 3.0f * Info<float>::Grid::Size();

    //Max grab distance is 2/3rds of a tile
    static constexpr float MAX_DIST_GRAB = Info<float>::Grid::Size() * 0.66f;

    // find the slot from the grip
    slot_t slot = grip_offset_to_slot( grip_off );
    if ( slot >= SLOT_COUNT ) return false;

    // Make sure the character doesn't have something already, and that it has hands
    if (_currentModule->getObjectHandler().exists( _object.holdingwhich[slot] ) || !_object.getProfile()->isSlotValid(slot)) {        
        return false;
    }

    //Determine the position of the grip
    oct_vec_v2_t mids = _object.slot_cv[slot].getMid();

    Vector3f   slot_pos = Vector3f(mids[OCT_X], mids[OCT_Y], mids[OCT_Z]) + _object.getPosition();

    //The object that we grab
    std::shared_ptr<Object> bestMatch = nullptr;
    float bestMatchDistance = std::numeric_limits<float>::max();

    // Go through all nearby objects to find the best match
    std::vector<std::shared_ptr<Object>> nearbyObjects = _currentModule->getObjectHandler().findObjects(slot_pos.x(), slot_pos.y(), MAX_SEARCH_DIST, false);
    for(const std::shared_ptr<Object> &pchr_c : nearbyObjects)
    {
        //Skip invalid objects
        if(pchr_c->isTerminated()) {
            continue;
        }

        // do nothing to yourself
        if (_object.getObjRef() == pchr_c->getObjRef()) continue;

        // Dont do hidden objects
        if (pchr_c->isHidden()) continue;

        // disarm and pickpocket not allowed yet
        if (pchr_c->isBeingHeld()) continue;

        // do not pick up your mount
        if ( pchr_c->holdingwhich[SLOT_LEFT] == _object.getObjRef() ||
             pchr_c->holdingwhich[SLOT_RIGHT] == _object.getObjRef() ) continue;

        // do not notice completely broken items?
        if (pchr_c->isItem() && !pchr_c->isAlive()) continue;

        // reasonable carrying capacity
        if (pchr_c->phys.weight > _object.phys.weight + FLOAT_TO_FP8(_object.getAttribute(Ego::Attribute::MIGHT)) * INV_FF<float>()) {
            continue;
        }

        // grab_people == true allows you to pick up living non-items
        // grab_people == false allows you to pick up living (functioning) items
        if (!grab_people && !pchr_c->isItem()) {
            continue;
        }

        // calculate the distance
        const float horizontalDistance = (pchr_c->getPosition() - slot_pos).length();
        const float verticalDistance = std::sqrt(Ego::Math::sq(_object.getPosZ() - pchr_c->getPosZ()));
 
        //Figure out if the character is looking towards the object
        const bool isFacingObject = _object.isFacingLocation(pchr_c->getPosX(), pchr_c->getPosY());

        // Is it too far away to interact with?
        if (horizontalDistance > MAX_SEARCH_DIST || verticalDistance > MAX_SEARCH_DIST) {
            continue;
        }

        // visibility affects the max grab distance.
        // if it is not visible then we have to be touching it.
        float maxHorizontalGrabDistance = MAX_DIST_GRAB;

        //Halve grab distance for items behind us
        if(!isFacingObject && !grab_people) {
            maxHorizontalGrabDistance *= 0.5f;
        }

        //Bigger characters have bigger grab size
        maxHorizontalGrabDistance += _object.bump.size / 4.0f;

        //Double grab distance for monsters that are trying to grapple
        if(grab_people) {
            maxHorizontalGrabDistance *= 2.0f;
        }

        // is it too far away to grab?
        if (horizontalDistance > maxHorizontalGrabDistance + _object.bump.size / 4.0f && horizontalDistance > _object.bump.size) {
            continue;
        }

        //Check vertical distance as well
        else
        {
            float maxVerticalGrabDistance = _object.bump.height / 2.0f;

            if(grab_people) {
                //This allows very flat creatures like the Carpet Mimics grab people
                maxVerticalGrabDistance = std::max(maxVerticalGrabDistance, MAX_DIST_GRAB);
            }

            if (verticalDistance > maxVerticalGrabDistance) {
                continue;
            }
        }

        //Is this one better to grab than any previous matches?
        if(horizontalDistance < bestMatchDistance) {
            bestMatchDistance = horizontalDistance;
            bestMatch = pchr_c;

            //Prioritize items in front of us over those behind us
            if(!isFacingObject) {
                bestMatchDistance *= 2.0f;
            }
        }
    }

    if(bestMatch != nullptr) {
        const std::shared_ptr<Object> &grabber = _currentModule->getObjectHandler()[_object.getObjRef()];
        if (Shop::canGrabItem(grabber, bestMatch))
        {
            // Stick 'em together and quit
            if(bestMatch->getObjectPhysics().attachToObject(grabber, grip_off))
            {
                if (grab_people)
                {
                    // Start the slam animation...  ( Be sure to drop!!! )
                    chr_play_action(&_object, ACTION_MC + slot, false);
                }
            }
            return true;
        }
        else
        {
            // Lift the item a little and quit...
            bestMatch->vel.z() = Object::DROPZVEL;
            bestMatch->hitready = true;
            SET_BIT(bestMatch->ai.alert, ALERTIF_DROPPED);
        }
    }

    return false;
}

bool ObjectPhysics::attachToObject(const std::shared_ptr<Object> &holder, grip_offset_t grip_off)
{
    /// @author ZZ
    /// @details This function attaches one object (rider) to another object (mount)
    ///          at a certain vertex offset ( grip_off )
    ///   - This function is called as a part of spawning a module, so the rider or the mount may not
    ///     be fully instantiated
    ///   - This function should do very little testing to see if attachment is allowed.
    ///     Most of that checking should be done by the calling function

    //Don't attach a character to itself!
    if (_object.getObjRef() == holder->getObjRef()) {
        return false;
    }

    // do not deal with packed items at this time
    // this would have to be changed to allow for pickpocketing
    if (_object.isBeingHeld()) {
       return false; 
    } 

    // This is a small fix that allows special grabbable mounts not to be mountable while
    // held by another character (such as the magic carpet for example)
    if(holder->isBeingHeld()) {
        return false;
    }

    // make a reasonable time for the character to remount something
    // for characters jumping out of pots, etc
    if (holder->getObjRef() == _object.dismount_object && _object.dismount_timer > 0) {
        return false;
    }

    // Figure out which slot this grip_off relates to
    slot_t slot = grip_offset_to_slot(grip_off);

    // Make sure the the slot is valid
    if (!holder->getProfile()->isSlotValid(slot)) {
        return false;
    }

    //Make sure it is no longer attach to a platform
    detachFromPlatform();

    // Put 'em together
    _object.inwhich_slot       = slot;
    _object.attachedto         = holder->getObjRef();
    holder->holdingwhich[slot] = _object.getObjRef();

    // set the grip vertices for the irider
    set_weapongrip(_object.getObjRef(), holder->getObjRef(), grip_off);

    chr_update_matrix(&_object, true);

    _object.setPosition(mat_getTranslate(_object.inst.matrix));

    _object.inwater  = false;
    _object.jump_timer = Object::JUMPDELAY * 4;

    // Run the held animation
    if (holder->isMount() && (GRIP_ONLY == grip_off))
    {
        // Riding imount
        if (_object.getLeftHandItem() || _object.getRightHandItem())
        {
            // if the character is holding anything, make the animation
            // ACTION_MH == "sitting" so that it does not look so silly
            chr_play_action(&_object, ACTION_MH, true);
        }
        else
        {
            // if it is not holding anything, go for the riding animation
            chr_play_action(&_object, ACTION_MI, true);
        }

        // set tehis action to loop
        chr_instance_t::set_action_loop(_object.inst, true);
    }
    else if (_object.isAlive())
    {
        /// @note ZF@> hmm, here is the torch holding bug. Removing
        /// the interpolation seems to fix it...
        chr_play_action(&_object, ACTION_MM + slot, false );
        chr_instance_t::remove_interpolation(_object.inst);

        // set the action to keep for items
        if (_object.isItem()) {
            chr_instance_t::set_action_keep(_object.inst, true);
        }
    }

    // Set the team
    if (_object.isItem())
    {
        _object.team = holder->team;

        // Set the alert
        if (_object.isAlive()) {
            SET_BIT(_object.ai.alert, ALERTIF_GRABBED);
        }

        // Lore Master perk identifies everything
        if (holder->hasPerk(Ego::Perks::LORE_MASTER)) {
            _object.getProfile()->makeUsageKnown();
            _object.nameknown = true;
            _object.ammoknown = true;
        }
    }

    if (holder->isMount())
    {
        holder->team = _object.team;

        // Set the alert
        if (!holder->isItem() && holder->isAlive())
        {
            SET_BIT(holder->ai.alert, ALERTIF_GRABBED);
        }
    }

    // It's not gonna hit the floor
    _object.hitready = false;

    return rv_success;
}

void ObjectPhysics::updateCollisionSize(bool update_matrix)
{
    // re-initialize the collision volumes
    _object.chr_min_cv = oct_bb_t();
    _object.chr_max_cv = oct_bb_t();
    for (size_t cnt = 0; cnt < SLOT_COUNT; cnt++) {
        _object.slot_cv[cnt] = oct_bb_t();
    }

    // make sure the matrix is updated properly
    if (update_matrix) {
        // call chr_update_matrix() but pass in a false value to prevent a recursive call
        if (rv_error == chr_update_matrix(&_object, false)) {
            return;
        }
    }

    // make sure the bounding box is calculated properly
    if (gfx_error == chr_instance_t::update_bbox(_object.inst)) {
        return;
    }

    // convert the point cloud in the GLvertex array (_object.inst.vrt_lst) to
    // a level 1 bounding box. Subtract off the position of the character
    oct_bb_t bsrc = _object.inst.bbox;

    Vector4f  src[16];  // for the upper and lower octagon points
    Vector4f  dst[16];  // for the upper and lower octagon points

    // convert the corners of the level 1 bounding box to a point cloud
    // keep track of the actual number of vertices, in case the object is square
    int vcount = oct_bb_t::to_points(bsrc, src, 16);

   // transform the new point cloud
    Utilities::transform(_object.inst.matrix, src, dst, vcount);

    // convert the new point cloud into a level 1 bounding box
    oct_bb_t bdst;
    oct_bb_t::points_to_oct_bb(bdst, dst, vcount);

    //---- set the bounding boxes
    _object.chr_min_cv = bdst;
    _object.chr_max_cv = bdst;

    oct_bb_t bmin;
    bmin.assign(_object.bump);

    // only use _object.bump.size if it was overridden in data.txt through the [MODL] expansion
    if ( _object.getProfile()->getBumpOverrideSize() )
    {
        _object.chr_min_cv.cut(bmin, OCT_X);
        _object.chr_min_cv.cut(bmin, OCT_Y);

        _object.chr_max_cv.join(bmin, OCT_X);
        _object.chr_max_cv.join(bmin, OCT_Y);
    }

    // only use _object.bump.size_big if it was overridden in data.txt through the [MODL] expansion
    if (_object.getProfile()->getBumpOverrideSizeBig()) {
        _object.chr_min_cv.cut(bmin, OCT_XY);
        _object.chr_min_cv.cut(bmin, OCT_YX);

        _object.chr_max_cv.join(bmin, OCT_XY);
        _object.chr_max_cv.join(bmin, OCT_YX);
    }

    // only use _object.bump.height if it was overridden in data.txt through the [MODL] expansion
    if (_object.getProfile()->getBumpOverrideHeight()) {
        _object.chr_min_cv.cut(bmin, OCT_Z);
        _object.chr_max_cv.join(bmin, OCT_Z);
    }

    //// raise the upper bound for platforms
    //if ( _object.platform )
    //{
    //    _object.chr_max_cv.maxs[OCT_Z] += PLATTOLERANCE;
    //}

    //This makes it easier to jump on top of mounts
    if(_object.isMount()) {
       _object.chr_max_cv._maxs[OCT_Z] = Ego::Math::constrain<float>(_object.chr_max_cv._maxs[OCT_Z]-MOUNTTOLERANCE, MOUNTTOLERANCE, MOUNTTOLERANCE*3.0f);
       _object.chr_min_cv._maxs[OCT_Z] = Ego::Math::constrain<float>(_object.chr_min_cv._maxs[OCT_Z]-MOUNTTOLERANCE, MOUNTTOLERANCE, MOUNTTOLERANCE*3.0f);
    }

    // calculate collision volumes for various slots
    for (size_t cnt = 0; cnt < SLOT_COUNT; cnt++)
    {
        if (!_object.getProfile()->isSlotValid(static_cast<slot_t>(cnt))) continue;

        chr_calc_grip_cv(&_object, GRIP_LEFT, &_object.slot_cv[cnt], false);

        _object.chr_max_cv.join(_object.slot_cv[cnt]);
    }

    // convert the level 1 bounding box to a level 0 bounding box
    oct_bb_t::downgrade(bdst, _object.bump_stt, _object.bump, _object.bump_1);

    //Recalculate the fast 2D collision box
    _aabb2D = AxisAlignedBox2f(Point2f(_object.getPosX() + _object.chr_min_cv.getMin()[OCT_X],
                               _object.getPosY() + _object.chr_min_cv.getMin()[OCT_Y]),
                               Point2f(_object.getPosX() + _object.chr_min_cv.getMax()[OCT_X],
                               _object.getPosY() + _object.chr_min_cv.getMax()[OCT_Y]));
}

bool ObjectPhysics::floorIsSlippy() const
{
    //Water tiles are never slippy
    if(_object.inwater && _currentModule->getWater()._is_water) return false;

    //Check tile slippy bit
    return 0 != _currentModule->getMeshPointer()->test_fx(_object.getTile(), MAPFX_SLIPPY);
}

bool ObjectPhysics::isTouchingGround() const
{
    //Never touching the ground while levitating
    if(_object.isFlying()) {
        return false;
    }

    return std::abs(_object.getPosZ() - _groundElevation) <= FLOOR_TOLERANCE;
}

float ObjectPhysics::getGroundElevation() const
{
    return _groundElevation;
}

const AxisAlignedBox2f& ObjectPhysics::getAxisAlignedBox2D() const
{
    return _aabb2D;
}

} //Physics
} //Ego
