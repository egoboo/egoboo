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

/// @file game/Entities/Object.hpp
/// @details An object representing instances of in-game egoboo objects (Object)
/// @author Johan Jansen

#define GAME_ENTITIES_PRIVATE 1
#include "game/Entities/Object.hpp"
#include "egolib/Profiles/_Include.hpp"
#include "game/Entities/Object.hpp"
#include "game/Entities/ObjectHandler.hpp"
#include "game/Logic/Player.hpp"
#include "game/game.h"
#include "game/renderer_2d.h"
#include "game/char.h" //ZF> TODO: remove
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/script_implementation.h" //for stealth

//For the minimap
#include "game/Core/GameEngine.hpp"
#include "game/GameStates/PlayingState.hpp"
#include "game/GUI/MiniMap.hpp"

//Declare class static constants
const std::shared_ptr<Object> Object::INVALID_OBJECT = nullptr;


Object::Object(const PRO_REF proRef, ObjectRef objRef) : 
    spawn_data(),
    ai(),
    latch(),
    gender(GENDER_MALE),
    experience(0),
    experiencelevel(0),
    money(0),
    ammomax(0),
    ammo(0),
    holdingwhich(),
    equipment(),
    team(Team::TEAM_NULL),
    team_base(Team::TEAM_NULL),
    fat_stt(0.0f),
    fat(0.0f),
    fat_goto(0.0f),
    fat_goto_time(0),

    jump_timer(JUMPDELAY),
    jumpnumber(0),
    jumpready(false),

    attachedto(),
    inwhich_slot(SLOT_LEFT),
    inwhich_inventory(),
    platform(false),
    canuseplatforms(false),
    holdingweight(0),
    damagetarget_damagetype(DamageType::DAMAGE_SLASH),
    reaffirm_damagetype(DamageType::DAMAGE_SLASH),
    damage_threshold(0),
    is_which_player(INVALID_PLA_REF),
    islocalplayer(false),
    invictus(false),
    iskursed(false),
    nameknown(false),
    ammoknown(false),
    hitready(true),
    isequipped(false),
    isitem(false),
    isshopitem(false),
    canbecrushed(false),
    
    //Misc timers
    grog_timer(0),
    daze_timer(0),
    bore_timer(BORETIME),
    careful_timer(CAREFULTIME),
    reload_timer(0),
    damage_timer(0),

    draw_icon(false),
    sparkle(NOSPARKLE),
    shadow_size_stt(0.0f),
    shadow_size(0),
    shadow_size_save(0),
    is_overlay(false),
    skin(0),
    basemodel_ref(proRef),
    inst(),

    bump_stt(),
    bump(),
    bump_save(),
    bump_1(),      
    chr_max_cv(),  
    chr_min_cv(),  
    slot_cv(),

    stoppedby(0),

    ori(),
    ori_old(),
    bumplist_next(),

    turnmode(TURNMODE_VELOCITY),
    movement_bits(( unsigned )(~0)),    // all movements valid

    inwater(false),
    dismount_timer(0),  /// @note ZF@> If this is != 0 then scorpion claws and riders are dropped at spawn (non-item objects)
    dismount_object(),
    
    _terminateRequested(false),
    _objRef(objRef),
    _profileID(proRef),
    _profile(ProfileSystem::get().getProfile(_profileID)),
    _showStatus(false),
    _isAlive(true),
    _name("*NONE*"),

    _currentLife(0.0f),
    _currentMana(0.0f),
    _baseAttribute(),
    _tempAttribute(),

    _inventory(),
    _perks(),
    _levelUpSeed(Random::next(std::numeric_limits<uint32_t>::max())),

    //Physics
    _objectPhysics(*this),

    //Non-persistent variables
    _hasBeenKilled(false),
    _reallyDuration(0),
    _stealth(false),
    _stealthTimer(0),
    _observationTimer((objRef.get() % ONESECOND) + update_wld), //spread observations so all characters don't happen at the same time

    //Enchants
    _activeEnchants(),
    _lastEnchantSpawned()
{
    // Grip info
    holdingwhich.fill(ObjectRef::Invalid);

    //Clear initial base attributes
    _baseAttribute.fill(0.0f);

    // pack/inventory info
    equipment.fill(ObjectRef::Invalid);

    // Set up position
    ori.map_twist_facing_y = MAP_TURN_OFFSET;  // These two mean on level surface
    ori.map_twist_facing_x = MAP_TURN_OFFSET;

    //Initialize primary attributes
    for(size_t i = 0; i < Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES; ++i) {
        const FRange& baseRange = _profile->getAttributeBase(static_cast<Ego::Attribute::AttributeType>(i));
        _baseAttribute[i] = Random::next(baseRange);
    }
}

Object::~Object()
{
    /// @author ZZ
    /// @details Make character safely deleteable

    // Detach the character from the active game
    if(_currentModule) {
        removeFromGame(this);

        // free the character's inventory
        for(const std::shared_ptr<Object> pitem : _inventory.iterate())
        {
            pitem->requestTerminate();
        }

        // Handle the team
        if ( isAlive() && !getProfile()->isInvincible() )
        {
            _currentModule->getTeamList()[team_base].decreaseMorale();
        }

        if ( _currentModule->getTeamList()[team].getLeader().get() == this )
        {
            _currentModule->getTeamList()[team].setLeader(INVALID_OBJECT);
        }

        // remove any attached particles
        disaffirm_attached_particles(getObjRef());    
    }

    chr_instance_t::dtor(inst);

    EGOBOO_ASSERT( nullptr == inst.vrt_lst );    
}

bool Object::setSkin(const size_t skinNumber)
{
    if(!getProfile()->isValidSkin(skinNumber)) {
        return false;
    }
    const SkinInfo& newSkin = getProfile()->getSkinInfo(skinNumber);

    //Damage resistance and modifiers from Armour
    for(size_t i = 0; i < DAMAGE_COUNT; ++i) {
        _baseAttribute[Ego::Attribute::resistFromDamageType(static_cast<DamageType>(i))] = newSkin.damageResistance[i];
        _baseAttribute[Ego::Attribute::modifierFromDamageType(static_cast<DamageType>(i))] = newSkin.damageModifier[i];
    }

    //Armour movement speed
    _baseAttribute[Ego::Attribute::ACCELERATION] = newSkin.maxAccel;

    //Defence from Armour
    _baseAttribute[Ego::Attribute::DEFENCE] = newSkin.defence;

    //Set new skin
    this->skin = skinNumber;

    //Change the model texture
    if (!this->inst.imad) {
        const std::shared_ptr<Ego::ModelDescriptor> &model = getProfile()->getModel();
        if (chr_instance_t::set_mad(this->inst, model)) {
            getObjectPhysics().updateCollisionSize(true);
        }
    }
    chr_instance_t::set_texture(this->inst, getProfile()->getSkin(this->skin));

    return true;
}


bool Object::isOnWaterTile() const
{
    return 0 != _currentModule->getMeshPointer()->test_fx(getTile(), MAPFX_WATER);
}

bool Object::isSubmerged() const
{
    return isOnWaterTile() && getPosZ() <= _currentModule->getWater().get_level();
}

void Object::movePosition(const float x, const float y, const float z)
{
    _position += Vector3f(x, y, z);
}

void Object::setAlpha(const int alpha)
{
    inst.alpha = Ego::Math::constrain(alpha, 0, 0xFF);

    //This prevents players from becoming completely invisible
    if (isPlayer())
    {
        inst.alpha = std::max<uint8_t>(128, inst.alpha);
    }

    chr_instance_t::update_ref(inst, _currentModule->getMeshPointer()->getElevation(Vector2f(getPosX(), getPosY()), false), false);
}

void Object::setLight(const int light)
{
    inst.light = Ego::Math::constrain(light, 0, 0xFF);

    //This prevents players from becoming completely invisible
    if (isPlayer())
    {
        inst.light = std::max<uint8_t>(128, inst.light);
    }

    chr_instance_t::update_ref(inst, _currentModule->getMeshPointer()->getElevation(Vector2f(getPosX(), getPosY()), false), false);
}

void Object::setSheen(const int sheen)
{
    inst.sheen = Ego::Math::constrain(sheen, 0, 0xFF);
    chr_instance_t::update_ref(inst, _currentModule->getMeshPointer()->getElevation(Vector2f(getPosX(), getPosY()), false), false);
}

bool Object::canMount(const std::shared_ptr<Object> mount) const
{
    //Cannot mount ourselves!
    if(this == mount.get())
    {
        return false;
    }

    //Make sure they are a mount and alive
    if(!mount->isMount() || !mount->isAlive())
    {
        return false;
    }

    //We must be alive and not an item to become a rider
    if(!isAlive() || isitem || isBeingHeld())
    {
        return false;
    }

    //Cannot mount while flying
    if(isFlying())
    {
        return false;
    }

    //Make sure they aren't mounted already
    if(!mount->getProfile()->isSlotValid(SLOT_LEFT) || _currentModule->getObjectHandler().exists(mount->holdingwhich[SLOT_LEFT]))
    {
        return false;
    }

    //We need a riding animation to be able to mount stuff
    int action_mi = getProfile()->getModel()->getAction(ACTION_MI);
    bool has_ride_anim = ( ACTION_COUNT != action_mi && !ACTION_IS_TYPE( action_mi, D ) );

    return has_ride_anim;
}

