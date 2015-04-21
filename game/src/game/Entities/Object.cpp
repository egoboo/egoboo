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
#include "game/Module/Module.hpp"
#include "game/Entities/ObjectHandler.hpp"
#include "game/Profiles/_Include.hpp"
#include "game/Entities/Object.hpp"
#include "game/Entities/ObjectHandler.hpp"
#include "game/Entities/EnchantHandler.hpp"
#include "game/Profiles/_Include.hpp"
#include "game/game.h"
#include "game/player.h"
#include "game/char.h" //ZF> TODO: remove


//Declare class static constants
const size_t Object::MAXNUMINPACK;


Object::Object(const PRO_REF profile, const CHR_REF id) : 
    bsp_leaf(),
    spawn_data(),
    ai(),
    latch(),
    Name(),
    gender(GENDER_MALE),
    life_color(0),
    life(0),
    life_max(0),
    life_return(0),
    mana_color(0),
    mana(0),
    mana_max(0),
    mana_return(0),
    mana_flow(0),
    strength(0),
    wisdom(0),
    intelligence(0),
    dexterity(0),
    experience(0),
    experiencelevel(0),
    money(0),
    ammomax(0),
    ammo(0),
    holdingwhich(),
    equipment(),
    inventory(),
    team(TEAM_NULL),
    team_base(TEAM_NULL),
    firstenchant(INVALID_ENC_REF),
    undoenchant(INVALID_ENC_REF), 
    fat_stt(0.0f),
    fat(0.0f),
    fat_goto(0.0f),
    fat_goto_time(0),
    jump_power(0.0f),
    jump_timer(JUMPDELAY),
    jumpnumber(0),
    jumpnumberreset(0),
    jumpready(0),
    attachedto(INVALID_CHR_REF),
    inwhich_slot(SLOT_LEFT),
    inwhich_inventory(INVALID_CHR_REF),
    platform(false),
    canuseplatforms(false),
    holdingweight(0),
    targetplatform_level(0.0f),
    targetplatform_ref(INVALID_CHR_REF),
    onwhichplatform_ref(INVALID_CHR_REF),
    onwhichplatform_update(0),
    damagetarget_damagetype(0),
    reaffirm_damagetype(0),
    damage_modifier(),
    damage_resistance(),
    defense(0),
    damage_boost(0),
    damage_threshold(0),
    missiletreatment(MISSILE_NORMAL),
    missilecost(0),
    missilehandler(INVALID_CHR_REF),
    is_hidden(false),
    alive(true),
    waskilled(false),
    is_which_player(INVALID_PLA_REF),
    islocalplayer(false),
    invictus(false),
    iskursed(false),
    nameknown(false),
    ammoknown(false),
    hitready(true),
    isequipped(false),
    isitem(false),
    cangrabmoney(false),
    openstuff(false),
    stickybutt(false),
    isshopitem(false),
    canbecrushed(false),
    canchannel(false),
    grog_timer(0),
    daze_timer(0),
    bore_timer(BORETIME),
    careful_timer(CAREFULTIME),
    reload_timer(0),
    damage_timer(0),
    flashand(0),
    transferblend(false),
    draw_icon(false),
    sparkle(NOSPARKLE),
    show_stats(false),
    uoffvel(0),
    voffvel(0),
    shadow_size_stt(0.0f),
    shadow_size(0),
    shadow_size_save(0),
    ibillboard(INVALID_BBOARD_REF),
    is_overlay(false),
    skin(0),
    profile_ref(profile),
    basemodel_ref(profile),
    inst(),
    darkvision_level(0),
    see_kurse_level(0),
    see_invisible_level(0),
    skills(),

    bump_stt(),
    bump(),
    bump_save(),
    bump_1(),      
    chr_max_cv(),  
    chr_min_cv(),  
    slot_cv(),

    stoppedby(0),
    pos_stt(0, 0, 0),
    vel(0, 0, 0),

    ori(),
    pos_old(0, 0, 0), 
    vel_old(0, 0, 0),
    ori_old(),
    onwhichgrid(),
    onwhichblock(),
    bumplist_next(INVALID_CHR_REF),

    waterwalk(false),
    turnmode(TURNMODE_VELOCITY),
    movement_bits(( unsigned )(~0)),    // all movements valid
    anim_speed_sneak(0.0f),
    anim_speed_walk(0.0f),
    anim_speed_run(0.0f),
    maxaccel(0.0f),
    maxaccel_reset(0.0f),

    flyheight(0),
    phys(),
    enviro(),
    dismount_timer(0),  /// @note ZF@> If this is != 0 then scorpion claws and riders are dropped at spawn (non-item objects)
    dismount_object(INVALID_CHR_REF),
    safe_valid(false),
    safe_pos(0, 0, 0),
    safe_time(0),
    safe_grid(0),
    crumbs(),

    _terminateRequested(false),
    _characterID(id),
    _profile(ProfileSystem::get().getProfile(profile)),
    _position(0.0f, 0.0f, 0.0f)
{
    // Construct the BSP node for this entity.
    bsp_leaf.ctor(&bsp_leaf, BSP_LEAF_CHR, _characterID);

    // Grip info
    holdingwhich.fill(INVALID_CHR_REF);

    //Damage resistance
    damage_modifier.fill(0);
    damage_resistance.fill(0);

    // pack/inventory info
    equipment.fill(INVALID_CHR_REF);
    inventory.fill(INVALID_CHR_REF);

    // Set up position
    ori.map_twist_facing_y = MAP_TURN_OFFSET;  // These two mean on level surface
    ori.map_twist_facing_x = MAP_TURN_OFFSET;

    //---- call the constructors of the "has a" classes

    // set the insance values to safe values
    chr_instance_ctor( &inst );

    // intialize the ai_state
    ai_state_ctor( &ai );

    // initialize the physics
    phys_data_ctor( &phys );
}

