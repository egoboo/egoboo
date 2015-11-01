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
#include "game/Physics/ObjectPhysics.h"
#include "game/mesh.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int chr_stoppedby_tests = 0;
int chr_pressure_tests = 0;

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
    const std::shared_ptr<Object> &pholder = _currentModule->getObjectHandler()[iholder];

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
        std::shared_ptr<Object> pthrown = _currentModule->spawnObject(pchr->getPosition(), pweapon->getProfileID(), pholder->getTeam().toRef(), pweapon->skin, pchr->ori.facing_z, pweapon->getName(), INVALID_CHR_REF);
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
                        spawn_vrt_offset, pholder->getTeam().toRef(), iholder);

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
						Log::debug("character_swipe() - unable to spawn attack particle for %s\n", weaponProfile->getClassName().c_str());
                    }
                }
            }
            else
            {
				Log::debug("character_swipe() - Invalid attack particle: %s\n", weaponProfile->getClassName().c_str());
            }
        }
        else
        {
            pweapon->ammoknown = true;
        }
    }
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