int Object::damage(const FACING_T direction, const IPair  damage, const DamageType damagetype, const TEAM_REF attackerTeam,
                   const std::shared_ptr<Object> &attacker, const bool ignoreArmour, const bool setDamageTime, const bool ignoreInvictus)
{
    int action;
    bool do_feedback = (Ego::FeedbackType::None != egoboo_config_t::get().hud_feedback.getValue());

    // Simply ignore damaging invincible targets.
    if(invictus && !ignoreInvictus)
    {
        return 0;
    }

    // Don't continue if there is no damage or the character isn't alive.
    int max_damage = std::abs( damage.base ) + std::abs( damage.rand );
    if ( !isAlive() || 0 == max_damage ) return 0;

    // make a special exception for DAMAGE_DIRECT
    uint8_t damageModifier = ( damagetype >= DAMAGE_COUNT ) ? 0 : getAttribute(Ego::Attribute::modifierFromDamageType(damagetype));

    // determine some optional behavior
    bool friendly_fire = false;
    if ( !attacker )
    {
        do_feedback = false;
    }
    else
    {
        // do not show feedback for damaging yourself
        if ( attacker.get() == this )
        {
            do_feedback = false;
        }

        // identify friendly fire for color selection :)
        if ( getTeam() == attacker->getTeam() )
        {
            friendly_fire = true;
        }

        // don't show feedback from random objects hitting each other
        //if ( !attacker->show_stats )
        //{
        //    do_feedback = false;
        //}

        // don't show damage to players since they get feedback from the status bars
        //if ( show_stats || VALID_PLA( is_which_player ) )
        //{
        //    do_feedback = false;
        //}
    }

    // Lessen actual damage taken by resistance
    // This can also be used to lessen effectiveness of healing
    int base_damage = Random::next(damage.base, damage.base+damage.rand);
    int actual_damage = base_damage - base_damage*getDamageReduction(damagetype, !ignoreArmour);

    // Increase electric damage when in water
    if (damagetype == DAMAGE_ZAP && isSubmerged() && _currentModule->getWater()._is_water)
    {
        actual_damage *= 2.0f;     /// @note ZF> Is double damage too much?
    }

    // Allow actual_damage to be dealt to mana (mana shield spell)
    if (HAS_SOME_BITS(damageModifier, DAMAGEMANA))
    {
        setMana(getMana() - FP8_TO_FLOAT(actual_damage));
        actual_damage -= std::max<int>(FLOAT_TO_FP8(getMana()) - actual_damage, 0);
        updateLastAttacker(attacker, false);
    }

    // Allow charging (Invert actual_damage to mana)
    if (HAS_SOME_BITS(damageModifier, DAMAGECHARGE))
    {
        setMana(getMana() + FP8_TO_FLOAT(actual_damage));
        return 0;
    }

    // Invert actual_damage to heal
    if (HAS_SOME_BITS(damageModifier, DAMAGEINVERT))
    {
        actual_damage = -actual_damage;        
    }

    // Remember the actual_damage type
    ai.damagetypelast = damagetype;
    ai.directionlast  = direction;

    // Check for characters who are immune to this damage, no need to continue if they have
    bool immune_to_damage = HAS_SOME_BITS(damageModifier, DAMAGEINVICTUS) || (actual_damage > 0 && actual_damage <= damage_threshold);
    if ( immune_to_damage && !ignoreInvictus )
    {
        actual_damage = 0;

        //Tell that the character is simply immune to the damage
        //but don't do message and ping for mounts, it's just irritating
        if ( !isMount() && 0 == damage_timer )
        {
            //Ping!
            ParticleHandler::get().spawnDefencePing(toSharedPointer(), attacker);

            //Only draw "Immune!" if we are truly completely immune and it was not simply a weak attack
            if(HAS_SOME_BITS(damageModifier, DAMAGEINVICTUS) || damage.base + damage.rand <= damage_threshold) {
                BillboardSystem::get().makeBillboard(_objRef, "Immune!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f(0, 0.5, 0, 1), 3, Billboard::Flags::All);
            }
        }
    }

    // Do it already
    if ( actual_damage > 0 )
    {
        // Only actual_damage if not invincible
        if ( 0 == damage_timer || ignoreInvictus )
        {
            // Normal mode reduces damage dealt by monsters with 30%!
            if (egoboo_config_t::get().game_difficulty.getValue() == Ego::GameDifficulty::Normal && isPlayer())
            {
                actual_damage *= 0.70f;
            }

            // Easy mode deals 25% extra actual damage by players and 50% less to players
            if (attacker && egoboo_config_t::get().game_difficulty.getValue() <= Ego::GameDifficulty::Easy)
            {
                if ( attacker->isPlayer()  && !isPlayer() ) actual_damage *= 1.25f;
                if ( !attacker->isPlayer() &&  isPlayer() ) actual_damage *= 0.5f;
            }

            if ( 0 != actual_damage )
            {
                _currentLife -= FP8_TO_FLOAT(actual_damage);

                // Spawn blud particles
                if ( _profile->getBludType() )
                {
                    if ( _profile->getBludType() == ULTRABLUDY || ( base_damage > HURTDAMAGE && DamageType_isPhysical( damagetype ) ) )
                    {
                        ParticleHandler::get().spawnParticle( getPosition(), ori.facing_z + direction, _profile->getSlotNumber(), _profile->getBludParticleProfile(),
                                            ObjectRef::Invalid, GRIP_LAST, attackerTeam, _objRef);
                    }
                }

                // Set attack alert if it wasn't an accident
                if ( base_damage > HURTDAMAGE )
                {
                    if ( attackerTeam == Team::TEAM_DAMAGE )
                    {
                        ai.setLastAttacker(ObjectRef::Invalid);
                    }
                    else
                    {
                        updateLastAttacker(attacker, false );
                    }
                }

                //Did we survive?
                if (_currentLife <= 0)
                {
                    this->kill(attacker, ignoreInvictus);
                }
                else
                {
                    //Yes, but play the hurt animation
                    action = ACTION_HA;
                    if ( base_damage > HURTDAMAGE )
                    {
                        //If we have Endurance perk, we have 1% chance per Might to resist hurt animation (which cause a minor delay)
                        if(!hasPerk(Ego::Perks::ENDURANCE) || Random::getPercent() > getAttribute(Ego::Attribute::MIGHT))
                        {
                            if(inst.imad->isActionValid(ACTION_HA)) {
                                action += Random::next(3);
                                chr_play_action(this, action, false);
                            }
                        }

                        // Make the character invincible for a limited time only
                        if (setDamageTime)
                        {
                            damage_timer = DAMAGETIME;
                        }
                    }
                }
            }

            /// @test spawn a fly-away damage indicator?
            if ( do_feedback )
            {
#if 0     
                const char * tmpstr;
                int rank;


                tmpstr = describe_wounds( life_max, life );

                tmpstr = describe_value( actual_damage, UINT_TO_UFP8( 10 ), &rank );
                if ( rank < 4 )
                {
                    tmpstr = describe_value( actual_damage, max_damage, &rank );
                    if ( rank < 0 )
                    {
                        tmpstr = "Fumble!";
                    }
                    else
                    {
                        tmpstr = describe_damage( actual_damage, life_max, &rank );
                        if ( rank >= -1 && rank <= 1 )
                        {
                            tmpstr = describe_wounds( life_max, life );
                        }
                    }
                }

                if ( NULL != tmpstr )
#endif
                {
                    const int lifetime = 2;
                    STRING text_buffer = EMPTY_CSTR;

                    // friendly damage = "purple"
                    // @todo MH: The colour here is approximately "mauve" and it is already associated with "holy" damage.
                    const auto tint_friend = Ego::Math::Colour4f(0.88, 0.75, 1, 1);

                    // enemy damage color depends on damage type
                    const auto tint_enemy = Ego::Math::Colour4f(DamageType_getColour(damagetype), 1);

                    // write the string into the buffer
                    snprintf( text_buffer, SDL_arraysize( text_buffer ), "%.1f", static_cast<float>(actual_damage) / 256.0f );

                    //Size depends on the amount of damage (more = bigger)
                    //TODO: not implemented                    

                    BillboardSystem::get().makeBillboard(_objRef, text_buffer, Ego::Math::Colour4f::white(), friendly_fire ? tint_friend : tint_enemy, lifetime, Billboard::Flags::All );
                }
            }
        }
    }

    // Heal 'em instead
    else if ( actual_damage < 0 )
    {
        heal(attacker, -actual_damage, ignoreInvictus);

        // Isssue an alert
        if ( attackerTeam == Team::TEAM_DAMAGE )
        {
            ai.setLastAttacker(ObjectRef::Invalid);
        }

        /// @test spawn a fly-away heal indicator?
#if 0
        if ( do_feedback )
        {
            const float lifetime = 3;
            STRING text_buffer = EMPTY_CSTR;

            // "white" text
            const auto text_color = Ego::Math::Colour4f::white();
            // heal == yellow, right ;)
            const auto tint = Ego::Math::Colour4f(1, 1, 0.75, 1);

            // write the string into the buffer
            snprintf( text_buffer, SDL_arraysize( text_buffer ), "%s", describe_value( -actual_damage, damage.base + damage.rand, NULL ) );

            chr_make_text_billboard(_objRef, text_buffer, text_color, tint, lifetime, Billboard::Flags::All );
        }
#endif
    }

    return actual_damage;
}

void Object::updateLastAttacker(const std::shared_ptr<Object> &attacker, bool healing)
{
    // Don't let characters chase themselves...  That would be silly
    if ( this == attacker.get() ) return;

    // Don't alert the character too much if under constant fire
    if (0 != careful_timer) return;

    ObjectRef actual_attacker = ObjectRef::Invalid;

    // Figure out who is the real attacker, in case we are a held item or a controlled mount
    if(attacker)
    {
        actual_attacker = attacker->getObjRef();

        //Dont alert if the attacker/healer was on the null team
        if(attacker->getTeam() == Team::TEAM_NULL) {
            return;
        }
    
        //Do not alert items damaging (or healing) their holders, healing potions for example
        if ( attacker->attachedto == ai.getSelf() ) return;

        //If we are held, the holder is the real attacker... unless the holder is a mount
        if ( attacker->isBeingHeld() && !_currentModule->getObjectHandler().get(attacker->attachedto)->isMount() )
        {
            actual_attacker = attacker->attachedto;
        }

        //If the attacker is a mount, try to blame the rider
        else if ( attacker->isMount() && _currentModule->getObjectHandler().exists( attacker->holdingwhich[SLOT_LEFT] ) )
        {
            actual_attacker = attacker->holdingwhich[SLOT_LEFT];
        }
    }

    //Update alerts and timers
    ai.setLastAttacker(actual_attacker);
    SET_BIT(ai.alert, healing ? ALERTIF_HEALED : ALERTIF_ATTACKED);
    careful_timer = CAREFULTIME;
}

bool Object::heal(const std::shared_ptr<Object> &healer, const UFP8_T amount, const bool ignoreInvincibility)
{
    //Don't heal dead and invincible stuff
    if (!isAlive() || (invictus && !ignoreInvincibility)) return false;

    //This actually heals the character
    setLife(_currentLife + FP8_TO_FLOAT(amount));

    //With Magical Attunement perk 25% of healing effects also refills mana
    if(hasPerk(Ego::Perks::MAGIC_ATTUNEMENT)) {
        setMana(_currentMana + FP8_TO_FLOAT(amount)*0.25f);
    }

    // Set alerts, but don't alert that we healed ourselves
    if (healer && this != healer.get() && healer->attachedto != _objRef && amount > HURTDAMAGE)
    {
        updateLastAttacker(healer, true);
    }

    return true;
}

bool Object::isAttacking() const
{
    return inst.action_which >= ACTION_UA && inst.action_which <= ACTION_FD;
}

bool Object::teleport(const Vector3f& position, const FACING_T facing_z)
{
    //Cannot teleport outside the level
    if(!_currentModule->isInside(position[kX], position[kY])) return false;

    Vector3f newPosition = position;

    //Cannot teleport inside a wall
	Vector2f nrm;
    if ( !hit_wall(newPosition, nrm, NULL) )
    {
        // Yeah!  It worked!

        // update the old position
        ori_old.facing_z = facing_z;

        // update the new position
        setPosition(newPosition);
        ori.facing_z = facing_z;

        if (!detatchFromHolder(true, false))
        {
            // detach_character_from_mount() updates the character matrix unless it is not mounted
            chr_update_matrix(this, true);
        }

        return true;
    }

    return false;
}

void Object::update()
{
    //Update active enchantments on this Object
    if(!_activeEnchants.empty()) {
        _activeEnchants.remove_if([this](const std::shared_ptr<Ego::Enchantment> &enchant) 
            {
                //Update enchantment 
                enchant->update();

                //Remove all terminated enchants
                if(enchant->isTerminated()) {
                    enchant->playEndSound();

                    if(enchant->getProfile()->killtargetonend) {
                        this->kill(enchant->getOwner(), true);
                    }

                    return true;
                }

                return false; 
            });
    }

    // the following functions should not be done the first time through the update loop
    if (0 == update_wld) return;

    //Don't do items that are inside an inventory
    if (isInsideInventory()) {
        return;
    }

    // do the character interaction with water
    if (!isHidden() && isSubmerged() && !isScenery())
    {
        // do splash when entering water the first time
        if (!inwater)
        {
            // Splash
            ParticleHandler::get().spawnGlobalParticle(Vector3f(getPosX(), getPosY(), _currentModule->getWater().get_level() + RAISE), ATK_FRONT, LocalParticleProfileRef(PIP_SPLASH), 0);

            if ( _currentModule->getWater()._is_water )
            {
                SET_BIT(ai.alert, ALERTIF_INWATER);
            }
        }

        //Submerged in water (fully or partially)
        else
        {
            // Ripples
            if(isAlive())
            {
                if ( !isBeingHeld() && getProfile()->causesRipples()
                    && getPosZ() + chr_min_cv._maxs[OCT_Z] + RIPPLETOLERANCE > _currentModule->getWater().get_level() 
                    && getPosZ() + chr_min_cv._mins[OCT_Z] < _currentModule->getWater().get_level())
                {
                    // suppress ripples if we are far below the surface
                    int ripple_suppression = _currentModule->getWater().get_level() - (getPosZ() + chr_min_cv._maxs[OCT_Z]);
                    ripple_suppression = ( 4 * ripple_suppression ) / RIPPLETOLERANCE;
                    ripple_suppression = Ego::Math::constrain(ripple_suppression, 0, 4);

                    // make more ripples if we are moving
                    ripple_suppression -= (( int )vel[kX] != 0 ) | (( int )vel[kY] != 0 );

                    int ripand;
                    if ( ripple_suppression > 0 )
                    {
                        ripand = ~(RIPPLEAND << ripple_suppression);
                    }
                    else
                    {
                        ripand = RIPPLEAND >> ( -ripple_suppression );
                    }

                    if ( 0 == ( (update_wld + getObjRef().get()) & ripand ))
                    {
                        ParticleHandler::get().spawnGlobalParticle(Vector3f(getPosX(), getPosY(), _currentModule->getWater().get_level()), ATK_FRONT, LocalParticleProfileRef(PIP_RIPPLE), 0);
                    }
                }
            }

            if (_currentModule->getWater()._is_water && HAS_NO_BITS(update_wld, 7))
            {
                jumpready = true;
                jumpnumber = 1; //Limit to 1 jump while in water
            }
        }

        inwater = true;
    }
    else
    {
        inwater = false;
    }

    //---- Do timers and such

    // reduce attack cooldowns
    if ( reload_timer > 0 ) reload_timer--;

    // decrement the dismount timer
    if ( dismount_timer > 0 ) dismount_timer--;

    if ( 0 == dismount_timer ) {
        dismount_object = ObjectRef::Invalid;
    }

    // Down jump timer
    if(jump_timer > 0) {
        if (isBeingHeld() || getObjectPhysics().isTouchingGround() || jumpnumber > 0) { 
            jump_timer--;
        }
    }
    // Down that ol' damage timer
    if ( damage_timer > 0 ) damage_timer--;

    // Do "Be careful!" delay
    if ( careful_timer > 0 ) careful_timer--;

    //Reduce stealth timeout
    if(_stealthTimer > 0) _stealthTimer--;

    // Texture movement
    inst.uoffset += getProfile()->getTextureMovementRateX();
    inst.voffset += getProfile()->getTextureMovementRateY();

    // Texture tint
    inst.redshift = Ego::Math::constrain<int>(1 + getAttribute(Ego::Attribute::RED_SHIFT), 0, 6);
    inst.grnshift = Ego::Math::constrain<int>(1 + getAttribute(Ego::Attribute::GREEN_SHIFT), 0, 6);
    inst.blushift = Ego::Math::constrain<int>(1 + getAttribute(Ego::Attribute::BLUE_SHIFT), 0, 6);
    chr_instance_t::update_ref(inst, _currentModule->getMeshPointer()->getElevation(Vector2f(getPosX(), getPosY()), false), false); //update reflection as well

    // do the mana and life regeneration for "living" characters
    if (isAlive()) {
        _currentMana += getAttribute(Ego::Attribute::MANA_REGEN) / GameEngine::GAME_TARGET_UPS;
        _currentMana = Ego::Math::constrain(_currentMana, 0.0f, getAttribute(Ego::Attribute::MAX_MANA));

        _currentLife += getAttribute(Ego::Attribute::LIFE_REGEN) / GameEngine::GAME_TARGET_UPS;
        _currentLife = Ego::Math::constrain(_currentLife, 0.01f, getAttribute(Ego::Attribute::MAX_LIFE));
    }

    // Do stats once every second
    if ( clock_chr_stat >= ONESECOND )
    {
        // check for a level up
        checkLevelUp();

        // countdown confuse effects
        if (grog_timer > 0) {
           grog_timer--;
        }

        if (daze_timer > 0) {
           daze_timer--;
        }

        // update some special skills (players and NPC's)
        if(getShowStatus())
        {
            //Cartography perk reveals the minimap
            if(hasPerk(Ego::Perks::CARTOGRAPHY)) {
                _gameEngine->getActivePlayingState()->getMiniMap()->setVisible(true);
            }

            //Navigation reveals the players position on the minimap
            if(hasPerk(Ego::Perks::NAVIGATION)) {
                _gameEngine->getActivePlayingState()->getMiniMap()->setShowPlayerPosition(true);
            }

            //Danger Sense reveals enemies on the minimap
            if(hasPerk(Ego::Perks::DANGER_SENSE)) {
                local_stats.sense_enemies_team = this->team;
                local_stats.sense_enemies_idsz = IDSZ2::None;     //Reveal all
            }

            //Danger Sense reveals enemies on the minimap
            else if(hasPerk(Ego::Perks::SENSE_UNDEAD)) {
                local_stats.sense_enemies_team = this->team;
                local_stats.sense_enemies_idsz = IDSZ2('U','N','D','E');     //Reveal only undead
            }
        }        

        //Give Rally bonus to friends within 6 tiles
        if(hasPerk(Ego::Perks::RALLY)) {
            for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().findObjects(getPosX(), getPosY(), WIDE, false))
            {
                //Only valid objects that are on our team
                if(object->isTerminated() || object->getTeam() != getTeam()) continue;

                //Don't give bonus to ourselves!
                if(object.get() == this) continue;

                object->_reallyDuration = update_wld + GameEngine::GAME_TARGET_UPS*3;    //Apply bonus for 3 seconds
            }
        }
    }

    //Try to detect any hidden objects every so often (unless we are scenery object) 
    if(!isScenery() && isAlive() && !isBeingHeld() && inst.action_which != ACTION_MK) {  //ACTION_MK = sleeping
        if(update_wld > _observationTimer) 
        {
            _observationTimer = update_wld + ONESECOND;

            //Setup line of sight data
            line_of_sight_info_t lineOfSightInfo;
            lineOfSightInfo.x0         = getPosX();
            lineOfSightInfo.y0         = getPosY();
            lineOfSightInfo.z0         = getPosZ() + std::max(1.0f, bump.height);
            lineOfSightInfo.stopped_by = stoppedby;

            //Check for nearby enemies
            std::vector<std::shared_ptr<Object>> nearbyObjects = _currentModule->getObjectHandler().findObjects(getPosX(), getPosY(), WIDE, false);
            for(const std::shared_ptr<Object> &target : nearbyObjects) {
                //Valid objects only
                if(target->isTerminated() || target->isHidden()) continue;

                //Only look for stealthed objects
                if(!target->isStealthed()) continue;

                //Are they a enemy of us?
                if(!target->getTeam().hatesTeam(getTeam())) {
                    continue;
                }

                //Can we see them?
                lineOfSightInfo.x1 = target->getPosX();
                lineOfSightInfo.y1 = target->getPosY();
                lineOfSightInfo.z1 = target->getPosZ() + std::max(1.0f, target->bump.height);
                if (line_of_sight_info_t::blocked(lineOfSightInfo, _currentModule->getMeshPointer())) {
                    continue;
                }

                //Sense Invisible = automatic detection
                if(target->canSeeInvisible()) {
                    target->deactivateStealth();
                    target->_stealthTimer = ONESECOND * 6; //6 second timeout
                    break;
                }

                //Check for detection chance, Base chance 20%
                int chance = 20;

                //+0.5% per Intellect
                chance += getAttribute(Ego::Attribute::INTELLECT)*0.5f;

                //-0.5% per target Agility
                chance -= target->getAttribute(Ego::Attribute::AGILITY)*0.5f;

                //-5% per tile distance
                chance -= 5 * ((getPosition()-target->getPosition()).length() / Info<float>::Grid::Size());

                //Perceptive Perk doubles chance
                if(target->hasPerk(Ego::Perks::PERCEPTIVE)) {
                    chance *= 2;
                }

                //If they are not looking towards us, then halve detection chance
                if(!target->isFacingLocation(getPosX(), getPosY())) {
                    chance /= 2;
                }

                //Were they detected by us?
                if(Random::getPercent() <= chance) {
                    target->deactivateStealth();
                    target->_stealthTimer = ONESECOND * 6; //6 second timeout
                    break;
                }
            }
        }
    }

    //Finally update model resizing effects
    updateResize();
}