Object::~Object()
{
    /// @author ZZ
    /// @details Make character safely deleteable

    // Detach the character from the game
    if(PMod) {
        cleanup_one_character(this);
    }

    // free the character's inventory
    free_inventory_in_game(getCharacterID());

    //If we are inside an inventory we need to remove us
    const std::shared_ptr<Object> &inventoryHolder = _gameObjects[inwhich_inventory];
    if(inventoryHolder) {
        for (size_t i = 0; i < inventoryHolder->inventory.size(); i++)
        {
            if(inventoryHolder->inventory[i] == getCharacterID()) 
            {
                inventoryHolder->inventory[i] = INVALID_CHR_REF;
                break;
            }
        }
    }

    // Remove from stat list
    if (show_stats)
    {
        size_t  cnt;
        bool stat_found;

        show_stats = false;

        stat_found = false;
        for (cnt = 0; cnt < StatusList.count; cnt++)
        {
            if ( StatusList.lst[cnt].who == getCharacterID() )
            {
                stat_found = true;
                break;
            }
        }

        if ( stat_found )
        {
            for (cnt++; cnt < StatusList.count; cnt++)
            {
                SWAP( status_list_element_t, StatusList.lst[cnt-1], StatusList.lst[cnt] );
            }
            StatusList.count--;
        }
    }

    // Handle the team
    if ( isAlive() && !getProfile()->isInvincible() && TeamStack.lst[team_base].morale > 0 )
    {
        TeamStack.lst[team_base].morale--;
    }

    if ( TeamStack.lst[team].leader == getCharacterID() )
    {
        TeamStack.lst[team].leader = TEAM_NOLEADER;
    }

    // remove any attached particles
    disaffirm_attached_particles( getCharacterID() );    

    /// Free all allocated memory

    // deallocate
    BillboardList_free_one(ibillboard);

    chr_instance_dtor( &inst );
    ai_state_dtor( &ai );

    EGOBOO_ASSERT( nullptr == inst.vrt_lst );    
}

bool Object::isOverWater(bool anyLiquid) const
{
	//Make sure water in the current module is actually water (could be lava, acid, etc.)
	if(!anyLiquid && !water.is_water)
    {
		return false;
	}

	//Check if we are on a valid tile
    if (!ego_mesh_grid_is_valid(PMesh, onwhichgrid))
    {
    	return false;
    }

    return 0 != ego_mesh_t::test_fx(PMesh, onwhichgrid, MAPFX_WATER);
}

