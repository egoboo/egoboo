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
#include "game/collision.h"                  //Only or detach_character_from_platform()
#include "game/obj_BSP.h"
#include "game/egoboo.h"
#include "game/Module/Passage.hpp"
#include "egolib/Profiles/_Include.hpp"
#include "game/Module/Module.hpp"

#include "game/Entities/ObjectHandler.hpp"
#include "game/Entities/EnchantHandler.hpp"
#include "game/Entities/ParticleHandler.hpp"
#include "game/mesh.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int chr_stoppedby_tests = 0;
int chr_pressure_tests = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct grab_data_t
{
    grab_data_t() :
        object(nullptr),
        horizontalDistance(0.0f),
        verticalDistance(0.0f),
        visible(true),
        isFacingObject(false)
    {
        //ctor
    }

    std::shared_ptr<Object> object;
    float horizontalDistance;
    float verticalDistance;
    bool visible;
    bool isFacingObject;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void chr_set_enviro_grid_level( Object * pchr, const float level );

static bool chr_download_profile(Object * pchr, const std::shared_ptr<ObjectProfile> &profile);

Object * chr_config_do_init( Object * pchr );
static int chr_change_skin( const CHR_REF character, const SKIN_T skin );
static void switch_team_base( const CHR_REF character, const TEAM_REF team_new, const bool permanent );

static void move_one_character_do_floor_friction( Object * pchr );
static void move_one_character_do_voluntary( Object * pchr );
static void move_one_character( Object * pchr );
static void move_one_character_do_z_motion( Object * pchr );
static bool move_one_character_integrate_motion( Object * pchr );
static bool move_one_character_integrate_motion_attached( Object * pchr );


static bool chr_do_latch_button( Object * pchr );
static bool chr_do_latch_attack( Object * pchr, slot_t which_slot );

static breadcrumb_t * chr_get_last_breadcrumb( Object * pchr );
static void chr_init_size( Object * pchr, const std::shared_ptr<ObjectProfile> &profile);

//--------------------------------------------------------------------------------------------
egolib_rv flash_character_height( const CHR_REF character, Uint8 valuelow, Sint16 low,
                                  Uint8 valuehigh, Sint16 high )
{
    /// @author ZZ
    /// @details This function sets a character's lighting depending on vertex height...
    ///    Can make feet dark and head light...

    Uint32 cnt;
    Sint16 z;

    chr_instance_t * pinst;

    pinst = chr_get_pinstance( character );
    if ( NULL == pinst ) return rv_error;

    for ( cnt = 0; cnt < pinst->vrt_count; cnt++ )
    {
        z = pinst->vrt_lst[cnt].pos[ZZ];

        if ( z < low )
        {
            pinst->vrt_lst[cnt].col[RR] =
                pinst->vrt_lst[cnt].col[GG] =
                    pinst->vrt_lst[cnt].col[BB] = valuelow;
        }
        else if ( z > high )
        {
            pinst->vrt_lst[cnt].col[RR] =
                pinst->vrt_lst[cnt].col[GG] =
                    pinst->vrt_lst[cnt].col[BB] = valuehigh;
        }
        else if ( high != low )
        {
            Uint8 valuemid = ( valuehigh * ( z - low ) / ( high - low ) ) +
                             ( valuelow * ( high - z ) / ( high - low ) );

            pinst->vrt_lst[cnt].col[RR] =
                pinst->vrt_lst[cnt].col[GG] =
                    pinst->vrt_lst[cnt].col[BB] =  valuemid;
        }
        else
        {
            // z == high == low
            Uint8 valuemid = ( valuehigh + valuelow ) * 0.5f;

            pinst->vrt_lst[cnt].col[RR] =
                pinst->vrt_lst[cnt].col[GG] =
                    pinst->vrt_lst[cnt].col[BB] =  valuemid;
        }
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
void chr_set_enviro_grid_level( Object * pchr, const float level )
{
	if (!pchr) {
		return;
	}
	if (level != pchr->enviro.grid_level) {
		pchr->enviro.grid_level = level;

		chr_instance_t::apply_reflection_matrix(pchr->inst, level);
	}
}

//--------------------------------------------------------------------------------------------
bool chr_copy_enviro( Object * chr_psrc, Object * chr_pdst )
{
    /// @author BB
    /// @details do a deep copy on the character's enviro data

    chr_environment_t * psrc, * pdst;

    if ( NULL == chr_psrc || NULL == chr_pdst ) return false;

    if ( chr_psrc == chr_pdst ) return true;

    psrc = &( chr_psrc->enviro );
    pdst = &( chr_pdst->enviro );

    // use the special function to set the grid level
    // this must done first so that the character's reflection data is set properly
    chr_set_enviro_grid_level( chr_pdst, psrc->grid_level );

    // now just copy the other data.
    // use memmove() in the odd case the regions overlap
    memmove( psrc, pdst, sizeof( *psrc ) );

    return true;
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holder(const std::shared_ptr<Object> &pchr)
{
    /// @author ZZ
    /// @details This function keeps weapons near their holders

    CHR_REF iattached = pchr->attachedto;
    if ( _currentModule->getObjectHandler().exists( iattached ) )
    {
        Object * pattached = _currentModule->getObjectHandler().get( iattached );

        // Keep in hand weapons with iattached
        if ( chr_matrix_valid( pchr.get() ) )
        {
            pchr->setPosition(mat_getTranslate(pchr->inst.matrix));
        }
        else
        {
            //TODO: ZF> should this be the other way around?
            pchr->setPosition(pattached->getPosition());
        }

        pchr->ori.facing_z = pattached->ori.facing_z;

        // Copy this stuff ONLY if it's a weapon, not for mounts
        if ( pattached->transferblend && pchr->isitem )
        {

            // Items become partially invisible in hands of players
            if ( VALID_PLA( pattached->is_which_player ) && 255 != pattached->inst.alpha )
            {
                pchr->setAlpha(SEEINVISIBLE);
            }
            else
            {
                // Only if not naturally transparent
                if ( 255 == pchr->getProfile()->getAlpha() )
                {
                    pchr->setAlpha(pattached->inst.alpha);
                }
                else
                {
                    pchr->setAlpha(pchr->getProfile()->getAlpha());
                }
            }

            // Do light too
            if ( VALID_PLA( pattached->is_which_player ) && 255 != pattached->inst.light )
            {
                pchr->setLight(SEEINVISIBLE);
            }
            else
            {
                // Only if not naturally transparent
                if ( 255 == pchr->getProfile()->getLight())
                {
                    pchr->setLight(pattached->inst.light);
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
        pchr->attachedto = INVALID_CHR_REF;

        // Keep inventory with iattached
        if ( !_currentModule->getObjectHandler().exists( pchr->inwhich_inventory ) )
        {
            for(const std::shared_ptr<Object> pitem : pchr->getInventory().iterate())
            {
                pitem->setPosition(pchr->getPosition());

                // Copy olds to make SendMessageNear work
                pitem->pos_old = pchr->pos_old;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix( const CHR_REF ichr )
{
    /// @author ZZ
    /// @details This function sets one character's matrix

    Object * pchr;
    chr_instance_t * pinst;

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return;
    pchr = _currentModule->getObjectHandler().get( ichr );
    pinst = &( pchr->inst );

    // invalidate this matrix
    pinst->matrix_cache.matrix_valid = false;

    if ( pchr->is_overlay )
    {
        // This character is an overlay and its ai.target points to the object it is overlaying
        // Overlays are kept with their target...
        if ( _currentModule->getObjectHandler().exists( pchr->ai.target ) )
        {
            Object * ptarget = _currentModule->getObjectHandler().get( pchr->ai.target );

            pchr->setPosition(ptarget->getPosition());

            // copy the matrix
            pinst->matrix = ptarget->inst.matrix;

            // copy the matrix data
            pinst->matrix_cache = ptarget->inst.matrix_cache;
        }
    }
    else
    {
        if ( pchr->stickybutt )
        {
            mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(
				pinst->matrix,
                fvec3_t(pchr->fat, pchr->fat, pchr->fat),
                TO_TURN( pchr->ori.facing_z ),
                TO_TURN( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET ),
                TO_TURN( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET ),
                pchr->getPosition());
        }
        else
        {
            mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(
                pinst->matrix,
                fvec3_t(pchr->fat, pchr->fat, pchr->fat),
                TO_TURN( pchr->ori.facing_z ),
                TO_TURN( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET ),
                TO_TURN( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET ),
                pchr->getPosition());
        }

        pinst->matrix_cache.valid        = true;
        pinst->matrix_cache.matrix_valid = true;
        pinst->matrix_cache.type_bits    = MAT_CHARACTER;

        pinst->matrix_cache.self_scale[kX] = pchr->fat;
        pinst->matrix_cache.self_scale[kY] = pchr->fat;
        pinst->matrix_cache.self_scale[kZ] = pchr->fat;

        pinst->matrix_cache.rotate[kX] = CLIP_TO_16BITS( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET );
        pinst->matrix_cache.rotate[kY] = CLIP_TO_16BITS( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET );
        pinst->matrix_cache.rotate[kZ] = pchr->ori.facing_z;

        pinst->matrix_cache.pos = pchr->getPosition();
    }
}

//--------------------------------------------------------------------------------------------
#if 0
void update_all_character_matrices()
{
    /// @author ZZ
    /// @details This function makes all of the character's matrices

    // just call chr_update_matrix on every character
    for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
        chr_update_matrix( pchr.get(), true );
    }
}
#endif

//--------------------------------------------------------------------------------------------
void free_all_chraracters()
{
    /// @author ZZ
    /// @details This function resets the character allocation list

    //Remove all enchants
    for (ENC_REF ref = 0; ref < ENCHANTS_MAX; ++ref)
    {
        remove_enchant(ref, nullptr);
    }

    // free all the characters
    if(_currentModule) {
        _currentModule->getObjectHandler().clear();
    }

    // free_all_players
    PlaStack.count = 0;
    local_stats.player_count = 0;
    local_stats.noplayers = true;
}

//--------------------------------------------------------------------------------------------
float chr_get_mesh_pressure(Object *chr, const fvec3_t& pos)
{
    if (!chr)
    {
        return 0.0f;
    }

    if (CHR_INFINITE_WEIGHT == chr->phys.weight)
    {
        return 0.0f;
    }

    // Calculate the radius based on whether the character is on camera.
    float radius = 0.0f;
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() && !SDL_KEYDOWN(keyb, SDLK_F8))
    {
        ego_tile_info_t *tile = _currentModule->getMeshPointer()->get_ptile(chr->getTile());

        if (nullptr != tile && tile->inrenderlist)
        {
            radius = chr->bump_1.size;
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    float result = ego_mesh_t::get_pressure( _currentModule->getMeshPointer(), pos, radius, chr->stoppedby);
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;
    return result;
}

//--------------------------------------------------------------------------------------------
fvec3_t chr_get_mesh_diff(Object *chr, const fvec3_t& pos, float center_pressure)
{
    if (!chr)
    {
        return fvec3_t::zero();
    }

    if (CHR_INFINITE_WEIGHT == chr->phys.weight)
    {
        return fvec3_t::zero();
    }

    // Calculate the radius based on whether the character is on camera.
    float radius = 0.0f;
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() && !SDL_KEYDOWN(keyb, SDLK_F8))
    {
        ego_tile_info_t *tile = _currentModule->getMeshPointer()->get_ptile(chr->getTile());

        if (nullptr != tile && tile->inrenderlist)
        {
            radius = chr->bump_1.size;
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    fvec3_t result = ego_mesh_t::get_diff(_currentModule->getMeshPointer(), pos, radius, center_pressure, chr->stoppedby);
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;
    return result;
}

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
bool character_grab_stuff( const CHR_REF ichr_a, grip_offset_t grip_off, bool grab_people )
{
    /// @author ZZ
    /// @details This function makes the character pick up an item if there's one around

    const auto color_red = Ego::Math::Colour4f::parse(0xFF, 0x7F, 0x7F, 0xFF);
    const auto color_grn = Ego::Math::Colour4f::parse(0x7F, 0xFF, 0x7F, 0xFF);
    const auto color_blu = Ego::Math::Colour4f::parse(0x7F, 0x7F, 0xFF, 0xFF);
    const auto default_tint = Ego::Math::Colour4f::white();

    //Max search distance in quad tree relative to object position
    const float MAX_SEARCH_DIST = 3.0f * GRID_FSIZE;

    //Max grab distance is 2/3rds of a tile
    const float MAX_DIST_GRAB = GRID_FSIZE * 0.66f;

    CHR_REF   ichr_b;
    slot_t    slot;
    oct_vec_v2_t mids;
    fvec3_t   slot_pos;

    bool retval;

    // valid objects that can be grabbed
    size_t      grab_visible_count = 0;
    std::vector<grab_data_t> grabList;

    // valid objects that cannot be grabbed
    size_t      ungrab_visible_count = 0;
    std::vector<grab_data_t> ungrabList;

    const std::shared_ptr<Object> &pchr_a = _currentModule->getObjectHandler()[ichr_a];
    if (!pchr_a) return false;

    // find the slot from the grip
    slot = grip_offset_to_slot( grip_off );
    if ( slot >= SLOT_COUNT ) return false;

    // Make sure the character doesn't have something already, and that it has hands
    if ( _currentModule->getObjectHandler().exists( pchr_a->holdingwhich[slot] ) || !pchr_a->getProfile()->isSlotValid(slot) )
        return false;

    //Determine the position of the grip
    mids = pchr_a->slot_cv[slot].getMid();
    slot_pos[kX] = mids[OCT_X];
    slot_pos[kY] = mids[OCT_Y];
    slot_pos[kZ] = mids[OCT_Z];
	slot_pos += pchr_a->getPosition();

    // Go through all characters to find the best match
    std::vector<std::shared_ptr<Object>> nearbyObjects = _currentModule->getObjectHandler().findObjects(slot_pos[kX], slot_pos[kY], MAX_SEARCH_DIST);
    for(const std::shared_ptr<Object> &pchr_c : nearbyObjects)
    {
        grab_data_t grabData;
        bool canGrab = true;

        //Skip invalid objects
        if(pchr_c->isTerminated()) {
            continue;
        }

        // do nothing to yourself
        if (pchr_a == pchr_c) continue;

        // Dont do hidden objects
        if ( pchr_c->is_hidden ) continue;

        // pickpocket not allowed yet
        if ( _currentModule->getObjectHandler().exists( pchr_c->inwhich_inventory ) ) continue;

        // disarm not allowed yet
        if ( INVALID_CHR_REF != pchr_c->attachedto ) continue;

        // do not pick up your mount
        if ( pchr_c->holdingwhich[SLOT_LEFT] == ichr_a ||
             pchr_c->holdingwhich[SLOT_RIGHT] == ichr_a ) continue;

        // do not notice completely broken items?
        if ( pchr_c->isitem && !pchr_c->isAlive() ) continue;

        // reasonable carrying capacity
        if ( pchr_c->phys.weight > pchr_a->phys.weight + FLOAT_TO_FP8(pchr_a->getAttribute(Ego::Attribute::MIGHT)) * INV_FF )
        {
            canGrab = false;
        }

        // grab_people == true allows you to pick up living non-items
        // grab_people == false allows you to pick up living (functioning) items
        if ( !grab_people && !pchr_c->isitem )
        {
            canGrab = false;
        }

        // is the object visible
        grabData.visible = pchr_a->canSeeObject(pchr_c);

        // calculate the distance
        grabData.horizontalDistance = (pchr_c->getPosition() - slot_pos).length();
        grabData.verticalDistance = std::sqrt(Ego::Math::sq( pchr_a->getPosZ() - pchr_c->getPosZ()));
 
        //Figure out if the character is looking towards the object
        grabData.isFacingObject = pchr_a->isFacingLocation(pchr_c->getPosX(), pchr_c->getPosY());

        // Is it too far away to interact with?
        if (grabData.horizontalDistance > MAX_SEARCH_DIST || grabData.verticalDistance > MAX_SEARCH_DIST) continue;

        // visibility affects the max grab distance.
        // if it is not visible then we have to be touching it.
        float maxHorizontalGrabDistance = MAX_DIST_GRAB;
        if ( !grabData.visible )
        {
            maxHorizontalGrabDistance *= 0.5f;
        }

        //Halve grab distance for objects behind us
        if(!grabData.isFacingObject) {
            maxHorizontalGrabDistance *= 0.5f;
        }

        //Bigger characters have bigger grab size
        maxHorizontalGrabDistance += pchr_a->bump.size / 4.0f;

        // is it too far away to grab?
        if (grabData.horizontalDistance > maxHorizontalGrabDistance + pchr_a->bump.size / 4.0f && grabData.horizontalDistance > pchr_a->bump.size)
        {
            canGrab = false;
        }

        //Check vertical distance as well
        else
        {
            float maxVerticalGrabDistance = pchr_a->bump.height / 2.0f;

            if(grab_people)
            {
                //This allows very flat creatures like the Carpet Mimics grab people
                maxVerticalGrabDistance = std::max(maxVerticalGrabDistance, MAX_DIST_GRAB);
            }

            if (grabData.verticalDistance > maxVerticalGrabDistance)
            {
                canGrab = false;
            }
        }

        // count the number of objects that are within the max range
        // a difference between the *_total_count and the *_count
        // indicates that some objects were not detectable
        if ( grabData.visible )
        {
            if (canGrab)
            {
                grab_visible_count++;
            }
            else
            {
                ungrab_visible_count++;
            }
        }

        grabData.object = pchr_c;
        if (canGrab)
        {
            grabList.push_back(grabData);
        }
        else
        {
            ungrabList.push_back(grabData);
        }
    }

    // sort the grab list
    if (!grabList.empty())
    {
        std::sort(grabList.begin(), grabList.end(), 
            [](const grab_data_t &a, const grab_data_t &b)
            { 
                float distance = a.horizontalDistance - b.horizontalDistance;
                if(distance <= FLT_EPSILON)
                {
                    distance += a.verticalDistance - b.verticalDistance;
                }

                return distance < 0.0f;
            });
    }

    // try to grab something
    retval = false;
    if (grabList.empty() && ( 0 != grab_visible_count ) )
    {
        // There are items within the "normal" range that could be grabbed
        // but somehow they can't be seen.
        // Generate a billboard that tells the player what the problem is.
        // NOTE: this is not corerect since it could alert a player to an invisible object

        // 5 seconds and blue
        chr_make_text_billboard( ichr_a, "I can't feel anything...", color_blu, default_tint, 3, Billboard::Flags::Fade );

        retval = true;
    }

    if ( !retval )
    {
        for(const grab_data_t &grabData : grabList)
        {
            if (!grabData.visible) {
                continue;
            } 

            bool can_grab = can_grab_item_in_shop(ichr_a, grabData.object->getCharacterID());

            if ( can_grab )
            {
                // Stick 'em together and quit
                if ( rv_success == attach_character_to_mount(grabData.object->getCharacterID(), ichr_a, grip_off) )
                {
                    if (grab_people)
                    {
                        // Start the slam animation...  ( Be sure to drop!!! )
                        chr_play_action( pchr_a.get(), ACTION_MC + slot, false );
                    }
                    retval = true;
                }
                break;
            }
            else
            {
                // Lift the item a little and quit...
                grabData.object->vel[kZ] = DROPZVEL;
                grabData.object->hitready = true;
                SET_BIT( grabData.object->ai.alert, ALERTIF_DROPPED );
                break;
            }
        }
    }

    if ( !retval )
    {
        fvec3_t vforward;

        //---- generate billboards for things that players can interact with
        if (Ego::FeedbackType::None != egoboo_config_t::get().hud_feedback.getValue() && VALID_PLA(pchr_a->is_which_player))
        {
            // things that can be grabbed
            for(const grab_data_t &grabData : grabList)
            {
                ichr_b = grabData.object->getCharacterID();
                if (!grabData.visible)
                {
                    // (5 secs and blue)
                    chr_make_text_billboard( ichr_b, "Something...", color_blu, default_tint, 3, Billboard::Flags::Fade );
                }
                else
                {
                    // (5 secs and green)
                    chr_make_text_billboard( ichr_b, grabData.object->getName(true, false, true).c_str(), color_grn, default_tint, 3, Billboard::Flags::Fade );
                }
            }

            // things that can't be grabbed
            for(const grab_data_t &grabData : ungrabList)
            {
                ichr_b = grabData.object->getCharacterID();
                if (!grabData.visible)
                {
                    // (5 secs and blue)
                    chr_make_text_billboard( ichr_b, "Something...", color_blu, default_tint, 3, Billboard::Flags::Fade );
                }
                else
                {
                    // (5 secs and red)
                    chr_make_text_billboard( ichr_b, grabData.object->getName(true, false, true).c_str(), color_red, default_tint, 3, Billboard::Flags::Fade );
                }
            }
        }

        //---- if you can't grab anything, activate something using ALERTIF_BUMPED
        if ( VALID_PLA( pchr_a->is_which_player ) && !ungrabList.empty() )
        {
            // sort the ungrab list
            std::sort(ungrabList.begin(), ungrabList.end(), 
                [](const grab_data_t &a, const grab_data_t &b)
                { 
                    //If objects are basically on top of each other, then sort by vertical distance
                    if(std::abs(a.horizontalDistance - b.horizontalDistance) <= FLT_EPSILON) {
                        return a.verticalDistance < b.verticalDistance;
                    }

                    return a.horizontalDistance < b.horizontalDistance;
                });

            for(const grab_data_t &grabData : ungrabList)
            {
                // only do visible objects
                if (!grabData.visible) continue;

                // only bump the closest character that is in front of the character
                // (ignore vertical displacement)
                if (grabData.isFacingObject && grabData.horizontalDistance < MAX_DIST_GRAB)
                {
                    ai_state_set_bumplast(grabData.object->ai, ichr_a);
                    break;
                }
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void character_swipe( const CHR_REF ichr, slot_t slot )
{
    /// @author ZZ
    /// @details This function spawns an attack particle
    int    spawn_vrt_offset;
    int    action;
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
        action = pchr->inst.action_which;
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
        std::shared_ptr<Object> pthrown = _currentModule->spawnObject(pchr->getPosition(), pweapon->profile_ref, chr_get_iteam( iholder ), 0, pchr->ori.facing_z, pweapon->Name, INVALID_CHR_REF);
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
                        fvec3_t tmp_pos = particle->getPosition();

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
                        particle->damage.base += pweapon->damage_boost;

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

	fvec3_t loc_pos = pchr->getPosition();

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

    return RandomName::exportName(_currentModule->getObjectHandler().get(character)->Name, szSaveName);
}

//--------------------------------------------------------------------------------------------
bool chr_download_profile(Object * pchr, const std::shared_ptr<ObjectProfile> &profile)
{
    /// @author BB
    /// @details grab all of the data from the data.txt file

    int iTmp, tnc;

    if (nullptr == ( pchr ) || !profile ) return false;

    // Set up model stuff
    pchr->stoppedby = profile->getStoppedByMask();
    pchr->nameknown = profile->isNameKnown();
    pchr->ammoknown = profile->isNameKnown();
    pchr->draw_icon = profile->isDrawIcon();

    // calculate a base kurse state. this may be overridden later
    if ( profile->isItem() )
    {
        pchr->iskursed = Random::getPercent() <= profile->getKurseChance();
    }

    // Starting Perks
    for(size_t i = 0; i < Ego::Perks::NR_OF_PERKS; ++i) {
        Ego::Perks::PerkID id = static_cast<Ego::Perks::PerkID>(i);
        if(profile->beginsWithPerk(id)) {
            pchr->addPerk(id);
        }
    }

    pchr->darkvision_level = 0;
    if(pchr->hasPerk(Ego::Perks::NIGHT_VISION)) {
        pchr->darkvision_level += 1;
    }
    pchr->see_invisible_level = profile->canSeeInvisible();

    // Ammo
    pchr->ammomax = profile->getMaxAmmo();
    pchr->ammo = profile->getAmmo();

    // Gender
    pchr->gender = profile->getGender();
    if ( pchr->gender == GENDER_RANDOM )
    {
        //50% male or female
        if(Random::nextBool())
        {
            pchr->gender = GENDER_FEMALE;
        }
        else
        {
            pchr->gender = GENDER_MALE;
        }
    }

    // Life and Mana bars
    pchr->life_color = profile->getLifeColor();
    pchr->mana_color = profile->getManaColor();

    // Skin
    pchr->skin = profile->getSkinOverride();

    // Resistances
    const SkinInfo &skin = profile->getSkinInfo(pchr->skin);
    pchr->defense = skin.defence;
    pchr->damagetarget_damagetype = profile->getDamageTargetType();
    for ( tnc = 0; tnc < DAMAGE_COUNT; tnc++ )
    {
        pchr->damage_modifier[tnc] = skin.damageModifier[tnc];
        pchr->damage_resistance[tnc] = skin.damageResistance[tnc];
    }

    // Flags
    pchr->stickybutt      = profile->hasStickyButt();
    pchr->openstuff       = profile->canOpenStuff();
    pchr->transferblend   = profile->transferBlending();
    pchr->waterwalk       = profile->canWalkOnWater();
    pchr->platform        = profile->isPlatform();
    pchr->canuseplatforms = profile->canUsePlatforms();
    pchr->isitem          = profile->isItem();
    pchr->invictus        = profile->isInvincible();
    pchr->cangrabmoney    = profile->canGrabMoney();

    // Jumping
    pchr->jump_power = profile->getJumpPower();
    pchr->jumpnumberreset = profile->getJumpNumber();

    // Other junk
    pchr->flyheight   = profile->getFlyHeight();
    pchr->maxaccel    = pchr->maxaccel_reset = skin.maxAccel;
    pchr->flashand    = profile->getFlashAND();
    pchr->phys.dampen = profile->getBounciness();

    // Clamp life to [1,life_max] and mana to [0,life_max]. This may be overridden later
	pchr->life = CLIP(profile->getSpawnLife(), UINT_TO_UFP8(1), FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::MAX_LIFE)));
	pchr->mana = CLIP(profile->getSpawnMana(), UINT_TO_UFP8(0), FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::MAX_MANA)));

    pchr->phys.bumpdampen = profile->getBumpDampen();
    if ( CAP_INFINITE_WEIGHT == profile->getWeight() )
    {
        pchr->phys.weight = CHR_INFINITE_WEIGHT;
    }
    else
    {
        Uint32 itmp = profile->getWeight() * profile->getSize() * profile->getSize() * profile->getSize();
        pchr->phys.weight = std::min( itmp, CHR_MAX_WEIGHT );
    }

    // Image rendering
    pchr->uoffvel = profile->getTextureMovementRateX();
    pchr->voffvel = profile->getTextureMovementRateY();

    // Movement
    pchr->anim_speed_sneak = profile->getSneakAnimationSpeed();
    pchr->anim_speed_walk = profile->getWalkAnimationSpeed();
    pchr->anim_speed_run = profile->getRunAnimationSpeed();

    // Money is added later
    pchr->money = profile->getStartingMoney();

    // Experience
    iTmp = Random::next( profile->getStartingExperience() );
    pchr->experience      = std::min( iTmp, MAXXP );
    pchr->experiencelevel = profile->getStartingLevel();

    // Particle attachments
    pchr->reaffirm_damagetype = profile->getReaffirmDamageType();

    // Character size and bumping
    chr_init_size(pchr, profile);

    return true;
}

//--------------------------------------------------------------------------------------------
void cleanup_one_character( Object * pchr )
{
    /// @author BB
    /// @details Everything necessary to disconnect one character from the game

    CHR_REF ichr = pchr->getCharacterID();

    pchr->sparkle = NOSPARKLE;

    // Remove it from the team
    pchr->team = pchr->team_base;
    _currentModule->getTeamList()[pchr->team].decreaseMorale();

    if ( _currentModule->getTeamList()[pchr->team].getLeader().get() == pchr )
    {
        // The team now has no leader if the character is the leader
        _currentModule->getTeamList()[pchr->team].setLeader(Object::INVALID_OBJECT);
    }

    // Clear all shop passages that it owned..
    _currentModule->removeShopOwner(ichr);

    // detach from any mount
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        pchr->detatchFromHolder(true, false);
    }

    // drop your left item
    const std::shared_ptr<Object> &leftItem = pchr->getLeftHandItem();
    if(leftItem && leftItem->isItem()) {
        leftItem->detatchFromHolder(true, false);
    }

    // drop your right item
    const std::shared_ptr<Object> &rightItem = pchr->getRightHandItem();
    if(rightItem && rightItem->isItem()) {
        rightItem->detatchFromHolder(true, false);
    }

    // start with a clean list
    cleanup_character_enchants( pchr );

    // remove enchants from the character
    if ( pchr->life >= 0 )
    {
        disenchant_character( ichr );
    }
    else
    {
        std::shared_ptr<eve_t> peve;
        ENC_REF ienc_now, ienc_nxt;
        size_t  ienc_count;

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // remove all invalid enchants
        ienc_now = pchr->firstenchant;
        ienc_count = 0;
        while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
        {
            ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

            peve = enc_get_peve( ienc_now );
            if ( NULL != peve && !peve->_target._stay )
            {
                remove_enchant( ienc_now, NULL );
            }

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
    }

    // Stop all sound loops for this object
    AudioSystem::get().stopObjectLoopingSounds(ichr);
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
//--------------------------------------------------------------------------------------------
Object * chr_config_do_init( Object * pchr )
{
    CHR_REF  ichr;
    TEAM_REF loc_team;
    int      tnc, iteam, kursechance;

    chr_spawn_data_t * spawn_ptr;
    fvec3_t pos_tmp;

    if ( NULL == pchr ) return NULL;
    ichr = GET_INDEX_PCHR( pchr );
    spawn_ptr = &( pchr->spawn_data );

    // get a pointer to the character profile
    std::shared_ptr<ObjectProfile> ppro = ProfileSystem::get().getProfile(spawn_ptr->profile);
    if ( NULL == ppro )
    {
        log_debug( "chr_config_do_init() - cannot initialize character.\n" );

        return NULL;
    }

    // make a copy of the data in spawn_ptr->pos
    pos_tmp = spawn_ptr->pos;

    // download all the values from the character spawn_ptr->profile
    chr_download_profile( pchr, ppro );

    // Make sure the spawn_ptr->team is valid
    loc_team = spawn_ptr->team;
    iteam = REF_TO_INT( loc_team );
    iteam = CLIP( iteam, 0, (int)Team::TEAM_MAX );
    loc_team = ( TEAM_REF )iteam;

    // IMPORTANT!!!
    pchr->missilehandler = ichr;

    // Set up model stuff
    pchr->profile_ref   = spawn_ptr->profile;
    pchr->basemodel_ref = spawn_ptr->profile;

    // Kurse state
    if ( ppro->isItem() )
    {
        kursechance = ppro->getKurseChance();
        if (egoboo_config_t::get().game_difficulty.getValue() >= Ego::GameDifficulty::Hard)
        {
            kursechance *= 2.0f;  // Hard mode doubles chance for Kurses
        }
        if (egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Normal && kursechance != 100)
        {
            kursechance *= 0.5f;  // Easy mode halves chance for Kurses
        }
        pchr->iskursed = Random::getPercent() <= kursechance;
    }

    // AI stuff
    ai_state_spawn( &( pchr->ai ), ichr, pchr->profile_ref, _currentModule->getTeamList()[loc_team].getMorale() );

    // Team stuff
    pchr->team     = loc_team;
    pchr->team_base = loc_team;
    if ( !pchr->isInvincible() )  _currentModule->getTeamList()[loc_team].increaseMorale();

    // Firstborn becomes the leader
    if ( !_currentModule->getTeamList()[loc_team].getLeader() )
    {
        _currentModule->getTeamList()[loc_team].setLeader(_currentModule->getObjectHandler()[ichr]);
    }

    // Heal the spawn_ptr->skin, if needed
    if (spawn_ptr->skin < 0 || ppro->getSkinOverride() != ObjectProfile::NO_SKIN_OVERRIDE)
    {
        spawn_ptr->skin = ppro->getSkinOverride();
    }

    // cap_get_skin_overide() can return NO_SKIN_OVERRIDE or SKINS_PEROBJECT_MAX, so we need to check
    // for the "random skin marker" even if that function is called
    if (spawn_ptr->skin == ObjectProfile::NO_SKIN_OVERRIDE)
    {
        // This is a "random" skin.
        // Force it to some specific value so it will go back to the same skin every respawn
        // We are now ensuring that there are skin graphics for all skins, so there
        // is no need to count the skin graphics loaded into the profile.
        // Limiting the available skins to ones that had unique graphics may have been a mistake since
        // the skin-dependent properties in data.txt may exist even if there are no unique graphics.
        spawn_ptr->skin = ppro->getRandomSkinID();
    }

    // actually set the character skin
    pchr->skin = spawn_ptr->skin;

    // fix the spawn_ptr->skin-related parameters, in case there was some funny business with overriding
    // the spawn_ptr->skin from the data.txt file
    {
        const SkinInfo &skin = ppro->getSkinInfo(pchr->skin);
        pchr->defense = skin.defence;
        for ( tnc = 0; tnc < DAMAGE_COUNT; tnc++ )
        {
            pchr->damage_modifier[tnc] = skin.damageModifier[tnc];
            pchr->damage_resistance[tnc] = skin.damageResistance[tnc];
        }

        chr_set_maxaccel( pchr, skin.maxAccel );
    }

    // override the default behavior for an "easy" game
    if (egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Normal)
    {
        pchr->life = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::MAX_LIFE));
        pchr->mana = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::MAX_LIFE));
    }

    // Character size and bumping
    pchr->fat_goto      = pchr->fat;
    pchr->fat_goto_time = 0;

    // grab all of the environment information
    move_one_character_get_environment( pchr );

    pchr->setPosition(pos_tmp);

    pchr->pos_stt  = pos_tmp;
    pchr->pos_old  = pos_tmp;

    pchr->ori.facing_z     = spawn_ptr->facing;
    pchr->ori_old.facing_z = pchr->ori.facing_z;

    // Name the character
    if ( CSTR_END == spawn_ptr->name[0] )
    {
        // Generate a random spawn_ptr->name
        snprintf( pchr->Name, SDL_arraysize( pchr->Name ), "%s", ppro->generateRandomName().c_str() );
    }
    else
    {
        // A spawn_ptr->name has been given
        tnc = 0;

        while ( spawn_ptr->name[tnc] != '\0' ) //ZF> TODO: dangerous copy here! no bounds check
        {
            pchr->Name[tnc] = spawn_ptr->name[tnc];
            tnc++;
        }

        pchr->Name[tnc] = CSTR_END;
    }

    // initalize the character instance
    chr_instance_t::spawn(pchr->inst, spawn_ptr->profile, spawn_ptr->skin);
    chr_update_matrix( pchr, true );

    // Particle attachments
    for ( tnc = 0; tnc < ppro->getAttachedParticleAmount(); tnc++ )
    {
        ParticleHandler::get().spawnParticle( pchr->getPosition(), pchr->ori.facing_z, ppro->getSlotNumber(), ppro->getAttachedParticleProfile(),
                            ichr, GRIP_LAST + tnc, pchr->team, ichr, INVALID_PRT_REF, tnc);
    }

    // is the object part of a shop's inventory?
    if ( pchr->isitem )
    {
        // Items that are spawned inside shop passages are more expensive than normal

        CHR_REF shopOwner = _currentModule->getShopOwner(pchr->getPosX(), pchr->getPosY());
        if(shopOwner != Passage::SHOP_NOOWNER) {
            pchr->isshopitem = true;               // Full value
            pchr->iskursed   = false;              // Shop items are never kursed
            pchr->nameknown  = true;               // identified
        }
        else {
            pchr->isshopitem = false;
        }
    }

    /// @author ZF
    /// @details override the shopitem flag if the item is known to be valuable
    /// @author BB
    /// @details this prevents (essentially) all books from being able to be burned
    //if ( pcap->isvaluable )
    //{
    //    pchr->isshopitem = true;
    //}

    // determine whether the object is hidden
    chr_update_hide( pchr );

    chr_instance_t::update_ref(pchr->inst, pchr->enviro.grid_level, true );

#if defined(_DEBUG) && defined(DEBUG_WAYPOINTS)
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) && CHR_INFINITE_WEIGHT != pchr->phys.weight && !pchr->safe_valid )
    {
        log_warning( "spawn_one_character() - \n\tinitial spawn position <%f,%f> is \"inside\" a wall. Wall normal is <%f,%f>\n",
                     pchr->getPosX(), pchr->getPosY(), nrm[kX], nrm[kY] );
    }
#endif

    return pchr;
}

//--------------------------------------------------------------------------------------------
int chr_change_skin( const CHR_REF character, const SKIN_T skin )
{
    TX_REF new_texture = (TX_REF)TX_WATER_TOP;

	if (!_currentModule->getObjectHandler().exists(character)) {
		return 0;
	}
	Object *pchr = _currentModule->getObjectHandler().get(character);
	chr_instance_t& pinst = pchr->inst;

	const std::shared_ptr<Ego::ModelDescriptor> &model = pchr->getProfile()->getModel();

    // make sure that the instance has a valid imad
    if (!pinst.imad) {
        if (chr_instance_t::set_mad(pinst, model)) {
            chr_update_collision_size(pchr, true);
        }
    }

    // the normal thing to happen
    new_texture = pchr->getProfile()->getSkin(skin);
    pchr->skin = skin;

    chr_instance_t::set_texture(pinst, new_texture);

    return pchr->skin;
}

//--------------------------------------------------------------------------------------------
int change_armor( const CHR_REF character, const SKIN_T skin )
{
    /// @author ZZ
    /// @details This function changes the armor of the character

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;
    int     loc_skin = skin;

    int     iTmp;
    Object * pchr;

    if ( !_currentModule->getObjectHandler().exists( character ) ) return 0;
    pchr = _currentModule->getObjectHandler().get( character );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Remove armor enchantments
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
    {
        ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

        enc_remove_set(ienc_now, eve_t::SETSLASHMODIFIER);
        enc_remove_set(ienc_now, eve_t::SETCRUSHMODIFIER);
        enc_remove_set(ienc_now, eve_t::SETPOKEMODIFIER);
        enc_remove_set(ienc_now, eve_t::SETHOLYMODIFIER);
        enc_remove_set(ienc_now, eve_t::SETEVILMODIFIER);
        enc_remove_set(ienc_now, eve_t::SETFIREMODIFIER);
        enc_remove_set(ienc_now, eve_t::SETICEMODIFIER);
        enc_remove_set(ienc_now, eve_t::SETZAPMODIFIER);

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    // Change the skin
    std::shared_ptr<ObjectProfile> profile = ProfileSystem::get().getProfile(pchr->profile_ref);
    loc_skin = chr_change_skin( character, loc_skin );

    const SkinInfo &skinInfo = profile->getSkinInfo(loc_skin);

    // Change stats associated with skin
    pchr->defense = skinInfo.defence;

    for ( iTmp = 0; iTmp < DAMAGE_COUNT; iTmp++ )
    {
        pchr->damage_modifier[iTmp] = skinInfo.damageModifier[iTmp];
        pchr->damage_resistance[iTmp] = skinInfo.damageResistance[iTmp];
    }

    // set the character's maximum acceleration
    chr_set_maxaccel( pchr, skinInfo.maxAccel );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Reset armor enchantments
    /// @todo These should really be done in reverse order ( Start with last enchant ), but
    /// I don't care at this point !!!BAD!!!
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
    {
        PRO_REF ipro = enc_get_ipro( ienc_now );

        ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

        if (ProfileSystem::get().isValidProfileID(ipro))
        {
            EVE_REF ieve = ProfileSystem::get().pro_get_ieve(ipro);

            enc_apply_set(ienc_now, eve_t::SETSLASHMODIFIER, ipro);
            enc_apply_set(ienc_now, eve_t::SETCRUSHMODIFIER, ipro);
            enc_apply_set(ienc_now, eve_t::SETPOKEMODIFIER, ipro);
            enc_apply_set(ienc_now, eve_t::SETHOLYMODIFIER, ipro);
            enc_apply_set(ienc_now, eve_t::SETEVILMODIFIER, ipro);
            enc_apply_set(ienc_now, eve_t::SETFIREMODIFIER, ipro);
            enc_apply_set(ienc_now, eve_t::SETICEMODIFIER, ipro);
            enc_apply_set(ienc_now, eve_t::SETZAPMODIFIER, ipro);

            enc_apply_add(ienc_now, eve_t::ADDACCEL, ieve);
            enc_apply_add(ienc_now, eve_t::ADDDEFENSE, ieve);
            enc_apply_add(ienc_now, eve_t::ADDSLASHRESIST, ieve);
            enc_apply_add(ienc_now, eve_t::ADDCRUSHRESIST, ieve);
            enc_apply_add(ienc_now, eve_t::ADDPOKERESIST, ieve);
            enc_apply_add(ienc_now, eve_t::ADDHOLYRESIST, ieve);
            enc_apply_add(ienc_now, eve_t::ADDEVILRESIST, ieve);
            enc_apply_add(ienc_now, eve_t::ADDFIRERESIST, ieve);
            enc_apply_add(ienc_now, eve_t::ADDICERESIST, ieve);
            enc_apply_add(ienc_now, eve_t::ADDZAPRESIST, ieve);
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    return loc_skin;
}

//--------------------------------------------------------------------------------------------
void change_character_full( const CHR_REF ichr, const PRO_REF profile, const int skin, const Uint8 leavewhich )
{
    /// @author ZF
    /// @details This function polymorphs a character permanently so that it can be exported properly
    /// A character turned into a frog with this function will also export as a frog!

    const std::shared_ptr<ObjectProfile>& newProfile = ProfileSystem::get().getProfile(profile);
    if (!newProfile) return;

    // copy the new name
    //strncpy( MadStack.lst[imad_old].name, MadStack.lst[imad_new].name, SDL_arraysize( MadStack.lst[imad_old].name ) );

    // change their model
    change_character( ichr, profile, skin, leavewhich );

    // set the base model to the new model, too
    _currentModule->getObjectHandler().get(ichr)->basemodel_ref = profile;
}

//--------------------------------------------------------------------------------------------
void change_character( const CHR_REF ichr, const PRO_REF profile_new, const int skin, const Uint8 leavewhich )
{
    /// @author ZZ
    /// @details This function polymorphs a character, changing stats, dropping weapons
    CHR_REF item;
    Object * pchr;

    int old_attached_prt_count, new_attached_prt_count;

    if (!ProfileSystem::get().isValidProfileID(profile_new) || !_currentModule->getObjectHandler().exists(ichr)) return;
    pchr = _currentModule->getObjectHandler().get( ichr );

    old_attached_prt_count = number_of_attached_particles( ichr );

    std::shared_ptr<ObjectProfile> newProfile = ProfileSystem::get().getProfile(profile_new);
    if(!newProfile) {
        return;
    }

    // Drop left weapon
    const std::shared_ptr<Object> &leftItem = pchr->getLeftHandItem();
    if ( leftItem && ( !newProfile->isSlotValid(SLOT_LEFT) || newProfile->isMount() ) )
    {
        leftItem->detatchFromHolder(true, true);
        detach_character_from_platform(leftItem.get());

        if ( pchr->isMount() )
        {
            leftItem->vel[kZ]    = DISMOUNTZVEL;
            leftItem->jump_timer = JUMPDELAY;
            leftItem->movePosition(0.0f, 0.0f, DISMOUNTZVEL);
        }
    }

    // Drop right weapon
    const std::shared_ptr<Object> &rightItem = pchr->getRightHandItem();
    if ( rightItem && !newProfile->isSlotValid(SLOT_RIGHT) )
    {
        rightItem->detatchFromHolder(true, true);
        detach_character_from_platform(rightItem.get());

        if ( pchr->isMount() )
        {
            rightItem->vel[kZ]    = DISMOUNTZVEL;
            rightItem->jump_timer = JUMPDELAY;
            rightItem->movePosition(0.0f, 0.0f, DISMOUNTZVEL);
        }
    }

    // Remove particles
    disaffirm_attached_particles( ichr );

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pchr );

    // Remove enchantments
    if ( leavewhich == ENC_LEAVE_FIRST )
    {
        // Remove all enchantments except top one
        if ( INVALID_ENC_REF != pchr->firstenchant )
        {
            ENC_REF ienc_now, ienc_nxt;
            size_t  ienc_count;

            ienc_now = EnchantHandler::get().get_ptr(pchr->firstenchant)->nextenchant_ref;
            ienc_count = 0;
            while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
            {
                ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;

                remove_enchant( ienc_now, NULL );

                ienc_now = ienc_nxt;
                ienc_count++;
            }
            if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
        }
    }
    else if ( ENC_LEAVE_NONE == leavewhich )
    {
        // Remove all enchantments
        disenchant_character( ichr );
    }

    // Stuff that must be set
    pchr->profile_ref  = profile_new;
    pchr->stoppedby = newProfile->getStoppedByMask();

    // Ammo
    pchr->ammomax = newProfile->getMaxAmmo();
    pchr->ammo    = newProfile->getAmmo();

    // Gender
    if ( newProfile->getGender() != GENDER_RANDOM )  // GENDER_RANDOM means keep old gender
    {
        pchr->gender = newProfile->getGender();
    }

    // AI stuff
    chr_set_ai_state( pchr, 0 );
    pchr->ai.timer          = 0;
    pchr->turnmode          = TURNMODE_VELOCITY;

    pchr->latch.clear();

    // Flags
    pchr->stickybutt      = newProfile->hasStickyButt();
    pchr->openstuff       = newProfile->canOpenStuff();
    pchr->transferblend   = newProfile->transferBlending();
    pchr->platform        = newProfile->isPlatform();
    pchr->canuseplatforms = newProfile->canUsePlatforms();
    pchr->isitem          = newProfile->isItem();
    pchr->invictus        = newProfile->isInvincible();
    pchr->cangrabmoney    = newProfile->canGrabMoney();
    pchr->jump_timer      = JUMPDELAY;

    // change the skillz, too, jack!
    pchr->darkvision_level = 0; 
    if(pchr->hasPerk(Ego::Perks::NIGHT_VISION)) {
        pchr->darkvision_level += 1;        
    }
    pchr->see_invisible_level = newProfile->canSeeInvisible();

    /// @note BB@> changing this could be disasterous, in case you can't un-morph youself???
    /// @note ZF@> No, we want this, I have specifically scripted morph books to handle unmorphing
    /// even if you cannot cast arcane spells. Some morph spells specifically morph the player
    /// into a fighter or a tech user, but as a balancing factor prevents other spellcasting.
    // pchr->canusearcane          = pcap_new->canusearcane;

    // Character size and bumping
    // set the character size so that the new model is the same size as the old model
    // the model will then morph its size to the correct size over time
    {
        float old_fat = pchr->fat;
        float new_fat;

        if ( 0.0f == pchr->bump.size )
        {
            new_fat = newProfile->getSize();
        }
        else
        {
            new_fat = ( newProfile->getBumpSize() * newProfile->getSize() ) / pchr->bump.size;
        }

        // Spellbooks should stay the same size, even if their spell effect cause changes in size
        if ( pchr->profile_ref == SPELLBOOK ) new_fat = old_fat = 1.00f;

        // copy all the cap size info over, as normal
        chr_init_size( pchr, newProfile );

        // make the model's size congruent
        if ( 0.0f != new_fat && new_fat != old_fat )
        {
            pchr->setFat(new_fat);
            pchr->fat_goto      = old_fat;
            pchr->fat_goto_time = SIZETIME;
        }
        else
        {
            pchr->setFat(old_fat);
            pchr->fat_goto      = old_fat;
            pchr->fat_goto_time = 0;
        }
    }

    //Physics
    pchr->phys.bumpdampen = newProfile->getBumpDampen();

    if ( CAP_INFINITE_WEIGHT == newProfile->getWeight() )
    {
        pchr->phys.weight = CHR_INFINITE_WEIGHT;
    }
    else
    {
        Uint32 itmp = newProfile->getWeight() * pchr->fat * pchr->fat * pchr->fat;
        pchr->phys.weight = std::min( itmp, CHR_MAX_WEIGHT );
    }

    // Image rendering
    pchr->uoffvel = newProfile->getTextureMovementRateX();
    pchr->voffvel = newProfile->getTextureMovementRateY();

    // Movement
    pchr->anim_speed_sneak = newProfile->getSneakAnimationSpeed();
    pchr->anim_speed_walk  = newProfile->getWalkAnimationSpeed();
    pchr->anim_speed_run   = newProfile->getRunAnimationSpeed();

    // initialize the instance
    chr_instance_t::spawn(pchr->inst, profile_new, skin);
    chr_update_matrix( pchr, true );

    // Action stuff that must be down after chr_instance_spawn()
    chr_instance_t::set_action_ready(pchr->inst, false);
    chr_instance_t::set_action_keep(pchr->inst, false);
    chr_instance_t::set_action_loop(pchr->inst, false);
    if ( pchr->alive )
    {
        chr_play_action( pchr, ACTION_DA, false );
    }
    else
    {
        chr_play_action( pchr, Random::next((int)ACTION_KA, ACTION_KA + 3), false );
        chr_instance_t::set_action_keep(pchr->inst, true);
    }

    // Set the skin after changing the model in chr_instance_spawn()
    change_armor( ichr, skin );

    // Must set the wepon grip AFTER the model is changed in chr_instance_spawn()
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        set_weapongrip( ichr, pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );
    }

    item = pchr->holdingwhich[SLOT_LEFT];
    if ( _currentModule->getObjectHandler().exists( item ) )
    {
        EGOBOO_ASSERT( _currentModule->getObjectHandler().get(item)->attachedto == ichr );
        set_weapongrip( item, ichr, GRIP_LEFT );
    }

    item = pchr->holdingwhich[SLOT_RIGHT];
    if ( _currentModule->getObjectHandler().exists( item ) )
    {
        EGOBOO_ASSERT( _currentModule->getObjectHandler().get(item)->attachedto == ichr );
        set_weapongrip( item, ichr, GRIP_RIGHT );
    }

    // determine whether the object is hidden
    chr_update_hide( pchr );

    // Reaffirm them particles...
    pchr->reaffirm_damagetype = newProfile->getReaffirmDamageType();

    /// @note ZF@> so that books dont burn when dropped
    //reaffirm_attached_particles( ichr );

    new_attached_prt_count = number_of_attached_particles( ichr );

    ai_state_set_changed(pchr->ai);

    chr_instance_t::update_ref(pchr->inst, pchr->enviro.grid_level, true );
}

//--------------------------------------------------------------------------------------------
void switch_team_base( const CHR_REF character, const TEAM_REF team_new, const bool permanent )
{
    Object  * pchr;
    bool   can_have_team;

    if ( !_currentModule->getObjectHandler().exists( character ) ) return;
    pchr = _currentModule->getObjectHandler().get( character );

    // do we count this character as being on a team?
    can_have_team = !pchr->isitem && pchr->alive && !pchr->invictus;

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
    Object * pchr;

    // change the base object
    switch_team_base( character, team, true );

    // grab a pointer to the character
    if ( !_currentModule->getObjectHandler().exists( character ) ) return;
    pchr = _currentModule->getObjectHandler().get( character );

    // change our mount team as well
    tmp_ref = pchr->attachedto;
    if ( VALID_CHR_RANGE( tmp_ref ) )
    {
        switch_team_base( tmp_ref, team, false );
    }

    // update the team of anything we are holding as well
    tmp_ref = pchr->holdingwhich[SLOT_LEFT];
    if ( VALID_CHR_RANGE( tmp_ref ) )
    {
        switch_team_base( tmp_ref, team, false );
    }

    tmp_ref = pchr->holdingwhich[SLOT_RIGHT];
    if ( VALID_CHR_RANGE( tmp_ref ) )
    {
        switch_team_base( tmp_ref, team, false );
    }
}

//--------------------------------------------------------------------------------------------
void issue_clean( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function issues a clean up order to all teammates

    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[character];

    if ( !pchr ) return;

    for(const std::shared_ptr<Object> &listener : _currentModule->getObjectHandler().iterator())
    {
        if ( pchr->getTeam() != listener->getTeam() ) continue;

        if ( !listener->isAlive() )
        {
            listener->ai.timer  = update_wld + 2;  // Don't let it think too much...
        }

        SET_BIT( listener->ai.alert, ALERTIF_CLEANEDUP );
    }
}

//--------------------------------------------------------------------------------------------
int restock_ammo( const CHR_REF character, IDSZ idsz )
{
    /// @author ZZ
    /// @details This function restocks the characters ammo, if it needs ammo and if
    ///    either its parent or type idsz match the given idsz.  This
    ///    function returns the amount of ammo given.

    int amount;

    Object * pchr;

    if ( !_currentModule->getObjectHandler().exists( character ) ) return 0;
    pchr = _currentModule->getObjectHandler().get( character );

    amount = 0;
    if ( chr_is_type_idsz( character, idsz ) )
    {
        if ( _currentModule->getObjectHandler().get(character)->ammo < _currentModule->getObjectHandler().get(character)->ammomax )
        {
            amount = _currentModule->getObjectHandler().get(character)->ammomax - _currentModule->getObjectHandler().get(character)->ammo;
            _currentModule->getObjectHandler().get(character)->ammo = _currentModule->getObjectHandler().get(character)->ammomax;
        }
    }

    return amount;
}

//--------------------------------------------------------------------------------------------
bool chr_get_skill( Object *pchr, IDSZ whichskill )
{
    //Maps between old IDSZ skill system and new Perk system
    IDSZ_node_t *pskill;

    if ( !ACTIVE_PCHR( pchr ) ) return false;

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
//--------------------------------------------------------------------------------------------
bool update_chr_darkvision( const CHR_REF character )
{
    /// @author BB
    /// @details as an offset to negative status effects like things like poisoning, a
    ///               character gains darkvision ability the more they are "poisoned".
    ///               True poisoning can be removed by [HEAL] and tints the character green

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;

    std::shared_ptr<eve_t> peve;
    int life_regen = 0;

    Object * pchr;

    if ( !_currentModule->getObjectHandler().exists( character ) ) return false;
    pchr = _currentModule->getObjectHandler().get( character );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // grab the life loss due poison to determine how much darkvision a character has earned, he he he!
    // clean up the enchant list before doing anything
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while ( VALID_ENC_RANGE( ienc_now ) && ( ienc_count < ENCHANTS_MAX ) )
    {
        ienc_nxt = EnchantHandler::get().get_ptr(ienc_now)->nextenchant_ref;
        peve = enc_get_peve( ienc_now );

        //Is it true poison?
        if ( NULL != peve && MAKE_IDSZ( 'H', 'E', 'A', 'L' ) == peve->removedByIDSZ )
        {
            life_regen += EnchantHandler::get().get_ptr(ienc_now)->target_life;
            if (EnchantHandler::get().get_ptr(ienc_now)->owner_ref == pchr->ai.index ) life_regen += EnchantHandler::get().get_ptr(ienc_now)->owner_life;
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= ENCHANTS_MAX ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    if ( life_regen < 0 )
    {
        int tmp_level  = ( 0 == pchr->getAttribute(Ego::Attribute::MAX_LIFE) ) ? 0 : ( 10 * -life_regen ) / FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::MAX_LIFE));                      // Darkvision gained by poison

        //Use the better of the two darkvision abilities
        pchr->darkvision_level = tmp_level;

        //Nightvision skill
        if(pchr->hasPerk(Ego::Perks::NIGHT_VISION)) {
            pchr->darkvision_level += 1;
        }
    }

    return true;
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

        object->update();
    }

    // fix the stat timer
    if ( clock_chr_stat >= ONESECOND )
    {
        // Reset the clock
        clock_chr_stat -= ONESECOND;
    }

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void move_one_character_get_environment( Object * pchr )
{

    float   grid_level, water_level;
    Object * pplatform = NULL;

    chr_environment_t * penviro;

    if ( !ACTIVE_PCHR( pchr ) ) return;
    penviro = &( pchr->enviro );

    // determine if the character is standing on a platform
    pplatform = NULL;
    if ( _currentModule->getObjectHandler().exists( pchr->onwhichplatform_ref ) )
    {
        pplatform = _currentModule->getObjectHandler().get( pchr->onwhichplatform_ref );
    }

    //---- character "floor" level
    grid_level = get_mesh_level( _currentModule->getMeshPointer(), pchr->getPosX(), pchr->getPosY(), false );
    water_level = get_mesh_level( _currentModule->getMeshPointer(), pchr->getPosX(), pchr->getPosY(), true );

    // chr_set_enviro_grid_level() sets up the reflection level and reflection matrix
    chr_set_enviro_grid_level( pchr, grid_level );

    penviro->grid_lerp  = ( pchr->getPosZ() - grid_level ) / PLATTOLERANCE;
    penviro->grid_lerp  = CLIP( penviro->grid_lerp, 0.0f, 1.0f );

    penviro->water_level = water_level;
    penviro->water_lerp  = ( pchr->getPosZ() - water_level ) / PLATTOLERANCE;
    penviro->water_lerp  = CLIP( penviro->water_lerp, 0.0f, 1.0f );

    // The actual level of the floor underneath the character.
    if ( NULL != pplatform )
    {
        penviro->floor_level = pplatform->getPosZ() + pplatform->chr_min_cv._maxs[OCT_Z];
    }
    else
    {
        penviro->floor_level = pchr->waterwalk ? water_level : grid_level;
    }

    //---- The actual level of the characer.
    //     Estimate platform attachment from whatever is in the onwhichplatform_ref variable from the
    //     last loop
    penviro->level = penviro->floor_level;
    if ( NULL != pplatform )
    {
        penviro->level = pplatform->getPosZ() + pplatform->chr_min_cv._maxs[OCT_Z];
    }

    //---- The flying height of the character, the maximum of tile level, platform level and water level
    if ( 0 != ego_mesh_t::test_fx( _currentModule->getMeshPointer(), pchr->getTile(), MAPFX_WATER ) )
    {
        penviro->fly_level = std::max( penviro->level, water._surface_level );
    }

    if ( penviro->fly_level < 0 )
    {
        penviro->fly_level = 0;  // fly above pits...
    }

    // set the zlerp after we have done everything to the particle's level we care to
    penviro->zlerp = ( pchr->getPosZ() - penviro->level ) / PLATTOLERANCE;
    penviro->zlerp = CLIP( penviro->zlerp, 0.0f, 1.0f );

    penviro->grounded = (( 0 == pchr->flyheight ) && ( penviro->zlerp < 0.25f ) );

    //---- the "twist" of the floor
    penviro->grid_twist = ego_mesh_get_twist( _currentModule->getMeshPointer(), pchr->getTile() );

    // the "watery-ness" of whatever water might be here
    penviro->is_watery = water._is_water && penviro->inwater;
    penviro->is_slippy = !penviro->is_watery && ( 0 != ego_mesh_t::test_fx( _currentModule->getMeshPointer(), pchr->getTile(), MAPFX_SLIPPY ) );

    //---- traction
    penviro->traction = 1.0f;
    if ( 0 != pchr->flyheight )
    {
        // any traction factor here
        /* traction = ??; */
    }
    else if ( NULL != pplatform )
    {
        // in case the platform is tilted
        // unfortunately platforms are attached in the collision section
        // which occurs after the movement section.

        fvec3_t platform_up = fvec3_t( 0.0f, 0.0f, 1.0f );

        chr_getMatUp(pplatform, platform_up);
		platform_up.normalize();

        penviro->traction = std::abs(platform_up[kZ]) * ( 1.0f - penviro->zlerp ) + 0.25f * penviro->zlerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= 4.00f * Physics::g_environment.hillslide * (1.0f - penviro->zlerp) + 1.0f * penviro->zlerp;
        }
    }
    else if ( ego_mesh_t::grid_is_valid( _currentModule->getMeshPointer(), pchr->getTile() ) )
    {
        penviro->traction = std::abs( map_twist_nrm[penviro->grid_twist][kZ] ) * ( 1.0f - penviro->zlerp ) + 0.25f * penviro->zlerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= 4.00f * Physics::g_environment.hillslide * (1.0f - penviro->zlerp) + 1.0f * penviro->zlerp;
        }
    }

    //---- the friction of the fluid we are in
    if ( penviro->is_watery )
    {
        //Athletics perk halves penality for moving in water
        if(pchr->hasPerk(Ego::Perks::ATHLETICS)) {
            penviro->fluid_friction_vrt  = (Physics::g_environment.waterfriction + penviro->air_friction)*0.5f;
            penviro->fluid_friction_hrz  = (Physics::g_environment.waterfriction + penviro->air_friction)*0.5f;
        }
        else {
            penviro->fluid_friction_vrt  = Physics::g_environment.waterfriction;
            penviro->fluid_friction_hrz = Physics::g_environment.waterfriction;            
        }

    }
    else
    {
        penviro->fluid_friction_hrz = penviro->air_friction;       // like real-life air friction
        penviro->fluid_friction_vrt  = penviro->air_friction;
    }

    //---- friction
    penviro->friction_hrz       = 1.0f;
    if ( 0 != pchr->flyheight )
    {
        if ( pchr->platform )
        {
            // override the z friction for platforms.
            // friction in the z direction will make the bouncing stop
            penviro->fluid_friction_vrt = 1.0f;
        }
    }
    else
    {
        // Make the characters slide
        float temp_friction_xy = Physics::g_environment.noslipfriction;
        if ( ego_mesh_t::grid_is_valid( _currentModule->getMeshPointer(), pchr->getTile() ) && penviro->is_slippy )
        {
            // It's slippy all right...
            temp_friction_xy = Physics::g_environment.slippyfriction;
        }

        penviro->friction_hrz = penviro->zlerp * 1.0f + ( 1.0f - penviro->zlerp ) * temp_friction_xy;
    }

    //---- jump stuff
    if ( 0 != pchr->flyheight )
    {
        // Flying
        pchr->jumpready = false;
    }
    else
    {
        // Character is in the air
        pchr->jumpready = penviro->grounded;

        // Down jump timer
        if (( _currentModule->getObjectHandler().exists( pchr->attachedto ) || pchr->jumpready || pchr->jumpnumber > 0 ) && pchr->jump_timer > 0 ) pchr->jump_timer--;

        // Do ground hits
        if ( penviro->grounded && pchr->vel[kZ] < -STOPBOUNCING && pchr->hitready )
        {
            SET_BIT( pchr->ai.alert, ALERTIF_HITGROUND );
            pchr->hitready = false;
        }

        // Special considerations for slippy surfaces
        if ( penviro->is_slippy )
        {
            if ( map_twist_flat[penviro->grid_twist] )
            {
                // Reset jumping on flat areas of slippiness
                if ( penviro->grounded && 0 == pchr->jump_timer )
                {
                    pchr->jumpnumber = pchr->jumpnumberreset;
                }
            }
        }
        else if ( penviro->grounded && 0 == pchr->jump_timer )
        {
            // Reset jumping
            pchr->jumpnumber = pchr->jumpnumberreset;
        }
    }

    // add in something for the "ground speed"
    if ( NULL == pplatform )
    {
		penviro->floor_speed = fvec3_t::zero();
    }
    else
    {
		penviro->floor_speed = pplatform->vel;
    }

}

//--------------------------------------------------------------------------------------------
void move_one_character_do_floor_friction( Object * pchr )
{
    /// @author BB
    /// @details Friction is complicated when you want to have sliding characters :P

    float temp_friction_xy;
    fvec3_t   vup, floor_acc, fric, fric_floor;
    chr_environment_t * penviro;

    if ( !ACTIVE_PCHR( pchr ) ) return;
    penviro = &( pchr->enviro );

    if ( 0 != pchr->flyheight ) return;

    // assume the best
	floor_acc = fvec3_t::zero();
    temp_friction_xy = 1.0f;
    vup = fvec3_t(0.0f, 0.0f, 1.0f);

    const std::shared_ptr<Object> &platform = _currentModule->getObjectHandler()[pchr->onwhichplatform_ref];

    // figure out the acceleration due to the current "floor"
    if (platform != nullptr)
    {
        chr_getMatUp(platform.get(), vup);
    }
    else if ( !pchr->alive || pchr->isitem )
    {
        temp_friction_xy = 0.5f;

        if ( TWIST_FLAT != penviro->grid_twist )
        {
            vup = map_twist_nrm[penviro->grid_twist];
        }

    }
    else
    {
        temp_friction_xy = penviro->friction_hrz;

        if ( TWIST_FLAT != penviro->grid_twist )
        {
            vup = map_twist_nrm[penviro->grid_twist];
        }
    }

    floor_acc = penviro->floor_speed - pchr->vel;
	floor_acc *= 1.0f - penviro->zlerp;

    // reduce the volountary acceleration peopendicular to the direction of motion?
    if (floor_acc.length_abs() > 0.0f)
    {
        fvec3_t acc_para, acc_perp;
        fvec3_t vfront;

        // get the direction of motion
        vfront = mat_getChrForward(pchr->inst.matrix);
		vfront.normalize();

        // decompose the acceleration into parallel and perpendicular components
		fvec3_decompose(floor_acc, vfront, acc_para, acc_perp);

        // re-compose the acceleration with 1/2 of the perpendicular taken away
		floor_acc = acc_perp * 0.5f;
        floor_acc += acc_para;
    }

    // the first guess about the floor friction
	fric_floor = floor_acc * (penviro->traction *(1.0f - temp_friction_xy));

    // the total "friction" with to the floor
    fric = fric_floor + penviro->acc;

    // limit the friction to whatever is horizontal to the mesh
    if (1.0f == std::abs(vup[kZ]))
    {
        fric[kZ] = 0.0f;
        floor_acc[kZ] = 0.0f;
    }
    else
    {
        fvec3_t acc_perp, acc_para;

        fvec3_decompose(fric, vup, acc_perp, acc_para);
        fric = acc_para;

        fvec3_decompose(floor_acc, vup, acc_perp, acc_para);
        floor_acc = acc_para;
    }

    // test to see if the player has any more friction left?
    penviro->is_slipping = ( fric.length_abs() > penviro->friction_hrz );

    if ( penviro->is_slipping )
    {
        penviro->traction *= 0.5f;
        temp_friction_xy = std::sqrt( temp_friction_xy );

        // the first guess about the floor friction
		fric_floor = floor_acc *  (penviro->traction * (1.0f - temp_friction_xy));
    }

    // Apply the floor friction
	pchr->vel += fric_floor;

    // Apply fluid friction from last time
    pchr->vel[kX] += -pchr->vel[kX] * ( 1.0f - penviro->fluid_friction_hrz );
    pchr->vel[kY] += -pchr->vel[kY] * ( 1.0f - penviro->fluid_friction_hrz );
    pchr->vel[kZ] += -pchr->vel[kZ] * ( 1.0f - penviro->fluid_friction_vrt );
}

//--------------------------------------------------------------------------------------------
void move_one_character_do_voluntary( Object * pchr )
{
    // do voluntary motion
    if ( !ACTIVE_PCHR( pchr ) ) return;

    CHR_REF ichr = GET_INDEX_PCHR( pchr );

    if ( !pchr->alive || pchr->maxaccel == 0.00f ) return;

    pchr->enviro.new_v[kX] = pchr->vel[kX];
    pchr->enviro.new_v[kY] = pchr->vel[kY];

    //Mounted?
    if ( pchr->isBeingHeld() ) return;

    float new_ax = 0.0f, new_ay = 0.0f;

    // Character latches for generalized movement
    float dvx = pchr->latch.x;
    float dvy = pchr->latch.y;

    // Reverse movements for daze
    if ( pchr->daze_timer > 0 )
    {
        dvx = -dvx;
        dvy = -dvy;
    }

    // Switch x and y for grog
    if ( pchr->grog_timer > 0 )
    {
        std::swap(dvx, dvy);
    }

    // this is the maximum speed that a character could go under the v2.22 system
    float maxspeed = pchr->maxaccel * Physics::g_environment.airfriction / (1.0f - Physics::g_environment.airfriction);
    float speedBonus = 1.0f;

    //Sprint perk gives +10% movement speed if above 75% life remaining
    if(pchr->hasPerk(Ego::Perks::SPRINT) && pchr->getLife() >= pchr->getAttribute(Ego::Attribute::MAX_LIFE)*0.75f) {
        speedBonus += 0.1f;

        //Uninjured? (Dash perk can give another 10% extra speed)
        if(pchr->hasPerk(Ego::Perks::DASH) && pchr->getAttribute(Ego::Attribute::MAX_LIFE)-pchr->getLife() < 1.0f) {
            speedBonus += 0.1f;
        }
    }

    //Rally Bonus? (+10%)
    if(pchr->hasPerk(Ego::Perks::RALLY) && update_wld < pchr->getRallyDuration()) {
        speedBonus += 0.1f;
    }    

    //Increase movement by 1% per Agility above 10 (below 10 agility reduces movement speed!)
    speedBonus += (pchr->getAttribute(Ego::Attribute::AGILITY)-10.0f) * 0.01f;

    //Now apply speed modifiers
    maxspeed *= speedBonus;

    //Check animation frame freeze movement
    if ( chr_get_framefx( pchr ) & MADFX_STOP )
    {
        //Allow 50% movement while using Shield and have the Mobile Defence perk
        if(pchr->hasPerk(Ego::Perks::MOBILE_DEFENCE) && ACTION_IS_TYPE(pchr->inst.action_which, P))
        {
            maxspeed *= 0.5f;
        }
        //Allow 50% movement with Mobility perk and attacking with a weapon
        else if(pchr->hasPerk(Ego::Perks::MOBILITY) && pchr->isAttacking())
        {
            maxspeed *= 0.5f;
        }
        else
        {
            //No movement allowed
            maxspeed = 0.0f;
        }
    }

    bool sneak_mode_active = false;
    if ( VALID_PLA( pchr->is_which_player ) )
    {
        // determine whether the user is hitting the "sneak button"
        player_t * ppla = PlaStack.get_ptr( pchr->is_which_player );
        sneak_mode_active = input_device_control_active( ppla->pdevice, CONTROL_SNEAK );
    }

    pchr->enviro.new_v[kX] = pchr->enviro.new_v[kY] = 0.0f;
	if (std::abs(dvx) + std::abs(dvy) > 0.05f)
    {
        float dv2 = dvx * dvx + dvy * dvy;

        if (pchr->isPlayer())
        {
            float dv = std::pow( dv2, 0.25f );

            // determine whether the character is sneaking
            sneak_mode_active = TO_C_BOOL( dv2 < 1.0f / 9.0f );

            pchr->enviro.new_v[kX] = maxspeed * dvx / dv;
            pchr->enviro.new_v[kY] = maxspeed * dvy / dv;
        }
        else
        {
            float scale = 1.0f;

            if ( dv2 < 1.0f )
            {
                scale = std::pow( dv2, 0.25f );
            }

            scale /= std::pow( dv2, 0.5f );

            pchr->enviro.new_v[kX] = dvx * maxspeed * scale;
            pchr->enviro.new_v[kY] = dvy * maxspeed * scale;
        }
    }

    if ( sneak_mode_active )
    {
        // sneak mode
        pchr->maxaccel      = pchr->maxaccel_reset * 0.33f;
        pchr->movement_bits = CHR_MOVEMENT_BITS_SNEAK | CHR_MOVEMENT_BITS_STOP;
    }
    else
    {
        // non-sneak mode
        pchr->movement_bits = ( unsigned )( ~CHR_MOVEMENT_BITS_SNEAK );
        pchr->maxaccel      = pchr->maxaccel_reset;
    }

    // do platform friction
    if ( _currentModule->getObjectHandler().exists( pchr->onwhichplatform_ref ) )
    {
        Object * pplat = _currentModule->getObjectHandler().get( pchr->onwhichplatform_ref );

        new_ax += ( pplat->vel[kX] + pchr->enviro.new_v[kX] - ( pchr->vel[kX] ) );
        new_ay += ( pplat->vel[kY] + pchr->enviro.new_v[kY] - ( pchr->vel[kY] ) );
    }
    else
    {
        new_ax += ( pchr->enviro.new_v[kX] - pchr->vel[kX] );
        new_ay += ( pchr->enviro.new_v[kY] - pchr->vel[kY] );
    }

    new_ax *= pchr->enviro.traction;
    new_ay *= pchr->enviro.traction;

    //Limit movement to the max acceleration
    new_ax = Ego::Math::constrain( new_ax, -pchr->maxaccel, pchr->maxaccel );
    new_ay = Ego::Math::constrain( new_ay, -pchr->maxaccel, pchr->maxaccel );

    //Figure out how to turn around
    if ( 0 != pchr->maxaccel )
    {
        switch ( pchr->turnmode )
        {
            // Get direction from ACTUAL change in velocity
            default:
            case TURNMODE_VELOCITY:
                {
                    if (std::abs(dvx) > TURNSPD || std::abs(dvy) > TURNSPD)
                    {
                        if ( VALID_PLA( pchr->is_which_player ) )
                        {
                            // Players turn quickly
                            pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing( dvx , dvy ), 2 );
                        }
                        else
                        {
                            // AI turn slowly
                            pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing( dvx , dvy ), 8 );
                        }
                    }
                }
                break;

            // Get direction from the DESIRED change in velocity
            case TURNMODE_WATCH:
                {
                    if (( std::abs( dvx ) > WATCHMIN || std::abs( dvy ) > WATCHMIN ) )
                    {
                        pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing( dvx , dvy ), 8 );
                    }
                }
                break;

            // Face the target
            case TURNMODE_WATCHTARGET:
                {
                    if ( ichr != pchr->ai.target )
                    {
                        pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing( _currentModule->getObjectHandler().get(pchr->ai.target)->getPosX() - pchr->getPosX() , _currentModule->getObjectHandler().get(pchr->ai.target)->getPosY() - pchr->getPosY() ), 8 );
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

    //Update velocity
    pchr->vel[kX] += new_ax;
    pchr->vel[kY] += new_ay;
}

//--------------------------------------------------------------------------------------------
bool chr_do_latch_attack( Object * pchr, slot_t which_slot )
{
    CHR_REF ichr, iweapon;

    int    base_action, hand_action, action;
    bool action_valid, allowedtoattack;

    bool retval = false;

    if ( !ACTIVE_PCHR( pchr ) ) return false;
    ichr = GET_INDEX_PCHR( pchr );


    if (which_slot >= SLOT_COUNT) return false;

    // Which iweapon?
    iweapon = pchr->holdingwhich[which_slot];
    if ( !_currentModule->getObjectHandler().exists( iweapon ) )
    {
        // Unarmed means character itself is the iweapon
        iweapon = ichr;
    }
    Object *pweapon     = _currentModule->getObjectHandler().get(iweapon);
    const std::shared_ptr<ObjectProfile> &weaponProfile = pweapon->getProfile();

    //No need to continue if we have an attack cooldown
    if ( 0 != pweapon->reload_timer ) return false;

    // grab the iweapon's action
    base_action = weaponProfile->getWeaponAction();
    hand_action = pchr->getProfile()->getModel()->randomizeAction( base_action, which_slot );

    // see if the character can play this action
    action       = pchr->getProfile()->getModel()->getAction(hand_action);
    action_valid = TO_C_BOOL( ACTION_COUNT != action );

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
            if ( !chr_get_skill( pchr, chr_get_idsz( iweapon, IDSZ_SKILL ) ) )
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
            if ( pmount->isMount() && pmount->alive )
            {
                // can the mount be told what to do?
                if ( !VALID_PLA( pmount->is_which_player ) && pmount->inst.action_ready )
                {
                    if ( !ACTION_IS_TYPE( action, P ) || !mountProfile->riderCanAttack() )
                    {
                        chr_play_action( pmount.get(), Random::next((int)ACTION_UA, ACTION_UA + 1), false );
                        SET_BIT( pmount->ai.alert, ALERTIF_USED );
                        pchr->ai.lastitemused = pmount->getCharacterID();

                        retval = true;
                    }
                }
            }
        }
    }

    // Attack button
    if ( allowedtoattack )
    {
        if ( pchr->inst.action_ready && action_valid )
        {
            //Check if we are attacking unarmed and cost mana to do so
            if(iweapon == pchr->getCharacterID())
            {
                if(pchr->getProfile()->getUseManaCost() <= pchr->mana)
                {
                    pchr->costMana(pchr->getProfile()->getUseManaCost(), pchr->getCharacterID());
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
                    if ( iweapon != ichr )
                    {
                        chr_play_action( pweapon, ACTION_MJ, false );
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
                        chr_make_text_billboard(pchr->getCharacterID(), "Quick Strike!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::blue(), 3, Billboard::Flags::All);
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

//--------------------------------------------------------------------------------------------
bool chr_do_latch_button( Object * pchr )
{
    /// @author BB
    /// @details Character latches for generalized buttons

    ai_state_t * pai;

    bool attack_handled;

    if ( !ACTIVE_PCHR( pchr ) ) return false;
    CHR_REF ichr = pchr->getCharacterID();

    pai = &( pchr->ai );

    if ( !pchr->alive || pchr->latch.b.none() ) return true;

    const std::shared_ptr<ObjectProfile> &profile = pchr->getProfile();

    if ( pchr->latch.b[LATCHBUTTON_JUMP] && 0 == pchr->jump_timer )
    {

        //Jump from our mount
        if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
        {
            pchr->detatchFromHolder(true, true);
            detach_character_from_platform( pchr );

            pchr->jump_timer = JUMPDELAY;
            if ( 0 != pchr->flyheight )
            {
                pchr->vel[kZ] += DISMOUNTZVELFLY;
            }
            else
            {
                pchr->vel[kZ] += DISMOUNTZVEL;
            }

            pchr->setPosition(pchr->getPosX(), pchr->getPosY(), pchr->getPosZ() + pchr->vel[kZ]);

            if ( pchr->jumpnumberreset != JUMPINFINITE && 0 != pchr->jumpnumber )
                pchr->jumpnumber--;

            // Play the jump sound
            AudioSystem::get().playSound(pchr->getPosition(), profile->getJumpSound());
        }

        //Normal jump
        else if ( 0 != pchr->jumpnumber && 0 == pchr->flyheight )
        {
            if ( 1 != pchr->jumpnumberreset || pchr->jumpready )
            {

                // Make the character jump
                pchr->hitready = true;
                if ( pchr->enviro.inwater || pchr->enviro.is_slippy )
                {
                    pchr->jump_timer = JUMPDELAY * 4;         //To prevent 'bunny jumping' in water
                    pchr->vel[kZ] += WATERJUMP;
                }
                else
                {
                    pchr->jump_timer = JUMPDELAY;
                    pchr->vel[kZ] += pchr->jump_power * 1.5f;
                }

                pchr->jumpready = false;
                if ( pchr->jumpnumberreset != JUMPINFINITE ) pchr->jumpnumber--;

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
        if ( !_currentModule->getObjectHandler().exists( pchr->holdingwhich[SLOT_LEFT] ) )
        {
            // Grab left
            chr_play_action( pchr, ACTION_ME, false );
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
        if ( !_currentModule->getObjectHandler().exists( pchr->holdingwhich[SLOT_RIGHT] ) )
        {
            // Grab right
            chr_play_action( pchr, ACTION_MF, false );
        }
        else
        {
            // Drop right
            chr_play_action( pchr, ACTION_MB, false );
        }
    }

    // LATCHBUTTON_LEFT and LATCHBUTTON_RIGHT are mutually exclusive
    attack_handled = false;
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

//--------------------------------------------------------------------------------------------
void move_one_character_do_z_motion( Object * pchr )
{
    if ( !ACTIVE_PCHR( pchr ) ) return;

    //---- do z acceleration
    if ( 0 != pchr->flyheight )
    {
        pchr->vel[kZ] += ( pchr->enviro.fly_level + pchr->flyheight - pchr->getPosZ() ) * FLYDAMPEN;
    }

    else if (
        pchr->enviro.is_slippy && ( pchr->enviro.grid_twist != TWIST_FLAT ) &&
        ( CHR_INFINITE_WEIGHT != pchr->phys.weight ) && ( pchr->enviro.grid_lerp <= pchr->enviro.zlerp ) )
    {
        // Slippy hills make characters slide

        fvec3_t   gperp;    // gravity perpendicular to the mesh
        fvec3_t   gpara;    // gravity parallel      to the mesh (what pushes you)

        // RELATIVE TO THE GRID, since you might be riding a platform!
        float     loc_zlerp = pchr->enviro.grid_lerp;

        gpara = map_twist_vel[pchr->enviro.grid_twist];

        gperp[kX] = 0       - gpara[kX];
        gperp[kY] = 0       - gpara[kY];
        gperp[kZ] = Physics::g_environment.gravity - gpara[kZ];

        pchr->vel += gpara * ( 1.0f - loc_zlerp ) + gperp * loc_zlerp;
    }
    else
    {
        pchr->vel[kZ] += pchr->enviro.zlerp * Physics::g_environment.gravity;
    }
}

//--------------------------------------------------------------------------------------------
bool chr_update_safe_raw( Object * pchr )
{
    bool retval = false;

    BIT_FIELD hit_a_wall;
    float pressure = 0.0f;

    if ( nullptr == ( pchr ) ) return false;

    fvec2_t nrm;
    hit_a_wall = pchr->hit_wall( nrm, &pressure, NULL );
    if (( 0 == hit_a_wall ) && ( 0.0f == pressure ) )
    {
        pchr->safe_valid = true;
        pchr->safe_pos = pchr->getPosition();
        pchr->safe_time  = update_wld;
        pchr->safe_grid  = ego_mesh_t::get_grid( _currentModule->getMeshPointer(), PointWorld(pchr->getPosX(), pchr->getPosY())).getI();

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool chr_update_safe( Object * pchr, bool force )
{
    bool retval = false;
    bool needs_update = false;

    if ( nullptr == ( pchr ) ) return false;

    if ( force || !pchr->safe_valid )
    {
        needs_update = true;
    }
    else
    {
        TileIndex new_grid = ego_mesh_t::get_grid(_currentModule->getMeshPointer(), PointWorld(pchr->getPosX(), pchr->getPosY()));

        if (TileIndex::Invalid == new_grid )
        {
            if ( std::abs( pchr->getPosX() - pchr->safe_pos[kX] ) > GRID_FSIZE ||
                 std::abs( pchr->getPosY() - pchr->safe_pos[kY] ) > GRID_FSIZE )
            {
                needs_update = true;
            }
        }
        else if ( new_grid != pchr->safe_grid )
        {
            needs_update = true;
        }
    }

    if ( needs_update )
    {
        retval = chr_update_safe_raw( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool chr_get_safe(Object * pchr)
{
    bool found = false;
    fvec3_t loc_pos;

    if (nullptr == pchr) return false;

    // DO NOT require objects that are spawning in a module to have a
    // valid position at spawn-time. For instance, if a suit of armor is
    // spawned in a closed hallway, don't complain.

    /// @note ZF@> I fixed a bug that caused this boolean variable always to be true.
    /// by fixing it I broke other stuff like specific objects spawning after parsing spawn.txt, I've tried a hotfix here instead
    if ( HAS_SOME_BITS( ALERTIF_SPAWNED, pchr->ai.alert ) )
    {
        return true;
    }

    if ( !found && pchr->safe_valid )
    {
        fvec2_t nrm;
        if ( !pchr->hit_wall( nrm, NULL, NULL ) )
        {
            found = true;
        }
    }

    if ( !found )
    {
        breadcrumb_t * bc;

        bc = breadcrumb_list_t::last_valid( &( pchr->crumbs ) );

        if ( NULL != bc )
        {
            found = true;
        }
    }

    // maybe there is one last fallback after this? we could check the character's current position?
    if ( !found )
    {
        log_debug( "Uh oh! We could not find a valid non-wall position for %s!\n", _currentModule->getObjectHandler()[pchr->ai.index]->getProfile()->getClassName().c_str() );
    }

    return found;
}

//--------------------------------------------------------------------------------------------
bool chr_update_breadcrumb_raw(Object * object)
{
    breadcrumb_t bc;
    bool retval = false;

    if (nullptr == object) return false;

    breadcrumb_t::init(&bc, object);

    if (bc.valid)
    {
        retval = breadcrumb_list_t::add(&(object->crumbs), &bc);
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool chr_update_breadcrumb( Object * object, bool force )
{
    bool retval = false;
    bool needs_update = false;
    breadcrumb_t * bc_ptr, bc;

    if (nullptr == (object)) return false;

    bc_ptr = breadcrumb_list_t::last_valid(&(object->crumbs));
    if ( NULL == bc_ptr )
    {
        force  = true;
        bc_ptr = &bc;
        breadcrumb_t::init(bc_ptr, object);
    }

    if ( force )
    {
        needs_update = true;
    }
    else
    {
        TileIndex new_grid = ego_mesh_t::get_grid(_currentModule->getMeshPointer(), PointWorld(object->getPosX(), object->getPosY()));

        if (TileIndex::Invalid == new_grid )
        {
            if (std::abs(object->getPosX() - bc_ptr->pos[kX]) > GRID_FSIZE ||
                std::abs(object->getPosY() - bc_ptr->pos[kY]) > GRID_FSIZE)
            {
                needs_update = true;
            }
        }
        else if ( new_grid != bc_ptr->grid )
        {
            needs_update = true;
        }
    }

    if ( needs_update )
    {
        retval = chr_update_breadcrumb_raw(object);
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
breadcrumb_t * chr_get_last_breadcrumb( Object * pchr )
{
    if ( nullptr == ( pchr ) ) return NULL;

    if ( 0 == pchr->crumbs.count ) return NULL;

    return breadcrumb_list_t::last_valid( &( pchr->crumbs ) );
}

//--------------------------------------------------------------------------------------------
bool move_one_character_integrate_motion_attached( Object * pchr )
{
    Uint32 chr_update;

    if ( !ACTIVE_PCHR( pchr ) ) return false;

    // make a timer that is individual for each object
    chr_update = pchr->getCharacterID() + update_wld;

    if ( 0 == ( chr_update & 7 ) )
    {
        chr_update_safe( pchr, true );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool move_one_character_integrate_motion( Object * pchr )
{
    /// @author BB
    /// @details Figure out the next position of the character.
    ///    Include collisions with the mesh in this step.

    CHR_REF  ichr;
    ai_state_t * pai;

    float   bumpdampen;
    bool  needs_test, updated_2d;

    fvec3_t tmp_pos;

    if ( !ACTIVE_PCHR( pchr ) ) return false;

    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        return move_one_character_integrate_motion_attached( pchr );
    }

    tmp_pos = pchr->getPosition();;

    pai = &( pchr->ai );
    ichr = pai->index;

    bumpdampen = CLIP( pchr->phys.bumpdampen, 0.0f, 1.0f );
    bumpdampen = ( bumpdampen + 1.0f ) * 0.5f;

    // interaction with the mesh
    //if ( std::abs( pchr->vel[kZ] ) > 0.0f )
    {
        const float vert_offset = RAISE * 0.25f;
        float grid_level = pchr->enviro.grid_level + vert_offset + 5;

        tmp_pos[kZ] += pchr->vel[kZ];
        LOG_NAN( tmp_pos[kZ] );
        if ( tmp_pos[kZ] < grid_level )
        {
            if ( std::abs( pchr->vel[kZ] ) < STOPBOUNCING )
            {
                pchr->vel[kZ] = 0.0f;
                tmp_pos[kZ] = grid_level;
            }
            else
            {
                if ( pchr->vel[kZ] < 0.0f )
                {
                    float diff = grid_level - tmp_pos[kZ];

                    pchr->vel[kZ] *= -pchr->phys.bumpdampen;
                    diff        *= -pchr->phys.bumpdampen;

                    tmp_pos[kZ] = std::max( tmp_pos[kZ] + diff, grid_level );
                }
                else
                {
                    tmp_pos[kZ] = grid_level;
                }
            }
        }
    }

    // fixes to the z-position
    if ( 0.0f != pchr->flyheight )
    {
        if ( tmp_pos[kZ] < 0.0f ) tmp_pos[kZ] = 0.0f;  // Don't fall in pits...
    }

    // interaction with the grid flags
    updated_2d = false;
    needs_test = false;

    //if (std::abs(pchr->vel[kX]) + std::abs(pchr->vel[kY]) > 0.0f)
    {
        mesh_wall_data_t wdata;

        float old_x, old_y, new_x, new_y;

        old_x = tmp_pos[kX]; LOG_NAN( old_x );
        old_y = tmp_pos[kY]; LOG_NAN( old_y );

        new_x = old_x + pchr->vel[kX]; LOG_NAN( new_x );
        new_y = old_y + pchr->vel[kY]; LOG_NAN( new_y );

        tmp_pos[kX] = new_x;
        tmp_pos[kY] = new_y;

        if ( EMPTY_BIT_FIELD == pchr->test_wall( tmp_pos, &wdata ) )
        {
            updated_2d = true;
        }
        else
        {
            fvec2_t nrm;
            float   pressure;
            bool diff_function_called = false;

            pchr->hit_wall( tmp_pos, nrm, &pressure, &wdata );

            // how is the character hitting the wall?
            if ( 0.0f != pressure )
            {
                bool         found_nrm  = false;
                bool         found_safe = false;
                fvec3_t      safe_pos   = fvec3_t::zero();

                bool         found_diff = false;
                fvec2_t      diff       = fvec2_t::zero();

                breadcrumb_t * bc         = NULL;

                // try to get the correct "outward" pressure from nrm
				if (!found_nrm && nrm.length_abs() > 0.0f)
                {
                    found_nrm = true;
                }

                if ( !found_diff && pchr->safe_valid )
                {
                    if ( !found_safe )
                    {
                        found_safe = true;
                        safe_pos   = pchr->safe_pos;
                    }

                    diff[XX] = pchr->safe_pos[kX] - pchr->getPosX();
                    diff[YY] = pchr->safe_pos[kY] - pchr->getPosY();

					if (diff.length_abs() > 0.0f)
                    {
                        found_diff = true;
                    }
                }

                // try to get a diff from a breadcrumb
                if ( !found_diff )
                {
                    bc = chr_get_last_breadcrumb( pchr );

                    if ( NULL != bc && bc->valid )
                    {
                        if ( !found_safe )
                        {
                            found_safe = true;
                            safe_pos   = pchr->safe_pos;
                        }

                        diff[XX] = bc->pos[kX] - pchr->getPosX();
                        diff[YY] = bc->pos[kY] - pchr->getPosY();

                        if (diff.length_abs() > 0.0f )
                        {
                            found_diff = true;
                        }
                    }
                }

                // try to get a normal from the ego_mesh_get_diff() function
                if ( !found_nrm )
                {
                    fvec3_t tmp_diff;

                    tmp_diff = chr_get_mesh_diff(pchr, tmp_pos, pressure);
                    diff_function_called = true;

                    nrm[XX] = tmp_diff[kX];
                    nrm[YY] = tmp_diff[kY];

                    if (nrm.length_abs() > 0.0f)
                    {
                        found_nrm = true;
                    }
                }

                if ( !found_diff )
                {
                    // try to get the diff from the character velocity
                    diff[XX] = pchr->vel[XX];
                    diff[YY] = pchr->vel[YY];

                    // make sure that the diff is in the same direction as the velocity
                    if ( diff.dot(nrm) < 0.0f )
                    {
                        diff = -diff;
                    }

					if (diff.length_abs() > 0.0f)
                    {
                        found_diff = true;
                    }
                }

                if ( !found_nrm )
                {
                    // After all of our best efforts, we can't generate a normal to the wall.
                    // This can happen if the object is completely inside a wall,
                    // (like if it got pushed in there) or if a passage closed around it.
                    // Just teleport the character to a "safe" position.

                    if ( !found_safe && NULL == bc )
                    {
                        bc = chr_get_last_breadcrumb( pchr );

                        if ( NULL != bc && bc->valid )
                        {
                            found_safe = true;
                            safe_pos   = pchr->safe_pos;
                        }
                    }

                    if ( !found_safe )
                    {
                        // the only safe position is the spawn point???
                        found_safe = true;
                        safe_pos = pchr->pos_stt;
                    }

                    tmp_pos = safe_pos;
                }
                else if ( found_diff && found_nrm )
                {
                    const float tile_fraction = 0.1f;
                    float ftmp, dot, pressure_old, pressure_new;
                    fvec3_t save_pos;
                    float nrm2;

                    fvec2_t v_perp = fvec2_t::zero();
                    fvec2_t diff_perp = fvec2_t::zero();

					nrm2 = nrm.dot(nrm);

                    save_pos = tmp_pos;

                    // make the diff point "out"
					dot = diff.dot(nrm);
                    if ( dot < 0.0f )
                    {
                        diff = -diff;
                        dot    *= -1.0f;
                    }

                    // find the part of the diff that is parallel to the normal
                    diff_perp = fvec2_t::zero();
                    if ( nrm2 > 0.0f )
                    {
                        diff_perp = nrm * (dot / nrm2);
                    }

                    // normalize the diff_perp so that it is at most tile_fraction of a grid in any direction
                    ftmp = diff_perp.length_max();

                    // protect us from a virtual divide by zero
                    if (ftmp < 1e-6) ftmp = 1.00f;

					diff_perp *= tile_fraction * GRID_FSIZE / ftmp;

                    // scale the diff_perp by the pressure
					diff_perp *= pressure;

                    // try moving the character
					tmp_pos += fvec3_t(diff_perp[kX],diff_perp[kY], 0.0f);

                    // determine whether the pressure is less at this location
                    pressure_old = chr_get_mesh_pressure(pchr, save_pos);
                    pressure_new = chr_get_mesh_pressure(pchr, tmp_pos);

                    if ( pressure_new < pressure_old )
                    {
                        // !!success!!
                        needs_test = ( tmp_pos[kX] != save_pos[kX] ) || ( tmp_pos[kY] != save_pos[kY] );
                    }
                    else
                    {
                        // !!failure!! restore the saved position
                        tmp_pos = save_pos;
                    }

                    dot = fvec2_t(pchr->vel[kX],pchr->vel[kY]).dot(nrm);
                    if ( dot < 0.0f )
                    {
                        float loc_bumpdampen;
   
                        loc_bumpdampen = ProfileSystem::get().getProfile(pchr->profile_ref)->getBumpDampen();

                        v_perp = fvec2_t::zero();
                        if ( 0.0f != nrm2 )
                        {
                            v_perp = nrm * (dot / nrm2);
                        }

                        pchr->vel[XX] += - ( 1.0f + loc_bumpdampen ) * v_perp[XX] * pressure;
                        pchr->vel[YY] += - ( 1.0f + loc_bumpdampen ) * v_perp[YY] * pressure;
                    }
                }
            }
        }
    }

    pchr->setPosition(tmp_pos);

    // we need to test the validity of the current position every 8 frames or so,
    // no matter what
    if ( !needs_test )
    {
        // make a timer that is individual for each object
        Uint32 chr_update = pchr->getCharacterID() + update_wld;

        needs_test = ( 0 == ( chr_update & 7 ) );
    }

    if ( needs_test || updated_2d )
    {
        chr_update_safe( pchr, needs_test );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void move_one_character( Object * pchr )
{
    if ( !ACTIVE_PCHR( pchr ) ) return;

    if ( _currentModule->getObjectHandler().exists( pchr->inwhich_inventory ) ) return;

    // save the velocity and acceleration from the last time-step
    pchr->enviro.vel = pchr->getPosition() - pchr->pos_old;
    pchr->enviro.acc = pchr->vel - pchr->vel_old;

    // Character's old location
    pchr->pos_old = pchr->getPosition();
    pchr->vel_old          = pchr->vel;
    pchr->ori_old.facing_z = pchr->ori.facing_z;

    pchr->enviro.new_v[kX] = pchr->vel[kX];
    pchr->enviro.new_v[kY] = pchr->vel[kY];

    move_one_character_get_environment( pchr );

    // do friction with the floor before voluntary motion
    move_one_character_do_floor_friction( pchr );

    move_one_character_do_voluntary( pchr );

    chr_do_latch_button( pchr );

    move_one_character_do_z_motion( pchr );

    move_one_character_integrate_motion( pchr );

    move_one_character_do_animation( pchr );

    // Characters with sticky butts lie on the surface of the mesh
    if ( pchr->stickybutt || !pchr->alive )
    {
        float fkeep = ( 7 + pchr->enviro.zlerp ) / 8.0f;
        float fnew  = ( 1 - pchr->enviro.zlerp ) / 8.0f;

        if ( fnew > 0 )
        {
            pchr->ori.map_twist_facing_x = pchr->ori.map_twist_facing_x * fkeep + map_twist_facing_x[pchr->enviro.grid_twist] * fnew;
            pchr->ori.map_twist_facing_y = pchr->ori.map_twist_facing_y * fkeep + map_twist_facing_y[pchr->enviro.grid_twist] * fnew;
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_all_characters()
{
    /// @author ZZ
    /// @details This function handles character physics

    chr_stoppedby_tests = 0;

    // Move every character
    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        // prime the environment
        object->enviro.air_friction = Physics::g_environment.airfriction;
        object->enviro.ice_friction = Physics::g_environment.icefriction;

        move_one_character( object.get() );

        chr_update_matrix( object.get(), true );
        keep_weapons_with_holder(object);
    }

    // The following functions need to be called any time you actually change a charcter's position
    //keep_weapons_with_holders();
    attach_all_particles();
    //update_all_character_matrices();
}

//--------------------------------------------------------------------------------------------
bool is_invictus_direction( FACING_T direction, const CHR_REF character, BIT_FIELD effects )
{
    FACING_T left, right;

    Object * pchr;

    bool  is_invictus;

    if ( !_currentModule->getObjectHandler().exists( character ) ) return true;
    pchr = _currentModule->getObjectHandler().get( character );

    // if the invictus flag is set, we are invictus
    if ( pchr->invictus ) return true;

    // if the effect is shield piercing, ignore shielding
    if ( HAS_SOME_BITS( effects, DAMFX_NBLOC ) ) return false;

    // if the character's frame is invictus, then check the angles
    if ( HAS_SOME_BITS( chr_get_framefx( pchr ), MADFX_INVICTUS ) )
    {
        //I Frame
        direction -= pchr->getProfile()->getInvictusFrameFacing();
        left       = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(pchr->getProfile()->getInvictusFrameAngle()) );
        right      = pchr->getProfile()->getInvictusFrameAngle();

        // If using shield, use the shield invictus instead
        if ( ACTION_IS_TYPE( pchr->inst.action_which, P ) )
        {
            bool parry_left = ( pchr->inst.action_which < ACTION_PC );

            // Using a shield?
            if ( parry_left && pchr->getLeftHandItem() )
            {
                // Check left hand
                left = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(pchr->getLeftHandItem()->getProfile()->getInvictusFrameAngle()) );
                right = pchr->getLeftHandItem()->getProfile()->getInvictusFrameAngle();
            }
            else if(pchr->getRightHandItem())
            {
                // Check right hand
                left = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(pchr->getRightHandItem()->getProfile()->getInvictusFrameAngle()) );
                right = pchr->getRightHandItem()->getProfile()->getInvictusFrameAngle();
            }
        }
    }
    else
    {
        // N Frame
        direction -= pchr->getProfile()->getNormalFrameFacing();
        left       = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(pchr->getProfile()->getNormalFrameAngle()) );
        right      = pchr->getProfile()->getNormalFrameAngle();
    }

    // Check that direction
    is_invictus = false;
    if ( direction <= left && direction <= right )
    {
        is_invictus = true;
    }

    return is_invictus;
}

//--------------------------------------------------------------------------------------------
grip_offset_t slot_to_grip_offset( slot_t slot )
{
    return static_cast<grip_offset_t>((slot+1) * GRIP_VERTS);
}

//--------------------------------------------------------------------------------------------
slot_t grip_offset_to_slot( grip_offset_t grip_off )
{
    slot_t retval = SLOT_LEFT;

    if ( 0 != grip_off % GRIP_VERTS )
    {
        // this does not correspond to a valid slot
        // coerce it to the "default" slot
        retval = SLOT_LEFT;
    }
    else
    {
        int islot = (( int )grip_off / GRIP_VERTS ) - 1;

        // coerce the slot number to fit within the valid range
        islot = CLIP( islot, 0, (int)SLOT_COUNT );

        retval = ( slot_t ) islot;
    }

    return retval;
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
    Object *pchr = _currentModule->getObjectHandler().get( ichr );
    if (!ProfileSystem::get().isValidProfileID(pchr->profile_ref)) {
        return "*INVALID*";
    } else {
        std::shared_ptr<ObjectProfile> ppro = ProfileSystem::get().getProfile(pchr->profile_ref);
        return ppro->getPathname();
    }
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
    fvec4_t   src[16];  // for the upper and lower octagon points
    fvec4_t   dst[16];  // for the upper and lower octagon points

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
    pchr->inst.matrix.transform(src, dst, vcount);

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
const char* describe_value( float value, float maxval, int * rank_ptr )
{
    /// @author ZF
    /// @details This converts a stat number into a more descriptive word

    static STRING retval;

    int local_rank;

    if ( NULL == rank_ptr ) rank_ptr = &local_rank;

    if (egoboo_config_t::get().hud_feedback.getValue() == Ego::FeedbackType::Number)
    {
        snprintf( retval, SDL_arraysize( retval ), "%2.1f", value );
        return retval;
    }

    float fval = ( 0 == maxval ) ? 1.0f : value / maxval;

    *rank_ptr = -5;
    strcpy( retval, "Unknown" );

    if ( fval >= .83 )        { strcpy( retval, "Godlike!" );   *rank_ptr =  8; }
    else if ( fval >= .66 ) { strcpy( retval, "Ultimate" );   *rank_ptr =  7; }
    else if ( fval >= .56 ) { strcpy( retval, "Epic" );       *rank_ptr =  6; }
    else if ( fval >= .50 ) { strcpy( retval, "Powerful" );   *rank_ptr =  5; }
    else if ( fval >= .43 ) { strcpy( retval, "Heroic" );     *rank_ptr =  4; }
    else if ( fval >= .36 ) { strcpy( retval, "Very High" );  *rank_ptr =  3; }
    else if ( fval >= .30 ) { strcpy( retval, "High" );       *rank_ptr =  2; }
    else if ( fval >= .23 ) { strcpy( retval, "Good" );       *rank_ptr =  1; }
    else if ( fval >= .17 ) { strcpy( retval, "Average" );    *rank_ptr =  0; }
    else if ( fval >= .11 ) { strcpy( retval, "Pretty Low" ); *rank_ptr = -1; }
    else if ( fval >= .07 ) { strcpy( retval, "Bad" );        *rank_ptr = -2; }
    else if ( fval >  .00 ) { strcpy( retval, "Terrible" );   *rank_ptr = -3; }
    else                    { strcpy( retval, "None" );       *rank_ptr = -4; }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char* describe_damage( float value, float maxval, int * rank_ptr )
{
    /// @author ZF
    /// @details This converts a damage value into a more descriptive word

    static STRING retval;

    float fval;
    int local_rank;

    if ( NULL == rank_ptr ) rank_ptr = &local_rank;

    if (egoboo_config_t::get().hud_feedback.getValue() == Ego::FeedbackType::Number)
    {
        snprintf( retval, SDL_arraysize( retval ), "%2.1f", FP8_TO_FLOAT( value ) );
        return retval;
    }

    fval = ( 0 == maxval ) ? 1.0f : value / maxval;

    *rank_ptr = -5;
    strcpy( retval, "Unknown" );

    if ( fval >= 1.50f )      { strcpy( retval, "Annihilation!" ); *rank_ptr =  5; }
    else if ( fval >= 1.00f ) { strcpy( retval, "Overkill!" );     *rank_ptr =  4; }
    else if ( fval >= 0.80f ) { strcpy( retval, "Deadly" );        *rank_ptr =  3; }
    else if ( fval >= 0.70f ) { strcpy( retval, "Crippling" );     *rank_ptr =  2; }
    else if ( fval >= 0.50f ) { strcpy( retval, "Devastating" );   *rank_ptr =  1; }
    else if ( fval >= 0.25f ) { strcpy( retval, "Hurtful" );       *rank_ptr =  0; }
    else if ( fval >= 0.10f ) { strcpy( retval, "A Scratch" );     *rank_ptr = -1; }
    else if ( fval >= 0.05f ) { strcpy( retval, "Ticklish" );      *rank_ptr = -2; }
    else if ( fval >= 0.00f ) { strcpy( retval, "Meh..." );        *rank_ptr = -3; }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char* describe_wounds( float max, float current )
{
    /// @author ZF
    /// @details This tells us how badly someone is injured

    static STRING retval;
    float fval;

    //Is it already dead?
    if ( current <= 0 )
    {
        strcpy( retval, "Dead!" );
        return retval;
    }

    //Calculate the percentage
    if ( 0 == max ) return NULL;
    fval = ( current / max ) * 100;

    if (egoboo_config_t::get().hud_feedback.getValue() == Ego::FeedbackType::Number)
    {
        snprintf( retval, SDL_arraysize( retval ), "%2.1f", FP8_TO_FLOAT( current ) );
        return retval;
    }

    strcpy( retval, "Uninjured" );
    if ( fval <= 5 )            strcpy( retval, "Almost Dead!" );
    else if ( fval <= 10 )        strcpy( retval, "Near Death" );
    else if ( fval <= 25 )        strcpy( retval, "Mortally Wounded" );
    else if ( fval <= 40 )        strcpy( retval, "Badly Wounded" );
    else if ( fval <= 60 )        strcpy( retval, "Injured" );
    else if ( fval <= 75 )        strcpy( retval, "Hurt" );
    else if ( fval <= 90 )        strcpy( retval, "Bruised" );
    else if ( fval < 100 )        strcpy( retval, "Scratched" );

    return retval;
}

//--------------------------------------------------------------------------------------------
TX_REF chr_get_txtexture_icon_ref( const CHR_REF item )
{
    /// @author BB
    /// @details Get the index to the icon texture (in TxList) that is supposed to be used with this object.
    ///               If none can be found, return the index to the texture of the null icon.

    const std::shared_ptr<Object> &pitem = _currentModule->getObjectHandler()[item];
    if (!pitem) {
        return static_cast<TX_REF>(TX_ICON_NULL);
    }

    const std::shared_ptr<ObjectProfile> &itemProfile = pitem->getProfile();

    // what do we need to draw?
    // the value of spelleffect_type == the skin of the book or -1 for not a spell effect
    bool is_book = (SPELLBOOK == pitem->profile_ref) || (itemProfile->getSpellEffectType() >= 0 && !pitem->draw_icon);

    //bool is_spell_fx = ( itemProfile->getSpellEffectType() >= 0 );     
    //bool draw_book   = ( is_book || ( is_spell_fx && !pitem->draw_icon ) /*|| ( is_spell_fx && INVALID_CHR_REF != pitem->attachedto )*/ ); /// ZF@> uncommented a part because this caused a icon bug when you were morphed and mounted

    if (!is_book)
    {
        return itemProfile->getIcon(pitem->skin);
    }
    else
    {
        return ProfileSystem::get().getSpellBookIcon(itemProfile->getSpellEffectType());
    }
}

//--------------------------------------------------------------------------------------------
Object * chr_update_hide( Object * pchr )
{
    /// @author BB
    /// @details Update the hide state of the character. Should be called every time that
    ///               the state variable in an ai_state_t structure is updated

    if ( nullptr == ( pchr ) ) return pchr;

    pchr->is_hidden = false;
    int8_t hideState = ProfileSystem::get().getProfile(pchr->profile_ref)->getHideState();
    if ( hideState != NOHIDE && hideState == pchr->ai.state )
    {
        pchr->is_hidden = true;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_holding_idsz( const CHR_REF ichr, IDSZ idsz )
{
    /// @author BB
    /// @details check the character's hands for a matching item

    bool found;
    CHR_REF item, tmp_item;
    Object * pchr;

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return INVALID_CHR_REF;
    pchr = _currentModule->getObjectHandler().get( ichr );

    item = INVALID_CHR_REF;
    found = false;

    if ( !found )
    {
        // Check right hand. technically a held item cannot be equipped...
        tmp_item = pchr->holdingwhich[SLOT_RIGHT];

        if ( chr_is_type_idsz( tmp_item, idsz ) )
        {
            found = true;
            item = tmp_item;
        }
    }

    if ( !found )
    {
        // Check left hand. technically a held item cannot be equipped...
        tmp_item = pchr->holdingwhich[SLOT_LEFT];

        if ( chr_is_type_idsz( tmp_item, idsz ) )
        {
            found = true;
            item = tmp_item;
        }
    }

    return item;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_has_item_idsz( const CHR_REF ichr, IDSZ idsz, bool equipped )
{
    /// @author BB
    /// @details is ichr holding an item matching idsz, or is such an item in his pack?
    ///               return the index of the found item, or INVALID_CHR_REF if not found. Also return
    ///               the previous pack item in *pack_last, or INVALID_CHR_REF if it was not in a pack.

    bool found;
    CHR_REF item;
    Object * pchr;

    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return INVALID_CHR_REF;
    pchr = _currentModule->getObjectHandler().get( ichr );

    // Check the pack
    item       = INVALID_CHR_REF;
    found      = false;

    if ( !found )
    {
        item = chr_holding_idsz( ichr, idsz );
        found = _currentModule->getObjectHandler().exists( item );
    }

    if ( !found )
    {
        item = Inventory::findItem( ichr, idsz, equipped );
        found = _currentModule->getObjectHandler().exists( item );
    }

    return item;
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
void chr_set_redshift( Object * pchr, const int rs )
{
    if ( nullptr == ( pchr ) ) return;

    pchr->inst.redshift = CLIP( rs, 0, 9 );

    chr_instance_t::update_ref(pchr->inst, pchr->enviro.grid_level, false );
}

//--------------------------------------------------------------------------------------------
void chr_set_grnshift( Object * pchr, const int gs )
{
    if ( nullptr == ( pchr ) ) return;

    pchr->inst.grnshift = CLIP( gs, 0, 9 );

    chr_instance_t::update_ref(pchr->inst, pchr->enviro.grid_level, false );
}

//--------------------------------------------------------------------------------------------
void chr_set_blushift( Object * pchr, const int bs )
{
    if ( nullptr == ( pchr ) ) return;

    pchr->inst.blushift = CLIP( bs, 0, 9 );

    chr_instance_t::update_ref(pchr->inst, pchr->enviro.grid_level, false );
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
bool chr_set_maxaccel( Object * pchr, float new_val )
{
    bool retval = false;
    float ftmp;

    if ( nullptr == ( pchr ) ) return retval;

    if ( 0.0f == pchr->maxaccel_reset )
    {
        pchr->maxaccel_reset = new_val;
        pchr->maxaccel       = new_val;
    }
    else
    {
        ftmp = pchr->maxaccel / pchr->maxaccel_reset;
        pchr->maxaccel_reset = new_val;
        pchr->maxaccel = ftmp * pchr->maxaccel_reset;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
Object * chr_set_ai_state( Object * pchr, int state )
{
    if ( nullptr == ( pchr ) ) return pchr;

    pchr->ai.state = state;

    chr_update_hide( pchr );

    return pchr;
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
static void chr_init_size( Object * pchr, const std::shared_ptr<ObjectProfile> &profile)
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