void Object::updateResize()
{
    if (fat_goto_time < 0) {
        return;
    }

    if (fat_goto != fat)
    {
        int bump_increase = ( fat_goto - fat ) * 0.10f * bump.size;

        // Make sure it won't get caught in a wall
        bool willgetcaught = false;
        if ( fat_goto > fat )
        {
            bump.size += bump_increase;

            if ( EMPTY_BIT_FIELD != Collidable::test_wall() )
            {
                willgetcaught = true;
            }

            bump.size -= bump_increase;
        }

        // If it is getting caught, simply halt growth until later
        if ( !willgetcaught )
        {
            // Figure out how big it is
            fat_goto_time--;

            float newsize = fat_goto;
            if ( fat_goto_time > 0 )
            {
                newsize = ( fat * 0.90f ) + ( newsize * 0.10f );
            }

            // Make it that big...
            setFat(newsize);

            if ( CAP_INFINITE_WEIGHT == getProfile()->getWeight() )
            {
                phys.weight = CHR_INFINITE_WEIGHT;
            }
            else
            {
                phys.weight = std::min<uint32_t>(getProfile()->getWeight() * fat * fat * fat, CHR_MAX_WEIGHT);
            }
        }
    }
}

std::string Object::getName(bool prefixArticle, bool prefixDefinite, bool capitalLetter) const
{
    std::string result;

    if (isNameKnown())
    {
        result = _name;

        // capitalize the name ?
        if (capitalLetter)
        {
            result[0] = Ego::toupper(result[0]);
        }
    }
    else
    {
        //Are we a spellbook?
        if(getProfile()->getSpellEffectType() != ObjectProfile::NO_SKIN_OVERRIDE) {
            result = ProfileSystem::get().getProfile(SPELLBOOK)->getClassName();
        }
        else {
            result = getProfile()->getClassName();
        }

        if (prefixArticle)
        {
            // capitalize the name ?
            if (capitalLetter)
            {
                result[0] = std::toupper(result[0]);
            }

            if (prefixDefinite)
            {
                result.insert(0, "the ");
            }
            else
            {
                char lTmp = Ego::toupper(result[0]);

                if ( 'A' == lTmp || 'E' == lTmp || 'I' == lTmp || 'O' == lTmp || 'U' == lTmp )
                {
                    result.insert(0, "an ");
                }
                else
                {
                    result.insert(0, "a ");
                }
            }
        }
    }

    return result;
}

void Object::requestTerminate() 
{
    //Mark object as terminated
    _currentModule->getObjectHandler().remove(getObjRef());
}


 bool Object::isFacingLocation(const float x, const float y) const
 {
    FACING_T facing = vec_to_facing(x - getPosX(), y - getPosY());
    facing -= ori.facing_z;
    return (facing > 55535 || facing < 10000);
 }