bool Object::isInWater(bool anyLiquid) const
{
    return isOverWater(anyLiquid) && getPosZ() < water_instance_get_water_level(&water);
}


bool Object::setPosition(const fvec3_t& position)
{
    EGO_DEBUG_VALIDATE(position);

    //Has our position changed?
    if(position != _position)
    {
        _position = position;

        onwhichgrid   = ego_mesh_t::get_grid(PMesh, PointWorld(_position.x, _position.y));
        onwhichblock  = ego_mesh_t::get_block(PMesh, PointWorld(_position.x, _position.y));

        // update whether the current character position is safe
        chr_update_safe( this, false );

        // update the breadcrumb list
        chr_update_breadcrumb( this, false );

        return true;
    }

    return false;
}


void Object::movePosition(const float x, const float y, const float z)
{
    _position.x += x;
    _position.y += y;
    _position.z += z;
}

void Object::setAlpha(const int alpha)
{
    inst.alpha = Math::constrain(alpha, 0, 0xFF);
    chr_instance_update_ref(&inst, enviro.grid_level, false);
}

void Object::setLight(const int light)
{
    inst.light = Math::constrain(light, 0, 0xFF);

    //This prevents players from becoming completely invisible
    if (VALID_PLA(is_which_player))  
    {
        inst.light = std::max<uint8_t>(128, inst.light);
    }

    chr_instance_update_ref(&inst, enviro.grid_level, false);
}

void Object::setSheen(const int sheen)
{
    inst.sheen = Math::constrain(sheen, 0, 0xFF);
    chr_instance_update_ref(&inst, enviro.grid_level, false);
}

bool Object::canMount(const std::shared_ptr<Object> mount) const
{
    //Cannot mount ourselves!
    if(this == mount.get())
    {
        return false;
    }

    //Make sure they are a mount and alive
    if(!mount->isMount() || !mount->alive)
    {
        return false;
    }

    //We must be alive and not an item to become a rider
    if(!alive || isitem || _gameObjects.exists(attachedto))
    {
        return false;
    }

    //Cannot mount while flying
    if(flyheight != 0)
    {
        return false;
    }

    //Make sure they aren't mounted already
    if(!mount->getProfile()->isSlotValid(SLOT_LEFT) || _gameObjects.exists(mount->holdingwhich[SLOT_LEFT]))
    {
        return false;
    }

    //We need a riding animation to be able to mount stuff
    int action_mi = mad_get_action_ref( chr_get_imad( _characterID ), ACTION_MI );
    bool has_ride_anim = ( ACTION_COUNT != action_mi && !ACTION_IS_TYPE( action_mi, D ) );

    return has_ride_anim;
}

