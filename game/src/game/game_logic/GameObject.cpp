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

/// @file game/game_logic/GameObject.hpp
/// @details An object representing instances of in-game egoboo objects (GameObject)
/// @author Johan Jansen

#include "game/game_logic/GameObject.hpp"
#include "game/module/ObjectHandler.hpp"
#include "game/profiles/ProfileSystem.hpp"
#include "game/game.h"
#include "game/player.h"
#include "game/char.h" //ZF> TODO: remove

//Declare class static constants
const size_t GameObject::MAXNUMINPACK;


GameObject::GameObject(const PRO_REF profile, const CHR_REF id) : 
    terminateRequested(false),
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
    is_which_player(INVALID_PLAYER_REF),
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
    ibillboard(INVALID_BILLBOARD_REF),
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
    onwhichgrid(0),
    onwhichblock(0),
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

    _characterID(id),
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

GameObject::~GameObject()
{
    /// Free all allocated memory

    // deallocate
    BillboardList_free_one(ibillboard);

    chr_instance_dtor( &inst );
    ai_state_dtor( &ai );

    EGOBOO_ASSERT( nullptr == inst.vrt_lst );
}

const std::shared_ptr<ObjectProfile>& GameObject::getProfile() const
{
    return _profileSystem.getProfile(profile_ref);
}

bool GameObject::isOverWater() const
{
	//Make sure water in the current module is actually water (could be lava, acid, etc.)
	if(!water.is_water) {
		return false;
	}

	//Check if we are on a valid tile
    if (!ego_mesh_grid_is_valid(PMesh, onwhichgrid)) {
    	return false;
    }

    return 0 != ego_mesh_test_fx(PMesh, onwhichgrid, MAPFX_WATER);
}


bool GameObject::setPosition(const fvec3_t& position)
{
    LOG_NAN_FVEC3(position);

    //Has our position changed?
    if(position != _position)
    {
        _position = position;

        onwhichgrid   = ego_mesh_get_grid(PMesh, _position.x, _position.y);
        onwhichblock  = ego_mesh_get_block(PMesh, _position.x, _position.y);

        // update whether the current character position is safe
        chr_update_safe( this, false );

        // update the breadcrumb list
        chr_update_breadcrumb( this, false );

        return true;
    }

    return false;
}


void GameObject::movePosition(const float x, const float y, const float z)
{
    _position.x += x;
    _position.y += y;
    _position.z += z;
}

void GameObject::setAlpha(const int alpha)
{
    inst.alpha = Math::constrain(alpha, 0, 0xFF);
    chr_instance_update_ref(&inst, enviro.grid_level, false);
}

void GameObject::setLight(const int light)
{
    inst.light = Math::constrain(light, 0, 0xFF);

    //This prevents players from becoming completely invisible
    if (VALID_PLA(is_which_player))  
    {
        inst.light = std::max<uint8_t>(128, inst.light);
    }

    chr_instance_update_ref(&inst, enviro.grid_level, false);
}

void GameObject::setSheen(const int sheen)
{
    inst.sheen = Math::constrain(sheen, 0, 0xFF);
    chr_instance_update_ref(&inst, enviro.grid_level, false);
}

bool GameObject::canMount(const std::shared_ptr<GameObject> mount) const
{
    //Cannot mount ourselves!
    if(this == mount.get()) {
        return false;
    }

    //Make sure they are a mount and alive
    if(!mount->isMount() || !mount->alive) {
        return false;
    }

    //We must be alive and not an item to become a rider
    if(!alive || isitem || _gameObjects.exists(attachedto)) {
        return false;
    }

    //Cannot mount while flying
    if(flyheight != 0) {
        return false;
    }

    //Make sure they aren't mounted already
    if(!mount->getProfile()->isSlotValid(SLOT_LEFT) || _gameObjects.exists(mount->holdingwhich[SLOT_LEFT])) {
        return false;
    }

    //We need a riding animation to be able to mount stuff
    int action_mi = mad_get_action_ref( chr_get_imad( _characterID ), ACTION_MI );
    bool has_ride_anim = ( ACTION_COUNT != action_mi && !ACTION_IS_TYPE( action_mi, D ) );

    return has_ride_anim;
}