bool Object::detatchFromHolder(const bool ignoreKurse, const bool doShop)
{
    // Make sure the character is actually held by something first
    ObjectRef holder = attachedto;
    const std::shared_ptr<Object> &pholder = _currentModule->getObjectHandler()[holder];
    if (!pholder) {
        return false;  
    } 

    // Don't allow living characters to drop kursed weapons
    if ( !ignoreKurse && iskursed && pholder->isAlive() && isitem )
    {
        SET_BIT( ai.alert, ALERTIF_NOTDROPPED );
        return false;
    }

    // set the dismount timer
    dismount_timer  = PHYS_DISMOUNT_TIME;
    dismount_object = holder;

    // Figure out which hand it's in
    uint16_t hand = inwhich_slot;

    // Rip 'em apart
    attachedto = ObjectRef::Invalid;
	if (pholder->holdingwhich[SLOT_LEFT] == getObjRef()) {
		pholder->holdingwhich[SLOT_LEFT] = ObjectRef::Invalid;
	}

	if (pholder->holdingwhich[SLOT_RIGHT] == getObjRef()) {
		pholder->holdingwhich[SLOT_RIGHT] = ObjectRef::Invalid;
	}

    if ( isAlive() )
    {
        // play the falling animation...
        chr_play_action( this, ACTION_JB + hand, false );
    }
    else if ( inst.action_which < ACTION_KA || inst.action_which > ACTION_KD )
    {
        // play the "killed" animation...
        chr_play_action( this, Random::next((int)ACTION_KA, ACTION_KA + 3), false );
        chr_instance_t::set_action_keep(inst, true);
    }

    // Set the positions
    if ( chr_matrix_valid( this ) )
    {
        setPosition(mat_getTranslate(inst.matrix));
    }
    else
    {
        setPosition(pholder->getPosition());
    }

    // Make sure it's not dropped in a wall...
    if (EMPTY_BIT_FIELD != Collidable::test_wall())
    {
        Vector3f pos_tmp = pholder->getPosition();
        pos_tmp[kZ] = getPosZ();

        setPosition(pos_tmp);
    }

    // Check for shop passages
    bool inshop = false;
    if ( doShop )
    {
        inshop = Shop::drop(pholder, shared_from_this());
    }

    // Make sure it works right
    hitready = true;
    if ( inshop )
    {
        // Drop straight down to avoid theft
        vel[kX] = 0;
        vel[kY] = 0;
    }
    else
    {
        vel[kX] = pholder->vel[kX];
        vel[kY] = pholder->vel[kY];
    }

    //Throw us forward if we can collide with the holder (for example the Stool)
    //This prevents us from being dropped into the collision box of the holder
    if(bump.size > 0) {
        float angle = FACING_TO_RAD(ori.facing_z + ATK_BEHIND);
        vel[kX] += std::cos(angle) * DROPXYVEL * 0.5f;
        vel[kY] += std::sin(angle) * DROPXYVEL * 0.5f;
    }

    vel[kZ] = DROPZVEL;

    // Turn looping off
    chr_instance_t::set_action_loop(inst, false);

    // Reset the team if it is a mount
    if ( pholder->isMount() )
    {
        pholder->team = pholder->team_base;
        SET_BIT( pholder->ai.alert, ALERTIF_DROPPED );
    }

    team = team_base;
    SET_BIT( ai.alert, ALERTIF_DROPPED );

    // Reset transparency
    if ( isitem && pholder->getProfile()->transferBlending() )
    {
        setAlpha(getProfile()->getAlpha());
        setLight(getProfile()->getLight());
    }

    // Set twist
    ori.map_twist_facing_y = MAP_TURN_OFFSET;
    ori.map_twist_facing_x = MAP_TURN_OFFSET;

    // turn off keeping, unless the object is dead
    if (!isAlive())
    {
        // the object is dead. play the killed animation and make it freeze there
        chr_play_action( this, Random::next((int)ACTION_KA, ACTION_KA + 3), false );
        chr_instance_t::set_action_keep(inst, true);
    }
    else
    {
        // play the jump animation, and un-keep it
        chr_play_action( this, ACTION_JA, true );
        chr_instance_t::set_action_keep(inst, false);
    }

    chr_update_matrix( this, true );

    return true;
}

const std::shared_ptr<Object>& Object::getLeftHandItem() const
{
    return _currentModule->getObjectHandler()[holdingwhich[SLOT_LEFT]];
}

const std::shared_ptr<Object>& Object::getRightHandItem() const
{
    return _currentModule->getObjectHandler()[holdingwhich[SLOT_RIGHT]];
}

bool Object::canSeeObject(const std::shared_ptr<Object> &target) const
{
    /// @note ZF@> Invictus characters can always see through darkness (spells, items, quest handlers, etc.)
    // Scenery, spells and quest objects can always see through darkness
    // Checking Object invictus is not enough, since that could be temporary
    // and not indicate the appropriate objects so we check the profile instead
    if (getProfile()->isInvincible()) {
        return true;
    }

    //Too Dark?
    int enviro_light = ( target->inst.alpha * target->inst.max_light ) * INV_FF<float>();
    int self_light   = ( target->inst.light == 255 ) ? 0 : target->inst.light;
    int light        = std::max(enviro_light, self_light);
    light *= expf(0.32f * getAttribute(Ego::Attribute::DARKVISION));
    if(light < INVISIBLE) {
        return false;
    }

    //Is target stealthed?
    if(!canSeeInvisible() && target->isStealthed()) {
        return false;
    }

    //Too invisible?
    int alpha = target->inst.alpha;
    if (canSeeInvisible()) {
        alpha = get_alpha(alpha, expf(0.32f * getAttribute(Ego::Attribute::SEE_INVISIBLE)));
    }
    alpha = Ego::Math::constrain(alpha, 0, 255);
    if(alpha < INVISIBLE) {
        return false;
    }

    return true;
}

void Object::setFat(const float fat)
{
    this->fat = fat;
    recalculateCollisionSize();
}

void Object::setBumpHeight(const float height)
{
    bump_save.height = std::max(height, 0.0f);
    recalculateCollisionSize();
}

void Object::setBumpWidth(const float width)
{
    float ratio = 1.0f;
    if(bump_stt.size > 0) {
        std::abs(width / bump_stt.size);
    }

    shadow_size_save = shadow_size_stt * ratio;
    bump_save.size = bump_stt.size * ratio;
    bump_save.size_big = bump_stt.size_big * ratio;

    recalculateCollisionSize();
}

void Object::recalculateCollisionSize()
{
    shadow_size   = shadow_size_save   * fat;
    bump.size     = bump_save.size     * fat;
    bump.size_big = bump_save.size_big * fat;
    bump.height   = bump_save.height   * fat;

    getObjectPhysics().updateCollisionSize(true);
}

void Object::checkLevelUp()
{
    // Do level ups and stat changes
    uint8_t curlevel = experiencelevel + 1;
    if ( curlevel < MAXLEVEL )
    {
        uint32_t xpcurrent = experience;
        uint32_t xpneeded  = getProfile()->getXPNeededForLevel(curlevel);

        if ( xpcurrent >= xpneeded )
        {
            // The character is ready to advance...
            if(isPlayer()) {
                const std::shared_ptr<Ego::Player> &player = _currentModule->getPlayer(is_which_player);
                if(!player->hasUnspentLevel()) {
                    player->setLevelUpIndicator(true);
                    DisplayMsg_printf("%s gained a level!!!", getName().c_str());
                    AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_LEVELUP));
                }
                return;
            }

            //Automatic level up for AI characters
            experiencelevel++;
            SET_BIT(ai.alert, ALERTIF_LEVELUP);

            // Size Increase
            fat_goto += getProfile()->getSizeGainPerMight() * 0.25f;  // Limit this?
            fat_goto_time += SIZETIME;

            //Primary Attribute increase
            for(size_t i = 0; i < Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES; ++i) {
                _baseAttribute[i] += Random::next(getProfile()->getAttributeGain(static_cast<Ego::Attribute::AttributeType>(i)));
            }

            //Grab random Perk? (ZF> just uncomment if we want to do this for AI characters as well)
            //std::vector<Ego::Perks::PerkID> perkPool = getValidPerks();
            //if(!perkPool.empty()) {
            //    addPerk(Random::getRandomElement(perkPool));
            //}
            //else {
            //    addPerk(Ego::Perks::TOUGHNESS); //Add TOUGHNESS as default perk if none other are available
            //}
        }
    }
}

void Object::kill(const std::shared_ptr<Object> &originalKiller, bool ignoreInvincibility)
{
    //No need to continue is there?
    if (!isAlive() || (isInvincible() && !ignoreInvincibility)) return;

    //Too silly to Die perk?
    if(hasPerk(Ego::Perks::TOO_SILLY_TO_DIE) && !ignoreInvincibility)
    {
        //1% per character level to simply not die
        if(Random::getPercent() <= getExperienceLevel())
        {
            //Refill to full Life instead!
            _currentLife = getAttribute(Ego::Attribute::MAX_LIFE);
            BillboardSystem::get().makeBillboard(getObjRef(), "Too Silly to Die", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::white(), 3, Billboard::Flags::All);
            DisplayMsg_printf("%s decided not to die after all!", getName(false, true, true).c_str());
            AudioSystem::get().playSound(getPosition(), AudioSystem::get().getGlobalSound(GSND_DRUMS));
            return;
        }
    }

    //Guardian Angel perk?
    if(hasPerk(Ego::Perks::GUARDIAN_ANGEL) && !ignoreInvincibility)
    {
        //1% per character level to be rescued by your guardian angel
        if(Random::getPercent() <= getExperienceLevel())
        {
            //Refill to full Life instead!
            _currentLife = getAttribute(Ego::Attribute::MAX_LIFE);
            BillboardSystem::get().makeBillboard(getObjRef(), "Guardian Angel", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::white(), 3, Billboard::Flags::All);
            DisplayMsg_printf("%s was saved by a Guardian Angel!", getName(false, true, true).c_str());
            AudioSystem::get().playSound(getPosition(), AudioSystem::get().getGlobalSound(GSND_ANGEL_CHOIR));
            return;
        }
    }

    //Fix who is actually the killer if needed
    std::shared_ptr<Object> actualKiller = originalKiller;
    if (actualKiller)
    {
        //If we are a held item, try to figure out who the actual killer is
        if ( actualKiller->isBeingHeld() && !_currentModule->getObjectHandler().get(actualKiller->attachedto)->isMount() )
        {
            actualKiller = _currentModule->getObjectHandler()[actualKiller->attachedto];
        }

        //If the killer is a mount, try to award the kill to the rider
        else if (actualKiller->isMount() && actualKiller->getLeftHandItem())
        {
            actualKiller = actualKiller->getLeftHandItem();
        }
    }

    _isAlive = false;

    _currentLife    = -1.0f;
    platform        = true;
    canuseplatforms = true;
    phys.bumpdampen = phys.bumpdampen * 0.5f;
    setBumpWidth(bump_stt.size * 0.5f);

    //End stealth if we were hidden
    deactivateStealth();

    // Play the death animation
    int action = Random::next((int)ACTION_KA, ACTION_KA + 3);
    chr_play_action(this, action, false);
    chr_instance_t::set_action_keep(inst, true);

    // Give kill experience
    uint16_t experience = getProfile()->getExperienceValue() + (this->experience * getProfile()->getExperienceExchangeRate());

    // distribute experience to the attacker
    if (actualKiller)
    {
        // Set target
        ai.setTarget(actualKiller->getObjRef());
		if (actualKiller->getTeam() == Team::TEAM_DAMAGE || actualKiller->getTeam() == Team::TEAM_NULL) {
			ai.setTarget(getObjRef());
		}

        // Award experience for kill?
        if ( actualKiller->getTeam().hatesTeam(getTeam()) )
        {
            //Check for special hatred
            if ( actualKiller->getProfile()->getIDSZ(IDSZ_HATE) == getProfile()->getIDSZ(IDSZ_PARENT) ||
                 actualKiller->getProfile()->getIDSZ(IDSZ_HATE) == getProfile()->getIDSZ(IDSZ_TYPE) )
            {
                actualKiller->giveExperience(experience, XP_KILLHATED, false);
            }

            // Nope, award direct kill experience instead
            else actualKiller->giveExperience(experience, XP_KILLENEMY, false);

            //Mercenary Perk gives +1 Zenny per kill
            if(actualKiller->hasPerk(Ego::Perks::MERCENARY) && !_hasBeenKilled && actualKiller->money < MAXMONEY) {
                actualKiller->money += 1;
                AudioSystem::get().playSound(getPosition(), AudioSystem::get().getGlobalSound(GSND_COINGET));
            }
        
            //Crusader Perk regains 1 mana per Undead kill
            if(actualKiller->hasPerk(Ego::Perks::CRUSADER) && getProfile()->getIDSZ(IDSZ_PARENT).equals('U','N','D','E')) {
                actualKiller->costMana(-1, actualKiller->getObjRef());
                BillboardSystem::get().makeBillboard(actualKiller->getObjRef(), "Crusader", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::yellow(), 3, Billboard::Flags::All);
            }
        }
    }

    //Set various alerts to let others know it has died
    //and distribute experience to whoever needs it
    SET_BIT(ai.alert, ALERTIF_KILLED);

    for(const std::shared_ptr<Object> &listener : _currentModule->getObjectHandler().iterator())
    {
        if (!listener->isAlive()) continue;

        // All allies get team experience, but only if they also hate the dead guy's team
        if (actualKiller && listener != actualKiller && !listener->getTeam().hatesTeam(actualKiller->getTeam()) && listener->getTeam().hatesTeam(getTeam()) )
        {
            listener->giveExperience(experience, XP_TEAMKILL, false);
        }

        // Check if we were a leader
        if ( getTeam().getLeader().get() == this && listener->getTeam() == getTeam() )
        {
            // All folks on the leaders team get the alert
            SET_BIT( listener->ai.alert, ALERTIF_LEADERKILLED );
        }

        // Let the other characters know it died
        if ( listener->ai.getTarget() == getObjRef() )
        {
            SET_BIT( listener->ai.alert, ALERTIF_TARGETKILLED );
        }
    }

    // Detach the character from the game
	removeFromGame(this);

    // If it's a player, let it die properly before enabling respawn
    if (isPlayer())  {
        local_stats.revivetimer = ONESECOND; // 1 second
    }

    // Let it's AI script run one last time
    _hasBeenKilled = true;
    ai.timer = update_wld + 1;            // Prevent IfTimeOut in scr_run_chr_script()
    scr_run_chr_script(this);
}