int Object::damage(const FACING_T direction, const IPair  damage, const DamageType damagetype, const TEAM_REF team,
                   const std::shared_ptr<Object> &attacker, const BIT_FIELD effects, const bool ignore_invictus)
{
    int action;
    bool do_feedback = (Ego::FeedbackType::None != egoboo_config_t::get().hud_feedback.getValue());

    // Simply ignore damaging invincible targets.
    if(invictus && !ignore_invictus)
    {
        return 0;
    }

    // Don't continue if there is no damage or the character isn't alive.
    int max_damage = std::abs( damage.base ) + std::abs( damage.rand );
    if ( !isAlive() || 0 == max_damage ) return 0;

    // make a special exception for DAMAGE_NONE
    uint8_t damageModifier = ( damagetype >= DAMAGE_COUNT ) ? 0 : damage_modifier[damagetype];

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

    // Lessen actual_damage for resistance, resistance is done in percentages where 0.70f means 30% damage reduction from that damage type
    // This can also be used to lessen effectiveness of healing
    int actual_damage = Random::next(damage.base, damage.base+damage.rand);
    int base_damage   = actual_damage;
    actual_damage *= std::max( 0.00f, ( damagetype >= DAMAGE_COUNT ) ? 1.00f : 1.00f - damage_resistance[damagetype] );

    // Increase electric damage when in water
    if ( damagetype == DAMAGE_ZAP && isInWater(false) )
    {
        actual_damage *= 2.0f;     /// @note ZF> Is double damage too much?
    }

    // Allow actual_damage to be dealt to mana (mana shield spell)
    if (HAS_SOME_BITS(damageModifier, DAMAGEMANA))
    {
        int manadamage;
        manadamage = std::max( mana - actual_damage, 0 );
        mana = manadamage;
        actual_damage -= manadamage;
        updateLastAttacker(attacker, false);
    }

    // Allow charging (Invert actual_damage to mana)
    if (HAS_SOME_BITS(damageModifier, DAMAGECHARGE))
    {
        mana += actual_damage;
        if ( mana > mana_max )
        {
            mana = mana_max;
        }
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
    bool immune_to_damage = (actual_damage > 0 && actual_damage <= damage_threshold) || HAS_SOME_BITS(damageModifier, DAMAGEINVICTUS);
    if ( immune_to_damage && !ignore_invictus )
    {
        actual_damage = 0;

        //Tell that the character is simply immune to the damage
        //but don't do message and ping for mounts, it's just irritating
        if ( !isMount() )
        {
            //Dark green text
            const float lifetime = 3;
            SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};
            GLXvector4f tint  = { 0.0f, 0.5f, 0.00f, 1.00f };

            spawn_defense_ping(this, attacker ? attacker->getCharacterID() : INVALID_CHR_REF);
            chr_make_text_billboard(_characterID, "Immune!", text_color, tint, lifetime, bb_opt_all);
        }
    }

    // Do it already
    if ( actual_damage > 0 )
    {
        // Only actual_damage if not invincible
        if ( 0 == damage_timer || ignore_invictus )
        {
            // Normal mode reduces damage dealt by monsters with 30%!
            if (egoboo_config_t::get().game_difficulty.getValue() == Ego::GameDifficulty::Normal && VALID_PLA(is_which_player))
            {
                actual_damage *= 0.70f;
            }

            // Easy mode deals 25% extra actual damage by players and 50% less to players
            if (egoboo_config_t::get().game_difficulty.getValue() <= Ego::GameDifficulty::Easy)
            {
                if ( VALID_PLA( attacker->is_which_player )  && !VALID_PLA(is_which_player) ) actual_damage *= 1.25f;
                if ( !VALID_PLA( attacker->is_which_player ) &&  VALID_PLA(is_which_player) ) actual_damage *= 0.5f;
            }

            if ( 0 != actual_damage )
            {
                //Does armor apply?
                if ( HAS_NO_BITS( DAMFX_ARMO, effects ) )
                {
                    //Armor can reduce up to 50% of the damage (at 255)
                    actual_damage *= 0.5f + (256.0f - defense)/256.0f;
                }

                life -= actual_damage;

                // Spawn blud particles
                if ( _profile->getBludType() )
                {
                    if ( _profile->getBludType() == ULTRABLUDY || ( base_damage > HURTDAMAGE && DAMAGE_IS_PHYSICAL( damagetype ) ) )
                    {
                        spawnOneParticle( getPosition(), ori.facing_z + direction, _profile->getSlotNumber(), _profile->getBludParticleProfile(),
                                            INVALID_CHR_REF, GRIP_LAST, team, _characterID);
                    }
                }

                // Set attack alert if it wasn't an accident
                if ( base_damage > HURTDAMAGE )
                {
                    if ( team == TEAM_DAMAGE )
                    {
                        ai.attacklast = INVALID_CHR_REF;
                    }
                    else
                    {
                        updateLastAttacker(attacker, false );
                    }
                }

                //Did we survive?
                if (life <= 0)
                {
                    CHR_REF attacker_ref = INVALID_CHR_REF;
                    if (attacker) attacker_ref = attacker->getCharacterID();
                    kill_character( _characterID, attacker_ref, ignore_invictus );
                }
                else
                {
                    //Yes, but play the hurt animation
                    action = ACTION_HA;
                    if ( base_damage > HURTDAMAGE )
                    {
                        action += Random::next(3);
                        chr_play_action(this, action, false);

                        // Make the character invincible for a limited time only
                        if ( HAS_NO_BITS( effects, DAMFX_TIME ) )
                        {
                            damage_timer = DAMAGETIME;
                        }
                    }
                }
            }

            /// @test spawn a fly-away damage indicator?
            if ( do_feedback )
            {
/*                
                const char * tmpstr;
                int rank;


                tmpstr = describe_wounds( pchr->life_max, pchr->life );

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
*/
                {
                    const int lifetime = 2;
                    STRING text_buffer = EMPTY_CSTR;

                    // "white" text
                    SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};

                    // friendly fire damage = "purple"
                    GLXvector4f tint_friend = { 0.88f, 0.75f, 1.00f, 1.00f };

                    // enemy damage color depends on damage type
                    float r, g, b;
                    switch(damagetype)
                    {
                        //Blue
                        case DAMAGE_ZAP: r = 1.00f; g = 1.0f; b = 0.00f; break;

                        //Red
                        case DAMAGE_FIRE: r = 1.00f; g = 0.00f; b = 0.00f; break;

                        //Green
                        case DAMAGE_EVIL: r = 0.00f; g = 1.0f; b = 0.00f; break;

                        //Purple
                        case DAMAGE_HOLY: r = 0.88f; g = 0.75f; b = 1.00f; break;

                        //Blue
                        case DAMAGE_ICE: r = 0.00f; g = 1.0f; b = 1.00f; break;

                        //White
                        default: r = 1.00f; g = 1.0f; b = 1.00f; break;
                    }

                    GLXvector4f tint_enemy  = { r, g, b, 1.00f };

                    // write the string into the buffer
                    snprintf( text_buffer, SDL_arraysize( text_buffer ), "%.1f", static_cast<float>(actual_damage) / 256.0f );

                    chr_make_text_billboard(_characterID, text_buffer, text_color, friendly_fire ? tint_friend : tint_enemy, lifetime, bb_opt_all );
                }
            }
        }
    }

    // Heal 'em instead
    else if ( actual_damage < 0 )
    {
        heal(attacker, -actual_damage, ignore_invictus);

        // Isssue an alert
        if ( team == TEAM_DAMAGE )
        {
            ai.attacklast = INVALID_CHR_REF;
        }

        /// @test spawn a fly-away heal indicator?
        if ( do_feedback )
        {
            const float lifetime = 3;
            STRING text_buffer = EMPTY_CSTR;

            // "white" text
            SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};

            // heal == yellow, right ;)
            GLXvector4f tint = { 1.00f, 1.00f, 0.75f, 1.00f };

            // write the string into the buffer
            snprintf( text_buffer, SDL_arraysize( text_buffer ), "%s", describe_value( -actual_damage, damage.base + damage.rand, NULL ) );

            chr_make_text_billboard(_characterID, text_buffer, text_color, tint, lifetime, bb_opt_all );
        }
    }

    return actual_damage;
}

