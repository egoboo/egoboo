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

/// @file game/char.c
/// @brief Implementation of character functions
/// @details

#include "game/char.h"
#include "game/Inventory.hpp"

#include "egolib/Math/Random.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/Module/Passage.hpp"
#include "game/Module/Module.hpp"
#include "game/GUI/UIManager.hpp"

#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/player.h"
#include "egolib/Script/script.h"
#include "game/graphic_billboard.h"
#include "game/renderer_2d.h"
#include "game/renderer_3d.h"
#include "game/input.h"
#include "game/game.h"
#include "game/egoboo.h"
#include "game/Module/Passage.hpp"
#include "egolib/Profiles/_Include.hpp"
#include "game/Module/Module.hpp"

#include "game/Entities/ObjectHandler.hpp"
#include "game/Entities/ParticleHandler.hpp"
#include "game/ObjectPhysics.h"
#include "game/mesh.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int chr_stoppedby_tests = 0;
int chr_pressure_tests = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void switch_team_base( const CHR_REF character, const TEAM_REF team_new, const bool permanent );

//--------------------------------------------------------------------------------------------
egolib_rv attach_character_to_mount( const CHR_REF irider, const CHR_REF imount, grip_offset_t grip_off )
{
    /// @author ZZ
    /// @details This function attaches one character/item to another ( the holder/mount )
    ///    at a certain vertex offset ( grip_off )
    ///   - This function is called as a part of spawning a module, so the item or the holder may not
    ///     be fully instantiated
    ///   - This function should do very little testing to see if attachment is allowed.
    ///     Most of that checking should be done by the calling function

    Object * prider, * pmount;

    // Make sure the character/item is valid
    if ( !_currentModule->getObjectHandler().exists( irider ) ) return rv_error;
    prider = _currentModule->getObjectHandler().get( irider );

    // Make sure the holder/mount is valid
    if ( !_currentModule->getObjectHandler().exists( imount ) ) return rv_error;
    pmount = _currentModule->getObjectHandler().get( imount );

    //Don't attach a character to itself!
    if(irider == imount) {
        return rv_fail;
    }

    // do not deal with packed items at this time
    // this would have to be changed to allow for pickpocketing
    if ( _currentModule->getObjectHandler().exists( prider->inwhich_inventory ) || _currentModule->getObjectHandler().exists( pmount->inwhich_inventory ) ) return rv_fail;

    // make a reasonable time for the character to remount something
    // for characters jumping out of pots, etc
    if ( imount == prider->dismount_object && prider->dismount_timer > 0 ) return rv_fail;

    // Figure out which slot this grip_off relates to
    slot_t slot = grip_offset_to_slot( grip_off );

    // Make sure the the slot is valid
    if ( !pmount->getProfile()->isSlotValid(slot) ) return rv_fail;

    // This is a small fix that allows special grabbable mounts not to be mountable while
    // held by another character (such as the magic carpet for example)
    // ( this is an example of a test that should not be done here )
    if ( pmount->isMount() && _currentModule->getObjectHandler().exists( pmount->attachedto ) ) return rv_fail;

    // Put 'em together
    prider->inwhich_slot       = slot;
    prider->attachedto         = imount;
    pmount->holdingwhich[slot] = irider;

    // set the grip vertices for the irider
    set_weapongrip( irider, imount, grip_off );

    chr_update_matrix( prider, true );

    prider->setPosition(mat_getTranslate(prider->inst.matrix));

    prider->enviro.inwater  = false;
    prider->jump_timer = JUMPDELAY * 4;

    // Run the held animation
    if ( pmount->isMount() && ( GRIP_ONLY == grip_off ) )
    {
        // Riding imount

        if ( _currentModule->getObjectHandler().exists( prider->holdingwhich[SLOT_LEFT] ) || _currentModule->getObjectHandler().exists( prider->holdingwhich[SLOT_RIGHT] ) )
        {
            // if the character is holding anything, make the animation
            // ACTION_MH == "sitting" so that it dies not look so silly
            chr_play_action( prider, ACTION_MH, true );
        }
        else
        {
            // if it is not holding anything, go for the riding animation
            chr_play_action( prider, ACTION_MI, true );
        }

        // set tehis action to loop
        chr_instance_t::set_action_loop(prider->inst, true);
    }
    else if ( prider->isAlive() )
    {
        /// @note ZF@> hmm, here is the torch holding bug. Removing
        /// the interpolation seems to fix it...
        chr_play_action( prider, ACTION_MM + slot, false );

        chr_instance_t::remove_interpolation(prider->inst);

        // set the action to keep for items
        if ( prider->isItem() )
        {
            // Item grab
            chr_instance_t::set_action_keep(prider->inst, true);
        }
    }

    // Set the team
    if ( prider->isItem() )
    {
        prider->team = pmount->team;

        // Set the alert
        if ( prider->isAlive() )
        {
            SET_BIT( prider->ai.alert, ALERTIF_GRABBED );
        }

        //Lore Master perk identifies everything
        if(pmount->hasPerk(Ego::Perks::LORE_MASTER)) {
            prider->getProfile()->makeUsageKnown();
            prider->nameknown = true;
            prider->ammoknown = true;
        }
    }

    if ( pmount->isMount() )
    {
        pmount->team = prider->team;

        // Set the alert
        if ( !pmount->isItem() && pmount->isAlive() )
        {
            SET_BIT( pmount->ai.alert, ALERTIF_GRABBED );
        }
    }

    // It's not gonna hit the floor
    prider->hitready = false;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
void drop_keys( const CHR_REF character )
{
    Object  * pchr;

    FACING_T direction;
    IDSZ     testa, testz;

    if ( !_currentModule->getObjectHandler().exists( character ) ) return;
    pchr = _currentModule->getObjectHandler().get( character );

    // Don't lose keys in pits...
    if ( pchr->getPosZ() <= ( PITDEPTH >> 1 ) ) return;

    // The IDSZs to find
    testa = MAKE_IDSZ( 'K', 'E', 'Y', 'A' );  // [KEYA]
    testz = MAKE_IDSZ( 'K', 'E', 'Y', 'Z' );  // [KEYZ]

    //check each inventory item
    for(const std::shared_ptr<Object> &pkey : pchr->getInventory().iterate())
    {
        IDSZ idsz_parent;
        IDSZ idsz_type;
        TURN_T turn;

        idsz_parent = chr_get_idsz( pkey->getCharacterID(), IDSZ_PARENT );
        idsz_type   = chr_get_idsz( pkey->getCharacterID(), IDSZ_TYPE );

        //is it really a key?
        if (( idsz_parent < testa && idsz_parent > testz ) &&
            ( idsz_type < testa && idsz_type > testz ) ) continue;

        direction = Random::next(std::numeric_limits<uint16_t>::max());
        turn      = TO_TURN( direction );

        //remove it from inventory
        pchr->getInventory().removeItem(pkey, true);

        // fix the attachments
        pkey->dismount_timer         = PHYS_DISMOUNT_TIME;
        pkey->dismount_object        = pchr->getCharacterID();
        pkey->onwhichplatform_ref    = pchr->onwhichplatform_ref;
        pkey->onwhichplatform_update = pchr->onwhichplatform_update;

        // fix some flags
        pkey->hitready               = true;
        pkey->isequipped             = false;
        pkey->ori.facing_z           = direction + ATK_BEHIND;
        pkey->team                   = pkey->team_base;

        // fix the current velocity
        pkey->vel[kX]                  += turntocos[ turn ] * DROPXYVEL;
        pkey->vel[kY]                  += turntosin[ turn ] * DROPXYVEL;
        pkey->vel[kZ]                  += DROPZVEL;

        // do some more complicated things
        SET_BIT( pkey->ai.alert, ALERTIF_DROPPED );
        pkey->setPosition(pchr->getPosition());
        move_one_character_get_environment( pkey.get() );
        chr_set_floor_level( pkey.get(), pchr->enviro.floor_level );
    }
}

//--------------------------------------------------------------------------------------------
bool drop_all_items( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function drops all of a character's items
    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[character];

    //Drop held items
    const std::shared_ptr<Object> &leftItem = pchr->getLeftHandItem();
    if(leftItem) {
        leftItem->detatchFromHolder(true, false);
    }
    const std::shared_ptr<Object> &rightItem = pchr->getRightHandItem();
    if(rightItem) {
        rightItem->detatchFromHolder(true, false);
    }

    //simply count the number of items in inventory
    uint8_t pack_count = pchr->getInventory().iterate().size();

    //Don't continue if we have nothing to drop
    if(pack_count == 0)
    {
        return true;
    }

    //Calculate direction offset for each object
    const FACING_T diradd = 0xFFFF / pack_count;

    // now drop each item in turn
    FACING_T direction = pchr->ori.facing_z + ATK_BEHIND;
    for(const std::shared_ptr<Object> &pitem : pchr->getInventory().iterate())
    {
        //remove it from inventory
        pchr->getInventory().removeItem(pitem, true);

        // detach the item
        pitem->detatchFromHolder(true, true);

        // fix the attachments
        pitem->dismount_timer         = PHYS_DISMOUNT_TIME;
        pitem->dismount_object        = pchr->getCharacterID();
        pitem->onwhichplatform_ref    = pchr->onwhichplatform_ref;
        pitem->onwhichplatform_update = pchr->onwhichplatform_update;

        // fix some flags
        pitem->hitready               = true;
        pitem->ori.facing_z           = direction + ATK_BEHIND;
        pitem->team                   = pitem->team_base;

        // fix the current velocity
        TURN_T turn                   = TO_TURN( direction );
        pitem->vel[kX]                  += turntocos[ turn ] * DROPXYVEL;
        pitem->vel[kY]                  += turntosin[ turn ] * DROPXYVEL;
        pitem->vel[kZ]                  += DROPZVEL;

        // do some more complicated things
        SET_BIT(pitem->ai.alert, ALERTIF_DROPPED);
        pitem->setPosition(pchr->getPosition());
        move_one_character_get_environment(pitem.get());
        chr_set_floor_level(pitem.get(), pchr->enviro.floor_level);

        //drop out evenly in all directions
        direction += diradd;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void character_swipe( const CHR_REF ichr, slot_t slot )
{
    /// @author ZZ
    /// @details This function spawns an attack particle
    int    spawn_vrt_offset;
    TURN_T turn;
    float  velocity;


    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[ichr];
    if(!pchr) {
        return;
    }

    CHR_REF iweapon = pchr->holdingwhich[slot];

    // See if it's an unarmed attack...
    bool unarmed_attack;
    if ( !_currentModule->getObjectHandler().exists(iweapon) )
    {
        unarmed_attack   = true;
        iweapon          = ichr;
        spawn_vrt_offset = slot_to_grip_offset( slot );  // SLOT_LEFT -> GRIP_LEFT, SLOT_RIGHT -> GRIP_RIGHT
    }
    else
    {
        unarmed_attack   = false;
        spawn_vrt_offset = GRIP_LAST;
    }

    const std::shared_ptr<Object> &pweapon = _currentModule->getObjectHandler()[iweapon];
    const std::shared_ptr<ObjectProfile> &weaponProfile = pweapon->getProfile();

    // find the 1st non-item that is holding the weapon
    CHR_REF iholder = chr_get_lowest_attachment( iweapon, true );

    /*
        if ( iweapon != iholder && iweapon != ichr )
        {
            // This seems to be the "proper" place to activate the held object.
            // If the attack action  of the character holding the weapon does not have
            // MADFX_ACTLEFT or MADFX_ACTRIGHT bits (and so character_swipe function is never called)
            // then the action is played and the ALERTIF_USED bit is set in the chr_do_latch_attack()
            // function.
            //
            // It would be better to move all of this to the character_swipe() function, but we cannot be assured
            // that all models have the proper bits set.

            // Make the iweapon attack too
            chr_play_action( pweapon, ACTION_MJ, false );

            SET_BIT( pweapon->ai.alert, ALERTIF_USED );
        }
    */

    // What kind of attack are we going to do?
    if ( !unarmed_attack && (( weaponProfile->isStackable() && pweapon->ammo > 1 ) || ACTION_IS_TYPE( pweapon->inst.action_which, F ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation
        std::shared_ptr<Object> pthrown = _currentModule->spawnObject(pchr->getPosition(), pweapon->getProfileID(), chr_get_iteam( iholder ), pweapon->skin, pchr->ori.facing_z, pweapon->getName(), INVALID_CHR_REF);
        if (pthrown)
        {
            pthrown->iskursed = false;
            pthrown->ammo = 1;
            SET_BIT( pthrown->ai.alert, ALERTIF_THROWN );

            // deterimine the throw velocity
            velocity = MINTHROWVELOCITY;
            if ( 0 == pthrown->phys.weight )
            {
                velocity += MAXTHROWVELOCITY;
            }
            else
            {
                velocity += FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::MIGHT)) / ( pthrown->phys.weight * THROWFIX );
            }
            velocity = Ego::Math::constrain( velocity, MINTHROWVELOCITY, MAXTHROWVELOCITY );

            turn = TO_TURN( pchr->ori.facing_z + ATK_BEHIND );
            pthrown->vel[kX] += turntocos[ turn ] * velocity;
            pthrown->vel[kY] += turntosin[ turn ] * velocity;
            pthrown->vel[kZ] = DROPZVEL;

            //Was that the last one?
            if ( pweapon->ammo <= 1 ) {
                // Poof the item
                pweapon->requestTerminate();
                return;
            }
            else {
                pweapon->ammo--;
            }
        }
    }
    else
    {
        // A generic attack. Spawn the damage particle.
        if ( 0 == pweapon->ammomax || 0 != pweapon->ammo )
        {
            if ( pweapon->ammo > 0 && !weaponProfile->isStackable() )
            {
                //Is it a wand? (Wand Mastery perk has chance to not use charge)
                if(pweapon->getProfile()->getIDSZ(IDSZ_SKILL) == MAKE_IDSZ('W','A','N','D')
                    && pchr->hasPerk(Ego::Perks::WAND_MASTERY)) {

                    //1% chance per Intellect
                    if(Random::getPercent() <= pchr->getAttribute(Ego::Attribute::INTELLECT)) {
                        chr_make_text_billboard(pchr->getCharacterID(), "Wand Mastery!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::purple(), 3, Billboard::Flags::All);
                    }
                    else {
                        pweapon->ammo--;  // Ammo usage
                    }
                }
                else {
                    pweapon->ammo--;  // Ammo usage
                }
            }

            PIP_REF attackParticle = weaponProfile->getAttackParticleProfile();
            int NR_OF_ATTACK_PARTICLES = 1;

            //Handle Double Shot perk
            if(pchr->hasPerk(Ego::Perks::DOUBLE_SHOT) && weaponProfile->getIDSZ(IDSZ_PARENT) == MAKE_IDSZ('L','B','O','W'))
            {
                //1% chance per Agility
                if(Random::getPercent() <= pchr->getAttribute(Ego::Attribute::AGILITY) && pweapon->ammo > 0) {
                    NR_OF_ATTACK_PARTICLES = 2;
                    chr_make_text_billboard(pchr->getCharacterID(), "Double Shot!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::green(), 3, Billboard::Flags::All);                    

                    //Spend one extra ammo
                    pweapon->ammo--;
                }
            }

            // Spawn an attack particle
            if (INVALID_PIP_REF != attackParticle)
            {
                for(int i = 0; i < NR_OF_ATTACK_PARTICLES; ++i)
                {
                    // make the weapon's holder the owner of the attack particle?
                    // will this mess up wands?
                    std::shared_ptr<Ego::Particle> particle = ParticleHandler::get().spawnParticle(pweapon->getPosition(), 
                        pchr->ori.facing_z, weaponProfile->getSlotNumber(), 
                        attackParticle, weaponProfile->hasAttachParticleToWeapon() ? iweapon : INVALID_CHR_REF,  
                        spawn_vrt_offset, chr_get_iteam(iholder), iholder);

                    if (particle)
                    {
						Vector3f tmp_pos = particle->getPosition();

                        if ( weaponProfile->hasAttachParticleToWeapon() )
                        {
                            particle->phys.weight     = pchr->phys.weight;
                            particle->phys.bumpdampen = pweapon->phys.bumpdampen;

                            particle->placeAtVertex(pweapon, spawn_vrt_offset);
                        }
                        else if ( particle->getProfile()->startontarget && particle->hasValidTarget() )
                        {
                            particle->placeAtVertex(particle->getTarget(), spawn_vrt_offset);

                            // Correct Z spacing base, but nothing else...
                            tmp_pos[kZ] += particle->getProfile()->spacing_vrt_pair.base;
                        }
                        else
                        {
                            // NOT ATTACHED

                            // Don't spawn in walls
                            if ( EMPTY_BIT_FIELD != particle->test_wall( tmp_pos, NULL))
                            {
                                tmp_pos[kX] = pweapon->getPosX();
                                tmp_pos[kY] = pweapon->getPosY();
                                if ( EMPTY_BIT_FIELD != particle->test_wall( tmp_pos, NULL ) )
                                {
                                    tmp_pos[kX] = pchr->getPosX();
                                    tmp_pos[kY] = pchr->getPosY();
                                }
                            }
                        }

                        // Initial particles get a bonus, which may be zero. Increases damage with +(factor)% per attribute point (e.g Might=10 and MightFactor=0.06 then damageBonus=0.6=60%)
                        particle->damage.base += (pchr->getAttribute(Ego::Attribute::MIGHT)     * weaponProfile->getStrengthDamageFactor());
                        particle->damage.base += (pchr->getAttribute(Ego::Attribute::INTELLECT) * weaponProfile->getIntelligenceDamageFactor());
                        particle->damage.base += (pchr->getAttribute(Ego::Attribute::AGILITY)   * weaponProfile->getDexterityDamageFactor());

                        // Initial particles get an enchantment bonus
                        particle->damage.base += pweapon->getAttribute(Ego::Attribute::DAMAGE_BONUS);

                        //Handle traits that increase weapon damage
                        float damageBonus = 1.0f;
                        switch(weaponProfile->getIDSZ(IDSZ_PARENT))
                        {
                            //Wolverine perk gives +100% Claw damage
                            case MAKE_IDSZ('C','L','A','W'):
                                if(pchr->hasPerk(Ego::Perks::WOLVERINE)) {
                                    damageBonus += 1.0f;
                                }
                            break;

                            //+20% damage with polearms
                            case MAKE_IDSZ('P','O','L','E'):
                                if(pchr->hasPerk(Ego::Perks::POLEARM_MASTERY)) {
                                    damageBonus += 0.2f;
                                }
                            break;

                            //+20% damage with swords
                            case MAKE_IDSZ('S','W','O','R'):
                                if(pchr->hasPerk(Ego::Perks::SWORD_MASTERY)) {
                                    damageBonus += 0.2f;
                                }
                            break;

                            //+20% damage with Axes
                            case MAKE_IDSZ('A','X','E','E'):
                                if(pchr->hasPerk(Ego::Perks::AXE_MASTERY)) {
                                    damageBonus += 0.2f;
                                }       
                            break;

                            //+20% damage with Longbows
                            case MAKE_IDSZ('L','B','O','W'):
                                if(pchr->hasPerk(Ego::Perks::BOW_MASTERY)) {
                                    damageBonus += 0.2f;
                                }
                            break;

                            //+100% damage with Whips
                            case MAKE_IDSZ('W','H','I','P'):
                                if(pchr->hasPerk(Ego::Perks::WHIP_MASTERY)) {
                                    damageBonus += 1.0f;
                                }
                            break;
                        }

                        //Improvised Weapons perk gives +100% to some unusual weapons
                        if(pchr->hasPerk(Ego::Perks::IMPROVISED_WEAPONS)) {
                            if (weaponProfile->getIDSZ(IDSZ_PARENT) == MAKE_IDSZ('T','O','R','C')    //Torch
                             || weaponProfile->getIDSZ(IDSZ_TYPE) == MAKE_IDSZ('S','H','O','V')      //Shovel
                             || weaponProfile->getIDSZ(IDSZ_TYPE) == MAKE_IDSZ('P','L','U','N')      //Toilet Plunger
                             || weaponProfile->getIDSZ(IDSZ_TYPE) == MAKE_IDSZ('C','R','O','W')      //Crowbar
                             || weaponProfile->getIDSZ(IDSZ_TYPE) == MAKE_IDSZ('P','I','C','K')) {   //Pick
                                damageBonus += 1.0f;
                            }
                        }

                        //Berserker perk deals +25% damage if you are below 25% life
                        if(pchr->hasPerk(Ego::Perks::BERSERKER) && pchr->getLife() <= pchr->getAttribute(Ego::Attribute::MAX_LIFE)/4) {
                            damageBonus += 0.25f;
                        }
    
                        //If it is a ranged attack then Sharpshooter increases damage by 10%
                        if(pchr->hasPerk(Ego::Perks::SHARPSHOOTER) && weaponProfile->isRangedWeapon() && DamageType_isPhysical(particle->damagetype)) {
                            damageBonus += 0.1f;
                        }

                        //+25% damage with Blunt Weapons Mastery
                        if(particle->damagetype == DAMAGE_CRUSH && pchr->hasPerk(Ego::Perks::BLUNT_WEAPONS_MASTERY) && weaponProfile->isMeleeWeapon()) {
                            damageBonus += 0.25f;
                        }

                        //If it is a melee attack then Brute perk increases damage by 10%
                        if(pchr->hasPerk(Ego::Perks::BRUTE) && weaponProfile->isMeleeWeapon()) {
                            damageBonus += 0.1f;
                        }

                        //Rally Bonus? (+10%)
                        if(pchr->hasPerk(Ego::Perks::RALLY) && update_wld < pchr->getRallyDuration()) {
                            damageBonus += 0.1f;
                        }

                        //Apply damage bonus modifiers
                        particle->damage.base *= damageBonus;
                        particle->damage.rand *= damageBonus;                            

                        //If this is a double shot particle, then add a little space between the arrows
                        if(i > 0) {
                            float x, y;
                            facing_to_vec(particle->facing, &x, &y);
                            tmp_pos[kX] -= x*32.0f;
                            tmp_pos[kY] -= x*32.0f;
                        }

                        particle->setPosition(tmp_pos);
                    }
                    else
                    {
                        log_debug("character_swipe() - unable to spawn attack particle for %s\n", weaponProfile->getClassName().c_str());
                    }
                }
            }
            else
            {
                log_debug("character_swipe() - Invalid attack particle: %s\n", weaponProfile->getClassName().c_str());
            }
        }
        else
        {
            pweapon->ammoknown = true;
        }
    }
}

//--------------------------------------------------------------------------------------------
void drop_money( const CHR_REF character, int money )
{
    /// @author ZZ
    /// @details This function drops some of a character's money

    static const std::array<int, PIP_MONEY_COUNT> vals = {1, 5, 25, 100, 200, 500, 1000, 2000};
    static const std::array<PIP_REF, PIP_MONEY_COUNT> pips =
    {
        PIP_COIN1, PIP_COIN5, PIP_COIN25, PIP_COIN100,
        PIP_GEM200, PIP_GEM500, PIP_GEM1000, PIP_GEM2000
    };

    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[character];
    if(!pchr) {
        return;
    }

	Vector3f loc_pos = pchr->getPosition();

    // limit the about of money to the character's actual money
    if (money > pchr->getMoney()) {
        money = pchr->getMoney();
    }

    if ( money > 0 && loc_pos[kZ] > -2 )
    {
        // remove the money from inventory
        pchr->money = pchr->getMoney() - money;

        // make the particles emit from "waist high"
        loc_pos[kZ] += ( pchr->chr_min_cv._maxs[OCT_Z] + pchr->chr_min_cv._mins[OCT_Z] ) * 0.5f;

        // Give the character a time-out from interacting with particles so it
        // doesn't just grab the money again
        pchr->damage_timer = DAMAGETIME;

        // count and spawn the various denominations
        for (int cnt = PIP_MONEY_COUNT - 1; cnt >= 0 && money >= 0; cnt-- )
        {
            int count = money / vals[cnt];
            money -= count * vals[cnt];

            for ( int tnc = 0; tnc < count; tnc++)
            {
                ParticleHandler::get().spawnGlobalParticle( loc_pos, ATK_FRONT, LocalParticleProfileRef(pips[cnt]), tnc );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool export_one_character_quest_vfs( const char *szSaveName, const CHR_REF character )
{
    /// @author ZZ
    /// @details This function makes the naming.txt file for the character

    player_t *ppla;
    egolib_rv rv;

    if ( !_currentModule->getObjectHandler().exists( character ) ) return false;

    ppla = chr_get_ppla( character );
    if ( NULL == ppla ) return false;

    rv = quest_log_upload_vfs( ppla->quest_log, SDL_arraysize( ppla->quest_log ), szSaveName );
    return TO_C_BOOL( rv_success == rv );
}

//--------------------------------------------------------------------------------------------
bool export_one_character_name_vfs( const char *szSaveName, const CHR_REF character )
{
    /// @author ZZ
    /// @details This function makes the naming.txt file for the character

    if ( !_currentModule->getObjectHandler().exists( character ) ) return false;

    return RandomName::exportName(_currentModule->getObjectHandler()[character]->getName(), szSaveName);
}

//--------------------------------------------------------------------------------------------
void spawn_defense_ping( Object *pchr, const CHR_REF attacker )
{
    /// @author ZF
    /// @details Spawn a defend particle
    if ( 0 != pchr->damage_timer ) return;

    ParticleHandler::get().spawnGlobalParticle( pchr->getPosition(), pchr->ori.facing_z, LocalParticleProfileRef(PIP_DEFEND), 0 );

    pchr->damage_timer    = DEFENDTIME;
    SET_BIT( pchr->ai.alert, ALERTIF_BLOCKED );
    pchr->ai.attacklast = attacker;                 // For the ones attacking a shield
}

//--------------------------------------------------------------------------------------------
void spawn_poof( const CHR_REF character, const PRO_REF profileRef )
{
    /// @author ZZ
    /// @details This function spawns a character poof

    FACING_T facing_z;
    CHR_REF  origin;
    int      cnt;

    Object * pchr;

    if ( !_currentModule->getObjectHandler().exists( character ) ) return;
    pchr = _currentModule->getObjectHandler().get( character );

    const std::shared_ptr<ObjectProfile> &profile = ProfileSystem::get().getProfile(profileRef);
    if (!profile) return;

    origin = pchr->ai.owner;
    facing_z   = pchr->ori.facing_z;
    for ( cnt = 0; cnt < profile->getParticlePoofAmount(); cnt++ )
    {
        ParticleHandler::get().spawnParticle( pchr->pos_old, facing_z, profile->getSlotNumber(), profile->getParticlePoofProfile(),
                            INVALID_CHR_REF, GRIP_LAST, pchr->team, origin, INVALID_PRT_REF, cnt);

        facing_z += profile->getParticlePoofFacingAdd();
    }
}

//--------------------------------------------------------------------------------------------
void switch_team_base( const CHR_REF character, const TEAM_REF team_new, const bool permanent )
{
    Object  * pchr;
    bool   can_have_team;

    if ( !_currentModule->getObjectHandler().exists( character ) ) return;
    pchr = _currentModule->getObjectHandler().get( character );

    // do we count this character as being on a team?
    can_have_team = !pchr->isitem && pchr->isAlive() && !pchr->invictus;

    // take the character off of its old team
    if ( VALID_TEAM_RANGE( pchr->team ) )
    {
        // get the old team index
        TEAM_REF team_old = pchr->team;

        // remove the character from the old team
        if ( can_have_team )
        {
            _currentModule->getTeamList()[team_old].decreaseMorale();
        }

        if ( pchr == _currentModule->getTeamList()[team_old].getLeader().get() )
        {
            _currentModule->getTeamList()[team_old].setLeader(Object::INVALID_OBJECT);
        }
    }

    // make sure we have a valid value
    TEAM_REF loc_team_new = VALID_TEAM_RANGE(team_new) ? team_new : static_cast<TEAM_REF>(Team::TEAM_NULL);

    // place the character onto its new team
    if ( VALID_TEAM_RANGE( loc_team_new ) )
    {
        // switch the team
        pchr->team = loc_team_new;

        // switch the base team only if required
        if ( permanent )
        {
            pchr->team_base = loc_team_new;
        }

        // add the character to the new team
        if ( can_have_team )
        {
            _currentModule->getTeamList()[loc_team_new].increaseMorale();
        }

        // we are the new leader if there isn't one already
        if ( can_have_team && !_currentModule->getTeamList()[loc_team_new].getLeader() )
        {
            _currentModule->getTeamList()[loc_team_new].setLeader(_currentModule->getObjectHandler()[character]);
        }
    }
}

//--------------------------------------------------------------------------------------------
void switch_team( const CHR_REF character, const TEAM_REF team )
{
    /// @author ZZ
    /// @details This function makes a character join another team...

    CHR_REF tmp_ref;
    // change the base object
    switch_team_base( character, team, true );

    // grab a pointer to the character
    if ( !_currentModule->getObjectHandler().exists( character ) ) return;
    Object *pchr = _currentModule->getObjectHandler().get( character );

    // change our mount team as well
    tmp_ref = pchr->attachedto;
    if ( tmp_ref != INVALID_CHR_REF )
    {
        switch_team_base( tmp_ref, team, false );
    }

    // update the team of anything we are holding as well
    tmp_ref = pchr->holdingwhich[SLOT_LEFT];
    if ( tmp_ref != INVALID_CHR_REF )
    {
        switch_team_base( tmp_ref, team, false );
    }

    tmp_ref = pchr->holdingwhich[SLOT_RIGHT];
    if ( tmp_ref != INVALID_CHR_REF )
    {
        switch_team_base( tmp_ref, team, false );
    }
}

//--------------------------------------------------------------------------------------------
bool chr_get_skill( Object *pchr, IDSZ whichskill )
{
    if (!pchr || pchr->isTerminated()) return false;

    //Any [NONE] IDSZ returns always "true"
    if ( IDSZ_NONE == whichskill ) return true;

    // First check the character Skill ID matches
    // Then check for expansion skills too.
    if ( chr_get_idsz( pchr->ai.index, IDSZ_SKILL )  == whichskill ) {
        return true;
    }

    switch(whichskill)
    {
        case MAKE_IDSZ('P', 'O', 'I', 'S'):
            return pchr->hasPerk(Ego::Perks::POISONRY);

        case MAKE_IDSZ('C', 'K', 'U', 'R'):
            return pchr->hasPerk(Ego::Perks::SENSE_KURSES);

        case MAKE_IDSZ('D', 'A', 'R', 'K'):
            return pchr->hasPerk(Ego::Perks::NIGHT_VISION) || pchr->hasPerk(Ego::Perks::PERCEPTIVE);

        case MAKE_IDSZ('A', 'W', 'E', 'P'):
            return pchr->hasPerk(Ego::Perks::WEAPON_PROFICIENCY);

        case MAKE_IDSZ('W', 'M', 'A', 'G'):
            return pchr->hasPerk(Ego::Perks::ARCANE_MAGIC);

        case MAKE_IDSZ('D', 'M', 'A', 'G'):
        case MAKE_IDSZ('H', 'M', 'A', 'G'):
            return pchr->hasPerk(Ego::Perks::DIVINE_MAGIC);

        case MAKE_IDSZ('D', 'I', 'S', 'A'):
            return pchr->hasPerk(Ego::Perks::TRAP_LORE);

        case MAKE_IDSZ('F', 'I', 'N', 'D'):
            return pchr->hasPerk(Ego::Perks::PERCEPTIVE);

        case MAKE_IDSZ('T', 'E', 'C', 'H'):
            return pchr->hasPerk(Ego::Perks::USE_TECHNOLOGICAL_ITEMS);

        case MAKE_IDSZ('S', 'T', 'A', 'B'):
            return pchr->hasPerk(Ego::Perks::BACKSTAB);

        case MAKE_IDSZ('R', 'E', 'A', 'D'):
            return pchr->hasPerk(Ego::Perks::LITERACY);

        case MAKE_IDSZ('W', 'A', 'N', 'D'):
            return pchr->hasPerk(Ego::Perks::THAUMATURGY);

        case MAKE_IDSZ('J', 'O', 'U', 'S'):
            return pchr->hasPerk(Ego::Perks::JOUSTING);            

        case MAKE_IDSZ('T', 'E', 'L', 'E'):
            return pchr->hasPerk(Ego::Perks::TELEPORT_MASTERY); 

    }

    //Skill not found
    return false;
}

//--------------------------------------------------------------------------------------------
void update_all_characters()
{
    /// @author ZZ
    /// @details This function updates stats and such for every character

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        //Skip terminated objects
        if(object->isTerminated()) {
            continue;
        }

        //Update object logic
        object->update();

        //Check if this object should be poofed (destroyed)
        bool timeOut = ( object->ai.poof_time > 0 ) && ( object->ai.poof_time <= static_cast<int32_t>(update_wld) );
        if (timeOut) {
            object->requestTerminate();
        }
    }

    // fix the stat timer
    if ( clock_chr_stat >= ONESECOND )
    {
        // Reset the clock
        clock_chr_stat -= ONESECOND;
    }
}

//--------------------------------------------------------------------------------------------
std::shared_ptr<Billboard> chr_make_text_billboard( const CHR_REF ichr, const char *txt, const Ego::Math::Colour4f& text_color, const Ego::Math::Colour4f& tint, int lifetime_secs, const BIT_FIELD opt_bits )
{
    if (!_currentModule->getObjectHandler().exists(ichr)) {
        return nullptr;
    }
    auto obj_ptr = _currentModule->getObjectHandler()[ichr];

    // Pre-render the text.
    std::shared_ptr<oglx_texture_t> tex;
    try {
        tex = std::make_shared<oglx_texture_t>();
    } catch (...) {
        return nullptr;
    }
    _gameEngine->getUIManager()->getFloatingTextFont()->drawTextToTexture(tex.get(), txt, Ego::Math::Colour3f(text_color.getRed(), text_color.getGreen(), text_color.getBlue()));
    tex->setName("billboard text");

    // Create a new billboard.
    auto billboard = BillboardSystem::get()._billboardList.makeBillboard(lifetime_secs, tex, tint, opt_bits);
    if (!billboard) {
        return nullptr;
    }

    billboard->_obj_wptr = std::weak_ptr<Object>(obj_ptr);
    billboard->_position = obj_ptr->getPosition();

    return billboard;
}

//--------------------------------------------------------------------------------------------
std::string chr_get_dir_name( const CHR_REF ichr )
{
    if (!_currentModule->getObjectHandler().exists(ichr)) {
        return "*INVALID*";
    }
    return _currentModule->getObjectHandler()[ichr]->getProfile()->getPathname();
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_update_collision_size( Object * pchr, bool update_matrix )
{
    /// @author BB
    ///
    /// @details use this function to update the pchr->chr_max_cv and  pchr->chr_min_cv with
    ///       values that reflect the best possible collision volume
    ///
    /// @note This function takes quite a bit of time, so it must only be called when the
    /// vertices change because of an animation or because the matrix changes.
    ///
    /// @todo it might be possible to cache the src[] array used in this function.
    /// if the matrix changes, then you would not need to recalculate this data...

    int       vcount;   // the actual number of vertices, in case the object is square
	Vector4f  src[16];  // for the upper and lower octagon points
	Vector4f  dst[16];  // for the upper and lower octagon points

    int cnt;
    oct_bb_t bsrc, bdst, bmin;

    if ( nullptr == ( pchr ) ) return rv_error;

    // re-initialize the collision volumes
    oct_bb_t::ctor(pchr->chr_min_cv);
    oct_bb_t::ctor(pchr->chr_max_cv);
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        oct_bb_t::ctor(pchr->slot_cv[cnt]);
    }

    // make sure the matrix is updated properly
    if ( update_matrix )
    {
        // call chr_update_matrix() but pass in a false value to prevent a recursize call
        if ( rv_error == chr_update_matrix( pchr, false ) )
        {
            return rv_error;
        }
    }

    // make sure the bounding box is calculated properly
	if (gfx_error == chr_instance_t::update_bbox(pchr->inst)) {
		return rv_error;
	}

    // convert the point cloud in the GLvertex array (pchr->inst.vrt_lst) to
    // a level 1 bounding box. Subtract off the position of the character
    bsrc = pchr->inst.bbox;

    // convert the corners of the level 1 bounding box to a point cloud
    vcount = oct_bb_to_points(bsrc, src, 16);

    // transform the new point cloud
	Utilities::transform(pchr->inst.matrix,src, dst, vcount);

    // convert the new point cloud into a level 1 bounding box
    points_to_oct_bb( bdst, dst, vcount );

    //---- set the bounding boxes
    pchr->chr_min_cv = bdst;
    pchr->chr_max_cv = bdst;

    bmin.assign(pchr->bump);

    // only use pchr->bump.size if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->getProfile()->getBumpOverrideSize() )
    {
        pchr->chr_min_cv.cut(bmin, OCT_X);
        pchr->chr_min_cv.cut(bmin, OCT_Y);

        pchr->chr_max_cv.join(bmin, OCT_X);
        pchr->chr_max_cv.join(bmin, OCT_Y);
    }

    // only use pchr->bump.size_big if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->getProfile()->getBumpOverrideSizeBig() )
    {
        pchr->chr_min_cv.cut(bmin, OCT_XY);
        pchr->chr_min_cv.cut(bmin, OCT_YX);

        pchr->chr_max_cv.join(bmin, OCT_XY);
        pchr->chr_max_cv.join(bmin, OCT_YX);
    }

    // only use pchr->bump.height if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->getProfile()->getBumpOverrideHeight() )
    {
        pchr->chr_min_cv.cut(bmin, OCT_Z);
        pchr->chr_max_cv.join(bmin, OCT_Z );
    }

    //// raise the upper bound for platforms
    //if ( pchr->platform )
    //{
    //    pchr->chr_max_cv.maxs[OCT_Z] += PLATTOLERANCE;
    //}

    // calculate collision volumes for various slots
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        if ( !pchr->getProfile()->isSlotValid( static_cast<slot_t>(cnt) ) ) continue;

        chr_calc_grip_cv( pchr, GRIP_LEFT, &pchr->slot_cv[cnt], false );

        pchr->chr_max_cv.join(pchr->slot_cv[cnt]);
    }

    // convert the level 1 bounding box to a level 0 bounding box
    oct_bb_t::downgrade(bdst, pchr->bump_stt, pchr->bump, pchr->bump_1);

    return rv_success;
}

//--------------------------------------------------------------------------------------------
const oglx_texture_t* chr_get_txtexture_icon_ref( const CHR_REF item )
{
    /// @author BB
    /// @details Get the index to the icon texture (in TxList) that is supposed to be used with this object.
    ///               If none can be found, return the index to the texture of the null icon.

    const std::shared_ptr<Object> &pitem = _currentModule->getObjectHandler()[item];
    if (!pitem) {
        return TextureManager::get().getTexture("mp_data/nullicon").get();
    }

    //Is it a spellbook?
    if (pitem->getProfile()->getSpellEffectType() == ObjectProfile::NO_SKIN_OVERRIDE)
    {
        return pitem->getProfile()->getIcon(pitem->skin).get_ptr();
    }
    else
    {
        return ProfileSystem::get().getSpellBookIcon(pitem->getProfile()->getSpellEffectType()).get_ptr();
    }
}

//--------------------------------------------------------------------------------------------
void chr_set_floor_level( Object * pchr, const float level )
{
    if ( nullptr == ( pchr ) ) return;

    if ( level != pchr->enviro.floor_level )
    {
        pchr->enviro.floor_level = level;
    }
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_lowest_attachment( const CHR_REF ichr, bool non_item )
{
    /// @author BB
    /// @details Find the lowest attachment for a given object.
    ///               This was basically taken from the script function scr_set_TargetToLowestTarget()
    ///
    ///               You should be able to find the holder of a weapon by specifying non_item == true
    ///
    ///               To prevent possible loops in the data structures, use a counter to limit
    ///               the depth of the search, and make sure that ichr != _currentModule->getObjectHandler().get(object)->attachedto

    int cnt;
    CHR_REF original_object, object, object_next;

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return INVALID_CHR_REF;

    original_object = object = ichr;
    for ( cnt = 0, object = ichr; cnt < OBJECTS_MAX; cnt++ )
    {
        // check for one of the ending condiitons
        if ( non_item && !_currentModule->getObjectHandler().get(object)->isitem )
        {
            break;
        }

        // grab the next object in the list
        object_next = _currentModule->getObjectHandler().get(object)->attachedto;

        // check for an end of the list
        if ( !_currentModule->getObjectHandler().exists( object_next ) )
        {
            break;
        }

        // check for a list with a loop. shouldn't happen, but...
        if ( object_next == original_object )
        {
            break;
        }

        // go to the next object
        object = object_next;
    }

    return object;
}

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION (previously inline functions)
//--------------------------------------------------------------------------------------------

TEAM_REF chr_get_iteam( const CHR_REF ichr )
{

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return static_cast<TEAM_REF>(Team::TEAM_DAMAGE);
    Object * pchr = _currentModule->getObjectHandler().get( ichr );

    return static_cast<TEAM_REF>(pchr->team);
}

//--------------------------------------------------------------------------------------------
TEAM_REF chr_get_iteam_base( const CHR_REF ichr )
{
    Object * pchr;
    int iteam;

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return ( TEAM_REF )Team::TEAM_MAX;
    pchr = _currentModule->getObjectHandler().get( ichr );

    iteam = REF_TO_INT( pchr->team_base );
    iteam = CLIP( iteam, 0, (int)Team::TEAM_MAX );

    return ( TEAM_REF )iteam;
}

//--------------------------------------------------------------------------------------------
Team * chr_get_pteam( const CHR_REF ichr )
{
    Object * pchr;

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return NULL;
    pchr = _currentModule->getObjectHandler().get( ichr );

    return &_currentModule->getTeamList()[pchr->team];
}

//--------------------------------------------------------------------------------------------
Team * chr_get_pteam_base( const CHR_REF ichr )
{
    Object * pchr;

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return NULL;
    pchr = _currentModule->getObjectHandler().get( ichr );

    return &_currentModule->getTeamList()[pchr->team_base];
}

//--------------------------------------------------------------------------------------------
chr_instance_t * chr_get_pinstance( const CHR_REF ichr )
{
    Object * pchr;

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return NULL;
    pchr = _currentModule->getObjectHandler().get( ichr );

    return &( pchr->inst );
}

//--------------------------------------------------------------------------------------------
IDSZ chr_get_idsz( const CHR_REF ichr, int type )
{
    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return IDSZ_NONE;
    return _currentModule->getObjectHandler()[ichr]->getProfile()->getIDSZ(type);
}

//--------------------------------------------------------------------------------------------
bool chr_has_idsz( const CHR_REF ichr, IDSZ idsz )
{
    /// @author BB
    /// @details a wrapper for cap_has_idsz

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return IDSZ_NONE;
    return _currentModule->getObjectHandler()[ichr]->getProfile()->hasIDSZ(idsz);
}

//--------------------------------------------------------------------------------------------
bool chr_is_type_idsz( const CHR_REF item, IDSZ test_idsz )
{
    /// @author BB
    /// @details check IDSZ_PARENT and IDSZ_TYPE to see if the test_idsz matches. If we are not
    ///     picky (i.e. IDSZ_NONE == test_idsz), then it matches any valid item.

    if ( !_currentModule->getObjectHandler().exists( item ) ) return IDSZ_NONE;
    return _currentModule->getObjectHandler()[item]->getProfile()->hasTypeIDSZ(test_idsz);
}

//--------------------------------------------------------------------------------------------
bool chr_has_vulnie( const CHR_REF item, const PRO_REF test_profile )
{
    /// @author BB
    /// @details is item vulnerable to the type in profile test_profile?

    IDSZ vulnie;

    if ( !_currentModule->getObjectHandler().exists( item ) ) return false;
    vulnie = chr_get_idsz( item, IDSZ_VULNERABILITY );

    // not vulnerable if there is no specific weakness
    if ( IDSZ_NONE == vulnie ) return false;
    const std::shared_ptr<ObjectProfile> &profile = ProfileSystem::get().getProfile(test_profile);
    if (nullptr == profile) return false;

    // check vs. every IDSZ that could have something to do with attacking
    if ( vulnie == profile->getIDSZ(IDSZ_TYPE) ) return true;
    if ( vulnie == profile->getIDSZ(IDSZ_PARENT) ) return true;

    return false;
}

//--------------------------------------------------------------------------------------------
void chr_init_size( Object * pchr, const std::shared_ptr<ObjectProfile> &profile)
{
    /// @author BB
    /// @details initalize the character size info

    if ( nullptr == ( pchr ) ) return;

    pchr->fat_stt           = profile->getSize();
    pchr->shadow_size_stt   = profile->getShadowSize();
    pchr->bump_stt.size     = profile->getBumpSize();
    pchr->bump_stt.size_big = profile->getBumpSizeBig();
    pchr->bump_stt.height   = profile->getBumpHeight();

    pchr->fat                = pchr->fat_stt;
    pchr->shadow_size_save   = pchr->shadow_size_stt;
    pchr->bump_save.size     = pchr->bump_stt.size;
    pchr->bump_save.size_big = pchr->bump_stt.size_big;
    pchr->bump_save.height   = pchr->bump_stt.height;

    pchr->recalculateCollisionSize();
}