void Object::resetAlpha()
{
    // Make sure the character is mounted
    const std::shared_ptr<Object> &mount = _currentModule->getObjectHandler()[attachedto];
    if(!mount) {
        return;
    }

    if (isItem() && mount->getProfile()->transferBlending())
    {
        setAlpha(getProfile()->getAlpha());
        setLight(getProfile()->getLight());
    }
}

void Object::giveExperience(const int amount, const XPType xptype, const bool overrideInvincibility)
{
    //No xp to give
    if (0 == amount) return;

    if (!isInvincible() || overrideInvincibility)
    {
        // Figure out how much experience to give
        float newamount = amount;
        if ( xptype < XP_COUNT )
        {
            newamount = amount * getProfile()->getExperienceRate(xptype);
        }

        // Intellect affects xp gained (1% per intellect above 10, -1% per intellect below 10)
        float intadd = (getAttribute(Ego::Attribute::INTELLECT) - 10.0f) / 100.0f;
        newamount *= 1.00f + intadd;

        // Apply XP bonus/penality depending on game difficulty
        if (egoboo_config_t::get().game_difficulty.getValue() >= Ego::GameDifficulty::Hard)
        {
            newamount *= 1.20f; // 20% extra on hard
        }
        else if (egoboo_config_t::get().game_difficulty.getValue() >= Ego::GameDifficulty::Normal)
        {
            newamount *= 1.10f; // 10% extra on normal
        }


        //Fast Learner Perk gives +20% XP gain
        if(hasPerk(Ego::Perks::FAST_LEARNER)) {
            newamount *= 1.20f;
        }

        //Bookworm Perk gives +10% XP gain
        if(hasPerk(Ego::Perks::BOOKWORM)) {
            newamount *= 1.10f;
        }

        experience += newamount;
    }
}

int Object::getPrice() const
{
    /// @author BB
    /// @details determine the correct price for an item

    uint16_t  iskin;
    float   price;

    // Make sure spell books are priced according to their spell and not the book itself
    PRO_REF slotNumber = INVALID_PRO_REF;
    if (_profileID == SPELLBOOK)
    {
        slotNumber = basemodel_ref;
        iskin = 0;
    }
    else
    {
        slotNumber  = _profileID;
        iskin = skin;
    }

    std::shared_ptr<ObjectProfile> profile = ProfileSystem::get().getProfile(slotNumber);
    if(!profile) {
        return 0;
    }

    price = profile->getSkinInfo(iskin).cost;

    // Items spawned in shops are more valuable
    if ( !isshopitem ) price *= 0.5f;

    // base the cost on the number of items/charges
    if ( profile->isStackable() )
    {
        price *= ammo;
    }
    else if ( profile->isRangedWeapon() && ammo <ammomax )
    {
        if ( 0 != ammomax )
        {
            price *= static_cast<float>(ammo) / static_cast<float>(ammomax);
        }
    }
    
    return static_cast<int>(price);
}

bool Object::isBeingHeld() const
{
    //If we are inside an inventory then we are being "held"
    if(isInsideInventory()) {
        return true;
    }

    //Check if holder exists and not marked for removal
    const std::shared_ptr<Object> &holder = getHolder();
    if(holder && !holder->isTerminated()) {
        return true;
    }

    return false;
}

bool Object::isInsideInventory() const
{
    //Check if inventory exists and not marked for removal
    const std::shared_ptr<Object> &holder = _currentModule->getObjectHandler()[inwhich_inventory];
    if(!holder || holder->isTerminated()) {
        return false;
    }

    return true;
}

void Object::removeFromGame(Object *obj)
{
	auto objRef = obj->getObjRef();

	obj->sparkle = NOSPARKLE;

	// Remove it from the team
	obj->team = obj->team_base;
	_currentModule->getTeamList()[obj->team].decreaseMorale();

	if (_currentModule->getTeamList()[obj->team].getLeader().get() == obj)
	{
		// The team now has no leader if the character is the leader
		_currentModule->getTeamList()[obj->team].setLeader(Object::INVALID_OBJECT);
	}

	// Clear all shop passages that it owned..
	_currentModule->removeShopOwner(objRef);

	// detach from any mount
	if (_currentModule->getObjectHandler().exists(obj->attachedto))
	{
		obj->detatchFromHolder(true, false);
	}

	// drop your left item
	const std::shared_ptr<Object> &leftItem = obj->getLeftHandItem();
	if (leftItem && leftItem->isItem()) {
		leftItem->detatchFromHolder(true, false);
	}

	// drop your right item
	const std::shared_ptr<Object> &rightItem = obj->getRightHandItem();
	if (rightItem && rightItem->isItem()) {
		rightItem->detatchFromHolder(true, false);
	}

	// Stop all sound loops for this object
	AudioSystem::get().stopObjectLoopingSounds(objRef);
}

BIT_FIELD Object::hit_wall(const Vector3f& pos, Vector2f& nrm, float *pressure)
{
	if (CHR_INFINITE_WEIGHT == phys.weight)
	{
		return EMPTY_BIT_FIELD;
	}

	// Calculate the radius based on whether the character is on camera.
	float radius = 0.0f;
	if (CameraSystem::get() && CameraSystem::get()->getMainCamera()->getTileList()->inRenderList(getTile()))
	{
		radius = bump_1.size;
	}

	g_meshStats.mpdfxTests = 0;
	g_meshStats.boundTests = 0;
	g_meshStats.pressureTests = 0;
	BIT_FIELD result = _currentModule->getMeshPointer()->hit_wall(pos, radius, stoppedby, nrm, pressure);
	chr_stoppedby_tests += g_meshStats.mpdfxTests;
	chr_pressure_tests += g_meshStats.pressureTests;

	return result;
}

BIT_FIELD Object::hit_wall(const Vector3f& pos, Vector2f& nrm, float * pressure, mesh_wall_data_t& data)
{
	if (CHR_INFINITE_WEIGHT == phys.weight)
	{
		return EMPTY_BIT_FIELD;
	}

    // Calculate the radius based on whether the character is on camera.
	float radius = 0.0f;
	if (CameraSystem::get() && CameraSystem::get()->getMainCamera()->getTileList()->inRenderList(getTile()))
	{
		radius = bump_1.size;
	}

	g_meshStats.mpdfxTests = 0;
	g_meshStats.boundTests = 0;
	g_meshStats.pressureTests = 0;
	BIT_FIELD result = _currentModule->getMeshPointer()->hit_wall(pos, radius, stoppedby, nrm, pressure, data);
	chr_stoppedby_tests += g_meshStats.mpdfxTests;
	chr_pressure_tests += g_meshStats.pressureTests;

	return result;
}

BIT_FIELD Object::test_wall(const Vector3f& pos)
{
	if (isTerminated()) {
		return EMPTY_BIT_FIELD;
	}
	if (CHR_INFINITE_WEIGHT == phys.weight)
	{
		return EMPTY_BIT_FIELD;
	}

	// Calculate the radius based on whether the character is on camera.
	float radius = 0.0f;
#if 0
	if (egoboo_config_t::get().debug_developerMode_enable.getValue() && !SDL_KEYDOWN(keyb, SDLK_F8))
	{
		if (CameraSystem::get() && CameraSystem::get()->getMainCamera()->getTileList()->inRenderList(getTile()))
		{
			radius = bump_1.size;
		}
	}
#endif

	// Do the wall test.
	g_meshStats.mpdfxTests = 0;
	g_meshStats.boundTests = 0;
	g_meshStats.pressureTests = 0;

	BIT_FIELD result = _currentModule->getMeshPointer()->test_wall(pos, radius, stoppedby);
	chr_stoppedby_tests += g_meshStats.mpdfxTests;
	chr_pressure_tests += g_meshStats.pressureTests;

	return result;
}

bool Object::costMana(int amount, const ObjectRef killer)
{
    const std::shared_ptr<Object> &pkiller = _currentModule->getObjectHandler()[killer];

    bool manaPaid  = false;
    int manaFinal = FLOAT_TO_FP8(getMana()) - amount;

    if (manaFinal < 0)
    {
        int manaDebt = -manaFinal;
        _currentMana = 0.0f;

        if ( getAttribute(Ego::Attribute::CHANNEL_LIFE) > 0 )
        {
            _currentLife -= FP8_TO_FLOAT(manaDebt);

            if (_currentLife <= 0 && egoboo_config_t::get().game_difficulty.getValue() >= Ego::GameDifficulty::Hard)
            {
                kill(pkiller != nullptr ? pkiller : _currentModule->getObjectHandler()[this->getObjRef()], false);
            }

            manaPaid = true;
        }
    }
    else
    {
        int mana_surplus = 0;

        _currentMana = FP8_TO_FLOAT(manaFinal);

        if ( manaFinal > FLOAT_TO_FP8(getMaxMana()) )
        {
            mana_surplus = manaFinal - FLOAT_TO_FP8(getMaxMana());
            _currentMana = getMaxMana();
        }

        // allow surplus mana to go to health if you can channel?
        if ( getAttribute(Ego::Attribute::CHANNEL_LIFE) > 0 && mana_surplus > 0 )
        {
            // use some factor, divide by 2
            heal(pkiller, mana_surplus / 2, true);
        }

        manaPaid = true;
    }

    return manaPaid;
}

void Object::respawn()
{
    //already alive?
    if(isAlive()) {
        return;
    }

    const std::shared_ptr<ObjectProfile> &profile = getProfile();

    ParticleHandler::get().spawnPoof(this->toSharedPointer());
    disaffirm_attached_particles(getObjRef());

    _isAlive = true;
    bore_timer = BORETIME;
    careful_timer = CAREFULTIME;
    _currentLife = getAttribute(Ego::Attribute::MAX_LIFE);
    _currentMana = getAttribute(Ego::Attribute::MAX_MANA);
    setPosition(getSpawnPosition());
    vel = Vector3f::zero();
    team = team_base;
    canbecrushed = false;
    ori.map_twist_facing_y = MAP_TURN_OFFSET;  // These two mean on level surface
    ori.map_twist_facing_x = MAP_TURN_OFFSET;
    if ( !getTeam().getLeader() )  getTeam().setLeader( _currentModule->getObjectHandler()[getObjRef()] );
    if ( !isInvincible() )         getTeam().increaseMorale();

    // start the character out in the "dance" animation
    chr_start_anim(this, ACTION_DA, true, true);

    // reset all of the bump size information
    {
        fat_stt           = profile->getSize();
        shadow_size_stt   = profile->getShadowSize();
        bump_stt.size     = profile->getBumpSize();
        bump_stt.size_big = profile->getBumpSizeBig();
        bump_stt.height   = profile->getBumpHeight();

        shadow_size_save   = shadow_size_stt;
        bump_save.size     = bump_stt.size;
        bump_save.size_big = bump_stt.size_big;
        bump_save.height   = bump_stt.height;

        recalculateCollisionSize();
    }

    platform        = profile->isPlatform();
    canuseplatforms = profile->canUsePlatforms();
    _baseAttribute[Ego::Attribute::FLY_TO_HEIGHT] = profile->getFlyHeight();
    phys.bumpdampen = profile->getBumpDampen();

    ai.alert = ALERTIF_CLEANEDUP;
    ai.setTarget(getObjRef());
    ai.timer  = 0;

    grog_timer = 0;
    daze_timer = 0;

    // Let worn items come back
    for(const std::shared_ptr<Object> pitem : _inventory.iterate())
    {
        if ( pitem->isequipped )
        {
            pitem->isequipped = false;
            SET_BIT( ai.alert, ALERTIF_PUTAWAY ); // same as ALERTIF_ATLASTWAYPOINT
        }
    }

    // re-initialize the instance
    chr_instance_t::spawn(inst, _profileID, skin);
    chr_update_matrix( this, true );

    if ( !isHidden() )
    {
        reaffirm_attached_particles(getObjRef());
    }

    chr_instance_t::update_ref(inst, _currentModule->getMeshPointer()->getElevation(Vector2f(getPosX(), getPosY()), false), true );
}