void Object::updateLastAttacker(const std::shared_ptr<Object> &attacker, bool healing)
{
    // Don't let characters chase themselves...  That would be silly
    if ( this == attacker.get() ) return;


    // Don't alert the character too much if under constant fire
    if (0 != careful_timer) return;

    CHR_REF actual_attacker = INVALID_CHR_REF;

    // Figure out who is the real attacker, in case we are a held item or a controlled mount
    if(attacker)
    {
        actual_attacker = attacker->getCharacterID();

        //Dont alert if the attacker/healer was on the null team
        if(attacker->getTeam() == TEAM_NULL) {
            return;
        }
    
        //Do not alert items damaging (or healing) their holders, healing potions for example
        if ( attacker->attachedto == ai.index ) return;

        //If we are held, the holder is the real attacker... unless the holder is a mount
        if ( _gameObjects.exists( attacker->attachedto ) && !_gameObjects.get(attacker->attachedto)->isMount() )
        {
            actual_attacker = attacker->attachedto;
        }

        //If the attacker is a mount, try to blame the rider
        else if ( attacker->isMount() && _gameObjects.exists( attacker->holdingwhich[SLOT_LEFT] ) )
        {
            actual_attacker = attacker->holdingwhich[SLOT_LEFT];
        }
    }

    //Update alerts and timers
    ai.attacklast = actual_attacker;
    SET_BIT(ai.alert, healing ? ALERTIF_HEALED : ALERTIF_ATTACKED);
    careful_timer = CAREFULTIME;
}

