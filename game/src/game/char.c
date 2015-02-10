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

#include "egolib/_math.h"
#include "egolib/math/Random.hpp"

#include "game/mad.h"
#include "game/player.h"
#include "game/script.h"
#include "game/graphic_billboard.h"
#include "game/graphic_texture.h"
#include "game/renderer_2d.h"
#include "game/renderer_3d.h"
#include "game/menu.h"
#include "game/input.h"
#include "game/game.h"
#include "game/ui.h"
#include "game/collision.h"                  //Only or detach_character_from_platform()
#include "game/obj_BSP.h"
#include "game/egoboo.h"
#include "game/module/Passage.hpp"
#include "game/audio/AudioSystem.hpp"
#include "game/profiles/ProfileSystem.hpp"
#include "game/module/Module.hpp"

#include "game/module/ObjectHandler.hpp"
#include "game/EncList.h"
#include "game/PrtList.h"
#include "game/mesh.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_grab_data;
typedef struct s_grab_data grab_data_t;

struct s_chr_anim_data;
typedef struct s_chr_anim_data chr_anim_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static IDSZ    inventory_idsz[INVEN_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Stack<team_t, TEAM_MAX> TeamStack;
int chr_stoppedby_tests = 0;
int chr_pressure_tests = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_grab_data
{
    CHR_REF ichr;
    fvec3_t diff;
    float   diff2_hrz;
    float   diff2_vrt;
    bool  too_dark, too_invis;
};

static int grab_data_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_chr_anim_data
{
    bool allowed;
    int    action;
    int    lip;
    float  speed;
};

static int cmp_chr_anim_data( void const * vp_lhs, void const * vp_rhs );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static CHR_REF chr_pack_has_a_stack( const CHR_REF item, const CHR_REF character );
//static bool  chr_add_pack_item( const CHR_REF item, const CHR_REF character );
//static CHR_REF chr_get_pack_item( const CHR_REF character, grip_offset_t grip_off, bool ignorekurse );

static bool set_weapongrip( const CHR_REF iitem, const CHR_REF iholder, Uint16 vrt_off );

static BBOARD_REF chr_add_billboard( const CHR_REF ichr, Uint32 lifetime_secs );

static GameObject * resize_one_character( GameObject * pchr );
//static void    resize_all_characters();

static bool  chr_free( GameObject * pchr );

static int get_grip_verts( Uint16 grip_verts[], const CHR_REF imount, int vrt_offset );

static bool apply_one_character_matrix( GameObject * pchr, matrix_cache_t * mcache );
static bool apply_one_weapon_matrix( GameObject * pweap, matrix_cache_t * mcache );

static int convert_grip_to_local_points( GameObject * pholder, Uint16 grip_verts[], fvec4_t   dst_point[] );
static int convert_grip_to_global_points( const CHR_REF iholder, Uint16 grip_verts[], fvec4_t   dst_point[] );

// definition that is consistent with using it as a callback in qsort() or some similar function
static int  cmp_matrix_cache( const void * vlhs, const void * vrhs );

static void cleanup_one_character( GameObject * pchr );

//static void chr_log_script_time( const CHR_REF ichr );

static bool update_chr_darkvision( const CHR_REF character );

static fvec2_t chr_get_mesh_diff( GameObject * pchr, float test_pos[], float center_pressure );
static float   chr_get_mesh_pressure( GameObject * pchr, float test_pos[] );

static egolib_rv chr_invalidate_child_instances( GameObject * pchr );

static void chr_set_enviro_grid_level( GameObject * pchr, const float level );
static void chr_log_script_time( const CHR_REF ichr );

static bool chr_download_profile(GameObject * pchr, const std::shared_ptr<ObjectProfile> &profile);

static bool chr_get_environment( GameObject * pchr );

GameObject * chr_config_do_init( GameObject * pchr );
static GameObject * chr_config_do_active( GameObject * pchr );
static int chr_change_skin( const CHR_REF character, const SKIN_T skin );
static void switch_team_base( const CHR_REF character, const TEAM_REF team_new, const bool permanent );

static egolib_rv matrix_cache_needs_update( GameObject * pchr, matrix_cache_t * pmc );
static bool apply_matrix_cache( GameObject * pchr, matrix_cache_t * mc_tmp );
static bool chr_get_matrix_cache( GameObject * pchr, matrix_cache_t * mc_tmp );

static void move_one_character_do_floor_friction( GameObject * pchr );
static void move_one_character_do_voluntary( GameObject * pchr );
static void move_one_character( GameObject * pchr );
static void move_one_character_do_animation( GameObject * pchr );
static void move_one_character_do_z_motion( GameObject * pchr );
static bool move_one_character_integrate_motion( GameObject * pchr );
static bool move_one_character_integrate_motion_attached( GameObject * pchr );

static float set_character_animation_rate( GameObject * pchr );

static bool chr_handle_madfx( GameObject * pchr );
static bool chr_do_latch_button( GameObject * pchr );
static bool chr_do_latch_attack( GameObject * pchr, slot_t which_slot );

static breadcrumb_t * chr_get_last_breadcrumb( GameObject * pchr );
static void chr_init_size( GameObject * pchr, const std::shared_ptr<ObjectProfile> &profile);

//--------------------------------------------------------------------------------------------
egolib_rv flash_character_height( const CHR_REF character, Uint8 valuelow, Sint16 low,
                                  Uint8 valuehigh, Sint16 high )
{
    /// @author ZZ
    /// @details This function sets a character's lighting depending on vertex height...
    ///    Can make feet dark and head light...

    Uint32 cnt;
    Sint16 z;

    mad_t * pmad;
    chr_instance_t * pinst;

    pinst = chr_get_pinstance( character );
    if ( NULL == pinst ) return rv_error;

    pmad = chr_get_pmad( character );
    if ( NULL == pmad ) return rv_error;

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
void chr_set_enviro_grid_level( GameObject * pchr, const float level )
{
    if ( nullptr == ( pchr ) ) return;

    if ( level != pchr->enviro.grid_level )
    {
        pchr->enviro.grid_level = level;

        chr_instance_apply_reflection_matrix( &( pchr->inst ), level );
    }
}

//--------------------------------------------------------------------------------------------
bool chr_copy_enviro( GameObject * chr_psrc, GameObject * chr_pdst )
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
void keep_weapons_with_holders()
{
    /// @author ZZ
    /// @details This function keeps weapons near their holders

    for(const std::shared_ptr<GameObject> &pchr : _gameObjects.iterator())
    {
        CHR_REF iattached = pchr->attachedto;

        if ( _gameObjects.exists( iattached ) )
        {
            GameObject * pattached = _gameObjects.get( iattached );

            // Keep in hand weapons with iattached
            if ( chr_matrix_valid( pchr.get() ) )
            {
                pchr->setPosition(mat_getTranslate_v(pchr->inst.matrix.v));
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
            if ( !_gameObjects.exists( pchr->inwhich_inventory ) )
            {
                PACK_BEGIN_LOOP( pchr->inventory, pitem, iitem )
                {
                    pitem->setPosition(pchr->getPosition());

                    // Copy olds to make SendMessageNear work
                    pitem->pos_old = pchr->pos_old;
                }
                PACK_END_LOOP();
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix( const CHR_REF ichr )
{
    /// @author ZZ
    /// @details This function sets one character's matrix

    GameObject * pchr;
    chr_instance_t * pinst;

    if ( !_gameObjects.exists( ichr ) ) return;
    pchr = _gameObjects.get( ichr );
    pinst = &( pchr->inst );

    // invalidate this matrix
    pinst->matrix_cache.matrix_valid = false;

    if ( pchr->is_overlay )
    {
        // This character is an overlay and its ai.target points to the object it is overlaying
        // Overlays are kept with their target...
        if ( _gameObjects.exists( pchr->ai.target ) )
        {
            GameObject * ptarget = _gameObjects.get( pchr->ai.target );

            pchr->setPosition(ptarget->getPosition());

            // copy the matrix
            CopyMatrix( &( pinst->matrix ), &( ptarget->inst.matrix ) );

            // copy the matrix data
            pinst->matrix_cache = ptarget->inst.matrix_cache;
        }
    }
    else
    {
        if ( pchr->stickybutt )
        {
            mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(
                pinst->matrix.v,
                pchr->fat, pchr->fat, pchr->fat,
                TO_TURN( pchr->ori.facing_z ),
                TO_TURN( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET ),
                TO_TURN( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET ),
                pchr->getPosition().x, pchr->getPosition().y, pchr->getPosition().z );
        }
        else
        {
            mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(
                pinst->matrix.v,
                pchr->fat, pchr->fat, pchr->fat,
                TO_TURN( pchr->ori.facing_z ),
                TO_TURN( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET ),
                TO_TURN( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET ),
                pchr->getPosition().x, pchr->getPosition().y, pchr->getPosition().z );
        }

        pinst->matrix_cache.valid        = true;
        pinst->matrix_cache.matrix_valid = true;
        pinst->matrix_cache.type_bits    = MAT_CHARACTER;

        pinst->matrix_cache.self_scale.x = pchr->fat;
        pinst->matrix_cache.self_scale.y = pchr->fat;
        pinst->matrix_cache.self_scale.z = pchr->fat;

        pinst->matrix_cache.rotate.x = CLIP_TO_16BITS( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET );
        pinst->matrix_cache.rotate.y = CLIP_TO_16BITS( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET );
        pinst->matrix_cache.rotate.z = pchr->ori.facing_z;

        pinst->matrix_cache.pos = pchr->getPosition();
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void chr_log_script_time( const CHR_REF ichr )
{
    // log the amount of script time that this object used up

    GameObject * pchr;
    vfs_FILE * ftmp;

    if ( !_gameObjects.exists( ichr ) ) return;
    pchr = _gameObjects.get( ichr );

    if ( pchr->ai._clkcount <= 0 ) return;

    ObjectProfile *profile = chr_get_ppro(ichr);
    if ( nullptr == profile ) return;

    ftmp = vfs_openAppendB("/debug/script_timing.txt");
    if ( NULL != ftmp )
    {
        vfs_printf( ftmp, "update == %d\tindex == %d\tname == \"%s\"\tclassname == \"%s\"\ttotal_time == %e\ttotal_calls == %f\n",
                 update_wld, REF_TO_INT( ichr ), pchr->Name, profile->getClassName().c_str(),
                 pchr->ai._clktime, pchr->ai._clkcount );
        vfs_close( ftmp );
    }
}

//--------------------------------------------------------------------------------------------
void free_one_character_in_game( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function sticks a character back on the free character stack
    ///
    /// @note This should only be called by cleanup_all_characters() or free_inventory_in_game()

    size_t  cnt;
    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return;
    pchr = _gameObjects.get( character );

    ObjectProfile *profile = chr_get_ppro(character);
    if ( nullptr == profile ) return;

    // Remove from stat list
    if ( pchr->show_stats )
    {
        bool stat_found;

        pchr->show_stats = false;

        stat_found = false;
        for ( cnt = 0; cnt < StatusList.count; cnt++ )
        {
            if ( StatusList.lst[cnt].who == character )
            {
                stat_found = true;
                break;
            }
        }

        if ( stat_found )
        {
            for ( cnt++; cnt < StatusList.count; cnt++ )
            {
                SWAP( status_list_element_t, StatusList.lst[cnt-1], StatusList.lst[cnt] );
            }
            StatusList.count--;
        }
    }

    // Make sure everyone knows it died
    for(const std::shared_ptr<GameObject> &chr : _gameObjects.iterator())
    {
        ai_state_t * pai;

        //Don't do ourselves or terminated characters
        if ( chr->terminateRequested || chr->getCharacterID() == character ) continue;
        pai = chr_get_pai( chr->getCharacterID() );

        if ( pai->target == character )
        {
            SET_BIT( pai->alert, ALERTIF_TARGETKILLED );
            pai->target = chr->getCharacterID();
        }

        if ( chr_get_pteam( chr->getCharacterID() )->leader == character )
        {
            SET_BIT( pai->alert, ALERTIF_LEADERKILLED );
        }
    }

    // Handle the team
    if ( pchr->alive && !profile->isInvincible() && TeamStack.lst[pchr->team_base].morale > 0 )
    {
        TeamStack.lst[pchr->team_base].morale--;
    }

    if ( TeamStack.lst[pchr->team].leader == character )
    {
        TeamStack.lst[pchr->team].leader = TEAM_NOLEADER;
    }

    // remove any attached particles
    disaffirm_attached_particles( character );

    // actually get rid of the character
    _gameObjects.remove(character);

    //Stop any looped sounds allocated to this character
    _audioSystem.stopObjectLoopingSounds(character);
}

//--------------------------------------------------------------------------------------------
void free_inventory_in_game( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function frees every item in the character's inventory
    ///
    /// @note this should only be called by cleanup_all_characters()

    int i;

    if ( !_gameObjects.exists( character ) ) return;

    PACK_BEGIN_LOOP( _gameObjects.get(character)->inventory, pitem, iitem )
    {
        free_one_character_in_game( iitem );
    }
    PACK_END_LOOP();

    // set the inventory to the "empty" state
    for ( i = 0; i < MAXINVENTORY; i++ )
    {
        _gameObjects.get(character)->inventory[i] = INVALID_CHR_REF;
    }
}

//--------------------------------------------------------------------------------------------
prt_t * place_particle_at_vertex( prt_t * pprt, const CHR_REF character, int vertex_offset )
{
    /// @author ZZ
    /// @details This function sets one particle's position to be attached to a character.
    ///    It will kill the particle if the character is no longer around

    int     vertex;
    fvec4_t point[1], nupoint[1];

    GameObject * pchr;

    if ( !DEFINED_PPRT( pprt ) ) return pprt;

    if ( !_gameObjects.exists( character ) )
    {
        goto place_particle_at_vertex_fail;
    }
    pchr = _gameObjects.get( character );

    // Check validity of attachment
    if ( _gameObjects.exists( pchr->inwhich_inventory ) )
    {
        goto place_particle_at_vertex_fail;
    }

    // Do we have a matrix???
    if ( !chr_matrix_valid( pchr ) )
    {
        chr_update_matrix( pchr, true );
    }

    // Do we have a matrix???
    if ( chr_matrix_valid( pchr ) )
    {
        // Transform the weapon vertex_offset from model to world space
        mad_t * pmad = chr_get_pmad( character );

        if ( vertex_offset == GRIP_ORIGIN )
        {
            fvec3_t tmp_pos;

            tmp_pos.x = pchr->inst.matrix.CNV( 3, 0 );
            tmp_pos.y = pchr->inst.matrix.CNV( 3, 1 );
            tmp_pos.z = pchr->inst.matrix.CNV( 3, 2 );

            prt_t::set_pos(pprt, tmp_pos);

            return pprt;
        }

        vertex = 0;
        if ( NULL != pmad )
        {
            vertex = (( int )pchr->inst.vrt_count ) - vertex_offset;

            // do the automatic update
            chr_instance_update_vertices( &( pchr->inst ), vertex, vertex, false );

            // Calculate vertex_offset point locations with linear interpolation and other silly things
            point[0].x = pchr->inst.vrt_lst[vertex].pos[XX];
            point[0].y = pchr->inst.vrt_lst[vertex].pos[YY];
            point[0].z = pchr->inst.vrt_lst[vertex].pos[ZZ];
            point[0].w = 1.0f;
        }
        else
        {
            point[0].x =
                point[0].y =
                    point[0].z = 0.0f;
            point[0].w = 1.0f;
        }

        // Do the transform
        pchr->inst.matrix.transform(point, nupoint, 1);

        prt_t::set_pos(pprt, fvec3_t(nupoint[0][kX],nupoint[0][kY],nupoint[0][kZ]));
    }
    else
    {
        // No matrix, so just wing it...
        prt_t::set_pos(pprt, pchr->getPosition());
    }

    return pprt;

place_particle_at_vertex_fail:

    prt_t::request_terminate( pprt );

    return NULL;
}

//--------------------------------------------------------------------------------------------
void update_all_character_matrices()
{
    /// @author ZZ
    /// @details This function makes all of the character's matrices

    // just call chr_update_matrix on every character
    for(const std::shared_ptr<GameObject> &pchr : _gameObjects.iterator())
    {
        chr_update_matrix( pchr.get(), true );
    }
}

//--------------------------------------------------------------------------------------------
void free_all_chraracters()
{
    /// @author ZZ
    /// @details This function resets the character allocation list

    //Remove all enchants
    for (size_t iTmp = 0; iTmp < MAX_ENC; iTmp++ )
    {
        remove_enchant( iTmp, nullptr );
    }

    // free all the characters
    _gameObjects.clear();

    // free_all_players
    PlaStack.count = 0;
    local_stats.player_count = 0;
    local_stats.noplayers = true;

    // free_all_stats
    StatusList.count = 0;
}

//--------------------------------------------------------------------------------------------
float chr_get_mesh_pressure( GameObject * pchr, float test_pos[] )
{
    float retval = 0.0f;
    float radius = 0.0f;
    const float *loc_test_pos = NULL;

    if ( nullptr == ( pchr ) ) return retval;

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) return retval;

    // deal with the optional parameters
    loc_test_pos = ( NULL == test_pos ) ? pchr->getPosition().v : test_pos;
    if ( NULL == loc_test_pos ) return 0;

    // calculate the radius based on whether the character is on camera
    radius = 0.0f;
    if ( cfg.dev_mode && !SDL_KEYDOWN( keyb, SDLK_F8 ) )
    {
        ego_tile_info_t * ptile = ego_mesh_get_ptile( PMesh, pchr->onwhichgrid );

        if ( NULL != ptile && ptile->inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = ego_mesh_get_pressure( PMesh, test_pos, radius, pchr->stoppedby );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
fvec2_t chr_get_mesh_diff( GameObject * pchr, float test_pos[], float center_pressure )
{
    fvec2_t retval = fvec2_t::zero;
    float radius;
    const float * loc_test_pos = NULL;

    if ( nullptr == ( pchr ) ) return retval;

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) return retval;

    // deal with the optional parameters
    loc_test_pos = ( NULL == test_pos ) ? pchr->getPosition().v : test_pos;
    if ( NULL == loc_test_pos ) return retval;

    // calculate the radius based on whether the character is on camera
    radius = 0.0f;
    if ( cfg.dev_mode && !SDL_KEYDOWN( keyb, SDLK_F8 ) )
    {
        ego_tile_info_t * ptile = ego_mesh_get_ptile( PMesh, pchr->onwhichgrid );

        if ( NULL != ptile && ptile->inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = ego_mesh_get_diff( PMesh, loc_test_pos, radius, center_pressure, pchr->stoppedby );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD chr_hit_wall( GameObject * pchr, const float test_pos[], float nrm[], float * pressure, mesh_wall_data_t * pdata )
{
    /// @author ZZ
    /// @details This function returns nonzero if the character hit a wall that the
    ///    character is not allowed to cross

    BIT_FIELD    retval;
    float        radius;
    const float * loc_test_pos = NULL;

    if ( nullptr == ( pchr ) ) return 0;

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) return 0;

    // deal with the optional parameters
    loc_test_pos = ( NULL == test_pos ) ? pchr->getPosition().v : test_pos;
    if ( NULL == loc_test_pos ) return 0;

    // calculate the radius based on whether the character is on camera
    radius = 0.0f;
    if ( cfg.dev_mode && !SDL_KEYDOWN( keyb, SDLK_F8 ) )
    {
        ego_tile_info_t * ptile = ego_mesh_get_ptile( PMesh, pchr->onwhichgrid );

        if ( NULL != ptile && ptile->inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = ego_mesh_hit_wall( PMesh, loc_test_pos, radius, pchr->stoppedby, nrm, pressure, pdata );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests  += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD GameObjectest_wall( GameObject * pchr, const float test_pos[], mesh_wall_data_t * pdata )
{
    /// @author ZZ
    /// @details This function returns nonzero if the character hit a wall that the
    ///    character is not allowed to cross

    BIT_FIELD retval;
    float  radius;

    if ( !ACTIVE_PCHR( pchr ) ) return EMPTY_BIT_FIELD;

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) return EMPTY_BIT_FIELD;

    const float *loc_test_pos = ( NULL == test_pos ) ? pchr->getPosition().v : test_pos;
    if(NULL == loc_test_pos) {
        return EMPTY_BIT_FIELD;
    }

    // calculate the radius based on whether the character is on camera
    radius = 0.0f;
    if ( cfg.dev_mode && !SDL_KEYDOWN( keyb, SDLK_F8 ) )
    {
        ego_tile_info_t * ptile = ego_mesh_get_ptile( PMesh, pchr->onwhichgrid );
        if ( NULL != ptile && ptile->inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    // do the wall test
    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = ego_mesh_test_wall( PMesh, loc_test_pos, radius, pchr->stoppedby, pdata );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
void reset_character_accel( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function fixes a character's max acceleration

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;
    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return;
    pchr = _gameObjects.get( character );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Okay, remove all acceleration enchants
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        enc_remove_add( ienc_now, ADDACCEL );

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    // Set the starting value
    pchr->maxaccel_reset = 0;

    ObjectProfile *profile = chr_get_ppro(character);
    if ( nullptr != profile )
    {
        pchr->maxaccel = pchr->maxaccel_reset = profile->getSkinInfo(pchr->skin).maxAccel;
    }

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Put the acceleration enchants back on
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        enc_apply_add( ienc_now, ADDACCEL, enc_get_ieve( ienc_now ) );

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
}

//--------------------------------------------------------------------------------------------
bool detach_character_from_mount( const CHR_REF character, Uint8 ignorekurse, Uint8 doshop )
{
    /// @author ZZ
    /// @details This function drops an item

    CHR_REF mount;
    Uint16  hand;
    bool  inshop;
    GameObject * pchr, * pmount;

    // Make sure the character is valid
    if ( !_gameObjects.exists( character ) ) return false;
    pchr = _gameObjects.get( character );

    // Make sure the character is mounted
    mount = _gameObjects.get(character)->attachedto;
    if ( !_gameObjects.exists( mount ) ) return false;
    pmount = _gameObjects.get( mount );

    // Don't allow living characters to drop kursed weapons
    if ( !ignorekurse && pchr->iskursed && pmount->alive && pchr->isitem )
    {
        SET_BIT( pchr->ai.alert, ALERTIF_NOTDROPPED );
        return false;
    }

    // set the dismount timer
    if ( !pchr->isitem ) pchr->dismount_timer  = PHYS_DISMOUNT_TIME;
    pchr->dismount_object = mount;

    // Figure out which hand it's in
    hand = pchr->inwhich_slot;

    // Rip 'em apart
    pchr->attachedto = INVALID_CHR_REF;
    if ( pmount->holdingwhich[SLOT_LEFT] == character )
        pmount->holdingwhich[SLOT_LEFT] = INVALID_CHR_REF;

    if ( pmount->holdingwhich[SLOT_RIGHT] == character )
        pmount->holdingwhich[SLOT_RIGHT] = INVALID_CHR_REF;

    if ( pchr->alive )
    {
        // play the falling animation...
        chr_play_action( pchr, ACTION_JB + hand, false );
    }
    else if ( pchr->inst.action_which < ACTION_KA || pchr->inst.action_which > ACTION_KD )
    {
        // play the "killed" animation...
        chr_play_action( pchr, generate_randmask( ACTION_KA, 3 ), false );
        chr_instance_set_action_keep( &( pchr->inst ), true );
    }

    // Set the positions
    if ( chr_matrix_valid( pchr ) )
    {
        pchr->setPosition(mat_getTranslate_v(pchr->inst.matrix.v));
    }
    else
    {
        pchr->setPosition(pmount->getPosition());
    }

    // Make sure it's not dropped in a wall...
    if (EMPTY_BIT_FIELD != GameObjectest_wall(pchr, NULL, NULL))
    {
        fvec3_t pos_tmp = pmount->getPosition();
        pos_tmp.z = pchr->getPosZ();

        pchr->setPosition(pos_tmp);

        chr_update_breadcrumb(pchr, true);
    }

    // Check for shop passages
    inshop = false;
    if ( doshop )
    {
        inshop = do_shop_drop( mount, character );
    }

    // Make sure it works right
    pchr->hitready = true;
    if ( inshop )
    {
        // Drop straight down to avoid theft
        pchr->vel.x = 0;
        pchr->vel.y = 0;
    }
    else
    {
        pchr->vel.x = pmount->vel.x;
        pchr->vel.y = pmount->vel.y;
    }

    pchr->vel.z = DROPZVEL;

    // Turn looping off
    chr_instance_set_action_loop( &( pchr->inst ), false );

    // Reset the team if it is a mount
    if ( pmount->isMount() )
    {
        pmount->team = pmount->team_base;
        SET_BIT( pmount->ai.alert, ALERTIF_DROPPED );
    }

    pchr->team = pchr->team_base;
    SET_BIT( pchr->ai.alert, ALERTIF_DROPPED );

    // Reset transparency
    if ( pchr->isitem && pmount->transferblend )
    {
        ENC_REF ienc_now, ienc_nxt;
        size_t  ienc_count;

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // Okay, reset transparency
        ienc_now = pchr->firstenchant;
        ienc_count = 0;
        while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
        {
            ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

            enc_remove_set( ienc_now, SETALPHABLEND );
            enc_remove_set( ienc_now, SETLIGHTBLEND );

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

        pchr->setAlpha(pchr->getProfile()->getAlpha());
        pchr->setLight(pchr->getProfile()->getLight());

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // apply the blend enchants
        ienc_now = pchr->firstenchant;
        ienc_count = 0;
        while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
        {
            PRO_REF ipro = enc_get_ipro( ienc_now );
            ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

            if ( _profileSystem.isValidProfileID( ipro ) )
            {
                enc_apply_set( ienc_now, SETALPHABLEND, ipro );
                enc_apply_set( ienc_now, SETLIGHTBLEND, ipro );
            }

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
    }

    // Set twist
    pchr->ori.map_twist_facing_y = MAP_TURN_OFFSET;
    pchr->ori.map_twist_facing_x = MAP_TURN_OFFSET;

    // turn off keeping, unless the object is dead
    if ( !pchr->alive )
    {
        // the object is dead. play the killed animation and make it freeze there
        chr_play_action( pchr, generate_randmask( ACTION_KA, 3 ), false );
        chr_instance_set_action_keep( &( pchr->inst ), true );
    }
    else
    {
        // play the jump animation, and un-keep it
        chr_play_action( pchr, ACTION_JA, true );
        chr_instance_set_action_keep( &( pchr->inst ), false );
    }

    chr_update_matrix( pchr, true );

    return true;
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function fixes an item's transparency

    CHR_REF mount;
    GameObject * pchr, * pmount;

    // Make sure the character is valid
    if ( !_gameObjects.exists( character ) ) return;
    pchr = _gameObjects.get( character );

    // Make sure the character is mounted
    mount = _gameObjects.get(character)->attachedto;
    if ( !_gameObjects.exists( mount ) ) return;
    pmount = _gameObjects.get( mount );

    if ( pchr->isitem && pmount->transferblend )
    {
        ENC_REF ienc_now, ienc_nxt;
        size_t  ienc_count;

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // Okay, reset transparency
        ienc_now = pchr->firstenchant;
        ienc_count = 0;
        while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
        {
            ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

            enc_remove_set( ienc_now, SETALPHABLEND );
            enc_remove_set( ienc_now, SETLIGHTBLEND );

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

        pchr->setAlpha(pchr->getProfile()->getAlpha());
        pchr->setLight(pchr->getProfile()->getLight());

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        ienc_now = pchr->firstenchant;
        ienc_count = 0;
        while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
        {
            PRO_REF ipro = enc_get_ipro( ienc_now );

            ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

            if ( _profileSystem.isValidProfileID( ipro ) )
            {
                enc_apply_set( ienc_now, SETALPHABLEND, ipro );
                enc_apply_set( ienc_now, SETLIGHTBLEND, ipro );
            }

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
    }
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

    GameObject * prider, * pmount;

    // Make sure the character/item is valid
    if ( !_gameObjects.exists( irider ) ) return rv_error;
    prider = _gameObjects.get( irider );

    // Make sure the holder/mount is valid
    if ( !_gameObjects.exists( imount ) ) return rv_error;
    pmount = _gameObjects.get( imount );

    //Don't attach a character to itself!
    if(irider == imount) {
        return rv_fail;
    }

    ObjectProfile *mountProfile = chr_get_ppro(imount);
    if ( nullptr == mountProfile ) return rv_error;

    // do not deal with packed items at this time
    // this would have to be changed to allow for pickpocketing
    if ( _gameObjects.exists( prider->inwhich_inventory ) || _gameObjects.exists( pmount->inwhich_inventory ) ) return rv_fail;

    // make a reasonable time for the character to remount something
    // for characters jumping out of pots, etc
    if ( imount == prider->dismount_object && prider->dismount_timer > 0 ) return rv_fail;

    // Figure out which slot this grip_off relates to
    slot_t slot = grip_offset_to_slot( grip_off );

    // Make sure the the slot is valid
    if ( !mountProfile->isSlotValid(slot) ) return rv_fail;

    // This is a small fix that allows special grabbable mounts not to be mountable while
    // held by another character (such as the magic carpet for example)
    // ( this is an example of a test that should not be done here )
    if ( pmount->isMount() && _gameObjects.exists( pmount->attachedto ) ) return rv_fail;

    // Put 'em together
    prider->inwhich_slot       = slot;
    prider->attachedto         = imount;
    pmount->holdingwhich[slot] = irider;

    // set the grip vertices for the irider
    set_weapongrip( irider, imount, grip_off );

    chr_update_matrix( prider, true );

    prider->setPosition(mat_getTranslate_v(prider->inst.matrix.v));

    prider->enviro.inwater  = false;
    prider->jump_timer = JUMPDELAY * 4;

    // Run the held animation
    if ( pmount->isMount() && ( GRIP_ONLY == grip_off ) )
    {
        // Riding imount

        if ( _gameObjects.exists( prider->holdingwhich[SLOT_LEFT] ) || _gameObjects.exists( prider->holdingwhich[SLOT_RIGHT] ) )
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
        chr_instance_set_action_loop( &( prider->inst ), true );
    }
    else if ( prider->alive )
    {
        /// @note ZF@> hmm, here is the torch holding bug. Removing
        /// the interpolation seems to fix it...
        chr_play_action( prider, ACTION_MM + slot, false );

        chr_instance_remove_interpolation( &( prider->inst ) );

        // set the action to keep for items
        if ( prider->isitem )
        {
            // Item grab
            chr_instance_set_action_keep( &( prider->inst ), true );
        }
    }

    // Set the team
    if ( prider->isitem )
    {
        prider->team = pmount->team;

        // Set the alert
        if ( prider->alive )
        {
            SET_BIT( prider->ai.alert, ALERTIF_GRABBED );
        }
    }

    if ( pmount->isMount() )
    {
        pmount->team = prider->team;

        // Set the alert
        if ( !pmount->isitem && pmount->alive )
        {
            SET_BIT( pmount->ai.alert, ALERTIF_GRABBED );
        }
    }

    // It's not gonna hit the floor
    prider->hitready = false;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool inventory_add_item( const CHR_REF ichr, const CHR_REF item, Uint8 inventory_slot, const bool ignorekurse )
{
    /// @author ZF
    /// @details This adds a new item into the specified inventory slot. Fails if there already is an item there.
    ///               If the specified inventory slot is MAXINVENTORY, it will find the first free inventory slot.

    CHR_REF stack;
    GameObject *pchr, *pitem;
    int newammo;

    //valid character?
    if ( !_gameObjects.exists( ichr ) || !_gameObjects.exists( item ) ) return false;
    pchr = _gameObjects.get( ichr );
    pitem = _gameObjects.get( item );
    ObjectProfile *itemProfile = chr_get_ppro(item);

    //try get the first free slot found?
    if ( inventory_slot >= MAXINVENTORY )
    {
        int i;
        for ( i = 0; i < GameObject::MAXNUMINPACK; i++ )
        {
            if ( !_gameObjects.exists( pchr->inventory[i] ) )
            {
                //found a free slot
                inventory_slot = i;
                break;
            }
        }

        //did we find one?
        if ( i == MAXINVENTORY ) return false;
    }

    //don't override existing items
    if ( _gameObjects.exists( pchr->inventory[inventory_slot] ) ) return false;

    // don't allow sub-inventories
    if ( _gameObjects.exists( pitem->inwhich_inventory ) ) return false;

    //kursed?
    if ( pitem->iskursed && !ignorekurse )
    {
        // Flag the item as not put away
        SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
        if ( pchr->islocalplayer ) DisplayMsg_printf( "%s is sticky...", chr_get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ) );
        return false;
    }

    //too big item?
    if ( itemProfile->isBigItem() )
    {
        SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
        if ( pchr->islocalplayer ) DisplayMsg_printf( "%s is too big to be put away...", chr_get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ) );
        return false;
    }

    //put away inhand item
    stack = chr_pack_has_a_stack( item, ichr );
    if ( _gameObjects.exists( stack ) )
    {
        // We found a similar, stackable item in the pack
        GameObject  * pstack      = _gameObjects.get( stack );
        ObjectProfile *stackProfile = chr_get_ppro(stack);

        // reveal the name of the item or the stack
        if ( pitem->nameknown || pstack->nameknown )
        {
            pitem->nameknown  = true;
            pstack->nameknown = true;
        }

        // reveal the usage of the item or the stack
        if ( itemProfile->isUsageKnown() || stackProfile->isUsageKnown() )
        {
            itemProfile->makeUsageKnown();
            stackProfile->makeUsageKnown();
        }

        // add the item ammo to the stack
        newammo = pitem->ammo + pstack->ammo;
        if ( newammo <= pstack->ammomax )
        {
            // All transfered, so kill the in hand item
            pstack->ammo = newammo;
            detach_character_from_mount( item, true, false );
            _gameObjects.remove(item);
        }
        else
        {
            // Only some were transfered,
            pitem->ammo     = pitem->ammo + pstack->ammo - pstack->ammomax;
            pstack->ammo    = pstack->ammomax;
            SET_BIT( pchr->ai.alert, ALERTIF_TOOMUCHBAGGAGE );
        }
    }
    else
    {
        //@todo: implement weight check here
        // Make sure we have room for another item
        //if ( pchr_pack->count >= GameObject::MAXNUMINPACK )
        // {
        //    SET_BIT( pchr->ai.alert, ALERTIF_TOOMUCHBAGGAGE );
        //    return false;
        //}

        // Take the item out of hand
        detach_character_from_mount( item, true, false );

        // clear the dropped flag
        UNSET_BIT( pitem->ai.alert, ALERTIF_DROPPED );

        //now put the item into the inventory
        pitem->attachedto = INVALID_CHR_REF;
        pitem->inwhich_inventory = ichr;
        pchr->inventory[inventory_slot] = item;

        // fix the flags
        if ( itemProfile->isEquipment() )
        {
            SET_BIT( pitem->ai.alert, ALERTIF_PUTAWAY );  // same as ALERTIF_ATLASTWAYPOINT;
        }

        //@todo: add in the equipment code here
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool inventory_swap_item( const CHR_REF ichr, Uint8 inventory_slot, const slot_t grip_off, const bool ignorekurse )
{
    /// @author ZF
    /// @details This function swaps items between the specified inventory slot and the specified grip
    ///               If MAXINVENTORY is specified by inventory_slot, the function will swap with the first item found
    ///               in the inventory

    CHR_REF item, inventory_item;
    GameObject *pchr;
    bool success = false;
    bool inventory_rv;

    //valid character?
    if ( !_gameObjects.exists( ichr ) ) return false;
    pchr = _gameObjects.get( ichr );

    //try get the first used slot found?
    if ( inventory_slot >= MAXINVENTORY )
    {
        int i;
        for ( i = 0; i < GameObject::MAXNUMINPACK; i++ )
        {
            if ( !_gameObjects.exists( pchr->inventory[i] ) )
            {
                //found a free slot
                inventory_slot = i;
                break;
            }
        }
    }

    inventory_item = pchr->inventory[inventory_slot];
    item           = pchr->holdingwhich[grip_off];

    // Make sure everything is hunkydori
    if ( pchr->isitem || _gameObjects.exists( pchr->inwhich_inventory ) ) return false;

    //remove existing item
    if ( _gameObjects.exists( inventory_item ) )
    {
        inventory_rv = inventory_remove_item( ichr, inventory_slot, ignorekurse );
        if ( inventory_rv ) success = true;
    }

    //put in the new item
    if ( _gameObjects.exists( item ) )
    {
        inventory_rv = inventory_add_item( ichr, item, inventory_slot, ignorekurse );
        if ( inventory_rv ) success = true;
    }

    //now put the inventory item into the character's hand
    if ( _gameObjects.exists( inventory_item ) && success )
    {
        GameObject *pitem = _gameObjects.get( inventory_item );
        attach_character_to_mount( inventory_item, ichr, grip_off == SLOT_RIGHT ? GRIP_RIGHT : GRIP_LEFT );

        //fix flags
        UNSET_BIT( pitem->ai.alert, ALERTIF_GRABBED );
        SET_BIT( pitem->ai.alert, ALERTIF_TAKENOUT );
    }

    return success;
}

//--------------------------------------------------------------------------------------------
bool inventory_remove_item( const CHR_REF ichr, const size_t inventory_slot, const bool ignorekurse )
{
    /// @author ZF
    /// @details This function removes the item specified in the inventory slot from the
    ///               character's inventory. Note that you still have to handle it falling out

    CHR_REF item;
    GameObject *pitem;
    GameObject *pholder;

    //ignore invalid slots
    if ( inventory_slot >= MAXINVENTORY )  return false;

    //valid char?
    if ( !_gameObjects.exists( ichr ) ) return false;
    pholder = _gameObjects.get( ichr );
    item = pholder->inventory[inventory_slot];

    //valid item?
    if ( !_gameObjects.exists( item ) ) return false;
    pitem = _gameObjects.get( item );

    //is it kursed?
    if ( pitem->iskursed && !ignorekurse )
    {
        // Flag the last found_item as not removed
        SET_BIT( pitem->ai.alert, ALERTIF_NOTTAKENOUT );  // Same as ALERTIF_NOTPUTAWAY
        if ( pholder->islocalplayer ) DisplayMsg_printf( "%s won't go out!", chr_get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ) );
        return false;
    }

    //no longer in an inventory
    pitem->inwhich_inventory = INVALID_CHR_REF;
    pholder->inventory[inventory_slot] = INVALID_CHR_REF;

    return true;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_pack_has_a_stack( const CHR_REF item, const CHR_REF character )
{
    /// @author ZZ
    /// @details This function looks in the character's pack for an item similar
    ///    to the one given.  If it finds one, it returns the similar item's
    ///    index number, otherwise it returns MAX_CHR.

    CHR_REF istack;
    Uint16  id;
    bool  found;

    GameObject * pitem;

    found  = false;
    istack = INVALID_CHR_REF;

    if ( !_gameObjects.exists( item ) ) return istack;
    pitem = _gameObjects.get( item );
    ObjectProfile *objectProfile = chr_get_ppro( item );

    if ( objectProfile->isStackable() )
    {
        PACK_BEGIN_LOOP( _gameObjects.get(character)->inventory, pstack, istack_new )
        {
            ObjectProfile *stackProfile = chr_get_ppro( istack_new );

            found = stackProfile->isStackable();

            if ( pstack->ammo >= pstack->ammomax )
            {
                found = false;
            }

            // you can still stack something even if the profiles don't match exactly,
            // but they have to have all the same IDSZ properties
            if ( found && ( stackProfile->getSlotNumber() != pitem->profile_ref ) )
            {
                for ( id = 0; id < IDSZ_COUNT && found; id++ )
                {
                    if ( chr_get_idsz( istack_new, id ) != chr_get_idsz( item, id ) )
                    {
                        found = false;
                    }
                }
            }

            if ( found )
            {
                istack = istack_new;
                break;
            }
        }
        PACK_END_LOOP();
    }

    return istack;
}

//--------------------------------------------------------------------------------------------
void drop_keys( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function drops all keys ( [KEYA] to [KEYZ] ) that are in a character's
    ///    inventory ( Not hands ).

    GameObject  * pchr;

    FACING_T direction;
    IDSZ     testa, testz;
    size_t    cnt;

    if ( !_gameObjects.exists( character ) ) return;
    pchr = _gameObjects.get( character );

    // Don't lose keys in pits...
    if ( pchr->getPosZ() <= ( PITDEPTH >> 1 ) ) return;

    // The IDSZs to find
    testa = MAKE_IDSZ( 'K', 'E', 'Y', 'A' );  // [KEYA]
    testz = MAKE_IDSZ( 'K', 'E', 'Y', 'Z' );  // [KEYZ]

    //check each inventory item
    for ( cnt = 0; cnt < MAXINVENTORY; cnt++ )
    {
        IDSZ idsz_parent;
        IDSZ idsz_type;
        TURN_T turn;

        GameObject *pkey;
        CHR_REF ikey = pchr->inventory[cnt];

        //only valid items
        if ( !_gameObjects.exists( ikey ) ) continue;
        pkey = _gameObjects.get( ikey );

        idsz_parent = chr_get_idsz( ikey, IDSZ_PARENT );
        idsz_type   = chr_get_idsz( ikey, IDSZ_TYPE );

        //is it really a key?
        if (( idsz_parent < testa && idsz_parent > testz ) &&
            ( idsz_type < testa && idsz_type > testz ) ) continue;

        direction = RANDIE;
        turn      = TO_TURN( direction );

        //remove it from inventory
        inventory_remove_item( character, cnt, true );

        // fix the attachments
        pkey->dismount_timer         = PHYS_DISMOUNT_TIME;
        pkey->dismount_object        = GET_INDEX_PCHR( pchr );
        pkey->onwhichplatform_ref    = pchr->onwhichplatform_ref;
        pkey->onwhichplatform_update = pchr->onwhichplatform_update;

        // fix some flags
        pkey->hitready               = true;
        pkey->isequipped             = false;
        pkey->ori.facing_z           = direction + ATK_BEHIND;
        pkey->team                   = pkey->team_base;

        // fix the current velocity
        pkey->vel.x                  += turntocos[ turn ] * DROPXYVEL;
        pkey->vel.y                  += turntosin[ turn ] * DROPXYVEL;
        pkey->vel.z                  += DROPZVEL;

        // do some more complicated things
        SET_BIT( pkey->ai.alert, ALERTIF_DROPPED );
        pkey->setPosition(pchr->getPosition());
        move_one_character_get_environment( pkey );
        chr_set_floor_level( pkey, pchr->enviro.floor_level );
    }
}

//--------------------------------------------------------------------------------------------
bool drop_all_items( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function drops all of a character's items
    const std::shared_ptr<GameObject> &pchr = _gameObjects[character];

    //Drop held items
    detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], true, false );
    detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], true, false );

    //simply count the number of items in inventory
    uint8_t pack_count = 0;
    PACK_BEGIN_LOOP( pchr->inventory, pitem, item )
    {
        pack_count++;
    }
    PACK_END_LOOP();

    //Don't continue if we have nothing to drop
    if(pack_count == 0)
    {
        return true;
    }

    //Calculate direction offset for each object
    const FACING_T diradd = 0xFFFF / pack_count;

    // now drop each item in turn
    FACING_T direction = pchr->ori.facing_z + ATK_BEHIND;
    for ( size_t cnt = 0; cnt < MAXINVENTORY; cnt++ )
    {
        CHR_REF item = pchr->inventory[cnt];

        //only valid items
        const std::shared_ptr<GameObject> &pitem = _gameObjects[item];
        if(!pitem) {
            continue;
        }

        //remove it from inventory
        inventory_remove_item( character, cnt, true );

        // detach the item
        detach_character_from_mount( item, true, true );

        // fix the attachments
        pitem->dismount_timer         = PHYS_DISMOUNT_TIME;
        pitem->dismount_object        = GET_INDEX_PCHR( pchr );
        pitem->onwhichplatform_ref    = pchr->onwhichplatform_ref;
        pitem->onwhichplatform_update = pchr->onwhichplatform_update;

        // fix some flags
        pitem->hitready               = true;
        pitem->ori.facing_z           = direction + ATK_BEHIND;
        pitem->team                   = pitem->team_base;

        // fix the current velocity
        TURN_T turn                   = TO_TURN( direction );
        pitem->vel.x                  += turntocos[ turn ] * DROPXYVEL;
        pitem->vel.y                  += turntosin[ turn ] * DROPXYVEL;
        pitem->vel.z                  += DROPZVEL;

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
//--------------------------------------------------------------------------------------------
int grab_data_cmp( const void * pleft, const void * pright )
{
    int rv;
    float diff;

    grab_data_t * dleft  = ( grab_data_t * )pleft;
    grab_data_t * dright = ( grab_data_t * )pright;

    // use only the horizontal distance
    diff = dleft->diff2_hrz - dright->diff2_hrz;

    // unless they are equal, then use the vertical distance
    if ( 0.0f == diff )
    {
        diff = dleft->diff2_vrt - dright->diff2_vrt;
    }

    if ( diff < 0.0f )
    {
        rv = -1;
    }
    else if ( diff > 0.0f )
    {
        rv = 1;
    }
    else
    {
        rv = 0;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
bool character_grab_stuff( const CHR_REF ichr_a, grip_offset_t grip_off, bool grab_people )
{
    /// @author ZZ
    /// @details This function makes the character pick up an item if there's one around

    const SDL_Color color_red = {0xFF, 0x7F, 0x7F, 0xFF};
    const SDL_Color color_grn = {0x7F, 0xFF, 0x7F, 0xFF};
    const SDL_Color color_blu = {0x7F, 0x7F, 0xFF, 0xFF};
    const GLXvector4f default_tint = { 1.00f, 1.00f, 1.00f, 1.00f };

    // 1 a grid is fine. anything more than that and it gets crazy
    const float const_info2_hrz = SQR( 3.0f * GRID_FSIZE );
    const float const_grab2_hrz = SQR( 1.0f * GRID_FSIZE );
    const float const_grab2_vrt = SQR( GRABSIZE );
    CHR_REF   ichr_b;
    slot_t    slot;
    oct_vec_t mids;
    fvec3_t   slot_pos;
    float     bump_size2_a;

    GameObject * pchr_a;

    bool retval;

    // valid objects that can be grabbed
    size_t      grab_count = 0;
    size_t      grab_visible_count = 0;
    grab_data_t grab_list[MAX_CHR];

    // valid objects that cannot be grabbed
    size_t      ungrab_count = 0;
    size_t      ungrab_visible_count = 0;
    grab_data_t ungrab_list[MAX_CHR];

    if ( !_gameObjects.exists( ichr_a ) ) return false;
    pchr_a = _gameObjects.get( ichr_a );

    ObjectProfile *objectProfile = chr_get_ppro(ichr_a);
    if ( NULL == objectProfile ) return false;

    // find the slot from the grip
    slot = grip_offset_to_slot( grip_off );
    if ( slot < 0 || slot >= SLOT_COUNT ) return false;

    // Make sure the character doesn't have something already, and that it has hands
    if ( _gameObjects.exists( pchr_a->holdingwhich[slot] ) || !objectProfile->isSlotValid(slot) )
        return false;

    //Determine the position of the grip
    oct_bb_get_mids( &pchr_a->slot_cv[slot], mids );
    slot_pos.x = mids[OCT_X];
    slot_pos.y = mids[OCT_Y];
    slot_pos.z = mids[OCT_Z];
	slot_pos += pchr_a->getPosition();

    // get the size of object a
    bump_size2_a = SQR( 1.5f * pchr_a->bump.size );

    // Go through all characters to find the best match
    for(const std::shared_ptr<GameObject> &pchr_c : _gameObjects.iterator())
    {
        fvec3_t   diff;
        float     bump_size2_b;
        float     diff2_hrz, diff2_vrt;
        float     grab2_vrt, grab2_hrz, info2_hrz;
        bool    can_grab = true;
        bool    too_dark = true;
        bool    too_invis = true;

        // do nothing to yourself
        if ( ichr_a == pchr_c->getCharacterID() ) continue;

        // Dont do hidden objects
        if ( pchr_c->is_hidden ) continue;

        // pickpocket not allowed yet
        if ( _gameObjects.exists( pchr_c->inwhich_inventory ) ) continue;

        // disarm not allowed yet
        if ( INVALID_CHR_REF != pchr_c->attachedto ) continue;

        // do not pick up your mount
        if ( pchr_c->holdingwhich[SLOT_LEFT] == ichr_a ||
             pchr_c->holdingwhich[SLOT_RIGHT] == ichr_a ) continue;

        // do not notice completely broken items?
        if ( pchr_c->isitem && !pchr_c->alive ) continue;

        // reasonable carrying capacity
        if ( pchr_c->phys.weight > pchr_a->phys.weight + pchr_a->strength * INV_FF )
        {
            can_grab = false;
        }

        // grab_people == true allows you to pick up living non-items
        // grab_people == false allows you to pick up living (functioning) items
        if ( !grab_people && !pchr_c->isitem )
        {
            can_grab = false;
        }

        // is the object visible
        too_dark  = !chr_can_see_dark( pchr_a, pchr_c.get() );
        too_invis = !chr_can_see_invis( pchr_a, pchr_c.get() );

        // calculate the distance
        diff = pchr_c->getPosition() - slot_pos;
        diff.z += pchr_c->bump.height * 0.5f;

        // find the squared difference horizontal and vertical
        diff2_hrz = fvec2_t(diff[kX], diff[kY]).length_2();
        diff2_vrt = diff.z * diff.z;

        // determine the actual max vertical distance
        grab2_vrt = SQR( pchr_c->bump.height );
        grab2_vrt = std::max( grab2_vrt, const_grab2_vrt );

        // the normal horizontal grab distance is dependent on the size of the two objects
        bump_size2_b = SQR( pchr_c->bump.size );

        // visibility affects the max grab distance.
        // if it is not visible then we have to be touching it.
        grab2_hrz = std::max( bump_size2_a, bump_size2_b );
        if ( !too_dark && !too_invis )
        {
            grab2_hrz = std::max( grab2_hrz, const_grab2_hrz );
        }

        // the player can get info from objects that are farther away
        info2_hrz = std::max( grab2_hrz, const_info2_hrz );

        // Is it too far away to interact with?
        if ( diff2_hrz > info2_hrz || diff2_vrt > grab2_vrt ) continue;

        // is it too far away to grab?
        if ( diff2_hrz > grab2_hrz )
        {
            can_grab = false;
        }

        // count the number of objects that are within the max range
        // a difference between the *_total_count and the *_count
        // indicates that some objects were not detectable
        if ( !too_invis )
        {
            if ( can_grab )
            {
                grab_visible_count++;
            }
            else
            {
                ungrab_visible_count++;
            }
        }

        if ( can_grab )
        {
            grab_list[grab_count].ichr      = pchr_c->getCharacterID();
            grab_list[grab_count].diff      = diff;
            grab_list[grab_count].diff2_hrz = diff2_hrz;
            grab_list[grab_count].diff2_vrt = diff2_vrt;
            grab_list[grab_count].too_dark  = too_dark;
            grab_list[grab_count].too_invis = too_invis;
            grab_count++;
        }
        else
        {
            ungrab_list[ungrab_count].ichr      = pchr_c->getCharacterID();
            ungrab_list[ungrab_count].diff      = diff;
            ungrab_list[ungrab_count].diff2_hrz = diff2_hrz;
            ungrab_list[ungrab_count].diff2_vrt = diff2_vrt;
            ungrab_list[ungrab_count].too_dark  = too_dark;
            ungrab_list[ungrab_count].too_invis = too_invis;
            ungrab_count++;
        }
    }

    // sort the grab list
    if ( grab_count > 1 )
    {
        qsort( grab_list, grab_count, sizeof( grab_data_t ), grab_data_cmp );
    }

    // try to grab something
    retval = false;
    if (( 0 == grab_count ) && ( 0 != grab_visible_count ) )
    {
        // There are items within the "normal" range that could be grabbed
        // but somehow they can't be seen.
        // Generate a billboard that tells the player what the problem is.
        // NOTE: this is not corerect since it could alert a player to an invisible object

        // 5 seconds and blue
        chr_make_text_billboard( ichr_a, "I can't feel anything...", color_blu, default_tint, 5, bb_opt_fade );

        retval = true;
    }

    if ( !retval )
    {
        for ( size_t cnt = 0; cnt < grab_count; cnt++ )
        {
            bool can_grab;

            GameObject * pchr_b;

            if ( grab_list[cnt].too_dark || grab_list[cnt].too_invis ) continue;

            ichr_b = grab_list[cnt].ichr;
            pchr_b = _gameObjects.get( ichr_b );

            can_grab = can_grab_item_in_shop( ichr_a, ichr_b );

            if ( can_grab )
            {
                // Stick 'em together and quit
                if ( rv_success == attach_character_to_mount( ichr_b, ichr_a, grip_off ) )
                {
                    if ( grab_people )
                    {
                        // Start the slam animation...  ( Be sure to drop!!! )
                        chr_play_action( pchr_a, ACTION_MC + slot, false );
                    }
                    retval = true;
                }
                break;
            }
            else
            {
                // Lift the item a little and quit...
                pchr_b->vel.z = DROPZVEL;
                pchr_b->hitready = true;
                SET_BIT( pchr_b->ai.alert, ALERTIF_DROPPED );
                break;
            }
        }
    }

    if ( !retval )
    {
        fvec3_t vforward;

        //---- generate billboards for things that players can interact with
        if ( EGO_FEEDBACK_TYPE_OFF != cfg.feedback && VALID_PLA( pchr_a->is_which_player ) )
        {
            // things that can be grabbed
            for ( size_t cnt = 0; cnt < grab_count; cnt++ )
            {
                ichr_b = grab_list[cnt].ichr;
                if ( grab_list[cnt].too_dark || grab_list[cnt].too_invis )
                {
                    // (5 secs and blue)
                    chr_make_text_billboard( ichr_b, "Something...", color_blu, default_tint, 5, bb_opt_fade );
                }
                else
                {
                    // (5 secs and green)
                    chr_make_text_billboard( ichr_b, chr_get_name( ichr_b, CHRNAME_ARTICLE | CHRNAME_CAPITAL, NULL, 0 ), color_grn, default_tint, 5, bb_opt_fade );
                }
            }

            // things that can't be grabbed
            for ( size_t cnt = 0; cnt < ungrab_count; cnt++ )
            {
                ichr_b = ungrab_list[cnt].ichr;

                if ( ungrab_list[cnt].too_dark || ungrab_list[cnt].too_invis )
                {
                    // (5 secs and blue)
                    chr_make_text_billboard( ichr_b, "Something...", color_blu, default_tint, 5, bb_opt_fade );
                }
                else
                {
                    // (5 secs and red)
                    chr_make_text_billboard( ichr_b, chr_get_name( ichr_b, CHRNAME_ARTICLE | CHRNAME_CAPITAL, NULL, 0 ), color_red, default_tint, 5, bb_opt_fade );
                }
            }
        }

        //---- if you can't grab anything, activate something using ALERTIF_BUMPED
        if ( VALID_PLA( pchr_a->is_which_player ) && ungrab_count > 0 )
        {
            chr_getMatForward(pchr_a, vforward);

            // sort the ungrab list
            if ( ungrab_count > 1 )
            {
                qsort( ungrab_list, ungrab_count, sizeof( grab_data_t ), grab_data_cmp );
            }

            for ( size_t cnt = 0; cnt < ungrab_count; cnt++ )
            {
                float ftmp;
                GameObject *pchr_b;

                // only do visible objects
                if ( ungrab_list[cnt].too_dark || ungrab_list[cnt].too_invis ) continue;

                pchr_b = _gameObjects.get( ungrab_list[cnt].ichr );

                // only bump the closest character that is in front of the character
                // (ignore vertical displacement)
                ftmp = fvec2_t(vforward[kX],vforward[kY]).dot(fvec2_t(ungrab_list[cnt].diff[kX],ungrab_list[cnt].diff[kY]));
                if ( ftmp > 0.0f )
                {
                    ai_state_set_bumplast( &( pchr_b->ai ), ichr_a );
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


    const std::shared_ptr<GameObject> &pchr = _gameObjects[ichr];
    if(!pchr) {
        return;
    }

    CHR_REF iweapon = pchr->holdingwhich[slot];

    // See if it's an unarmed attack...
    bool unarmed_attack;
    if ( !_gameObjects.exists(iweapon) )
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

    const std::shared_ptr<GameObject> &pweapon = _gameObjects[iweapon];
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
        CHR_REF ithrown = spawn_one_character(pchr->getPosition(), pweapon->profile_ref, chr_get_iteam( iholder ), 0, pchr->ori.facing_z, pweapon->Name, INVALID_CHR_REF);
        if (_gameObjects.exists(ithrown))
        {
            GameObject * pthrown = _gameObjects.get( ithrown );

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
                velocity += pchr->strength / ( pthrown->phys.weight * THROWFIX );
            }
            velocity = CLIP( velocity, MINTHROWVELOCITY, MAXTHROWVELOCITY );

            turn = TO_TURN( pchr->ori.facing_z + ATK_BEHIND );
            pthrown->vel.x += turntocos[ turn ] * velocity;
            pthrown->vel.y += turntosin[ turn ] * velocity;
            pthrown->vel.z = DROPZVEL;
            if ( pweapon->ammo <= 1 )
            {
                // Poof the item
                detach_character_from_mount( iweapon, true, false );
                pweapon->requestTerminate();
            }
            else
            {
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
                pweapon->ammo--;  // Ammo usage
            }

            PIP_REF attackParticle = weaponProfile->getAttackParticleProfile();

            // Spawn an attack particle
            if (INVALID_PIP_REF != attackParticle)
            {
                // make the weapon's holder the owner of the attack particle?
                // will this mess up wands?
                PRT_REF iparticle = spawnOneParticle(pweapon->getPosition(), pchr->ori.facing_z, weaponProfile->getSlotNumber(), attackParticle, iweapon, spawn_vrt_offset, chr_get_iteam(iholder), iweapon);

                if ( DEFINED_PRT( iparticle ) )
                {
                    fvec3_t tmp_pos;
                    prt_t * pprt = PrtList_get_ptr( iparticle );

                    prt_t::get_pos(pprt, tmp_pos);

                    if ( weaponProfile->spawnsAttackParticle() )
                    {
                        // attached particles get a strength bonus for reeling...
                        // dampen = REELBASE + ( pchr->strength / REEL );

                        // this gives a factor of 10 increase in bumping
                        // at a stat of 60, and a penalty for stats below about 10
                        float bumpdampen = exp( -1.8e-4 * ( pchr->strength - 2611 ) );

                        pprt->phys.weight     = pweapon->phys.weight;
                        pprt->phys.bumpdampen = pweapon->phys.bumpdampen * bumpdampen;

                        pprt = place_particle_at_vertex( pprt, iweapon, spawn_vrt_offset );
                        if ( NULL == pprt ) return;
                    }
                    else if ( prt_get_ppip( iparticle )->startontarget && _gameObjects.exists( pprt->target_ref ) )
                    {
                        pprt = place_particle_at_vertex( pprt, pprt->target_ref, spawn_vrt_offset );
                        if ( NULL == pprt ) return;

                        // Correct Z spacing base, but nothing else...
                        tmp_pos.z += prt_get_ppip( iparticle )->spacing_vrt_pair.base;
                    }
                    else
                    {
                        // NOT ATTACHED
                        pprt->attachedto_ref = INVALID_CHR_REF;

                        // Don't spawn in walls
                        if ( EMPTY_BIT_FIELD != prt_t::test_wall( pprt, tmp_pos.v, NULL ) )
                        {
                            tmp_pos.x = pweapon->getPosX();
                            tmp_pos.y = pweapon->getPosY();
                            if ( EMPTY_BIT_FIELD != prt_t::test_wall( pprt, tmp_pos.v, NULL ) )
                            {
                                tmp_pos.x = pchr->getPosX();
                                tmp_pos.y = pchr->getPosY();
                            }
                        }
                    }

                    // Initial particles get a bonus, which may be 0.00f
                    pprt->damage.base += ( pchr->strength     * weaponProfile->getStrengthDamageFactor());
                    pprt->damage.base += ( pchr->wisdom       * weaponProfile->getWisdomDamageFactor());
                    pprt->damage.base += ( pchr->intelligence * weaponProfile->getIntelligenceDamageFactor());
                    pprt->damage.base += ( pchr->dexterity    * weaponProfile->getDexterityDamageFactor());

                    // Initial particles get an enchantment bonus
                    pprt->damage.base += pweapon->damage_boost;

                    prt_t::set_pos(pprt, tmp_pos);
                }
                else
                {
                    log_debug("character_swipe() - unable to spawn attack particle for %s\n", weaponProfile->getClassName().c_str());
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

    const std::shared_ptr<GameObject> &pchr = _gameObjects[character];
    if(!pchr) {
        return;
    }

	fvec3_t loc_pos = pchr->getPosition();

    // limit the about of money to the character's actual money
    if ( money > pchr->money )
    {
        money = pchr->money;
    }

    if ( money > 0 && loc_pos.z > -2 )
    {
        int cnt, tnc;
        int count;

        // remove the money from inventory
        pchr->money = ( int )pchr->money - money;

        // make the particles emit from "waist high"
        loc_pos.z += ( pchr->chr_min_cv.maxs[OCT_Z] + pchr->chr_min_cv.mins[OCT_Z] ) * 0.5f;

        // Give the character a time-out from interacting with particles so it
        // doesn't just grab the money again
        pchr->damage_timer = DAMAGETIME;

        // count and spawn the various denominations
        for ( cnt = PIP_MONEY_COUNT - 1; cnt >= 0 && money >= 0; cnt-- )
        {
            count = money / vals[cnt];
            money -= count * vals[cnt];

            for ( tnc = 0; tnc < count; tnc++ )
            {
                spawn_one_particle_global( loc_pos, ATK_FRONT, pips[cnt], tnc );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void call_for_help( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function issues a call for help to all allies

    TEAM_REF team;

    if ( !_gameObjects.exists( character ) ) return;

    team = chr_get_iteam( character );
    TeamStack.lst[team].sissy = character;

    for(const std::shared_ptr<GameObject> &chr : _gameObjects.iterator())
    {
        if ( chr->getCharacterID() != character && !team_hates_team( chr->team, team ) )
        {
            SET_BIT( chr->ai.alert, ALERTIF_CALLEDFORHELP );
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_level_up( const CHR_REF character )
{
    /// @author BB
    /// @details level gains are done here, but only once a second

    Uint8 curlevel;
    int number;
    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return;
    pchr = _gameObjects.get( character );

    ObjectProfile * profile = chr_get_ppro( character );
    if ( nullptr == profile ) return;

    // Do level ups and stat changes
    curlevel = pchr->experiencelevel + 1;
    if ( curlevel < MAXLEVEL )
    {
        Uint32 xpcurrent, xpneeded;

        xpcurrent = pchr->experience;
        xpneeded  = profile->getXPNeededForLevel(curlevel);
        if ( xpcurrent >= xpneeded )
        {
            // do the level up
            pchr->experiencelevel++;
            SET_BIT( pchr->ai.alert, ALERTIF_LEVELUP );

            // The character is ready to advance...
            if ( VALID_PLA( pchr->is_which_player ) )
            {
                DisplayMsg_printf( "%s gained a level!!!", chr_get_name( GET_INDEX_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ) );
                _audioSystem.playSoundFull(_audioSystem.getGlobalSound(GSND_LEVELUP));
            }

            // Size
            pchr->fat_goto += profile->getSizeGainPerLevel() * 0.25f;  // Limit this?
            pchr->fat_goto_time += SIZETIME;

            // Strength
            number = generate_irand_range( profile->getStrengthGainPerLevel() );
            number += pchr->strength;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->strength = number;

            // Wisdom
            number = generate_irand_range( profile->getWisdomGainPerLevel() );
            number += pchr->wisdom;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->wisdom = number;

            // Intelligence
            number = generate_irand_range( profile->getIntelligenceGainPerLevel() );
            number += pchr->intelligence;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->intelligence = number;

            // Dexterity
            number = generate_irand_range( profile->getDexterityGainPerLevel() );
            number += pchr->dexterity;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->dexterity = number;

            // Life
            number = generate_irand_range( profile->getLifeGainPerLevel() );
            number += pchr->life_max;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            pchr->life += ( number - pchr->life_max );
            pchr->life_max = number;

            // Mana
            number = generate_irand_range( profile->getManaGainPerLevel() );
            number += pchr->mana_max;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            pchr->mana += ( number - pchr->mana_max );
            pchr->mana_max = number;

            // Mana Return
            number = generate_irand_range( profile->getManaRegenerationGainPerLevel() );
            number += pchr->mana_return;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->mana_return = number;

            // Mana Flow
            number = generate_irand_range( profile->getManaFlowGainPerLevel() );
            number += pchr->mana_flow;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->mana_flow = number;
        }
    }
}

//--------------------------------------------------------------------------------------------
void give_experience( const CHR_REF character, int amount, XPType xptype, bool override_invictus )
{
    /// @author ZZ
    /// @details This function gives a character experience

    float newamount;

    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return;
    pchr = _gameObjects.get( character );

    ObjectProfile* profile = chr_get_ppro( character );
    if ( nullptr == profile ) return;

    //No xp to give
    if ( 0 == amount ) return;

    if ( !pchr->invictus || override_invictus )
    {
        float intadd = ( FP8_TO_FLOAT( pchr->intelligence ) - 10.0f ) / 200.0f;
        float wisadd = ( FP8_TO_FLOAT( pchr->wisdom )       - 10.0f ) / 400.0f;

        // Figure out how much experience to give
        newamount = amount;
        if ( xptype < XP_COUNT )
        {
            newamount = amount * profile->getExperienceRate(xptype);
        }

        // Intelligence and slightly wisdom increases xp gained (0,5% per int and 0,25% per wisdom above 10)
        newamount *= 1.00f + intadd + wisadd;

        // Apply XP bonus/penality depending on game difficulty
        if ( cfg.difficulty >= GAME_HARD ) newamount *= 1.20f;                // 20% extra on hard
        else if ( cfg.difficulty >= GAME_NORMAL ) newamount *= 1.10f;       // 10% extra on normal

        pchr->experience += newamount;
    }
}

//--------------------------------------------------------------------------------------------
void give_team_experience( const TEAM_REF team, int amount, XPType xptype )
{
    /// @author ZZ
    /// @details This function gives every character on a team experience

    for(const std::shared_ptr<GameObject> &chr : _gameObjects.iterator())
    {
        if ( chr->team == team )
        {
            give_experience( chr->getCharacterID(), amount, ( XPType )xptype, false );
        }
    }
}

//--------------------------------------------------------------------------------------------
GameObject * resize_one_character( GameObject * pchr )
{
    /// @author ZZ
    /// @details This function makes the characters get bigger or smaller, depending
    ///    on their fat_goto and fat_goto_time. Spellbooks do not resize
    /// @note BB@> assume that this will only be called from inside chr_config_do_active(),
    ///         so pchr is just right to be used here

    CHR_REF ichr;
    bool  willgetcaught;
    float   newsize;

    if ( NULL == pchr ) return pchr;

    ichr = GET_INDEX_PCHR( pchr );

    if ( pchr->fat_goto_time < 0 ) return pchr;

    if ( pchr->fat_goto != pchr->fat )
    {
        int bump_increase;

        bump_increase = ( pchr->fat_goto - pchr->fat ) * 0.10f * pchr->bump.size;

        // Make sure it won't get caught in a wall
        willgetcaught = false;
        if ( pchr->fat_goto > pchr->fat )
        {
            pchr->bump.size += bump_increase;

            if ( EMPTY_BIT_FIELD != GameObjectest_wall( pchr, NULL, NULL ) )
            {
                willgetcaught = true;
            }

            pchr->bump.size -= bump_increase;
        }

        // If it is getting caught, simply halt growth until later
        if ( !willgetcaught )
        {
            // Figure out how big it is
            pchr->fat_goto_time--;

            newsize = pchr->fat_goto;
            if ( pchr->fat_goto_time > 0 )
            {
                newsize = ( pchr->fat * 0.90f ) + ( newsize * 0.10f );
            }

            // Make it that big...
            chr_set_fat( pchr, newsize );

            const std::shared_ptr<ObjectProfile> &profile = _profileSystem.getProfile( pchr->profile_ref );

            if ( CAP_INFINITE_WEIGHT == profile->getWeight() )
            {
                pchr->phys.weight = CHR_INFINITE_WEIGHT;
            }
            else
            {
                Uint32 itmp = profile->getWeight() * pchr->fat * pchr->fat * pchr->fat;
                pchr->phys.weight = std::min( itmp, CHR_MAX_WEIGHT );
            }
        }
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
bool export_one_character_quest_vfs( const char *szSaveName, const CHR_REF character )
{
    /// @author ZZ
    /// @details This function makes the naming.txt file for the character

    player_t *ppla;
    egolib_rv rv;

    if ( !_gameObjects.exists( character ) ) return false;

    ppla = chr_get_ppla( character );
    if ( NULL == ppla ) return false;

    rv = quest_log_upload_vfs( ppla->quest_log, SDL_arraysize( ppla->quest_log ), szSaveName );
    return TO_C_BOOL( rv_success == rv );
}

//--------------------------------------------------------------------------------------------
//void resize_all_characters()
//{
//    /// @author ZZ
/// @details This function makes the characters get bigger or smaller, depending
//    ///    on their fat_goto and fat_goto_time. Spellbooks do not resize
//
//    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
//    {
//        resize_one_character( pchr );
//    }
//    CHR_END_LOOP();
//}

//--------------------------------------------------------------------------------------------
bool export_one_character_name_vfs( const char *szSaveName, const CHR_REF character )
{
    /// @author ZZ
    /// @details This function makes the naming.txt file for the character

    if ( !_gameObjects.exists( character ) ) return false;

    return RandomName::exportName(_gameObjects.get(character)->Name, szSaveName);
}

//--------------------------------------------------------------------------------------------
bool chr_download_profile(GameObject * pchr, const std::shared_ptr<ObjectProfile> &profile)
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

    // Skillz
    idsz_map_init(pchr->skills, SDL_arraysize(pchr->skills));
    for(const auto &element : profile->getSkillMap())
    {
        idsz_map_add(pchr->skills, SDL_arraysize(pchr->skills), element.first, element.second);
    }

    pchr->darkvision_level = chr_get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
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

    // Life and Mana
    pchr->life_color = profile->getLifeColor();
    pchr->mana_color = profile->getManaColor();
    pchr->life_max = generate_irand_range( profile->getBaseLife() );
    pchr->life_return = profile->getBaseLifeRegeneration();
    pchr->mana_max = generate_irand_range( profile->getBaseMana() );
    pchr->mana_flow = generate_irand_range( profile->getBaseManaFlow() );
    pchr->mana_return = generate_irand_range( profile->getBaseManaRegeneration());

    // SWID
    pchr->strength = generate_irand_range( profile->getBaseStrength() );
    pchr->wisdom = generate_irand_range( profile->getBaseWisdom() );
    pchr->intelligence = generate_irand_range( profile->getBaseIntelligence() );
    pchr->dexterity = generate_irand_range( profile->getBaseDexterity() );

    // Skin
    pchr->skin = profile->getSkinOverride();
    if ( pchr->skin >= MAX_SKIN )
    {
        int irnd = RANDIE;
        pchr->skin = irnd % MAX_SKIN;
    }

    // Damage
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
	pchr->life = CLIP(profile->getSpawnLife(), UINT_TO_UFP8(1), pchr->life_max);
	pchr->mana = CLIP(profile->getSpawnMana(), UINT_TO_UFP8(0), pchr->mana_max);

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
    iTmp = generate_irand_range( profile->getStartingExperience() );
    pchr->experience      = std::min( iTmp, MAXXP );
    pchr->experiencelevel = profile->getStartingLevel();

    // Particle attachments
    pchr->reaffirm_damagetype = profile->getReaffirmDamageType();

    // Character size and bumping
    chr_init_size(pchr, profile);

    return true;
}

//--------------------------------------------------------------------------------------------
void cleanup_one_character( GameObject * pchr )
{
    /// @author BB
    /// @details Everything necessary to disconnect one character from the game

    CHR_REF  ichr, itmp;

    if ( nullptr == ( pchr ) ) return;
    ichr = GET_INDEX_PCHR( pchr );

    pchr->sparkle = NOSPARKLE;

    // Remove it from the team
    pchr->team = pchr->team_base;
    if ( TeamStack.lst[pchr->team].morale > 0 ) TeamStack.lst[pchr->team].morale--;

    if ( TeamStack.lst[pchr->team].leader == ichr )
    {
        // The team now has no leader if the character is the leader
        TeamStack.lst[pchr->team].leader = TEAM_NOLEADER;
    }

    // Clear all shop passages that it owned..
    PMod->removeShopOwner(ichr);

    // detach from any mount
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        detach_character_from_mount( ichr, true, false );
    }

    // drop your left item
    itmp = pchr->holdingwhich[SLOT_LEFT];
    if ( _gameObjects.exists( itmp ) && _gameObjects.get(itmp)->isitem )
    {
        detach_character_from_mount( itmp, true, false );
    }

    // drop your right item
    itmp = pchr->holdingwhich[SLOT_RIGHT];
    if ( _gameObjects.exists( itmp ) && _gameObjects.get(itmp)->isitem )
    {
        detach_character_from_mount( itmp, true, false );
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
        eve_t * peve;
        ENC_REF ienc_now, ienc_nxt;
        size_t  ienc_count;

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // remove all invalid enchants
        ienc_now = pchr->firstenchant;
        ienc_count = 0;
        while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
        {
            ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

            peve = enc_get_peve( ienc_now );
            if ( NULL != peve && !peve->stayiftargetdead )
            {
                remove_enchant( ienc_now, NULL );
            }

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
    }

    // Stop all sound loops for this object
    _audioSystem.stopObjectLoopingSounds(ichr);
}

//--------------------------------------------------------------------------------------------
void kill_character( const CHR_REF ichr, const CHR_REF original_killer, bool ignore_invictus )
{
    /// @author BB
    /// @details Handle a character death. Set various states, disconnect it from the world, etc.

    GameObject * pchr;
    int action;
    Uint16 experience;
    TEAM_REF killer_team;
    CHR_REF actual_killer;

    if ( !_gameObjects.exists( ichr ) ) return;
    pchr = _gameObjects.get( ichr );

    //No need to continue is there?
    if ( !pchr->alive || ( pchr->invictus && !ignore_invictus ) ) return;

    std::shared_ptr<ObjectProfile> profile = _profileSystem.getProfile( pchr->profile_ref );
    if ( !profile ) return;

    //Fix who is actually the killer if needed
    actual_killer = original_killer;
    if ( _gameObjects.exists( actual_killer ) )
    {
        GameObject *pkiller = _gameObjects.get( actual_killer );

        //If we are a held item, try to figure out who the actual killer is
        if ( _gameObjects.exists( pkiller->attachedto ) && !_gameObjects.get(pkiller->attachedto)->isMount() )
        {
            actual_killer = pkiller->attachedto;
        }

        //If the killer is a mount, try to award the kill to the rider
        else if ( pkiller->isMount() && pkiller->holdingwhich[SLOT_LEFT] )
        {
            actual_killer = pkiller->holdingwhich[SLOT_LEFT];
        }
    }

    killer_team = chr_get_iteam( actual_killer );

    pchr->alive = false;
    pchr->waskilled = true;

    pchr->life            = -1;
    pchr->platform        = true;
    pchr->canuseplatforms = true;
    pchr->phys.bumpdampen = pchr->phys.bumpdampen * 0.5f;

    // Play the death animation
    action = generate_randmask( ACTION_KA, 3 );
    chr_play_action( pchr, action, false );
    chr_instance_set_action_keep( &( pchr->inst ), true );

    // Give kill experience
    experience = profile->getExperienceValue() + ( pchr->experience * profile->getExperienceExchangeRate() );

    // distribute experience to the attacker
    if ( _gameObjects.exists( actual_killer ) )
    {
        // Set target
        pchr->ai.target = actual_killer;
        if ( killer_team == TEAM_DAMAGE || killer_team == TEAM_NULL )  pchr->ai.target = ichr;

        // Award experience for kill?
        if ( team_hates_team( killer_team, pchr->team ) )
        {
            //Check for special hatred
            if ( chr_get_idsz( actual_killer, IDSZ_HATE ) == chr_get_idsz( ichr, IDSZ_PARENT ) ||
                 chr_get_idsz( actual_killer, IDSZ_HATE ) == chr_get_idsz( ichr, IDSZ_TYPE ) )
            {
                give_experience( actual_killer, experience, XP_KILLHATED, false );
            }

            // Nope, award direct kill experience instead
            else give_experience( actual_killer, experience, XP_KILLENEMY, false );
        }
    }

    //Set various alerts to let others know it has died
    //and distribute experience to whoever needs it
    SET_BIT( pchr->ai.alert, ALERTIF_KILLED );

    for(const std::shared_ptr<GameObject> &listener : _gameObjects.iterator())
    {
        if ( !listener->alive ) continue;

        // All allies get team experience, but only if they also hate the dead guy's team
        if ( listener->getCharacterID() != actual_killer && !team_hates_team( listener->team, killer_team ) && team_hates_team( listener->team, pchr->team ) )
        {
            give_experience( listener->getCharacterID(), experience, XP_TEAMKILL, false );
        }

        // Check if it was a leader
        if ( TeamStack.lst[pchr->team].leader == ichr && listener->getTeam() == pchr->team )
        {
            // All folks on the leaders team get the alert
            SET_BIT( listener->ai.alert, ALERTIF_LEADERKILLED );
        }

        // Let the other characters know it died
        if ( listener->ai.target == ichr )
        {
            SET_BIT( listener->ai.alert, ALERTIF_TARGETKILLED );
        }
    }

    // Detach the character from the game
    cleanup_one_character( pchr );

    // If it's a player, let it die properly before enabling respawn
    if ( VALID_PLA( pchr->is_which_player ) ) local_stats.revivetimer = ONESECOND; // 1 second

    // Let it's AI script run one last time
    pchr->ai.timer = update_wld + 1;            // Prevent IfTimeOut in scr_run_chr_script()
    scr_run_chr_script( ichr );

    // Stop any looped sounds
    _audioSystem.stopObjectLoopingSounds( ichr );
}

//--------------------------------------------------------------------------------------------
void spawn_defense_ping( GameObject *pchr, const CHR_REF attacker )
{
    /// @author ZF
    /// @details Spawn a defend particle
    if ( 0 != pchr->damage_timer ) return;

    spawn_one_particle_global( pchr->getPosition(), pchr->ori.facing_z, PIP_DEFEND, 0 );

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

    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return;
    pchr = _gameObjects.get( character );

    const std::shared_ptr<ObjectProfile> &profile = _profileSystem.getProfile(profileRef);
    if (!profile) return;

    origin = pchr->ai.owner;
    facing_z   = pchr->ori.facing_z;
    for ( cnt = 0; cnt < profile->getParticlePoofAmount(); cnt++ )
    {
        spawnOneParticle( pchr->pos_old, facing_z, profile->getSlotNumber(), profile->getParticlePoofProfile(),
                            INVALID_CHR_REF, GRIP_LAST, pchr->team, origin, INVALID_PRT_REF, cnt);

        facing_z += profile->getParticlePoofFacingAdd();
    }
}

//--------------------------------------------------------------------------------------------
bool chr_get_environment( GameObject * pchr )
{
    if ( NULL == pchr ) return false;

    move_one_character_get_environment( pchr );

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
GameObject * chr_config_do_init( GameObject * pchr )
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
    std::shared_ptr<ObjectProfile> ppro = _profileSystem.getProfile( spawn_ptr->profile );
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
    iteam = CLIP( iteam, 0, (int)TEAM_MAX );
    loc_team = ( TEAM_REF )iteam;

    // IMPORTANT!!!
    pchr->missilehandler = ichr;

    // Set up model stuff
    pchr->profile_ref   = spawn_ptr->profile;
    pchr->basemodel_ref = spawn_ptr->profile;

    // Kurse state
    if ( ppro->isItem() )
    {
        IPair loc_rand = {1, 100};

        kursechance = ppro->getKurseChance();
        if ( cfg.difficulty >= GAME_HARD )                        kursechance *= 2.0f;  // Hard mode doubles chance for Kurses
        if ( cfg.difficulty < GAME_NORMAL && kursechance != 100 ) kursechance *= 0.5f;  // Easy mode halves chance for Kurses
        pchr->iskursed = ( generate_irand_pair( loc_rand ) <= kursechance );
    }

    // AI stuff
    ai_state_spawn( &( pchr->ai ), ichr, pchr->profile_ref, TeamStack.lst[loc_team].morale );

    // Team stuff
    pchr->team     = loc_team;
    pchr->team_base = loc_team;
    if ( !pchr->invictus )  TeamStack.lst[loc_team].morale++;

    // Firstborn becomes the leader
    if ( TeamStack.lst[loc_team].leader == TEAM_NOLEADER )
    {
        TeamStack.lst[loc_team].leader = ichr;
    }

    // Heal the spawn_ptr->skin, if needed
    if ( spawn_ptr->skin < 0 )
    {
        spawn_ptr->skin = ppro->getSkinOverride();
    }

    // cap_get_skin_overide() can return NO_SKIN_OVERIDE or MAX_SKIN, so we need to check
    // for the "random skin marker" even if that function is called
    if ( spawn_ptr->skin >= MAX_SKIN )
    {
        // This is a "random" skin.
        // Force it to some specific value so it will go back to the same skin every respawn
        // We are now ensuring that there are skin graphics for all skins up to MAX_SKIN, so there
        // is no need to count the skin graphics loaded into the profile.
        // Limiting the available skins to ones that had unique graphics may have been a mistake since
        // the skin-dependent properties in data.txt may exist even if there are no unique graphics.

        int irand = RANDIE;

        spawn_ptr->skin = irand % MAX_SKIN;
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
    if ( cfg.difficulty < GAME_NORMAL )
    {
        pchr->life = pchr->life_max;
        pchr->mana = pchr->mana_max;
    }

    // Character size and bumping
    pchr->fat_goto      = pchr->fat;
    pchr->fat_goto_time = 0;

    // grab all of the environment information
    chr_get_environment( pchr );

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

    // Particle attachments
    for ( tnc = 0; tnc < ppro->getAttachedParticleAmount(); tnc++ )
    {
        spawnOneParticle( pchr->getPosition(), pchr->ori.facing_z, ppro->getSlotNumber(), ppro->getAttachedParticleProfile(),
                            ichr, GRIP_LAST + tnc, pchr->team, ichr, INVALID_PRT_REF, tnc);
    }

    // is the object part of a shop's inventory?
    if ( pchr->isitem )
    {
        // Items that are spawned inside shop passages are more expensive than normal

        CHR_REF shopOwner = PMod->getShopOwner(pchr->getPosX(), pchr->getPosY());
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

    // initalize the character instance
    chr_instance_spawn( &( pchr->inst ), spawn_ptr->profile, spawn_ptr->skin );
    chr_update_matrix( pchr, true );

    // determine whether the object is hidden
    chr_update_hide( pchr );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, true );

#if defined(_DEBUG) && defined(DEBUG_WAYPOINTS)
    if ( _gameObjects.exists( pchr->attachedto ) && CHR_INFINITE_WEIGHT != pchr->phys.weight && !pchr->safe_valid )
    {
        log_warning( "spawn_one_character() - \n\tinitial spawn position <%f,%f> is \"inside\" a wall. Wall normal is <%f,%f>\n",
                     pchr->getPosX(), pchr->getPosY(), nrm.x, nrm.y );
    }
#endif

    return pchr;
}

//--------------------------------------------------------------------------------------------
GameObject * chr_config_do_active( GameObject * pchr )
{
    int     ripand;
    CHR_REF ichr;
    float water_level = 0.0f;

    if ( NULL == pchr ) return pchr;
    ichr = GET_INDEX_PCHR( pchr );

    //then do status updates
    chr_update_hide( pchr );

    //Don't do items that are in inventory
    if ( _gameObjects.exists( pchr->inwhich_inventory ) ) return pchr;

    ObjectProfile *profile = chr_get_ppro(ichr);
    if (!profile) return pchr;

    water_level = water_instance_get_water_level( &water );

    // do the character interaction with water
    if ( !pchr->is_hidden && pchr->getPosZ() < water_level && ( 0 != ego_mesh_test_fx( PMesh, pchr->onwhichgrid, MAPFX_WATER ) ) )
    {
        // do splash and ripple
        if ( !pchr->enviro.inwater )
        {
            // Splash
            fvec3_t vtmp;

            vtmp.x = pchr->getPosX();
            vtmp.y = pchr->getPosY();
            vtmp.z = water_level + RAISE;

            spawn_one_particle_global( vtmp, ATK_FRONT, PIP_SPLASH, 0 );

            if ( water.is_water )
            {
                SET_BIT( pchr->ai.alert, ALERTIF_INWATER );
            }
        }
        else
        {
            // Ripples
            if ( !_gameObjects.exists( pchr->attachedto ) && profile->causesRipples() && pchr->getPosZ() + pchr->chr_min_cv.maxs[OCT_Z] + RIPPLETOLERANCE > water_level && pchr->getPosZ() + pchr->chr_min_cv.mins[OCT_Z] < water_level )
            {
                int ripple_suppression;

                // suppress ripples if we are far below the surface
                ripple_suppression = water_level - ( pchr->getPosZ() + pchr->chr_min_cv.maxs[OCT_Z] );
                ripple_suppression = ( 4 * ripple_suppression ) / RIPPLETOLERANCE;
                ripple_suppression = CLIP( ripple_suppression, 0, 4 );

                // make more ripples if we are moving
                ripple_suppression -= (( int )pchr->vel.x != 0 ) | (( int )pchr->vel.y != 0 );

                if ( ripple_suppression > 0 )
                {
                    ripand = ~(( ~RIPPLEAND ) << ripple_suppression );
                }
                else
                {
                    ripand = RIPPLEAND >> ( -ripple_suppression );
                }

                if ( 0 == ( (update_wld + pchr->getCharacterID()) & ripand ) && pchr->getPosZ() < water_level && pchr->alive )
                {
                    fvec3_t vtmp;

                    vtmp.x = pchr->getPosX();
                    vtmp.y = pchr->getPosY();
                    vtmp.z = water_level;

                    spawn_one_particle_global( vtmp, ATK_FRONT, PIP_RIPPLE, 0 );
                }
            }

            if ( water.is_water && HAS_NO_BITS( update_wld, 7 ) )
            {
                pchr->jumpready = true;
                pchr->jumpnumber = 1;
            }
        }

        pchr->enviro.inwater  = true;
    }
    else
    {
        pchr->enviro.inwater = false;
    }

    // the following functions should not be done the first time through the update loop
    if ( 0 == update_wld ) return pchr;

    //---- Do timers and such

    // reduce attack cooldowns
    if ( pchr->reload_timer > 0 ) pchr->reload_timer--;

    // decrement the dismount timer
    if ( pchr->dismount_timer > 0 ) pchr->dismount_timer--;

    if ( 0 == pchr->dismount_timer )
    {
        pchr->dismount_object = INVALID_CHR_REF;
    }

    // Down that ol' damage timer
    if ( pchr->damage_timer > 0 ) pchr->damage_timer--;

    // Do "Be careful!" delay
    if ( pchr->careful_timer > 0 ) pchr->careful_timer--;

    // Texture movement
    pchr->inst.uoffset += pchr->uoffvel;
    pchr->inst.voffset += pchr->voffvel;

    // Do stats once every second
    if ( clock_chr_stat >= ONESECOND )
    {
        // check for a level up
        do_level_up( ichr );

        // do the mana and life regen for "living" characters
        if ( pchr->alive )
        {
            int manaregen = 0;
            int liferegen = 0;
            get_chr_regeneration( pchr, &liferegen, &manaregen );

            pchr->mana += manaregen;
			pchr->mana = CLIP((UFP8_T)pchr->mana, (UFP8_T)0, pchr->mana_max);

            pchr->life += liferegen;
			pchr->life = CLIP((UFP8_T)pchr->life, (UFP8_T)1, pchr->life_max);
        }

        // countdown confuse effects
        if ( pchr->grog_timer > 0 )
        {
            pchr->grog_timer--;
        }

        if ( pchr->daze_timer > 0 )
        {
            pchr->daze_timer--;
        }

        // possibly gain/lose darkvision
        update_chr_darkvision( ichr );
    }

    pchr = resize_one_character( pchr );

    // update some special skills
    pchr->see_kurse_level  = std::max( pchr->see_kurse_level,  chr_get_skill( pchr, MAKE_IDSZ( 'C', 'K', 'U', 'R' ) ) );
    pchr->darkvision_level = std::max( pchr->darkvision_level, chr_get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) ) );

    return pchr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF spawn_one_character( const fvec3_t& pos, const PRO_REF profile, const TEAM_REF team,
                             const int skin, const FACING_T facing, const char *name, const CHR_REF override )
{
    CHR_REF   ichr;

    // fix a "bad" name
    if ( NULL == name ) name = "";

    if ( !_profileSystem.isValidProfileID( profile ) )
    {
        if ( profile > PMod->getImportAmount() * MAX_IMPORT_PER_PLAYER )
        {
            log_warning( "spawn_one_character() - trying to spawn using invalid profile %d\n", REF_TO_INT( profile ) );
        }
        return INVALID_CHR_REF;
    }
    std::shared_ptr<ObjectProfile> ppro = _profileSystem.getProfile( profile );

    // count all the requests for this character type
    ppro->requestCount++;

    // allocate a new character
    std::shared_ptr<GameObject> pchr = _gameObjects.insert(profile, override);
    if (!pchr)
    {
        log_warning( "spawn_one_character() - failed to spawn character\n" );
        return INVALID_CHR_REF;
    }

    // just set the spawn info
	pchr->spawn_data.pos = pos;
    pchr->spawn_data.profile  = profile;
    pchr->spawn_data.team     = team;
    pchr->spawn_data.skin     = skin;
    pchr->spawn_data.facing   = facing;
    strncpy( pchr->spawn_data.name, name, SDL_arraysize( pchr->spawn_data.name ) );
    pchr->spawn_data.override = override;

    chr_config_do_init(pchr.get());

    // start the character out in the "dance" animation
    chr_start_anim( pchr.get(), ACTION_DA, true, true );

    // count all the successful spawns of this character
    ppro->spawnCount++;

#if defined(DEBUG_OBJECT_SPAWN) && defined(_DEBUG)
    {
        log_debug( "spawn_one_character() - slot: %i, index: %i, name: %s, class: %s\n", REF_TO_INT( profile ), REF_TO_INT( pchr->getCharacterID() ), name, ppro->getClassName().c_str() );
    }
#endif

    return pchr->getCharacterID();
}

//--------------------------------------------------------------------------------------------
void respawn_character( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function respawns a character

    int old_attached_prt_count, new_attached_prt_count;

    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return;
    pchr = _gameObjects.get( character );

    //already alive?
    if(pchr->alive) {
        return;
    }

    const std::shared_ptr<ObjectProfile> &profile = _profileSystem.getProfile( pchr->profile_ref );

    old_attached_prt_count = number_of_attached_particles( character );

    spawn_poof( character, pchr->profile_ref );
    disaffirm_attached_particles( character );

    pchr->alive = true;
    pchr->bore_timer = BORETIME;
    pchr->careful_timer = CAREFULTIME;
    pchr->life = pchr->life_max;
    pchr->mana = pchr->mana_max;
    pchr->setPosition(pchr->pos_stt);
    pchr->vel.x = 0;
    pchr->vel.y = 0;
    pchr->vel.z = 0;
    pchr->team = pchr->team_base;
    pchr->canbecrushed = false;
    pchr->ori.map_twist_facing_y = MAP_TURN_OFFSET;  // These two mean on level surface
    pchr->ori.map_twist_facing_x = MAP_TURN_OFFSET;
    if ( TEAM_NOLEADER == TeamStack.lst[pchr->team].leader )  TeamStack.lst[pchr->team].leader = character;
    if ( !pchr->invictus )  TeamStack.lst[pchr->team_base].morale++;

    // start the character out in the "dance" animation
    chr_start_anim( pchr, ACTION_DA, true, true );

    // reset all of the bump size information
    {
        float old_fat = pchr->fat;
        chr_init_size(pchr, profile);
        chr_set_fat( pchr, old_fat );
    }

    pchr->platform        = profile->isPlatform();
    pchr->canuseplatforms = profile->canUsePlatforms();
    pchr->flyheight       = profile->getFlyHeight();
    pchr->phys.bumpdampen = profile->getBumpDampen();

    pchr->ai.alert = ALERTIF_CLEANEDUP;
    pchr->ai.target = character;
    pchr->ai.timer  = 0;

    pchr->grog_timer = 0;
    pchr->daze_timer = 0;

    // Let worn items come back
    PACK_BEGIN_LOOP( pchr->inventory, pitem, item )
    {
        if ( _gameObjects.get(item)->isequipped )
        {
            _gameObjects.get(item)->isequipped = false;
            SET_BIT( pchr->ai.alert, ALERTIF_PUTAWAY ); // same as ALERTIF_ATLASTWAYPOINT
        }
    }
    PACK_END_LOOP();

    // re-initialize the instance
    chr_instance_spawn( &( pchr->inst ), pchr->profile_ref, pchr->skin );
    chr_update_matrix( pchr, true );

    // determine whether the object is hidden
    chr_update_hide( pchr );

    if ( !pchr->is_hidden )
    {
        reaffirm_attached_particles( character );
        new_attached_prt_count = number_of_attached_particles( character );
    }

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, true );
}

//--------------------------------------------------------------------------------------------
int chr_change_skin( const CHR_REF character, const SKIN_T skin )
{
    GameObject * pchr;
    ObjectProfile * ppro;
    mad_t * pmad;
    chr_instance_t * pinst;
    TX_REF new_texture = ( TX_REF )TX_WATER_TOP;
    SKIN_T loc_skin = skin;

    if ( !_gameObjects.exists( character ) ) return 0;
    pchr  = _gameObjects.get( character );
    pinst = &( pchr->inst );

    ppro = chr_get_ppro( character );

    pmad = _profileSystem.pro_get_pmad( pchr->profile_ref );
    if ( NULL == pmad )
    {
        // make sure that the instance has a valid imad
        if ( NULL != ppro && !LOADED_MAD( pinst->imad ) )
        {
            if ( chr_instance_set_mad( pinst, ppro->getModelRef() ) )
            {
                chr_update_collision_size( pchr, true );
            }
            pmad = chr_get_pmad( character );
        }
    }

    if ( NULL == pmad )
    {
        pchr->skin     = 0;
        pinst->texture = TX_WATER_TOP;
    }
    else
    {
        // do the best we can to change the skin

        // all skin numbers are technically valid
        if ( loc_skin < 0 )
        {
            loc_skin = 0;
        }
        else
        {
            loc_skin %= MAX_SKIN;
        }

        // assume we cannot find a texture
        new_texture = TX_WATER_TOP;

        if ( NULL == ppro )
        {
            // should never happen
            /* do nothing */
        }
        else
        {
            // the normal thing to happen
            new_texture = ppro->getSkin(loc_skin);
        }

        pchr->skin = skin;
    }

    chr_instance_set_texture( pinst, new_texture );

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
    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return 0;
    pchr = _gameObjects.get( character );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Remove armor enchantments
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        enc_remove_set( ienc_now, SETSLASHMODIFIER );
        enc_remove_set( ienc_now, SETCRUSHMODIFIER );
        enc_remove_set( ienc_now, SETPOKEMODIFIER );
        enc_remove_set( ienc_now, SETHOLYMODIFIER );
        enc_remove_set( ienc_now, SETEVILMODIFIER );
        enc_remove_set( ienc_now, SETFIREMODIFIER );
        enc_remove_set( ienc_now, SETICEMODIFIER );
        enc_remove_set( ienc_now, SETZAPMODIFIER );

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    // Change the skin
    std::shared_ptr<ObjectProfile> profile = _profileSystem.getProfile( pchr->profile_ref );
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
    while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        PRO_REF ipro = enc_get_ipro( ienc_now );

        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        if ( _profileSystem.isValidProfileID( ipro ) )
        {
            EVE_REF ieve = _profileSystem.pro_get_ieve( ipro );

            enc_apply_set( ienc_now, SETSLASHMODIFIER, ipro );
            enc_apply_set( ienc_now, SETCRUSHMODIFIER, ipro );
            enc_apply_set( ienc_now, SETPOKEMODIFIER,  ipro );
            enc_apply_set( ienc_now, SETHOLYMODIFIER,  ipro );
            enc_apply_set( ienc_now, SETEVILMODIFIER,  ipro );
            enc_apply_set( ienc_now, SETFIREMODIFIER,  ipro );
            enc_apply_set( ienc_now, SETICEMODIFIER,   ipro );
            enc_apply_set( ienc_now, SETZAPMODIFIER,   ipro );

            enc_apply_add( ienc_now, ADDACCEL,         ieve );
            enc_apply_add( ienc_now, ADDDEFENSE,       ieve );
            enc_apply_add( ienc_now, ADDSLASHRESIST,   ieve );
            enc_apply_add( ienc_now, ADDCRUSHRESIST,   ieve );
            enc_apply_add( ienc_now, ADDPOKERESIST,    ieve );
            enc_apply_add( ienc_now, ADDHOLYRESIST,    ieve );
            enc_apply_add( ienc_now, ADDEVILRESIST,    ieve );
            enc_apply_add( ienc_now, ADDFIRERESIST,    ieve );
            enc_apply_add( ienc_now, ADDICERESIST,     ieve );
            enc_apply_add( ienc_now, ADDZAPRESIST,     ieve );
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    return loc_skin;
}

//--------------------------------------------------------------------------------------------
void change_character_full( const CHR_REF ichr, const PRO_REF profile, const int skin, const Uint8 leavewhich )
{
    /// @author ZF
    /// @details This function polymorphs a character permanently so that it can be exported properly
    /// A character turned into a frog with this function will also export as a frog!

    MAD_REF imad_old, imad_new;

    if ( !_profileSystem.isValidProfileID( profile ) ) return;

    imad_new = _profileSystem.getProfile( profile )->getModelRef();
    if ( !LOADED_MAD( imad_new ) ) return;

    imad_old = chr_get_imad( ichr );
    if ( !LOADED_MAD( imad_old ) ) return;

    // copy the new name
    strncpy( MadStack.lst[imad_old].name, MadStack.lst[imad_new].name, SDL_arraysize( MadStack.lst[imad_old].name ) );

    // change their model
    change_character( ichr, profile, skin, leavewhich );

    // set the base model to the new model, too
    _gameObjects.get(ichr)->basemodel_ref = profile;
}

//--------------------------------------------------------------------------------------------
bool set_weapongrip( const CHR_REF iitem, const CHR_REF iholder, Uint16 vrt_off )
{
    int i;

    bool needs_update;
    Uint16 grip_verts[GRIP_VERTS];

    matrix_cache_t * mcache;
    GameObject * pitem;

    needs_update = false;

    if ( !_gameObjects.exists( iitem ) ) return false;
    pitem = _gameObjects.get( iitem );
    mcache = &( pitem->inst.matrix_cache );

    // is the item attached to this valid holder?
    if ( pitem->attachedto != iholder ) return false;

    needs_update  = true;

    if ( GRIP_VERTS == get_grip_verts( grip_verts, iholder, vrt_off ) )
    {
        //---- detect any changes in the matrix_cache data

        needs_update  = false;

        if ( iholder != mcache->grip_chr || pitem->attachedto != iholder )
        {
            needs_update  = true;
        }

        if ( pitem->inwhich_slot != mcache->grip_slot )
        {
            needs_update  = true;
        }

        // check to see if any of the
        for ( i = 0; i < GRIP_VERTS; i++ )
        {
            if ( grip_verts[i] != mcache->grip_verts[i] )
            {
                needs_update = true;
                break;
            }
        }
    }

    if ( needs_update )
    {
        // cannot create the matrix, therefore the current matrix must be invalid
        mcache->matrix_valid = false;

        mcache->grip_chr  = iholder;
        mcache->grip_slot = pitem->inwhich_slot;

        for ( i = 0; i < GRIP_VERTS; i++ )
        {
            mcache->grip_verts[i] = grip_verts[i];
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void change_character( const CHR_REF ichr, const PRO_REF profile_new, const int skin, const Uint8 leavewhich )
{
    /// @author ZZ
    /// @details This function polymorphs a character, changing stats, dropping weapons
#if 0
    int tnc;
#endif
    CHR_REF item_ref, item;
    GameObject * pchr;

    mad_t * pmad_new;

    int old_attached_prt_count, new_attached_prt_count;

    if ( !_profileSystem.isValidProfileID( profile_new ) || !_gameObjects.exists( ichr ) ) return;
    pchr = _gameObjects.get( ichr );

    old_attached_prt_count = number_of_attached_particles( ichr );

    std::shared_ptr<ObjectProfile> newProfile = _profileSystem.getProfile( profile_new );
    if(!newProfile) {
        return;
    }

    pmad_new = _profileSystem.pro_get_pmad( profile_new );

    // Drop left weapon
    item_ref = pchr->holdingwhich[SLOT_LEFT];
    if ( _gameObjects.exists( item_ref ) && ( !newProfile->isSlotValid(SLOT_LEFT) || newProfile->isMount() ) )
    {
        detach_character_from_mount( item_ref, true, true );
        detach_character_from_platform( _gameObjects.get( item_ref ) );

        if ( pchr->isMount() )
        {
            _gameObjects.get(item_ref)->vel.z    = DISMOUNTZVEL;
            _gameObjects.get(item_ref)->jump_timer = JUMPDELAY;
            _gameObjects.get(item_ref)->movePosition(0.0f, 0.0f, DISMOUNTZVEL);
        }
    }

    // Drop right weapon
    item_ref = pchr->holdingwhich[SLOT_RIGHT];
    if ( _gameObjects.exists( item_ref ) && !newProfile->isSlotValid(SLOT_RIGHT) )
    {
        detach_character_from_mount( item_ref, true, true );
        detach_character_from_platform( _gameObjects.get( item_ref ) );

        if ( pchr->isMount() )
        {
            _gameObjects.get(item_ref)->vel.z    = DISMOUNTZVEL;
            _gameObjects.get(item_ref)->jump_timer = JUMPDELAY;
            _gameObjects.get(item_ref)->movePosition(0.0f, 0.0f, DISMOUNTZVEL);
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

            ienc_now = EncList.lst[pchr->firstenchant].nextenchant_ref;
            ienc_count = 0;
            while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
            {
                ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

                remove_enchant( ienc_now, NULL );

                ienc_now = ienc_nxt;
                ienc_count++;
            }
            if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
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
    idsz_map_init(pchr->skills, SDL_arraysize(pchr->skills));
    for(const auto &element : newProfile->getSkillMap())
    {
        idsz_map_add(pchr->skills, SDL_arraysize(pchr->skills), element.first, element.second);
    }
    pchr->darkvision_level = chr_get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
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
            chr_set_fat( pchr, new_fat );
            pchr->fat_goto      = old_fat;
            pchr->fat_goto_time = SIZETIME;
        }
        else
        {
            chr_set_fat( pchr, old_fat );
            pchr->fat_goto      = old_fat;
            pchr->fat_goto_time = 0;
        }
    }

    //Physics
    pchr->phys.bumpdampen     = newProfile->getBumpDampen();

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
    chr_instance_spawn( &( pchr->inst ), profile_new, skin );
    chr_update_matrix( pchr, true );

    // Action stuff that must be down after chr_instance_spawn()
    chr_instance_set_action_ready( &( pchr->inst ), false );
    chr_instance_set_action_keep( &( pchr->inst ), false );
    chr_instance_set_action_loop( &( pchr->inst ), false );
    if ( pchr->alive )
    {
        chr_play_action( pchr, ACTION_DA, false );
    }
    else
    {
        chr_play_action( pchr, generate_randmask( ACTION_KA, 3 ), false );
        chr_instance_set_action_keep( &( pchr->inst ), true );
    }

    // Set the skin after changing the model in chr_instance_spawn()
    change_armor( ichr, skin );

    // Must set the wepon grip AFTER the model is changed in chr_instance_spawn()
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        set_weapongrip( ichr, pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );
    }

    item = pchr->holdingwhich[SLOT_LEFT];
    if ( _gameObjects.exists( item ) )
    {
        EGOBOO_ASSERT( _gameObjects.get(item)->attachedto == ichr );
        set_weapongrip( item, ichr, GRIP_LEFT );
    }

    item = pchr->holdingwhich[SLOT_RIGHT];
    if ( _gameObjects.exists( item ) )
    {
        EGOBOO_ASSERT( _gameObjects.get(item)->attachedto == ichr );
        set_weapongrip( item, ichr, GRIP_RIGHT );
    }

    // determine whether the object is hidden
    chr_update_hide( pchr );

    // Reaffirm them particles...
    pchr->reaffirm_damagetype = newProfile->getReaffirmDamageType();

    /// @note ZF@> so that books dont burn when dropped
    //reaffirm_attached_particles( ichr );

    new_attached_prt_count = number_of_attached_particles( ichr );

    ai_state_set_changed( &( pchr->ai ) );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, true );
}

//--------------------------------------------------------------------------------------------
bool cost_mana( const CHR_REF character, int amount, const CHR_REF killer )
{
    /// @author ZZ
    /// @details This function takes mana from a character ( or gives mana ),
    ///    and returns true if the character had enough to pay, or false
    ///    otherwise. This can kill a character in hard mode.

    int mana_final;
    bool mana_paid;

    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return false;
    pchr = _gameObjects.get( character );

    mana_paid  = false;
    mana_final = pchr->mana - amount;

    if ( mana_final < 0 )
    {
        int mana_debt = -mana_final;

        pchr->mana = 0;

        if ( pchr->canchannel )
        {
            pchr->life -= mana_debt;

            if ( pchr->life <= 0 && cfg.difficulty >= GAME_HARD )
            {
                kill_character( character, !_gameObjects.exists( killer ) ? character : killer, false );
            }

            mana_paid = true;
        }
    }
    else
    {
        int mana_surplus = 0;

        pchr->mana = mana_final;

        if ( mana_final > pchr->mana_max )
        {
            mana_surplus = mana_final - pchr->mana_max;
            pchr->mana   = pchr->mana_max;
        }

        // allow surplus mana to go to health if you can channel?
        if ( pchr->canchannel && mana_surplus > 0 )
        {
            // use some factor, divide by 2
            pchr->heal(_gameObjects[killer], mana_surplus / 2, true);
        }

        mana_paid = true;

    }

    return mana_paid;
}

//--------------------------------------------------------------------------------------------
void switch_team_base( const CHR_REF character, const TEAM_REF team_new, const bool permanent )
{
    GameObject  * pchr;
    bool   can_have_team;
    TEAM_REF loc_team_new;

    if ( !_gameObjects.exists( character ) ) return;
    pchr = _gameObjects.get( character );

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
            if ( TeamStack.lst[team_old].morale > 0 ) TeamStack.lst[team_old].morale--;
        }

        if ( character == TeamStack.lst[team_old].leader )
        {
            TeamStack.lst[team_old].leader = TEAM_NOLEADER;
        }
    }

    // make sure we have a valid value
    loc_team_new = VALID_TEAM_RANGE( team_new ) ? team_new : TEAM_NULL;

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
            TeamStack.lst[loc_team_new].morale++;
        }

        // we are the new leader if there isn't one already
        if ( can_have_team && !_gameObjects.exists( TeamStack.lst[loc_team_new].leader ) )
        {
            TeamStack.lst[loc_team_new].leader = character;
        }
    }
}

//--------------------------------------------------------------------------------------------
void switch_team( const CHR_REF character, const TEAM_REF team )
{
    /// @author ZZ
    /// @details This function makes a character join another team...

    CHR_REF tmp_ref;
    GameObject * pchr;

    // change the base object
    switch_team_base( character, team, true );

    // grab a pointer to the character
    if ( !_gameObjects.exists( character ) ) return;
    pchr = _gameObjects.get( character );

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

    TEAM_REF team;

    if ( !_gameObjects.exists( character ) ) return;

    team = chr_get_iteam( character );

    for(const std::shared_ptr<GameObject> &listener : _gameObjects.iterator())
    {
        if ( team != listener->getTeam() ) continue;

        if ( !listener->alive )
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

    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return 0;
    pchr = _gameObjects.get( character );

    amount = 0;
    if ( chr_is_type_idsz( character, idsz ) )
    {
        if ( _gameObjects.get(character)->ammo < _gameObjects.get(character)->ammomax )
        {
            amount = _gameObjects.get(character)->ammomax - _gameObjects.get(character)->ammo;
            _gameObjects.get(character)->ammo = _gameObjects.get(character)->ammomax;
        }
    }

    return amount;
}

//--------------------------------------------------------------------------------------------
int chr_get_skill( GameObject *pchr, IDSZ whichskill )
{
    /// @author ZF
    /// @details This returns the skill level for the specified skill or 0 if the character doesn't
    ///                  have the skill. Also checks the skill IDSZ.
    IDSZ_node_t *pskill;

    if ( !ACTIVE_PCHR( pchr ) ) return false;

    //Any [NONE] IDSZ returns always "true"
    if ( IDSZ_NONE == whichskill ) return 1;

    //Do not allow poison or backstab skill if we are restricted by code of conduct
    if ( MAKE_IDSZ( 'P', 'O', 'I', 'S' ) == whichskill || MAKE_IDSZ( 's', 'T', 'A', 'B' ) == whichskill )
    {
        if ( NULL != idsz_map_get( pchr->skills, SDL_arraysize( pchr->skills ), MAKE_IDSZ( 'C', 'O', 'D', 'E' ) ) )
        {
            return 0;
        }
    }

    // First check the character Skill ID matches
    // Then check for expansion skills too.
    if ( chr_get_idsz( pchr->ai.index, IDSZ_SKILL )  == whichskill )
    {
        return 1;
    }

    // Simply return the skill level if we have the skill
    pskill = idsz_map_get( pchr->skills, SDL_arraysize( pchr->skills ), whichskill );
    if ( pskill != NULL )
    {
        return pskill->level;
    }

    // Truesight allows reading
    if ( MAKE_IDSZ( 'R', 'E', 'A', 'D' ) == whichskill )
    {
        pskill = idsz_map_get( pchr->skills, SDL_arraysize( pchr->skills ), MAKE_IDSZ( 'C', 'K', 'U', 'R' ) );
        if ( pskill != NULL && pchr->see_invisible_level > 0 )
        {
            return pchr->see_invisible_level + pskill->level;
        }
    }

    //Skill not found
    return 0;
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

    eve_t * peve;
    int life_regen = 0;

    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return false;
    pchr = _gameObjects.get( character );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // grab the life loss due poison to determine how much darkvision a character has earned, he he he!
    // clean up the enchant list before doing anything
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while ( _VALID_ENC_RANGE( ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;
        peve = enc_get_peve( ienc_now );

        //Is it true poison?
        if ( NULL != peve && MAKE_IDSZ( 'H', 'E', 'A', 'L' ) == peve->removedbyidsz )
        {
            life_regen += EncList.lst[ienc_now].target_life;
            if ( EncList.lst[ienc_now].owner_ref == pchr->ai.index ) life_regen += EncList.lst[ienc_now].owner_life;
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    if ( life_regen < 0 )
    {
        int tmp_level  = ( 0 == pchr->life_max ) ? 0 : ( 10 * -life_regen ) / pchr->life_max;                      // Darkvision gained by poison
        int base_level = chr_get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );     // Natural darkvision

        //Use the better of the two darkvision abilities
        pchr->darkvision_level = std::max( base_level, tmp_level );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void update_all_characters()
{
    /// @author ZZ
    /// @details This function updates stats and such for every character

    for(const std::shared_ptr<GameObject> &object : _gameObjects.iterator())
    {
        if(object->terminateRequested) {
            continue;
        }
        
        chr_config_do_active(object.get());
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
void move_one_character_get_environment( GameObject * pchr )
{

    float   grid_level, water_level;
    GameObject * pplatform = NULL;

    chr_environment_t * penviro;

    if ( !ACTIVE_PCHR( pchr ) ) return;
    penviro = &( pchr->enviro );

    // determine if the character is standing on a platform
    pplatform = NULL;
    if ( _gameObjects.exists( pchr->onwhichplatform_ref ) )
    {
        pplatform = _gameObjects.get( pchr->onwhichplatform_ref );
    }

    //---- character "floor" level
    grid_level = get_mesh_level( PMesh, pchr->getPosX(), pchr->getPosY(), false );
    water_level = get_mesh_level( PMesh, pchr->getPosX(), pchr->getPosY(), true );

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
        penviro->floor_level = pplatform->getPosZ() + pplatform->chr_min_cv.maxs[OCT_Z];
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
        penviro->level = pplatform->getPosZ() + pplatform->chr_min_cv.maxs[OCT_Z];
    }

    //---- The flying height of the character, the maximum of tile level, platform level and water level
    if ( 0 != ego_mesh_test_fx( PMesh, pchr->onwhichgrid, MAPFX_WATER ) )
    {
        penviro->fly_level = std::max( penviro->level, water.surface_level );
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
    penviro->grid_twist = ego_mesh_get_twist( PMesh, pchr->onwhichgrid );

    // the "watery-ness" of whatever water might be here
    penviro->is_watery = water.is_water && penviro->inwater;
    penviro->is_slippy = !penviro->is_watery && ( 0 != ego_mesh_test_fx( PMesh, pchr->onwhichgrid, MAPFX_SLIPPY ) );

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

        penviro->traction = ABS( platform_up.z ) * ( 1.0f - penviro->zlerp ) + 0.25f * penviro->zlerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= 4.00f * hillslide * ( 1.0f - penviro->zlerp ) + 1.0f * penviro->zlerp;
        }
    }
    else if ( ego_mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
    {
        penviro->traction = ABS( map_twist_nrm[penviro->grid_twist].z ) * ( 1.0f - penviro->zlerp ) + 0.25f * penviro->zlerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= 4.00f * hillslide * ( 1.0f - penviro->zlerp ) + 1.0f * penviro->zlerp;
        }
    }

    //---- the friction of the fluid we are in
    if ( penviro->is_watery )
    {
        penviro->fluid_friction_vrt  = waterfriction;
        penviro->fluid_friction_hrz = waterfriction;
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
        float temp_friction_xy = noslipfriction;
        if ( ego_mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) && penviro->is_slippy )
        {
            // It's slippy all right...
            temp_friction_xy = slippyfriction;
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
        if (( _gameObjects.exists( pchr->attachedto ) || pchr->jumpready || pchr->jumpnumber > 0 ) && pchr->jump_timer > 0 ) pchr->jump_timer--;

        // Do ground hits
        if ( penviro->grounded && pchr->vel.z < -STOPBOUNCING && pchr->hitready )
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
		penviro->floor_speed = fvec3_t::zero;
    }
    else
    {
		penviro->floor_speed = pplatform->vel;
    }

}

//--------------------------------------------------------------------------------------------
void move_one_character_do_floor_friction( GameObject * pchr )
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
	floor_acc = fvec3_t::zero;
    temp_friction_xy = 1.0f;
    vup.x = 0.0f; vup.y = 0.0f; vup.z = 1.0f;

    const std::shared_ptr<GameObject> &platform = _gameObjects[pchr->onwhichplatform_ref];

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
        mat_getChrForward(pchr->inst.matrix, vfront);
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
    if ( 1.0f == ABS( vup.z ) )
    {
        fric.z = 0.0f;
        floor_acc.z = 0.0f;
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
    penviro->is_slipping = ( std::abs( fric.x ) + std::abs( fric.y ) + std::abs( fric.z ) > penviro->friction_hrz );

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
    pchr->vel.x += -pchr->vel.x * ( 1.0f - penviro->fluid_friction_hrz );
    pchr->vel.y += -pchr->vel.y * ( 1.0f - penviro->fluid_friction_hrz );
    pchr->vel.z += -pchr->vel.z * ( 1.0f - penviro->fluid_friction_vrt );
}

//--------------------------------------------------------------------------------------------
void move_one_character_do_voluntary( GameObject * pchr )
{
    // do voluntary motion

    float dvx, dvy;
    float new_ax, new_ay;
    CHR_REF ichr;

    if ( !ACTIVE_PCHR( pchr ) ) return;

    ichr = GET_INDEX_PCHR( pchr );

    if ( !pchr->alive || pchr->maxaccel == 0.00f ) return;

    pchr->enviro.new_v[kX] = pchr->vel[kX];
    pchr->enviro.new_v[kY] = pchr->vel[kY];

    if ( _gameObjects.exists( pchr->attachedto ) ) return;

    dvx = dvy = 0.0f;
    new_ax = new_ay = 0.0f;

    // Character latches for generalized movement
    dvx = pchr->latch.x;
    dvy = pchr->latch.y;

    // Reverse movements for daze
    if ( pchr->daze_timer > 0 )
    {
        dvx = -dvx;
        dvy = -dvy;
    }

    // Switch x and y for grog
    if ( pchr->grog_timer > 0 )
    {
        SWAP( float, dvx, dvy );
    }

    // this is the maximum speed that a character could go under the v2.22 system
    float maxspeed = pchr->maxaccel * airfriction / ( 1.0f - airfriction );

    //Check animation frame freeze movement
    if ( chr_get_framefx( pchr ) & MADFX_STOP )
    {
        //TODO: ZF> might want skill that allows movement while blocking and attacking
        maxspeed = 0;
    }

    bool sneak_mode_active = false;
    if ( VALID_PLA( pchr->is_which_player ) )
    {
        // determine whether the user is hitting the "sneak button"
        player_t * ppla = PlaStack.get_ptr( pchr->is_which_player );
        sneak_mode_active = input_device_control_active( ppla->pdevice, CONTROL_SNEAK );
    }

    pchr->enviro.new_v.x = pchr->enviro.new_v.y = 0.0f;
	if (std::abs(dvx) + std::abs(dvy) > 0.05f)
    {
        float dv2 = dvx * dvx + dvy * dvy;

        if ( VALID_PLA( pchr->is_which_player ) )
        {
            float dv = POW( dv2, 0.25f );

            // determine whether the character is sneaking
            sneak_mode_active = TO_C_BOOL( dv2 < 1.0f / 9.0f );

            pchr->enviro.new_v.x = maxspeed * dvx / dv;
            pchr->enviro.new_v.y = maxspeed * dvy / dv;
        }
        else
        {
            float scale = 1.0f;

            if ( dv2 < 1.0f )
            {
                scale = POW( dv2, 0.25f );
            }

            scale /= POW( dv2, 0.5f );

            pchr->enviro.new_v.x = dvx * maxspeed * scale;
            pchr->enviro.new_v.y = dvy * maxspeed * scale;
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
    if ( _gameObjects.exists( pchr->onwhichplatform_ref ) )
    {
        GameObject * pplat = _gameObjects.get( pchr->onwhichplatform_ref );

        new_ax += ( pplat->vel.x + pchr->enviro.new_v.x - ( pchr->vel.x ) );
        new_ay += ( pplat->vel.y + pchr->enviro.new_v.y - ( pchr->vel.y ) );
    }
    else
    {
        new_ax += ( pchr->enviro.new_v.x - pchr->vel.x );
        new_ay += ( pchr->enviro.new_v.y - pchr->vel.y );
    }

    new_ax *= pchr->enviro.traction;
    new_ay *= pchr->enviro.traction;

    //Limit movement to the max acceleration
    new_ax = CLIP( new_ax, -pchr->maxaccel, pchr->maxaccel );
    new_ay = CLIP( new_ay, -pchr->maxaccel, pchr->maxaccel );

    //Figure out how to turn around
    if ( 0 != pchr->maxaccel )
    {
        switch ( pchr->turnmode )
        {
                // Get direction from ACTUAL change in velocity
            default:
            case TURNMODE_VELOCITY:
                {
                    if ( ABS( dvx ) > TURNSPD || ABS( dvy ) > TURNSPD )
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
                    if (( ABS( dvx ) > WATCHMIN || ABS( dvy ) > WATCHMIN ) )
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
                        pchr->ori.facing_z = ( int )pchr->ori.facing_z + terp_dir( pchr->ori.facing_z, vec_to_facing( _gameObjects.get(pchr->ai.target)->getPosX() - pchr->getPosX() , _gameObjects.get(pchr->ai.target)->getPosY() - pchr->getPosY() ), 8 );
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
    pchr->vel.x += new_ax;
    pchr->vel.y += new_ay;
}

//--------------------------------------------------------------------------------------------
bool chr_do_latch_attack( GameObject * pchr, slot_t which_slot )
{
    CHR_REF ichr, iweapon;
    MAD_REF imad;

    int    base_action, hand_action, action;
    bool action_valid, allowedtoattack;

    bool retval = false;

    if ( !ACTIVE_PCHR( pchr ) ) return false;
    ichr = GET_INDEX_PCHR( pchr );

    imad = chr_get_imad( ichr );

    if ( which_slot < 0 || which_slot >= SLOT_COUNT ) return false;

    // Which iweapon?
    iweapon = pchr->holdingwhich[which_slot];
    if ( !_gameObjects.exists( iweapon ) )
    {
        // Unarmed means character itself is the iweapon
        iweapon = ichr;
    }
    GameObject *pweapon     = _gameObjects.get(iweapon);
    const std::shared_ptr<ObjectProfile> &weaponProfile = pweapon->getProfile();

    //No need to continue if we have an attack cooldown
    if ( 0 != pweapon->reload_timer ) return false;

    // grab the iweapon's action
    base_action = weaponProfile->getWeaponAction();
    hand_action = randomize_action( base_action, which_slot );

    // see if the character can play this action
    action       = mad_get_action_ref( imad, hand_action );
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
        CHR_REF test_weapon;
        test_weapon = pchr->holdingwhich[which_slot == SLOT_LEFT ? SLOT_RIGHT : SLOT_LEFT];
        if ( _gameObjects.exists( test_weapon ) )
        {
            GameObject * weapon;
            weapon     = _gameObjects.get( test_weapon );
            if ( weapon->iskursed ) allowedtoattack = false;
        }
    }

    if ( !allowedtoattack )
    {
        // This character can't use this iweapon
        pweapon->reload_timer = ONESECOND;
        if ( pchr->show_stats || cfg.dev_mode )
        {
            // Tell the player that they can't use this iweapon
            DisplayMsg_printf( "%s can't use this item...", chr_get_name( GET_INDEX_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_CAPITAL, NULL, 0 ) );
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
        const std::shared_ptr<GameObject> &pmount = _gameObjects[pchr->attachedto];

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
                        chr_play_action( pmount.get(), generate_randmask( ACTION_UA, 1 ), false );
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
            int manacost;
            if(iweapon == pchr->getCharacterID())
            {
                if(pchr->getProfile()->getUseManaCost() <= pchr->mana)
                {
                    cost_mana(pchr->getCharacterID(), pchr->getProfile()->getUseManaCost(), pchr->getCharacterID());
                }
                else
                {
                    allowedtoattack = false;
                }
            }

            if(allowedtoattack)
            {
                Uint32 action_madfx = 0;

                // randomize the action
                action = randomize_action( action, which_slot );

                // make sure it is valid
                action = mad_get_action_ref( imad, action );

                // grab the MADFX_* flags for this action
                action_madfx = mad_get_action_ref( imad, action );

                if ( ACTION_IS_TYPE( action, P ) )
                {
                    // we must set parry actions to be interrupted by anything
                    chr_play_action( pchr, action, true );
                }
                else
                {
                    float chr_dex = FP8_TO_FLOAT( pchr->dexterity );

                    chr_play_action( pchr, action, false );

                    // Make the weapon animate the attack as well as the character holding it
                    if ( HAS_NO_BITS( action, MADFX_ACTLEFT | MADFX_ACTRIGHT ) )
                    {
                        if ( iweapon != ichr )
                        {
                            // the attacking character has no bits in the animation telling it
                            // to use the weapon, so we play the animation here

                            // Make the iweapon attack too
                            chr_play_action( pweapon, ACTION_MJ, false );
                        }
                    }

                    //Determine the attack speed (how fast we play the animation)
                    pchr->inst.rate  = 0.35f;                                 //base attack speed
                    pchr->inst.rate += std::min( 2.00f, chr_dex * 0.035f );         //every 10 dex increases base attack speed by 100%

                    //Add some reload time as a true limit to attacks per second
                    //Dexterity decreases the reload time for all weapons. We could allow other stats like intelligence
                    //reduce reload time for spells or gonnes here.
                    if ( !weaponProfile->hasFastAttack() )
                    {
                        int base_reload_time = -chr_dex;
                        if ( ACTION_IS_TYPE( action, U ) ) base_reload_time += 50;          //Unarmed  (Fists)
                        else if ( ACTION_IS_TYPE( action, T ) ) base_reload_time += 55;     //Thrust   (Spear)
                        else if ( ACTION_IS_TYPE( action, C ) ) base_reload_time += 85;     //Chop     (Axe)
                        else if ( ACTION_IS_TYPE( action, S ) ) base_reload_time += 65;     //Slice    (Sword)
                        else if ( ACTION_IS_TYPE( action, B ) ) base_reload_time += 70;     //Bash     (Mace)
                        else if ( ACTION_IS_TYPE( action, L ) ) base_reload_time += 60;     //Longbow  (Longbow)
                        else if ( ACTION_IS_TYPE( action, X ) ) base_reload_time += 110;    //Xbow     (Crossbow)
                        else if ( ACTION_IS_TYPE( action, F ) ) base_reload_time += 60;     //Flinged  (Unused)

                        //it is possible to have so high dex to eliminate all reload time
                        if ( base_reload_time > 0 ) pweapon->reload_timer = ( int )pweapon->reload_timer + base_reload_time;
                    }
                }

                // let everyone know what we did
                pchr->ai.lastitemused = iweapon;

                /// @note ZF@> why should there any reason the weapon should NOT be alerted when it is used?
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
bool chr_do_latch_button( GameObject * pchr )
{
    /// @author BB
    /// @details Character latches for generalized buttons

    CHR_REF ichr;
    ai_state_t * pai;

    bool attack_handled;

    if ( !ACTIVE_PCHR( pchr ) ) return false;
    ichr = GET_INDEX_PCHR( pchr );

    pai = &( pchr->ai );

    if ( !pchr->alive || pchr->latch.b.none() ) return true;

    const std::shared_ptr<ObjectProfile> &profile = pchr->getProfile();

    if ( pchr->latch.b[LATCHBUTTON_JUMP] && 0 == pchr->jump_timer )
    {

        //Jump from our mount
        if ( _gameObjects.exists( pchr->attachedto ) )
        {
            detach_character_from_mount( ichr, true, true );
            detach_character_from_platform( _gameObjects.get( ichr ) );

            pchr->jump_timer = JUMPDELAY;
            if ( 0 != pchr->flyheight )
            {
                pchr->vel.z += DISMOUNTZVELFLY;
            }
            else
            {
                pchr->vel.z += DISMOUNTZVEL;
            }

            pchr->setPosition(pchr->getPosX(), pchr->getPosY(), pchr->getPosZ() + pchr->vel.z);

            if ( pchr->jumpnumberreset != JUMPINFINITE && 0 != pchr->jumpnumber )
                pchr->jumpnumber--;

            // Play the jump sound
            _audioSystem.playSound(pchr->getPosition(), profile->getJumpSound());
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
                    pchr->vel.z += WATERJUMP;
                }
                else
                {
                    pchr->jump_timer = JUMPDELAY;
                    pchr->vel.z += pchr->jump_power * 1.5f;
                }

                pchr->jumpready = false;
                if ( pchr->jumpnumberreset != JUMPINFINITE ) pchr->jumpnumber--;

                // Set to jump animation if not doing anything better
                if ( pchr->inst.action_ready )
                {
                    chr_play_action( pchr, ACTION_JA, true );
                }

                // Play the jump sound (Boing!)
                _audioSystem.playSound(pchr->getPosition(), profile->getJumpSound());
            }
        }

    }
    if ( pchr->latch.b[LATCHBUTTON_PACKLEFT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = PACKDELAY;
        inventory_swap_item( ichr, MAXINVENTORY, SLOT_LEFT, false );
    }
    if ( pchr->latch.b[LATCHBUTTON_PACKRIGHT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = PACKDELAY;
        inventory_swap_item( ichr, MAXINVENTORY, SLOT_RIGHT, false );
    }

    if ( pchr->latch.b[LATCHBUTTON_ALTLEFT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = GRABDELAY;
        if ( !_gameObjects.exists( pchr->holdingwhich[SLOT_LEFT] ) )
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
        if ( !_gameObjects.exists( pchr->holdingwhich[SLOT_RIGHT] ) )
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
void move_one_character_do_z_motion( GameObject * pchr )
{
    if ( !ACTIVE_PCHR( pchr ) ) return;

    //---- do z acceleration
    if ( 0 != pchr->flyheight )
    {
        pchr->vel.z += ( pchr->enviro.fly_level + pchr->flyheight - pchr->getPosZ() ) * FLYDAMPEN;
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

        gperp.x = 0       - gpara.x;
        gperp.y = 0       - gpara.y;
        gperp.z = gravity - gpara.z;

        pchr->vel.x += gpara.x * ( 1.0f - loc_zlerp ) + gperp.x * loc_zlerp;
        pchr->vel.y += gpara.y * ( 1.0f - loc_zlerp ) + gperp.y * loc_zlerp;
        pchr->vel.z += gpara.z * ( 1.0f - loc_zlerp ) + gperp.z * loc_zlerp;
    }
    else
    {
        pchr->vel.z += pchr->enviro.zlerp * gravity;
    }
}

//--------------------------------------------------------------------------------------------
bool chr_update_safe_raw( GameObject * pchr )
{
    bool retval = false;

    BIT_FIELD hit_a_wall;
    float pressure;

    if ( nullptr == ( pchr ) ) return false;

    hit_a_wall = chr_hit_wall( pchr, NULL, NULL, &pressure, NULL );
    if (( 0 == hit_a_wall ) && ( 0.0f == pressure ) )
    {
        pchr->safe_valid = true;
        pchr->safe_pos = pchr->getPosition();
        pchr->safe_time  = update_wld;
        pchr->safe_grid  = ego_mesh_get_grid( PMesh, pchr->getPosX(), pchr->getPosY() );

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool chr_update_safe( GameObject * pchr, bool force )
{
    Uint32 new_grid;
    bool retval = false;
    bool needs_update = false;

    if ( nullptr == ( pchr ) ) return false;

    if ( force || !pchr->safe_valid )
    {
        needs_update = true;
    }
    else
    {
        new_grid = ego_mesh_get_grid( PMesh, pchr->getPosX(), pchr->getPosY() );

        if ( INVALID_TILE == new_grid )
        {
            if ( ABS( pchr->getPosX() - pchr->safe_pos.x ) > GRID_FSIZE ||
                 ABS( pchr->getPosY() - pchr->safe_pos.y ) > GRID_FSIZE )
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
bool chr_get_safe(GameObject * pchr)
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
        if ( !chr_hit_wall( pchr, NULL, NULL, NULL, NULL ) )
        {
            found = true;
        }
    }

    if ( !found )
    {
        breadcrumb_t * bc;

        bc = breadcrumb_list_last_valid( &( pchr->crumbs ) );

        if ( NULL != bc )
        {
            found = true;
        }
    }

    // maybe there is one last fallback after this? we could check the character's current position?
    if ( !found )
    {
        log_debug( "Uh oh! We could not find a valid non-wall position for %s!\n", chr_get_ppro( pchr->ai.index )->getClassName().c_str() );
    }

    return found;
}

//--------------------------------------------------------------------------------------------
bool chr_update_breadcrumb_raw( GameObject * pchr )
{
    breadcrumb_t bc;
    bool retval = false;

    if ( nullptr == ( pchr ) ) return false;

    breadcrumb_init_chr( &bc, pchr );

    if ( bc.valid )
    {
        retval = breadcrumb_list_add( &( pchr->crumbs ), &bc );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool chr_update_breadcrumb( GameObject * pchr, bool force )
{
    Uint32 new_grid;
    bool retval = false;
    bool needs_update = false;
    breadcrumb_t * bc_ptr, bc;

    if ( nullptr == ( pchr ) ) return false;

    bc_ptr = breadcrumb_list_last_valid( &( pchr->crumbs ) );
    if ( NULL == bc_ptr )
    {
        force  = true;
        bc_ptr = &bc;
        breadcrumb_init_chr( bc_ptr, pchr );
    }

    if ( force )
    {
        needs_update = true;
    }
    else
    {
        new_grid = ego_mesh_get_grid( PMesh, pchr->getPosX(), pchr->getPosY() );

        if ( INVALID_TILE == new_grid )
        {
            if ( ABS( pchr->getPosX() - bc_ptr->pos.x ) > GRID_FSIZE ||
                 ABS( pchr->getPosY() - bc_ptr->pos.y ) > GRID_FSIZE )
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
        retval = chr_update_breadcrumb_raw( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
breadcrumb_t * chr_get_last_breadcrumb( GameObject * pchr )
{
    if ( nullptr == ( pchr ) ) return NULL;

    if ( 0 == pchr->crumbs.count ) return NULL;

    return breadcrumb_list_last_valid( &( pchr->crumbs ) );
}

//--------------------------------------------------------------------------------------------
bool move_one_character_integrate_motion_attached( GameObject * pchr )
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
bool move_one_character_integrate_motion( GameObject * pchr )
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

    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        return move_one_character_integrate_motion_attached( pchr );
    }

    tmp_pos = pchr->getPosition();;

    pai = &( pchr->ai );
    ichr = pai->index;

    bumpdampen = CLIP( pchr->phys.bumpdampen, 0.0f, 1.0f );
    bumpdampen = ( bumpdampen + 1.0f ) * 0.5f;

    // interaction with the mesh
    //if ( ABS( pchr->vel.z ) > 0.0f )
    {
        const float vert_offset = RAISE * 0.25f;
        float grid_level = pchr->enviro.grid_level + vert_offset + 5;

        tmp_pos.z += pchr->vel.z;
        LOG_NAN( tmp_pos.z );
        if ( tmp_pos.z < grid_level )
        {
            if ( ABS( pchr->vel.z ) < STOPBOUNCING )
            {
                pchr->vel.z = 0.0f;
                tmp_pos.z = grid_level;
            }
            else
            {
                if ( pchr->vel.z < 0.0f )
                {
                    float diff = grid_level - tmp_pos.z;

                    pchr->vel.z *= -pchr->phys.bumpdampen;
                    diff        *= -pchr->phys.bumpdampen;

                    tmp_pos.z = std::max( tmp_pos.z + diff, grid_level );
                }
                else
                {
                    tmp_pos.z = grid_level;
                }
            }
        }
    }

    // fixes to the z-position
    if ( 0.0f != pchr->flyheight )
    {
        if ( tmp_pos.z < 0.0f ) tmp_pos.z = 0.0f;  // Don't fall in pits...
    }

    updated_2d = false;
    needs_test = false;

    // interaction with the grid flags
    updated_2d = false;
    needs_test = false;
    //if ( ABS( pchr->vel.x ) + ABS( pchr->vel.y ) > 0.0f )
    {
        mesh_wall_data_t wdata;

        float old_x, old_y, new_x, new_y;

        old_x = tmp_pos.x; LOG_NAN( old_x );
        old_y = tmp_pos.y; LOG_NAN( old_y );

        new_x = old_x + pchr->vel.x; LOG_NAN( new_x );
        new_y = old_y + pchr->vel.y; LOG_NAN( new_y );

        tmp_pos.x = new_x;
        tmp_pos.y = new_y;

        if ( EMPTY_BIT_FIELD == GameObjectest_wall( pchr, tmp_pos.v, &wdata ) )
        {
            updated_2d = true;
        }
        else
        {
            fvec2_t nrm;
            float   pressure;
            bool diff_function_called = false;

            chr_hit_wall( pchr, tmp_pos.v, nrm.v, &pressure, &wdata );

            // how is the character hitting the wall?
            if ( 0.0f != pressure )
            {
                bool         found_nrm  = false;
                bool         found_safe = false;
                fvec3_t      safe_pos   = fvec3_t::zero;

                bool         found_diff = false;
                fvec2_t      diff       = fvec2_t::zero;

                breadcrumb_t * bc         = NULL;

                // try to get the correct "outward" pressure from nrm
				if (!found_nrm && std::abs(nrm.x) + std::abs(nrm.y) > 0.0f)
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

                    diff.x = pchr->safe_pos.x - pchr->getPosX();
                    diff.y = pchr->safe_pos.y - pchr->getPosY();

					if (std::abs(diff.x) + std::abs(diff.y) > 0.0f)
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

                        diff.x = bc->pos.x - pchr->getPosX();
                        diff.y = bc->pos.y - pchr->getPosY();

                        if ( ABS( diff.x ) + ABS( diff.y ) > 0.0f )
                        {
                            found_diff = true;
                        }
                    }
                }

                // try to get a normal from the ego_mesh_get_diff() function
                if ( !found_nrm )
                {
                    fvec2_t tmp_diff;

                    tmp_diff = chr_get_mesh_diff( pchr, tmp_pos.v, pressure );
                    diff_function_called = true;

                    nrm.x = tmp_diff.x;
                    nrm.y = tmp_diff.y;

                    if ( ABS( nrm.x ) + ABS( nrm.y ) > 0.0f )
                    {
                        found_nrm = true;
                    }
                }

                if ( !found_diff )
                {
                    // try to get the diff from the character velocity
                    diff.x = pchr->vel.x;
                    diff.y = pchr->vel.y;

                    // make sure that the diff is in the same direction as the velocity
                    if ( diff.dot(nrm) < 0.0f )
                    {
                        diff.x *= -1.0f;
                        diff.y *= -1.0f;
                    }

					if (std::abs(diff.x) + std::abs(diff.y) > 0.0f)
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

                    fvec2_t v_perp = fvec2_t::zero;
                    fvec2_t diff_perp = fvec2_t::zero;

					nrm2 = nrm.dot(nrm);

                    save_pos = tmp_pos;

                    // make the diff point "out"
					dot = diff.dot(nrm);
                    if ( dot < 0.0f )
                    {
                        diff.x *= -1.0f;
                        diff.y *= -1.0f;
                        dot    *= -1.0f;
                    }

                    // find the part of the diff that is parallel to the normal
                    diff_perp.x = diff_perp.y = 0.0f;
                    if ( nrm2 > 0.0f )
                    {
                        diff_perp.x = nrm.x * dot / nrm2;
                        diff_perp.y = nrm.y * dot / nrm2;
                    }

                    // normalize the diff_perp so that it is at most tile_fraction of a grid in any direction
                    ftmp = std::max(ABS(diff_perp.x), ABS(diff_perp.y));

                    // protect us from a virtual divide by zero
                    if (ftmp < 1e-6) ftmp = 1.00f;

					diff_perp *= tile_fraction * GRID_FSIZE / ftmp;

                    // scale the diff_perp by the pressure
					diff_perp *= pressure;

                    // try moving the character
					tmp_pos += fvec3_t(diff_perp[kX],diff_perp[kY], 0.0f);

                    // determine whether the pressure is less at this location
                    pressure_old = chr_get_mesh_pressure( pchr, save_pos.v );
                    pressure_new = chr_get_mesh_pressure( pchr, tmp_pos.v );

                    if ( pressure_new < pressure_old )
                    {
                        // !!success!!
                        needs_test = ( tmp_pos.x != save_pos.x ) || ( tmp_pos.y != save_pos.y );
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
   
                        loc_bumpdampen = _profileSystem.getProfile(pchr->profile_ref)->getBumpDampen();

                        v_perp.x = v_perp.y = 0.0f;
                        if ( 0.0f != nrm2 )
                        {
                            v_perp.x = nrm.x * dot / nrm2;
                            v_perp.y = nrm.y * dot / nrm2;
                        }

                        pchr->vel.x += - ( 1.0f + loc_bumpdampen ) * v_perp.x * pressure;
                        pchr->vel.y += - ( 1.0f + loc_bumpdampen ) * v_perp.y * pressure;
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
bool chr_handle_madfx( GameObject * pchr )
{
    ///@details This handles special commands an animation frame might execute, for example
    ///         grabbing stuff or spawning attack particles.

    CHR_REF ichr;
    Uint32 framefx;

    if ( NULL == pchr ) return false;

    framefx = chr_get_framefx( pchr );
    if ( 0 == framefx ) return true;

    ichr    = GET_INDEX_PCHR( pchr );

    // Check frame effects
    if ( HAS_SOME_BITS( framefx, MADFX_ACTLEFT ) )
    {
        character_swipe( ichr, SLOT_LEFT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_ACTRIGHT ) )
    {
        character_swipe( ichr, SLOT_RIGHT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABLEFT ) )
    {
        character_grab_stuff( ichr, GRIP_LEFT, false );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABRIGHT ) )
    {
        character_grab_stuff( ichr, GRIP_RIGHT, false );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARLEFT ) )
    {
        character_grab_stuff( ichr, GRIP_LEFT, true );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARRIGHT ) )
    {
        character_grab_stuff( ichr, GRIP_RIGHT, true );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPLEFT ) )
    {
        detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], false, true );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPRIGHT ) )
    {
        detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], false, true );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_POOF ) && !VALID_PLA( pchr->is_which_player ) )
    {
        pchr->ai.poof_time = update_wld;
    }

    //Do footfall sound effect
    if ( cfg.sound_footfall && HAS_SOME_BITS( framefx, MADFX_FOOTFALL ) )
    {
        _audioSystem.playSound(pchr->getPosition(), _profileSystem.getProfile(pchr->profile_ref)->getFootFallSound());
    }

    return true;
}

//--------------------------------------------------------------------------------------------
int cmp_chr_anim_data( void const * vp_lhs, void const * vp_rhs )
{
    /// @author BB
    /// @details Sort MOD REF values based on the rank of the module that they point to.
    ///               Trap all stupid values.

    chr_anim_data_t * plhs = ( chr_anim_data_t * )vp_lhs;
    chr_anim_data_t * prhs = ( chr_anim_data_t * )vp_rhs;

    int retval = 0;

    if ( NULL == plhs && NULL == prhs )
    {
        return 0;
    }
    else if ( NULL == plhs )
    {
        return 1;
    }
    else if ( NULL == prhs )
    {
        return -1;
    }

    retval = ( int )prhs->allowed - ( int )plhs->allowed;
    if ( 0 != retval ) return retval;

    retval = SGN( plhs->speed - prhs->speed );

    return retval;
}

//--------------------------------------------------------------------------------------------
float set_character_animation_rate( GameObject * pchr )
{
    /// @author ZZ
    /// @details Get running, walking, sneaking, or dancing, from speed
    ///
    /// @author BB
    /// @details added automatic calculation of variable animation rates for movement animations

    float  speed;
    bool can_be_interrupted;
    bool is_walk_type;
    int    cnt, anim_count;
    int    action, lip;
    bool found;

    chr_anim_data_t anim_info[CHR_MOVEMENT_COUNT];

    chr_instance_t * pinst;
    mad_t          * pmad;
    CHR_REF          ichr;

    // set the character speed to zero
    speed = 0;

    if ( NULL == pchr ) return 1.0f;
    pinst = &( pchr->inst );
    ichr  = GET_INDEX_PCHR( pchr );

    // if the action is set to keep then do nothing
    can_be_interrupted = !pinst->action_keep;
    if ( !can_be_interrupted ) return pinst->rate = 1.0f;

    // dont change the rate if it is an attack animation
    if ( pchr->isAttacking() )  return pinst->rate;

    // if the character is mounted or sitting, base the rate off of the mounr
    if ( _gameObjects.exists( pchr->attachedto ) && (( ACTION_MI == pinst->action_which ) || ( ACTION_MH == pinst->action_which ) ) )
    {
        // just copy the rate from the mount
        pinst->rate = _gameObjects.get(pchr->attachedto)->inst.rate;
        return pinst->rate;
    }

    // if the animation is not a walking-type animation, ignore the variable animation rates
    // and the automatic determination of the walk animation
    // "dance" is walking with zero speed
    is_walk_type = ACTION_IS_TYPE( pinst->action_which, D ) || ACTION_IS_TYPE( pinst->action_which, W );
    if ( !is_walk_type ) return pinst->rate = 1.0f;

    // if the action cannot be changed on the at this time, there's nothing to do.
    // keep the same animation rate
    if ( !pinst->action_ready )
    {
        if ( 0.0f == pinst->rate ) pinst->rate = 1.0f;
        return pinst->rate;
    }

    // go back to a base animation rate, in case the next frame is not a
    // "variable speed frame"
    pinst->rate = 1.0f;

    // for non-flying objects, you have to be touching the ground
    if ( !pchr->enviro.grounded && 0 == pchr->flyheight ) return pinst->rate;

    // get the model
    pmad = chr_get_pmad( ichr );
    if ( NULL == pmad ) return pinst->rate;

    //---- set up the anim_info structure
    anim_info[CHR_MOVEMENT_STOP ].speed = 0;
    anim_info[CHR_MOVEMENT_SNEAK].speed = pchr->anim_speed_sneak;
    anim_info[CHR_MOVEMENT_WALK ].speed = pchr->anim_speed_walk;
    anim_info[CHR_MOVEMENT_RUN  ].speed = pchr->anim_speed_run;

    if ( 0 != pchr->flyheight )
    {
        // for flying characters, you have to flap like crazy to stand still and
        // do nothing to move quickly
        anim_info[CHR_MOVEMENT_STOP ].action = ACTION_WC;
        anim_info[CHR_MOVEMENT_SNEAK].action = ACTION_WB;
        anim_info[CHR_MOVEMENT_WALK ].action = ACTION_WA;
        anim_info[CHR_MOVEMENT_RUN  ].action = ACTION_DA;
    }
    else
    {
        anim_info[CHR_MOVEMENT_STOP ].action = ACTION_DA;
        anim_info[CHR_MOVEMENT_SNEAK].action = ACTION_WA;
        anim_info[CHR_MOVEMENT_WALK ].action = ACTION_WB;
        anim_info[CHR_MOVEMENT_RUN  ].action = ACTION_WC;
    }

    anim_info[CHR_MOVEMENT_STOP ].lip = 0;
    anim_info[CHR_MOVEMENT_SNEAK].lip = LIPWA;
    anim_info[CHR_MOVEMENT_WALK ].lip = LIPWB;
    anim_info[CHR_MOVEMENT_RUN  ].lip = LIPWC;

    // set up the arrays that are going to
    // determine whether the various movements are allowed
    for ( cnt = 0; cnt < CHR_MOVEMENT_COUNT; cnt++ )
    {
        anim_info[cnt].allowed = HAS_SOME_BITS( pchr->movement_bits, 1 << cnt );
    }

    if ( ACTION_WA != pmad->action_map[ACTION_WA] )
    {
        // no specific walk animation exists
        anim_info[CHR_MOVEMENT_SNEAK].allowed = false;

        /// @note ZF@> small fix here, if there is no sneak animation, try to default to normal walk with reduced animation speed
        if ( HAS_SOME_BITS( pchr->movement_bits, CHR_MOVEMENT_BITS_SNEAK ) )
        {
            anim_info[CHR_MOVEMENT_WALK].allowed = true;
            anim_info[CHR_MOVEMENT_WALK].speed *= 2;
        }
    }

    if ( ACTION_WB != pmad->action_map[ACTION_WB] )
    {
        // no specific walk animation exists
        anim_info[CHR_MOVEMENT_WALK].allowed = false;
    }

    if ( ACTION_WC != pmad->action_map[ACTION_WC] )
    {
        // no specific walk animation exists
        anim_info[CHR_MOVEMENT_RUN].allowed = false;
    }

    // sort the allowed movement(s) data
    qsort( anim_info, CHR_MOVEMENT_COUNT, sizeof( chr_anim_data_t ), cmp_chr_anim_data );

    // count the allowed movements
    for ( cnt = 0, anim_count = 0; cnt < CHR_MOVEMENT_COUNT; cnt++ )
    {
        if ( anim_info[cnt].allowed ) anim_count++;
    }

    // nothing to be done
    if ( 0 == anim_count )
    {
        return pinst->rate;
    }

    // estimate our speed
    if ( 0 != pchr->flyheight )
    {
        // for flying objects, the speed is the actual speed
        speed = ABS( pchr->vel.x ) + ABS( pchr->vel.y ) + ABS( pchr->vel.z );
    }
    else
    {
        // For non-flying objects, we use the intended speed.
		// new_v.x, new_v.y is the speed before any latches are applied.
        speed = fvec2_t(pchr->enviro.new_v[kX], pchr->enviro.new_v[kY]).length_abs();
        if ( pchr->enviro.is_slipping )
        {
            // The character is slipping as on ice.
			// Make his little legs move based on his intended speed, for comic effect! :)
			speed *= 2.0f;
        }
    }

    if ( pchr->fat != 0.0f ) speed /= pchr->fat;

    // handle a special case
    if ( 1 == anim_count )
    {
        if ( 0.0f != anim_info[0].speed )
        {
            pinst->rate = speed / anim_info[0].speed ;
        }

        return pinst->rate;
    }

    // search for the correct animation
    action = ACTION_DA;
    lip    = 0;
    found  = false;
    for ( cnt = 0; cnt < anim_count - 1; cnt++ )
    {
        float speed_mid = 0.5f * ( anim_info[cnt].speed + anim_info[cnt+1].speed );

        // make a special case for dance animation(s)
        if ( 0.0f == anim_info[cnt].speed && speed <= 1e-3 )
        {
            found = true;
        }
        else
        {
            found = ( speed < speed_mid );
        }

        if ( found )
        {
            action = anim_info[cnt].action;
            lip    = anim_info[cnt].lip;
            if ( 0.0f != anim_info[cnt].speed )
            {
                pinst->rate = speed / anim_info[cnt].speed;
            }
            break;
        }
    }

    if ( !found )
    {
        action = anim_info[cnt].action;
        lip    = anim_info[cnt].lip;
        if ( 0.0f != anim_info[cnt].speed )
        {
            pinst->rate = speed / anim_info[cnt].speed;
        }
        found = true;
    }

    if ( !found )
    {
        return pinst->rate;
    }

    if ( ACTION_DA == action )
    {
        // Do standstill

        // handle boredom
        pchr->bore_timer--;
        if ( pchr->bore_timer < 0 )
        {
            int tmp_action, rand_val;

            SET_BIT( pchr->ai.alert, ALERTIF_BORED );
            pchr->bore_timer = BORETIME;

            // set the action to "bored", which is ACTION_DB, ACTION_DC, or ACTION_DD
            rand_val   = RANDIE;
            tmp_action = mad_get_action_ref( pinst->imad, ACTION_DB + ( rand_val % 3 ) );
            chr_start_anim( pchr, tmp_action, true, true );
        }
        else
        {
            // if the current action is not ACTION_D* switch to ACTION_DA
            if ( !ACTION_IS_TYPE( pinst->action_which, D ) )
            {
                // get an appropriate version of the boredom action
                int tmp_action = mad_get_action_ref( pinst->imad, ACTION_DA );

                // start the animation
                chr_start_anim( pchr, tmp_action, true, true );
            }
        }
    }
    else
    {
        int tmp_action = mad_get_action_ref( pinst->imad, action );
        if ( ACTION_COUNT != tmp_action )
        {
            if ( pinst->action_which != tmp_action )
            {
                const MD2_Frame &nextFrame  = chr_instnce_get_frame_nxt( &( pchr->inst ) );
                chr_set_anim( pchr, tmp_action, pmad->framelip_to_walkframe[lip][nextFrame.framelip], true, true );
            }

            // "loop" the action
            chr_instance_set_action_next( pinst, tmp_action );
        }
    }

    pinst->rate = CLIP( pinst->rate, 0.1f, 10.0f );

    return pinst->rate;
}

//--------------------------------------------------------------------------------------------
void move_one_character_do_animation( GameObject * pchr )
{
    // Animate the character.
    // Right now there are 50/4 = 12.5 animation frames per second

    float flip_diff, flip_next;

    chr_instance_t * pinst;
    CHR_REF          ichr;

    if ( NULL == pchr ) return;
    ichr  = GET_INDEX_PCHR( pchr );
    pinst = &( pchr->inst );

    flip_diff  = 0.25f * pinst->rate;

    flip_next = chr_instance_get_remaining_flip( pinst );

    while ( flip_next > 0.0f && flip_diff >= flip_next )
    {
        flip_diff -= flip_next;

        chr_instance_update_one_lip( pinst );

        // handle frame FX for the new frame
        if ( 3 == pinst->ilip )
        {
            chr_handle_madfx( pchr );
        }

        if ( 4 == pinst->ilip )
        {
            if ( rv_success != chr_increment_frame( pchr ) )
            {
                log_warning( "chr_increment_frame() did not succeed\n" );
            }
        }

        if ( pinst->ilip > 4 )
        {
            log_warning( "chr_increment_frame() - invalid ilip\n" );
            pinst->ilip = 0;
            break;
        }

        flip_next = chr_instance_get_remaining_flip( pinst );
    }

    if ( flip_diff > 0.0f )
    {
        int ilip_old = pinst->ilip;

        chr_instance_update_one_flip( pinst, flip_diff );

        if ( ilip_old != pinst->ilip )
        {
            // handle frame FX for the new frame
            if ( 3 == pinst->ilip )
            {
                chr_handle_madfx( pchr );
            }

            if ( 4 == pinst->ilip )
            {
                if ( rv_success != chr_increment_frame( pchr ) )
                {
                    log_warning( "chr_increment_frame() did not succeed\n" );
                }
            }

            if ( pinst->ilip > 4 )
            {
                log_warning( "chr_increment_frame() - invalid ilip\n" );
                pinst->ilip = 0;
            }
        }
    }

    set_character_animation_rate( pchr );
}

//--------------------------------------------------------------------------------------------
void move_one_character( GameObject * pchr )
{
    if ( !ACTIVE_PCHR( pchr ) ) return;

    if ( _gameObjects.exists( pchr->inwhich_inventory ) ) return;

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
    for(const std::shared_ptr<GameObject> &object : _gameObjects.iterator())
    {
        // prime the environment
        object->enviro.air_friction = air_friction;
        object->enviro.ice_friction = ice_friction;

        move_one_character( object.get() );
    }

    // The following functions need to be called any time you actually change a charcter's position
    keep_weapons_with_holders();
    attach_all_particles();
    update_all_character_matrices();
}

//--------------------------------------------------------------------------------------------
void cleanup_all_characters()
{
    // Do poofing
    for(const std::shared_ptr<GameObject> &object : _gameObjects.iterator())
    {
        bool time_out;

        time_out = ( object->ai.poof_time >= 0 ) && ( object->ai.poof_time <= static_cast<int32_t>(update_wld) );
        if ( !time_out ) continue;

        // detach the character from the game
        cleanup_one_character( object.get() );

        // free the character's inventory
        free_inventory_in_game( object->getCharacterID() );

        // free the character
        free_one_character_in_game( object->getCharacterID() );
    }
}

//--------------------------------------------------------------------------------------------
#if 0
void bump_all_characters_update_counters()
{
    for(const auto &chr : _characterList)
    {
        Ego::Entity *pbase = POBJ_GET_PBASE( chr.second.get() );
        if ( !ACTIVE_PBASE( pbase ) ) continue;

        pbase->update_count++;
    }
}
#endif

//--------------------------------------------------------------------------------------------
bool is_invictus_direction( FACING_T direction, const CHR_REF character, BIT_FIELD effects )
{
    FACING_T left, right;

    GameObject * pchr;
    mad_t * pmad;

    bool  is_invictus;

    if ( !_gameObjects.exists( character ) ) return true;
    pchr = _gameObjects.get( character );

    pmad = chr_get_pmad( character );
    if ( NULL == pmad ) return true;

    // if the invictus flag is set, we are invictus
    if ( pchr->invictus ) return true;

    // if the effect is shield piercing, ignore shielding
    if ( HAS_SOME_BITS( effects, DAMFX_NBLOC ) ) return false;

    const ObjectProfile *profile = chr_get_ppro( character );

    // if the character's frame is invictus, then check the angles
    if ( HAS_SOME_BITS( chr_get_framefx( pchr ), MADFX_INVICTUS ) )
    {
        //I Frame
        direction -= profile->getInvictusFrameFacing();
        left       = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(profile->getInvictusFrameAngle()) );
        right      = profile->getInvictusFrameAngle();

        // If using shield, use the shield invictus instead
        if ( ACTION_IS_TYPE( pchr->inst.action_which, P ) )
        {
            bool parry_left = ( pchr->inst.action_which < ACTION_PC );

            // Using a shield?
            if ( parry_left )
            {
                // Check left hand
                const ObjectProfile *shieldProfile = chr_get_ppro(pchr->holdingwhich[SLOT_LEFT]);
                if (shieldProfile)
                {
                    left = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(shieldProfile->getInvictusFrameAngle()) );
                    right = shieldProfile->getInvictusFrameAngle();
                }
            }
            else
            {
                // Check right hand
                const ObjectProfile *shieldProfile = chr_get_ppro(pchr->holdingwhich[SLOT_LEFT]);
                if (shieldProfile)
                {
                    left = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(shieldProfile->getInvictusFrameAngle()) );
                    right = shieldProfile->getInvictusFrameAngle();
                }
            }
        }
    }
    else
    {
        // N Frame
        direction -= profile->getNormalFrameFacing();
        left       = static_cast<FACING_T>( static_cast<int>(0x00010000L) - static_cast<int>(profile->getNormalFrameAngle()) );
        right      = profile->getNormalFrameAngle();
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
    int retval = GRIP_ORIGIN;

    retval = ( slot + 1 ) * GRIP_VERTS;

    return ( grip_offset_t )retval;
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
void init_slot_idsz()
{
    inventory_idsz[INVEN_PACK]  = IDSZ_NONE;
    inventory_idsz[INVEN_NECK]  = MAKE_IDSZ( 'N', 'E', 'C', 'K' );
    inventory_idsz[INVEN_WRIS]  = MAKE_IDSZ( 'W', 'R', 'I', 'S' );
    inventory_idsz[INVEN_FOOT]  = MAKE_IDSZ( 'F', 'O', 'O', 'T' );
}

//--------------------------------------------------------------------------------------------
BBOARD_REF chr_add_billboard( const CHR_REF ichr, Uint32 lifetime_secs )
{
    /// @author BB
    /// @details Attach a basic billboard to a character. You set the billboard texture
    ///     at any time after this. Returns the index of the billboard or INVALID_BILLBOARD
    ///     if the allocation fails.
    ///
    ///    must be called with a valid character, so be careful if you call this function from within
    ///    spawn_one_character()

    GameObject * pchr;

    if ( !_gameObjects.exists( ichr ) ) return INVALID_BILLBOARD_REF;
    pchr = _gameObjects.get( ichr );

    if ( INVALID_BILLBOARD_REF != pchr->ibillboard )
    {
        BillboardList_free_one( REF_TO_INT( pchr->ibillboard ) );
        pchr->ibillboard = INVALID_BILLBOARD_REF;
    }

    pchr->ibillboard = ( BBOARD_REF )BillboardList_get_free_ref( lifetime_secs );

    // attachr the billboard to the character
    if ( INVALID_BILLBOARD_REF != pchr->ibillboard )
    {
        billboard_data_t * pbb = BillboardList_get_ptr( pchr->ibillboard );

        pbb->ichr = ichr;
    }

    return pchr->ibillboard;
}

//--------------------------------------------------------------------------------------------
billboard_data_t * chr_make_text_billboard( const CHR_REF ichr, const char * txt, const SDL_Color text_color, const GLXvector4f tint, int lifetime_secs, const BIT_FIELD opt_bits )
{
    GameObject            * pchr;
    billboard_data_t * pbb;
    int                rv;

    BBOARD_REF ibb = INVALID_BILLBOARD_REF;

    if ( !_gameObjects.exists( ichr ) ) return NULL;
    pchr = _gameObjects.get( ichr );

    // create a new billboard or override the old billboard
    ibb = chr_add_billboard( ichr, lifetime_secs );
    if ( INVALID_BILLBOARD_REF == ibb ) return NULL;

    pbb = BillboardList_get_ptr( pchr->ibillboard );
    if ( NULL == pbb ) return pbb;

    rv = billboard_data_printf_ttf( pbb, ui_getFont(), text_color, "%s", txt );

    if ( rv < 0 )
    {
        pchr->ibillboard = INVALID_BILLBOARD_REF;
        BillboardList_free_one( REF_TO_INT( ibb ) );
        pbb = NULL;
    }
    else
    {
        // copy the tint over
        memmove( pbb->tint, tint, sizeof( GLXvector4f ) );

        if ( HAS_SOME_BITS( opt_bits, bb_opt_randomize_pos ) )
        {
            // make a random offset from the character
            pbb->offset[XX] = ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * GRID_FSIZE / 5.0f;
            pbb->offset[YY] = ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * GRID_FSIZE / 5.0f;
            pbb->offset[ZZ] = ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * GRID_FSIZE / 5.0f;
        }

        if ( HAS_SOME_BITS( opt_bits, bb_opt_randomize_vel ) )
        {
            // make the text fly away in a random direction
            pbb->offset_add[XX] += ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * 2.0f * GRID_FSIZE / lifetime_secs / TARGET_UPS;
            pbb->offset_add[YY] += ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * 2.0f * GRID_FSIZE / lifetime_secs / TARGET_UPS;
            pbb->offset_add[ZZ] += ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * 2.0f * GRID_FSIZE / lifetime_secs / TARGET_UPS;
        }

        if ( HAS_SOME_BITS( opt_bits, bb_opt_fade ) )
        {
            // make the billboard fade to transparency
            pbb->tint_add[AA] = -1.0f / lifetime_secs / TARGET_UPS;
        }

        if ( HAS_SOME_BITS( opt_bits, bb_opt_burn ) )
        {
            float minval, maxval;

			minval = std::min({ pbb->tint[RR], pbb->tint[GG], pbb->tint[BB] });
			maxval = std::max({ pbb->tint[RR], pbb->tint[GG], pbb->tint[BB] });

            if ( pbb->tint[RR] != maxval )
            {
                pbb->tint_add[RR] = -pbb->tint[RR] / lifetime_secs / TARGET_UPS;
            }

            if ( pbb->tint[GG] != maxval )
            {
                pbb->tint_add[GG] = -pbb->tint[GG] / lifetime_secs / TARGET_UPS;
            }

            if ( pbb->tint[BB] != maxval )
            {
                pbb->tint_add[BB] = -pbb->tint[BB] / lifetime_secs / TARGET_UPS;
            }
        }
    }

    return pbb;
}

//--------------------------------------------------------------------------------------------
const char * chr_get_name( const CHR_REF ichr, const BIT_FIELD bits, char * buffer, size_t buffer_size )
{
    static STRING _default_buffer;

    char   * loc_buffer      = NULL;
    size_t   loc_buffer_size = 0;

    if ( NULL == buffer )
    {
        loc_buffer      = _default_buffer;
        loc_buffer_size = SDL_arraysize( _default_buffer );
    }
    else
    {
        loc_buffer      = buffer;
        loc_buffer_size = buffer_size;
    }

    if ( 0 == loc_buffer_size )
    {
        _default_buffer[0] = CSTR_END;
        return loc_buffer;
    }

    if ( !_gameObjects.exists( ichr ) )
    {
        // the default name
        strncpy( loc_buffer, "Unknown", loc_buffer_size );
    }
    else
    {
        GameObject * pchr = _gameObjects.get( ichr );

        if ( pchr->nameknown )
        {
            snprintf( loc_buffer, loc_buffer_size, "%s", pchr->Name );
        }
        else
        {
            const std::shared_ptr<ObjectProfile> &profile = _profileSystem.getProfile( pchr->profile_ref );

            char lTmp;

            if ( 0 != ( bits & CHRNAME_ARTICLE ) )
            {
                const char * article;

                if ( 0 != ( bits & CHRNAME_DEFINITE ) )
                {
                    article = "the";
                }
                else
                {
                    lTmp = char_toupper( profile->getClassName()[0] );

                    if ( 'A' == lTmp || 'E' == lTmp || 'I' == lTmp || 'O' == lTmp || 'U' == lTmp )
                    {
                        article = "an";
                    }
                    else
                    {
                        article = "a";
                    }
                }

                snprintf( loc_buffer, loc_buffer_size, "%s %s", article, profile->getClassName().c_str() );
            }
            else
            {
                snprintf( loc_buffer, loc_buffer_size, "%s", profile->getClassName().c_str() );
            }
        }
    }

    if ( 0 != ( bits & CHRNAME_CAPITAL ) )
    {
        // capitalize the name ?
        loc_buffer[0] = char_toupper(( unsigned )loc_buffer[0] );
    }

    return loc_buffer;
}

//--------------------------------------------------------------------------------------------
const char * chr_get_dir_name( const CHR_REF ichr )
{
    static STRING buffer = EMPTY_CSTR;
    GameObject * pchr;

    strncpy( buffer, "/debug", SDL_arraysize( buffer ) );

    if ( !_gameObjects.exists( ichr ) ) return buffer;
    pchr = _gameObjects.get( ichr );

    if ( !_profileSystem.isValidProfileID( pchr->profile_ref ) )
    {
        char * sztmp;

        EGOBOO_ASSERT( false );

        // copy the character's data.txt path
        strncpy( buffer, "*INVALID*", SDL_arraysize( buffer ) );

        // the name should be "...some path.../data.txt"
        // grab the path

        sztmp = strstr( buffer, "/\\" );
        if ( NULL != sztmp ) *sztmp = CSTR_END;
    }
    else
    {
        std::shared_ptr<ObjectProfile> ppro = _profileSystem.getProfile( pchr->profile_ref );

        // copy the character's data.txt path
        strncpy( buffer, ppro->getFilePath().c_str(), SDL_arraysize( buffer ) );
    }

    return buffer;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_update_collision_size( GameObject * pchr, bool update_matrix )
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

    mad_t * pmad;

    if ( nullptr == ( pchr ) ) return rv_error;

    // re-initialize the collision volumes
    oct_bb_t::ctor( &( pchr->chr_min_cv ) );
    oct_bb_t::ctor( &( pchr->chr_max_cv ) );
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        oct_bb_t::ctor( &pchr->slot_cv[cnt] );
    }

    std::shared_ptr<ObjectProfile> profile = _profileSystem.getProfile( pchr->profile_ref );

    pmad = chr_get_pmad( GET_INDEX_PCHR( pchr ) );
    if ( NULL == pmad ) return rv_error;

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
    if ( gfx_error == chr_instance_update_bbox( &( pchr->inst ) ) )
    {
        return rv_error;
    }

    // convert the point cloud in the GLvertex array (pchr->inst.vrt_lst) to
    // a level 1 bounding box. Subtract off the position of the character
    memcpy( &bsrc, &( pchr->inst.bbox ), sizeof( bsrc ) );

    // convert the corners of the level 1 bounding box to a point cloud
    vcount = oct_bb_to_points( &bsrc, src, 16 );

    // transform the new point cloud
    pchr->inst.matrix.transform(src, dst, vcount);

    // convert the new point cloud into a level 1 bounding box
    points_to_oct_bb( &bdst, dst, vcount );

    //---- set the bounding boxes
    oct_bb_copy( &( pchr->chr_min_cv ), &bdst );
    oct_bb_copy( &( pchr->chr_max_cv ), &bdst );

    oct_bb_set_bumper( &bmin, pchr->bump );

    // only use pchr->bump.size if it was overridden in data.txt through the [MODL] expansion
    if ( profile->getBumpOverrideSize() )
    {
        oct_bb_self_intersection_index( &( pchr->chr_min_cv ), &bmin, OCT_X );
        oct_bb_self_intersection_index( &( pchr->chr_min_cv ), &bmin, OCT_Y );

        oct_bb_self_union_index( &( pchr->chr_max_cv ), &bmin, OCT_X );
        oct_bb_self_union_index( &( pchr->chr_max_cv ), &bmin, OCT_Y );
    }

    // only use pchr->bump.size_big if it was overridden in data.txt through the [MODL] expansion
    if ( profile->getBumpOverrideSizeBig() )
    {
        oct_bb_self_intersection_index( &( pchr->chr_min_cv ), &bmin, OCT_XY );
        oct_bb_self_intersection_index( &( pchr->chr_min_cv ), &bmin, OCT_YX );

        oct_bb_self_union_index( &( pchr->chr_max_cv ), &bmin, OCT_XY );
        oct_bb_self_union_index( &( pchr->chr_max_cv ), &bmin, OCT_YX );
    }

    // only use pchr->bump.height if it was overridden in data.txt through the [MODL] expansion
    if ( profile->getBumpOverrideHeight() )
    {
        oct_bb_self_intersection_index( &( pchr->chr_min_cv ), &bmin, OCT_Z );

        oct_bb_self_union_index( &( pchr->chr_max_cv ), &bmin, OCT_Z );
    }

    //// raise the upper bound for platforms
    //if ( pchr->platform )
    //{
    //    pchr->chr_max_cv.maxs[OCT_Z] += PLATTOLERANCE;
    //}

    // calculate collision volumes for various slots
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        if ( !profile->isSlotValid( static_cast<slot_t>(cnt) ) ) continue;

        chr_calc_grip_cv( pchr, GRIP_LEFT, &pchr->slot_cv[cnt], false );

        oct_bb_self_union( &( pchr->chr_max_cv ), &pchr->slot_cv[cnt] );
    }

    // convert the level 1 bounding box to a level 0 bounding box
    oct_bb_downgrade( &bdst, pchr->bump_stt, pchr->bump, &( pchr->bump_1 ), NULL );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
const char* describe_value( float value, float maxval, int * rank_ptr )
{
    /// @author ZF
    /// @details This converts a stat number into a more descriptive word

    static STRING retval;

    float fval;
    int local_rank;

    if ( NULL == rank_ptr ) rank_ptr = &local_rank;

    if ( cfg.feedback == EGO_FEEDBACK_TYPE_NUMBER )
    {
        snprintf( retval, SDL_arraysize( retval ), "%2.1f", value );
        return retval;
    }

    fval = ( 0 == maxval ) ? 1.0f : value / maxval;

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

    if ( cfg.feedback == EGO_FEEDBACK_TYPE_NUMBER )
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

    if ( cfg.feedback == EGO_FEEDBACK_TYPE_NUMBER )
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

    size_t iskin;
    TX_REF icon_ref = ( TX_REF )TX_ICON_NULL;
    bool is_spell_fx, is_book, draw_book;

    GameObject * pitem;

    if ( !_gameObjects.exists( item ) ) return icon_ref;
    pitem = _gameObjects.get( item );

    if ( !_profileSystem.isValidProfileID( pitem->profile_ref ) ) return icon_ref;
    std::shared_ptr<ObjectProfile> itemProfile = _profileSystem.getProfile( pitem->profile_ref );

    // what do we need to draw?
    is_spell_fx = ( itemProfile->getSpellEffectType() >= 0 );     // the value of spelleffect_type == the skin of the book or -1 for not a spell effect
    is_book     = ( SPELLBOOK == pitem->profile_ref );
    draw_book   = ( is_book || ( is_spell_fx && !pitem->draw_icon ) /*|| ( is_spell_fx && INVALID_CHR_REF != pitem->attachedto )*/ ); /// ZF@> uncommented a part because this caused a icon bug when you were morphed and mounted

    if ( !draw_book )
    {
        iskin = pitem->skin;

        icon_ref = itemProfile->getIcon(iskin);
    }
    else if ( draw_book )
    {
        iskin = itemProfile->getSkinOverride();

        if ( iskin < 0 || iskin >= MAX_SKIN )
        {
            // no book info
            iskin = pitem->skin;
            icon_ref = itemProfile->getIcon(iskin);
        }
        else
        {
            icon_ref = _profileSystem.getSpellBookIcon(iskin);
        }
    }

    return icon_ref;
}

//--------------------------------------------------------------------------------------------
void reset_teams()
{
    /// @author ZZ
    /// @details This function makes everyone hate everyone else

    TEAM_REF teama, teamb;

    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        // Make the team hate everyone
        for ( teamb = 0; teamb < TEAM_MAX; teamb++ )
        {
            TeamStack.lst[teama].hatesteam[REF_TO_INT( teamb )] = true;
        }

        // Make the team like itself
        TeamStack.lst[teama].hatesteam[REF_TO_INT( teama )] = false;

        // Set defaults
        TeamStack.lst[teama].leader = TEAM_NOLEADER;
        TeamStack.lst[teama].sissy = 0;
        TeamStack.lst[teama].morale = 0;
    }

    // Keep the null team neutral
    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        TeamStack.lst[teama].hatesteam[TEAM_NULL] = false;
        TeamStack.lst[( TEAM_REF )TEAM_NULL].hatesteam[REF_TO_INT( teama )] = false;
    }
}

//--------------------------------------------------------------------------------------------
bool GameObjecteleport( const CHR_REF ichr, float x, float y, float z, FACING_T facing_z )
{
    /// @author BB
    /// @details Determine whether the character can be teleported to the specified location
    ///               and do it, if possible. Success returns true, failure returns false;

    GameObject  * pchr;
    FACING_T facing_old, facing_new;
    fvec3_t  pos_old, pos_new;
    bool   retval;

    if ( !_gameObjects.exists( ichr ) ) return false;
    pchr = _gameObjects.get( ichr );

    if ( x < 0.0f || x > PMesh->gmem.edge_x ) return false;
    if ( y < 0.0f || y > PMesh->gmem.edge_y ) return false;

    pos_old = pchr->getPosition();
    facing_old = pchr->ori.facing_z;

    pos_new.x  = x;
    pos_new.y  = y;
    pos_new.z  = z;
    facing_new = facing_z;

    if ( chr_hit_wall( pchr, pos_new.v, NULL, NULL, NULL ) )
    {
        // No it didn't...
        pchr->setPosition(pos_old);
        pchr->ori.facing_z = facing_old;

        retval = false;
    }
    else
    {
        // Yeah!  It worked!

        // update the old position
        pchr->pos_old          = pos_new;
        pchr->ori_old.facing_z = facing_new;

        // update the new position
        pchr->setPosition(pos_new);
        pchr->ori.facing_z = facing_new;

        if ( !detach_character_from_mount( ichr, true, false ) )
        {
            // detach_character_from_mount() updates the character matrix unless it is not mounted
            chr_update_matrix( pchr, true );
        }

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
GameObject * chr_update_hide( GameObject * pchr )
{
    /// @author BB
    /// @details Update the hide state of the character. Should be called every time that
    ///               the state variable in an ai_state_t structure is updated

    if ( nullptr == ( pchr ) ) return pchr;

    pchr->is_hidden = false;
    int8_t hideState = _profileSystem.getProfile(pchr->profile_ref)->getHideState();
    if ( hideState != NOHIDE && hideState == pchr->ai.state )
    {
        pchr->is_hidden = true;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
bool chr_matrix_valid( const GameObject * pchr )
{
    /// @author BB
    /// @details Determine whether the character has a valid matrix

    if ( nullptr == ( pchr ) ) return false;

    // both the cache and the matrix need to be valid
    return pchr->inst.matrix_cache.valid && pchr->inst.matrix_cache.matrix_valid;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int get_grip_verts( Uint16 grip_verts[], const CHR_REF imount, int vrt_offset )
{
    /// @author BB
    /// @details Fill the grip_verts[] array from the mount's data.
    ///     Return the number of vertices found.

    Uint32  i;
    int vrt_count, tnc;

    GameObject * pmount;
    mad_t * pmount_mad;

    if ( NULL == grip_verts ) return 0;

    // set all the vertices to a "safe" value
    for ( i = 0; i < GRIP_VERTS; i++ )
    {
        grip_verts[i] = 0xFFFF;
    }

    if ( !_gameObjects.exists( imount ) ) return 0;
    pmount = _gameObjects.get( imount );

    pmount_mad = chr_get_pmad( imount );
    if ( NULL == pmount_mad ) return 0;

    if ( 0 == pmount->inst.vrt_count ) return 0;

    //---- set the proper weapongrip vertices
    tnc = ( int )pmount->inst.vrt_count - ( int )vrt_offset;

    // if the starting vertex is less than 0, just take the first vertex
    if ( tnc < 0 )
    {
        grip_verts[0] = 0;
        return 1;
    }

    vrt_count = 0;
    for ( i = 0; i < GRIP_VERTS; i++ )
    {
        if ( tnc + i < pmount->inst.vrt_count )
        {
            grip_verts[i] = tnc + i;
            vrt_count++;
        }
        else
        {
            grip_verts[i] = 0xFFFF;
        }
    }

    return vrt_count;
}

//--------------------------------------------------------------------------------------------
bool chr_get_matrix_cache( GameObject * pchr, matrix_cache_t * mc_tmp )
{
    /// @author BB
    /// @details grab the matrix cache data for a given character and put it into mc_tmp.

    bool handled;
    CHR_REF itarget, ichr;

    if ( NULL == mc_tmp ) return false;
    if ( nullptr == ( pchr ) ) return false;
    ichr = GET_INDEX_PCHR( pchr );

    handled = false;
    itarget = INVALID_CHR_REF;

    // initialize xome parameters in case we fail
    mc_tmp->valid     = false;
    mc_tmp->type_bits = MAT_UNKNOWN;

    mc_tmp->self_scale.x = mc_tmp->self_scale.y = mc_tmp->self_scale.z = pchr->fat;

    // handle the overlay first of all
    if ( !handled && pchr->is_overlay && ichr != pchr->ai.target && _gameObjects.exists( pchr->ai.target ) )
    {
        // this will pretty much fail the cmp_matrix_cache() every time...

        GameObject * ptarget = _gameObjects.get( pchr->ai.target );

        // make sure we have the latst info from the target
        chr_update_matrix( ptarget, true );

        // grab the matrix cache into from the character we are overlaying
        memcpy( mc_tmp, &( ptarget->inst.matrix_cache ), sizeof( matrix_cache_t ) );

        // just in case the overlay's matrix cannot be corrected
        // then treat it as if it is not an overlay
        handled = mc_tmp->valid;
    }

    // this will happen if the overlay "failed" or for any non-overlay character
    if ( !handled )
    {
        // assume that the "target" of the MAT_CHARACTER data will be the character itself
        itarget = GET_INDEX_PCHR( pchr );

        //---- update the MAT_WEAPON data
        if ( _gameObjects.exists( pchr->attachedto ) )
        {
            GameObject * pmount = _gameObjects.get( pchr->attachedto );

            // make sure we have the latst info from the target
            chr_update_matrix( pmount, true );

            // just in case the mounts's matrix cannot be corrected
            // then treat it as if it is not mounted... yuck
            if ( pmount->inst.matrix_cache.matrix_valid )
            {
                mc_tmp->valid     = true;
                SET_BIT( mc_tmp->type_bits, MAT_WEAPON );        // add in the weapon data

                mc_tmp->grip_chr  = pchr->attachedto;
                mc_tmp->grip_slot = pchr->inwhich_slot;
                get_grip_verts( mc_tmp->grip_verts, pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );

                itarget = pchr->attachedto;
            }
        }

        //---- update the MAT_CHARACTER data
        if ( _gameObjects.exists( itarget ) )
        {
            GameObject * ptarget = _gameObjects.get( itarget );

            mc_tmp->valid   = true;
            SET_BIT( mc_tmp->type_bits, MAT_CHARACTER );  // add in the MAT_CHARACTER-type data for the object we are "connected to"

            mc_tmp->rotate.x = CLIP_TO_16BITS( ptarget->ori.map_twist_facing_x - MAP_TURN_OFFSET );
            mc_tmp->rotate.y = CLIP_TO_16BITS( ptarget->ori.map_twist_facing_y - MAP_TURN_OFFSET );
            mc_tmp->rotate.z = ptarget->ori.facing_z;

            mc_tmp->pos = ptarget->getPosition();

            mc_tmp->grip_scale.x = mc_tmp->grip_scale.y = mc_tmp->grip_scale.z = ptarget->fat;
        }
    }

    return mc_tmp->valid;
}

//--------------------------------------------------------------------------------------------
int convert_grip_to_local_points( GameObject * pholder, Uint16 grip_verts[], fvec4_t dst_point[] )
{
    /// @author ZZ
    /// @details a helper function for apply_one_weapon_matrix()

    int cnt, point_count;

    if ( NULL == grip_verts || NULL == dst_point ) return 0;

    if ( !ACTIVE_PCHR( pholder ) ) return 0;

    // count the valid weapon connection dst_points
    point_count = 0;
    for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
    {
        if ( 0xFFFF != grip_verts[cnt] )
        {
            point_count++;
        }
    }

    // do the best we can
    if ( 0 == point_count )
    {
        // punt! attach to origin
        dst_point[0].x = pholder->getPosX();
        dst_point[0].y = pholder->getPosY();
        dst_point[0].z = pholder->getPosZ();
        dst_point[0].w = 1;

        point_count = 1;
    }
    else
    {
        // update the grip vertices
        chr_instance_update_grip_verts( &( pholder->inst ), grip_verts, GRIP_VERTS );

        // copy the vertices into dst_point[]
        for ( point_count = 0, cnt = 0; cnt < GRIP_VERTS; cnt++, point_count++ )
        {
            Uint16 vertex = grip_verts[cnt];

            if ( 0xFFFF == vertex ) continue;

            dst_point[point_count].x = pholder->inst.vrt_lst[vertex].pos[XX];
            dst_point[point_count].y = pholder->inst.vrt_lst[vertex].pos[YY];
            dst_point[point_count].z = pholder->inst.vrt_lst[vertex].pos[ZZ];
            dst_point[point_count].w = 1.0f;
        }
    }

    return point_count;
}

//--------------------------------------------------------------------------------------------
int convert_grip_to_global_points( const CHR_REF iholder, Uint16 grip_verts[], fvec4_t   dst_point[] )
{
    /// @author ZZ
    /// @details a helper function for apply_one_weapon_matrix()

    GameObject *   pholder;
    int       point_count;
    fvec4_t   src_point[GRIP_VERTS];

    if ( !_gameObjects.exists( iholder ) ) return 0;
    pholder = _gameObjects.get( iholder );

    // find the grip points in the character's "local" or "body-fixed" coordinates
    point_count = convert_grip_to_local_points( pholder, grip_verts, src_point );

    if ( 0 == point_count ) return 0;

    // use the math function instead of rolling out own
    pholder->inst.matrix.transform(src_point, dst_point, point_count );

    return point_count;
}

//--------------------------------------------------------------------------------------------
bool apply_one_weapon_matrix( GameObject * pweap, matrix_cache_t * mc_tmp )
{
    /// @author ZZ
    /// @details Request that the data in the matrix cache be used to create a "character matrix".
    ///               i.e. a matrix that is not being held by anything.

    fvec4_t   nupoint[GRIP_VERTS];
    int       iweap_points;

    GameObject * pholder;
    matrix_cache_t * pweap_mcache;

    if ( NULL == mc_tmp || !mc_tmp->valid || 0 == ( MAT_WEAPON & mc_tmp->type_bits ) ) return false;

    if ( nullptr == ( pweap ) ) return false;
    pweap_mcache = &( pweap->inst.matrix_cache );

    if ( !_gameObjects.exists( mc_tmp->grip_chr ) ) return false;
    pholder = _gameObjects.get( mc_tmp->grip_chr );

    // make sure that the matrix is invalid incase of an error
    pweap_mcache->matrix_valid = false;

    // grab the grip points in world coordinates
    iweap_points = convert_grip_to_global_points( mc_tmp->grip_chr, mc_tmp->grip_verts, nupoint );

    if ( 4 == iweap_points )
    {
        // Calculate weapon's matrix based on positions of grip points
        // chrscale is recomputed at time of attachment
        mat_FourPoints( pweap->inst.matrix.v, nupoint[0].v, nupoint[1].v, nupoint[2].v, nupoint[3].v, mc_tmp->self_scale.z );

        // update the weapon position
        pweap->setPosition(fvec3_t(nupoint[3][kX],nupoint[3][kY],nupoint[3][kZ]));

        memcpy( &( pweap->inst.matrix_cache ), mc_tmp, sizeof( matrix_cache_t ) );

        pweap_mcache->matrix_valid = true;
    }
    else if ( iweap_points > 0 )
    {
        // cannot find enough vertices. punt.
        // ignore the shape of the grip and just stick the character to the single mount point

        // update the character position
        pweap->setPosition(fvec3_t(nupoint[0][kX],nupoint[0][kY],nupoint[0][kZ]));

        // make sure we have the right data
        chr_get_matrix_cache( pweap, mc_tmp );

        // add in the appropriate mods
        // this is a hybrid character and weapon matrix
        SET_BIT( mc_tmp->type_bits, MAT_CHARACTER );

        // treat it like a normal character matrix
        apply_one_character_matrix( pweap, mc_tmp );
    }

    return pweap_mcache->matrix_valid;
}

//--------------------------------------------------------------------------------------------
bool apply_one_character_matrix( GameObject * pchr, matrix_cache_t * mc_tmp )
{
    /// @author ZZ
    /// @details Request that the matrix cache data be used to create a "weapon matrix".
    ///               i.e. a matrix that is attached to a specific grip.

    if ( NULL == mc_tmp ) return false;

    // only apply character matrices using this function
    if ( 0 == ( MAT_CHARACTER & mc_tmp->type_bits ) ) return false;

    pchr->inst.matrix_cache.matrix_valid = false;

    if ( nullptr == ( pchr ) ) return false;

    if ( pchr->stickybutt )
    {
        mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(
            pchr->inst.matrix.v,
            mc_tmp->self_scale.x, mc_tmp->self_scale.y, mc_tmp->self_scale.z,
            TO_TURN( mc_tmp->rotate.z ), TO_TURN( mc_tmp->rotate.x ), TO_TURN( mc_tmp->rotate.y ),
            mc_tmp->pos.x, mc_tmp->pos.y, mc_tmp->pos.z );
    }
    else
    {
        mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(
            pchr->inst.matrix.v,
            mc_tmp->self_scale.x, mc_tmp->self_scale.y, mc_tmp->self_scale.z,
            TO_TURN( mc_tmp->rotate.z ), TO_TURN( mc_tmp->rotate.x ), TO_TURN( mc_tmp->rotate.y ),
            mc_tmp->pos.x, mc_tmp->pos.y, mc_tmp->pos.z );
    }

    memcpy( &( pchr->inst.matrix_cache ), mc_tmp, sizeof( matrix_cache_t ) );

    pchr->inst.matrix_cache.matrix_valid = true;

    return true;
}

//--------------------------------------------------------------------------------------------
bool apply_matrix_cache( GameObject * pchr, matrix_cache_t * mc_tmp )
{
    /// @author BB
    /// @details request that the info in the matrix cache mc_tmp, be used to
    ///               make a matrix for the character pchr.

    bool applied = false;

    if ( nullptr == ( pchr ) ) return false;
    if ( NULL == mc_tmp || !mc_tmp->valid ) return false;

    if ( 0 != ( MAT_WEAPON & mc_tmp->type_bits ) )
    {
        if ( _gameObjects.exists( mc_tmp->grip_chr ) )
        {
            applied = apply_one_weapon_matrix( pchr, mc_tmp );
        }
        else
        {
            matrix_cache_t * mcache = &( pchr->inst.matrix_cache );

            // !!!the mc_tmp was mis-labeled as a MAT_WEAPON!!!
            make_one_character_matrix( GET_INDEX_PCHR( pchr ) );

            // recover the matrix_cache values from the character
            SET_BIT( mcache->type_bits, MAT_CHARACTER );
            if ( mcache->matrix_valid )
            {
                mcache->valid     = true;
                mcache->type_bits = MAT_CHARACTER;

                mcache->self_scale.x =
                    mcache->self_scale.y =
                        mcache->self_scale.z = pchr->fat;

                mcache->grip_scale = mcache->self_scale;

                mcache->rotate.x = CLIP_TO_16BITS( pchr->ori.map_twist_facing_x - MAP_TURN_OFFSET );
                mcache->rotate.y = CLIP_TO_16BITS( pchr->ori.map_twist_facing_y - MAP_TURN_OFFSET );
                mcache->rotate.z = pchr->ori.facing_z;

                mcache->pos =pchr->getPosition();

                applied = true;
            }
        }
    }
    else if ( 0 != ( MAT_CHARACTER & mc_tmp->type_bits ) )
    {
        applied = apply_one_character_matrix( pchr, mc_tmp );
    }

    if ( applied )
    {
        chr_instance_apply_reflection_matrix( &( pchr->inst ), pchr->enviro.grid_level );
    }

    return applied;
}

//--------------------------------------------------------------------------------------------
int cmp_matrix_cache( const void * vlhs, const void * vrhs )
{
    /// @author BB
    /// @details check for differences between the data pointed to
    ///     by vlhs and vrhs, assuming that they point to matrix_cache_t data.
    ///
    ///    The function is implemented this way so that in pronciple
    ///    if could be used in a function like qsort().
    ///
    ///    We could almost certainly make something easier and quicker by
    ///    using the function memcmp()

    int   itmp, cnt;
    float ftmp;

    matrix_cache_t * plhs = ( matrix_cache_t * )vlhs;
    matrix_cache_t * prhs = ( matrix_cache_t * )vrhs;

    // handle problems with pointers
    if ( plhs == prhs )
    {
        return 0;
    }
    else if ( NULL == plhs )
    {
        return 1;
    }
    else if ( NULL == prhs )
    {
        return -1;
    }

    // handle one of both if the matrix caches being invalid
    if ( !plhs->valid && !prhs->valid )
    {
        return 0;
    }
    else if ( !plhs->valid )
    {
        return 1;
    }
    else if ( !prhs->valid )
    {
        return -1;
    }

    // handle differences in the type
    itmp = plhs->type_bits - prhs->type_bits;
    if ( 0 != itmp ) goto cmp_matrix_cache_end;

    //---- check for differences in the MAT_WEAPON data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_WEAPON ) )
    {
        itmp = ( signed )REF_TO_INT( plhs->grip_chr ) - ( signed )REF_TO_INT( prhs->grip_chr );
        if ( 0 != itmp ) goto cmp_matrix_cache_end;

        itmp = ( signed )plhs->grip_slot - ( signed )prhs->grip_slot;
        if ( 0 != itmp ) goto cmp_matrix_cache_end;

        for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
        {
            itmp = ( signed )plhs->grip_verts[cnt] - ( signed )prhs->grip_verts[cnt];
            if ( 0 != itmp ) goto cmp_matrix_cache_end;
        }

        // handle differences in the scale of our mount
        for ( cnt = 0; cnt < 3; cnt ++ )
        {
            ftmp = plhs->grip_scale.v[cnt] - prhs->grip_scale.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }
    }

    //---- check for differences in the MAT_CHARACTER data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_CHARACTER ) )
    {
        // handle differences in the "Euler" rotation angles in 16-bit form
        for ( cnt = 0; cnt < 3; cnt++ )
        {
            ftmp = plhs->rotate.v[cnt] - prhs->rotate.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }

        // handle differences in the translate vector
        for ( cnt = 0; cnt < 3; cnt++ )
        {
            ftmp = plhs->pos.v[cnt] - prhs->pos.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }
    }

    //---- check for differences in the shared data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_WEAPON ) || HAS_SOME_BITS( plhs->type_bits, MAT_CHARACTER ) )
    {
        // handle differences in our own scale
        for ( cnt = 0; cnt < 3; cnt ++ )
        {
            ftmp = plhs->self_scale.v[cnt] - prhs->self_scale.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }
    }

    // if it got here, the data is all the same
    itmp = 0;

cmp_matrix_cache_end:

    return SGN( itmp );
}

//--------------------------------------------------------------------------------------------
egolib_rv matrix_cache_needs_update( GameObject * pchr, matrix_cache_t * pmc )
{
    /// @author BB
    /// @details determine whether a matrix cache has become invalid and needs to be updated

    matrix_cache_t local_mc;
    bool needs_cache_update;

    if ( nullptr == ( pchr ) ) return rv_error;

    if ( NULL == pmc ) pmc = &local_mc;

    // get the matrix data that is supposed to be used to make the matrix
    chr_get_matrix_cache( pchr, pmc );

    // compare that data to the actual data used to make the matrix
    needs_cache_update = ( 0 != cmp_matrix_cache( pmc, &( pchr->inst.matrix_cache ) ) );

    return needs_cache_update ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_update_matrix( GameObject * pchr, bool update_size )
{
    /// @author BB
    /// @details Do everything necessary to set the current matrix for this character.
    ///     This might include recursively going down the list of this character's mounts, etc.
    ///
    ///     Return true if a new matrix is applied to the character, false otherwise.

    egolib_rv      retval;
    bool         needs_update = false;
    bool         applied      = false;
    matrix_cache_t mc_tmp;
    matrix_cache_t *pchr_mc = NULL;

    if ( nullptr == ( pchr ) ) return rv_error;
    pchr_mc = &( pchr->inst.matrix_cache );

    // recursively make sure that any mount matrices are updated
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        egolib_rv attached_update = rv_error;

        attached_update = chr_update_matrix( _gameObjects.get( pchr->attachedto ), true );

        // if this fails, we should probably do something...
        if ( rv_error == attached_update )
        {
            // there is an error so this matrix is not defined and no readon to go farther
            pchr_mc->matrix_valid = false;
            return attached_update;
        }
        else if ( rv_success == attached_update )
        {
            // the holder/mount matrix has changed.
            // this matrix is no longer valid.
            pchr_mc->matrix_valid = false;
        }
    }

    // does the matrix cache need an update at all?
    retval = matrix_cache_needs_update( pchr, &mc_tmp );
    if ( rv_error == retval ) return rv_error;
    needs_update = ( rv_success == retval );

    // Update the grip vertices no matter what (if they are used)
    if ( HAS_SOME_BITS( mc_tmp.type_bits, MAT_WEAPON ) && _gameObjects.exists( mc_tmp.grip_chr ) )
    {
        egolib_rv grip_retval;
        GameObject   * ptarget = _gameObjects.get( mc_tmp.grip_chr );

        // has that character changes its animation?
        grip_retval = ( egolib_rv )chr_instance_update_grip_verts( &( ptarget->inst ), mc_tmp.grip_verts, GRIP_VERTS );

        if ( rv_error   == grip_retval ) return rv_error;
        if ( rv_success == grip_retval ) needs_update = true;
    }

    // if it is not the same, make a new matrix with the new data
    applied = false;
    if ( needs_update )
    {
        // we know the matrix is not valid
        pchr_mc->matrix_valid = false;

        applied = apply_matrix_cache( pchr, &mc_tmp );
    }

    if ( applied && update_size )
    {
        // call chr_update_collision_size() but pass in a false value to prevent a recursize call
        chr_update_collision_size( pchr, false );
    }

    return applied ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF chr_has_inventory_idsz( const CHR_REF ichr, IDSZ idsz, bool equipped )
{
    /// @author BB
    /// @details check the pack a matching item

    bool matches_equipped;
    CHR_REF result;
    GameObject * pchr;

    if ( !_gameObjects.exists( ichr ) ) return INVALID_CHR_REF;
    pchr = _gameObjects.get( ichr );

    result = INVALID_CHR_REF;

    PACK_BEGIN_LOOP( pchr->inventory, pitem, item )
    {
        matches_equipped = ( !equipped || pitem->isequipped );

        if ( chr_is_type_idsz( item, idsz ) && matches_equipped )
        {
            result = item;
            break;
        }
    }
    PACK_END_LOOP();

    return result;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_holding_idsz( const CHR_REF ichr, IDSZ idsz )
{
    /// @author BB
    /// @details check the character's hands for a matching item

    bool found;
    CHR_REF item, tmp_item;
    GameObject * pchr;

    if ( !_gameObjects.exists( ichr ) ) return INVALID_CHR_REF;
    pchr = _gameObjects.get( ichr );

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
    ///               return the index of the found item, or MAX_CHR if not found. Also return
    ///               the previous pack item in *pack_last, or MAX_CHR if it was not in a pack.

    bool found;
    CHR_REF item;
    GameObject * pchr;

    if ( !_gameObjects.exists( ichr ) ) return INVALID_CHR_REF;
    pchr = _gameObjects.get( ichr );

    // Check the pack
    item       = INVALID_CHR_REF;
    found      = false;

    if ( !found )
    {
        item = chr_holding_idsz( ichr, idsz );
        found = _gameObjects.exists( item );
    }

    if ( !found )
    {
        item = chr_has_inventory_idsz( ichr, idsz, equipped );
        found = _gameObjects.exists( item );
    }

    return item;
}

//--------------------------------------------------------------------------------------------
bool chr_can_see_invis( const GameObject * pchr, const GameObject * pobj )
{
    /// @author BB
    /// @details can ichr see iobj?

    int     alpha;

    if ( NULL == pchr || NULL == pobj ) return false;

    /// @note ZF@> Invictus characters can always see through darkness (spells, items, quest handlers, etc.)
    if ( pchr->invictus ) return true;

    alpha = pobj->inst.alpha;
    if ( 0 != pchr->see_invisible_level )
    {
        alpha = get_alpha( alpha, exp( 0.32f * ( float )pchr->see_invisible_level ) );
    }
    alpha = CLIP( alpha, 0, 255 );

    return alpha >= INVISIBLE;
}

//--------------------------------------------------------------------------------------------
bool chr_can_see_dark( const GameObject * pchr, const GameObject * pobj )
{
    /// @author BB
    /// @details can ichr see iobj?
    int     light, self_light, enviro_light;

    if ( NULL == pchr || NULL == pobj ) return false;

    enviro_light = ( pobj->inst.alpha * pobj->inst.max_light ) * INV_FF;
    self_light   = ( pobj->inst.light == 255 ) ? 0 : pobj->inst.light;
    light        = std::max( enviro_light, self_light );

    if ( 0 != pchr->darkvision_level )
    {
        light *= exp( 0.32f * ( float )pchr->darkvision_level );
    }

    // Scenery, spells and quest objects can always see through darkness
    // Checking pchr->invictus is not enough, since that could be temporary
    // and not indicate the appropriate objects
    if( _profileSystem.getProfile(pchr->profile_ref)->isInvincible() ) {
        return true;
    }

    return light >= INVISIBLE;
}

//--------------------------------------------------------------------------------------------
bool chr_can_see_object( const GameObject * pchr, const GameObject * pobj )
{
    /// @author BB
    /// @details can ichr see iobj?

    bool too_dark, too_invis;

    if ( !INGAME_PCHR( pchr ) || !INGAME_PCHR( pobj ) )
    {
        return false;
    }

    too_dark  = !chr_can_see_dark( pchr, pobj );
    too_invis = !chr_can_see_invis( pchr, pobj );

    return !too_dark && !too_invis;
}

//--------------------------------------------------------------------------------------------
int chr_get_price( const CHR_REF ichr )
{
    /// @author BB
    /// @details determine the correct price for an item

    Uint16  iskin;
    float   price;

    GameObject * pchr;

    if ( !_gameObjects.exists( ichr ) ) return 0;
    pchr = _gameObjects.get( ichr );

    // Make sure spell books are priced according to their spell and not the book itself
    PRO_REF slotNumber = INVALID_PRO_REF;
    if ( pchr->profile_ref == SPELLBOOK )
    {
        slotNumber = pchr->basemodel_ref;
        iskin = 0;
    }
    else
    {
        slotNumber  = pchr->profile_ref;
        iskin = pchr->skin;
    }

    std::shared_ptr<ObjectProfile> profile = _profileSystem.getProfile(slotNumber);
    if(!profile) {
        return 0;
    }

    price = profile->getSkinInfo(iskin).cost;

    // Items spawned in shops are more valuable
    if ( !pchr->isshopitem ) price *= 0.5f;

    // base the cost on the number of items/charges
    if ( profile->isStackable() )
    {
        price *= pchr->ammo;
    }
    else if ( profile->isRangedWeapon() && pchr->ammo < pchr->ammomax )
    {
        if ( 0 != pchr->ammomax )
        {
            price *= ( float ) pchr->ammo / ( float ) pchr->ammomax;
        }
    }

    return ( int )price;
}

//--------------------------------------------------------------------------------------------
void chr_set_floor_level( GameObject * pchr, const float level )
{
    if ( nullptr == ( pchr ) ) return;

    if ( level != pchr->enviro.floor_level )
    {
        pchr->enviro.floor_level = level;
    }
}

//--------------------------------------------------------------------------------------------
void chr_set_redshift( GameObject * pchr, const int rs )
{
    if ( nullptr == ( pchr ) ) return;

    pchr->inst.redshift = CLIP( rs, 0, 9 );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, false );
}

//--------------------------------------------------------------------------------------------
void chr_set_grnshift( GameObject * pchr, const int gs )
{
    if ( nullptr == ( pchr ) ) return;

    pchr->inst.grnshift = CLIP( gs, 0, 9 );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, false );
}

//--------------------------------------------------------------------------------------------
void chr_set_blushift( GameObject * pchr, const int bs )
{
    if ( nullptr == ( pchr ) ) return;

    pchr->inst.blushift = CLIP( bs, 0, 9 );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, false );
}

//--------------------------------------------------------------------------------------------
/// @brief Set the fat value of a character.
/// @param chr the character
/// @param fat the new fat value
/// @remark The fat value influences the character size.
void chr_set_fat(GameObject *chr, const float fat)
{
	if (nullptr == (chr)) return;
	chr->fat = fat;
	chr_update_size(chr);
}

//--------------------------------------------------------------------------------------------
/// @brief Set the (base) height of a character.
/// @param chr the character
/// @param height the new height
/// @remark The (base) height influences the character size.
void chr_set_height(GameObject *chr, const float height)
{
	if (nullptr == (chr)) return;
	chr->bump_save.height = std::max(height, 0.0f);
	chr_update_size(chr);
}

//--------------------------------------------------------------------------------------------
/// @brief Set the (base) width of a character.
/// @param chr the character
/// @param width the new width
/// @remark Also modifies the shadow size.
void chr_set_width(GameObject *chr, const float width)
{
	if (nullptr == (chr)) return;

	float ratio = std::abs(width / chr->bump_stt.size);

	chr->shadow_size_stt *= ratio;
	chr->bump_stt.size *= ratio;
	chr->bump_stt.size_big *= ratio;

	chr_update_size(chr);
}

//--------------------------------------------------------------------------------------------
/// @brief Set the scale of a character such that it matches the given size.
/// @param chr the character
/// @param size the size
void chr_set_size(GameObject *chr, const float size)
{
	if (nullptr == (chr)) return;

	float ratio = size / chr->bump.size;

	chr->shadow_size_save *= ratio;
	chr->bump_save.size *= ratio;
	chr->bump_save.size_big *= ratio;
	chr->bump_save.height *= ratio;

	chr_update_size(chr);
}

//--------------------------------------------------------------------------------------------
/// @brief Update the (base) shadow size of a character.
/// @param chr the character
/// @param size the base shadow size
void chr_set_shadow(GameObject *chr, const float size)
{
	/// @author BB
	/// @details update the base shadow size

	if (nullptr == (chr)) return;

	chr->shadow_size_save = size;

	chr_update_size(chr);
}

//--------------------------------------------------------------------------------------------
bool chr_getMatUp(GameObject *pchr, fvec3_t& up)
{
	bool rv;

	if (nullptr == (pchr)) return false;

	if (!chr_matrix_valid(pchr))
	{
		chr_update_matrix(pchr, true);
	}

	rv = false;
	if (chr_matrix_valid(pchr))
	{
		rv = mat_getChrUp(pchr->inst.matrix, up);
	}

	if (!rv)
	{
		// assume default Up is +z
		up[kZ] = 1.0f;
		up[kX] = up[kY] = 0.0f;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool chr_getMatRight(GameObject *pchr, fvec3_t& right)
{
	bool rv;

	if (nullptr == (pchr)) return false;

	if (!chr_matrix_valid(pchr))
	{
		chr_update_matrix(pchr, true);
	}

	rv = false;
	if (chr_matrix_valid(pchr))
	{
		rv = mat_getChrRight(pchr->inst.matrix, right);
	}

	if (!rv)
	{
		// assume default Right is +y
		right[kY] = 1.0f;
		right[kX] = right[kZ] = 0.0f;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool chr_getMatForward(GameObject *pchr, fvec3_t& forward)
{
	bool rv;

	if (nullptr == (pchr)) return false;

	if (!chr_matrix_valid(pchr))
	{
		chr_update_matrix(pchr, true);
	}

	rv = false;
	if (chr_matrix_valid(pchr))
	{
		rv = mat_getChrForward(pchr->inst.matrix, forward);
	}

	if (!rv)
	{
		// assume default Forward is +x
		forward[kX] = 1.0f;
		forward[kY] = forward[kZ] = 0.0f;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool chr_getMatTranslate(GameObject *pchr, fvec3_t& translate)
{
	bool rv;

	if (nullptr == (pchr)) return false;

	if (!chr_matrix_valid(pchr))
	{
		chr_update_matrix(pchr, true);
	}

	rv = false;
	if (chr_matrix_valid(pchr))
	{
		rv = mat_getTranslate(pchr->inst.matrix, translate);
	}

	if (!rv)
	{
		translate = pchr->getPosition();
	}

	return true;
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
    ///               the depth of the search, and make sure that ichr != _gameObjects.get(object)->attachedto

    int cnt;
    CHR_REF original_object, object, object_next;

    if ( !_gameObjects.exists( ichr ) ) return INVALID_CHR_REF;

    original_object = object = ichr;
    for ( cnt = 0, object = ichr; cnt < MAX_CHR; cnt++ )
    {
        // check for one of the ending condiitons
        if ( non_item && !_gameObjects.get(object)->isitem )
        {
            break;
        }

        // grab the next object in the list
        object_next = _gameObjects.get(object)->attachedto;

        // check for an end of the list
        if ( !_gameObjects.exists( object_next ) )
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
Uint32 chr_get_framefx( GameObject * pchr )
{
    if ( nullptr == ( pchr ) ) return 0;

    return chr_instance_get_framefx( &( pchr->inst ) );
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_invalidate_child_instances( GameObject * pchr )
{
    int cnt;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    // invalidate vlst_cache of everything in this character's holdingwhich array
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        CHR_REF iitem = pchr->holdingwhich[cnt];
        if ( !_gameObjects.exists( iitem ) ) continue;

        // invalidate the matrix_cache
        _gameObjects.get(iitem)->inst.matrix_cache.valid = false;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_set_action( GameObject * pchr, int action, bool action_ready, bool override_action )
{
    egolib_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egolib_rv )chr_instance_set_action( &( pchr->inst ), action, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_start_anim( GameObject * pchr, int action, bool action_ready, bool override_action )
{
    egolib_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egolib_rv )chr_instance_start_anim( &( pchr->inst ), action, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_set_anim( GameObject * pchr, int action, int frame, bool action_ready, bool override_action )
{
    egolib_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egolib_rv )chr_instance_set_anim( &( pchr->inst ), action, frame, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_increment_action( GameObject * pchr )
{
    egolib_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egolib_rv )chr_instance_increment_action( &( pchr->inst ) );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_increment_frame( GameObject * pchr )
{
    egolib_rv retval;
    mad_t * pmad;
    int mount_action;
    CHR_REF imount;
    bool needs_keep;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;
    imount = pchr->attachedto;

    pmad = chr_get_pmad( GET_INDEX_PCHR( pchr ) );
    if ( NULL == pmad ) return rv_error;

    // do we need to keep this animation?
    needs_keep = false;

    if ( !_gameObjects.exists( imount ) )
    {
        imount = INVALID_CHR_REF;
        mount_action = ACTION_DA;
    }
    else
    {
        // determine what kind of action we are going to substitute for a riding character
        if ( _gameObjects.exists( pchr->holdingwhich[SLOT_LEFT] ) || _gameObjects.exists( pchr->holdingwhich[SLOT_RIGHT] ) )
        {
            // if the character is holding anything, make the animation
            // ACTION_MH == "sitting" so that it does not look so silly

            mount_action = mad_get_action_ref( pchr->inst.imad, ACTION_MH );
            if ( ACTION_MH != mount_action )
            {
                // no real sitting animation. set the animation to keep
                needs_keep = true;
            }
        }
        else
        {
            // if it is not holding anything, go for the riding animation
            mount_action = mad_get_action_ref( pchr->inst.imad, ACTION_MI );
            if ( ACTION_MI != mount_action )
            {
                // no real riding animation. set the animation to keep
                needs_keep = true;
            }
        }
    }

    retval = ( egolib_rv )chr_instance_increment_frame( &( pchr->inst ), pmad, imount, mount_action );
    if ( rv_success != retval ) return retval;

    /// @note BB@> this did not work as expected...
    // set keep if needed
    //if ( needs_keep )
    //{
    //    chr_instance_set_action_keep( &( pchr->inst ), true );
    //}

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_play_action( GameObject * pchr, int action, bool action_ready )
{
    egolib_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egolib_rv )chr_instance_play_action( &( pchr->inst ), action, action_ready );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool chr_heal_mad( GameObject * pchr )
{
    // try to repair a bad model if it exists

    MAD_REF          imad_tmp = INVALID_MAD_REF;
    chr_instance_t * pinst    = NULL;

    if ( nullptr == ( pchr ) ) return false;
    pinst = &( pchr->inst );

    if ( LOADED_MAD( pinst->imad ) ) return true;

    // get whatever mad index the profile says to use
    imad_tmp = _profileSystem.getProfile( pchr->profile_ref )->getModelRef();

    // set the mad index to whatever the profile says, even if it is wrong,
    // since we know that our current one is invalid
    chr_instance_set_mad( pinst, imad_tmp );

    // if we healed the mad index, make sure to recalculate the collision size
    if ( LOADED_MAD( pinst->imad ) )
    {
        chr_update_collision_size( pchr, true );
    }

    return LOADED_MAD( pinst->imad );
}

//--------------------------------------------------------------------------------------------
MAD_REF chr_get_imad( const CHR_REF ichr )
{
    GameObject * pchr   = NULL;
    MAD_REF retval = INVALID_MAD_REF;

    pchr = _gameObjects.get( ichr );
    if ( NULL == pchr ) return retval;

    // heal the mad index if it is invalid
    if ( chr_heal_mad( pchr ) )
    {
        retval = pchr->inst.imad;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
mad_t * chr_get_pmad( const CHR_REF ichr )
{
    return MadStack.get_ptr( chr_get_imad( ichr ) );
}

//--------------------------------------------------------------------------------------------
bool chr_set_maxaccel( GameObject * pchr, float new_val )
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
GameObject * chr_set_ai_state( GameObject * pchr, int state )
{
    if ( nullptr == ( pchr ) ) return pchr;

    pchr->ai.state = state;

    chr_update_hide( pchr );

    return pchr;
}

//--------------------------------------------------------------------------------------------
bool chr_calc_grip_cv( GameObject * pmount, int grip_offset, oct_bb_t * grip_cv_ptr, const bool shift_origin )
{
    /// @author BB
    /// @details use a standard size for the grip

    // take the character size from the adventurer model
    const float default_chr_height = 88.0f;
    const float default_chr_radius = 22.0f;

    int              cnt;
    chr_instance_t * pmount_inst;
    oct_bb_t         tmp_cv;

    int     grip_count;
    Uint16  grip_verts[GRIP_VERTS];
    fvec4_t grip_points[GRIP_VERTS];
    fvec4_t grip_nupoints[GRIP_VERTS];
    bumper_t bmp;

    if ( nullptr == ( pmount ) ) return false;

    // alias this variable for notation simplicity
    pmount_inst = &( pmount->inst );

    // tune the grip radius
    bmp.size     = default_chr_radius * pmount->fat * 0.75f;
    bmp.height   = default_chr_height * pmount->fat * 2.00f;
    bmp.size_big = bmp.size * SQRT_TWO;

    oct_bb_set_bumper( &tmp_cv, bmp );

    // move the vertical bounding box down a little
    tmp_cv.mins[OCT_Z] -= bmp.height * 0.25f;
    tmp_cv.maxs[OCT_Z] -= bmp.height * 0.25f;

    // get appropriate vertices for this model's grip
    {
        // do the automatic vertex update
        int vert_stt = ( signed )( pmount_inst->vrt_count ) - ( signed )grip_offset;
        if ( vert_stt < 0 ) return false;

        if ( gfx_error == chr_instance_update_vertices( pmount_inst, vert_stt, vert_stt + grip_offset, false ) )
        {
            grip_count = 0;
            for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
            {
                grip_verts[cnt] = 0xFFFF;
            }
        }
        else
        {
            // calculate the grip vertices
            for ( grip_count = 0, cnt = 0; cnt < GRIP_VERTS && ( size_t )( vert_stt + cnt ) < pmount_inst->vrt_count; grip_count++, cnt++ )
            {
                grip_verts[cnt] = vert_stt + cnt;
            }
            for ( /* nothing */ ; cnt < GRIP_VERTS; cnt++ )
            {
                grip_verts[cnt] = 0xFFFF;
            }
        }

        // calculate grip_origin and grip_up
        if ( 4 == grip_count )
        {
            // Calculate grip point locations with linear interpolation and other silly things
            convert_grip_to_local_points( pmount, grip_verts, grip_points );
        }
        else if ( grip_count > 0 )
        {
            // Calculate grip point locations with linear interpolation and other silly things
            convert_grip_to_local_points( pmount, grip_verts, grip_points );

            if ( grip_count < 2 )
            {
                fvec4_self_clear( grip_points[2].v );
                grip_points[2].y = 1.0f;
            }

            if ( grip_count < 3 )
            {
                fvec4_self_clear( grip_points[3].v );
                grip_points[3].z = 1.0f;
            }
        }
        else if ( 0 == grip_count )
        {
            // choose the location point at the model's origin and axis aligned

            for ( cnt = 0; cnt < 4; cnt++ )
            {
                fvec4_self_clear( grip_points[cnt].v );
            }

            grip_points[1].x = 1.0f;
            grip_points[2].y = 1.0f;
            grip_points[3].z = 1.0f;
        }

        // fix the 4th component depending on the whether we shift the origin of the cv
        if ( !shift_origin )
        {
            for ( cnt = 0; cnt < grip_count; cnt++ )
            {
                grip_points[cnt].w = 0.0f;
            }
        }
    }

    // transform the vertices to calculate the grip_vecs[]
    // we only need one vertex
    pmount_inst->matrix.transform(grip_points, grip_nupoints, 1);

    // add in the "origin" of the grip, if necessary
    if ( NULL != grip_cv_ptr )
    {
        oct_bb_add_fvec3( &tmp_cv, fvec3_t(grip_nupoints[0][kX],grip_nupoints[0][kY],grip_nupoints[0][kZ]), grip_cv_ptr );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION (previously inline functions)
//--------------------------------------------------------------------------------------------
CHR_REF team_get_ileader( const TEAM_REF iteam )
{
    CHR_REF ichr;

    if ( iteam >= TEAM_MAX ) return INVALID_CHR_REF;

    ichr = TeamStack.lst[iteam].leader;
    if ( !_gameObjects.exists( ichr ) ) return INVALID_CHR_REF;

    return ichr;
}

//--------------------------------------------------------------------------------------------
GameObject  * team_get_pleader( const TEAM_REF iteam )
{
    CHR_REF ichr;

    if ( iteam >= TEAM_MAX ) return NULL;

    ichr = TeamStack.lst[iteam].leader;
    if ( !_gameObjects.exists( ichr ) ) return NULL;

    return _gameObjects.get( ichr );
}

//--------------------------------------------------------------------------------------------
bool team_hates_team( const TEAM_REF ipredator_team, const TEAM_REF iprey_team )
{
    /// @author BB
    /// @details a wrapper function for access to the hatesteam data

    if ( ipredator_team >= TEAM_MAX || iprey_team >= TEAM_MAX ) return false;

    return TeamStack.lst[ipredator_team].hatesteam[ REF_TO_INT( iprey_team )];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PRO_REF chr_get_ipro( const CHR_REF ichr )
{
    GameObject * pchr;

    if ( !_gameObjects.exists( ichr ) ) return INVALID_PRO_REF;
    pchr = _gameObjects.get( ichr );

    if ( !_profileSystem.isValidProfileID( pchr->profile_ref ) ) return INVALID_PRO_REF;

    return pchr->profile_ref;
}

//--------------------------------------------------------------------------------------------
TEAM_REF chr_get_iteam( const CHR_REF ichr )
{

    if ( !_gameObjects.exists( ichr ) ) return static_cast<TEAM_REF>(TEAM_DAMAGE);
    GameObject * pchr = _gameObjects.get( ichr );

    return static_cast<TEAM_REF>(pchr->team);
}

//--------------------------------------------------------------------------------------------
TEAM_REF chr_get_iteam_base( const CHR_REF ichr )
{
    GameObject * pchr;
    int iteam;

    if ( !_gameObjects.exists( ichr ) ) return ( TEAM_REF )TEAM_MAX;
    pchr = _gameObjects.get( ichr );

    iteam = REF_TO_INT( pchr->team_base );
    iteam = CLIP( iteam, 0, (int)TEAM_MAX );

    return ( TEAM_REF )iteam;
}

//--------------------------------------------------------------------------------------------
ObjectProfile * chr_get_ppro( const CHR_REF ichr )
{
    //This function should -never- return nullptr
    if(!_gameObjects.exists(ichr)) {
        //throw error
        return nullptr;
    }

    GameObject * pchr = _gameObjects.get( ichr );

    //This function should -never- return nullptr
    if(!_profileSystem.isValidProfileID(pchr->profile_ref)) {
        //throw error
        return nullptr;
    }

    return _profileSystem.getProfile(pchr->profile_ref).get();
}

//--------------------------------------------------------------------------------------------
team_t * chr_get_pteam( const CHR_REF ichr )
{
    GameObject * pchr;

    if ( !_gameObjects.exists( ichr ) ) return NULL;
    pchr = _gameObjects.get( ichr );

    return TeamStack.get_ptr( pchr->team );
}

//--------------------------------------------------------------------------------------------
team_t * chr_get_pteam_base( const CHR_REF ichr )
{
    GameObject * pchr;

    if ( !_gameObjects.exists( ichr ) ) return NULL;
    pchr = _gameObjects.get( ichr );

    return TeamStack.get_ptr( pchr->team_base );
}

//--------------------------------------------------------------------------------------------
ai_state_t * chr_get_pai( const CHR_REF ichr )
{
    GameObject * pchr;

    if ( !_gameObjects.exists( ichr ) ) return NULL;
    pchr = _gameObjects.get( ichr );

    return &( pchr->ai );
}

//--------------------------------------------------------------------------------------------
chr_instance_t * chr_get_pinstance( const CHR_REF ichr )
{
    GameObject * pchr;

    if ( !_gameObjects.exists( ichr ) ) return NULL;
    pchr = _gameObjects.get( ichr );

    return &( pchr->inst );
}

//--------------------------------------------------------------------------------------------
IDSZ chr_get_idsz( const CHR_REF ichr, int type )
{
    if ( !_gameObjects.exists( ichr ) ) return IDSZ_NONE;
    return chr_get_ppro(ichr)->getIDSZ(type);
}

//--------------------------------------------------------------------------------------------
bool chr_has_idsz( const CHR_REF ichr, IDSZ idsz )
{
    /// @author BB
    /// @details a wrapper for cap_has_idsz

    if ( !_gameObjects.exists( ichr ) ) return IDSZ_NONE;
    return chr_get_ppro(ichr)->hasIDSZ(idsz);
}

//--------------------------------------------------------------------------------------------
bool chr_is_type_idsz( const CHR_REF item, IDSZ test_idsz )
{
    /// @author BB
    /// @details check IDSZ_PARENT and IDSZ_TYPE to see if the test_idsz matches. If we are not
    ///     picky (i.e. IDSZ_NONE == test_idsz), then it matches any valid item.

    if ( !_gameObjects.exists( item ) ) return IDSZ_NONE;
    return chr_get_ppro(item)->hasTypeIDSZ(test_idsz);
}

//--------------------------------------------------------------------------------------------
bool chr_has_vulnie( const CHR_REF item, const PRO_REF test_profile )
{
    /// @author BB
    /// @details is item vulnerable to the type in profile test_profile?

    IDSZ vulnie;

    if ( !_gameObjects.exists( item ) ) return false;
    vulnie = chr_get_idsz( item, IDSZ_VULNERABILITY );

    // not vulnerable if there is no specific weakness
    if ( IDSZ_NONE == vulnie ) return false;
    const std::shared_ptr<ObjectProfile> &profile = _profileSystem.getProfile(test_profile);
    if (nullptr == profile) return false;

    // check vs. every IDSZ that could have something to do with attacking
    if ( vulnie == profile->getIDSZ(IDSZ_TYPE) ) return true;
    if ( vulnie == profile->getIDSZ(IDSZ_PARENT) ) return true;

    return false;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void chr_update_size( GameObject * pchr )
{
    /// @author BB
    /// @details Convert the base size values to the size values that are used in the game

    if ( nullptr == ( pchr ) ) return;

    pchr->shadow_size   = pchr->shadow_size_save   * pchr->fat;
    pchr->bump.size     = pchr->bump_save.size     * pchr->fat;
    pchr->bump.size_big = pchr->bump_save.size_big * pchr->fat;
    pchr->bump.height   = pchr->bump_save.height   * pchr->fat;

    chr_update_collision_size( pchr, true );
}

//--------------------------------------------------------------------------------------------
static void chr_init_size( GameObject * pchr, const std::shared_ptr<ObjectProfile> &profile)
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

    chr_update_size( pchr );
}