float Object::getRawDamageResistance(const DamageType type, const bool includeArmor) const
{
    if(type >= DAMAGE_COUNT) {
        return 0.0f;
    }

    float resistance = getAttribute(Ego::Attribute::resistFromDamageType(type));

    //Stalwart perk increases CRUSH, SLASH and POKE by 1
    if(type == DAMAGE_CRUSH || type == DAMAGE_POKE || type == DAMAGE_SLASH) {
        if(hasPerk(Ego::Perks::STALWART)) {
            resistance += 1.0f;
        }
    }

    //Elemental Resistance perk increases FIRE, ICE and ZAP by 1
    if(type == DAMAGE_FIRE || type == DAMAGE_ICE || type == DAMAGE_ZAP) {
        if(hasPerk(Ego::Perks::ELEMENTAL_RESISTANCE)) {
            resistance += 1.0f;
        }
    
        //Ward perks increases it by further 3
        if(type == DAMAGE_FIRE && hasPerk(Ego::Perks::FIRE_WARD)) {
            resistance += 3.0f;
        }
        else if(type == DAMAGE_ZAP && hasPerk(Ego::Perks::ZAP_WARD)) {
            resistance += 3.0f;
        }
        else if(type == DAMAGE_ICE && hasPerk(Ego::Perks::ICE_WARD)) {
            resistance += 3.0f;
        }
    }

    //Rosemary perk gives +4
    if(type == DAMAGE_EVIL && hasPerk(Ego::Perks::ROSEMARY)) {
        resistance += 4.0f;
    }

    //Pyromaniac and Troll Blood perks *reduces* FIRE resistance by 10 each
    if(type == DAMAGE_FIRE) {
        if(hasPerk(Ego::Perks::PYROMANIAC)) {
            resistance -= 10.0f;
        }
        if(hasPerk(Ego::Perks::TROLL_BLOOD)) {
            resistance -= 10.0f;
        }
    }

    //Negative armor means it's a weakness
    if(resistance < 0.0f) {

        //Defence reduces weakness, but cannot eliminate it completely (50% weakness reduction at 255 defence)
        if(includeArmor) {
            resistance *= 1.0f - (getAttribute(Ego::Attribute::DEFENCE) / 512.0f);
        }
        return resistance;
    }

    //Defence bonus increases all damage type resistances (every 14 points gives +1.0 resistance)
    //This means at 255 defence and 0% resistance results in 52% damage reduction
    if(includeArmor && HAS_NO_BITS( static_cast<int>(getAttribute(Ego::Attribute::modifierFromDamageType(type))), DAMAGEINVERT)) {
        resistance += getAttribute(Ego::Attribute::DEFENCE) / 14.0f;
    }

    return resistance;
}

float Object::getDamageReduction(const DamageType type, const bool includeArmor) const
{
    //DAMAGE_COUNT simply means not affected by damage resistances
    if(type >= DAMAGE_COUNT) {
        return 0.0f;
    }

    //Immunity to damage type?
    if( HAS_SOME_BITS(static_cast<int>(getAttribute(Ego::Attribute::modifierFromDamageType(type))), DAMAGEINVICTUS) ) {
        return 1.0f;
    }

    const float resistance = getRawDamageResistance(type, includeArmor);

    //Negative resistance *increases* damage a lot
    if(resistance < 0.0f) {
        return 1.0f - std::pow(0.94f, resistance);
    }

    //Positive resistance reduces damage, but never 100%
    return ((resistance*0.06f) / (1.0f + resistance*0.06f));
}

float Object::getBaseAttribute(const Ego::Attribute::AttributeType type) const
{
    EGOBOO_ASSERT(type < _baseAttribute.size() && type != Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES);
    return _baseAttribute[type];
}

void Object::setBaseAttribute(const Ego::Attribute::AttributeType type, float value)
{
    EGOBOO_ASSERT(type < _baseAttribute.size() && type != Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES);
    _baseAttribute[type] = value;
}

float Object::getAttribute(const Ego::Attribute::AttributeType type) const 
{ 
    EGOBOO_ASSERT(type < _baseAttribute.size() && type != Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES);

    float attributeValue = _baseAttribute[type];

    //Try to find temp value in map, but don't create it if it doesn't already exist
    const auto& result = _tempAttribute.find(type);
    if(result != _tempAttribute.end()) {

        //Is this a SET type attribute or a cumulative ADD type attribute?
        if(isOverrideSetAttribute(type)) {
            return (*result).second;
        }
        else {
            //Total value is base plus temp bonuses from enchants
            attributeValue += (*result).second;
        }
    }

    switch(type) {

        //Wolverine perk gives +0.25 Life Regeneration while holding a Claw weapon
        case Ego::Attribute::LIFE_REGEN:
            if(hasPerk(Ego::Perks::WOLVERINE)) {
                if( (getLeftHandItem() && getLeftHandItem()->getProfile()->getIDSZ(IDSZ_PARENT).equals('C','L','A','W'))
                 || (getRightHandItem() && getRightHandItem()->getProfile()->getIDSZ(IDSZ_PARENT).equals('C','L','A','W')))
                 {
                    attributeValue += 0.25f;
                 }                
            }
        break;

        case Ego::Attribute::JUMP_POWER:
            //Special value for flying Objects
            if(getAttribute(Ego::Attribute::FLY_TO_HEIGHT) > 0.0f) {
                return JUMPINFINITE;
            }

            //Athletics Perks gives +25% jump power
            if(hasPerk(Ego::Perks::ATHLETICS)) {
                attributeValue *= 1.25f;
            }
        break;

        //Limit lowest acceleration to zero
        case Ego::Attribute::ACCELERATION:
        {
            if(attributeValue < 0.0f) return 0.0f;
        }
        break;

        //Limit lowest base attribute to 1
        case Ego::Attribute::MIGHT:
        case Ego::Attribute::AGILITY:
        case Ego::Attribute::INTELLECT:
        {
            if(attributeValue < 1.0f) return 1.0f;
        }
        break;

        default:
            //nothing, keep default case to quench GCC warnings
        break;
    }

    return attributeValue; 
}

void Object::increaseBaseAttribute(const Ego::Attribute::AttributeType type, float value)
{
    EGOBOO_ASSERT(type < _baseAttribute.size() && type != Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES);
    _baseAttribute[type] = Ego::Math::constrain(_baseAttribute[type] + value, 0.0f, 255.0f);

    //Handle current life and mana increase as well
    if(type == Ego::Attribute::MAX_LIFE) {
        _currentLife += value;
    }
    else if(type == Ego::Attribute::MAX_MANA) {
        _currentMana += value;
    }
}

Inventory& Object::getInventory()
{
    return _inventory;
}

bool Object::hasPerk(Ego::Perks::PerkID perk) const
{
    if(perk == Ego::Perks::NR_OF_PERKS) return true;

    //@note ZF> We also have to check our profile in case we are polymorphed and gain new
    //          skills from our new form (e.g Lumpkin form allows gunplay)
    return _perks[perk] || getProfile()->beginsWithPerk(perk);
}

std::vector<Ego::Perks::PerkID> Object::getValidPerks() const
{
    //Build list of perks we can learn
    std::vector<Ego::Perks::PerkID> result;
    for(size_t i = 0; i < Ego::Perks::NR_OF_PERKS; ++i)
    {
        const Ego::Perks::PerkID id = static_cast<Ego::Perks::PerkID>(i);

        //Can we learn this perk?
        if(!getProfile()->canLearnPerk(id)) {
            continue;
        }

        //Cannot learn the same perk twice
        if(hasPerk(id)) {
            continue;
        }

        //Do we fulfill the requirements for this perk?
        const Ego::Perks::Perk& perk = Ego::Perks::PerkHandler::get().getPerk(id);
        if(perk.getRequirement() == Ego::Perks::NR_OF_PERKS || hasPerk(perk.getRequirement())) {
            result.push_back(id);
        }
    }

    return result;
}

void Object::addPerk(Ego::Perks::PerkID perk)
{
    if(perk == Ego::Perks::NR_OF_PERKS) return;
    _perks[perk] = true;
}

float Object::getLife() const
{
    return _currentLife;
}

float Object::getMana() const
{
    return _currentMana;
}

std::shared_ptr<Ego::Enchantment> Object::addEnchant(ENC_REF enchantProfile, PRO_REF spawnerProfile, const std::shared_ptr<Object>& owner, const std::shared_ptr<Object> &spawner)
{
    if (enchantProfile >= ENCHANTPROFILES_MAX) {
		Log::get().warn("%s:%d:%s: cannot add enchant with invalid enchant profile %d\n", __FILE__, __LINE__, __FUNCTION__, enchantProfile);
        return nullptr;
    }    
    const std::shared_ptr<EnchantProfile> &enchantmentProfile = EnchantProfileSystem.get_ptr(enchantProfile);
    
    if(!ProfileSystem::get().isValidProfileID(spawnerProfile)) {
		Log::get().warn("%s:%d:%s: cannot add enchant with invalid spawner profile %d\n", __FILE__, __LINE__, __FUNCTION__, spawnerProfile);
        return nullptr;
    }

    std::shared_ptr<Ego::Enchantment> enchant = std::make_shared<Ego::Enchantment>(enchantmentProfile, spawnerProfile, owner);
    enchant->applyEnchantment(this->toSharedPointer());

    //Succeeded to apply the enchantment to the target?
    if(!enchant->isTerminated() && spawner) {
        spawner->_lastEnchantSpawned = enchant;
        return enchant;
    }

    return nullptr;
}

void Object::removeEnchantsWithIDSZ(const IDSZ2& idsz)
{
    //Nothing to do?
    if(idsz == IDSZ2::None) return;

    //Remove all active enchants that have the corresponding IDSZ
    for(const std::shared_ptr<Ego::Enchantment> &enchant : _activeEnchants)
    {
        if(enchant->isTerminated()) continue;
        if(idsz == enchant->getProfile()->removedByIDSZ) {
            enchant->requestTerminate();
        }
    }
}

std::forward_list<std::shared_ptr<Ego::Enchantment>>& Object::getActiveEnchants()
{
    return _activeEnchants;
}

bool Object::disenchant()
{
    bool oneRemoved = false;

    for(const std::shared_ptr<Ego::Enchantment> &enchant : _activeEnchants) {
        if(enchant->isTerminated()) continue;
        enchant->requestTerminate();
        oneRemoved = true;
    }

    return oneRemoved;
}

std::unordered_map<Ego::Attribute::AttributeType, float, std::hash<uint8_t>>& Object::getTempAttributes()
{
    return _tempAttribute;
}

bool Object::isFlying() const
{
    return getAttribute(Ego::Attribute::FLY_TO_HEIGHT) > 0.0f;
}

std::shared_ptr<Ego::Enchantment> Object::getLastEnchantmentSpawned() const
{
    return _lastEnchantSpawned.lock();
}