bool Object::heal(const std::shared_ptr<Object> &healer, const UFP8_T amount, const bool ignoreInvincibility)
{
    //Don't heal dead and invincible stuff
    if (!alive || (invictus && !ignoreInvincibility)) return false;

    //This actually heals the character
    life = CLIP(static_cast<UFP8_T>(life), life + amount, life_max);

    // Set alerts, but don't alert that we healed ourselves
    if (healer && this != healer.get() && healer->attachedto != _characterID && amount > HURTDAMAGE)
    {
        updateLastAttacker(healer, true);
    }

    return true;
}

bool Object::isAttacking() const
{
    return inst.action_which >= ACTION_UA && inst.action_which <= ACTION_FD;
}

bool Object::teleport(const float x, const float y, const float z, const FACING_T facing_z)
{
    //Cannot teleport outside the level
    if ( x < 0.0f || x > PMesh->gmem.edge_x ) return false;
    if ( y < 0.0f || y > PMesh->gmem.edge_y ) return false;

    fvec3_t newPosition = fvec3_t(x, y, z);

    //Cannot teleport inside a wall
    if ( !chr_hit_wall(this, newPosition, NULL, NULL, NULL) )
    {
        // Yeah!  It worked!

        // update the old position
        pos_old          = newPosition;
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
    //then do status updates
    chr_update_hide(this);

    //Don't do items that are in inventory
    if ( _gameObjects.exists( inwhich_inventory ) ) {
        return;
    }

    const float WATER_LEVEL = water_instance_get_water_level(&water);

    // do the character interaction with water
    if (!isHidden() && isInWater(true))
    {
        // do splash and ripple
        if ( !enviro.inwater )
        {
            // Splash
            fvec3_t vtmp;

            vtmp.x = getPosX();
            vtmp.y = getPosY();
            vtmp.z = WATER_LEVEL + RAISE;

            spawn_one_particle_global( vtmp, ATK_FRONT, PIP_SPLASH, 0 );

            if ( water.is_water )
            {
                SET_BIT(ai.alert, ALERTIF_INWATER);
            }
        }

        //Submerged in water (fully or partially)
        else
        {
            // Ripples
            if(getPosZ() < WATER_LEVEL && isAlive())
            {
                if ( !_gameObjects.exists(attachedto) && getProfile()->causesRipples() 
                    && getPosZ() + chr_min_cv.maxs[OCT_Z] + RIPPLETOLERANCE > WATER_LEVEL 
                    && getPosZ() + chr_min_cv.mins[OCT_Z] < WATER_LEVEL)
                {
                    int ripple_suppression;

                    // suppress ripples if we are far below the surface
                    ripple_suppression = WATER_LEVEL - (getPosZ() + chr_min_cv.maxs[OCT_Z]);
                    ripple_suppression = ( 4 * ripple_suppression ) / RIPPLETOLERANCE;
                    ripple_suppression = Math::constrain(ripple_suppression, 0, 4);

                    // make more ripples if we are moving
                    ripple_suppression -= (( int )vel.x != 0 ) | (( int )vel.y != 0 );

                    int ripand;
                    if ( ripple_suppression > 0 )
                    {
                        ripand = ~(( ~RIPPLEAND ) << ripple_suppression );
                    }
                    else
                    {
                        ripand = RIPPLEAND >> ( -ripple_suppression );
                    }

                    if ( 0 == ( (update_wld + getCharacterID()) & ripand ))
                    {
                        fvec3_t vtmp;

                        vtmp.x = getPosX();
                        vtmp.y = getPosY();
                        vtmp.z = WATER_LEVEL;

                        spawn_one_particle_global( vtmp, ATK_FRONT, PIP_RIPPLE, 0 );
                    }    
                }
            }

            if (water.is_water && HAS_NO_BITS(update_wld, 7))
            {
                jumpready = true;
                jumpnumber = 1;
            }
        }

        enviro.inwater  = true;
    }
    else
    {
        enviro.inwater = false;
    }

    // the following functions should not be done the first time through the update loop
    if (0 == update_wld) return;

    //---- Do timers and such

    // reduce attack cooldowns
    if ( reload_timer > 0 ) reload_timer--;

    // decrement the dismount timer
    if ( dismount_timer > 0 ) dismount_timer--;

    if ( 0 == dismount_timer )
    {
        dismount_object = INVALID_CHR_REF;
    }

    // Down that ol' damage timer
    if ( damage_timer > 0 ) damage_timer--;

    // Do "Be careful!" delay
    if ( careful_timer > 0 ) careful_timer--;

    // Texture movement
    inst.uoffset += uoffvel;
    inst.voffset += voffvel;

    // Do stats once every second
    if ( clock_chr_stat >= ONESECOND )
    {
        // check for a level up
        do_level_up( getCharacterID() );

        // do the mana and life regen for "living" characters
        if (isAlive())
        {
            int manaregen = 0;
            int liferegen = 0;
            get_chr_regeneration( this, &liferegen, &manaregen );

            mana += manaregen;
            mana = CLIP((UFP8_T)mana, (UFP8_T)0, mana_max);

            life += liferegen;
            life = CLIP((UFP8_T)life, (UFP8_T)1, life_max);
        }

        // countdown confuse effects
        if (grog_timer > 0)
        {
           grog_timer--;
        }

        if (daze_timer > 0)
        {
           daze_timer--;
        }

        // possibly gain/lose darkvision
        update_chr_darkvision( getCharacterID() );
    }

    updateResize();

    // update some special skills
    see_kurse_level  = std::max(see_kurse_level,  chr_get_skill(this, MAKE_IDSZ( 'C', 'K', 'U', 'R' )));
    darkvision_level = std::max(darkvision_level, chr_get_skill(this, MAKE_IDSZ( 'D', 'A', 'R', 'K' )));
}

void Object::updateResize()
{
    if (fat_goto_time < 0) {
        return;
    }

    if (fat_goto != fat)
    {
        int bump_increase;

        bump_increase = ( fat_goto - fat ) * 0.10f * bump.size;

        // Make sure it won't get caught in a wall
        bool willgetcaught = false;
        if ( fat_goto > fat )
        {
            bump.size += bump_increase;

            if ( EMPTY_BIT_FIELD != Objectest_wall(this, NULL ) )
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
            chr_set_fat(this, newsize);

            if ( CAP_INFINITE_WEIGHT == getProfile()->getWeight() )
            {
                phys.weight = CHR_INFINITE_WEIGHT;
            }
            else
            {
                Uint32 itmp = getProfile()->getWeight() * fat * fat * fat;
                phys.weight = std::min( itmp, CHR_MAX_WEIGHT );
            }
        }
    }
}

std::string Object::getName(bool prefixArticle, bool prefixDefinite, bool capitalLetter) const
{
    std::string result;

    if (isNameKnown())
    {
        result = Name;

        // capitalize the name ?
        if (capitalLetter)
        {
            result[0] = Ego::toupper(result[0]);
        }
    }
    else
    {
        if(getProfile()->getSpellEffectType() >= 0) {
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
                result = std::string("the ") + result;
            }
            else
            {
                char lTmp = Ego::toupper(result[0]);

                if ( 'A' == lTmp || 'E' == lTmp || 'I' == lTmp || 'O' == lTmp || 'U' == lTmp )
                {
                    result = std::string("an ") + result;
                }
                else
                {
                    result = std::string("a ") + result;
                }
            }
        }
    }

    return result;
}

