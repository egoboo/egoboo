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

#include "game/game.h" //TODO: Remove (only for LATCHBUTTON_JUMP)
#include "egolib/Graphics/ModelDescriptor.hpp" //TODO: Remove (only for latches)
#include "game/renderer_2d.h" //TODO: Remove (only for latches)

namespace Ego
{
namespace Physics
{

static constexpr float MAX_DISPLACEMENT_XY = 20.0f;     //< Max velocity correction due to being inside a wall

//ZF> TODO: These have nothing to do with Physics, move elsewhere
static bool chr_do_latch_button( Object * pchr );
static bool chr_do_latch_attack( Object * pchr, slot_t which_slot );

static bool chr_do_latch_button( Object * pchr )
{
    /// @author BB
    /// @details Character latches for generalized buttons

    if (!pchr || pchr->isTerminated()) return false;
    auto ichr = pchr->getObjRef();

    if ( !pchr->isAlive() || pchr->latch.b.none() ) return true;

    const std::shared_ptr<ObjectProfile> &profile = pchr->getProfile();

    if ( pchr->latch.b[LATCHBUTTON_JUMP] && 0 == pchr->jump_timer )
    {

        //Jump from our mount
        if (pchr->isBeingHeld())
        {
            pchr->detatchFromHolder(true, true);
            pchr->getObjectPhysics().detachFromPlatform(pchr);

            pchr->jump_timer = JUMPDELAY;
            if ( pchr->isFlying() )
            {
                pchr->vel[kZ] += DISMOUNTZVELFLY;
            }
            else
            {
                pchr->vel[kZ] += DISMOUNTZVEL;
            }

            pchr->setPosition(pchr->getPosX(), pchr->getPosY(), pchr->getPosZ() + pchr->vel[kZ]);

            if ( pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) != JUMPINFINITE && 0 != pchr->jumpnumber ) {
                pchr->jumpnumber--;
            }

            // Play the jump sound
            AudioSystem::get().playSound(pchr->getPosition(), profile->getJumpSound());
        }

        //Normal jump
        else if ( 0 != pchr->jumpnumber && !pchr->isFlying() )
        {
            if (1 != pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) || pchr->jumpready)
            {
                //Exit stealth unless character has Stalker Perk
                if(!pchr->hasPerk(Ego::Perks::STALKER)) {
                    pchr->deactivateStealth();
                }

                // Make the character jump
                float jumpPower = 0.0f;
                pchr->hitready = true;
                if (pchr->enviro.inwater || pchr->enviro.is_slippy)
                {
                    pchr->jump_timer = JUMPDELAY * 4;         //To prevent 'bunny jumping' in water
                    jumpPower = WATERJUMP;
                }
                else
                {
                    pchr->jump_timer = JUMPDELAY;
                    jumpPower = pchr->getAttribute(Ego::Attribute::JUMP_POWER) * 1.5f;
                }

                pchr->vel[kZ] += jumpPower;
                pchr->jumpready = false;

                if (pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) != JUMPINFINITE) { 
                    pchr->jumpnumber--;
                }

                // Set to jump animation if not doing anything better
                if ( pchr->inst.action_ready )
                {
                    chr_play_action( pchr, ACTION_JA, true );
                }

                // Play the jump sound (Boing!)
                AudioSystem::get().playSound(pchr->getPosition(), profile->getJumpSound());
            }
        }

    }
    if ( pchr->latch.b[LATCHBUTTON_PACKLEFT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = PACKDELAY;
        Inventory::swap_item( ichr, pchr->getInventory().getFirstFreeSlotNumber(), SLOT_LEFT, false );
    }
    if ( pchr->latch.b[LATCHBUTTON_PACKRIGHT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = PACKDELAY;
        Inventory::swap_item( ichr, pchr->getInventory().getFirstFreeSlotNumber(), SLOT_RIGHT, false );
    }

    if ( pchr->latch.b[LATCHBUTTON_ALTLEFT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = GRABDELAY;
        if ( !pchr->getLeftHandItem() )
        {
            // Grab left
            if(!pchr->getProfile()->getModel()->isActionValid(ACTION_ME)) {
                //No grab animation valid
                character_grab_stuff( ichr, GRIP_LEFT, false );
            }
            else {
                chr_play_action( pchr, ACTION_ME, false );
            }
        }
        else
        {
            // Drop left
            chr_play_action( pchr, ACTION_MA, false );
        }
    }
    if ( pchr->latch.b[LATCHBUTTON_ALTRIGHT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_ALTRIGHT;

        pchr->reload_timer = GRABDELAY;
        if ( !pchr->getRightHandItem() )
        {
            // Grab right
            if(!pchr->getProfile()->getModel()->isActionValid(ACTION_MF)) {
                //No grab animation valid
                character_grab_stuff( ichr, GRIP_RIGHT, false );
            }
            else {
                chr_play_action( pchr, ACTION_MF, false );
            }
        }
        else
        {
            // Drop right
            chr_play_action( pchr, ACTION_MB, false );
        }
    }

    // LATCHBUTTON_LEFT and LATCHBUTTON_RIGHT are mutually exclusive
    bool attack_handled = false;
    if ( !attack_handled && pchr->latch.b[LATCHBUTTON_LEFT] && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_LEFT;
        attack_handled = chr_do_latch_attack( pchr, SLOT_LEFT );
    }
    if ( !attack_handled && pchr->latch.b[LATCHBUTTON_RIGHT] && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_RIGHT;

        attack_handled = chr_do_latch_attack( pchr, SLOT_RIGHT );
    }

    return true;
}

static bool chr_do_latch_attack( Object * pchr, slot_t which_slot )
{
    int base_action, hand_action, action;
    bool action_valid, allowedtoattack;

    bool retval = false;

    if (!pchr || pchr->isTerminated()) return false;
    auto iobj = GET_INDEX_PCHR( pchr );


    if (which_slot >= SLOT_COUNT) return false;

    // Which iweapon?
    auto iweapon = pchr->holdingwhich[which_slot];
    if ( !_currentModule->getObjectHandler().exists( iweapon ) )
    {
        // Unarmed means object itself is the weapon
        iweapon = iobj;
    }
    Object *pweapon = _currentModule->getObjectHandler().get(iweapon);
    const std::shared_ptr<ObjectProfile> &weaponProfile = pweapon->getProfile();

    // No need to continue if we have an attack cooldown
    if ( 0 != pweapon->reload_timer ) return false;

    // grab the iweapon's action
    base_action = weaponProfile->getWeaponAction();
    hand_action = pchr->getProfile()->getModel()->randomizeAction( base_action, which_slot );

    // see if the character can play this action
    action       = pchr->getProfile()->getModel()->getAction(hand_action);
    action_valid = ACTION_COUNT != action;

    // Can it do it?
    allowedtoattack = true;

    // First check if reload time and action is okay
    if ( !action_valid )
    {
        allowedtoattack = false;
    }
    else
    {
        // Then check if a skill is needed
        if ( weaponProfile->requiresSkillIDToUse() )
        {
            if (!pchr->hasSkillIDSZ(pweapon->getProfile()->getIDSZ(IDSZ_SKILL)))
            {
                allowedtoattack = false;
            }
        }
    }

    // Don't allow users with kursed weapon in the other hand to use longbows
    if ( allowedtoattack && ACTION_IS_TYPE( action, L ) )
    {
        const std::shared_ptr<Object> &offhandItem = which_slot == SLOT_LEFT ? pchr->getLeftHandItem() : pchr->getRightHandItem();
        if(offhandItem && offhandItem->iskursed) allowedtoattack = false;
    }

    if ( !allowedtoattack )
    {
        // This character can't use this iweapon
        pweapon->reload_timer = ONESECOND;
        if (pchr->getShowStatus() || egoboo_config_t::get().debug_developerMode_enable.getValue())
        {
            // Tell the player that they can't use this iweapon
            DisplayMsg_printf( "%s can't use this item...", pchr->getName(false, true, true).c_str());
        }
        return false;
    }

    if ( ACTION_DA == action )
    {
        allowedtoattack = false;
        if ( 0 == pweapon->reload_timer )
        {
            SET_BIT( pweapon->ai.alert, ALERTIF_USED );
        }
    }

    // deal with your mount (which could steal your attack)
    if ( allowedtoattack )
    {
        // Rearing mount
        const std::shared_ptr<Object> &pmount = _currentModule->getObjectHandler()[pchr->attachedto];

        if (pmount)
        {
            const std::shared_ptr<ObjectProfile> &mountProfile = pmount->getProfile();

            // let the mount steal the rider's attack
            if (!mountProfile->riderCanAttack()) allowedtoattack = false;

            // can the mount do anything?
            if ( pmount->isMount() && pmount->isAlive() )
            {
                // can the mount be told what to do?
                if ( !pmount->isPlayer() && pmount->inst.action_ready )
                {
                    if ( !ACTION_IS_TYPE( action, P ) || !mountProfile->riderCanAttack() )
                    {
                        chr_play_action( pmount.get(), Random::next((int)ACTION_UA, ACTION_UA + 1), false );
                        SET_BIT( pmount->ai.alert, ALERTIF_USED );
                        pchr->ai.lastitemused = pmount->getObjRef();

                        retval = true;
                    }
                }
            }
        }
    }

    // Attack button
    if ( allowedtoattack )
    {
        //Attacking or using an item disables stealth
        pchr->deactivateStealth();

        if ( pchr->inst.action_ready && action_valid )
        {
            //Check if we are attacking unarmed and cost mana to do so
            if(iweapon == pchr->getObjRef())
            {
                if(pchr->getProfile()->getUseManaCost() <= pchr->getMana())
                {
                    pchr->costMana(pchr->getProfile()->getUseManaCost(), pchr->getObjRef());
                }
                else
                {
                    allowedtoattack = false;
                }
            }

            if(allowedtoattack)
            {
                // randomize the action
                action = pchr->getProfile()->getModel()->randomizeAction( action, which_slot );

                // make sure it is valid
                action = pchr->getProfile()->getModel()->getAction(action);

                if ( ACTION_IS_TYPE( action, P ) )
                {
                    // we must set parry actions to be interrupted by anything
                    chr_play_action( pchr, action, true );
                }
                else
                {
                    float agility = pchr->getAttribute(Ego::Attribute::AGILITY);

                    chr_play_action( pchr, action, false );

                    // Make the weapon animate the attack as well as the character holding it
                    if (iweapon != iobj)
                    {
                        chr_play_action(pweapon, ACTION_MJ, false);
                    }

                    //Crossbow Mastery increases XBow attack speed by 30%
                    if(pchr->hasPerk(Ego::Perks::CROSSBOW_MASTERY) && 
                       pweapon->getProfile()->getIDSZ(IDSZ_PARENT) == MAKE_IDSZ('X','B','O','W')) {
                        agility *= 1.30f;
                    }

                    //Determine the attack speed (how fast we play the animation)
                    pchr->inst.rate  = 0.80f;                                 //base attack speed
                    pchr->inst.rate += std::min(3.00f, agility * 0.02f);      //every Agility increases base attack speed by 2%

                    //If Quick Strike perk triggers then we have fastest possible attack (10% chance)
                    if(pchr->hasPerk(Ego::Perks::QUICK_STRIKE) && pweapon->getProfile()->isMeleeWeapon() && Random::getPercent() <= 10) {
                        pchr->inst.rate = 3.00f;
                        chr_make_text_billboard(pchr->getObjRef(), "Quick Strike!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::blue(), 3, Billboard::Flags::All);
                    }

                    //Add some reload time as a true limit to attacks per second
                    //Dexterity decreases the reload time for all weapons. We could allow other stats like intelligence
                    //reduce reload time for spells or gonnes here.
                    else if ( !weaponProfile->hasFastAttack() )
                    {
                        int base_reload_time = -agility;
                        if ( ACTION_IS_TYPE( action, U ) )      base_reload_time += 50;     //Unarmed  (Fists)
                        else if ( ACTION_IS_TYPE( action, T ) ) base_reload_time += 55;     //Thrust   (Spear)
                        else if ( ACTION_IS_TYPE( action, C ) ) base_reload_time += 85;     //Chop     (Axe)
                        else if ( ACTION_IS_TYPE( action, S ) ) base_reload_time += 65;     //Slice    (Sword)
                        else if ( ACTION_IS_TYPE( action, B ) ) base_reload_time += 70;     //Bash     (Mace)
                        else if ( ACTION_IS_TYPE( action, L ) ) base_reload_time += 60;     //Longbow  (Longbow)
                        else if ( ACTION_IS_TYPE( action, X ) ) base_reload_time += 130;    //Xbow     (Crossbow)
                        else if ( ACTION_IS_TYPE( action, F ) ) base_reload_time += 60;     //Flinged  (Unused)

                        //it is possible to have so high dex to eliminate all reload time
                        if ( base_reload_time > 0 ) pweapon->reload_timer += base_reload_time;
                    }
                }

                // let everyone know what we did
                pchr->ai.lastitemused = iweapon;

                /// @note ZF@> why should there any reason the weapon should NOT be alerted when it is used?
                // grab the MADFX_* flags for this action
//                BIT_FIELD action_madfx = getProfile()->getModel()->getActionFX(action);
//                if ( iweapon == ichr || HAS_NO_BITS( action, MADFX_ACTLEFT | MADFX_ACTRIGHT ) )
                {
                    SET_BIT( pweapon->ai.alert, ALERTIF_USED );
                }

                retval = true;
            }
        }
    }

    //Reset boredom timer if the attack succeeded
    if ( retval )
    {
        pchr->bore_timer = BORETIME;
    }

    return retval;
}

void ObjectPhysics::keepItemsWithHolder(const std::shared_ptr<Object> &pchr)
{
    /// @author ZZ
    /// @details This function keeps weapons near their holders

    //Ignore invalid objects
    if(!pchr || pchr->isTerminated()) {
        return;
    }

   const std::shared_ptr<Object> &holder = pchr->getHolder();
    if (holder)
    {
        // Keep in hand weapons with iattached
        if ( chr_matrix_valid( pchr.get() ) )
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

ObjectPhysics::ObjectPhysics() :
    _platformOffset(0.0f, 0.0f)
{
    //ctor
}

void ObjectPhysics::updateMovement(const std::shared_ptr<Object> &object)
{
    //Mounted?
    if (object->isBeingHeld()) {
        return;
    }

    //Desired velocity in scaled space [-1 , 1]
    Vector2f targetVelocity = Vector2f(0.0f, 0.0f);

    //Can it move?
    if (object->isAlive() && object->getAttribute(Ego::Attribute::ACCELERATION) > 0.0f)  {
        targetVelocity[kX] = object->latch.x;
        targetVelocity[kY] = object->latch.y;

        // Reverse movements for daze
        if (object->daze_timer > 0) {
            targetVelocity[kX] = -targetVelocity[kX];
            targetVelocity[kY] = -targetVelocity[kY];
        }

        // Switch x and y for grog
        if (object->grog_timer > 0) {
            std::swap(targetVelocity[kX], targetVelocity[kY]);
        }

        //Update which way we are looking
        updateFacing(object, targetVelocity);
    }

    //Is there any movement going on?
    if(targetVelocity.length_abs() > 0.05f) {
        const float maxSpeed = getMaxSpeed(object.get());

        //Scale [-1 , 1] to velocity of the object
        targetVelocity *= maxSpeed;

        //Limit to max velocity
        if(targetVelocity.length() > maxSpeed) {
            targetVelocity *= maxSpeed / targetVelocity.length();
        }
    }
    else {
        if (!object->getAttachedPlatform())
        {
            //Try to stand still
            targetVelocity.setZero();
        }
    }

    //Determine acceleration/deceleration
    Vector2f acceleration;
    acceleration[kX] = (targetVelocity[kX] - object->vel[kX]) * (4.0f / GameEngine::GAME_TARGET_UPS);
    acceleration[kY] = (targetVelocity[kY] - object->vel[kY]) * (4.0f / GameEngine::GAME_TARGET_UPS);

    //How good grip do we have to add additional momentum?
    if (object->enviro.grounded)
    {
        acceleration *= object->enviro._traction;
    }

    //Finally apply acceleration to velocity
    object->vel[kX] += acceleration[kX];
    object->vel[kY] += acceleration[kY];

    if(object->getAttachedPlatform() != nullptr) {
        _platformOffset += acceleration;
    }
}

void ObjectPhysics::updateFriction(const std::shared_ptr<Object> &pchr)
{
    //Apply air/water friction
    //pchr->vel[kX] -= pchr->vel[kX] * (1.0f - pchr->enviro.fluid_friction_hrz);
    //pchr->vel[kY] -= pchr->vel[kY] * (1.0f - pchr->enviro.fluid_friction_hrz);
    //pchr->vel[kZ] -= pchr->vel[kZ] * (1.0f - pchr->enviro.fluid_friction_vrt);

    //This makes it hard for characters to jump uphill
    if(pchr->vel[kZ] > 0.0f && pchr->enviro.is_slippy && !g_meshLookupTables.twist_flat[pchr->enviro.grid_twist]) {
        pchr->vel[kZ] *= 0.8f;
    }

    //Only do floor friction if we are touching the ground
    if(pchr->enviro.grounded) {

        //Apply floor friction
        //pchr->vel[kX] *= pchr->enviro.friction_hrz;
        //pchr->vel[kY] *= pchr->enviro.friction_hrz;

        //Can the character slide on this floor?
        if (pchr->enviro.is_slippy && !_currentModule->getObjectHandler()[pchr->onwhichplatform_ref])
        {
            //Make characters slide down hills
            if(!g_meshLookupTables.twist_flat[pchr->enviro.grid_twist]) {
                const float hillslide = Ego::Physics::g_environment.hillslide * (1.0f - pchr->enviro.zlerp) * (1.0f - pchr->enviro._traction);
                pchr->vel[kX] += g_meshLookupTables.twist_nrm[pchr->enviro.grid_twist][kX] * hillslide;
                pchr->vel[kY] += g_meshLookupTables.twist_nrm[pchr->enviro.grid_twist][kY] * hillslide;

                //Reduce traction while we are sliding downhill
                pchr->enviro._traction *= 0.8f;
            }
            else {
                //TODO: flat icy floor?

                //Reset traction
                pchr->enviro._traction = 1.0f;
            }
        }
        else {
            //Reset traction
            pchr->enviro._traction = 1.0f;
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
        return;
    }

    // save the velocity and acceleration from the last time-step
    pchr->enviro.vel = pchr->getPosition() - pchr->getOldPosition();
    pchr->enviro.acc = pchr->vel - pchr->vel_old;

    // Character's old location
    pchr->vel_old          = pchr->vel;
    pchr->ori_old.facing_z = pchr->ori.facing_z;

    chr_do_latch_button(pchr.get());

    // do friction with the floor before voluntary motion
    updateFriction(pchr);

    updateMovement(pchr);

    updatePlatformPhysics(pchr);

    //Apply gravity
    if(!pchr->isFlying()) {
        pchr->vel[kZ] += pchr->enviro.zlerp * Ego::Physics::g_environment.gravity;
    }
    else {
        pchr->vel[kZ] += (pchr->enviro.fly_level + pchr->getAttribute(Ego::Attribute::FLY_TO_HEIGHT) - pchr->getPosZ()) * FLYDAMPEN;
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

void ObjectPhysics::updateFacing(const std::shared_ptr<Object> &pchr, const Vector2f &desiredVelocity)
{
    //Figure out how to turn around
    switch ( pchr->turnmode )
    {
        // Get direction from ACTUAL change in velocity
        default:
        case TURNMODE_VELOCITY:
            {
                if (desiredVelocity.length_abs() > TURNSPD)
                {
                    if (pchr->isPlayer())
                    {
                        // Players turn quickly
                        pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing(desiredVelocity[kX], desiredVelocity[kY]), 2 );
                    }
                    else
                    {
                        // AI turn slowly
                        pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing(desiredVelocity[kX], desiredVelocity[kY]), 8 );
                    }
                }
            }
            break;

        // Get direction from the DESIRED change in velocity
        case TURNMODE_WATCH:
            {
                if (desiredVelocity.length_abs() > WATCHMIN )
                {
                    pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing(desiredVelocity[kX], desiredVelocity[kY]), 8 );
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

    // update the character's relationship to the ground
    object->enviro.level     = std::max(object->enviro.floor_level, platform->getPosZ() + platform->chr_min_cv._maxs[OCT_Z]);
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

} //Physics
} //Ego