const std::shared_ptr<Object>& Object::toSharedPointer() const 
{ 
    return _currentModule->getObjectHandler()[getObjRef()]; 
}

void Object::setMana(const float value)
{
    _currentMana = Ego::Math::constrain(_currentMana+value, 0.00f, getAttribute(Ego::Attribute::MAX_MANA));
}

void Object::setLife(const float value)
{
    _currentLife = Ego::Math::constrain(_currentLife+value, 0.01f, getAttribute(Ego::Attribute::MAX_LIFE));
}

void Object::setName(const std::string &name)
{
    _name = name;
}

const std::shared_ptr<ObjectProfile>& Object::getProfile() const 
{
    return _profile;
}

void Object::polymorphObject(const PRO_REF profileID, const SKIN_T newSkin)
{
    if(!ProfileSystem::get().isValidProfileID(profileID)) {
		Log::get().warn("Tried to polymorph object (%s) into an invalid profile ID: %d\n", getProfile()->getClassName().c_str(), profileID);
        return;
    }

    _profileID = profileID;
    _profile = ProfileSystem::get().getProfile(_profileID);

    //Exit stealth if we change form
    deactivateStealth();

    //Get any items we are holding
    const std::shared_ptr<Object> &leftItem = getLeftHandItem();
    const std::shared_ptr<Object> &rightItem = getRightHandItem();

    // Drop left weapon if we have no left grip
    if ( leftItem && ( !_profile->isSlotValid(SLOT_LEFT) || _profile->isMount() ) )
    {
        leftItem->detatchFromHolder(true, true);
        leftItem->getObjectPhysics().detachFromPlatform();

        if ( isMount() )
        {
            leftItem->vel[kZ]    = DISMOUNTZVEL;
            leftItem->jump_timer = JUMPDELAY;
            leftItem->movePosition(0.0f, 0.0f, DISMOUNTZVEL);
        }
    }

    // Drop right weapon if we have no right grip
    if ( rightItem && !_profile->isSlotValid(SLOT_RIGHT) )
    {
        rightItem->detatchFromHolder(true, true);
        rightItem->getObjectPhysics().detachFromPlatform();

        if ( isMount() )
        {
            rightItem->vel[kZ]    = DISMOUNTZVEL;
            rightItem->jump_timer = JUMPDELAY;
            rightItem->movePosition(0.0f, 0.0f, DISMOUNTZVEL);
        }
    }

    // Stuff that must be set
    stoppedby = _profile->getStoppedByMask();

    // Ammo
    ammomax = _profile->getMaxAmmo();
    ammo    = _profile->getAmmo();

    // Gender
    if(_profile->getGender() != GENDER_RANDOM)  // GENDER_RANDOM means keep old gender
    {
        gender = _profile->getGender();
    }

    // AI stuff
    ai.state = 0;
    ai.timer          = 0;
    turnmode          = TURNMODE_VELOCITY;
    latch.clear();

    // Flags
    platform        = _profile->isPlatform();
    canuseplatforms = _profile->canUsePlatforms();
    isitem          = _profile->isItem();
    invictus        = _profile->isInvincible();
    jump_timer      = JUMPDELAY;
    reaffirm_damagetype = _profile->getReaffirmDamageType();

    //Physics
    phys.bumpdampen = _profile->getBumpDampen();

    if (CAP_INFINITE_WEIGHT == _profile->getWeight())
    {
        phys.weight = CHR_INFINITE_WEIGHT;
    }
    else
    {
        phys.weight = std::min<uint32_t>(_profile->getWeight() * fat * fat * fat, CHR_MAX_WEIGHT);
    }

    /// @note BB@> changing this could be disasterous, in case you can't un-morph youself???
    /// @note ZF@> No, we want this, I have specifically scripted morph books to handle unmorphing
    /// even if you cannot cast arcane spells. Some morph spells specifically morph the player
    /// into a fighter or a tech user, but as a balancing factor prevents other spellcasting.
    // pchr->canusearcane          = pcap_new->canusearcane;

    // Character size and bumping
    // set the character size so that the new model is the same size as the old model
    // the model will then morph its size to the correct size over time
    {
        float oldFat = fat;
        float newFat;

        if ( 0.0f == bump.size ) {
            newFat = _profile->getSize();
        }
        else {
            newFat = ( _profile->getBumpSize() * _profile->getSize() ) / bump.size;
        }

        // Spellbooks should stay the same size, even if their spell effect cause changes in size
        if (getProfileID() == SPELLBOOK) newFat = oldFat = 1.00f;

        // copy all the cap size info over, as normal
        fat_stt           = _profile->getSize();
        shadow_size_stt   = _profile->getShadowSize();
        bump_stt.size     = _profile->getBumpSize();
        bump_stt.size_big = _profile->getBumpSizeBig();
        bump_stt.height   = _profile->getBumpHeight();

        //Initialize model size and collision box
        fat                = fat_stt;
        shadow_size_save   = shadow_size_stt;
        bump_save.size     = bump_stt.size;
        bump_save.size_big = bump_stt.size_big;
        bump_save.height   = bump_stt.height;
        recalculateCollisionSize();

        // make the model's size congruent
        if (0.0f != newFat && newFat != oldFat)
        {
            setFat(newFat);
            fat_goto      = oldFat;
            fat_goto_time = SIZETIME;
        }
        else
        {
            setFat(oldFat);
            fat_goto      = oldFat;
            fat_goto_time = 0;
        }
    }

    //Remove attached particles before changing our model
    disaffirm_attached_particles(getObjRef());

    //Actually change the model
    chr_instance_t::spawn(inst, profileID, newSkin);
    chr_update_matrix(this, true);

    // Action stuff that must be down after chr_instance_t::spawn()
    chr_instance_t::set_action_ready(inst, false);
    chr_instance_t::set_action_keep(inst, false);
    chr_instance_t::set_action_loop(inst, false);
    if (isAlive())
    {
        chr_play_action(this, ACTION_DA, false);
    }
    else
    {
        chr_play_action(this, Random::next<int>(ACTION_KA, ACTION_KA + 3), false);
        chr_instance_t::set_action_keep(inst, true);
    }

    // Set the skin after changing the model in chr_instance_t::spawn()
    setSkin(newSkin);

    // Must set the wepon grip AFTER the model is changed in chr_instance_t::spawn()
    if (isBeingHeld())
    {
        set_weapongrip(getObjRef(), attachedto, slot_to_grip_offset(inwhich_slot) );
    }

    if (leftItem)
    {
        EGOBOO_ASSERT(leftItem->attachedto == getObjRef());
        set_weapongrip(leftItem->getObjRef(), getObjRef(), GRIP_LEFT);
    }

    if (rightItem)
    {
        EGOBOO_ASSERT(rightItem->attachedto == getObjRef());
        set_weapongrip(rightItem->getObjRef(), getObjRef(), GRIP_RIGHT);
    }

    /// @note ZF@> disabled so that books dont burn when dropped
    //reaffirm_attached_particles( ichr );

    ai_state_t::set_changed(ai);

    chr_instance_t::update_ref(inst, _currentModule->getMeshPointer()->getElevation(Vector2f(getPosX(), getPosY()), false), true );
}

bool Object::isInvictusDirection(FACING_T direction) const
{
    FACING_T left;
    FACING_T right;

    // if the invictus flag is set, we are invictus
    if (isInvincible()) return true;

    // if the character's frame is invictus, then check the angles
    if (HAS_SOME_BITS(chr_instance_t::get_framefx(inst), MADFX_INVICTUS))
    {
        //I Frame
        direction -= getProfile()->getInvictusFrameFacing();
        left       = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(getProfile()->getInvictusFrameAngle()) );
        right      = getProfile()->getInvictusFrameAngle();

        // If using shield, use the shield invictus instead
        if (ACTION_IS_TYPE(inst.action_which, P))
        {
            bool parry_left = ( inst.action_which < ACTION_PC );

            // Using a shield?
            if (parry_left && getLeftHandItem())
            {
                // Check left hand
                left = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(getLeftHandItem()->getProfile()->getInvictusFrameAngle()) );
                right = getLeftHandItem()->getProfile()->getInvictusFrameAngle();
            }
            else if(getRightHandItem())
            {
                // Check right hand
                left = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(getRightHandItem()->getProfile()->getInvictusFrameAngle()) );
                right = getRightHandItem()->getProfile()->getInvictusFrameAngle();
            }
        }
    }
    else
    {
        // Non invictus Frame
        direction -= getProfile()->getNormalFrameFacing();
        left       = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(getProfile()->getNormalFrameAngle()) );
        right      = getProfile()->getNormalFrameAngle();
    }

    // Check that direction
    if (direction <= left && direction <= right) {
        return true;
    }

    return false;
}

bool Object::isStealthed() const
{
    return _stealth;
}