void Object::requestTerminate() 
{
    //Mark object as terminated
    _gameObjects.remove(getCharacterID());
}


 bool Object::isFacingLocation(const float x, const float y) const
 {
    FACING_T facing = vec_to_facing(x - getPosX(), y - getPosY());
    facing -= ori.facing_z;
    return (facing > 55535 || facing < 10000);
 }

//--------------------------------------------------------------------------------------------
bool Object::detatchFromHolder(const bool ignoreKurse, const bool doShop)
{
    // Make sure the character is actually held by something first
    CHR_REF holder = attachedto;
    const std::shared_ptr<Object> &pholder = _gameObjects[holder];
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
    if ( !isitem ) dismount_timer  = PHYS_DISMOUNT_TIME;
    dismount_object = holder;

    // Figure out which hand it's in
    Uint16 hand = inwhich_slot;

    // Rip 'em apart
    attachedto = INVALID_CHR_REF;
    if ( pholder->holdingwhich[SLOT_LEFT] == getCharacterID() )
        pholder->holdingwhich[SLOT_LEFT] = INVALID_CHR_REF;

    if ( pholder->holdingwhich[SLOT_RIGHT] == getCharacterID() )
        pholder->holdingwhich[SLOT_RIGHT] = INVALID_CHR_REF;

    if ( isAlive() )
    {
        // play the falling animation...
        chr_play_action( this, ACTION_JB + hand, false );
    }
    else if ( inst.action_which < ACTION_KA || inst.action_which > ACTION_KD )
    {
        // play the "killed" animation...
        chr_play_action( this, Random::next((int)ACTION_KA, ACTION_KA + 3), false );
        chr_instance_set_action_keep( &( inst ), true );
    }

    // Set the positions
    if ( chr_matrix_valid( this ) )
    {
        setPosition(mat_getTranslate_v(inst.matrix.v));
    }
    else
    {
        setPosition(pholder->getPosition());
    }

    // Make sure it's not dropped in a wall...
    if (EMPTY_BIT_FIELD != Objectest_wall(this, NULL))
    {
        fvec3_t pos_tmp = pholder->getPosition();
        pos_tmp.z = getPosZ();

        setPosition(pos_tmp);

        chr_update_breadcrumb(this, true);
    }

    // Check for shop passages
    bool inshop = false;
    if ( doShop )
    {
        inshop = do_shop_drop(holder, getCharacterID());
    }

    // Make sure it works right
    hitready = true;
    if ( inshop )
    {
        // Drop straight down to avoid theft
        vel.x = 0;
        vel.y = 0;
    }
    else
    {
        vel.x = pholder->vel.x;
        vel.y = pholder->vel.y;
    }

    vel.z = DROPZVEL;

    // Turn looping off
    chr_instance_set_action_loop( &( inst ), false );

    // Reset the team if it is a mount
    if ( pholder->isMount() )
    {
        pholder->team = pholder->team_base;
        SET_BIT( pholder->ai.alert, ALERTIF_DROPPED );
    }

    team = team_base;
    SET_BIT( ai.alert, ALERTIF_DROPPED );

    // Reset transparency
    if ( isitem && pholder->transferblend )
    {
        ENC_REF ienc_now, ienc_nxt;
        size_t  ienc_count;

        // cleanup the enchant list
        cleanup_character_enchants( this );

        // Okay, reset transparency
        ienc_now = firstenchant;
        ienc_count = 0;
        while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
        {
            ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

            enc_remove_set( ienc_now, eve_t::SETALPHABLEND );
            enc_remove_set( ienc_now, eve_t::SETLIGHTBLEND );

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

        setAlpha(getProfile()->getAlpha());
        setLight(getProfile()->getLight());

        // cleanup the enchant list
        cleanup_character_enchants( this );

        // apply the blend enchants
        ienc_now = firstenchant;
        ienc_count = 0;
        while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
        {
            PRO_REF ipro = enc_get_ipro( ienc_now );
            ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

            if (ProfileSystem::get().isValidProfileID(ipro))
            {
                enc_apply_set( ienc_now, eve_t::SETALPHABLEND, ipro );
                enc_apply_set( ienc_now, eve_t::SETLIGHTBLEND, ipro );
            }

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
    }

    // Set twist
    ori.map_twist_facing_y = MAP_TURN_OFFSET;
    ori.map_twist_facing_x = MAP_TURN_OFFSET;

    // turn off keeping, unless the object is dead
    if (!isAlive())
    {
        // the object is dead. play the killed animation and make it freeze there
        chr_play_action( this, Random::next((int)ACTION_KA, ACTION_KA + 3), false );
        chr_instance_set_action_keep( &inst, true );
    }
    else
    {
        // play the jump animation, and un-keep it
        chr_play_action( this, ACTION_JA, true );
        chr_instance_set_action_keep( &inst, false );
    }

    chr_update_matrix( this, true );

    return true;
}

const std::shared_ptr<Object>& Object::getLeftHandItem() const
{
    return _gameObjects[holdingwhich[SLOT_LEFT]];
}

const std::shared_ptr<Object>& Object::getRightHandItem() const
{
    return _gameObjects[holdingwhich[SLOT_RIGHT]];
}