void Object::deactivateStealth()
{
    //Not in stealth?
    if(!isStealthed()) return;

    //Reset stealth timer
    _stealthTimer = std::max<uint16_t>(_stealthTimer, ONESECOND);
    _stealth = false;

    BillboardSystem::get().makeBillboard(getObjRef(), "Revealed!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::white(), 2, Billboard::Flags::All);
    AudioSystem::get().playSound(getPosition(), AudioSystem::get().getGlobalSound(GSND_STEALTH_END));
    setAlpha(0xFF);
}

bool Object::activateStealth()
{
    //Already in stealth?
    if(isStealthed()) return true;

    //Not allowed to stealth yet?
    if(_stealthTimer > 0) {
        return false;
    }

    //Limit stealth atttempts to once per second
    _stealthTimer = ONESECOND;

    //Do they have the required stealth Perk?
    if(!hasPerk(Ego::Perks::STEALTH)) {
        if(isPlayer()) {
            DisplayMsg_printf("%s does not know how to stealth...", getName().c_str());
        }
        return false;
    }

    //Setup line of sight data
    line_of_sight_info_t lineOfSightInfo;
    lineOfSightInfo.x1 = getPosX();
    lineOfSightInfo.y1 = getPosY();
    lineOfSightInfo.z1 = getPosZ() + std::max(1.0f, bump.height);

    //Check if there are any nearby Objects disrupting our stealth attempt
    std::vector<std::shared_ptr<Object>> nearbyObjects = _currentModule->getObjectHandler().findObjects(getPosX(), getPosY(), WIDE, false);
    for(const std::shared_ptr<Object> &object : nearbyObjects) {
        //Valid objects only
        if(object->isTerminated() || !object->isAlive() || object->isBeingHeld()) continue;

        //Skip scenery objects
        if (object->isScenery()) {
            continue;
        }

        //Ignore objects that are doing the sleep animation
        if(object->inst.action_which == ACTION_MK) {
            continue;
        }

        //Do they consider us an enemy?
        if(!object->getTeam().hatesTeam(getTeam())) {
            continue;
        }

        //Can they see us?
        lineOfSightInfo.x0         = object->getPosX();
        lineOfSightInfo.y0         = object->getPosY();
        lineOfSightInfo.z0         = object->getPosZ() + std::max(1.0f, object->bump.height);
        lineOfSightInfo.stopped_by = object->stoppedby;
        if (line_of_sight_info_t::blocked(lineOfSightInfo, _currentModule->getMeshPointer())) {
            continue;
        }
        
        //Camouflage Perk allows us to hide as long as enemies aren't directly looking at us
        if(hasPerk(Ego::Perks::CAMOUFLAGE) && !object->isFacingLocation(getPosX(), getPosY())) {
            continue;
        }

        //We can't stealth while an enemy is nearby
        if(isPlayer()) {
            BillboardSystem::get().makeBillboard(getObjRef(), "Hide Failed!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::white(), 2, Billboard::Flags::All);
            AudioSystem::get().playSound(getPosition(), AudioSystem::get().getGlobalSound(GSND_STEALTH_END));
        }
        return false;
    }

    //All good, we are now stealthed!
    _stealth = true;
    setAlpha(0);
    BillboardSystem::get().makeBillboard(getObjRef(), "Hidden!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::white(), 2, Billboard::Flags::All);
    AudioSystem::get().playSound(getPosition(), AudioSystem::get().getGlobalSound(GSND_STEALTH));
   
    return true;
}

bool Object::isScenery() const
{
    //If it can be grabbed then it is not really a scenery object
    if(isItem()) {
        return false;
    }
    
    //Can it move?
    if(getBaseAttribute(Ego::Attribute::ACCELERATION) > 0) {
        return false;
    }

    //Everything on team NULL that is not an item is neutral by definition
    if(getTeam() == Team::TEAM_NULL) {
        return true;
    }

    //Objects on other teams than TEAM_NULL can still be scenery, such as destructable tents or idols
    //These are scenery objects if they are either invincible or have infinite weight
    return getProfile()->isInvincible() || getProfile()->getWeight() == CAP_INFINITE_WEIGHT;
}

const std::shared_ptr<Object>& Object::isWieldingItemIDSZ(const IDSZ2& idsz) const
{
    //Check left hand
    const std::shared_ptr<Object> &leftHandItem = getLeftHandItem();
    if(leftHandItem) {
        if(leftHandItem->getProfile()->hasTypeIDSZ(idsz)) {
            return leftHandItem;
        }
    }

    //Check right hand
    const std::shared_ptr<Object> &rightHandItem = getRightHandItem();
    if(rightHandItem) {
        if(rightHandItem->getProfile()->hasTypeIDSZ(idsz)) {
            return rightHandItem;
        }
    }

    //No matching IDSZ
    return INVALID_OBJECT;
}

bool Object::isHidden() const 
{
    if(getProfile()->getHideState() == NOHIDE) return false;
    return getProfile()->getHideState() == ai.state;
}

const std::shared_ptr<Object>& Object::getHolder() const
{
    return _currentModule->getObjectHandler()[attachedto];
}

void Object::setTeam(TEAM_REF team_new, bool permanent)
{
    //No change?
    if(getTeam() == team_new) {
        return;
    }

    // do we count this character as being on a team?
    const bool canHaveTeam = !isItem() && isAlive() && !isInvincible();

    // take the character off of its old team
    if ( VALID_TEAM_RANGE(this->team) )
    {
        // remove the character from the old team
        if ( canHaveTeam )
        {
            getTeam().decreaseMorale();
        }

        //Were we the leader?
        if (this == getTeam().getLeader().get())
        {
            getTeam().setLeader(Object::INVALID_OBJECT);
        }
    }

    // make sure we have a valid value
    if(!VALID_TEAM_RANGE(team_new)) {
        team_new = static_cast<TEAM_REF>(Team::TEAM_NULL);
    }

    // place the character onto its new team
    this->team = team_new;

    // switch the base team only if required
    if (permanent) {
        team_base = this->team;
    }

    // add the character to the new team
    if (canHaveTeam) {
        getTeam().increaseMorale();
    }

    // we are the new leader if there isn't one already
    if (canHaveTeam && !getTeam().getLeader()) {
        getTeam().setLeader(_currentModule->getObjectHandler()[getObjRef()]);
    }

    if(permanent) {
        //Set the team of our mount as well
        if(isBeingHeld() && getHolder()->isMount()) {
            getHolder()->setTeam(team_new, false);
        }

        //Switch team of whatever we are holding as well
        if(getLeftHandItem()) {
            getLeftHandItem()->setTeam(team_new, false);
        }
        if(getRightHandItem()) {
            getRightHandItem()->setTeam(team_new, false);
        }
    }
}

bool Object::hasSkillIDSZ(const IDSZ2& whichskill) const
{
    if (isTerminated()) return false;

    //Any [NONE] IDSZ returns always "true"
    if ( IDSZ2::None == whichskill ) return true;

    // First check the character Skill ID matches
    if ( getProfile()->getIDSZ(IDSZ_SKILL) == whichskill ) {
        return true;
    }

    // Then check if any Perk matches
    switch(whichskill.toUint32())
    {
        case IDSZ2::caseLabel('P', 'O', 'I', 'S'):
            return hasPerk(Ego::Perks::POISONRY);

        case IDSZ2::caseLabel('C', 'K', 'U', 'R'):
            return hasPerk(Ego::Perks::SENSE_KURSES);

        case IDSZ2::caseLabel('D', 'A', 'R', 'K'):
            return hasPerk(Ego::Perks::NIGHT_VISION) || hasPerk(Ego::Perks::PERCEPTIVE);

        case IDSZ2::caseLabel('A', 'W', 'E', 'P'):
            return hasPerk(Ego::Perks::WEAPON_PROFICIENCY);

        case IDSZ2::caseLabel('W', 'M', 'A', 'G'):
            return hasPerk(Ego::Perks::ARCANE_MAGIC);

        case IDSZ2::caseLabel('D', 'M', 'A', 'G'):
        case IDSZ2::caseLabel('H', 'M', 'A', 'G'):
            return hasPerk(Ego::Perks::DIVINE_MAGIC);

        case IDSZ2::caseLabel('D', 'I', 'S', 'A'):
            return hasPerk(Ego::Perks::TRAP_LORE);

        case IDSZ2::caseLabel('F', 'I', 'N', 'D'):
            return hasPerk(Ego::Perks::PERCEPTIVE);

        case IDSZ2::caseLabel('T', 'E', 'C', 'H'):
            return hasPerk(Ego::Perks::USE_TECHNOLOGICAL_ITEMS);

        case IDSZ2::caseLabel('S', 'T', 'A', 'B'):
            return hasPerk(Ego::Perks::BACKSTAB);

        case IDSZ2::caseLabel('R', 'E', 'A', 'D'):
            return hasPerk(Ego::Perks::LITERACY);

        case IDSZ2::caseLabel('W', 'A', 'N', 'D'):
            return hasPerk(Ego::Perks::THAUMATURGY);

        case IDSZ2::caseLabel('J', 'O', 'U', 'S'):
            return hasPerk(Ego::Perks::JOUSTING);            

        case IDSZ2::caseLabel('T', 'E', 'L', 'E'):
            return hasPerk(Ego::Perks::TELEPORT_MASTERY); 
    }

    //Skill not found
    return false;    
}

void Object::dropMoney(int amount)
{
    static constexpr std::array<int, PIP_MONEY_COUNT> vals = {1, 5, 25, 100, 200, 500, 1000, 2000};
    static constexpr std::array<PIP_REF, PIP_MONEY_COUNT> pips =
    {
        PIP_COIN1, PIP_COIN5, PIP_COIN25, PIP_COIN100,
        PIP_GEM200, PIP_GEM500, PIP_GEM1000, PIP_GEM2000
    };

    // limit the about of money to the character's actual money
    amount = Ego::Math::constrain<int>(amount, 0, getMoney());

    //Not valid to drop any money?
    if(getPosZ() <= (PITDEPTH/2) || amount <= 0) {
        return;
    }

    // remove the money from inventory
    money -= amount;

    // make the particles emit from "waist high"
    Vector3f pos = getPosition();
    pos[kZ] += (chr_min_cv._maxs[OCT_Z] + chr_min_cv._mins[OCT_Z]) * 0.5f;

    // Give the character a time-out from interacting with particles so it
    // doesn't just grab the money again
    damage_timer = DAMAGETIME;

    // count and spawn the various denominations
    for (size_t cnt = PIP_MONEY_COUNT - 1; amount > 0; cnt--)
    {
        int count = amount / vals[cnt];
        amount -= count * vals[cnt];

        for (size_t tnc = 0; tnc < count; tnc++)
        {
            ParticleHandler::get().spawnGlobalParticle(pos, ATK_FRONT, LocalParticleProfileRef(pips[cnt]), tnc );
        }
    }
}

void Object::dropKeys()
{
    // Don't lose keys in pits...
    if (getPosZ() <= (PITDEPTH / 2)) return;

    // The IDSZs to find
    const IDSZ2 testa = IDSZ2( 'K', 'E', 'Y', 'A' );  // [KEYA]
    const IDSZ2 testz = IDSZ2( 'K', 'E', 'Y', 'Z' );  // [KEYZ]

    //check each inventory item
    for(const std::shared_ptr<Object> &pkey : getInventory().iterate())
    {
        TURN_T turn;

        const IDSZ2& idsz_parent = pkey->getProfile()->getIDSZ(IDSZ_PARENT);
        const IDSZ2& idsz_type   = pkey->getProfile()->getIDSZ(IDSZ_TYPE);

        //is it really a key?
        if (( idsz_parent.toUint32() < testa.toUint32() && idsz_parent.toUint32() > testz.toUint32() ) &&
            ( idsz_type.toUint32() < testa.toUint32() && idsz_type.toUint32() > testz.toUint32() ) ) continue;

        FACING_T direction = Random::next(std::numeric_limits<FACING_T>::max());
        turn      = TO_TURN(direction);

        //remove it from inventory
        getInventory().removeItem(pkey, true);

        // fix the attachments
        pkey->dismount_timer         = PHYS_DISMOUNT_TIME;
        pkey->dismount_object        = getObjRef();
        pkey->onwhichplatform_ref    = onwhichplatform_ref;
        pkey->onwhichplatform_update = onwhichplatform_update;

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
        pkey->setPosition(getPosition());
    }    
}

void Object::dropAllItems()
{
    //Drop held items
    const std::shared_ptr<Object> &leftItem = getLeftHandItem();
    if(leftItem) {
        leftItem->detatchFromHolder(true, false);
    }
    const std::shared_ptr<Object> &rightItem = getRightHandItem();
    if(rightItem) {
        rightItem->detatchFromHolder(true, false);
    }

    //simply count the number of items in inventory
    uint8_t pack_count = getInventory().iterate().size();

    //Don't continue if we have nothing to drop
    if(pack_count == 0) {
        return;
    }

    //Calculate direction offset for each object
    const FACING_T diradd = 0xFFFF / pack_count;

    // now drop each item in turn
    FACING_T direction = ori.facing_z + ATK_BEHIND;
    for(const std::shared_ptr<Object> &pitem : getInventory().iterate())
    {
        //remove it from inventory
        getInventory().removeItem(pitem, true);

        // detach the item
        pitem->detatchFromHolder(true, true);

        // fix the attachments
        pitem->dismount_timer         = PHYS_DISMOUNT_TIME;
        pitem->dismount_object        = getObjRef();
        pitem->onwhichplatform_ref    = onwhichplatform_ref;
        pitem->onwhichplatform_update = onwhichplatform_update;

        // fix some flags
        pitem->hitready               = true;
        pitem->ori.facing_z           = direction + ATK_BEHIND;
        pitem->team                   = pitem->team_base;

        // fix the current velocity
        TURN_T turn                   = TO_TURN( direction );
        pitem->vel[kX]                += turntocos[ turn ] * DROPXYVEL;
        pitem->vel[kY]                += turntosin[ turn ] * DROPXYVEL;
        pitem->vel[kZ]                += DROPZVEL;

        // do some more complicated things
        SET_BIT(pitem->ai.alert, ALERTIF_DROPPED);
        pitem->setPosition(getPosition());

        //drop out evenly in all directions
        direction += diradd;
    }
}

bool Object::canCollide() const
{
    //Removed from game?
    if(isTerminated()) {
        return false;
    }

    //Hidden state
    if(isHidden()) {
        return false;
    }

    //Inside inventory or being held?
    if(isBeingHeld()) {
        return false;
    }

    //No collision box?
    if (oct_bb_t::empty(chr_max_cv)) {
        return false;
    }

    return true;
}

const std::shared_ptr<Object>& Object::getAttachedPlatform() const
{
    return _currentModule->getObjectHandler()[onwhichplatform_ref];
}

std::shared_ptr<const Ego::Texture> Object::getIcon() const
{
    //Is it a spellbook?
    if (getProfile()->getSpellEffectType() == ObjectProfile::NO_SKIN_OVERRIDE)
    {
        return getProfile()->getIcon(skin).get_ptr();
    }
    else
    {
        return ProfileSystem::get().getSpellBookIcon(getProfile()->getSpellEffectType()).get_ptr();
    }
}
