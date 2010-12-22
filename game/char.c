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

/// @file char.c
/// @brief Implementation of character functions
/// @details

#include "char.inl"
#include "ChrList.h"

#include "mad.h"

#include "log.h"
#include "script.h"
#include "menu.h"
#include "sound.h"
#include "camera.h"
#include "input.h"
#include "passage.h"
#include "graphic.h"
#include "game.h"
#include "texture.h"
#include "ui.h"
#include "collision.h"                  //Only or detach_character_from_platform()
#include "quest.h"
#include "obj_BSP.h"

#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

#include "egoboo_math.inl"
#include "mesh.inl"

#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static IDSZ    inventory_idsz[INVEN_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INSTANTIATE_STACK( ACCESS_TYPE_NONE, cap_t, CapStack, MAX_PROFILE );
INSTANTIATE_STACK( ACCESS_TYPE_NONE, team_t, TeamStack, TEAM_MAX );

int chr_stoppedby_tests = 0;
int chr_pressure_tests = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static CHR_REF chr_pack_has_a_stack( const CHR_REF item, const CHR_REF character );
static bool_t  chr_add_pack_item( const CHR_REF item, const CHR_REF character );
static CHR_REF chr_get_pack_item( const CHR_REF character, grip_offset_t grip_off, bool_t ignorekurse );

static bool_t set_weapongrip( const CHR_REF iitem, const CHR_REF iholder, Uint16 vrt_off );

static BBOARD_REF chr_add_billboard( const CHR_REF ichr, Uint32 lifetime_secs );

static chr_t * resize_one_character( chr_t * pchr );
//static void    resize_all_characters();

static bool_t  chr_free( chr_t * pchr );

static chr_t * chr_config_ctor( chr_t * pchr );
static chr_t * chr_config_init( chr_t * pchr );
static chr_t * chr_config_deinit( chr_t * pchr );
static chr_t * chr_config_active( chr_t * pchr );
static chr_t * chr_config_dtor( chr_t * pchr );

static int get_grip_verts( Uint16 grip_verts[], const CHR_REF imount, int vrt_offset );

bool_t apply_one_character_matrix( chr_t * pchr, matrix_cache_t * mcache );
bool_t apply_one_weapon_matrix( chr_t * pweap, matrix_cache_t * mcache );

int convert_grip_to_local_points( chr_t * pholder, Uint16 grip_verts[], fvec4_t   dst_point[] );
int convert_grip_to_global_points( const CHR_REF iholder, Uint16 grip_verts[], fvec4_t   dst_point[] );

// definition that is consistent with using it as a callback in qsort() or some similar function
static int  cmp_matrix_cache( const void * vlhs, const void * vrhs );

static bool_t chr_upload_cap( chr_t * pchr, cap_t * pcap );

void cleanup_one_character( chr_t * pchr );

static void chr_log_script_time( const CHR_REF ichr );

static bool_t update_chr_darkvision( const CHR_REF character );

static fvec2_t chr_get_mesh_diff( chr_t * pchr, float test_pos[], float center_pressure );
static float   chr_get_mesh_pressure( chr_t * pchr, float test_pos[] );

static egoboo_rv chr_invalidate_child_instances( chr_t * pchr );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void character_system_begin()
{
    ChrList_init();
    init_all_cap();
}

//--------------------------------------------------------------------------------------------
void character_system_end()
{
    release_all_cap();
    ChrList_dtor();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t chr_free( chr_t * pchr )
{
    /// Free all allocated memory

    if ( !ALLOCATED_PCHR( pchr ) )
    {
        EGOBOO_ASSERT( NULL == pchr->inst.vrt_lst );
        return bfalse;
    }

    // do some list clean-up
    disenchant_character( GET_REF_PCHR( pchr ) );

    // deallocate
    BillboardList_free_one( REF_TO_INT( pchr->ibillboard ) );

    LoopedList_remove( pchr->loopedsound_channel );

    chr_instance_dtor( &( pchr->inst ) );
    BSP_leaf_dtor( &( pchr->bsp_leaf ) );
    ai_state_dtor( &( pchr->ai ) );

    EGOBOO_ASSERT( NULL == pchr->inst.vrt_lst );

    return btrue;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_ctor( chr_t * pchr )
{
    /// @details BB@> initialize the character data to safe values
    ///     since we use memset(..., 0, ...), everything == 0 == false == 0.0f
    ///     statements are redundant

    int cnt;
    obj_data_t save_base;
    obj_data_t * pbase;

    if ( NULL == pchr ) return pchr;

    // grab the base object
    pbase = POBJ_GET_PBASE( pchr );
    if ( NULL == pbase ) return NULL;

    //---- construct the character object

    // save the base object data
    memcpy( &save_base, pbase, sizeof( obj_data_t ) );

    if ( ALLOCATED_PCHR( pchr ) )
    {
        // deallocate any existing data
        chr_free( pchr );

        EGOBOO_ASSERT( NULL == pchr->inst.vrt_lst );
    }

    // clear out all data
    memset( pchr, 0, sizeof( *pchr ) );

    // restore the base object data
    memcpy( pbase, &save_base, sizeof( obj_data_t ) );

    // reset the base counters
    pbase->update_count = 0;
    pbase->frame_count = 0;

    // IMPORTANT!!!
    pchr->ibillboard = INVALID_BILLBOARD;
    pchr->sparkle = NOSPARKLE;
    pchr->loopedsound_channel = INVALID_SOUND_CHANNEL;

    // Set up model stuff
    pchr->inwhich_slot = SLOT_LEFT;
    pchr->hitready = btrue;
    pchr->bore_timer = BORETIME;
    pchr->careful_timer = CAREFULTIME;

    // Enchant stuff
    pchr->firstenchant = ( ENC_REF ) MAX_ENC;
    pchr->undoenchant = ( ENC_REF ) MAX_ENC;
    pchr->missiletreatment = MISSILE_NORMAL;

    // Character stuff
    pchr->turnmode = TURNMODE_VELOCITY;
    pchr->alive = btrue;

    // Jumping
    pchr->jump_timer = JUMPDELAY;

    // Grip info
    pchr->attachedto = ( CHR_REF )MAX_CHR;
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        pchr->holdingwhich[cnt] = ( CHR_REF )MAX_CHR;
    }

    // pack/inventory info
    pchr->pack.next = ( CHR_REF )MAX_CHR;
    for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
    {
        pchr->inventory[cnt] = ( CHR_REF )MAX_CHR;
    }

    // Set up position
    pchr->ori.map_facing_y = MAP_TURN_OFFSET;  // These two mean on level surface
    pchr->ori.map_facing_x = MAP_TURN_OFFSET;

    // start the character out in the "dance" animation
    chr_start_anim( pchr, ACTION_DA, btrue, btrue );

    // I think we have to set the dismount timer, otherwise objects that
    // are spawned by chests will behave strangely...
    // nope this did not fix it
    // ZF@> If this is != 0 then scorpion claws and riders are dropped at spawn (non-item objects)
    pchr->dismount_timer  = 0;
    pchr->dismount_object = ( CHR_REF )MAX_CHR;

    // set all of the integer references to invalid values
    pchr->firstenchant = ( ENC_REF ) MAX_ENC;
    pchr->undoenchant  = ( ENC_REF ) MAX_ENC;
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        pchr->holdingwhich[cnt] = ( CHR_REF )MAX_CHR;
    }

    pchr->pack.next = ( CHR_REF )MAX_CHR;
    for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
    {
        pchr->inventory[cnt] = ( CHR_REF )MAX_CHR;
    }

    pchr->onwhichplatform_ref    = ( CHR_REF )MAX_CHR;
    pchr->onwhichplatform_update = 0;
    pchr->targetplatform_ref     = ( CHR_REF )MAX_CHR;

    // all movements valid
    pchr->movement_bits   = ( unsigned )( ~0 );

    // not a player
    pchr->is_which_player = MAX_PLAYER;

    // initialize the bsp node for this character
    pchr->bsp_leaf.data      = pchr;
    pchr->bsp_leaf.data_type = BSP_LEAF_CHR;
    pchr->bsp_leaf.index     = GET_INDEX_PCHR( pchr );

    //---- call the constructors of the "has a" classes

    // set the insance values to safe values
    chr_instance_ctor( &( pchr->inst ) );

    // intialize the ai_state
    ai_state_ctor( &( pchr->ai ) );

    // initialize the bsp node for this character
    BSP_leaf_ctor( &( pchr->bsp_leaf ), 3, pchr, BSP_LEAF_CHR );
    pchr->bsp_leaf.index = GET_INDEX_PCHR( pchr );

    // initialize the physics
    phys_data_ctor( &( pchr->phys ) );

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_dtor( chr_t * pchr )
{
    if ( NULL == pchr ) return pchr;

    // destruct/free any allocated data
    chr_free( pchr );

    // Destroy the base object.
    // Sets the state to ego_object_terminated automatically.
    POBJ_TERMINATE( pchr );

    return pchr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int chr_count_free()
{
    return ChrList.free_count;
}

//--------------------------------------------------------------------------------------------
int chr_count_used()
{
    return ChrList.used_count; // MAX_CHR - ChrList.free_count;
}

//--------------------------------------------------------------------------------------------
egoboo_rv flash_character_height( const CHR_REF character, Uint8 valuelow, Sint16 low,
                                  Uint8 valuehigh, Sint16 high )
{
    /// @details ZZ@> This function sets a character's lighting depending on vertex height...
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
        else
        {
            if ( z > high )
            {
                pinst->vrt_lst[cnt].col[RR] =
                    pinst->vrt_lst[cnt].col[GG] =
                        pinst->vrt_lst[cnt].col[BB] = valuehigh;
            }
            else
            {
                Uint8 valuemid = ( valuehigh * ( z - low ) / ( high - low ) ) +
                                 ( valuelow * ( high - z ) / ( high - low ) );

                pinst->vrt_lst[cnt].col[RR] =
                    pinst->vrt_lst[cnt].col[GG] =
                        pinst->vrt_lst[cnt].col[BB] =  valuemid;
            }
        }
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
void chr_set_enviro_grid_level( chr_t * pchr, float level )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    if ( level != pchr->enviro.grid_level )
    {
        pchr->enviro.grid_level = level;

        apply_reflection_matrix( &( pchr->inst ), level );
    }
}

//--------------------------------------------------------------------------------------------
bool_t chr_copy_enviro( chr_t * chr_psrc, chr_t * chr_pdst )
{
    /// BB@> do a deep copy on the character's enviro data

    chr_environment_t * psrc, * pdst;

    if ( NULL == chr_psrc || NULL == chr_pdst ) return bfalse;

    if ( chr_psrc == chr_pdst ) return btrue;

    psrc = &( chr_psrc->enviro );
    pdst = &( chr_pdst->enviro );

    // use the special function to set the grid level
    // this must done first so that the character's reflection data is set properly
    chr_set_enviro_grid_level( chr_pdst, psrc->grid_level );

    // now just copy the other data.
    // use memmove() in the odd case the regions overlap
    memmove( psrc, pdst, sizeof( *psrc ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders()
{
    /// @details ZZ@> This function keeps weapons near their holders

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        CHR_REF iattached = pchr->attachedto;

        if ( INGAME_CHR( iattached ) )
        {
            chr_t * pattached = ChrList.lst + iattached;

            // Keep in hand weapons with iattached
            if ( chr_matrix_valid( pchr ) )
            {
                chr_set_pos( pchr, mat_getTranslate_v( pchr->inst.matrix.v ) );
            }
            else
            {
                chr_set_pos( pchr, chr_get_pos_v( pattached ) );
            }

            pchr->ori.facing_z = pattached->ori.facing_z;

            // Copy this stuff ONLY if it's a weapon, not for mounts
            if ( pattached->transferblend && pchr->isitem )
            {

                // Items become partially invisible in hands of players
                if ( VALID_PLA( pattached->is_which_player ) && 255 != pattached->inst.alpha )
                {
                    chr_set_alpha( pchr, SEEINVISIBLE );
                }
                else
                {
                    // Only if not naturally transparent
                    if ( 255 == pchr->alpha_base )
                    {
                        chr_set_alpha( pchr, pattached->inst.alpha );
                    }
                    else
                    {
                        chr_set_alpha( pchr, pchr->alpha_base );
                    }
                }

                // Do light too
                if ( VALID_PLA( pattached->is_which_player ) && 255 != pattached->inst.light )
                {
                    chr_set_light( pchr, SEEINVISIBLE );
                }
                else
                {
                    // Only if not naturally transparent
                    if ( 255 == chr_get_pcap( cnt )->light )
                    {
                        chr_set_light( pchr, pattached->inst.light );
                    }
                    else
                    {
                        chr_set_light( pchr, pchr->light_base );
                    }
                }
            }
        }
        else
        {
            pchr->attachedto = ( CHR_REF )MAX_CHR;

            // Keep inventory with iattached
            if ( !pchr->pack.is_packed )
            {
                PACK_BEGIN_LOOP( ipacked, pchr->pack.next )
                {
                    chr_set_pos( ChrList.lst + ipacked, chr_get_pos_v( pchr ) );

                    // Copy olds to make SendMessageNear work
                    ChrList.lst[ipacked].pos_old = pchr->pos_old;
                }
                PACK_END_LOOP( ipacked );
            }
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix( const CHR_REF ichr )
{
    /// @details ZZ@> This function sets one character's matrix

    chr_t * pchr;
    chr_instance_t * pinst;

    if ( !INGAME_CHR( ichr ) ) return;
    pchr = ChrList.lst + ichr;
    pinst = &( pchr->inst );

    // invalidate this matrix
    pinst->matrix_cache.matrix_valid = bfalse;

    if ( pchr->is_overlay )
    {
        // This character is an overlay and its ai.target points to the object it is overlaying
        // Overlays are kept with their target...
        if ( INGAME_CHR( pchr->ai.target ) )
        {
            chr_t * ptarget = ChrList.lst + pchr->ai.target;

            chr_set_pos( pchr, chr_get_pos_v( ptarget ) );

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
            pinst->matrix = ScaleXYZRotateXYZTranslate_SpaceFixed( pchr->fat, pchr->fat, pchr->fat,
                            TO_TURN( pchr->ori.facing_z ),
                            TO_TURN( pchr->ori.map_facing_x - MAP_TURN_OFFSET ),
                            TO_TURN( pchr->ori.map_facing_y - MAP_TURN_OFFSET ),
                            pchr->pos.x, pchr->pos.y, pchr->pos.z );
        }
        else
        {
            pinst->matrix = ScaleXYZRotateXYZTranslate_BodyFixed( pchr->fat, pchr->fat, pchr->fat,
                            TO_TURN( pchr->ori.facing_z ),
                            TO_TURN( pchr->ori.map_facing_x - MAP_TURN_OFFSET ),
                            TO_TURN( pchr->ori.map_facing_y - MAP_TURN_OFFSET ),
                            pchr->pos.x, pchr->pos.y, pchr->pos.z );
        }

        pinst->matrix_cache.valid        = btrue;
        pinst->matrix_cache.matrix_valid = btrue;
        pinst->matrix_cache.type_bits    = MAT_CHARACTER;

        pinst->matrix_cache.self_scale.x = pchr->fat;
        pinst->matrix_cache.self_scale.y = pchr->fat;
        pinst->matrix_cache.self_scale.z = pchr->fat;

        pinst->matrix_cache.rotate.x = CLIP_TO_16BITS( pchr->ori.map_facing_x - MAP_TURN_OFFSET );
        pinst->matrix_cache.rotate.y = CLIP_TO_16BITS( pchr->ori.map_facing_y - MAP_TURN_OFFSET );
        pinst->matrix_cache.rotate.z = pchr->ori.facing_z;

        pinst->matrix_cache.pos = chr_get_pos( pchr );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void chr_log_script_time( const CHR_REF ichr )
{
    // log the amount of script time that this object used up

    chr_t * pchr;
    cap_t * pcap;
    FILE * ftmp;

    if ( !DEFINED_CHR( ichr ) ) return;
    pchr = ChrList.lst + ichr;

    if ( pchr->ai._clkcount <= 0 ) return;

    pcap = chr_get_pcap( ichr );
    if ( NULL == pcap ) return;

    ftmp = fopen( vfs_resolveWriteFilename( "/debug/script_timing.txt" ), "a+" );
    if ( NULL != ftmp )
    {
        fprintf( ftmp, "update == %d\tindex == %d\tname == \"%s\"\tclassname == \"%s\"\ttotal_time == %e\ttotal_calls == %f\n",
                 update_wld, REF_TO_INT( ichr ), pchr->Name, pcap->classname,
                 pchr->ai._clktime, pchr->ai._clkcount );
        fflush( ftmp );
        fclose( ftmp );
    }
}

//--------------------------------------------------------------------------------------------
void free_one_character_in_game( const CHR_REF character )
{
    /// @details ZZ@> This function sticks a character back on the free character stack
    ///
    /// @note This should only be called by cleanup_all_characters() or free_inventory_in_game()

    int     cnt;
    cap_t * pcap;
    chr_t * pchr;

    if ( !DEFINED_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return;

    // Remove from stat list
    if ( pchr->StatusList_on )
    {
        bool_t stat_found;

        pchr->StatusList_on = bfalse;

        stat_found = bfalse;
        for ( cnt = 0; cnt < StatusList_count; cnt++ )
        {
            if ( StatusList[cnt] == character )
            {
                stat_found = btrue;
                break;
            }
        }

        if ( stat_found )
        {
            for ( cnt++; cnt < StatusList_count; cnt++ )
            {
                SWAP( CHR_REF, StatusList[cnt-1], StatusList[cnt] );
            }
            StatusList_count--;
        }
    }

    // Make sure everyone knows it died
    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        ai_state_t * pai;

        if ( !INGAME_CHR( cnt ) || cnt == character ) continue;
        pai = chr_get_pai( cnt );

        if ( pai->target == character )
        {
            SET_BIT( pai->alert, ALERTIF_TARGETKILLED );
            pai->target = cnt;
        }

        if ( chr_get_pteam( cnt )->leader == character )
        {
            SET_BIT( pai->alert, ALERTIF_LEADERKILLED );
        }
    }
    CHR_END_LOOP();

    // Handle the team
    if ( pchr->alive && !pcap->invictus && TeamStack.lst[pchr->baseteam].morale > 0 )
    {
        TeamStack.lst[pchr->baseteam].morale--;
    }

    if ( TeamStack.lst[pchr->team].leader == character )
    {
        TeamStack.lst[pchr->team].leader = NOLEADER;
    }

    // remove any attached particles
    disaffirm_attached_particles( character );

    // actually get rid of the character
    ChrList_free_one( character );
}

//--------------------------------------------------------------------------------------------
void free_inventory_in_game( const CHR_REF character )
{
    /// @details ZZ@> This function frees every item in the character's inventory
    ///
    /// @note this should only be called by cleanup_all_characters()

    if ( !DEFINED_CHR( character ) ) return;

    PACK_BEGIN_LOOP( ipacked, ChrList.lst[character].pack.next )
    {
        free_one_character_in_game( ipacked );
    }
    PACK_END_LOOP( ipacked );

    // set the inventory to the "empty" state
    ChrList.lst[character].pack.count = 0;
    ChrList.lst[character].pack.next  = ( CHR_REF )MAX_CHR;
}

//--------------------------------------------------------------------------------------------
prt_t * place_particle_at_vertex( prt_t * pprt, const CHR_REF character, int vertex_offset )
{
    /// @details ZZ@> This function sets one particle's position to be attached to a character.
    ///    It will kill the particle if the character is no longer around

    int     vertex;
    fvec4_t point[1], nupoint[1];

    chr_t * pchr;

    if ( !DEFINED_PPRT( pprt ) ) return pprt;

    if ( !INGAME_CHR( character ) )
    {
        goto place_particle_at_vertex_fail;
    }
    pchr = ChrList.lst + character;

    // Check validity of attachment
    if ( pchr->pack.is_packed )
    {
        goto place_particle_at_vertex_fail;
    }

    // Do we have a matrix???
    if ( !chr_matrix_valid( pchr ) )
    {
        chr_update_matrix( pchr, btrue );
    }

    // Do we have a matrix???
    if ( chr_matrix_valid( pchr ) )
    {
        // Transform the weapon vertex_offset from model to world space
        mad_t * pmad = chr_get_pmad( character );

        if ( vertex_offset == GRIP_ORIGIN )
        {
            fvec3_t tmp_pos = VECT3( pchr->inst.matrix.CNV( 3, 0 ), pchr->inst.matrix.CNV( 3, 1 ), pchr->inst.matrix.CNV( 3, 2 ) );
            prt_set_pos( pprt, tmp_pos.v );

            return pprt;
        }

        vertex = 0;
        if ( NULL != pmad )
        {
            vertex = (( int )pchr->inst.vrt_count ) - vertex_offset;

            // do the automatic update
            chr_instance_update_vertices( &( pchr->inst ), vertex, vertex, bfalse );

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
        TransformVertices( &( pchr->inst.matrix ), point, nupoint, 1 );

        prt_set_pos( pprt, nupoint[0].v );
    }
    else
    {
        // No matrix, so just wing it...
        prt_set_pos( pprt, chr_get_pos_v( pchr ) );
    }

    return pprt;

place_particle_at_vertex_fail:

    prt_request_terminate( GET_REF_PPRT( pprt ) );

    return NULL;
}

//--------------------------------------------------------------------------------------------
void update_all_character_matrices()
{
    /// @details ZZ@> This function makes all of the character's matrices

    // just call chr_update_matrix on every character
    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        chr_update_matrix( pchr, btrue );
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void free_all_chraracters()
{
    /// @details ZZ@> This function resets the character allocation list

    // free all the characters
    ChrList_free_all();

    // free_all_players
    PlaStack.count = 0;
    local_numlpla = 0;
    local_stats.noplayers = btrue;

    // free_all_stats
    StatusList_count = 0;
}

//--------------------------------------------------------------------------------------------
float chr_get_mesh_pressure( chr_t * pchr, float test_pos[] )
{
    float retval = 0.0f;
    float radius = 0.0f;

    if ( !DEFINED_PCHR( pchr ) ) return retval;

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) return retval;

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // calculate the radius based on whether the character is on camera
    // ZF> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( cfg.dev_mode && !SDLKEYDOWN( SDLK_F8 ) )
    {
        if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
        {
            if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
            {
                radius = pchr->bump_1.size;
            }
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = mesh_get_pressure( PMesh, test_pos, radius, pchr->stoppedby );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
fvec2_t chr_get_mesh_diff( chr_t * pchr, float test_pos[], float center_pressure )
{
    fvec2_t retval = ZERO_VECT2;
    float   radius;

    if ( !DEFINED_PCHR( pchr ) ) return retval;

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) return retval;

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // calculate the radius based on whether the character is on camera
    // ZF> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( cfg.dev_mode && !SDLKEYDOWN( SDLK_F8 ) )
    {
        if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
        {
            if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
            {
                radius = pchr->bump_1.size;
            }
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = mesh_get_diff( PMesh, test_pos, radius, center_pressure, pchr->stoppedby );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD chr_hit_wall( chr_t * pchr, const float test_pos[], float nrm[], float * pressure, mesh_wall_data_t * pdata )
{
    /// @details ZZ@> This function returns nonzero if the character hit a wall that the
    ///    character is not allowed to cross

    BIT_FIELD    retval;
    float        radius;

    if ( !DEFINED_PCHR( pchr ) ) return 0;

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) return 0;

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // calculate the radius based on whether the character is on camera
    // ZF> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( cfg.dev_mode && !SDLKEYDOWN( SDLK_F8 ) )
    {
        if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
        {
            if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
            {
                radius = pchr->bump_1.size;
            }
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = mesh_hit_wall( PMesh, test_pos, radius, pchr->stoppedby, nrm, pressure, pdata );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests  += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD chr_test_wall( chr_t * pchr, const float test_pos[], mesh_wall_data_t * pdata )
{
    /// @details ZZ@> This function returns nonzero if the character hit a wall that the
    ///    character is not allowed to cross

    BIT_FIELD retval;
    float  radius;

    if ( !ACTIVE_PCHR( pchr ) ) return EMPTY_BIT_FIELD;

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight ) return EMPTY_BIT_FIELD;

    // calculate the radius based on whether the character is on camera
    // ZF> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( cfg.dev_mode && !SDLKEYDOWN( SDLK_F8 ) )
    {
        if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
        {
            if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
            {
                radius = pchr->bump_1.size;
            }
        }
    }

    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // do the wall test
    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = mesh_test_wall( PMesh, test_pos, radius, pchr->stoppedby, pdata );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_is_over_water( chr_t *pchr )
{
    /// @details ZF@> This function returns true if the character is over a water tile

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    if ( !water.is_water || !mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) ) return bfalse;

    return 0 != mesh_test_fx( PMesh, pchr->onwhichgrid, MPDFX_WATER );
}

//--------------------------------------------------------------------------------------------
void reset_character_accel( const CHR_REF character )
{
    /// @details ZZ@> This function fixes a character's max acceleration

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;
    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Okay, remove all acceleration enchants
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        enchant_remove_add( ienc_now, ADDACCEL );

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    // Set the starting value
    pchr->maxaccel_reset = 0;
    pcap = chr_get_pcap( character );
    if ( NULL != pcap )
    {
        pchr->maxaccel = pchr->maxaccel_reset = pcap->maxaccel[pchr->skin];
    }

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Put the acceleration enchants back on
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        enchant_apply_add( ienc_now, ADDACCEL, enc_get_ieve( ienc_now ) );

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
}

//--------------------------------------------------------------------------------------------
bool_t detach_character_from_mount( const CHR_REF character, Uint8 ignorekurse, Uint8 doshop )
{
    /// @details ZZ@> This function drops an item

    CHR_REF mount;
    Uint16  hand;
    bool_t  inshop;
    chr_t * pchr, * pmount;

    // Make sure the character is valid
    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    // Make sure the character is mounted
    mount = ChrList.lst[character].attachedto;
    if ( !INGAME_CHR( mount ) ) return bfalse;
    pmount = ChrList.lst + mount;

    // Don't allow living characters to drop kursed weapons
    if ( !ignorekurse && pchr->iskursed && pmount->alive && pchr->isitem )
    {
        SET_BIT( pchr->ai.alert, ALERTIF_NOTDROPPED );
        return bfalse;
    }

    // set the dismount timer
    if ( !pchr->isitem ) pchr->dismount_timer  = PHYS_DISMOUNT_TIME;
    pchr->dismount_object = mount;

    // Figure out which hand it's in
    hand = pchr->inwhich_slot;

    // Rip 'em apart
    pchr->attachedto = ( CHR_REF )MAX_CHR;
    if ( pmount->holdingwhich[SLOT_LEFT] == character )
        pmount->holdingwhich[SLOT_LEFT] = ( CHR_REF )MAX_CHR;

    if ( pmount->holdingwhich[SLOT_RIGHT] == character )
        pmount->holdingwhich[SLOT_RIGHT] = ( CHR_REF )MAX_CHR;

    if ( pchr->alive )
    {
        // play the falling animation...
        chr_play_action( pchr, ACTION_JB + hand, bfalse );
    }
    else if ( pchr->inst.action_which < ACTION_KA || pchr->inst.action_which > ACTION_KD )
    {
        // play the "killed" animation...
        chr_play_action( pchr, ACTION_KA + generate_randmask( 0, 3 ), bfalse );
        chr_instance_set_action_keep( &( pchr->inst ), btrue );
    }

    // Set the positions
    if ( chr_matrix_valid( pchr ) )
    {
        chr_set_pos( pchr, mat_getTranslate_v( pchr->inst.matrix.v ) );
    }
    else
    {
        chr_set_pos( pchr, chr_get_pos_v( pmount ) );
    }

    // Make sure it's not dropped in a wall...
    if ( EMPTY_BIT_FIELD != chr_test_wall( pchr, NULL, NULL ) )
    {
        fvec3_t pos_tmp;

        pos_tmp.x = pmount->pos.x;
        pos_tmp.y = pmount->pos.y;
        pos_tmp.z = pchr->pos.z;

        chr_set_pos( pchr, pos_tmp.v );

        chr_update_breadcrumb( pchr, btrue );
    }

    // Check for shop passages
    inshop = bfalse;
    if ( doshop )
    {
        inshop = do_shop_drop( mount, character );
    }

    // Make sure it works right
    pchr->hitready = btrue;
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
    chr_instance_set_action_loop( &( pchr->inst ), bfalse );

    // Reset the team if it is a mount
    if ( pmount->ismount )
    {
        pmount->team = pmount->baseteam;
        SET_BIT( pmount->ai.alert, ALERTIF_DROPPED );
    }

    pchr->team = pchr->baseteam;
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
        while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
        {
            ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

            enchant_remove_set( ienc_now, SETALPHABLEND );
            enchant_remove_set( ienc_now, SETLIGHTBLEND );

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

        chr_set_alpha( pchr, pchr->alpha_base );
        chr_set_light( pchr, pchr->light_base );

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // apply the blend enchants
        ienc_now = pchr->firstenchant;
        ienc_count = 0;
        while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
        {
            PRO_REF ipro = enc_get_ipro( ienc_now );
            ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

            if ( LOADED_PRO( ipro ) )
            {
                enchant_apply_set( ienc_now, SETALPHABLEND, ipro );
                enchant_apply_set( ienc_now, SETLIGHTBLEND, ipro );
            }

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
    }

    // Set twist
    pchr->ori.map_facing_y = MAP_TURN_OFFSET;
    pchr->ori.map_facing_x = MAP_TURN_OFFSET;

    // turn off keeping, unless the object is dead
    if ( pchr->life <= 0 )
    {
        // the object is dead. play the killed animation and make it freeze there
        chr_play_action( pchr, ACTION_KA + generate_randmask( 0, 3 ), bfalse );
        chr_instance_set_action_keep( &( pchr->inst ), btrue );
    }
    else
    {
        // play the jump animation, and un-keep it
        chr_play_action( pchr, ACTION_JA, btrue );
        chr_instance_set_action_keep( &( pchr->inst ), bfalse );
    }

    chr_update_matrix( pchr, btrue );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha( const CHR_REF character )
{
    /// @details ZZ@> This function fixes an item's transparency

    CHR_REF mount;
    chr_t * pchr, * pmount;

    // Make sure the character is valid
    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    // Make sure the character is mounted
    mount = ChrList.lst[character].attachedto;
    if ( !INGAME_CHR( mount ) ) return;
    pmount = ChrList.lst + mount;

    if ( pchr->isitem && pmount->transferblend )
    {
        ENC_REF ienc_now, ienc_nxt;
        size_t  ienc_count;

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // Okay, reset transparency
        ienc_now = pchr->firstenchant;
        ienc_count = 0;
        while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
        {
            ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

            enchant_remove_set( ienc_now, SETALPHABLEND );
            enchant_remove_set( ienc_now, SETLIGHTBLEND );

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

        chr_set_alpha( pchr, pchr->alpha_base );
        chr_set_light( pchr, pchr->light_base );

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        ienc_now = pchr->firstenchant;
        ienc_count = 0;
        while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
        {
            PRO_REF ipro = enc_get_ipro( ienc_now );

            ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

            if ( LOADED_PRO( ipro ) )
            {
                enchant_apply_set( ienc_now, SETALPHABLEND, ipro );
                enchant_apply_set( ienc_now, SETLIGHTBLEND, ipro );
            }

            ienc_now = ienc_nxt;
            ienc_count++;
        }
        if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );
    }
}

//--------------------------------------------------------------------------------------------
egoboo_rv attach_character_to_mount( const CHR_REF irider, const CHR_REF imount, grip_offset_t grip_off )
{
    /// @details ZZ@> This function attaches one character/item to another ( the holder/mount )
    ///    at a certain vertex offset ( grip_off )
    ///   - This function is called as a part of spawning a module, so the item or the holder may not
    ///     be fully instantiated
    ///   - This function should do very little testing to see if attachment is allowed.
    ///     Most of that checking should be done by the calling function

    slot_t slot;

    chr_t * prider, * pmount;
    cap_t *pmount_cap;

    // Make sure the character/item is valid
    if ( !DEFINED_CHR( irider ) ) return rv_error;
    prider = ChrList.lst + irider;

    // Make sure the holder/mount is valid
    if ( !DEFINED_CHR( imount ) ) return rv_error;
    pmount = ChrList.lst + imount;

    pmount_cap = chr_get_pcap( imount );
    if ( NULL == pmount_cap ) return rv_error;

    // do not deal with packed items at this time
    // this would have to be changed to allow for pickpocketing
    if ( prider->pack.is_packed || pmount->pack.is_packed ) return rv_fail;

    // make a reasonable time for the character to remount something
    // for characters jumping out of pots, etc
    if ( imount == prider->dismount_object && prider->dismount_timer > 0 ) return rv_fail;

    // Figure out which slot this grip_off relates to
    slot = grip_offset_to_slot( grip_off );

    // Make sure the the slot is valid
    if ( !pmount_cap->slotvalid[slot] ) return rv_fail;

    // This is a small fix that allows special grabbable mounts not to be mountable while
    // held by another character (such as the magic carpet for example)
    // ( this is an example of a test that should not be done here )
    if ( pmount->ismount && INGAME_CHR( pmount->attachedto ) ) return rv_fail;

    // Put 'em together
    prider->inwhich_slot       = slot;
    prider->attachedto         = imount;
    pmount->holdingwhich[slot] = irider;

    // set the grip vertices for the irider
    set_weapongrip( irider, imount, grip_off );

    chr_update_matrix( prider, btrue );

    chr_set_pos( prider, mat_getTranslate_v( prider->inst.matrix.v ) );

    prider->enviro.inwater  = bfalse;
    prider->jump_timer = JUMPDELAY * 4;

    // Run the held animation
    if ( pmount->ismount && ( GRIP_ONLY == grip_off ) )
    {
        // Riding imount

        if ( INGAME_CHR( prider->holdingwhich[SLOT_LEFT] ) || INGAME_CHR( prider->holdingwhich[SLOT_RIGHT] ) )
        {
            // if the character is holding anything, make the animation
            // ACTION_MH == "sitting" so that it dies not look so silly
            chr_play_action( prider, ACTION_MH, btrue );
        }
        else
        {
            // if it is not holding anything, go for the riding animation
            chr_play_action( prider, ACTION_MI, btrue );
        }

        // set tehis action to loop
        chr_instance_set_action_loop( &( prider->inst ), btrue );
    }
    else if ( prider->alive )
    {
        /// @note ZF@> hmm, here is the torch holding bug. Removing
        /// the interpolation seems to fix it...
        chr_play_action( prider, ACTION_MM + slot, bfalse );

        chr_instance_remove_interpolation( &( prider->inst ) );

        // set the action to keep for items
        if ( prider->isitem )
        {
            // Item grab
            chr_instance_set_action_keep( &( prider->inst ), btrue );
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

    if ( pmount->ismount )
    {
        pmount->team = prider->team;

        // Set the alert
        if ( !pmount->isitem && pmount->alive )
        {
            SET_BIT( pmount->ai.alert, ALERTIF_GRABBED );
        }
    }

    // It's not gonna hit the floor
    prider->hitready = bfalse;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool_t inventory_add_item( const CHR_REF item, const CHR_REF character )
{
    chr_t * pchr, * pitem;
    cap_t * pitem_cap;
    bool_t  slot_found, pack_added;
    int     slot_count;
    int     cnt;

    if ( !INGAME_CHR( item ) ) return bfalse;
    pitem = ChrList.lst + item;

    // don't allow sub-inventories
    if ( pitem->pack.is_packed || pitem->isequipped ) return bfalse;

    pitem_cap = pro_get_pcap( pitem->profile_ref );
    if ( NULL == pitem_cap ) return bfalse;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    // don't allow sub-inventories
    if ( pchr->pack.is_packed || pchr->isequipped ) return bfalse;

    slot_found = bfalse;
    slot_count = 0;
    for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
    {
        if ( IDSZ_NONE == inventory_idsz[cnt] ) continue;

        if ( inventory_idsz[cnt] == pitem_cap->idsz[IDSZ_PARENT] )
        {
            slot_count = cnt;
            slot_found = btrue;
        }
    }

    if ( slot_found )
    {
        if ( INGAME_CHR( pchr->holdingwhich[slot_count] ) )
        {
            pchr->inventory[slot_count] = ( CHR_REF )MAX_CHR;
        }
    }

    pack_added = chr_add_pack_item( item, character );

    if ( slot_found && pack_added )
    {
        pchr->inventory[slot_count] = item;
    }

    return pack_added;
}

//--------------------------------------------------------------------------------------------
CHR_REF inventory_get_item( const CHR_REF ichr, grip_offset_t grip_off, bool_t ignorekurse )
{
    chr_t * pchr;
    CHR_REF iitem;
    int     cnt;

    if ( !INGAME_CHR( ichr ) ) return ( CHR_REF )MAX_CHR;
    pchr = ChrList.lst + ichr;

    if ( pchr->pack.is_packed || pchr->isitem || MAX_CHR == pchr->pack.next )
        return ( CHR_REF )MAX_CHR;

    if ( 0 == pchr->pack.count ) return ( CHR_REF )MAX_CHR;

    iitem = chr_get_pack_item( ichr, grip_off, ignorekurse );

    // remove it from the "equipped" slots
    if ( INGAME_CHR( iitem ) )
    {
        for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
        {
            if ( pchr->inventory[cnt] == iitem )
            {
                pchr->inventory[cnt] = iitem;
                ChrList.lst[iitem].isequipped = bfalse;
                break;
            }
        }
    }

    return iitem;
}

//--------------------------------------------------------------------------------------------
bool_t pack_add_item( pack_t * ppack, CHR_REF item )
{
    CHR_REF oldfirstitem;
    chr_t  * pitem;
    cap_t  * pitem_cap;
    pack_t * pitem_pack;

    if ( NULL == ppack || !INGAME_CHR( item ) ) return bfalse;

    if ( !INGAME_CHR( item ) ) return bfalse;
    pitem      = ChrList.lst + item;
    pitem_pack = &( pitem->pack );
    pitem_cap  = chr_get_pcap( item );

    oldfirstitem     = ppack->next;
    ppack->next      = item;
    pitem_pack->next = oldfirstitem;
    ppack->count++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t pack_remove_item( pack_t * ppack, CHR_REF iparent, CHR_REF iitem )
{
    CHR_REF old_next;
    chr_t * pitem, * pparent;

    // convert the iitem it to a pointer
    old_next = ( CHR_REF )MAX_CHR;
    pitem    = NULL;
    if ( DEFINED_CHR( iitem ) )
    {
        pitem    = ChrList.lst + iitem;
        old_next = pitem->pack.next;
    }

    // convert the pparent it to a pointer
    pparent = NULL;
    if ( DEFINED_CHR( iparent ) )
    {
        pparent = ChrList.lst + iparent;
    }

    // Remove the iitem from the pack
    if ( NULL != pitem )
    {
        pitem->pack.was_packed = pitem->pack.is_packed;
        pitem->pack.is_packed  = bfalse;
    }

    // adjust the iparent's next
    if ( NULL != pparent )
    {
        pparent->pack.next = old_next;
    }

    if ( NULL != ppack )
    {
        ppack->count--;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_pack_has_a_stack( const CHR_REF item, const CHR_REF character )
{
    /// @details ZZ@> This function looks in the character's pack for an item similar
    ///    to the one given.  If it finds one, it returns the similar item's
    ///    index number, otherwise it returns MAX_CHR.

    CHR_REF istack;
    Uint16  id;
    bool_t  found;

    chr_t * pitem;
    cap_t * pitem_cap;

    found  = bfalse;
    istack = ( CHR_REF )MAX_CHR;

    if ( !INGAME_CHR( item ) ) return istack;
    pitem = ChrList.lst + item;
    pitem_cap = chr_get_pcap( item );

    if ( pitem_cap->isstackable )
    {
        PACK_BEGIN_LOOP( ipacked, ChrList.lst[character].pack.next )
        {
            if ( INGAME_CHR( ipacked ) )
            {
                chr_t * pstack     = ChrList.lst + ipacked;
                cap_t * pstack_cap = chr_get_pcap( ipacked );

                found = pstack_cap->isstackable;

                if ( pstack->ammo >= pstack->ammomax )
                {
                    found = bfalse;
                }

                // you can still stack something even if the profiles don't match exactly,
                // but they have to have all the same IDSZ properties
                if ( found && ( pstack->profile_ref != pitem->profile_ref ) )
                {
                    for ( id = 0; id < IDSZ_COUNT && found; id++ )
                    {
                        if ( chr_get_idsz( ipacked, id ) != chr_get_idsz( item, id ) )
                        {
                            found = bfalse;
                        }
                    }
                }
            }

            if ( found )
            {
                istack = ipacked;
                break;
            }
        }
        PACK_END_LOOP( ipacked );
    }

    return istack;
}

//--------------------------------------------------------------------------------------------
bool_t chr_add_pack_item( const CHR_REF item, const CHR_REF character )
{
    /// @details ZZ@> This function puts one character inside the other's pack

    CHR_REF stack;
    int     newammo;

    chr_t  * pchr, * pitem;
    cap_t  * pchr_cap,  * pitem_cap;
    pack_t * pchr_pack, * pitem_pack;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr      = ChrList.lst + character;
    pchr_pack = &( pchr->pack );
    pchr_cap  = chr_get_pcap( character );

    if ( !INGAME_CHR( item ) ) return bfalse;
    pitem      = ChrList.lst + item;
    pitem_pack = &( pitem->pack );
    pitem_cap  = chr_get_pcap( item );

    // Make sure everything is hunkydori
    if ( pitem_pack->is_packed || pchr_pack->is_packed || pchr->isitem )
        return bfalse;

    stack = chr_pack_has_a_stack( item, character );
    if ( INGAME_CHR( stack ) )
    {
        // We found a similar, stackable item in the pack

        chr_t  * pstack      = ChrList.lst + stack;
        cap_t  * pstack_cap  = chr_get_pcap( stack );

        // reveal the name of the item or the stack
        if ( pitem->nameknown || pstack->nameknown )
        {
            pitem->nameknown  = btrue;
            pstack->nameknown = btrue;
        }

        // reveal the usage of the item or the stack
        if ( pitem_cap->usageknown || pstack_cap->usageknown )
        {
            pitem_cap->usageknown  = btrue;
            pstack_cap->usageknown = btrue;
        }

        // add the item ammo to the stack
        newammo = pitem->ammo + pstack->ammo;
        if ( newammo <= pstack->ammomax )
        {
            // All transfered, so kill the in hand item
            pstack->ammo = newammo;
            if ( INGAME_CHR( pitem->attachedto ) )
            {
                detach_character_from_mount( item, btrue, bfalse );
            }

            chr_request_terminate( item );
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
        // Make sure we have room for another item
        if ( pchr_pack->count >= MAXNUMINPACK )
        {
            SET_BIT( pchr->ai.alert, ALERTIF_TOOMUCHBAGGAGE );
            return bfalse;
        }

        // Take the item out of hand
        if ( INGAME_CHR( pitem->attachedto ) )
        {
            detach_character_from_mount( item, btrue, bfalse );

            // clear the dropped flag
            UNSET_BIT( pitem->ai.alert, ALERTIF_DROPPED );
        }

        // Remove the item from play
        pitem->hitready        = bfalse;
        pitem_pack->was_packed = pitem_pack->is_packed;
        pitem_pack->is_packed  = btrue;

        // Insert the item into the pack as the first one
        pack_add_item( pchr_pack, item );

        // fix the flags
        if ( pitem_cap->isequipment )
        {
            SET_BIT( pitem->ai.alert, ALERTIF_PUTAWAY );  // same as ALERTIF_ATLASTWAYPOINT;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_remove_pack_item( CHR_REF ichr, CHR_REF iparent, CHR_REF iitem )
{
    chr_t  * pchr;
    pack_t * pchr_pack;

    bool_t removed;

    if ( !DEFINED_CHR( ichr ) ) return bfalse;
    pchr = ChrList.lst + ichr;
    pchr_pack = &( pchr->pack );

    // remove it from the pack
    removed = pack_remove_item( pchr_pack, iparent, iitem );

    // unequip the item
    if ( removed && DEFINED_CHR( iitem ) )
    {
        ChrList.lst[iitem].isequipped = bfalse;
        ChrList.lst[iitem].team       = chr_get_iteam( ichr );
    }

    return removed;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_pack_item( const CHR_REF character, grip_offset_t grip_off, bool_t ignorekurse )
{
    /// @details ZZ@> This function takes the last item in the character's pack and puts
    ///    it into the designated hand.  It returns the item number or MAX_CHR.

    CHR_REF found_item, found_item_parent;

    chr_t  * pchr, * pfound_item, *pfound_item_parent;
    pack_t * pchr_pack, * pfound_item_pack, *pfound_item_parent_pack;

    // does the character exist?
    if ( !DEFINED_CHR( character ) ) return bfalse;
    pchr      = ChrList.lst + character;
    pchr_pack = &( pchr->pack );

    // Can the character have a pack?
    if ( pchr_pack->is_packed || pchr->isitem ) return ( CHR_REF )MAX_CHR;

    // is the pack empty?
    if ( MAX_CHR == pchr_pack->next || 0 == pchr_pack->count ) return ( CHR_REF )MAX_CHR;

    // Find the last item in the pack
    found_item_parent = character;
    found_item        = character;
    PACK_BEGIN_LOOP( ipacked, pchr_pack->next )
    {
        found_item_parent = found_item;
        found_item        = ipacked;
    }
    PACK_END_LOOP( ipacked );

    // did we find anything?
    if ( character == found_item || MAX_CHR == found_item ) return bfalse;

    // convert the found_item it to a pointer
    pfound_item      = NULL;
    pfound_item_pack = NULL;
    if ( DEFINED_CHR( found_item ) )
    {
        pfound_item = ChrList.lst + found_item;
        pfound_item_pack = &( pfound_item->pack );
    }

    // convert the pfound_item_parent it to a pointer
    pfound_item_parent      = NULL;
    pfound_item_parent_pack = NULL;
    if ( DEFINED_CHR( found_item_parent ) )
    {
        pfound_item_parent      = ChrList.lst + found_item_parent;
        pfound_item_parent_pack = &( pfound_item_parent->pack );
    }

    // did we find a valid object?
    if ( !INGAME_CHR( found_item ) )
    {
        chr_remove_pack_item( character, found_item_parent, found_item );

        return bfalse;
    }

    // Figure out what to do with it
    if ( pfound_item->iskursed && pfound_item->isequipped && !ignorekurse )
    {
        // Flag the last found_item as not removed
        SET_BIT( pfound_item->ai.alert, ALERTIF_NOTTAKENOUT );  // Same as ALERTIF_NOTPUTAWAY

        // Cycle it to the front
        pfound_item_pack->next        = pchr_pack->next;
        pfound_item_parent_pack->next = ( CHR_REF )MAX_CHR;
        pchr_pack->next               = found_item;

        if ( character == found_item_parent )
        {
            pfound_item_pack->next = ( CHR_REF )MAX_CHR;
        }

        found_item = ( CHR_REF )MAX_CHR;
    }
    else
    {
        // Remove the last found_item from the pack
        chr_remove_pack_item( character, found_item_parent, found_item );

        // Attach the found_item to the character's hand
        // (1) if the mounting doesn't succees, I guess it will drop it at the character's feet,
        // or will it drop the item at 0,0... I think the keep_weapons_with_holders() function
        // works
        // (2) if it fails, do we need to treat it as if it was dropped? Or just not take it out?
        attach_character_to_mount( found_item, character, grip_off );

        // fix the flags
        UNSET_BIT( pfound_item->ai.alert, ALERTIF_GRABBED );
        SET_BIT( pfound_item->ai.alert, ALERTIF_TAKENOUT );
    }

    if ( MAX_CHR == pchr_pack->next )
    {
        pchr_pack->count = 0;
    }

    return found_item;
}

//--------------------------------------------------------------------------------------------
void drop_keys( const CHR_REF character )
{
    /// @details ZZ@> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a character's
    ///    inventory ( Not hands ).

    chr_t  * pchr;

    FACING_T direction;
    IDSZ     testa, testz;

    CHR_REF   key_lst[MAXNUMINPACK];
    CHR_REF * key_parent[MAXNUMINPACK];
    size_t    key_count;

    size_t    cnt;
    CHR_REF * pparent;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    // Don't lose keys in pits...
    if ( pchr->pos.z <= ( PITDEPTH >> 1 ) ) return;

    // The IDSZs to find
    testa = MAKE_IDSZ( 'K', 'E', 'Y', 'A' );  // [KEYA]
    testz = MAKE_IDSZ( 'K', 'E', 'Y', 'Z' );  // [KEYZ]

    key_count = 0;
    pparent = &( pchr->pack.next );
    PACK_BEGIN_LOOP( ipacked, pchr->pack.next )
    {
        if ( INGAME_CHR( ipacked ) && ipacked != character )  // Should never happen...
        {
            IDSZ idsz_parent;
            IDSZ idsz_type;

            chr_t * pitem = ChrList.lst + ipacked;

            idsz_parent = chr_get_idsz( ipacked, IDSZ_PARENT );
            idsz_type   = chr_get_idsz( ipacked, IDSZ_TYPE );

            if (( idsz_parent >= testa && idsz_parent <= testz ) ||
                ( idsz_type >= testa && idsz_type <= testz ) )
            {
                key_lst[key_count]    = ipacked;
                key_parent[key_count] = pparent;
                key_count++;
            }
            else
            {
                // only save non-keys as parents
                pparent = &( pitem->pack.next );
            }
        }
    }
    PACK_END_LOOP( ipacked );

    // We found some keys?
    // since we are MODIFYING the pack list, do not change the list
    // while inside the PACK_BEGIN_LOOP() ... PACK_END_LOOP()
    for ( cnt = 0; cnt < key_count; cnt++ )
    {
        CHR_REF ikey     = key_lst[cnt];
        CHR_REF *pparent = key_parent[cnt];

        chr_t * pkey = ChrList.lst + ikey;

        TURN_T turn;

        direction = RANDIE;
        turn      = TO_TURN( direction );

        // unpack the ikey
        *pparent = pkey->pack.next;
        pkey->pack.next = ( CHR_REF )MAX_CHR;
        pchr->pack.count--;

        // fix the attachments
        pkey->attachedto             = ( CHR_REF )MAX_CHR;
        pkey->dismount_timer         = PHYS_DISMOUNT_TIME;
        pkey->dismount_object        = GET_REF_PCHR( pchr );
        pkey->onwhichplatform_ref    = pchr->onwhichplatform_ref;
        pkey->onwhichplatform_update = pchr->onwhichplatform_update;

        // fix some flags
        pkey->hitready               = btrue;
        pkey->pack.was_packed        = pkey->pack.is_packed;
        pkey->pack.is_packed         = bfalse;
        pkey->isequipped             = bfalse;
        pkey->ori.facing_z           = direction + ATK_BEHIND;
        pkey->team                   = pkey->baseteam;

        // fix the current velocity
        pkey->vel.x                  += turntocos[ turn ] * DROPXYVEL;
        pkey->vel.y                  += turntosin[ turn ] * DROPXYVEL;
        pkey->vel.z                  += DROPZVEL;

        // do some more complicated things
        SET_BIT( pkey->ai.alert, ALERTIF_DROPPED );
        chr_set_pos( pkey, chr_get_pos_v( pchr ) );
        move_one_character_get_environment( pkey );
        chr_set_floor_level( pkey, pchr->enviro.floor_level );
    }
}

//--------------------------------------------------------------------------------------------
bool_t drop_all_items( const CHR_REF character )
{
    /// @details ZZ@> This function drops all of a character's items

    CHR_REF  item;
    FACING_T direction;
    Sint16   diradd;
    chr_t  * pchr;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], btrue, bfalse );
    detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], btrue, bfalse );
    if ( pchr->pack.count > 0 )
    {
        direction = pchr->ori.facing_z + ATK_BEHIND;
        diradd    = 0x00010000 / pchr->pack.count;

        while ( pchr->pack.count > 0 )
        {
            item = inventory_get_item( character, GRIP_LEFT, bfalse );

            if ( INGAME_CHR( item ) )
            {
                chr_t * pitem = ChrList.lst + item;

                // detach the item
                detach_character_from_mount( item, btrue, btrue );

                // fix the attachments
                pitem->dismount_timer         = PHYS_DISMOUNT_TIME;
                pitem->dismount_object        = GET_REF_PCHR( pchr );
                pitem->onwhichplatform_ref    = pchr->onwhichplatform_ref;
                pitem->onwhichplatform_update = pchr->onwhichplatform_update;

                // fix some flags
                pitem->hitready               = btrue;
                pitem->ori.facing_z           = direction + ATK_BEHIND;
                pitem->team                   = pitem->baseteam;

                // fix the current velocity
                pitem->vel.x                  += turntocos[( direction>>2 ) & TRIG_TABLE_MASK ] * DROPXYVEL;
                pitem->vel.y                  += turntosin[( direction>>2 ) & TRIG_TABLE_MASK ] * DROPXYVEL;
                pitem->vel.z                  += DROPZVEL;

                // do some more complicated things
                SET_BIT( pitem->ai.alert, ALERTIF_DROPPED );
                chr_set_pos( pitem, chr_get_pos_v( pchr ) );
                move_one_character_get_environment( pitem );
                chr_set_floor_level( pitem, pchr->enviro.floor_level );
            }

            direction += diradd;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_grab_data
{
    CHR_REF ichr;
    fvec3_t diff;
    float   diff2_hrz;
    float   diff2_vrt;
    bool_t  too_dark, too_invis;
};
typedef struct s_grab_data grab_data_t;

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
bool_t character_grab_stuff( const CHR_REF ichr_a, grip_offset_t grip_off, bool_t grab_people )
{
    /// @details ZZ@> This function makes the character pick up an item if there's one around

    const SDL_Color color_red = {0xFF, 0x7F, 0x7F, 0xFF};
    const SDL_Color color_grn = {0x7F, 0xFF, 0x7F, 0xFF};
    const SDL_Color color_blu = {0x7F, 0x7F, 0xFF, 0xFF};
    const GLXvector4f default_tint = { 1.00f, 1.00f, 1.00f, 1.00f };

    // 1 a grid is fine. anything more than that and it gets crazy
    const float const_info2_hrz = SQR( 3.0f * GRID_FSIZE );
    const float const_grab2_hrz = SQR( 1.0f * GRID_FSIZE );
    const float const_grab2_vrt = SQR( GRABSIZE );

    int       cnt;
    CHR_REF   ichr_b;
    slot_t    slot;
    oct_vec_t mids;
    fvec3_t   slot_pos;
    float     bump_size2_a;

    chr_t * pchr_a;
    cap_t * pcap_a;

    bool_t retval;

    // valid objects that can be grabbed
    size_t      grab_count = 0;
    size_t      grab_visible_count = 0;
    grab_data_t grab_list[MAX_CHR];

    // valid objects that cannot be grabbed
    size_t      ungrab_count = 0;
    size_t      ungrab_visible_count = 0;
    grab_data_t ungrab_list[MAX_CHR];

    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    pcap_a = pro_get_pcap( pchr_a->profile_ref );
    if ( NULL == pcap_a ) return bfalse;

    // find the slot from the grip
    slot = grip_offset_to_slot( grip_off );
    if ( slot < 0 || slot >= SLOT_COUNT ) return bfalse;

    // Make sure the character doesn't have something already, and that it has hands
    if ( INGAME_CHR( pchr_a->holdingwhich[slot] ) || !pcap_a->slotvalid[slot] )
        return bfalse;

    //Determine the position of the grip
    oct_bb_get_mids( pchr_a->slot_cv + slot, mids );
    slot_pos.x = mids[OCT_X];
    slot_pos.y = mids[OCT_Y];
    slot_pos.z = mids[OCT_Z];
    fvec3_self_sum( slot_pos.v, chr_get_pos_v( pchr_a ) );

    // get the size of object a
    bump_size2_a = SQR( 1.5f * pchr_a->bump.size );

    // Go through all characters to find the best match
    CHR_BEGIN_LOOP_ACTIVE( ichr_b, pchr_b )
    {
        fvec3_t   diff;
        float     bump_size2_b;
        float     diff2_hrz, diff2_vrt;
        float     grab2_vrt, grab2_hrz, info2_hrz;
        bool_t    can_grab = btrue;
        bool_t    too_dark = btrue;
        bool_t    too_invis = btrue;

        // do nothing to yourself
        if ( ichr_a == ichr_b ) continue;

        // Dont do hidden objects
        if ( pchr_b->is_hidden ) continue;

        // pickpocket not allowed yet
        if ( pchr_b->pack.is_packed ) continue;

        // disarm not allowed yet
        if ( MAX_CHR != pchr_b->attachedto ) continue;

        // do not pick up your mount
        if ( pchr_b->holdingwhich[SLOT_LEFT] == ichr_a ||
             pchr_b->holdingwhich[SLOT_RIGHT] == ichr_a ) continue;

        // do not notice completely broken items?
        if ( pchr_b->isitem && !pchr_b->alive ) continue;

        // reasonable carrying capacity
        if ( pchr_b->phys.weight > pchr_a->phys.weight + pchr_a->strength * INV_FF )
        {
            can_grab = bfalse;
        }

        // grab_people == btrue allows you to pick up living non-items
        // grab_people == false allows you to pick up living (functioning) items
        if ( !grab_people && !pchr_b->isitem )
        {
            can_grab = bfalse;
        }

        // is the object visible
        too_dark  = !chr_can_see_dark( pchr_a, pchr_b );
        too_invis = !chr_can_see_invis( pchr_a, pchr_b );

        // calculate the distance
        diff = fvec3_sub( chr_get_pos_v( pchr_b ), slot_pos.v );
        diff.z += pchr_b->bump.height * 0.5f;

        // find the squared difference horizontal and vertical
        diff2_hrz = fvec2_length_2( diff.v );
        diff2_vrt = diff.z * diff.z;

        // determine the actual max vertical distance
        grab2_vrt = SQR( pchr_b->bump.height );
        grab2_vrt = MAX( grab2_vrt, const_grab2_vrt );

        // the normal horizontal grab distance is dependent on the size of the two objects
        bump_size2_b = SQR( pchr_b->bump.size );

        // visibility affects the max grab distance.
        // if it is not visible then we have to be touching it.
        grab2_hrz = MAX( bump_size2_a, bump_size2_b );
        if ( !too_dark && !too_invis )
        {
            grab2_hrz = MAX( grab2_hrz, const_grab2_hrz );
        }

        // the player can get info from objects that are farther away
        info2_hrz = MAX( grab2_hrz, const_info2_hrz );

        // Is it too far away to interact with?
        if ( diff2_hrz > info2_hrz || diff2_vrt > grab2_vrt ) continue;

        // is it too far away to grab?
        if ( diff2_hrz > grab2_hrz )
        {
            can_grab = bfalse;
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
            grab_list[grab_count].ichr      = ichr_b;
            grab_list[grab_count].diff      = diff;
            grab_list[grab_count].diff2_hrz = diff2_hrz;
            grab_list[grab_count].diff2_vrt = diff2_vrt;
            grab_list[grab_count].too_dark  = too_dark;
            grab_list[grab_count].too_invis = too_invis;
            grab_count++;
        }
        else
        {
            ungrab_list[ungrab_count].ichr      = ichr_b;
            ungrab_list[ungrab_count].diff      = diff;
            ungrab_list[ungrab_count].diff2_hrz = diff2_hrz;
            ungrab_list[ungrab_count].diff2_vrt = diff2_vrt;
            ungrab_list[ungrab_count].too_dark  = too_dark;
            ungrab_list[ungrab_count].too_invis = too_invis;
            ungrab_count++;
        }
    }
    CHR_END_LOOP();

    // sort the grab list
    if ( grab_count > 1 )
    {
        qsort( grab_list, grab_count, sizeof( grab_data_t ), grab_data_cmp );
    }

    // try to grab something
    retval = bfalse;
    if (( 0 == grab_count ) && ( 0 != grab_visible_count ) )
    {
        // There are items within the "normal" range that could be grabbed
        // but somehow they can't be seen.
        // Generate a billboard that tells the player what the problem is.
        // NOTE: this is not corerect since it could alert a player to an invisible object

        // 5 seconds and blue
        chr_make_text_billboard( ichr_a, "I can't feel anything...", color_blu, default_tint, 5, bb_opt_none );

        retval = btrue;
    }

    if ( !retval )
    {
        for ( cnt = 0; cnt < grab_count; cnt++ )
        {
            bool_t can_grab;

            chr_t * pchr_b;

            if ( grab_list[cnt].too_dark || grab_list[cnt].too_invis ) continue;

            ichr_b = grab_list[cnt].ichr;
            pchr_b = ChrList.lst + ichr_b;

            can_grab = can_grab_item_in_shop( ichr_a, ichr_b );

            if ( can_grab )
            {
                // Stick 'em together and quit
                if ( rv_success == attach_character_to_mount( ichr_b, ichr_a, grip_off ) )
                {
                    if ( grab_people )
                    {
                        // Start the slam animation...  ( Be sure to drop!!! )
                        chr_play_action( pchr_a, ACTION_MC + slot, bfalse );
                    }
                    retval = btrue;
                }
                break;
            }
            else
            {
                // Lift the item a little and quit...
                pchr_b->vel.z = DROPZVEL;
                pchr_b->hitready = btrue;
                SET_BIT( pchr_b->ai.alert, ALERTIF_DROPPED );
                break;
            }
        }
    }

    if ( !retval )
    {
        fvec3_t   vforward;

        //---- generate billboards for things that players can interact with
        if ( FEEDBACK_OFF != cfg.feedback && VALID_PLA( pchr_a->is_which_player ) )
        {
            // things that can be grabbed
            for ( cnt = 0; cnt < grab_count; cnt++ )
            {
                ichr_b = grab_list[cnt].ichr;
                if ( grab_list[cnt].too_dark || grab_list[cnt].too_invis )
                {
                    // (5 secs and blue)
                    chr_make_text_billboard( ichr_b, "Something...", color_blu, default_tint, 5, bb_opt_none );
                }
                else
                {
                    // (5 secs and green)
                    chr_make_text_billboard( ichr_b, chr_get_name( ichr_b, CHRNAME_ARTICLE | CHRNAME_CAPITAL ), color_grn, default_tint, 5, bb_opt_none );
                }
            }

            // things that can't be grabbed
            for ( cnt = 0; cnt < ungrab_count; cnt++ )
            {
                ichr_b = ungrab_list[cnt].ichr;

                if ( ungrab_list[cnt].too_dark || ungrab_list[cnt].too_invis )
                {
                    // (5 secs and blue)
                    chr_make_text_billboard( ichr_b, "Something...", color_blu, default_tint, 5, bb_opt_none );
                }
                else
                {
                    // (5 secs and red)
                    chr_make_text_billboard( ichr_b, chr_get_name( ichr_b, CHRNAME_ARTICLE | CHRNAME_CAPITAL ), color_red, default_tint, 5, bb_opt_none );
                }
            }
        }

        //---- if you can't grab anything, activate something using ALERTIF_BUMPED
        if ( VALID_PLA( pchr_a->is_which_player ) && ungrab_count > 0 )
        {
            chr_getMatForward( pchr_a, vforward.v );

            // sort the ungrab list
            if ( ungrab_count > 1 )
            {
                qsort( ungrab_list, ungrab_count, sizeof( grab_data_t ), grab_data_cmp );
            }

            for ( cnt = 0; cnt < ungrab_count; cnt++ )
            {
                float       ftmp;
                chr_t     * pchr_b;

                // only do visible objects
                if ( ungrab_list[cnt].too_dark || ungrab_list[cnt].too_invis ) continue;

                pchr_b = ChrList.lst + ungrab_list[cnt].ichr;

                // only bump the closest character that is in front of the character
                // (ignore vertical displacement)
                ftmp = fvec2_dot_product( vforward.v, ungrab_list[cnt].diff.v );
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
    /// @details ZZ@> This function spawns an attack particle

    CHR_REF iweapon, ithrown, iholder;
    chr_t * pchr, * pweapon;
    cap_t * pweapon_cap;

    PRT_REF iparticle;

    int   spawn_vrt_offset;
    Uint8 action;
    Uint16 turn;
    float velocity;

    bool_t unarmed_attack;

    if ( !INGAME_CHR( ichr ) ) return;
    pchr = ChrList.lst + ichr;

    iweapon = pchr->holdingwhich[slot];

    // See if it's an unarmed attack...
    if ( MAX_CHR == iweapon )
    {
        unarmed_attack   = btrue;
        iweapon          = ichr;
        spawn_vrt_offset = slot_to_grip_offset( slot );  // SLOT_LEFT -> GRIP_LEFT, SLOT_RIGHT -> GRIP_RIGHT
    }
    else
    {
        unarmed_attack   = bfalse;
        spawn_vrt_offset = GRIP_LAST;
        action = pchr->inst.action_which;
    }

    if ( !INGAME_CHR( iweapon ) ) return;
    pweapon = ChrList.lst + iweapon;

    pweapon_cap = chr_get_pcap( iweapon );
    if ( NULL == pweapon_cap ) return;

    // find the 1st non-item that is holding the weapon
    iholder = chr_get_lowest_attachment( iweapon, btrue );

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
        chr_play_action( pweapon, ACTION_MJ, bfalse );

        SET_BIT( pweapon->ai.alert, ALERTIF_USED );
    }

    // What kind of attack are we going to do?
    if ( !unarmed_attack && (( pweapon_cap->isstackable && pweapon->ammo > 1 ) || ACTION_IS_TYPE( pweapon->inst.action_which, F ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation
        ithrown = spawn_one_character( pchr->pos, pweapon->profile_ref, chr_get_iteam( iholder ), 0, pchr->ori.facing_z, pweapon->Name, ( CHR_REF )MAX_CHR );
        if ( DEFINED_CHR( ithrown ) )
        {
            chr_t * pthrown = ChrList.lst + ithrown;

            pthrown->iskursed = bfalse;
            pthrown->ammo = 1;
            SET_BIT( pthrown->ai.alert, ALERTIF_THROWN );
            velocity = pchr->strength / ( pthrown->phys.weight * THROWFIX );
            velocity += MINTHROWVELOCITY;
            velocity = MIN( velocity, MAXTHROWVELOCITY );

            turn = TO_TURN( pchr->ori.facing_z + ATK_BEHIND );
            pthrown->vel.x += turntocos[ turn ] * velocity;
            pthrown->vel.y += turntosin[ turn ] * velocity;
            pthrown->vel.z = DROPZVEL;
            if ( pweapon->ammo <= 1 )
            {
                // Poof the item
                detach_character_from_mount( iweapon, btrue, bfalse );
                chr_request_terminate( GET_REF_PCHR( pweapon ) );
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
            if ( pweapon->ammo > 0 && !pweapon_cap->isstackable )
            {
                pweapon->ammo--;  // Ammo usage
            }

            // Spawn an attack particle
            if ( -1 != pweapon_cap->attack_lpip )
            {
                // make the weapon's holder the owner of the attack particle?
                // will this mess up wands?
                iparticle = spawn_one_particle( pweapon->pos, pchr->ori.facing_z, pweapon->profile_ref, pweapon_cap->attack_lpip, iweapon, spawn_vrt_offset, chr_get_iteam( iholder ), iholder, ( PRT_REF )MAX_PRT, 0, ( CHR_REF )MAX_CHR );

                if ( DEFINED_PRT( iparticle ) )
                {
                    fvec3_t tmp_pos;
                    prt_t * pprt = PrtList.lst + iparticle;

                    tmp_pos = prt_get_pos( pprt );

                    if ( pweapon_cap->attack_attached )
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
                    else if ( prt_get_ppip( iparticle )->startontarget && INGAME_CHR( pprt->target_ref ) )
                    {
                        pprt = place_particle_at_vertex( pprt, pprt->target_ref, spawn_vrt_offset );
                        if ( NULL == pprt ) return;

                        // Correct Z spacing base, but nothing else...
                        tmp_pos.z += prt_get_ppip( iparticle )->spacing_vrt_pair.base;
                    }
                    else
                    {
                        // NOT ATTACHED
                        pprt->attachedto_ref = ( CHR_REF )MAX_CHR;

                        // Don't spawn in walls
                        if ( EMPTY_BIT_FIELD != prt_test_wall( pprt, tmp_pos.v, NULL ) )
                        {
                            tmp_pos.x = pweapon->pos.x;
                            tmp_pos.y = pweapon->pos.y;
                            if ( EMPTY_BIT_FIELD != prt_test_wall( pprt, tmp_pos.v, NULL ) )
                            {
                                tmp_pos.x = pchr->pos.x;
                                tmp_pos.y = pchr->pos.y;
                            }
                        }
                    }

                    // Initial particles get a bonus, which may be 0.00f
                    pprt->damage.base += ( pchr->strength     * pweapon_cap->str_bonus );
                    pprt->damage.base += ( pchr->wisdom       * pweapon_cap->wis_bonus );
                    pprt->damage.base += ( pchr->intelligence * pweapon_cap->int_bonus );
                    pprt->damage.base += ( pchr->dexterity    * pweapon_cap->dex_bonus );

                    // Initial particles get an enchantment bonus
                    pprt->damage.base += pweapon->damage_boost;

                    prt_set_pos( pprt, tmp_pos.v );
                }
            }
        }
        else
        {
            pweapon->ammoknown = btrue;
        }
    }
}

//--------------------------------------------------------------------------------------------
void drop_money( const CHR_REF character, int money )
{
    /// @details ZZ@> This function drops some of a character's money

    int vals[PIP_MONEY_COUNT] = {1, 5, 25, 100, 200, 500, 1000, 2000};
    int pips[PIP_MONEY_COUNT] =
    {
        PIP_COIN1, PIP_COIN5, PIP_COIN25, PIP_COIN100,
        PIP_GEM200, PIP_GEM500, PIP_GEM1000, PIP_GEM2000
    };

    chr_t * pchr;
    fvec3_t loc_pos;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    fvec3_base_copy( loc_pos.v, chr_get_pos_v( pchr ) );

    // limit the about of money to the character's actual money
    if ( money > ChrList.lst[character].money )
    {
        money = ChrList.lst[character].money;
    }

    if ( money > 0 && loc_pos.z > -2 )
    {
        int cnt, tnc;
        int count;

        // remove the money from inventory
        pchr->money -= money;

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
    /// @details ZZ@> This function issues a call for help to all allies

    TEAM_REF team;

    if ( !INGAME_CHR( character ) ) return;

    team = chr_get_iteam( character );
    TeamStack.lst[team].sissy = character;

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        if ( cnt != character && !team_hates_team( pchr->team, team ) )
        {
            SET_BIT( pchr->ai.alert, ALERTIF_CALLEDFORHELP );
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
bool_t setup_xp_table( const CAP_REF icap )
{
    /// @details ZF@> This calculates the xp needed to reach next level and stores it in an array for later use

    Uint8 level;
    cap_t * pcap;

    if ( !LOADED_CAP( icap ) ) return bfalse;
    pcap = CapStack.lst + icap;

    // Calculate xp needed
    for ( level = MAXBASELEVEL; level < MAXLEVEL; level++ )
    {
        Uint32 xpneeded = pcap->experience_forlevel[MAXBASELEVEL - 1];
        xpneeded += ( level * level * level * 15 );
        xpneeded -= (( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * 15 );
        pcap->experience_forlevel[level] = xpneeded;
    }
    return btrue;
}

//--------------------------------------------------------------------------------------------
void do_level_up( const CHR_REF character )
{
    /// @details BB@> level gains are done here, but only once a second

    Uint8 curlevel;
    int number;
    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return;

    // Do level ups and stat changes
    curlevel = pchr->experiencelevel + 1;
    if ( curlevel < MAXLEVEL )
    {
        Uint32 xpcurrent, xpneeded;

        xpcurrent = pchr->experience;
        xpneeded  = pcap->experience_forlevel[curlevel];
        if ( xpcurrent >= xpneeded )
        {
            // do the level up
            pchr->experiencelevel++;
            xpneeded = pcap->experience_forlevel[curlevel];
            SET_BIT( pchr->ai.alert, ALERTIF_LEVELUP );

            // The character is ready to advance...
            if ( VALID_PLA( pchr->is_which_player ) )
            {
                debug_printf( "%s gained a level!!!", chr_get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
                sound_play_chunk_full( g_wavelist[GSND_LEVELUP] );
            }

            // Size
            pchr->fat_goto += pcap->size_perlevel * 0.25f;  // Limit this?
            pchr->fat_goto_time += SIZETIME;

            // Strength
            number = generate_irand_range( pcap->strength_stat.perlevel );
            number += pchr->strength;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->strength = number;

            // Wisdom
            number = generate_irand_range( pcap->wisdom_stat.perlevel );
            number += pchr->wisdom;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->wisdom = number;

            // Intelligence
            number = generate_irand_range( pcap->intelligence_stat.perlevel );
            number += pchr->intelligence;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->intelligence = number;

            // Dexterity
            number = generate_irand_range( pcap->dexterity_stat.perlevel );
            number += pchr->dexterity;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->dexterity = number;

            // Life
            number = generate_irand_range( pcap->life_stat.perlevel );
            number += pchr->lifemax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            pchr->life += ( number - pchr->lifemax );
            pchr->lifemax = number;

            // Mana
            number = generate_irand_range( pcap->mana_stat.perlevel );
            number += pchr->manamax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            pchr->mana += ( number - pchr->manamax );
            pchr->manamax = number;

            // Mana Return
            number = generate_irand_range( pcap->manareturn_stat.perlevel );
            number += pchr->manareturn;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->manareturn = number;

            // Mana Flow
            number = generate_irand_range( pcap->manaflow_stat.perlevel );
            number += pchr->manaflow;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->manaflow = number;
        }
    }
}

//--------------------------------------------------------------------------------------------
void give_experience( const CHR_REF character, int amount, xp_type xptype, bool_t override_invictus )
{
    /// @details ZZ@> This function gives a character experience

    float newamount;

    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return;

    //No xp to give
    if ( 0 == amount ) return;

    if ( !pchr->invictus || override_invictus )
    {
        float intadd = ( FP8_TO_INT( pchr->intelligence ) - 10.0f ) / 200.0f;
        float wisadd = ( FP8_TO_INT( pchr->wisdom )       - 10.0f ) / 400.0f;

        // Figure out how much experience to give
        newamount = amount;
        if ( xptype < XP_COUNT )
        {
            newamount = amount * pcap->experience_rate[xptype];
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
void give_team_experience( const TEAM_REF team, int amount, Uint8 xptype )
{
    /// @details ZZ@> This function gives every character on a team experience

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        if ( pchr->team == team )
        {
            give_experience( cnt, amount, ( xp_type )xptype, bfalse );
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
chr_t * resize_one_character( chr_t * pchr )
{
    /// @details ZZ@> This function makes the characters get bigger or smaller, depending
    ///    on their fat_goto and fat_goto_time. Spellbooks do not resize
    ///    BB@> assume that this will only be called from inside chr_config_do_active(),
    ///         so pchr is just right to be used here

    CHR_REF ichr;
    cap_t * pcap;
    bool_t  willgetcaught;
    float   newsize;

    if ( NULL == pchr ) return pchr;

    ichr = GET_REF_PCHR( pchr );
    pcap = chr_get_pcap( ichr );

    if ( pchr->fat_goto_time < 0 ) return pchr;

    if ( pchr->fat_goto != pchr->fat )
    {
        int bump_increase;

        bump_increase = ( pchr->fat_goto - pchr->fat ) * 0.10f * pchr->bump.size;

        // Make sure it won't get caught in a wall
        willgetcaught = bfalse;
        if ( pchr->fat_goto > pchr->fat )
        {
            pchr->bump.size += bump_increase;

            if ( EMPTY_BIT_FIELD != chr_test_wall( pchr, NULL, NULL ) )
            {
                willgetcaught = btrue;
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

            if ( CAP_INFINITE_WEIGHT == pcap->weight )
            {
                pchr->phys.weight = CHR_INFINITE_WEIGHT;
            }
            else
            {
                Uint32 itmp = pcap->weight * pchr->fat * pchr->fat * pchr->fat;
                pchr->phys.weight = MIN( itmp, CHR_MAX_WEIGHT );
            }
        }
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_quest_vfs( const char *szSaveName, const CHR_REF character )
{
    /// @details ZZ@> This function makes the naming.txt file for the character

    player_t *ppla;
    egoboo_rv rv;

    if ( !INGAME_CHR( character ) ) return bfalse;

    ppla = chr_get_ppla( character );
    if ( ppla == NULL ) return bfalse;

    rv = quest_log_upload_vfs( ppla->quest_log, SDL_arraysize( ppla->quest_log ), szSaveName );
    return rv_success == rv ? btrue : bfalse;
}

//--------------------------------------------------------------------------------------------
//void resize_all_characters()
//{
//    /// @details ZZ@> This function makes the characters get bigger or smaller, depending
//    ///    on their fat_goto and fat_goto_time. Spellbooks do not resize
//
//    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
//    {
//        resize_one_character( pchr );
//    }
//    CHR_END_LOOP();
//}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_name_vfs( const char *szSaveName, const CHR_REF character )
{
    /// @details ZZ@> This function makes the naming.txt file for the character

    if ( !INGAME_CHR( character ) ) return bfalse;

    return chop_export_vfs( szSaveName, ChrList.lst[character].Name );
}

//--------------------------------------------------------------------------------------------
bool_t chr_upload_cap( chr_t * pchr, cap_t * pcap )
{
    /// @details BB@> prepare a character profile for exporting, by uploading some special values into the
    ///     cap. Just so that there is no confusion when you export multiple items of the same type,
    ///     DO NOT pass the pointer returned by chr_get_pcap(). Instead, use a custom cap_t declared on the stack,
    ///     or something similar
    ///
    /// @note This has been modified to basically reverse the actions of chr_download_cap().
    ///       If all enchants have been removed, this should export all permanent changes to the
    ///       base character profile.

    int tnc;

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    if ( NULL == pcap || !pcap->loaded ) return bfalse;

    // export values that override spawn.txt values
    pcap->content_override   = pchr->ai.content;
    pcap->state_override     = pchr->ai.state;
    pcap->money              = pchr->money;
    pcap->skin_override      = pchr->skin;
    pcap->level_override     = pchr->experiencelevel;

    // export the current experience
    ints_to_range( pchr->experience, 0, &( pcap->experience ) );

    // export the current mana and life
    pcap->life_spawn         = CLIP( pchr->life, 0, pchr->lifemax );
    pcap->mana_spawn         = CLIP( pchr->mana, 0, pchr->manamax );

    // Movement
    pcap->anim_speed_sneak = pchr->anim_speed_sneak;
    pcap->anim_speed_walk = pchr->anim_speed_walk;
    pcap->anim_speed_run = pchr->anim_speed_run;

    // weight and size
    pcap->size       = pchr->fat_goto;
    pcap->bumpdampen = pchr->phys.bumpdampen;
    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight )
    {
        pcap->weight = CAP_INFINITE_WEIGHT;
    }
    else
    {
        Uint32 itmp = pchr->phys.weight / pchr->fat / pchr->fat / pchr->fat;
        pcap->weight = MIN( itmp, CAP_MAX_WEIGHT );
    }

    // Other junk
    pcap->flyheight   = pchr->flyheight;
    pcap->alpha       = pchr->alpha_base;
    pcap->light       = pchr->light_base;
    pcap->flashand    = pchr->flashand;
    pcap->dampen      = pchr->phys.dampen;

    // Jumping
    pcap->jump       = pchr->jump_power;
    pcap->jumpnumber = pchr->jumpnumberreset;

    // Flags
    pcap->stickybutt      = pchr->stickybutt;
    pcap->canopenstuff    = pchr->openstuff;
    pcap->transferblend   = pchr->transferblend;
    pcap->waterwalk       = pchr->waterwalk;
    pcap->platform        = pchr->platform;
    pcap->canuseplatforms = pchr->canuseplatforms;
    pcap->isitem          = pchr->isitem;
    pcap->invictus        = pchr->invictus;
    pcap->ismount         = pchr->ismount;
    pcap->cangrabmoney    = pchr->cangrabmoney;

    // Damage
    pcap->attachedprt_reaffirm_damagetype = pchr->reaffirm_damagetype;
    pcap->damagetarget_damagetype         = pchr->damagetarget_damagetype;

    // SWID
    ints_to_range( pchr->strength    , 0, &( pcap->strength_stat.val ) );
    ints_to_range( pchr->wisdom      , 0, &( pcap->wisdom_stat.val ) );
    ints_to_range( pchr->intelligence, 0, &( pcap->intelligence_stat.val ) );
    ints_to_range( pchr->dexterity   , 0, &( pcap->dexterity_stat.val ) );

    // Life and Mana
    pcap->lifecolor = pchr->lifecolor;
    pcap->manacolor = pchr->manacolor;
    ints_to_range( pchr->lifemax     , 0, &( pcap->life_stat.val ) );
    ints_to_range( pchr->manamax     , 0, &( pcap->mana_stat.val ) );
    ints_to_range( pchr->manareturn  , 0, &( pcap->manareturn_stat.val ) );
    ints_to_range( pchr->manaflow    , 0, &( pcap->manaflow_stat.val ) );

    // Gender
    pcap->gender  = pchr->gender;

    // Ammo
    pcap->ammomax = pchr->ammomax;
    pcap->ammo    = pchr->ammo;

    // update any skills that have been learned
    idsz_map_copy( pchr->skills, SDL_arraysize( pchr->skills ), pcap->skills );

    // Enchant stuff
    pcap->see_invisible_level = pchr->see_invisible_level;

    // base kurse state
    pcap->kursechance = pchr->iskursed ? 100 : 0;

    // Model stuff
    pcap->stoppedby = pchr->stoppedby;
    pcap->life_heal = pchr->life_heal;
    pcap->manacost  = pchr->manacost;
    pcap->nameknown = pchr->nameknown || pchr->ammoknown;          // make sure that identified items are saved as identified
    pcap->draw_icon = pchr->draw_icon;

    // sound stuff...
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pcap->sound_index[tnc] = pchr->sound_index[tnc];
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_download_cap( chr_t * pchr, cap_t * pcap )
{
    /// @details BB@> grab all of the data from the data.txt file

    int iTmp, tnc;

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    if ( NULL == pcap || !pcap->loaded ) return bfalse;

    // sound stuff...  copy from the cap
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pchr->sound_index[tnc] = pcap->sound_index[tnc];
    }

    // Set up model stuff
    pchr->stoppedby = pcap->stoppedby;
    pchr->life_heal = pcap->life_heal;
    pchr->manacost  = pcap->manacost;
    pchr->nameknown = pcap->nameknown;
    pchr->ammoknown = pcap->nameknown;
    pchr->draw_icon = pcap->draw_icon;

    // calculate a base kurse state. this may be overridden later
    if ( pcap->isitem )
    {
        IPair loc_rand = {1, 100};
        pchr->iskursed = ( generate_irand_pair( loc_rand ) <= pcap->kursechance );
    }

    // Enchant stuff
    pchr->see_invisible_level = pcap->see_invisible_level;

    // Skillz
    idsz_map_copy( pcap->skills, SDL_arraysize( pcap->skills ), pchr->skills );
    pchr->darkvision_level = chr_get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
    pchr->see_invisible_level = pcap->see_invisible_level;

    // Ammo
    pchr->ammomax = pcap->ammomax;
    pchr->ammo = pcap->ammo;

    // Gender
    pchr->gender = pcap->gender;
    if ( pchr->gender == GENDER_RANDOM )  pchr->gender = generate_randmask( GENDER_FEMALE, GENDER_MALE );

    // Life and Mana
    pchr->lifecolor = pcap->lifecolor;
    pchr->manacolor = pcap->manacolor;
    pchr->lifemax = generate_irand_range( pcap->life_stat.val );
    pchr->life_return = pcap->life_return;
    pchr->manamax = generate_irand_range( pcap->mana_stat.val );
    pchr->manaflow = generate_irand_range( pcap->manaflow_stat.val );
    pchr->manareturn = generate_irand_range( pcap->manareturn_stat.val );

    // SWID
    pchr->strength = generate_irand_range( pcap->strength_stat.val );
    pchr->wisdom = generate_irand_range( pcap->wisdom_stat.val );
    pchr->intelligence = generate_irand_range( pcap->intelligence_stat.val );
    pchr->dexterity = generate_irand_range( pcap->dexterity_stat.val );

    // Skin
    pchr->skin = 0;
    if ( NO_SKIN_OVERRIDE != pcap->spelleffect_type )
    {
        pchr->skin = pcap->spelleffect_type % MAX_SKIN;
    }
    else if ( NO_SKIN_OVERRIDE != pcap->skin_override )
    {
        pchr->skin = pcap->skin_override % MAX_SKIN;
    }

    // Damage
    pchr->defense = pcap->defense[pchr->skin];
    pchr->reaffirm_damagetype = pcap->attachedprt_reaffirm_damagetype;
    pchr->damagetarget_damagetype = pcap->damagetarget_damagetype;
    for ( tnc = 0; tnc < DAMAGE_COUNT; tnc++ )
    {
        pchr->damage_modifier[tnc] = pcap->damage_modifier[tnc][pchr->skin];
    }

    // Flags
    pchr->stickybutt      = pcap->stickybutt;
    pchr->openstuff       = pcap->canopenstuff;
    pchr->transferblend   = pcap->transferblend;
    pchr->waterwalk       = pcap->waterwalk;
    pchr->platform        = pcap->platform;
    pchr->canuseplatforms = pcap->canuseplatforms;
    pchr->isitem          = pcap->isitem;
    pchr->invictus        = pcap->invictus;
    pchr->ismount         = pcap->ismount;
    pchr->cangrabmoney    = pcap->cangrabmoney;

    // Jumping
    pchr->jump_power = pcap->jump;
    pchr->jumpnumberreset = pcap->jumpnumber;

    // Other junk
    pchr->flyheight   = pcap->flyheight;
    pchr->maxaccel    = pchr->maxaccel_reset = pcap->maxaccel[pchr->skin];
    pchr->alpha_base  = pcap->alpha;
    pchr->light_base  = pcap->light;
    pchr->flashand    = pcap->flashand;
    pchr->phys.dampen = pcap->dampen;

    // Load current life and mana. this may be overridden later
    pchr->life = CLIP( pcap->life_spawn, LOWSTAT, pchr->lifemax );
    pchr->mana = CLIP( pcap->mana_spawn,       0, pchr->manamax );

    pchr->phys.bumpdampen = pcap->bumpdampen;
    if ( CAP_INFINITE_WEIGHT == pcap->weight )
    {
        pchr->phys.weight = CHR_INFINITE_WEIGHT;
    }
    else
    {
        Uint32 itmp = pcap->weight * pcap->size * pcap->size * pcap->size;
        pchr->phys.weight = MIN( itmp, CHR_MAX_WEIGHT );
    }

    // Image rendering
    pchr->uoffvel = pcap->uoffvel;
    pchr->voffvel = pcap->voffvel;

    // Movement
    pchr->anim_speed_sneak = pcap->anim_speed_sneak;
    pchr->anim_speed_walk = pcap->anim_speed_walk;
    pchr->anim_speed_run = pcap->anim_speed_run;

    // Money is added later
    pchr->money = pcap->money;

    // Experience
    iTmp = generate_irand_range( pcap->experience );
    pchr->experience      = MIN( iTmp, MAXXP );
    pchr->experiencelevel = pcap->level_override;

    // Particle attachments
    pchr->reaffirm_damagetype = pcap->attachedprt_reaffirm_damagetype;

    // Character size and bumping
    chr_init_size( pchr, pcap );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_profile_vfs( const char *szSaveName, const CHR_REF character )
{
    /// @details ZZ@> This function creates a data.txt file for the given character.
    ///    it is assumed that all enchantments have been done away with

    chr_t * pchr;
    cap_t * pcap;

    // a local version of the cap, so that the CapStack data won't be corrupted
    cap_t cap_tmp;

    if ( INVALID_CSTR( szSaveName ) && !DEFINED_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return bfalse;

    // load up the temporary cap
    memcpy( &cap_tmp, pcap, sizeof( cap_t ) );

    // fill in the cap values with the ones we want to export from the character profile
    chr_upload_cap( pchr, &cap_tmp );

    return save_one_cap_file_vfs( szSaveName, NULL, &cap_tmp );
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_skin_vfs( const char *szSaveName, const CHR_REF character )
{
    /// @details ZZ@> This function creates a skin.txt file for the given character.

    vfs_FILE* filewrite;

    if ( !INGAME_CHR( character ) ) return bfalse;

    // Open the file
    filewrite = vfs_openWrite( szSaveName );
    if ( NULL == filewrite ) return bfalse;

    vfs_printf( filewrite, "// This file is used only by the import menu\n" );
    vfs_printf( filewrite, ": %d\n", ChrList.lst[character].skin );
    vfs_close( filewrite );
    return btrue;
}

//--------------------------------------------------------------------------------------------
CAP_REF load_one_character_profile_vfs( const char * tmploadname, int slot_override, bool_t required )
{
    /// @details ZZ@> This function fills a character profile with data from data.txt, returning
    /// the icap slot that the profile was stuck into.  It may cause the program
    /// to abort if bad things happen.

    CAP_REF  icap = ( CAP_REF )MAX_CAP;
    cap_t * pcap;
    STRING  szLoadName;

    if ( VALID_PRO_RANGE( slot_override ) )
    {
        icap = ( CAP_REF )slot_override;
    }
    else
    {
        icap = pro_get_slot_vfs( tmploadname, MAX_PROFILE );
    }

    if ( !VALID_CAP_RANGE( icap ) )
    {
        // The data file wasn't found
        if ( required )
        {
            log_debug( "load_one_character_profile_vfs() - \"%s\" was not found. Overriding a global object?\n", szLoadName );
        }
        else if ( VALID_CAP_RANGE( slot_override ) && slot_override > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_debug( "load_one_character_profile_vfs() - Not able to open file \"%s\"\n", szLoadName );
        }

        return ( CAP_REF )MAX_CAP;
    }

    pcap = CapStack.lst + icap;

    // if there is data in this profile, release it
    if ( pcap->loaded )
    {
        // Make sure global objects don't load over existing models
        if ( required && SPELLBOOK == icap )
        {
            log_error( "Object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, szLoadName );
        }
        else if ( required && overrideslots )
        {
            log_error( "Object slot %i used twice (%s, %s)\n", REF_TO_INT( icap ), pcap->name, szLoadName );
        }
        else
        {
            // Stop, we don't want to override it
            return ( CAP_REF )MAX_CAP;
        }

        // If loading over an existing model is allowed (?how?) then make sure to release the old one
        release_one_cap( icap );
    }

    if ( NULL == load_one_cap_file_vfs( tmploadname, pcap ) )
    {
        return ( CAP_REF )MAX_CAP;
    }

    // do the rest of the levels not listed in data.txt
    setup_xp_table( icap );

    if ( cfg.gouraud_req )
    {
        pcap->uniformlit = bfalse;
    }

    // limit the wave indices to rational values
    pcap->sound_index[SOUND_FOOTFALL] = CLIP( pcap->sound_index[SOUND_FOOTFALL], INVALID_SOUND, MAX_WAVE );
    pcap->sound_index[SOUND_JUMP]     = CLIP( pcap->sound_index[SOUND_JUMP], INVALID_SOUND, MAX_WAVE );

    //0 == bumpdampenmeans infinite mass, and causes some problems
    pcap->bumpdampen = MAX( INV_FF, pcap->bumpdampen );

    return icap;
}

//--------------------------------------------------------------------------------------------
bool_t heal_character( const CHR_REF character, const CHR_REF healer, int amount, bool_t ignore_invictus )
{
    /// @details ZF@> This function gives some pure life points to the target, ignoring any resistances and so forth
    chr_t * pchr, *pchr_h;

    //Setup the healed character
    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    //Setup the healer
    if ( !INGAME_CHR( healer ) ) return bfalse;
    pchr_h = ChrList.lst + healer;

    //Don't heal dead and invincible stuff
    if ( !pchr->alive || ( pchr->invictus && !ignore_invictus ) ) return bfalse;

    //This actually heals the character
    pchr->life = CLIP( pchr->life, pchr->life + ABS( amount ), pchr->lifemax );

    // Set alerts, but don't alert that we healed ourselves
    if ( healer != character && pchr_h->attachedto != character && ABS( amount ) > HURTDAMAGE )
    {
        SET_BIT( pchr->ai.alert, ALERTIF_HEALED );
        pchr->ai.attacklast = healer;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void cleanup_one_character( chr_t * pchr )
{
    /// @details BB@> Everything necessary to disconnect one character from the game

    CHR_REF  ichr, itmp;
    SHOP_REF ishop;

    if ( !ALLOCATED_PCHR( pchr ) ) return;
    ichr = GET_REF_PCHR( pchr );

    pchr->sparkle = NOSPARKLE;

    // Remove it from the team
    pchr->team = pchr->baseteam;
    if ( TeamStack.lst[pchr->team].morale > 0 ) TeamStack.lst[pchr->team].morale--;

    if ( TeamStack.lst[pchr->team].leader == ichr )
    {
        // The team now has no leader if the character is the leader
        TeamStack.lst[pchr->team].leader = NOLEADER;
    }

    // Clear all shop passages that it owned...
    for ( ishop = 0; ishop < ShopStack.count; ishop++ )
    {
        if ( ShopStack.lst[ishop].owner != ichr ) continue;
        ShopStack.lst[ishop].owner = SHOP_NOOWNER;
    }

    // detach from any mount
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        detach_character_from_mount( ichr, btrue, bfalse );
    }

    // drop your left item
    itmp = pchr->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( itmp ) && ChrList.lst[itmp].isitem )
    {
        detach_character_from_mount( itmp, btrue, bfalse );
    }

    // drop your right item
    itmp = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( itmp ) && ChrList.lst[itmp].isitem )
    {
        detach_character_from_mount( itmp, btrue, bfalse );
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
        while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
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
    looped_stop_object_sounds( ichr );
}

//--------------------------------------------------------------------------------------------
void kill_character( const CHR_REF ichr, const CHR_REF killer, bool_t ignore_invictus )
{
    /// @details BB@> Handle a character death. Set various states, disconnect it from the world, etc.

    chr_t * pchr;
    cap_t * pcap;
    int action;
    Uint16 experience;
    TEAM_REF killer_team;

    if ( !DEFINED_CHR( ichr ) ) return;
    pchr = ChrList.lst + ichr;

    //No need to continue is there?
    if ( !pchr->alive || ( pchr->invictus && !ignore_invictus ) ) return;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return;

    killer_team = chr_get_iteam( killer );

    pchr->alive = bfalse;
    pchr->waskilled = btrue;

    pchr->life            = -1;
    pchr->platform        = btrue;
    pchr->canuseplatforms = btrue;
    pchr->phys.bumpdampen = pchr->phys.bumpdampen / 2.0f;

    // Play the death animation
    action = ACTION_KA + generate_randmask( 0, 3 );
    chr_play_action( pchr, action, bfalse );
    chr_instance_set_action_keep( &( pchr->inst ), btrue );

    // Give kill experience
    experience = pcap->experience_worth + ( pchr->experience * pcap->experience_exchange );

    // distribute experience to the attacker
    if ( INGAME_CHR( killer ) )
    {
        // Set target
        pchr->ai.target = killer;
        if ( killer_team == TEAM_DAMAGE || killer_team == TEAM_NULL )  pchr->ai.target = ichr;

        // Award experience for kill?
        if ( team_hates_team( killer_team, pchr->team ) )
        {
            //Check for special hatred
            if ( chr_get_idsz( killer, IDSZ_HATE ) == chr_get_idsz( ichr, IDSZ_PARENT ) ||
                 chr_get_idsz( killer, IDSZ_HATE ) == chr_get_idsz( ichr, IDSZ_TYPE ) )
            {
                give_experience( killer, experience, XP_KILLHATED, bfalse );
            }

            // Nope, award direct kill experience instead
            else give_experience( killer, experience, XP_KILLENEMY, bfalse );
        }
    }

    //Set various alerts to let others know it has died
    //and distribute experience to whoever needs it
    SET_BIT( pchr->ai.alert, ALERTIF_KILLED );

    CHR_BEGIN_LOOP_ACTIVE( tnc, plistener )
    {
        if ( !plistener->alive ) continue;

        // All allies get team experience, but only if they also hate the dead guy's team
        if ( tnc != killer && !team_hates_team( plistener->team, killer_team ) && team_hates_team( plistener->team, pchr->team ) )
        {
            give_experience( tnc, experience, XP_TEAMKILL, bfalse );
        }

        // Check if it was a leader
        if ( TeamStack.lst[pchr->team].leader == ichr && chr_get_iteam( tnc ) == pchr->team )
        {
            // All folks on the leaders team get the alert
            SET_BIT( plistener->ai.alert, ALERTIF_LEADERKILLED );
        }

        // Let the other characters know it died
        if ( plistener->ai.target == ichr )
        {
            SET_BIT( plistener->ai.alert, ALERTIF_TARGETKILLED );
        }
    }
    CHR_END_LOOP();

    // Detach the character from the game
    cleanup_one_character( pchr );

    // If it's a player, let it die properly before enabling respawn
    if ( VALID_PLA( pchr->is_which_player ) ) local_stats.revivetimer = ONESECOND; // 1 second

    // Let it's AI script run one last time
    pchr->ai.timer = update_wld + 1;            // Prevent IfTimeOut in scr_run_chr_script()
    scr_run_chr_script( ichr );

    // Stop any looped sounds
    if ( pchr->loopedsound_channel != INVALID_SOUND ) sound_stop_channel( pchr->loopedsound_channel );
    looped_stop_object_sounds( ichr );
    pchr->loopedsound_channel = INVALID_SOUND;
}

//--------------------------------------------------------------------------------------------
int damage_character( const CHR_REF character, FACING_T direction,
                      IPair  damage, Uint8 damagetype, TEAM_REF team,
                      CHR_REF attacker, BIT_FIELD effects, bool_t ignore_invictus )
{
    /// @details ZZ@> This function calculates and applies damage to a character.  It also
    ///    sets alerts and begins actions.  Blocking and frame invincibility
    ///    are done here too.  Direction is ATK_FRONT if the attack is coming head on,
    ///    ATK_RIGHT if from the right, ATK_BEHIND if from the back, ATK_LEFT if from the
    ///    left.

    int     action;
    int     actual_damage, base_damage, max_damage;
    chr_t * pchr;
    cap_t * pcap;
    bool_t do_feedback = ( FEEDBACK_OFF != cfg.feedback );
    bool_t friendly_fire = bfalse, immune_to_damage = bfalse;
    Uint8  damage_modifier = 0;

    // what to do is damagetype == NONE?

    if ( !INGAME_CHR( character ) ) return 0;
    pchr = ChrList.lst + character;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return 0;

    // make a special exception for DAMAGE_NONE
    damage_modifier = ( damagetype >= DAMAGE_COUNT ) ? 0 : pchr->damage_modifier[damagetype];

    //Don't continue if there is no damage or the character isn't alive
    max_damage = ABS( damage.base ) + ABS( damage.rand );
    if ( !pchr->alive || 0 == max_damage ) return 0;

    // determine some optional behavior
    if ( !INGAME_CHR( attacker ) )
    {
        do_feedback = bfalse;
    }
    else
    {
        // do not show feedback for damaging yourself
        if ( attacker == character )
        {
            do_feedback = bfalse;
        }

        // identify friendly fire for color selection :)
        if ( chr_get_iteam( character ) == chr_get_iteam( attacker ) )
        {
            friendly_fire = btrue;
        }

        // don't show feedback from random objects hitting each other
        if ( !ChrList.lst[attacker].StatusList_on )
        {
            do_feedback = bfalse;
        }

        // don't show damage to players since they get feedback from the status bars
        if ( pchr->StatusList_on || VALID_PLA( pchr->is_which_player ) )
        {
            do_feedback = bfalse;
        }
    }

    // Lessen actual_damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
    // This can also be used to lessen effectiveness of healing
    actual_damage = generate_irand_pair( damage );
    base_damage   = actual_damage;
    actual_damage = actual_damage >> GET_DAMAGE_RESIST( damage_modifier );

    // Increase electric damage when in water
    if ( damagetype == DAMAGE_ZAP && chr_is_over_water( pchr ) )
    {
        // Only if actually in the water
        if ( pchr->pos.z <= water.surface_level )
            actual_damage = actual_damage << 1;     //@note: ZF> Is double damage too much?
    }

    // Allow actual_damage to be dealt to mana (mana shield spell)
    if ( HAS_SOME_BITS( damage_modifier, DAMAGEMANA ) )
    {
        int manadamage;
        manadamage = MAX( pchr->mana - actual_damage, 0 );
        pchr->mana = manadamage;
        actual_damage -= manadamage;
        if ( pchr->ai.index != attacker )
        {
            SET_BIT( pchr->ai.alert, ALERTIF_ATTACKED );
            pchr->ai.attacklast = attacker;
        }
    }

    // Allow charging (Invert actual_damage to mana)
    if ( HAS_SOME_BITS( damage_modifier, DAMAGECHARGE ) )
    {
        pchr->mana += actual_damage;
        if ( pchr->mana > pchr->manamax )
        {
            pchr->mana = pchr->manamax;
        }
        return 0;
    }

    // Invert actual_damage to heal
    if ( HAS_SOME_BITS( damage_modifier, DAMAGEINVERT ) )
        actual_damage = -actual_damage;

    // Remember the actual_damage type
    pchr->ai.damagetypelast = damagetype;
    pchr->ai.directionlast  = direction;

    // Check for characters who are immune to this damage, no need to continue if they have
    immune_to_damage = ( actual_damage > 0 && actual_damage <= pchr->damage_threshold ) || HAS_SOME_BITS( damage_modifier, DAMAGEINVICTUS );
    if ( immune_to_damage )
    {
        //Dark green text
        const float lifetime = 3;
        SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};
        GLXvector4f tint  = { 0.0f, 0.5f, 0.00f, 1.00f };

        actual_damage = 0;
        spawn_defense_ping( pchr, attacker );

        //Character is simply immune to the damage
        chr_make_text_billboard( character, "Immune!", text_color, tint, lifetime, bb_opt_all );
    }

    // Do it already
    if ( actual_damage > 0 )
    {
        // Only actual_damage if not invincible
        if ( 0 == pchr->damage_timer || ignore_invictus )
        {
            // Hard mode deals 25% extra actual damage to players!
            if ( cfg.difficulty >= GAME_HARD && VALID_PLA( pchr->is_which_player ) && !VALID_PLA( ChrList.lst[attacker].is_which_player ) ) actual_damage *= 1.25f;

            // Easy mode deals 25% extra actual damage by players and 25% less to players
            if ( cfg.difficulty <= GAME_EASY )
            {
                if ( VALID_PLA( ChrList.lst[attacker].is_which_player ) && !VALID_PLA( pchr->is_which_player ) ) actual_damage *= 1.25f;
                if ( !VALID_PLA( ChrList.lst[attacker].is_which_player ) &&  VALID_PLA( pchr->is_which_player ) ) actual_damage *= 0.75f;
            }

            if ( 0 != actual_damage )
            {
                if ( HAS_NO_BITS( DAMFX_ARMO, effects ) )
                {
                    actual_damage *= pchr->defense * INV_FF;
                }

                pchr->life -= actual_damage;

                // Spawn blud particles
                if ( pcap->blud_valid )
                {
                    if ( pcap->blud_valid == ULTRABLUDY || ( base_damage > HURTDAMAGE && damagetype < DAMAGE_HOLY ) )
                    {
                        spawn_one_particle( pchr->pos, pchr->ori.facing_z + direction, pchr->profile_ref, pcap->blud_lpip,
                                            ( CHR_REF )MAX_CHR, GRIP_LAST, pchr->team, character, ( PRT_REF )MAX_PRT, 0, ( CHR_REF )MAX_CHR );
                    }
                }

                // Set attack alert if it wasn't an accident
                if ( base_damage > HURTDAMAGE )
                {
                    if ( team == TEAM_DAMAGE )
                    {
                        pchr->ai.attacklast = ( CHR_REF )MAX_CHR;
                    }
                    else
                    {
                        // Don't alert the character too much if under constant fire
                        if ( 0 == pchr->careful_timer )
                        {
                            // Don't let characters chase themselves...  That would be silly
                            if ( attacker != character )
                            {
                                SET_BIT( pchr->ai.alert, ALERTIF_ATTACKED );
                                pchr->ai.attacklast = attacker;
                                pchr->careful_timer = CAREFULTIME;
                            }
                        }
                    }
                }

                // Taking actual_damage action
                if ( pchr->life <= 0 )
                {
                    kill_character( character, attacker, ignore_invictus );
                }
                else
                {
                    action = ACTION_HA;
                    if ( base_damage > HURTDAMAGE )
                    {
                        action += generate_randmask( 0, 3 );
                        chr_play_action( pchr, action, bfalse );

                        // Make the character invincible for a limited time only
                        if ( HAS_NO_BITS( effects, DAMFX_TIME ) )
                        {
                            pchr->damage_timer = DAMAGETIME;
                        }
                    }
                }
            }

            /// @test spawn a fly-away damage indicator?
            if ( do_feedback )
            {
                const char * tmpstr;
                int rank;

                //tmpstr = describe_wounds( pchr->lifemax, pchr->life );

                tmpstr = describe_value( actual_damage, INT_TO_FP8( 10 ), &rank );
                if ( rank < 4 )
                {
                    tmpstr = describe_value( actual_damage, max_damage, &rank );
                    if ( rank < 0 )
                    {
                        tmpstr = "Fumble!";
                    }
                    else
                    {
                        tmpstr = describe_damage( actual_damage, pchr->lifemax, &rank );
                        if ( rank >= -1 && rank <= 1 )
                        {
                            tmpstr = describe_wounds( pchr->lifemax, pchr->life );
                        }
                    }
                }

                if ( NULL != tmpstr )
                {
                    const int lifetime = 3;
                    STRING text_buffer = EMPTY_CSTR;

                    // "white" text
                    SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};

                    // friendly fire damage = "purple"
                    GLXvector4f tint_friend = { 0.88f, 0.75f, 1.00f, 1.00f };

                    // enemy damage = "red"
                    GLXvector4f tint_enemy  = { 1.00f, 0.75f, 0.75f, 1.00f };

                    // write the string into the buffer
                    snprintf( text_buffer, SDL_arraysize( text_buffer ), "%s", tmpstr );

                    chr_make_text_billboard( character, text_buffer, text_color, friendly_fire ? tint_friend : tint_enemy, lifetime, bb_opt_all );
                }
            }
        }
    }

    // Heal 'em instead
    else if ( actual_damage < 0 )
    {
        heal_character( character, attacker, actual_damage, ignore_invictus );

        // Isssue an alert
        if ( team == TEAM_DAMAGE )
        {
            pchr->ai.attacklast = ( CHR_REF )MAX_CHR;
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

            chr_make_text_billboard( character, text_buffer, text_color, tint, lifetime, bb_opt_all );
        }
    }

    return actual_damage;
}

//--------------------------------------------------------------------------------------------
void spawn_defense_ping( chr_t *pchr, const CHR_REF attacker )
{
    /// @details ZF@> Spawn a defend particle
    if ( 0 != pchr->damage_timer ) return;

    spawn_one_particle_global( pchr->pos, pchr->ori.facing_z, PIP_DEFEND, 0 );

    pchr->damage_timer    = DEFENDTIME;
    SET_BIT( pchr->ai.alert, ALERTIF_BLOCKED );
    pchr->ai.attacklast = attacker;                 // For the ones attacking a shield
}

//--------------------------------------------------------------------------------------------
void spawn_poof( const CHR_REF character, const PRO_REF profile )
{
    /// @details ZZ@> This function spawns a character poof

    FACING_T facing_z;
    CHR_REF  origin;
    int      cnt;

    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    pcap = pro_get_pcap( profile );
    if ( NULL == pcap ) return;

    origin = pchr->ai.owner;
    facing_z   = pchr->ori.facing_z;
    for ( cnt = 0; cnt < pcap->gopoofprt_amount; cnt++ )
    {
        spawn_one_particle( pchr->pos_old, facing_z, profile, pcap->gopoofprt_lpip,
                            ( CHR_REF )MAX_CHR, GRIP_LAST, pchr->team, origin, ( PRT_REF )MAX_PRT, cnt, ( CHR_REF )MAX_CHR );

        facing_z += pcap->gopoofprt_facingadd;
    }
}

//--------------------------------------------------------------------------------------------
void ai_state_spawn( ai_state_t * pself, const CHR_REF index, const PRO_REF iobj, Uint16 rank )
{
    chr_t * pchr;
    pro_t * ppro;
    cap_t * pcap;

    pself = ai_state_ctor( pself );

    if ( NULL == pself || !DEFINED_CHR( index ) ) return;
    pchr = ChrList.lst + index;

    // a character cannot be spawned without a valid profile
    if ( !LOADED_PRO( iobj ) ) return;
    ppro = ProList.lst + iobj;

    // a character cannot be spawned without a valid cap
    pcap = pro_get_pcap( iobj );
    if ( NULL == pcap ) return;

    pself->index      = index;
    pself->type       = ppro->iai;
    pself->alert      = ALERTIF_SPAWNED;
    pself->state      = pcap->state_override;
    pself->content    = pcap->content_override;
    pself->passage    = 0;
    pself->target     = index;
    pself->owner      = index;
    pself->child      = index;
    pself->target_old = index;

    pself->bumplast   = index;
    pself->hitlast    = index;

    pself->order_counter = rank;
    pself->order_value   = 0;
}

//--------------------------------------------------------------------------------------------
bool_t chr_get_environment( chr_t * pchr )
{
    if ( NULL == pchr ) return bfalse;

    move_one_character_get_environment( pchr );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
chr_t * chr_config_do_init( chr_t * pchr )
{
    CHR_REF  ichr;
    CAP_REF  icap;
    TEAM_REF loc_team;
    int      tnc, iteam, kursechance;

    cap_t * pcap;
    fvec3_t pos_tmp;

    if ( NULL == pchr ) return NULL;
    ichr = GET_INDEX_PCHR( pchr );

    // get the character profile pointer
    pcap = pro_get_pcap( pchr->spawn_data.profile );
    if ( NULL == pcap )
    {
        log_debug( "chr_config_do_init() - cannot initialize character.\n" );

        return NULL;
    }

    // get the character profile index
    icap = pro_get_icap( pchr->spawn_data.profile );

    // turn the character on here. you can't fail to spawn after this point.
    POBJ_ACTIVATE( pchr, pcap->name );

    // make a copy of the data in pchr->spawn_data.pos
    pos_tmp = pchr->spawn_data.pos;

    // download all the values from the character pchr->spawn_data.profile
    chr_download_cap( pchr, pcap );

    // Make sure the pchr->spawn_data.team is valid
    loc_team = pchr->spawn_data.team;
    iteam = REF_TO_INT( loc_team );
    iteam = CLIP( iteam, 0, TEAM_MAX );
    loc_team = ( TEAM_REF )iteam;

    // IMPORTANT!!!
    pchr->missilehandler = ichr;

    // Set up model stuff
    pchr->profile_ref   = pchr->spawn_data.profile;
    pchr->basemodel_ref = pchr->spawn_data.profile;

    // Kurse state
    if ( pcap->isitem )
    {
        IPair loc_rand = {1, 100};

        kursechance = pcap->kursechance;
        if ( cfg.difficulty >= GAME_HARD )                        kursechance *= 2.0f;  // Hard mode doubles chance for Kurses
        if ( cfg.difficulty < GAME_NORMAL && kursechance != 100 ) kursechance *= 0.5f;  // Easy mode halves chance for Kurses
        pchr->iskursed = ( generate_irand_pair( loc_rand ) <= kursechance );
    }

    // AI stuff
    ai_state_spawn( &( pchr->ai ), ichr, pchr->profile_ref, TeamStack.lst[loc_team].morale );

    // Team stuff
    pchr->team     = loc_team;
    pchr->baseteam = loc_team;
    if ( !pchr->invictus )  TeamStack.lst[loc_team].morale++;

    // Firstborn becomes the leader
    if ( TeamStack.lst[loc_team].leader == NOLEADER )
    {
        TeamStack.lst[loc_team].leader = ichr;
    }

    // Skin
    if ( NO_SKIN_OVERRIDE != pcap->skin_override )
    {
        // override the value passed into the function from spawn.txt
        // with the value from the expansion in data.txt
        pchr->spawn_data.skin = pchr->skin;
    }
    if ( pchr->spawn_data.skin >= ProList.lst[pchr->spawn_data.profile].skins )
    {
        // place this here so that the random number generator advances
        // no matter the state of ProList.lst[pchr->spawn_data.profile].skins... Eases
        // possible synch problems with other systems?
        int irand = RANDIE;

        pchr->spawn_data.skin = 0;
        if ( 0 != ProList.lst[pchr->spawn_data.profile].skins )
        {
            pchr->spawn_data.skin = irand % ProList.lst[pchr->spawn_data.profile].skins;
        }
    }
    pchr->skin = pchr->spawn_data.skin;

    // fix the pchr->spawn_data.skin-related parameters, in case there was some funy business with overriding
    // the pchr->spawn_data.skin from the data.txt file
    if ( pchr->spawn_data.skin != pchr->skin )
    {
        pchr->skin = pchr->spawn_data.skin;

        pchr->defense = pcap->defense[pchr->skin];
        for ( tnc = 0; tnc < DAMAGE_COUNT; tnc++ )
        {
            pchr->damage_modifier[tnc] = pcap->damage_modifier[tnc][pchr->skin];
        }

        chr_set_maxaccel( pchr, pcap->maxaccel[pchr->skin] );
    }

    // override the default behavior for an "easy" game
    if ( cfg.difficulty < GAME_NORMAL )
    {
        pchr->life = pchr->lifemax;
        pchr->mana = pchr->manamax;
    }

    // Character size and bumping
    pchr->fat_goto      = pchr->fat;
    pchr->fat_goto_time = 0;

    // grab all of the environment information
    chr_get_environment( pchr );

    chr_set_pos( pchr, pos_tmp.v );

    pchr->pos_stt  = pos_tmp;
    pchr->pos_old  = pos_tmp;

    pchr->ori.facing_z     = pchr->spawn_data.facing;
    pchr->ori_old.facing_z = pchr->ori.facing_z;

    // Name the character
    if ( CSTR_END == pchr->spawn_data.name[0] )
    {
        // Generate a random pchr->spawn_data.name
        snprintf( pchr->Name, SDL_arraysize( pchr->Name ), "%s", pro_create_chop( pchr->spawn_data.profile ) );
    }
    else
    {
        // A pchr->spawn_data.name has been given
        tnc = 0;

        while ( tnc < MAXCAPNAMESIZE - 1 )
        {
            pchr->Name[tnc] = pchr->spawn_data.name[tnc];
            tnc++;
        }

        pchr->Name[tnc] = CSTR_END;
    }

    // Particle attachments
    for ( tnc = 0; tnc < pcap->attachedprt_amount; tnc++ )
    {
        spawn_one_particle( pchr->pos, pchr->ori.facing_z, pchr->profile_ref, pcap->attachedprt_lpip,
                            ichr, GRIP_LAST + tnc, pchr->team, ichr, ( PRT_REF )MAX_PRT, tnc, ( CHR_REF )MAX_CHR );
    }

    // is the object part of a shop's inventory?
    if ( pchr->isitem )
    {
        SHOP_REF ishop;

        // Items that are spawned inside shop passages are more expensive than normal
        pchr->isshopitem = bfalse;
        for ( ishop = 0; ishop < ShopStack.count; ishop++ )
        {
            // Make sure the owner is not dead
            if ( SHOP_NOOWNER == ShopStack.lst[ishop].owner ) continue;

            if ( object_is_in_passage( ShopStack.lst[ishop].passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
            {
                pchr->isshopitem = btrue;               // Full value
                pchr->iskursed   = bfalse;              // Shop items are never kursed
                pchr->nameknown  = btrue;
                break;
            }
        }
    }

    /// ZF@> override the shopitem flag if the item is known to be valuable
    /// BB@> this prevents (essentially) all books from being able to be burned
    //if ( pcap->isvaluable )
    //{
    //    pchr->isshopitem = btrue;
    //}

    // initalize the character instance
    chr_instance_spawn( &( pchr->inst ), pchr->spawn_data.profile, pchr->spawn_data.skin );
    chr_update_matrix( pchr, btrue );

    // determine whether the object is hidden
    chr_update_hide( pchr );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, btrue );

#if defined(_DEBUG) && defined(DEBUG_WAYPOINTS)
    if ( DEFINED_CHR( pchr->attachedto ) && CHR_INFINITE_WEIGHT != pchr->phys.weight && !pchr->safe_valid )
    {
        log_warning( "spawn_one_character() - \n\tinitial spawn position <%f,%f> is \"inside\" a wall. Wall normal is <%f,%f>\n",
                     pchr->pos.x, pchr->pos.y, nrm.x, nrm.y );
    }
#endif

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_config_do_active( chr_t * pchr )
{
    cap_t * pcap;
    int     ripand;
    CHR_REF ichr;
    float water_level = 0.0f;

    if ( NULL == pchr ) return pchr;
    ichr = GET_REF_PCHR( pchr );

    //then do status updates
    chr_update_hide( pchr );

    //Don't do items that are in inventory
    if ( pchr->pack.is_packed ) return pchr;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return pchr;

    water_level = water.layer[0].z + water.layer[0].amp;
    if ( cfg.twolayerwater_allowed )
    {
        int cnt;

        for ( cnt = 1; cnt < MAXWATERLAYER; cnt++ )
        {
            water_level = MAX( water_level, water.layer[cnt].z + water.layer[cnt].amp );
        }
    }

    // do the character interaction with water
    if ( !pchr->is_hidden && pchr->pos.z < water_level && ( 0 != mesh_test_fx( PMesh, pchr->onwhichgrid, MPDFX_WATER ) ) )
    {
        // do splash and ripple
        if ( !pchr->enviro.inwater )
        {
            // Splash
            fvec3_t vtmp = VECT3( pchr->pos.x, pchr->pos.y, water_level + RAISE );

            spawn_one_particle_global( vtmp, ATK_FRONT, PIP_SPLASH, 0 );

            if ( water.is_water )
            {
                SET_BIT( pchr->ai.alert, ALERTIF_INWATER );
            }
        }
        else
        {
            // Ripples
            if ( !INGAME_CHR( pchr->attachedto ) && pcap->ripple && pchr->pos.z + pchr->chr_min_cv.maxs[OCT_Z] + RIPPLETOLERANCE > water_level && pchr->pos.z + pchr->chr_min_cv.mins[OCT_Z] < water_level )
            {
                int ripple_suppression;

                // suppress ripples if we are far below the surface
                ripple_suppression = water_level - ( pchr->pos.z + pchr->chr_min_cv.maxs[OCT_Z] );
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

                if ( 0 == (( update_wld + pchr->obj_base.guid ) & ripand ) && pchr->pos.z < water_level && pchr->alive )
                {
                    fvec3_t   vtmp = VECT3( pchr->pos.x, pchr->pos.y, water_level );

                    spawn_one_particle_global( vtmp, ATK_FRONT, PIP_RIPPLE, 0 );
                }
            }

            if ( water.is_water && HAS_NO_BITS( update_wld, 7 ) )
            {
                pchr->jumpready = btrue;
                pchr->jumpnumber = 1;
            }
        }

        pchr->enviro.inwater  = btrue;
    }
    else
    {
        pchr->enviro.inwater = bfalse;
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
        pchr->dismount_object = ( CHR_REF )MAX_CHR;
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
            pchr->mana = MAX( 0, MIN( pchr->mana, pchr->manamax ) );

            pchr->life += liferegen;
            pchr->life = MAX( 1, MIN( pchr->life, pchr->lifemax ) );
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
    pchr->see_kurse_level  = MAX( pchr->see_kurse_level,  chr_get_skill( pchr, MAKE_IDSZ( 'C', 'K', 'U', 'R' ) ) );
    pchr->darkvision_level = MAX( pchr->darkvision_level, chr_get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) ) );

    return pchr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
chr_t * chr_config_construct( chr_t * pchr, int max_iterations )
{
    int          iterations;
    obj_data_t * pbase;

    if ( NULL == pchr ) return NULL;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !pbase->allocated ) return NULL;

    // if the character is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( ego_object_constructing + 1 ) )
    {
        chr_t * tmp_chr = chr_config_deconstruct( pchr, max_iterations );
        if ( tmp_chr == pchr ) return NULL;
    }

    iterations = 0;
    while ( NULL != pchr && pbase->state <= ego_object_constructing && iterations < max_iterations )
    {
        chr_t * ptmp = chr_run_config( pchr );
        if ( ptmp != pchr ) return NULL;
        iterations++;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_config_initialize( chr_t * pchr, int max_iterations )
{
    int          iterations;
    obj_data_t * pbase;

    if ( NULL == pchr )  return NULL;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !pbase->allocated ) return NULL;

    // if the character is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( ego_object_initializing + 1 ) )
    {
        chr_t * tmp_chr = chr_config_deconstruct( pchr, max_iterations );
        if ( tmp_chr == pchr ) return NULL;
    }

    iterations = 0;
    while ( NULL != pchr && pbase->state <= ego_object_initializing && iterations < max_iterations )
    {
        chr_t * ptmp = chr_run_config( pchr );
        if ( ptmp != pchr ) return NULL;
        iterations++;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_config_activate( chr_t * pchr, int max_iterations )
{
    int          iterations;
    obj_data_t * pbase;

    if ( NULL == pchr ) return NULL;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !pbase->allocated ) return NULL;

    // if the character is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( ego_object_active + 1 ) )
    {
        chr_t * tmp_chr = chr_config_deconstruct( pchr, max_iterations );
        if ( tmp_chr == pchr ) return NULL;
    }

    iterations = 0;
    while ( NULL != pchr && pbase->state < ego_object_active && iterations < max_iterations )
    {
        chr_t * ptmp = chr_run_config( pchr );
        if ( ptmp != pchr ) return NULL;
        iterations++;
    }

    EGOBOO_ASSERT( pbase->state == ego_object_active );
    if ( pbase->state == ego_object_active )
    {
        ChrList_add_used( GET_INDEX_PCHR( pchr ) );
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_config_deinitialize( chr_t * pchr, int max_iterations )
{
    int          iterations;
    obj_data_t * pbase;

    if ( NULL == pchr ) return NULL;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !pbase->allocated ) return NULL;

    // if the character is already beyond this stage, deinitialize it
    if ( pbase->state > ( int )( ego_object_deinitializing + 1 ) )
    {
        return pchr;
    }
    else if ( pbase->state < ego_object_deinitializing )
    {
        pbase->state = ego_object_deinitializing;
    }

    iterations = 0;
    while ( NULL != pchr && pbase->state <= ego_object_deinitializing && iterations < max_iterations )
    {
        chr_t * ptmp = chr_run_config( pchr );
        if ( ptmp != pchr ) return NULL;
        iterations++;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_config_deconstruct( chr_t * pchr, int max_iterations )
{
    int          iterations;
    obj_data_t * pbase;

    if ( NULL == pchr ) return NULL;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !pbase->allocated ) return NULL;

    // if the character is already beyond this stage, do nothing
    if ( pbase->state > ( int )( ego_object_destructing + 1 ) )
    {
        return pchr;
    }
    else if ( pbase->state < ego_object_deinitializing )
    {
        // make sure that you deinitialize before destructing
        pbase->state = ego_object_deinitializing;
    }

    iterations = 0;
    while ( NULL != pchr && pbase->state <= ego_object_destructing && iterations < max_iterations )
    {
        chr_t * ptmp = chr_run_config( pchr );
        if ( ptmp != pchr ) return NULL;
        iterations++;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
chr_t * chr_run_config( chr_t * pchr )
{
    obj_data_t * pbase;

    if ( NULL == pchr ) return NULL;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !pbase->allocated ) return NULL;

    // set the object to deinitialize if it is not "dangerous" and if was requested
    if ( pbase->kill_me )
    {
        if ( pbase->state > ego_object_constructing && pbase->state < ego_object_deinitializing )
        {
            pbase->state = ego_object_deinitializing;
        }

        pbase->kill_me = bfalse;
    }

    switch ( pbase->state )
    {
        default:
        case ego_object_invalid:
            pchr = NULL;
            break;

        case ego_object_constructing:
            pchr = chr_config_ctor( pchr );
            break;

        case ego_object_initializing:
            pchr = chr_config_init( pchr );
            break;

        case ego_object_active:
            pchr = chr_config_active( pchr );
            break;

        case ego_object_deinitializing:
            pchr = chr_config_deinit( pchr );
            break;

        case ego_object_destructing:
            pchr = chr_config_dtor( pchr );
            break;

        case ego_object_waiting:
        case ego_object_terminated:
            /* do nothing */
            break;
    }

    if ( NULL == pchr )
    {
        pbase->update_guid = INVALID_UPDATE_GUID;
    }
    else if ( ego_object_active == pbase->state )
    {
        pbase->update_guid = ChrList.update_guid;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_config_ctor( chr_t * pchr )
{
    /// @details BB@> initialize the character data to safe values
    ///     since we use memset(..., 0, ...), all = 0, = false, and = 0.0f
    ///     statements are redundant

    obj_data_t * pbase;

    // grab the base object
    if ( NULL == pchr ) return NULL;
    pbase = POBJ_GET_PBASE( pchr );

    // if we aren't in the correct state, abort.
    if ( !STATE_CONSTRUCTING_PBASE( pbase ) ) return pchr;

    pchr = chr_ctor( pchr );
    if ( NULL == pchr ) return pchr;

    // we are done constructing. move on to initializing.
    pchr->obj_base.state = ego_object_initializing;

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_config_init( chr_t * pchr )
{
    obj_data_t * pbase;

    if ( NULL == pchr ) return NULL;
    pbase = POBJ_GET_PBASE( pchr );

    if ( !STATE_INITIALIZING_PBASE( pbase ) ) return pchr;

    pchr = chr_config_do_init( pchr );
    if ( NULL == pchr ) return NULL;

    if ( 0 == chr_loop_depth )
    {
        pchr->obj_base.on = btrue;
    }
    else
    {
        ChrList_add_activation( GET_INDEX_PPRT( pchr ) );
    }

    pbase->state = ego_object_active;

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_config_active( chr_t * pchr )
{
    // there's nothing to configure if the object is active...

    obj_data_t * pbase;

    if ( NULL == pchr ) return NULL;

    pbase = POBJ_GET_PBASE( pchr );

    if ( !pbase->allocated ) return NULL;
    if ( !STATE_ACTIVE_PBASE( pbase ) ) return pchr;

    POBJ_END_SPAWN( pchr );

    pchr = chr_config_do_active( pchr );

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_config_deinit( chr_t * pchr )
{
    /// @details BB@> deinitialize the character data

    obj_data_t * pbase;

    if ( NULL == pchr ) return NULL;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !STATE_DEINITIALIZING_PBASE( pbase ) ) return pchr;

    POBJ_END_SPAWN( pchr );

    pbase->state = ego_object_destructing;
    pbase->on    = bfalse;

    return pchr;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_config_dtor( chr_t * pchr )
{
    /// @details BB@> deinitialize the character data

    obj_data_t * pbase;

    if ( NULL == pchr ) return NULL;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !STATE_DESTRUCTING_PBASE( pbase ) ) return pchr;

    POBJ_END_SPAWN( pchr );

    return chr_dtor( pchr );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF spawn_one_character( fvec3_t pos, const PRO_REF profile, const TEAM_REF team,
                             Uint8 skin, FACING_T facing, const char *name, const CHR_REF override )
{
    /// @details ZZ@> This function spawns a character and returns the character's index number
    ///               if it worked, MAX_CHR otherwise

    CHR_REF   ichr;
    chr_t   * pchr;
    cap_t   * pcap;
    pro_t   * ppro;

    // fix a "bad" name
    if ( NULL == name ) name = "";

    if ( profile >= MAX_PROFILE )
    {
        log_warning( "spawn_one_character() - profile value too large %d out of %d\n", REF_TO_INT( profile ), MAX_PROFILE );
        return ( CHR_REF )MAX_CHR;
    }

    if ( !LOADED_PRO( profile ) )
    {
        if ( profile > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "spawn_one_character() - trying to spawn using invalid profile %d\n", REF_TO_INT( profile ) );
        }
        return ( CHR_REF )MAX_CHR;
    }
    ppro = ProList.lst + profile;

    if ( !LOADED_CAP( ppro->icap ) )
    {
        log_debug( "spawn_one_character() - invalid character profile %d\n", ppro->icap );
        return ( CHR_REF )MAX_CHR;
    }
    pcap = CapStack.lst + ppro->icap;

    // count all the requests for this character type
    pcap->request_count++;

    // allocate a new character
    ichr = ChrList_allocate( override );
    if ( !DEFINED_CHR( ichr ) )
    {
        log_warning( "spawn_one_character() - failed to spawn character (invalid index number %d?)\n", REF_TO_INT( ichr ) );
        return ( CHR_REF )MAX_CHR;
    }
    pchr = ChrList.lst + ichr;

    POBJ_BEGIN_SPAWN( pchr );

    // just set the spawn info
    pchr->spawn_data.pos      = pos;
    pchr->spawn_data.profile  = profile;
    pchr->spawn_data.team     = team;
    pchr->spawn_data.skin     = skin;
    pchr->spawn_data.facing   = facing;
    strncpy( pchr->spawn_data.name, name, SDL_arraysize( pchr->spawn_data.name ) );
    pchr->spawn_data.override = override;

    // actually force the character to spawn
    pchr = chr_config_activate( pchr, 100 );

    // count all the successful spawns of this character
    if ( NULL != pchr )
    {
        POBJ_END_SPAWN( pchr );
        pcap->create_count++;
    }

#if defined(DEBUG_OBJECT_SPAWN) && defined(_DEBUG)
    {
        CAP_REF icap = pro_get_icap( profile );
        log_debug( "spawn_one_character() - slot: %i, index: %i, name: %s, class: %s\n", REF_TO_INT( profile ), REF_TO_INT( ichr ), name, CapStack.lst[icap].classname );
    }
#endif

    return ichr;
}

//--------------------------------------------------------------------------------------------
void respawn_character( const CHR_REF character )
{
    /// @details ZZ@> This function respawns a character

    int old_attached_prt_count, new_attached_prt_count;

    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( character ) || ChrList.lst[character].alive ) return;
    pchr = ChrList.lst + character;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return;

    old_attached_prt_count = number_of_attached_particles( character );

    spawn_poof( character, pchr->profile_ref );
    disaffirm_attached_particles( character );

    pchr->alive = btrue;
    pchr->bore_timer = BORETIME;
    pchr->careful_timer = CAREFULTIME;
    pchr->life = pchr->lifemax;
    pchr->mana = pchr->manamax;
    chr_set_pos( pchr, pchr->pos_stt.v );
    pchr->vel.x = 0;
    pchr->vel.y = 0;
    pchr->vel.z = 0;
    pchr->team = pchr->baseteam;
    pchr->canbecrushed = bfalse;
    pchr->ori.map_facing_y = MAP_TURN_OFFSET;  // These two mean on level surface
    pchr->ori.map_facing_x = MAP_TURN_OFFSET;
    if ( NOLEADER == TeamStack.lst[pchr->team].leader )  TeamStack.lst[pchr->team].leader = character;
    if ( !pchr->invictus )  TeamStack.lst[pchr->baseteam].morale++;

    // start the character out in the "dance" animation
    chr_start_anim( pchr, ACTION_DA, btrue, btrue );

    // reset all of the bump size information
    {
        float old_fat = pchr->fat;
        chr_init_size( pchr, pcap );
        chr_set_fat( pchr, old_fat );
    }

    pchr->platform        = pcap->platform;
    pchr->canuseplatforms = pcap->canuseplatforms;
    pchr->flyheight       = pcap->flyheight;
    pchr->phys.bumpdampen = pcap->bumpdampen;

    pchr->ai.alert = ALERTIF_CLEANEDUP;
    pchr->ai.target = character;
    pchr->ai.timer  = 0;

    pchr->grog_timer = 0;
    pchr->daze_timer = 0;

    // Let worn items come back
    PACK_BEGIN_LOOP( ipacked, pchr->pack.next )
    {
        if ( INGAME_CHR( ipacked ) && ChrList.lst[ipacked].isequipped )
        {
            ChrList.lst[ipacked].isequipped = bfalse;
            SET_BIT( chr_get_pai( ipacked )->alert, ALERTIF_PUTAWAY ); // same as ALERTIF_ATLASTWAYPOINT
        }
    }
    PACK_END_LOOP( ipacked );

    // re-initialize the instance
    chr_instance_spawn( &( pchr->inst ), pchr->profile_ref, pchr->skin );
    chr_update_matrix( pchr, btrue );

    // determine whether the object is hidden
    chr_update_hide( pchr );

    if ( !pchr->is_hidden )
    {
        reaffirm_attached_particles( character );
        new_attached_prt_count = number_of_attached_particles( character );
    }

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, btrue );
}

//--------------------------------------------------------------------------------------------
int chr_change_skin( const CHR_REF character, int skin )
{
    chr_t * pchr;
    pro_t * ppro;
    mad_t * pmad;
    chr_instance_t * pinst;
    TX_REF new_texture = TX_WATER_TOP;

    if ( !INGAME_CHR( character ) ) return 0;
    pchr  = ChrList.lst + character;
    pinst = &( pchr->inst );

    ppro = chr_get_ppro( character );

    pmad = pro_get_pmad( pchr->profile_ref );
    if ( NULL == pmad )
    {
        // make sure that the instance has a valid imad
        if ( NULL != ppro && !LOADED_MAD( pinst->imad ) )
        {
            if ( chr_instance_set_mad( pinst, ppro->imad ) )
            {
                chr_update_collision_size( pchr, btrue );
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
        new_texture = ( TX_REF )TX_WATER_TOP;

        // do the best we can to change the skin
        if ( NULL == ppro || 0 == ppro->skins )
        {
            ppro->skins = 1;
            ppro->tex_ref[0] = TX_WATER_TOP;

            skin  = 0;
            new_texture = TX_WATER_TOP;
        }
        else
        {
            if ( skin > ppro->skins )
            {
                skin = 0;
            }

            new_texture = ppro->tex_ref[skin];
        }

        pchr->skin = skin;
    }

    chr_instance_set_texture( pinst, new_texture );

    // If the we are respawning a player, then the camera needs to be reset
    // if ( VALID_PLA( pchr->is_which_player ) )
    // {
    //     camera_reset_target( PCamera, PMesh );
    // }

    return pchr->skin;
}

//--------------------------------------------------------------------------------------------
int change_armor( const CHR_REF character, int skin )
{
    /// @details ZZ@> This function changes the armor of the character

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;

    int     iTmp;
    cap_t * pcap;
    chr_t * pchr;

    if ( !INGAME_CHR( character ) ) return 0;
    pchr = ChrList.lst + character;

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Remove armor enchantments
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        enchant_remove_set( ienc_now, SETSLASHMODIFIER );
        enchant_remove_set( ienc_now, SETCRUSHMODIFIER );
        enchant_remove_set( ienc_now, SETPOKEMODIFIER );
        enchant_remove_set( ienc_now, SETHOLYMODIFIER );
        enchant_remove_set( ienc_now, SETEVILMODIFIER );
        enchant_remove_set( ienc_now, SETFIREMODIFIER );
        enchant_remove_set( ienc_now, SETICEMODIFIER );
        enchant_remove_set( ienc_now, SETZAPMODIFIER );

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    // Change the skin
    pcap = chr_get_pcap( character );
    skin = chr_change_skin( character, skin );

    // Change stats associated with skin
    pchr->defense = pcap->defense[skin];

    for ( iTmp = 0; iTmp < DAMAGE_COUNT; iTmp++ )
    {
        pchr->damage_modifier[iTmp] = pcap->damage_modifier[iTmp][skin];
    }

    // set the character's maximum acceleration
    chr_set_maxaccel( pchr, pcap->maxaccel[skin] );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Reset armor enchantments
    /// @todo These should really be done in reverse order ( Start with last enchant ), but
    /// I don't care at this point !!!BAD!!!
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        PRO_REF ipro = enc_get_ipro( ienc_now );

        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        if ( LOADED_PRO( ipro ) )
        {
            EVE_REF ieve = pro_get_ieve( ipro );

            enchant_apply_set( ienc_now, SETSLASHMODIFIER, ipro );
            enchant_apply_set( ienc_now, SETCRUSHMODIFIER, ipro );
            enchant_apply_set( ienc_now, SETPOKEMODIFIER,  ipro );
            enchant_apply_set( ienc_now, SETHOLYMODIFIER,  ipro );
            enchant_apply_set( ienc_now, SETEVILMODIFIER,  ipro );
            enchant_apply_set( ienc_now, SETFIREMODIFIER,  ipro );
            enchant_apply_set( ienc_now, SETICEMODIFIER,   ipro );
            enchant_apply_set( ienc_now, SETZAPMODIFIER,   ipro );

            enchant_apply_add( ienc_now, ADDACCEL,         ieve );
            enchant_apply_add( ienc_now, ADDDEFENSE,       ieve );
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    return skin;
}

//--------------------------------------------------------------------------------------------
void change_character_full( const CHR_REF ichr, const PRO_REF profile, Uint8 skin, Uint8 leavewhich )
{
    /// @details ZF@> This function polymorphs a character permanently so that it can be exported properly
    /// A character turned into a frog with this function will also export as a frog!

    MAD_REF imad_old, imad_new;

    if ( !LOADED_PRO( profile ) ) return;

    imad_new = pro_get_imad( profile );
    if ( !LOADED_MAD( imad_new ) ) return;

    imad_old = chr_get_imad( ichr );
    if ( !LOADED_MAD( imad_old ) ) return;

    // copy the new name
    strncpy( MadStack.lst[imad_old].name, MadStack.lst[imad_new].name, SDL_arraysize( MadStack.lst[imad_old].name ) );

    // change their model
    change_character( ichr, profile, skin, leavewhich );

    // set the base model to the new model, too
    ChrList.lst[ichr].basemodel_ref = profile;
}

//--------------------------------------------------------------------------------------------
bool_t set_weapongrip( const CHR_REF iitem, const CHR_REF iholder, Uint16 vrt_off )
{
    int i;

    bool_t needs_update;
    Uint16 grip_verts[GRIP_VERTS];

    matrix_cache_t * mcache;
    chr_t * pitem;

    needs_update = bfalse;

    if ( !INGAME_CHR( iitem ) ) return bfalse;
    pitem = ChrList.lst + iitem;
    mcache = &( pitem->inst.matrix_cache );

    // is the item attached to this valid holder?
    if ( pitem->attachedto != iholder ) return bfalse;

    needs_update  = btrue;

    if ( GRIP_VERTS == get_grip_verts( grip_verts, iholder, vrt_off ) )
    {
        //---- detect any changes in the matrix_cache data

        needs_update  = bfalse;

        if ( iholder != mcache->grip_chr || pitem->attachedto != iholder )
        {
            needs_update  = btrue;
        }

        if ( pitem->inwhich_slot != mcache->grip_slot )
        {
            needs_update  = btrue;
        }

        // check to see if any of the
        for ( i = 0; i < GRIP_VERTS; i++ )
        {
            if ( grip_verts[i] != mcache->grip_verts[i] )
            {
                needs_update = btrue;
                break;
            }
        }
    }

    if ( needs_update )
    {
        // cannot create the matrix, therefore the current matrix must be invalid
        mcache->matrix_valid = bfalse;

        mcache->grip_chr  = iholder;
        mcache->grip_slot = pitem->inwhich_slot;

        for ( i = 0; i < GRIP_VERTS; i++ )
        {
            mcache->grip_verts[i] = grip_verts[i];
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void change_character( const CHR_REF ichr, const PRO_REF profile_new, Uint8 skin, Uint8 leavewhich )
{
    /// @details ZZ@> This function polymorphs a character, changing stats, dropping weapons

    int tnc;
    CHR_REF item_ref, item;
    chr_t * pchr;

    pro_t * pobj_new;
    cap_t * pcap_new;
    mad_t * pmad_new;

    int old_attached_prt_count, new_attached_prt_count;

    if ( !LOADED_PRO( profile_new ) || !INGAME_CHR( ichr ) ) return;
    pchr = ChrList.lst + ichr;

    old_attached_prt_count = number_of_attached_particles( ichr );

    if ( !LOADED_PRO( profile_new ) ) return;
    pobj_new = ProList.lst + profile_new;

    pcap_new = pro_get_pcap( profile_new );
    pmad_new = pro_get_pmad( profile_new );

    // Drop left weapon
    item_ref = pchr->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( item_ref ) && ( !pcap_new->slotvalid[SLOT_LEFT] || pcap_new->ismount ) )
    {
        detach_character_from_mount( item_ref, btrue, btrue );
        detach_character_from_platform( ChrList.lst + item_ref );

        if ( pchr->ismount )
        {
            fvec3_t tmp_pos;

            ChrList.lst[item_ref].vel.z    = DISMOUNTZVEL;
            ChrList.lst[item_ref].jump_timer = JUMPDELAY;

            tmp_pos = chr_get_pos( ChrList.lst + item_ref );
            tmp_pos.z += DISMOUNTZVEL;
            chr_set_pos( ChrList.lst + item_ref, tmp_pos.v );
        }
    }

    // Drop right weapon
    item_ref = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( item_ref ) && !pcap_new->slotvalid[SLOT_RIGHT] )
    {
        detach_character_from_mount( item_ref, btrue, btrue );
        detach_character_from_platform( ChrList.lst + item_ref );

        if ( pchr->ismount )
        {
            fvec3_t tmp_pos;

            ChrList.lst[item_ref].vel.z    = DISMOUNTZVEL;
            ChrList.lst[item_ref].jump_timer = JUMPDELAY;

            tmp_pos = chr_get_pos( ChrList.lst + item_ref );
            tmp_pos.z += DISMOUNTZVEL;
            chr_set_pos( ChrList.lst + item_ref, tmp_pos.v );
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
        if ( MAX_ENC != pchr->firstenchant )
        {
            ENC_REF ienc_now, ienc_nxt;
            size_t  ienc_count;

            ienc_now = EncList.lst[pchr->firstenchant].nextenchant_ref;
            ienc_count = 0;
            while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
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
    pchr->stoppedby = pcap_new->stoppedby;
    pchr->life_heal  = pcap_new->life_heal;
    pchr->manacost  = pcap_new->manacost;

    // Ammo
    pchr->ammomax = pcap_new->ammomax;
    pchr->ammo    = pcap_new->ammo;

    // Gender
    if ( pcap_new->gender != GENDER_RANDOM )  // GENDER_RANDOM means keep old gender
    {
        pchr->gender = pcap_new->gender;
    }

    // Sound effects
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pchr->sound_index[tnc] = pcap_new->sound_index[tnc];
    }

    // AI stuff
    chr_set_ai_state( pchr, 0 );
    pchr->ai.type           = pobj_new->iai;
    pchr->ai.timer          = 0;
    pchr->turnmode          = TURNMODE_VELOCITY;

    latch_init( &( pchr->latch ) );

    // Flags
    pchr->stickybutt      = pcap_new->stickybutt;
    pchr->openstuff       = pcap_new->canopenstuff;
    pchr->transferblend   = pcap_new->transferblend;
    pchr->platform        = pcap_new->platform;
    pchr->canuseplatforms = pcap_new->canuseplatforms;
    pchr->isitem          = pcap_new->isitem;
    pchr->invictus        = pcap_new->invictus;
    pchr->ismount         = pcap_new->ismount;
    pchr->cangrabmoney    = pcap_new->cangrabmoney;
    pchr->jump_timer        = JUMPDELAY;
    pchr->alpha_base       = pcap_new->alpha;
    pchr->light_base       = pcap_new->light;

    // change the skillz, too, jack!
    idsz_map_copy( pcap_new->skills, SDL_arraysize( pcap_new->skills ), pchr->skills );
    pchr->darkvision_level = chr_get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
    pchr->see_invisible_level = pcap_new->see_invisible_level;

    /// @note BB@> changing this could be disasterous, in case you can't un-morph youself???
    /// pchr->canusearcane          = pcap_new->canusearcane;
    /// @note ZF@> No, we want this, I have specifically scripted morph books to handle unmorphing
    /// even if you cannot cast arcane spells. Some morph spells specifically morph the player
    /// into a fighter or a tech user, but as a balancing factor prevents other spellcasting.

    // Character size and bumping
    // set the character size so that the new model is the same size as the old model
    // the model will then morph its size to the correct size over time
    {
        float old_fat = pchr->fat;
        float new_fat;

        if ( 0.0f == pchr->bump.size )
        {
            new_fat = pcap_new->size;
        }
        else
        {
            new_fat = ( pcap_new->bump_size * pcap_new->size ) / pchr->bump.size;
        }

        // Spellbooks should stay the same size, even if their spell effect cause changes in size
        if ( pchr->profile_ref == SPELLBOOK ) new_fat = old_fat = 1.00f;

        // copy all the cap size info over, as normal
        chr_init_size( pchr, pcap_new );

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
    pchr->phys.bumpdampen     = pcap_new->bumpdampen;

    if ( CAP_INFINITE_WEIGHT == pcap_new->weight )
    {
        pchr->phys.weight = CHR_INFINITE_WEIGHT;
    }
    else
    {
        Uint32 itmp = pcap_new->weight * pchr->fat * pchr->fat * pchr->fat;
        pchr->phys.weight = MIN( itmp, CHR_MAX_WEIGHT );
    }

    // Image rendering
    pchr->uoffvel = pcap_new->uoffvel;
    pchr->voffvel = pcap_new->voffvel;

    // Movement
    pchr->anim_speed_sneak = pcap_new->anim_speed_sneak;
    pchr->anim_speed_walk  = pcap_new->anim_speed_walk;
    pchr->anim_speed_run   = pcap_new->anim_speed_run;

    // initialize the instance
    chr_instance_spawn( &( pchr->inst ), profile_new, skin );
    chr_update_matrix( pchr, btrue );

    // Action stuff that must be down after chr_instance_spawn()
    chr_instance_set_action_ready( &( pchr->inst ), bfalse );
    chr_instance_set_action_keep( &( pchr->inst ), bfalse );
    chr_instance_set_action_loop( &( pchr->inst ), bfalse );
    if ( pchr->alive )
    {
        chr_play_action( pchr, ACTION_DA, bfalse );
    }
    else
    {
        chr_play_action( pchr, ACTION_KA + generate_randmask( 0, 3 ), bfalse );
        chr_instance_set_action_keep( &( pchr->inst ), btrue );
    }

    // Set the skin after changing the model in chr_instance_spawn()
    change_armor( ichr, skin );

    // Must set the wepon grip AFTER the model is changed in chr_instance_spawn()
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        set_weapongrip( ichr, pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );
    }

    item = pchr->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( item ) )
    {
        EGOBOO_ASSERT( ChrList.lst[item].attachedto == ichr );
        set_weapongrip( item, ichr, GRIP_LEFT );
    }

    item = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( item ) )
    {
        EGOBOO_ASSERT( ChrList.lst[item].attachedto == ichr );
        set_weapongrip( item, ichr, GRIP_RIGHT );
    }

    // determine whether the object is hidden
    chr_update_hide( pchr );

    // Reaffirm them particles...
    pchr->reaffirm_damagetype = pcap_new->attachedprt_reaffirm_damagetype;
    //reaffirm_attached_particles( ichr );              /// @note ZF@> so that books dont burn when dropped
    new_attached_prt_count = number_of_attached_particles( ichr );

    ai_state_set_changed( &( pchr->ai ) );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, btrue );
}

//--------------------------------------------------------------------------------------------
bool_t cost_mana( const CHR_REF character, int amount, const CHR_REF killer )
{
    /// @details ZZ@> This function takes mana from a character ( or gives mana ),
    ///    and returns btrue if the character had enough to pay, or bfalse
    ///    otherwise. This can kill a character in hard mode.

    int mana_final;
    bool_t mana_paid;

    chr_t * pchr;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    mana_paid  = bfalse;
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
                kill_character( character, !INGAME_CHR( killer ) ? character : killer, bfalse );
            }

            mana_paid = btrue;
        }
    }
    else
    {
        int mana_surplus = 0;

        pchr->mana = mana_final;

        if ( mana_final > pchr->manamax )
        {
            mana_surplus = mana_final - pchr->manamax;
            pchr->mana   = pchr->manamax;
        }

        // allow surplus mana to go to health if you can channel?
        if ( pchr->canchannel && mana_surplus > 0 )
        {
            // use some factor, divide by 2
            heal_character( GET_REF_PCHR( pchr ), killer, mana_surplus << 1, btrue );
        }

        mana_paid = btrue;

    }

    return mana_paid;
}

//--------------------------------------------------------------------------------------------
void switch_team( const CHR_REF character, const TEAM_REF team )
{
    /// @details ZZ@> This function makes a character join another team...
    chr_t *pchr;

    if ( !INGAME_CHR( character ) || team >= TEAM_MAX || team < 0 ) return;
    pchr = ChrList.lst + character;

    // keep track of how many live ones we have on any team
    if ( !pchr->invictus )
    {
        if ( chr_get_pteam_base( character )->morale > 0 ) chr_get_pteam_base( character )->morale--;
        TeamStack.lst[team].morale++;
    }

    /*
    // change our current team if we are not a item or a mount

    /*
    // change our current team if we are not a item or a mount
    if (( !pchr->ismount || !INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) ) &&
        ( !pchr->isitem  || !INGAME_CHR( pchr->attachedto ) ) )
    {
        pchr->team = team;
    }
    */

    // actually change our team
    pchr->baseteam = team;
    pchr->team = team;

    //change our mount team as well
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        chr_t *pmount = ChrList.lst + pchr->attachedto;
        if ( pmount->ismount ) pmount->team = team;
    }
    //change our mount team as well
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        chr_t *pmount = ChrList.lst + pchr->attachedto;
        if ( pmount->ismount ) pmount->team = team;
    }

    // update the team of anything we are holding as well
    if ( INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
    {
        ChrList.lst[pchr->holdingwhich[SLOT_LEFT]].team = team;
    }
    if ( INGAME_CHR( pchr->holdingwhich[SLOT_RIGHT] ) )
    {
        ChrList.lst[pchr->holdingwhich[SLOT_RIGHT]].team = team;
    }
    // update the team of anything we are holding as well
    if ( INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
    {
        ChrList.lst[pchr->holdingwhich[SLOT_LEFT]].team = team;
    }
    if ( INGAME_CHR( pchr->holdingwhich[SLOT_RIGHT] ) )
    {
        ChrList.lst[pchr->holdingwhich[SLOT_RIGHT]].team = team;
    }

    // we are the new leader if there isn't one already
    if ( TeamStack.lst[team].leader == NOLEADER )
    {
        TeamStack.lst[team].leader = character;
    }
}

//--------------------------------------------------------------------------------------------
void issue_clean( const CHR_REF character )
{
    /// @details ZZ@> This function issues a clean up order to all teammates

    TEAM_REF team;

    if ( !INGAME_CHR( character ) ) return;

    team = chr_get_iteam( character );

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        if ( team != chr_get_iteam( cnt ) ) continue;

        if ( !pchr->alive )
        {
            pchr->ai.timer  = update_wld + 2;  // Don't let it think too much...
        }

        SET_BIT( pchr->ai.alert, ALERTIF_CLEANEDUP );
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
int restock_ammo( const CHR_REF character, IDSZ idsz )
{
    /// @details ZZ@> This function restocks the characters ammo, if it needs ammo and if
    ///    either its parent or type idsz match the given idsz.  This
    ///    function returns the amount of ammo given.

    int amount;

    chr_t * pchr;

    if ( !INGAME_CHR( character ) ) return 0;
    pchr = ChrList.lst + character;

    amount = 0;
    if ( chr_is_type_idsz( character, idsz ) )
    {
        if ( ChrList.lst[character].ammo < ChrList.lst[character].ammomax )
        {
            amount = ChrList.lst[character].ammomax - ChrList.lst[character].ammo;
            ChrList.lst[character].ammo = ChrList.lst[character].ammomax;
        }
    }

    return amount;
}

//--------------------------------------------------------------------------------------------
int chr_get_skill( chr_t *pchr, IDSZ whichskill )
{
    /// @details ZF@> This returns the skill level for the specified skill or 0 if the character doesn't
    ///                  have the skill. Also checks the skill IDSZ.
    IDSZ_node_t *pskill;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

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
bool_t update_chr_darkvision( const CHR_REF character )
{
    /// @detalis BB@> as an offset to negative status effects like things like poisoning, a
    ///               character gains darkvision ability the more they are "poisoned".
    ///               True poisoning can be removed by [HEAL] and tints the character green

    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;

    eve_t * peve;
    int life_regen = 0;

    chr_t * pchr;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // grab the life loss due poison to determine how much darkvision a character has earned, he he he!
    // clean up the enchant list before doing anything
    ienc_now = pchr->firstenchant;
    ienc_count = 0;
    while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
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
        int tmp_level = ( 10 * -life_regen ) / pchr->lifemax;                        //Darkvision gained by poison
        int base_level = chr_get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );    //Natural darkvision

        //Use the better of the two darkvision abilities
        pchr->darkvision_level = MAX( base_level, tmp_level );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void update_all_characters()
{
    /// @details ZZ@> This function updates stats and such for every character

    CHR_REF ichr;

    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        chr_run_config( ChrList.lst + ichr );
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
void move_one_character_get_environment( chr_t * pchr )
{

    float   grid_level, water_level;
    chr_t * pplatform = NULL;

    chr_environment_t * penviro;

    if ( !ACTIVE_PCHR( pchr ) ) return;
    penviro = &( pchr->enviro );

    // determine if the character is standing on a platform
    pplatform = NULL;
    if ( INGAME_CHR( pchr->onwhichplatform_ref ) )
    {
        pplatform = ChrList.lst + pchr->onwhichplatform_ref;
    }

    //---- character "floor" level
    grid_level = get_mesh_level( PMesh, pchr->pos.x, pchr->pos.y, bfalse );
    water_level = get_mesh_level( PMesh, pchr->pos.x, pchr->pos.y, btrue );

    // chr_set_enviro_grid_level() sets up the reflection level and reflection matrix
    chr_set_enviro_grid_level( pchr, grid_level );

    penviro->grid_lerp  = ( pchr->pos.z - grid_level ) / PLATTOLERANCE;
    penviro->grid_lerp  = CLIP( penviro->grid_lerp, 0.0f, 1.0f );

    penviro->water_level = water_level;
    penviro->water_lerp  = ( pchr->pos.z - water_level ) / PLATTOLERANCE;
    penviro->water_lerp  = CLIP( penviro->water_lerp, 0.0f, 1.0f );

    // The actual level of the floor underneath the character.
    if ( NULL != pplatform )
    {
        penviro->floor_level = pplatform->pos.z + pplatform->chr_min_cv.maxs[OCT_Z];
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
        penviro->level = pplatform->pos.z + pplatform->chr_min_cv.maxs[OCT_Z];
    }

    //---- The flying height of the character, the maximum of tile level, platform level and water level
    if ( 0 != mesh_test_fx( PMesh, pchr->onwhichgrid, MPDFX_WATER ) )
    {
        penviro->fly_level = MAX( penviro->level, water.surface_level );
    }

    if ( penviro->fly_level < 0 )
    {
        penviro->fly_level = 0;  // fly above pits...
    }

    // set the zlerp after we have done everything to the particle's level we care to
    penviro->zlerp = ( pchr->pos.z - penviro->level ) / PLATTOLERANCE;
    penviro->zlerp = CLIP( penviro->zlerp, 0.0f, 1.0f );

    penviro->grounded = ( 0 == pchr->flyheight ) && ( penviro->zlerp < 0.25f );

    //---- the "twist" of the floor
    penviro->grid_twist = TWIST_FLAT;
    if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
    {
        penviro->grid_twist = PMesh->gmem.grid_list[pchr->onwhichgrid].twist;
    }

    // the "watery-ness" of whatever water might be here
    penviro->is_watery = water.is_water && penviro->inwater;
    penviro->is_slippy = !penviro->is_watery && ( 0 != mesh_test_fx( PMesh, pchr->onwhichgrid, MPDFX_SLIPPY ) );

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

        fvec3_t   platform_up;

        chr_getMatUp( pplatform, platform_up.v );
        fvec3_self_normalize( platform_up.v );

        penviro->traction = ABS( platform_up.z ) * ( 1.0f - penviro->zlerp ) + 0.25f * penviro->zlerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= 4.00f * hillslide * ( 1.0f - penviro->zlerp ) + 1.0f * penviro->zlerp;
        }
    }
    else if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
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
        if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) && penviro->is_slippy )
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
        pchr->jumpready = bfalse;
    }
    else
    {
        // Character is in the air
        pchr->jumpready = penviro->grounded;

        // Down jump timer
        if (( INGAME_CHR( pchr->attachedto ) || pchr->jumpready || pchr->jumpnumber > 0 ) && pchr->jump_timer > 0 ) pchr->jump_timer--;

        // Do ground hits
        if ( penviro->grounded && pchr->vel.z < -STOPBOUNCING && pchr->hitready )
        {
            SET_BIT( pchr->ai.alert, ALERTIF_HITGROUND );
            pchr->hitready = bfalse;
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
        fvec3_self_clear( penviro->floor_speed.v );
    }
    else
    {
        fvec3_base_copy( penviro->floor_speed.v, pplatform->vel.v );
    }

}

//--------------------------------------------------------------------------------------------
void move_one_character_do_floor_friction( chr_t * pchr )
{
    /// @details BB@> Friction is complicated when you want to have sliding characters :P

    float temp_friction_xy;
    fvec3_t   vup, floor_acc, fric, fric_floor;
    chr_environment_t * penviro;

    if ( !ACTIVE_PCHR( pchr ) ) return;
    penviro = &( pchr->enviro );

    if ( 0 != pchr->flyheight ) return;

    // assume the best
    fvec3_self_clear( floor_acc.v );
    temp_friction_xy = 1.0f;
    vup.x = 0.0f; vup.y = 0.0f; vup.z = 1.0f;

    // figure out the acceleration due to the current "floor"
    if ( INGAME_CHR( pchr->onwhichplatform_ref ) )
    {
        chr_t * pplat = ChrList.lst + pchr->onwhichplatform_ref;

        temp_friction_xy = platstick;

        chr_getMatUp( pplat, vup.v );
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

    floor_acc = fvec3_sub( penviro->floor_speed.v, pchr->vel.v );
    fvec3_self_scale( floor_acc.v, ( 1.0f - penviro->zlerp ) );

    // reduce the volountary acceleration peopendicular to the direction of motion?
    if ( fvec3_length_abs( floor_acc.v ) > 0.0f )
    {
        fvec3_t acc_para, acc_perp;
        fvec3_t   vfront;

        // get the direction of motion
        mat_getChrForward( pchr->inst.matrix.v, vfront.v );
        fvec3_self_normalize( vfront.v );

        // decompose the acceleration into parallel and perpendicular components
        fvec3_decompose( floor_acc.v, vfront.v, acc_para.v, acc_perp.v );

        // re-compose the acceleration with 1/2 of the perpendicular taken away
        floor_acc = fvec3_scale( acc_perp.v, 0.5f );
        fvec3_self_sum( floor_acc.v, acc_para.v );
    }

    // the first guess about the floor friction
    fric_floor = fvec3_scale( floor_acc.v, penviro->traction * ( 1.0f - temp_friction_xy ) );

    // the total "friction" with to the floor
    fric = fvec3_add( fric_floor.v, penviro->acc.v );

    // limit the friction to whatever is horizontal to the mesh
    if ( 1.0f == ABS( vup.z ) )
    {
        fric.z      = 0.0f;
        floor_acc.z = 0.0f;
    }
    else
    {
        fvec3_t acc_perp, acc_para;

        fvec3_decompose( fric.v, vup.v, acc_perp.v, acc_para.v );
        fric = acc_para;

        fvec3_decompose( floor_acc.v, vup.v, acc_perp.v, acc_para.v );
        floor_acc = acc_para;
    }

    // test to see if the player has any more friction left?
    penviro->is_slipping = ( ABS( fric.x ) + ABS( fric.y ) + ABS( fric.z ) > penviro->friction_hrz );

    if ( penviro->is_slipping )
    {
        penviro->traction *= 0.5f;
        temp_friction_xy  = SQRT( temp_friction_xy );

        // the first guess about the floor friction
        fric_floor = fvec3_scale( floor_acc.v, penviro->traction * ( 1.0f - temp_friction_xy ) );
    }

    //apply the floor friction
    fvec3_self_sum( pchr->vel.v, fric_floor.v );

    // Apply fluid friction from last time
    pchr->vel.x += -pchr->vel.x * ( 1.0f - penviro->fluid_friction_hrz );
    pchr->vel.y += -pchr->vel.y * ( 1.0f - penviro->fluid_friction_hrz );
    pchr->vel.z += -pchr->vel.z * ( 1.0f - penviro->fluid_friction_vrt );
}

//--------------------------------------------------------------------------------------------
void move_one_character_do_voluntary( chr_t * pchr )
{
    // do voluntary motion

    float dvx, dvy;
    float maxspeed;
    float dv2;
    float new_ax, new_ay;
    CHR_REF ichr;
    bool_t sneak_mode_active = bfalse;

    if ( !ACTIVE_PCHR( pchr ) ) return;

    ichr = GET_REF_PCHR( pchr );

    if ( !pchr->alive ) return;

    fvec2_base_copy( pchr->enviro.new_v.v, pchr->vel.v );

    if ( INGAME_CHR( pchr->attachedto ) ) return;

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
    maxspeed = pchr->maxaccel * airfriction / ( 1.0f - airfriction );

    sneak_mode_active = bfalse;
    if ( VALID_PLA( pchr->is_which_player ) )
    {
        // determine whether the user is hitting the "sneak button"
        player_t * ppla = PlaStack.lst + pchr->is_which_player;

        if ( HAS_SOME_BITS( ppla->device.bits, INPUT_BITS_KEYBOARD ) )
        {
            // use the shift keys to enter sneak mode
            sneak_mode_active = SDLKEYDOWN( SDLK_LSHIFT ) || SDLKEYDOWN( SDLK_RSHIFT );
        }
    }

    pchr->enviro.new_v.x = pchr->enviro.new_v.y = 0.0f;
    if ( ABS( dvx ) + ABS( dvy ) > 0.05f )
    {
        PLA_REF ipla = pchr->is_which_player;

        dv2 = dvx * dvx + dvy * dvy;

        if ( VALID_PLA( ipla ) )
        {
            player_t * ppla;
            bool_t sneak_mode_active;

            float dv = POW( dv2, 0.25f );

            ppla = PlaStack.lst + ipla;

            // determine whether the character is sneaking
            if ( !HAS_SOME_BITS( ppla->device.bits, INPUT_BITS_KEYBOARD ) )
            {
                sneak_mode_active = ( dv2 < 1.0f / 9.0f );
            }

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
    if ( INGAME_CHR( pchr->onwhichplatform_ref ) )
    {
        chr_t * pplat = ChrList.lst + pchr->onwhichplatform_ref;

        new_ax += ( pplat->vel.x + pchr->enviro.new_v.x - pchr->vel.x );
        new_ay += ( pplat->vel.y + pchr->enviro.new_v.y - pchr->vel.y );
    }
    else
    {
        new_ax += ( pchr->enviro.new_v.x - pchr->vel.x );
        new_ay += ( pchr->enviro.new_v.y - pchr->vel.y );
    }

    new_ax *= pchr->enviro.traction;
    new_ay *= pchr->enviro.traction;

    new_ax = CLIP( new_ax, -pchr->maxaccel, pchr->maxaccel );
    new_ay = CLIP( new_ay, -pchr->maxaccel, pchr->maxaccel );

    //Figure out how to turn around
    if ( 0 != pchr->maxaccel )
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
                            pchr->ori.facing_z += terp_dir( pchr->ori.facing_z, vec_to_facing( dvx , dvy ), 2 );
                        }
                        else
                        {
                            // AI turn slowly
                            pchr->ori.facing_z += terp_dir( pchr->ori.facing_z, vec_to_facing( dvx , dvy ), 8 );
                        }
                    }
                }
                break;

                // Get direction from the DESIRED change in velocity
            case TURNMODE_WATCH:
                {
                    if (( ABS( dvx ) > WATCHMIN || ABS( dvy ) > WATCHMIN ) )
                    {
                        pchr->ori.facing_z += terp_dir( pchr->ori.facing_z, vec_to_facing( dvx , dvy ), 8 );
                    }
                }
                break;

                // Face the target
            case TURNMODE_WATCHTARGET:
                {
                    if ( ichr != pchr->ai.target )
                    {
                        pchr->ori.facing_z += terp_dir( pchr->ori.facing_z, vec_to_facing( ChrList.lst[pchr->ai.target].pos.x - pchr->pos.x , ChrList.lst[pchr->ai.target].pos.y - pchr->pos.y ), 8 );
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

    if ( chr_get_framefx( pchr ) & MADFX_STOP )
    {
        new_ax = 0;
        new_ay = 0;
    }
    else
    {
        pchr->vel.x += new_ax;
        pchr->vel.y += new_ay;
    }
}

//--------------------------------------------------------------------------------------------
bool_t chr_do_latch_attack( chr_t * pchr, slot_t which_slot )
{
    chr_t * pweapon;
    cap_t * pweapon_cap;
    CHR_REF ichr, iweapon;
    MAD_REF imad;

    int    base_action, hand_action, action;
    bool_t action_valid, allowedtoattack;

    bool_t retval = bfalse;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    ichr = GET_REF_PCHR( pchr );

    imad = chr_get_imad( ichr );

    if ( which_slot < 0 || which_slot >= SLOT_COUNT ) return bfalse;

    // Which iweapon?
    iweapon = pchr->holdingwhich[which_slot];
    if ( !INGAME_CHR( iweapon ) )
    {
        // Unarmed means character itself is the iweapon
        iweapon = ichr;
    }
    pweapon     = ChrList.lst + iweapon;
    pweapon_cap = chr_get_pcap( iweapon );

    //No need to continue if we have an attack cooldown
    if ( 0 != pweapon->reload_timer ) return bfalse;

    // grab the iweapon's action
    base_action = pweapon_cap->weaponaction;
    hand_action = randomize_action( base_action, which_slot );

    // see if the character can play this action
    action       = mad_get_action_ref( imad, hand_action );
    action_valid = ( ACTION_COUNT != action );

    // Can it do it?
    allowedtoattack = btrue;

    // First check if reload time and action is okay
    if ( !action_valid )
    {
        allowedtoattack = bfalse;
    }
    else
    {
        // Then check if a skill is needed
        if ( pweapon_cap->needskillidtouse )
        {
            if ( !chr_get_skill( pchr, chr_get_idsz( iweapon, IDSZ_SKILL ) ) )
            {
                allowedtoattack = bfalse;
            }
        }
    }

    // Don't allow users with kursed weapon in the other hand to use longbows
    if ( allowedtoattack && ACTION_IS_TYPE( action, L ) )
    {
        CHR_REF test_weapon;
        test_weapon = pchr->holdingwhich[which_slot == SLOT_LEFT ? SLOT_RIGHT : SLOT_LEFT];
        if ( INGAME_CHR( test_weapon ) )
        {
            chr_t * weapon;
            weapon     = ChrList.lst + test_weapon;
            if ( weapon->iskursed ) allowedtoattack = bfalse;
        }
    }

    if ( !allowedtoattack )
    {
        // This character can't use this iweapon
        pweapon->reload_timer = 50;
        if ( pchr->StatusList_on || cfg.dev_mode )
        {
            // Tell the player that they can't use this iweapon
            debug_printf( "%s can't use this item...", chr_get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_CAPITAL ) );
        }
        return bfalse;
    }

    if ( ACTION_DA == action )
    {
        allowedtoattack = bfalse;
        if ( 0 == pweapon->reload_timer )
        {
            SET_BIT( pweapon->ai.alert, ALERTIF_USED );
        }
    }

    // deal with your mount (which could steal your attack)
    if ( allowedtoattack )
    {
        // Rearing mount
        CHR_REF mount = pchr->attachedto;

        if ( INGAME_CHR( mount ) )
        {
            chr_t * pmount = ChrList.lst + mount;
            cap_t * pmount_cap = chr_get_pcap( mount );

            // let the mount steal the rider's attack
            if ( !pmount_cap->ridercanattack ) allowedtoattack = bfalse;

            // can the mount do anything?
            if ( pmount->ismount && pmount->alive )
            {
                // can the mount be told what to do?
                if ( !VALID_PLA( pmount->is_which_player ) && pmount->inst.action_ready )
                {
                    if ( !ACTION_IS_TYPE( action, P ) || !pmount_cap->ridercanattack )
                    {
                        chr_play_action( pmount, generate_randmask( ACTION_UA, 1 ), bfalse );
                        SET_BIT( pmount->ai.alert, ALERTIF_USED );
                        pchr->ai.lastitemused = mount;

                        retval = btrue;
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
            // Check mana cost
            bool_t mana_paid = cost_mana( ichr, pweapon->manacost, iweapon );

            if ( mana_paid )
            {
                Uint32 action_madfx = 0;

                // Check life healing
                pchr->life += pweapon->life_heal;
                if ( pchr->life > pchr->lifemax )  pchr->life = pchr->lifemax;

                // randomize the action
                action = randomize_action( action, which_slot );

                // make sure it is valid
                action = mad_get_action_ref( imad, action );

                // grab the MADFX_* flags for this action
                action_madfx = mad_get_action_ref( imad, action );

                if ( ACTION_IS_TYPE( action, P ) )
                {
                    // we must set parry actions to be interrupted by anything
                    chr_play_action( pchr, action, btrue );
                }
                else
                {
                    float chr_dex = FP8_TO_INT( pchr->dexterity );

                    chr_play_action( pchr, action, bfalse );

                    // Make the weapon animate the attack as well as the character holding it
                    if ( HAS_NO_BITS( action, MADFX_ACTLEFT | MADFX_ACTRIGHT ) )
                    {
                        if ( iweapon != ichr )
                        {
                            // the attacking character has no bits in the animation telling it
                            // to use the weapon, so we play the animation here

                            // Make the iweapon attack too
                            chr_play_action( pweapon, ACTION_MJ, bfalse );
                        }
                    }

                    //Determine the attack speed (how fast we play the animation)
                    pchr->inst.rate = 0.25f;                       //base attack speed
                    pchr->inst.rate += chr_dex / 20;                    //+0.25f for every 5 dexterity

                    //Add some reload time as a true limit to attacks per second
                    //Dexterity decreases the reload time for all weapons. We could allow other stats like intelligence
                    //reduce reload time for spells or gonnes here.
                    if ( !pweapon_cap->attack_fast )
                    {
                        int base_reload_time = -chr_dex;
                        if ( ACTION_IS_TYPE( action, U ) ) base_reload_time += 50;          //Unarmed  (Fists)
                        else if ( ACTION_IS_TYPE( action, T ) ) base_reload_time += 45;     //Thrust   (Spear)
                        else if ( ACTION_IS_TYPE( action, C ) ) base_reload_time += 75;     //Chop     (Axe)
                        else if ( ACTION_IS_TYPE( action, S ) ) base_reload_time += 55;     //Slice    (Sword)
                        else if ( ACTION_IS_TYPE( action, B ) ) base_reload_time += 60;     //Bash     (Mace)
                        else if ( ACTION_IS_TYPE( action, L ) ) base_reload_time += 50;     //Longbow  (Longbow)
                        else if ( ACTION_IS_TYPE( action, X ) ) base_reload_time += 100;    //Xbow     (Crossbow)
                        else if ( ACTION_IS_TYPE( action, F ) ) base_reload_time += 50;     //Flinged  (Unused)

                        //it is possible to have so high dex to eliminate all reload time
                        if ( base_reload_time > 0 ) pweapon->reload_timer += base_reload_time;
                    }
                }

                // let everyone know what we did
                pchr->ai.lastitemused = iweapon;
                if ( iweapon == ichr || HAS_NO_BITS( action, MADFX_ACTLEFT | MADFX_ACTRIGHT ) )
                {
                    SET_BIT( pweapon->ai.alert, ALERTIF_USED );
                }

                retval = btrue;
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
bool_t chr_do_latch_button( chr_t * pchr )
{
    /// @details BB@> Character latches for generalized buttons

    CHR_REF ichr;
    ai_state_t * pai;

    CHR_REF item;
    bool_t attack_handled;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    ichr = GET_REF_PCHR( pchr );

    pai = &( pchr->ai );

    if ( !pchr->alive || 0 == pchr->latch.b ) return btrue;

    if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_JUMP ) && 0 == pchr->jump_timer )
    {
        int ijump;
        cap_t * pcap;

        //Jump from our mount
        if ( INGAME_CHR( pchr->attachedto ) )
        {
            fvec3_t tmp_pos;

            detach_character_from_mount( ichr, btrue, btrue );
            detach_character_from_platform( ChrList.lst + ichr );

            pchr->jump_timer = JUMPDELAY;
            if ( 0 != pchr->flyheight )
            {
                pchr->vel.z += DISMOUNTZVELFLY;
            }
            else
            {
                pchr->vel.z += DISMOUNTZVEL;
            }

            tmp_pos = chr_get_pos( pchr );
            tmp_pos.z += pchr->vel.z;
            chr_set_pos( pchr, tmp_pos.v );

            if ( pchr->jumpnumberreset != JUMPINFINITE && 0 != pchr->jumpnumber )
                pchr->jumpnumber--;

            // Play the jump sound
            pcap = pro_get_pcap( pchr->profile_ref );
            if ( NULL != pcap )
            {
                ijump = pro_get_pcap( pchr->profile_ref )->sound_index[SOUND_JUMP];
                if ( VALID_SND( ijump ) )
                {
                    sound_play_chunk( pchr->pos, chr_get_chunk_ptr( pchr, ijump ) );
                }
            }

        }

        //Normal jump
        else if ( 0 != pchr->jumpnumber && 0 == pchr->flyheight )
        {
            if ( 1 != pchr->jumpnumberreset || pchr->jumpready )
            {

                // Make the character jump
                pchr->hitready = btrue;
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

                pchr->jumpready = bfalse;
                if ( pchr->jumpnumberreset != JUMPINFINITE ) pchr->jumpnumber--;

                // Set to jump animation if not doing anything better
                if ( pchr->inst.action_ready )
                {
                    chr_play_action( pchr, ACTION_JA, btrue );
                }

                // Play the jump sound (Boing!)
                pcap = pro_get_pcap( pchr->profile_ref );
                if ( NULL != pcap )
                {
                    ijump = pcap->sound_index[SOUND_JUMP];
                    if ( VALID_SND( ijump ) )
                    {
                        sound_play_chunk( pchr->pos, chr_get_chunk_ptr( pchr, ijump ) );
                    }
                }
            }
        }

    }
    if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_ALTLEFT ) && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_ALTLEFT;

        pchr->reload_timer = GRABDELAY;
        if ( !INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
        {
            // Grab left
            chr_play_action( pchr, ACTION_ME, bfalse );
        }
        else
        {
            // Drop left
            chr_play_action( pchr, ACTION_MA, bfalse );
        }
    }
    if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_ALTRIGHT ) && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_ALTRIGHT;

        pchr->reload_timer = GRABDELAY;
        if ( !INGAME_CHR( pchr->holdingwhich[SLOT_RIGHT] ) )
        {
            // Grab right
            chr_play_action( pchr, ACTION_MF, bfalse );
        }
        else
        {
            // Drop right
            chr_play_action( pchr, ACTION_MB, bfalse );
        }
    }
    if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_PACKLEFT ) && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_PACKLEFT;

        pchr->reload_timer = PACKDELAY;
        item = pchr->holdingwhich[SLOT_LEFT];

        if ( INGAME_CHR( item ) )
        {
            chr_t * pitem = ChrList.lst + item;

            if (( pitem->iskursed || pro_get_pcap( pitem->profile_ref )->istoobig ) && !pro_get_pcap( pitem->profile_ref )->isequipment )
            {
                // The item couldn't be put away
                SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
                if ( VALID_PLA( pchr->is_which_player ) )
                {
                    if ( pro_get_pcap( pitem->profile_ref )->istoobig )
                    {
                        debug_printf( "%s is too big to be put away...", chr_get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
                    }
                    else if ( pitem->iskursed )
                    {
                        debug_printf( "%s is sticky...", chr_get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
                    }
                }
            }
            else
            {
                // Put the item into the pack
                inventory_add_item( item, ichr );
            }
        }
        else
        {
            // Get a new one out and put it in hand
            inventory_get_item( ichr, GRIP_LEFT, bfalse );
        }

        // Make it take a little time
        chr_play_action( pchr, ACTION_MG, bfalse );
    }
    if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_PACKRIGHT ) && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_PACKRIGHT;

        pchr->reload_timer = PACKDELAY;
        item = pchr->holdingwhich[SLOT_RIGHT];
        if ( INGAME_CHR( item ) )
        {
            chr_t * pitem     = ChrList.lst + item;
            cap_t * pitem_cap = chr_get_pcap( item );

            if (( pitem->iskursed || pitem_cap->istoobig ) && !pitem_cap->isequipment )
            {
                // The item couldn't be put away
                SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
                if ( VALID_PLA( pchr->is_which_player ) )
                {
                    if ( pitem_cap->istoobig )
                    {
                        debug_printf( "%s is too big to be put away...", chr_get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
                    }
                    else if ( pitem->iskursed )
                    {
                        debug_printf( "%s is sticky...", chr_get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
                    }
                }
            }
            else
            {
                // Put the item into the pack
                inventory_add_item( item, ichr );
            }
        }
        else
        {
            // Get a new one out and put it in hand
            inventory_get_item( ichr, GRIP_RIGHT, bfalse );
        }

        // Make it take a little time
        chr_play_action( pchr, ACTION_MG, bfalse );
    }

    // LATCHBUTTON_LEFT and LATCHBUTTON_RIGHT are mutually exclusive
    attack_handled = bfalse;
    if ( !attack_handled && HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_LEFT ) && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_LEFT;
        attack_handled = chr_do_latch_attack( pchr, SLOT_LEFT );
    }
    if ( !attack_handled && HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_RIGHT ) && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_RIGHT;

        attack_handled = chr_do_latch_attack( pchr, SLOT_RIGHT );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void move_one_character_do_z_motion( chr_t * pchr )
{
    if ( !ACTIVE_PCHR( pchr ) ) return;

    //---- do z acceleration
    if ( 0 != pchr->flyheight )
    {
        pchr->vel.z += ( pchr->enviro.fly_level + pchr->flyheight - pchr->pos.z ) * FLYDAMPEN;
    }
    else if (
        pchr->enviro.is_slippy && ( pchr->enviro.grid_twist != TWIST_FLAT ) &&
        ( CHR_INFINITE_WEIGHT != pchr->phys.weight )  && ( pchr->enviro.grid_lerp <= pchr->enviro.zlerp ) )
    {
        // Slippy hills make characters slide

        fvec3_t   gperp;    // gravity perpendicular to the mesh
        fvec3_t   gpara;    // gravity parallel      to the mesh (what pushes you)

        // RELATIVE TO THE GRID, since you might be riding a platform!
        float     loc_zlerp = pchr->enviro.grid_lerp;

        gpara.x = map_twistvel_x[pchr->enviro.grid_twist];
        gpara.y = map_twistvel_y[pchr->enviro.grid_twist];
        gpara.z = map_twistvel_z[pchr->enviro.grid_twist];

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
bool_t chr_update_safe_raw( chr_t * pchr )
{
    bool_t retval = bfalse;

    BIT_FIELD hit_a_wall;
    float pressure;

    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    hit_a_wall = chr_hit_wall( pchr, NULL, NULL, &pressure, NULL );
    if (( 0 == hit_a_wall ) && ( 0.0f == pressure ) )
    {
        pchr->safe_valid = btrue;
        pchr->safe_pos   = chr_get_pos( pchr );
        pchr->safe_time  = update_wld;
        pchr->safe_grid  = mesh_get_grid( PMesh, pchr->pos.x, pchr->pos.y );

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_update_safe( chr_t * pchr, bool_t force )
{
    Uint32 new_grid;
    bool_t retval = bfalse;
    bool_t needs_update = bfalse;

    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    if ( force || !pchr->safe_valid )
    {
        needs_update = btrue;
    }
    else
    {
        new_grid = mesh_get_grid( PMesh, pchr->pos.x, pchr->pos.y );

        if ( INVALID_TILE == new_grid )
        {
            if ( ABS( pchr->pos.x - pchr->safe_pos.x ) > GRID_FSIZE ||
                 ABS( pchr->pos.y - pchr->safe_pos.y ) > GRID_FSIZE )
            {
                needs_update = btrue;
            }
        }
        else if ( new_grid != pchr->safe_grid )
        {
            needs_update = btrue;
        }
    }

    if ( needs_update )
    {
        retval = chr_update_safe_raw( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_get_safe( chr_t * pchr, fvec3_base_t pos_v )
{
    bool_t found = bfalse;
    fvec3_t loc_pos;

    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    // DO NOT require objects that are spawning in a module to have a
    // valid position at spawn-time. For instance, if a suit of armor is
    // spawned in a closed hallway, don't complain.
    if ( activate_spawn_file_active )
    {
        fvec3_base_copy( pos_v, chr_get_pos_v( pchr ) );
        return btrue;
    }

    // handle optional parameters
    if ( NULL == pos_v ) pos_v = loc_pos.v;

    if ( !found && pchr->safe_valid )
    {
        if ( !chr_hit_wall( pchr, NULL, NULL, NULL, NULL ) )
        {
            found = btrue;
            memmove( pos_v, pchr->safe_pos.v, sizeof( fvec3_base_t ) );
        }
    }

    if ( !found )
    {
        breadcrumb_t * bc;

        bc = breadcrumb_list_last_valid( &( pchr->crumbs ) );

        if ( NULL != bc )
        {
            found = btrue;
            memmove( pos_v, bc->pos.v, sizeof( fvec3_base_t ) );
        }
    }

    // maybe there is one last fallback after this? we could check the character's current position?
    if ( !found )
    {
        log_debug( "Uh oh! We could not find a valid non-wall position for %s!\n", chr_get_pcap( pchr->ai.index )->name );
    }

    return found;
}

//--------------------------------------------------------------------------------------------
bool_t chr_update_breadcrumb_raw( chr_t * pchr )
{
    breadcrumb_t bc;
    bool_t retval = bfalse;

    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    breadcrumb_init_chr( &bc, pchr );

    if ( bc.valid )
    {
        retval = breadcrumb_list_add( &( pchr->crumbs ), &bc );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_update_breadcrumb( chr_t * pchr, bool_t force )
{
    Uint32 new_grid;
    bool_t retval = bfalse;
    bool_t needs_update = bfalse;
    breadcrumb_t * bc_ptr, bc;

    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    bc_ptr = breadcrumb_list_last_valid( &( pchr->crumbs ) );
    if ( NULL == bc_ptr )
    {
        force  = btrue;
        bc_ptr = &bc;
        breadcrumb_init_chr( bc_ptr, pchr );
    }

    if ( force )
    {
        needs_update = btrue;
    }
    else
    {
        new_grid = mesh_get_grid( PMesh, pchr->pos.x, pchr->pos.y );

        if ( INVALID_TILE == new_grid )
        {
            if ( ABS( pchr->pos.x - bc_ptr->pos.x ) > GRID_FSIZE ||
                 ABS( pchr->pos.y - bc_ptr->pos.y ) > GRID_FSIZE )
            {
                needs_update = btrue;
            }
        }
        else if ( new_grid != bc_ptr->grid )
        {
            needs_update = btrue;
        }
    }

    if ( needs_update )
    {
        retval = chr_update_breadcrumb_raw( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
breadcrumb_t * chr_get_last_breadcrumb( chr_t * pchr )
{
    if ( !ALLOCATED_PCHR( pchr ) ) return NULL;

    if ( 0 == pchr->crumbs.count ) return NULL;

    return breadcrumb_list_last_valid( &( pchr->crumbs ) );
}

//--------------------------------------------------------------------------------------------
bool_t move_one_character_integrate_motion_attached( chr_t * pchr )
{
    Uint32 chr_update;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    // make a timer that is individual for each object
    chr_update = pchr->obj_base.guid + update_wld;

    if ( 0 == ( chr_update & 7 ) )
    {
        chr_update_safe( pchr, btrue );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t move_one_character_integrate_motion( chr_t * pchr )
{
    /// @details BB@> Figure out the next position of the character.
    ///    Include collisions with the mesh in this step.

    CHR_REF  ichr;
    ai_state_t * pai;

    float   bumpdampen;
    bool_t  needs_test, updated_2d;

    fvec3_t tmp_pos;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        return move_one_character_integrate_motion_attached( pchr );
    }

    tmp_pos = chr_get_pos( pchr );

    pai = &( pchr->ai );
    ichr = pai->index;

    bumpdampen = CLIP( pchr->phys.bumpdampen, 0.0f, 1.0f );
    bumpdampen = ( bumpdampen + 1.0f ) / 2.0f;

    // interaction with the mesh
    //if ( ABS( pchr->vel.z ) > 0.0f )
    {
        const float vert_offset = RAISE * 0.25f;
        float grid_level = pchr->enviro.grid_level + vert_offset;

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

                    tmp_pos.z = MAX( tmp_pos.z + diff, grid_level );
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

    updated_2d = bfalse;
    needs_test = bfalse;

    // interaction with the grid flags
    updated_2d = bfalse;
    needs_test = bfalse;
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

        if ( EMPTY_BIT_FIELD == chr_test_wall( pchr, tmp_pos.v, &wdata ) )
        {
            updated_2d = btrue;
        }
        else
        {
            fvec2_t nrm;
            float   pressure;
            bool_t diff_function_called = bfalse;

            chr_hit_wall( pchr, tmp_pos.v, nrm.v, &pressure, &wdata );

            // how is the character hitting the wall?
            if ( 0.0f != pressure )
            {
                bool_t         found_nrm  = bfalse;
                bool_t         found_safe = bfalse;
                fvec3_t        safe_pos   = ZERO_VECT3;

                bool_t         found_diff = bfalse;
                fvec2_t        diff       = ZERO_VECT2;

                breadcrumb_t * bc         = NULL;

                // try to get the correct "outward" pressure from nrm
                if ( !found_nrm && ABS( nrm.x ) + ABS( nrm.y ) > 0.0f )
                {
                    found_nrm = btrue;
                }

                if ( !found_diff && pchr->safe_valid )
                {
                    if ( !found_safe )
                    {
                        found_safe = btrue;
                        safe_pos   = pchr->safe_pos;
                    }

                    diff.x = pchr->safe_pos.x - pchr->pos.x;
                    diff.y = pchr->safe_pos.y - pchr->pos.y;

                    if ( ABS( diff.x ) + ABS( diff.y ) > 0.0f )
                    {
                        found_diff = btrue;
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
                            found_safe = btrue;
                            safe_pos   = pchr->safe_pos;
                        }

                        diff.x = bc->pos.x - pchr->pos.x;
                        diff.y = bc->pos.y - pchr->pos.y;

                        if ( ABS( diff.x ) + ABS( diff.y ) > 0.0f )
                        {
                            found_diff = btrue;
                        }
                    }
                }

                // try to get a normal from the mesh_get_diff() function
                if ( !found_nrm )
                {
                    fvec2_t diff;

                    diff = chr_get_mesh_diff( pchr, tmp_pos.v, pressure );
                    diff_function_called = btrue;

                    nrm.x = diff.x;
                    nrm.y = diff.y;

                    if ( ABS( nrm.x ) + ABS( nrm.y ) > 0.0f )
                    {
                        found_nrm = btrue;
                    }
                }

                if ( !found_diff )
                {
                    // try to get the diff from the character velocity
                    diff.x = pchr->vel.x;
                    diff.y = pchr->vel.y;

                    // make sure that the diff is in the same direction as the velocity
                    if ( fvec2_dot_product( diff.v, nrm.v ) < 0.0f )
                    {
                        diff.x *= -1.0f;
                        diff.y *= -1.0f;
                    }

                    if ( ABS( diff.x ) + ABS( diff.y ) > 0.0f )
                    {
                        found_diff = btrue;
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
                            found_safe = btrue;
                            safe_pos   = pchr->safe_pos;
                        }
                    }

                    if ( !found_safe )
                    {
                        // the only safe position is the spawn point???
                        found_safe = btrue;
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

                    fvec2_t v_perp = ZERO_VECT2;
                    fvec2_t diff_perp = ZERO_VECT2;

                    nrm2 = fvec2_dot_product( nrm.v, nrm.v );

                    save_pos = tmp_pos;

                    // make the diff point "out"
                    dot = fvec2_dot_product( diff.v, nrm.v );
                    if ( dot < 0.0f )
                    {
                        diff.x *= -1.0f;
                        diff.y *= -1.0f;
                        dot    *= -1.0f;
                    }

                    // find the part of the diff that is parallel to the normal
                    diff_perp.x = nrm.x * dot / nrm2;
                    diff_perp.y = nrm.y * dot / nrm2;

                    // normalize the diff_perp so that it is at most tile_fraction of a grid in any direction
                    ftmp = MAX( ABS( diff_perp.x ), ABS( diff_perp.y ) );
                    if ( 0 == ftmp ) ftmp = 1.00f;                    //EGOBOO_ASSERT(ftmp > 0.0f);

                    diff_perp.x *= tile_fraction * GRID_FSIZE / ftmp;
                    diff_perp.y *= tile_fraction * GRID_FSIZE / ftmp;

                    // try moving the character
                    tmp_pos.x += diff_perp.x * pressure;
                    tmp_pos.y += diff_perp.y * pressure;

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

                    dot = fvec2_dot_product( pchr->vel.v, nrm.v );
                    if ( dot < 0.0f )
                    {
                        float bumpdampen;
                        cap_t * pcap = chr_get_pcap( GET_REF_PCHR( pchr ) );

                        bumpdampen = 0.0f;
                        if ( NULL == pcap )
                        {
                            bumpdampen = pcap->bumpdampen;
                        }
                        v_perp.x = nrm.x * dot / nrm2;
                        v_perp.y = nrm.y * dot / nrm2;

                        pchr->vel.x += - ( 1.0f + bumpdampen ) * v_perp.x * pressure;
                        pchr->vel.y += - ( 1.0f + bumpdampen ) * v_perp.y * pressure;
                    }
                }
            }
        }
    }

    chr_set_pos( pchr, tmp_pos.v );

    // we need to test the validity of the current position every 8 frames or so,
    // no matter what
    if ( !needs_test )
    {
        // make a timer that is individual for each object
        Uint32 chr_update = pchr->obj_base.guid + update_wld;

        needs_test = ( 0 == ( chr_update & 7 ) );
    }

    if ( needs_test || updated_2d )
    {
        chr_update_safe( pchr, needs_test );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_handle_madfx( chr_t * pchr )
{
    ///@details This handles special commands an animation frame might execute, for example
    ///         grabbing stuff or spawning attack particles.

    CHR_REF ichr;
    Uint32 framefx;

    if ( NULL == pchr ) return bfalse;

    framefx = chr_get_framefx( pchr );
    if ( 0 == framefx ) return btrue;

    ichr    = GET_REF_PCHR( pchr );

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
        character_grab_stuff( ichr, GRIP_LEFT, bfalse );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABRIGHT ) )
    {
        character_grab_stuff( ichr, GRIP_RIGHT, bfalse );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARLEFT ) )
    {
        character_grab_stuff( ichr, GRIP_LEFT, btrue );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARRIGHT ) )
    {
        character_grab_stuff( ichr, GRIP_RIGHT, btrue );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPLEFT ) )
    {
        detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], bfalse, btrue );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPRIGHT ) )
    {
        detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], bfalse, btrue );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_POOF ) && !VALID_PLA( pchr->is_which_player ) )
    {
        pchr->ai.poof_time = update_wld;
    }

    //Do footfall sound effect
    if ( cfg.sound_footfall && HAS_SOME_BITS( framefx, MADFX_FOOTFALL ) )
    {
        cap_t * pcap = pro_get_pcap( pchr->profile_ref );
        if ( NULL != pcap )
        {
            int ifoot = pcap->sound_index[SOUND_FOOTFALL];
            if ( VALID_SND( ifoot ) )
            {
                sound_play_chunk( pchr->pos, chr_get_chunk_ptr( pchr, ifoot ) );
            }
        }
    }

    return btrue;
}

struct s_chr_anim_data
{
    bool_t allowed;
    int    action;
    int    lip;
    float  speed;
};
typedef struct s_chr_anim_data chr_anim_data_t;

int cmp_chr_anim_data( void const * vp_lhs, void const * vp_rhs )
{
    /// @details BB@> Sort MOD REF values based on the rank of the module that they point to.
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
float set_character_animation_rate( chr_t * pchr )
{
    /// @details ZZ@> Get running, walking, sneaking, or dancing, from speed
    ///
    /// BB@> added automatic calculation of variable animation rates for movement animations

    float  speed;
    bool_t can_be_interrupted;
    bool_t is_walk_type;
    int    cnt, anim_count;
    int    action, lip;
    bool_t found;

    chr_anim_data_t anim_info[CHR_MOVEMENT_COUNT];

    MD2_Frame_t * pframe_nxt;

    chr_instance_t * pinst;
    mad_t          * pmad;
    CHR_REF          ichr;

    // set the character speed to zero
    speed = 0;

    if ( NULL == pchr ) return 1.0f;
    pinst = &( pchr->inst );
    ichr  = GET_REF_PCHR( pchr );

    // if the action is set to keep then do nothing
    can_be_interrupted = !pinst->action_keep;
    if ( !can_be_interrupted ) return pinst->rate = 1.0f;

    // dont change the rate if it is an attack animation
    if ( character_is_attacking( pchr ) )  return pinst->rate;

    // if the character is mounted or sitting, base the rate off of the mounr
    if ( INGAME_CHR( pchr->attachedto ) && (( ACTION_MI == pinst->action_which ) || ( ACTION_MH == pinst->action_which ) ) )
    {
        // just copy the rate from the mount
        pinst->rate = ChrList.lst[pchr->attachedto].inst.rate;
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
        anim_info[CHR_MOVEMENT_SNEAK].allowed = bfalse;

        //ZF> small fix here, if there is no sneak animation, try to default to normal walk with reduced animation speed
        if ( HAS_SOME_BITS( pchr->movement_bits, CHR_MOVEMENT_BITS_SNEAK ) )
        {
            anim_info[CHR_MOVEMENT_WALK].allowed = btrue;
            anim_info[CHR_MOVEMENT_WALK].speed *= 2;
        }
    }

    if ( ACTION_WB != pmad->action_map[ACTION_WB] )
    {
        // no specific walk animation exists
        anim_info[CHR_MOVEMENT_WALK].allowed = bfalse;
    }

    if ( ACTION_WC != pmad->action_map[ACTION_WC] )
    {
        // no specific walk animation exists
        anim_info[CHR_MOVEMENT_RUN].allowed = bfalse;
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
        // for non-flying objects, we use the intended speed

        if ( pchr->enviro.is_slipping )
        {
            // the character is slipping as on ice. make their little legs move based on
            // their intended speed, for comic effect! :)
            speed = fvec2_length_abs( pchr->enviro.new_v.v );
        }
        else
        {
            // new_v.x, new_v.y is the speed before any latches are applied
            speed = fvec2_length_abs( pchr->enviro.new_v.v );
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
    found  = bfalse;
    for ( cnt = 0; cnt < anim_count - 1; cnt++ )
    {
        float speed_mid = 0.5f * ( anim_info[cnt].speed + anim_info[cnt+1].speed );

        // make a special case for dance animation(s)
        if ( 0.0f == anim_info[cnt].speed && speed <= 1e-3 )
        {
            found = btrue;
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
        found = btrue;
    }

    if ( !found )
    {
        return pinst->rate;
    }

    pframe_nxt  = chr_instnce_get_frame_nxt( &( pchr->inst ) );

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
            chr_start_anim( pchr, tmp_action, btrue, btrue );
        }
        else
        {
            // if the current action is not ACTION_D* switch to ACTION_DA
            if ( !ACTION_IS_TYPE( pinst->action_which, D ) )
            {
                // get an appropriate version of the boredom action
                int tmp_action = mad_get_action_ref( pinst->imad, ACTION_DA );

                // start the animation
                chr_start_anim( pchr, tmp_action, btrue, btrue );
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
                chr_set_anim( pchr, tmp_action, pmad->frameliptowalkframe[lip][pframe_nxt->framelip], btrue, btrue );
            }

            // "loop" the action
            chr_instance_set_action_next( pinst, tmp_action );
        }
    }

    pinst->rate = CLIP( pinst->rate, 0.1f, 10.0f );

    return pinst->rate;
}

//--------------------------------------------------------------------------------------------
bool_t character_is_attacking( chr_t *pchr )
{
    return pchr->inst.action_which >= ACTION_UA && pchr->inst.action_which <= ACTION_FD;
}

//--------------------------------------------------------------------------------------------
void move_one_character_do_animation( chr_t * pchr )
{
    // Animate the character.
    // Right now there are 50/4 = 12.5 animation frames per second

    float flip_diff, flip_next;

    chr_instance_t * pinst;
    CHR_REF          ichr;

    if ( NULL == pchr ) return;
    ichr  = GET_REF_PCHR( pchr );
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
                log_warning( "chr_increment_frame() did not succeed" );
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
                    log_warning( "chr_increment_frame() did not succeed" );
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
void move_one_character( chr_t * pchr )
{
    if ( !ACTIVE_PCHR( pchr ) ) return;

    if ( pchr->pack.is_packed ) return;

    // save the velocity and acceleration from the last time-step
    pchr->enviro.vel = fvec3_sub( pchr->pos.v, pchr->pos_old.v );
    pchr->enviro.acc = fvec3_sub( pchr->vel.v, pchr->vel_old.v );

    // Character's old location
    pchr->pos_old          = chr_get_pos( pchr );
    pchr->vel_old          = pchr->vel;
    pchr->ori_old.facing_z = pchr->ori.facing_z;

    fvec2_base_copy( pchr->enviro.new_v.v, pchr->vel.v );

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
            pchr->ori.map_facing_x = pchr->ori.map_facing_x * fkeep + map_twist_x[pchr->enviro.grid_twist] * fnew;
            pchr->ori.map_facing_y = pchr->ori.map_facing_y * fkeep + map_twist_y[pchr->enviro.grid_twist] * fnew;
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_all_characters( void )
{
    /// @details ZZ@> This function handles character physics

    chr_stoppedby_tests = 0;

    // Move every character
    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        // prime the environment
        pchr->enviro.air_friction = air_friction;
        pchr->enviro.ice_friction = ice_friction;

        move_one_character( pchr );
    }
    CHR_END_LOOP();

    // The following functions need to be called any time you actually change a charcter's position
    keep_weapons_with_holders();
    attach_all_particles();
    update_all_character_matrices();
}

//--------------------------------------------------------------------------------------------
void cleanup_all_characters()
{
    CHR_REF cnt;

    // Do poofing
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_t * pchr;
        bool_t time_out;

        if ( !ALLOCATED_CHR( cnt ) ) continue;
        pchr = ChrList.lst + cnt;

        time_out = ( pchr->ai.poof_time >= 0 ) && ( pchr->ai.poof_time <= ( Sint32 )update_wld );
        if ( !WAITING_PBASE( POBJ_GET_PBASE( pchr ) ) && !time_out ) continue;

        // detach the character from the game
        cleanup_one_character( pchr );

        // free the character's inventory
        free_inventory_in_game( cnt );

        // free the character
        free_one_character_in_game( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void bump_all_characters_update_counters()
{
    CHR_REF cnt;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        obj_data_t * pbase;

        pbase = POBJ_GET_PBASE( ChrList.lst + cnt );
        if ( !ACTIVE_PBASE( pbase ) ) continue;

        pbase->update_count++;
    }
}

//--------------------------------------------------------------------------------------------
bool_t is_invictus_direction( FACING_T direction, const CHR_REF character, Uint16 effects )
{
    FACING_T left, right;

    chr_t * pchr;
    cap_t * pcap;
    mad_t * pmad;

    bool_t  is_invictus;

    if ( !INGAME_CHR( character ) ) return btrue;
    pchr = ChrList.lst + character;

    pmad = chr_get_pmad( character );
    if ( NULL == pmad ) return btrue;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return btrue;

    // if the invictus flag is set, we are invictus
    if ( pchr->invictus ) return btrue;

    // if the effect is shield piercing, ignore shielding
    if ( HAS_SOME_BITS( effects, DAMFX_NBLOC ) ) return bfalse;

    // if the character's frame is invictus, then check the angles
    if ( HAS_SOME_BITS( chr_get_framefx( pchr ), MADFX_INVICTUS ) )
    {
        //I Frame
        direction -= pcap->iframefacing;
        left       = ( FACING_T )(( int )0x00010000L - ( int )pcap->iframeangle );
        right      = pcap->iframeangle;

        // If using shield, use the shield invictus instead
        if ( ACTION_IS_TYPE( pchr->inst.action_which, P ) )
        {
            bool_t parry_left = ( pchr->inst.action_which < ACTION_PC );

            // Using a shield?
            if ( parry_left )
            {
                // Check left hand
                cap_t * pcap_tmp = chr_get_pcap( pchr->holdingwhich[SLOT_LEFT] );
                if ( NULL != pcap )
                {
                    left  = ( FACING_T )(( int )0x00010000L - ( int )pcap_tmp->iframeangle );
                    right = pcap_tmp->iframeangle;
                }
            }
            else
            {
                // Check right hand
                cap_t * pcap_tmp = chr_get_pcap( pchr->holdingwhich[SLOT_RIGHT] );
                if ( NULL != pcap )
                {
                    left  = ( FACING_T )(( int )0x00010000L - ( int )pcap_tmp->iframeangle );
                    right = pcap_tmp->iframeangle;
                }
            }
        }
    }
    else
    {
        // N Frame
        direction -= pcap->nframefacing;
        left       = ( FACING_T )(( int )0x00010000L - ( int )pcap->nframeangle );
        right      = pcap->nframeangle;
    }

    // Check that direction
    is_invictus = bfalse;
    if ( direction <= left && direction <= right )
    {
        is_invictus = btrue;
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
        islot = CLIP( islot, 0, SLOT_COUNT );

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
bool_t ai_add_order( ai_state_t * pai, Uint32 value, Uint16 counter )
{
    bool_t retval;

    if ( NULL == pai ) return bfalse;

    // this function is only truely valid if there is no other order
    retval = HAS_NO_BITS( pai->alert, ALERTIF_ORDERED );

    SET_BIT( pai->alert, ALERTIF_ORDERED );
    pai->order_value   = value;
    pai->order_counter = counter;

    return retval;
}

//--------------------------------------------------------------------------------------------
BBOARD_REF chr_add_billboard( const CHR_REF ichr, Uint32 lifetime_secs )
{
    /// @details BB@> Attach a basic billboard to a character. You set the billboard texture
    ///     at any time after this. Returns the index of the billboard or INVALID_BILLBOARD
    ///     if the allocation fails.
    ///
    ///    must be called with a valid character, so be careful if you call this function from within
    ///    spawn_one_character()

    chr_t * pchr;

    if ( !INGAME_CHR( ichr ) ) return ( BBOARD_REF )INVALID_BILLBOARD;
    pchr = ChrList.lst + ichr;

    if ( INVALID_BILLBOARD != pchr->ibillboard )
    {
        BillboardList_free_one( REF_TO_INT( pchr->ibillboard ) );
        pchr->ibillboard = INVALID_BILLBOARD;
    }

    pchr->ibillboard = BillboardList_get_free( lifetime_secs );

    // attachr the billboard to the character
    if ( INVALID_BILLBOARD != pchr->ibillboard )
    {
        billboard_data_t * pbb = BillboardList.lst + pchr->ibillboard;

        pbb->ichr = ichr;
    }

    return pchr->ibillboard;
}

//--------------------------------------------------------------------------------------------
billboard_data_t * chr_make_text_billboard( const CHR_REF ichr, const char * txt, const SDL_Color text_color, const GLXvector4f tint, int lifetime_secs, BIT_FIELD opt_bits )
{
    chr_t            * pchr;
    billboard_data_t * pbb;
    int                rv;

    BBOARD_REF ibb = ( BBOARD_REF )INVALID_BILLBOARD;

    if ( !INGAME_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    // create a new billboard or override the old billboard
    ibb = chr_add_billboard( ichr, lifetime_secs );
    if ( INVALID_BILLBOARD == ibb ) return NULL;

    pbb = BillboardList_get_ptr( pchr->ibillboard );
    if ( NULL == pbb ) return pbb;

    rv = billboard_data_printf_ttf( pbb, ui_getFont(), text_color, "%s", txt );

    if ( rv < 0 )
    {
        pchr->ibillboard = INVALID_BILLBOARD;
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

            minval = MIN( MIN( pbb->tint[RR], pbb->tint[GG] ), pbb->tint[BB] );
            maxval = MAX( MAX( pbb->tint[RR], pbb->tint[GG] ), pbb->tint[BB] );

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
const char * chr_get_name( const CHR_REF ichr, Uint32 bits )
{
    static STRING szName;

    if ( !DEFINED_CHR( ichr ) )
    {
        // the default name
        strncpy( szName, "Unknown", SDL_arraysize( szName ) );
    }
    else
    {
        chr_t * pchr = ChrList.lst + ichr;
        cap_t * pcap = pro_get_pcap( pchr->profile_ref );

        if ( pchr->nameknown )
        {
            snprintf( szName, SDL_arraysize( szName ), "%s", pchr->Name );
        }
        else if ( NULL != pcap )
        {
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
                    lTmp = toupper( pcap->classname[0] );

                    if ( 'A' == lTmp || 'E' == lTmp || 'I' == lTmp || 'O' == lTmp || 'U' == lTmp )
                    {
                        article = "an";
                    }
                    else
                    {
                        article = "a";
                    }
                }

                snprintf( szName, SDL_arraysize( szName ), "%s %s", article, pcap->classname );
            }
            else
            {
                snprintf( szName, SDL_arraysize( szName ), "%s", pcap->classname );
            }
        }
        else
        {
            strncpy( szName, "Invalid", SDL_arraysize( szName ) );
        }
    }

    if ( 0 != ( bits & CHRNAME_CAPITAL ) )
    {
        // capitalize the name ?
        szName[0] = toupper( szName[0] );
    }

    return szName;
}

//--------------------------------------------------------------------------------------------
const char * chr_get_dir_name( const CHR_REF ichr )
{
    static STRING buffer = EMPTY_CSTR;
    chr_t * pchr;

    strncpy( buffer, "/debug", SDL_arraysize( buffer ) );

    if ( !DEFINED_CHR( ichr ) ) return buffer;
    pchr = ChrList.lst + ichr;

    if ( !LOADED_PRO( pchr->profile_ref ) )
    {
        char * sztmp;

        EGOBOO_ASSERT( bfalse );

        // copy the character's data.txt path
        strncpy( buffer, pchr->obj_base._name, SDL_arraysize( buffer ) );

        // the name should be "...some path.../data.txt"
        // grab the path

        sztmp = strstr( buffer, "/\\" );
        if ( NULL != sztmp ) *sztmp = CSTR_END;
    }
    else
    {
        pro_t * ppro = ProList.lst + pchr->profile_ref;

        // copy the character's data.txt path
        strncpy( buffer, ppro->name, SDL_arraysize( buffer ) );
    }

    return buffer;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_update_collision_size( chr_t * pchr, bool_t update_matrix )
{
    ///< @detalis BB@> use this function to update the pchr->chr_max_cv and  pchr->chr_min_cv with
    ///<       values that reflect the best possible collision volume
    ///<
    ///< @note This function takes quite a bit of time, so it must only be called when the
    ///< vertices change because of an animation or because the matrix changes.
    ///<
    ///< @todo it might be possible to cache the src[] array used in this function.
    ///< if the matrix changes, then you would not need to recalculate this data...

    int       vcount;   // the actual number of vertices, in case the object is square
    fvec4_t   src[16];  // for the upper and lower octagon points
    fvec4_t   dst[16];  // for the upper and lower octagon points

    int cnt;
    oct_bb_t bsrc, bdst, bmin;

    mad_t * pmad;
    cap_t * pcap;

    if ( !DEFINED_PCHR( pchr ) ) return rv_error;

    // re-initialize the collision volumes
    oct_bb_ctor( &( pchr->chr_min_cv ) );
    oct_bb_ctor( &( pchr->chr_max_cv ) );
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        oct_bb_ctor( pchr->slot_cv + cnt );
    }

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return rv_error;

    pmad = chr_get_pmad( GET_REF_PCHR( pchr ) );
    if ( NULL == pmad ) return rv_error;

    // make sure the matrix is updated properly
    if ( update_matrix )
    {
        // call chr_update_matrix() but pass in a false value to prevent a recursize call
        if ( rv_error == chr_update_matrix( pchr, bfalse ) )
        {
            return rv_error;
        }
    }

    // make sure the bounding box is calculated properly
    if ( rv_error == chr_instance_update_bbox( &( pchr->inst ) ) )
    {
        return rv_error;
    }

    // convert the point cloud in the GLvertex array (pchr->inst.vrt_lst) to
    // a level 1 bounding box. Subtract off the position of the character
    memcpy( &bsrc, &( pchr->inst.bbox ), sizeof( bsrc ) );

    // convert the corners of the level 1 bounding box to a point cloud
    vcount = oct_bb_to_points( &bsrc, src, 16 );

    // transform the new point cloud
    TransformVertices( &( pchr->inst.matrix ), src, dst, vcount );

    // convert the new point cloud into a level 1 bounding box
    points_to_oct_bb( &bdst, dst, vcount );

    //---- set the bounding boxes
    oct_bb_copy( &( pchr->chr_min_cv ), &bdst );
    oct_bb_copy( &( pchr->chr_max_cv ), &bdst );

    oct_bb_set_bumper( &bmin, pchr->bump );

    // only use pchr->bump.size if it was overridden in data.txt through the [MODL] expansion
    if ( pcap->bump_override_size )
    {
        oct_bb_self_intersection_index( &( pchr->chr_min_cv ), &bmin, OCT_X );
        oct_bb_self_intersection_index( &( pchr->chr_min_cv ), &bmin, OCT_Y );

        oct_bb_self_union_index( &( pchr->chr_max_cv ), &bmin, OCT_X );
        oct_bb_self_union_index( &( pchr->chr_max_cv ), &bmin, OCT_Y );
    }

    // only use pchr->bump.size_big if it was overridden in data.txt through the [MODL] expansion
    if ( pcap->bump_override_sizebig )
    {
        oct_bb_self_intersection_index( &( pchr->chr_min_cv ), &bmin, OCT_XY );
        oct_bb_self_intersection_index( &( pchr->chr_min_cv ), &bmin, OCT_YX );

        oct_bb_self_union_index( &( pchr->chr_max_cv ), &bmin, OCT_XY );
        oct_bb_self_union_index( &( pchr->chr_max_cv ), &bmin, OCT_YX );
    }

    // only use pchr->bump.height if it was overridden in data.txt through the [MODL] expansion
    if ( pcap->bump_override_height )
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
        if ( !pcap->slotvalid[ cnt ] ) continue;

        chr_calc_grip_cv( pchr, GRIP_LEFT, pchr->slot_cv + cnt, NULL, NULL, bfalse );

        oct_bb_self_union( &( pchr->chr_max_cv ), pchr->slot_cv + cnt );
    }

    // convert the level 1 bounding box to a level 0 bounding box
    oct_bb_downgrade( &bdst, pchr->bump_stt, pchr->bump, &( pchr->bump_1 ), NULL );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
const char* describe_value( float value, float maxval, int * rank_ptr )
{
    /// @details ZF@> This converts a stat number into a more descriptive word

    static STRING retval;

    float fval;
    int local_rank;

    if ( NULL == rank_ptr ) rank_ptr = &local_rank;

    if ( cfg.feedback == FEEDBACK_NUMBER )
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
    /// @details ZF@> This converts a damage value into a more descriptive word

    static STRING retval;

    float fval;
    int local_rank;

    if ( NULL == rank_ptr ) rank_ptr = &local_rank;

    if ( cfg.feedback == FEEDBACK_NUMBER )
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
    /// @details ZF@> This tells us how badly someone is injured

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

    if ( cfg.feedback == FEEDBACK_NUMBER )
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
TX_REF chr_get_icon_ref( const CHR_REF item )
{
    /// @details BB@> Get the index to the icon texture (in TxTexture) that is supposed to be used with this object.
    ///               If none can be found, return the index to the texture of the null icon.

    size_t iskin;
    TX_REF icon_ref = ( TX_REF )ICON_NULL;
    bool_t is_spell_fx, is_book, draw_book;

    cap_t * pitem_cap;
    chr_t * pitem;
    pro_t * pitem_pro;

    if ( !DEFINED_CHR( item ) ) return icon_ref;
    pitem = ChrList.lst + item;

    if ( !LOADED_PRO( pitem->profile_ref ) ) return icon_ref;
    pitem_pro = ProList.lst + pitem->profile_ref;

    pitem_cap = pro_get_pcap( pitem->profile_ref );
    if ( NULL == pitem_cap ) return icon_ref;

    // what do we need to draw?
    is_spell_fx = ( NO_SKIN_OVERRIDE != pitem_cap->spelleffect_type );     // the value of spelleffect_type == the skin of the book or -1 for not a spell effect
    is_book     = ( SPELLBOOK == pitem->profile_ref );
    draw_book   = ( is_book || ( is_spell_fx && !pitem->draw_icon ) /*|| ( is_spell_fx && MAX_CHR != pitem->attachedto )*/ ) && ( bookicon_count > 0 ); //>ZF> uncommented a part because this caused a icon bug when you were morphed and mounted

    if ( !draw_book )
    {
        iskin = pitem->skin;

        icon_ref = pitem_pro->ico_ref[iskin];
    }
    else if ( draw_book )
    {
        iskin = 0;

        if ( NO_SKIN_OVERRIDE != pitem_cap->spelleffect_type )
        {
            iskin = pitem_cap->spelleffect_type;
        }
        else if ( NO_SKIN_OVERRIDE != pitem_cap->skin_override )
        {
            iskin = pitem_cap->skin_override;
        }

        iskin = CLIP( iskin, 0, bookicon_count );

        icon_ref = bookicon_ref[ iskin ];
    }

    return icon_ref;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_all_cap()
{
    /// @details BB@> initialize every character profile in the game

    CAP_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        cap_init( CapStack.lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_cap()
{
    /// @details BB@> release every character profile in the game

    CAP_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_cap( cnt );
    };
}

//--------------------------------------------------------------------------------------------
bool_t release_one_cap( const CAP_REF icap )
{
    /// @details BB@> release any allocated data and return all values to safe values

    cap_t * pcap;

    if ( !VALID_CAP_RANGE( icap ) ) return bfalse;
    pcap = CapStack.lst + icap;

    if ( !pcap->loaded ) return btrue;

    cap_init( pcap );

    pcap->loaded  = bfalse;
    pcap->name[0] = CSTR_END;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void reset_teams()
{
    /// @details ZZ@> This function makes everyone hate everyone else

    TEAM_REF teama, teamb;

    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        // Make the team hate everyone
        for ( teamb = 0; teamb < TEAM_MAX; teamb++ )
        {
            TeamStack.lst[teama].hatesteam[REF_TO_INT( teamb )] = btrue;
        }

        // Make the team like itself
        TeamStack.lst[teama].hatesteam[REF_TO_INT( teama )] = bfalse;

        // Set defaults
        TeamStack.lst[teama].leader = NOLEADER;
        TeamStack.lst[teama].sissy = 0;
        TeamStack.lst[teama].morale = 0;
    }

    // Keep the null team neutral
    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        TeamStack.lst[teama].hatesteam[TEAM_NULL] = bfalse;
        TeamStack.lst[( TEAM_REF )TEAM_NULL].hatesteam[REF_TO_INT( teama )] = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
bool_t chr_teleport( const CHR_REF ichr, float x, float y, float z, FACING_T facing_z )
{
    /// @details BB@> Determine whether the character can be teleported to the specified location
    ///               and do it, if possible. Success returns btrue, failure returns bfalse;

    chr_t  * pchr;
    FACING_T facing_old, facing_new;
    fvec3_t  pos_old, pos_new;
    bool_t   retval;

    if ( !INGAME_CHR( ichr ) ) return bfalse;
    pchr = ChrList.lst + ichr;

    if ( x < 0.0f || x > PMesh->gmem.edge_x ) return bfalse;
    if ( y < 0.0f || y > PMesh->gmem.edge_y ) return bfalse;

    pos_old  = chr_get_pos( pchr );
    facing_old = pchr->ori.facing_z;

    pos_new.x  = x;
    pos_new.y  = y;
    pos_new.z  = z;
    facing_new = facing_z;

    if ( chr_hit_wall( pchr, pos_new.v, NULL, NULL, NULL ) )
    {
        // No it didn't...
        chr_set_pos( pchr, pos_old.v );
        pchr->ori.facing_z = facing_old;

        retval = bfalse;
    }
    else
    {
        // Yeah!  It worked!

        // update the old position
        pchr->pos_old          = pos_new;
        pchr->ori_old.facing_z = facing_new;

        // update the new position
        chr_set_pos( pchr, pos_new.v );
        pchr->ori.facing_z = facing_new;

        if ( !detach_character_from_mount( ichr, btrue, bfalse ) )
        {
            // detach_character_from_mount() updates the character matrix unless it is not mounted
            chr_update_matrix( pchr, btrue );
        }

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_request_terminate( const CHR_REF ichr )
{
    /// @details BB@> Mark this character for deletion

    if ( !DEFINED_CHR( ichr ) ) return bfalse;

    POBJ_REQUEST_TERMINATE( ChrList.lst + ichr );

    return btrue;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_update_hide( chr_t * pchr )
{
    /// @details BB@> Update the hide state of the character. Should be called every time that
    ///               the state variable in an ai_state_t structure is updated

    Sint8 hide;
    cap_t * pcap;

    if ( !DEFINED_PCHR( pchr ) ) return pchr;

    hide = NOHIDE;
    pcap = chr_get_pcap( GET_REF_PCHR( pchr ) );
    if ( NULL != pcap )
    {
        hide = pcap->hidestate;
    }

    pchr->is_hidden = bfalse;
    if ( hide != NOHIDE && hide == pchr->ai.state )
    {
        pchr->is_hidden = btrue;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
bool_t ai_state_set_changed( ai_state_t * pai )
{
    /// @details BB@> do something tricky here

    bool_t retval = bfalse;

    if ( NULL == pai ) return bfalse;

    if ( HAS_NO_BITS( pai->alert, ALERTIF_CHANGED ) )
    {
        SET_BIT( pai->alert, ALERTIF_CHANGED );
        retval = btrue;
    }

    if ( !pai->changed )
    {
        pai->changed = btrue;
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_matrix_valid( chr_t * pchr )
{
    /// @details BB@> Determine whether the character has a valid matrix

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    // both the cache and the matrix need to be valid
    return pchr->inst.matrix_cache.valid && pchr->inst.matrix_cache.matrix_valid;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int get_grip_verts( Uint16 grip_verts[], const CHR_REF imount, int vrt_offset )
{
    /// @details BB@> Fill the grip_verts[] array from the mount's data.
    ///     Return the number of vertices found.

    Uint32  i;
    int vrt_count, tnc;

    chr_t * pmount;
    mad_t * pmount_mad;

    if ( NULL == grip_verts ) return 0;

    // set all the vertices to a "safe" value
    for ( i = 0; i < GRIP_VERTS; i++ )
    {
        grip_verts[i] = 0xFFFF;
    }

    if ( !INGAME_CHR( imount ) ) return 0;
    pmount = ChrList.lst + imount;

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
bool_t chr_get_matrix_cache( chr_t * pchr, matrix_cache_t * mc_tmp )
{
    /// @details BB@> grab the matrix cache data for a given character and put it into mc_tmp.

    bool_t handled;
    CHR_REF itarget, ichr;

    if ( NULL == mc_tmp ) return bfalse;
    if ( !DEFINED_PCHR( pchr ) ) return bfalse;
    ichr = GET_REF_PCHR( pchr );

    handled = bfalse;
    itarget = ( CHR_REF )MAX_CHR;

    // initialize xome parameters in case we fail
    mc_tmp->valid     = bfalse;
    mc_tmp->type_bits = MAT_UNKNOWN;

    mc_tmp->self_scale.x = mc_tmp->self_scale.y = mc_tmp->self_scale.z = pchr->fat;

    // handle the overlay first of all
    if ( !handled && pchr->is_overlay && ichr != pchr->ai.target && INGAME_CHR( pchr->ai.target ) )
    {
        // this will pretty much fail the cmp_matrix_cache() every time...

        chr_t * ptarget = ChrList.lst + pchr->ai.target;

        // make sure we have the latst info from the target
        chr_update_matrix( ptarget, btrue );

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
        itarget = GET_REF_PCHR( pchr );

        //---- update the MAT_WEAPON data
        if ( DEFINED_CHR( pchr->attachedto ) )
        {
            chr_t * pmount = ChrList.lst + pchr->attachedto;

            // make sure we have the latst info from the target
            chr_update_matrix( pmount, btrue );

            // just in case the mounts's matrix cannot be corrected
            // then treat it as if it is not mounted... yuck
            if ( pmount->inst.matrix_cache.matrix_valid )
            {
                mc_tmp->valid     = btrue;
                SET_BIT( mc_tmp->type_bits, MAT_WEAPON );        // add in the weapon data

                mc_tmp->grip_chr  = pchr->attachedto;
                mc_tmp->grip_slot = pchr->inwhich_slot;
                get_grip_verts( mc_tmp->grip_verts, pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );

                itarget = pchr->attachedto;
            }
        }

        //---- update the MAT_CHARACTER data
        if ( DEFINED_CHR( itarget ) )
        {
            chr_t * ptarget = ChrList.lst + itarget;

            mc_tmp->valid   = btrue;
            SET_BIT( mc_tmp->type_bits, MAT_CHARACTER );  // add in the MAT_CHARACTER-type data for the object we are "connected to"

            mc_tmp->rotate.x = CLIP_TO_16BITS( ptarget->ori.map_facing_x - MAP_TURN_OFFSET );
            mc_tmp->rotate.y = CLIP_TO_16BITS( ptarget->ori.map_facing_y - MAP_TURN_OFFSET );
            mc_tmp->rotate.z = ptarget->ori.facing_z;

            mc_tmp->pos = chr_get_pos( ptarget );

            mc_tmp->grip_scale.x = mc_tmp->grip_scale.y = mc_tmp->grip_scale.z = ptarget->fat;
        }
    }

    return mc_tmp->valid;
}

//--------------------------------------------------------------------------------------------
int convert_grip_to_local_points( chr_t * pholder, Uint16 grip_verts[], fvec4_t dst_point[] )
{
    /// @details ZZ@> a helper function for apply_one_weapon_matrix()

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
        dst_point[0].x = pholder->pos.x;
        dst_point[0].y = pholder->pos.y;
        dst_point[0].z = pholder->pos.z;
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
    /// @details ZZ@> a helper function for apply_one_weapon_matrix()

    chr_t *   pholder;
    int       point_count;
    fvec4_t   src_point[GRIP_VERTS];

    if ( !INGAME_CHR( iholder ) ) return 0;
    pholder = ChrList.lst + iholder;

    // find the grip points in the character's "local" or "body-fixed" coordinates
    point_count = convert_grip_to_local_points( pholder, grip_verts, src_point );

    if ( 0 == point_count ) return 0;

    // use the math function instead of rolling out own
    TransformVertices( &( pholder->inst.matrix ), src_point, dst_point, point_count );

    return point_count;
}

//--------------------------------------------------------------------------------------------
bool_t apply_one_weapon_matrix( chr_t * pweap, matrix_cache_t * mc_tmp )
{
    /// @details ZZ@> Request that the data in the matrix cache be used to create a "character matrix".
    ///               i.e. a matrix that is not being held by anything.

    fvec4_t   nupoint[GRIP_VERTS];
    int       iweap_points;

    chr_t * pholder;
    matrix_cache_t * pweap_mcache;

    if ( NULL == mc_tmp || !mc_tmp->valid || 0 == ( MAT_WEAPON & mc_tmp->type_bits ) ) return bfalse;

    if ( !DEFINED_PCHR( pweap ) ) return bfalse;
    pweap_mcache = &( pweap->inst.matrix_cache );

    if ( !INGAME_CHR( mc_tmp->grip_chr ) ) return bfalse;
    pholder = ChrList.lst + mc_tmp->grip_chr;

    // make sure that the matrix is invalid incase of an error
    pweap_mcache->matrix_valid = bfalse;

    // grab the grip points in world coordinates
    iweap_points = convert_grip_to_global_points( mc_tmp->grip_chr, mc_tmp->grip_verts, nupoint );

    if ( 4 == iweap_points )
    {
        // Calculate weapon's matrix based on positions of grip points
        // chrscale is recomputed at time of attachment
        pweap->inst.matrix = FourPoints( nupoint[0].v, nupoint[1].v, nupoint[2].v, nupoint[3].v, mc_tmp->self_scale.z );

        // update the weapon position
        chr_set_pos( pweap, nupoint[3].v );

        memcpy( &( pweap->inst.matrix_cache ), mc_tmp, sizeof( matrix_cache_t ) );

        pweap_mcache->matrix_valid = btrue;
    }
    else if ( iweap_points > 0 )
    {
        // cannot find enough vertices. punt.
        // ignore the shape of the grip and just stick the character to the single mount point

        // update the character position
        chr_set_pos( pweap, nupoint[0].v );

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
bool_t apply_one_character_matrix( chr_t * pchr, matrix_cache_t * mc_tmp )
{
    /// @details ZZ@> Request that the matrix cache data be used to create a "weapon matrix".
    ///               i.e. a matrix that is attached to a specific grip.

    if ( NULL == mc_tmp ) return bfalse;

    // only apply character matrices using this function
    if ( 0 == ( MAT_CHARACTER & mc_tmp->type_bits ) ) return bfalse;

    pchr->inst.matrix_cache.matrix_valid = bfalse;

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    if ( pchr->stickybutt )
    {
        pchr->inst.matrix = ScaleXYZRotateXYZTranslate_SpaceFixed( mc_tmp->self_scale.x, mc_tmp->self_scale.y, mc_tmp->self_scale.z,
                            TO_TURN( mc_tmp->rotate.z ), TO_TURN( mc_tmp->rotate.x ), TO_TURN( mc_tmp->rotate.y ),
                            mc_tmp->pos.x, mc_tmp->pos.y, mc_tmp->pos.z );
    }
    else
    {
        pchr->inst.matrix = ScaleXYZRotateXYZTranslate_BodyFixed( mc_tmp->self_scale.x, mc_tmp->self_scale.y, mc_tmp->self_scale.z,
                            TO_TURN( mc_tmp->rotate.z ), TO_TURN( mc_tmp->rotate.x ), TO_TURN( mc_tmp->rotate.y ),
                            mc_tmp->pos.x, mc_tmp->pos.y, mc_tmp->pos.z );
    }

    memcpy( &( pchr->inst.matrix_cache ), mc_tmp, sizeof( matrix_cache_t ) );

    pchr->inst.matrix_cache.matrix_valid = btrue;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t apply_reflection_matrix( chr_instance_t * pinst, float grid_level )
{
    /// @detalis BB@> Generate the extra data needed to display a reflection for this character

    if ( NULL == pinst ) return bfalse;

    // invalidate the current matrix
    pinst->ref.matrix_valid = bfalse;

    // actually flip the matrix
    if ( pinst->matrix_cache.valid )
    {
        pinst->ref.matrix = pinst->matrix;

        pinst->ref.matrix.CNV( 0, 2 ) = -pinst->ref.matrix.CNV( 0, 2 );
        pinst->ref.matrix.CNV( 1, 2 ) = -pinst->ref.matrix.CNV( 1, 2 );
        pinst->ref.matrix.CNV( 2, 2 ) = -pinst->ref.matrix.CNV( 2, 2 );
        pinst->ref.matrix.CNV( 3, 2 ) = 2 * grid_level - pinst->ref.matrix.CNV( 3, 2 );

        pinst->ref.matrix_valid = btrue;

        // fix the reflection
        chr_instance_update_ref( pinst, grid_level, bfalse );
    }

    return pinst->ref.matrix_valid;
}

//--------------------------------------------------------------------------------------------
bool_t apply_matrix_cache( chr_t * pchr, matrix_cache_t * mc_tmp )
{
    /// @detalis BB@> request that the info in the matrix cache mc_tmp, be used to
    ///               make a matrix for the character pchr.

    bool_t applied = bfalse;

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;
    if ( NULL == mc_tmp || !mc_tmp->valid ) return bfalse;

    if ( 0 != ( MAT_WEAPON & mc_tmp->type_bits ) )
    {
        if ( DEFINED_CHR( mc_tmp->grip_chr ) )
        {
            applied = apply_one_weapon_matrix( pchr, mc_tmp );
        }
        else
        {
            matrix_cache_t * mcache = &( pchr->inst.matrix_cache );

            // !!!the mc_tmp was mis-labeled as a MAT_WEAPON!!!
            make_one_character_matrix( GET_REF_PCHR( pchr ) );

            // recover the matrix_cache values from the character
            SET_BIT( mcache->type_bits, MAT_CHARACTER );
            if ( mcache->matrix_valid )
            {
                mcache->valid     = btrue;
                mcache->type_bits = MAT_CHARACTER;

                mcache->self_scale.x =
                    mcache->self_scale.y =
                        mcache->self_scale.z = pchr->fat;

                mcache->grip_scale = mcache->self_scale;

                mcache->rotate.x = CLIP_TO_16BITS( pchr->ori.map_facing_x - MAP_TURN_OFFSET );
                mcache->rotate.y = CLIP_TO_16BITS( pchr->ori.map_facing_y - MAP_TURN_OFFSET );
                mcache->rotate.z = pchr->ori.facing_z;

                mcache->pos = chr_get_pos( pchr );

                applied = btrue;
            }
        }
    }
    else if ( 0 != ( MAT_CHARACTER & mc_tmp->type_bits ) )
    {
        applied = apply_one_character_matrix( pchr, mc_tmp );
    }

    if ( applied )
    {
        apply_reflection_matrix( &( pchr->inst ), pchr->enviro.grid_level );
    }

    return applied;
}

//--------------------------------------------------------------------------------------------
int cmp_matrix_cache( const void * vlhs, const void * vrhs )
{
    /// @details BB@> check for differences between the data pointed to
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
egoboo_rv matrix_cache_needs_update( chr_t * pchr, matrix_cache_t * pmc )
{
    /// @details BB@> determine whether a matrix cache has become invalid and needs to be updated

    matrix_cache_t local_mc;
    bool_t needs_cache_update;

    if ( !DEFINED_PCHR( pchr ) ) return rv_error;

    if ( NULL == pmc ) pmc = &local_mc;

    // get the matrix data that is supposed to be used to make the matrix
    chr_get_matrix_cache( pchr, pmc );

    // compare that data to the actual data used to make the matrix
    needs_cache_update = ( 0 != cmp_matrix_cache( pmc, &( pchr->inst.matrix_cache ) ) );

    return needs_cache_update ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_update_matrix( chr_t * pchr, bool_t update_size )
{
    /// @details BB@> Do everything necessary to set the current matrix for this character.
    ///     This might include recursively going down the list of this character's mounts, etc.
    ///
    ///     Return btrue if a new matrix is applied to the character, bfalse otherwise.

    egoboo_rv      retval;
    bool_t         needs_update = bfalse;
    bool_t         applied      = bfalse;
    matrix_cache_t mc_tmp;
    matrix_cache_t *pchr_mc = NULL;

    if ( !DEFINED_PCHR( pchr ) ) return rv_error;
    pchr_mc = &( pchr->inst.matrix_cache );

    // recursively make sure that any mount matrices are updated
    if ( DEFINED_CHR( pchr->attachedto ) )
    {
        egoboo_rv attached_update = rv_error;

        attached_update = chr_update_matrix( ChrList.lst + pchr->attachedto, btrue );

        // if this fails, we should probably do something...
        if ( rv_error == attached_update )
        {
            // there is an error so this matrix is not defined and no readon to go farther
            pchr_mc->matrix_valid = bfalse;
            return attached_update;
        }
        else if ( rv_success == attached_update )
        {
            // the holder/mount matrix has changed.
            // this matrix is no longer valid.
            pchr_mc->matrix_valid = bfalse;
        }
    }

    // does the matrix cache need an update at all?
    retval = matrix_cache_needs_update( pchr, &mc_tmp );
    if ( rv_error == retval ) return rv_error;
    needs_update = ( rv_success == retval );

    // Update the grip vertices no matter what (if they are used)
    if ( HAS_SOME_BITS( mc_tmp.type_bits, MAT_WEAPON ) && INGAME_CHR( mc_tmp.grip_chr ) )
    {
        egoboo_rv grip_retval;
        chr_t   * ptarget = ChrList.lst + mc_tmp.grip_chr;

        // has that character changes its animation?
        grip_retval = ( egoboo_rv )chr_instance_update_grip_verts( &( ptarget->inst ), mc_tmp.grip_verts, GRIP_VERTS );

        if ( rv_error   == grip_retval ) return rv_error;
        if ( rv_success == grip_retval ) needs_update = btrue;
    }

    // if it is not the same, make a new matrix with the new data
    applied = bfalse;
    if ( needs_update )
    {
        // we know the matrix is not valid
        pchr_mc->matrix_valid = bfalse;

        applied = apply_matrix_cache( pchr, &mc_tmp );
    }

    if ( applied && update_size )
    {
        // call chr_update_collision_size() but pass in a false value to prevent a recursize call
        chr_update_collision_size( pchr, bfalse );
    }

    return applied ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
bool_t ai_state_set_bumplast( ai_state_t * pself, const CHR_REF ichr )
{
    /// @details BB@> bumping into a chest can initiate whole loads of update messages.
    ///     Try to throttle the rate that new "bump" messages can be passed to the ai

    if ( NULL == pself ) return bfalse;

    if ( !INGAME_CHR( ichr ) ) return bfalse;

    // 5 bumps per second?
    if ( pself->bumplast != ichr ||  update_wld > pself->bumplast_time + TARGET_UPS / 5 )
    {
        pself->bumplast_time = update_wld;
        SET_BIT( pself->alert, ALERTIF_BUMPED );
    }
    pself->bumplast = ichr;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF chr_has_inventory_idsz( const CHR_REF ichr, IDSZ idsz, bool_t equipped, CHR_REF * pack_last )
{
    /// @details BB@> check the pack a matching item

    bool_t matches_equipped;
    CHR_REF item, tmp_var;
    chr_t * pchr;

    if ( !INGAME_CHR( ichr ) ) return ( CHR_REF )MAX_CHR;
    pchr = ChrList.lst + ichr;

    // make sure that pack_last points to something
    if ( NULL == pack_last ) pack_last = &tmp_var;

    item = ( CHR_REF )MAX_CHR;

    *pack_last = GET_REF_PCHR( pchr );

    PACK_BEGIN_LOOP( ipacked, pchr->pack.next )
    {
        matches_equipped = ( !equipped || ( INGAME_CHR( ipacked ) && ChrList.lst[ipacked].isequipped ) );

        if ( chr_is_type_idsz( ipacked, idsz ) && matches_equipped )
        {
            item = ipacked;
            break;
        }

        *pack_last = ipacked;
    }
    PACK_END_LOOP( ipacked );

    return item;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_holding_idsz( const CHR_REF ichr, IDSZ idsz )
{
    /// @details BB@> check the character's hands for a matching item

    bool_t found;
    CHR_REF item, tmp_item;
    chr_t * pchr;

    if ( !INGAME_CHR( ichr ) ) return ( CHR_REF )MAX_CHR;
    pchr = ChrList.lst + ichr;

    item = ( CHR_REF )MAX_CHR;
    found = bfalse;

    if ( !found )
    {
        // Check right hand. technically a held item cannot be equipped...
        tmp_item = pchr->holdingwhich[SLOT_RIGHT];

        if ( chr_is_type_idsz( tmp_item, idsz ) )
        {
            found = btrue;
            item = tmp_item;
        }
    }

    if ( !found )
    {
        // Check left hand. technically a held item cannot be equipped...
        tmp_item = pchr->holdingwhich[SLOT_LEFT];

        if ( chr_is_type_idsz( tmp_item, idsz ) )
        {
            found = btrue;
            item = tmp_item;
        }
    }

    return item;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_has_item_idsz( const CHR_REF ichr, IDSZ idsz, bool_t equipped, CHR_REF * pack_last )
{
    /// @detalis BB@> is ichr holding an item matching idsz, or is such an item in his pack?
    ///               return the index of the found item, or MAX_CHR if not found. Also return
    ///               the previous pack item in *pack_last, or MAX_CHR if it was not in a pack.

    bool_t found;
    CHR_REF item, tmp_var;
    chr_t * pchr;

    if ( !INGAME_CHR( ichr ) ) return ( CHR_REF )MAX_CHR;
    pchr = ChrList.lst + ichr;

    // make sure that pack_last points to something
    if ( NULL == pack_last ) pack_last = &tmp_var;

    // Check the pack
    *pack_last = ( CHR_REF )MAX_CHR;
    item       = ( CHR_REF )MAX_CHR;
    found      = bfalse;

    if ( !found )
    {
        item = chr_holding_idsz( ichr, idsz );
        found = INGAME_CHR( item );
    }

    if ( !found )
    {
        item = chr_has_inventory_idsz( ichr, idsz, equipped, pack_last );
        found = INGAME_CHR( item );
    }

    return item;
}

//--------------------------------------------------------------------------------------------
bool_t chr_can_see_invis( const chr_t * pchr, const chr_t * pobj )
{
    /// @detalis BB@> can ichr see iobj?

    int     alpha;

    if ( NULL == pchr || NULL == pobj ) return bfalse;

    /// @note ZF@> Invictus characters can always see through darkness (spells, items, quest handlers, etc.)
    if ( pchr->invictus ) return btrue;

    alpha = pobj->inst.alpha;
    if ( 0 != pchr->see_invisible_level )
    {
        alpha = get_alpha( alpha, exp( 0.32f * ( float )pchr->see_invisible_level ) );
    }
    alpha = CLIP( alpha, 0, 255 );

    return alpha >= INVISIBLE;
}

//--------------------------------------------------------------------------------------------
bool_t chr_can_see_dark( const chr_t * pchr, const chr_t * pobj )
{
    /// @detalis BB@> can ichr see iobj?

    cap_t * pcap;

    int     light, self_light, enviro_light;

    if ( NULL == pchr || NULL == pobj ) return bfalse;

    enviro_light = ( pobj->inst.alpha * pobj->inst.max_light ) * INV_FF;
    self_light   = ( pobj->inst.light == 255 ) ? 0 : pobj->inst.light;
    light        = MAX( enviro_light, self_light );

    if ( 0 != pchr->darkvision_level )
    {
        light *= exp( 0.32f * ( float )pchr->darkvision_level );
    }

    // Scenery, spells and quest objects can always see through darkness
    // Checking pchr->invictus is not enough, since that could be temporary
    // and not indicate the appropriate objects
    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL != pcap )
    {
        if ( pcap->invictus ) light = INVISIBLE;
    }

    return light >= INVISIBLE;
}

//--------------------------------------------------------------------------------------------
bool_t chr_can_see_object( const CHR_REF ichr, const CHR_REF iobj )
{
    /// @detalis BB@> can ichr see iobj?

    chr_t * pchr, * pobj;

    bool_t too_dark, too_invis;

    if ( !INGAME_CHR( ichr ) ) return bfalse;
    pchr = ChrList.lst + ichr;

    if ( !INGAME_CHR( iobj ) ) return bfalse;
    pobj = ChrList.lst + iobj;

    too_dark  = !chr_can_see_dark( pchr, pobj );
    too_invis = !chr_can_see_invis( pchr, pobj );

    return !too_dark && !too_invis;
}

//--------------------------------------------------------------------------------------------
int chr_get_price( const CHR_REF ichr )
{
    /// @detalis BB@> determine the correct price for an item

    CAP_REF icap;
    Uint16  iskin;
    float   price;

    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( ichr ) ) return 0;
    pchr = ChrList.lst + ichr;

    // Make sure spell books are priced according to their spell and not the book itself
    if ( pchr->profile_ref == SPELLBOOK )
    {
        icap = pro_get_icap( pchr->basemodel_ref );
        iskin = 0;
    }
    else
    {
        icap  = pro_get_icap( pchr->profile_ref );
        iskin = pchr->skin;
    }

    if ( !LOADED_CAP( icap ) ) return 0;
    pcap = CapStack.lst + icap;

    price = ( float ) pcap->skincost[iskin];

    // Items spawned in shops are more valuable
    if ( !pchr->isshopitem ) price *= 0.5f;

    // base the cost on the number of items/charges
    if ( pcap->isstackable )
    {
        price *= pchr->ammo;
    }
    else if ( pcap->isranged && pchr->ammo < pchr->ammomax )
    {
        if ( 0 != pchr->ammo )
        {
            price *= ( float ) pchr->ammo / ( float ) pchr->ammomax;
        }
    }

    return ( int )price;
}

//--------------------------------------------------------------------------------------------
void chr_set_floor_level( chr_t * pchr, float level )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    if ( level != pchr->enviro.floor_level )
    {
        pchr->enviro.floor_level = level;
    }
}

//--------------------------------------------------------------------------------------------
void chr_set_redshift( chr_t * pchr, int rs )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.redshift = rs;

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void chr_set_grnshift( chr_t * pchr, int gs )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.grnshift = gs;

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void chr_set_blushift( chr_t * pchr, int bs )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.blushift = bs;

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void chr_set_sheen( chr_t * pchr, int sheen )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.sheen = sheen;

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void chr_set_alpha( chr_t * pchr, int alpha )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.alpha = CLIP( alpha, 0, 255 );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void chr_set_light( chr_t * pchr, int light )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.light = CLIP( light, 0, 255 );

    //This prevents players from becoming completely invisible
    if ( VALID_PLA( pchr->is_which_player ) )  pchr->inst.light = MAX( 128, pchr->inst.light );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void chr_instance_get_tint( chr_instance_t * pinst, GLfloat * tint, BIT_FIELD bits )
{
    int i;
    float weight_sum;
    GLXvector4f local_tint;

    int local_alpha;
    int local_light;
    int local_sheen;
    int local_redshift;
    int local_grnshift;
    int local_blushift;

    if ( NULL == tint ) tint = local_tint;

    if ( HAS_SOME_BITS( bits, CHR_REFLECT ) )
    {
        // this is a reflection, use the reflected parameters
        local_alpha    = pinst->ref.alpha;
        local_light    = pinst->ref.light;
        local_sheen    = pinst->ref.sheen;
        local_redshift = pinst->ref.redshift;
        local_grnshift = pinst->ref.grnshift;
        local_blushift = pinst->ref.blushift;
    }
    else
    {
        // this is NOT a reflection, use the normal parameters
        local_alpha    = pinst->alpha;
        local_light    = pinst->light;
        local_sheen    = pinst->sheen;
        local_redshift = pinst->redshift;
        local_grnshift = pinst->grnshift;
        local_blushift = pinst->blushift;
    }

    // modify these values based on local character abilities
    local_alpha = get_alpha( local_alpha, local_stats.seeinvis_mag );
    local_light = get_light( local_light, local_stats.seedark_mag );

    // clear out the tint
    weight_sum = 0;
    for ( i = 0; i < 4; i++ ) tint[i] = 0;

    if ( HAS_SOME_BITS( bits, CHR_SOLID ) )
    {
        // solid characters are not blended onto the canvas
        // the alpha channel is not important
        weight_sum += 1.0f;

        tint[RR] += 1.0f / ( 1 << local_redshift );
        tint[GG] += 1.0f / ( 1 << local_grnshift );
        tint[BB] += 1.0f / ( 1 << local_blushift );
        tint[AA] += 1.0f;
    }

    if ( HAS_SOME_BITS( bits, CHR_ALPHA ) )
    {
        // alpha characters are blended onto the canvas using the alpha channel
        // the alpha channel is not important
        weight_sum += 1.0f;

        tint[RR] += 1.0f / ( 1 << local_redshift );
        tint[GG] += 1.0f / ( 1 << local_grnshift );
        tint[BB] += 1.0f / ( 1 << local_blushift );
        tint[AA] += local_alpha * INV_FF;
    }

    if ( HAS_SOME_BITS( bits, CHR_LIGHT ) )
    {
        // alpha characters are blended onto the canvas by adding their color
        // the more black the colors, the less visible the character
        // the alpha channel is not important

        weight_sum += 1.0f;

        if ( local_light < 255 )
        {
            tint[RR] += local_light * INV_FF / ( 1 << local_redshift );
            tint[GG] += local_light * INV_FF / ( 1 << local_grnshift );
            tint[BB] += local_light * INV_FF / ( 1 << local_blushift );
        }

        tint[AA] += 1.0f;
    }

    if ( HAS_SOME_BITS( bits, CHR_PHONG ) )
    {
        // phong is essentially the same as light, but it is the
        // sheen that sets the effect

        float amount;

        weight_sum += 1.0f;

        amount = ( CLIP( local_sheen, 0, 15 ) << 4 ) / 240.0f;

        tint[RR] += amount;
        tint[GG] += amount;
        tint[BB] += amount;
        tint[AA] += 1.0f;
    }

    // average the tint
    if ( weight_sum != 0.0f && weight_sum != 1.0f )
    {
        for ( i = 0; i < 4; i++ )
        {
            tint[i] /= weight_sum;
        }
    }
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_lowest_attachment( const CHR_REF ichr, bool_t non_item )
{
    /// @details BB@> Find the lowest attachment for a given object.
    ///               This was basically taken from the script function scr_set_TargetToLowestTarget()
    ///
    ///               You should be able to find the holder of a weapon by specifying non_item == btrue
    ///
    ///               To prevent possible loops in the data structures, use a counter to limit
    ///               the depth of the search, and make sure that ichr != ChrList.lst[object].attachedto

    int cnt;
    CHR_REF original_object, object, object_next;

    if ( !INGAME_CHR( ichr ) ) return ( CHR_REF )MAX_CHR;

    original_object = object = ichr;
    for ( cnt = 0, object = ichr; cnt < MAX_CHR && INGAME_CHR( object ); cnt++ )
    {
        object_next = ChrList.lst[object].attachedto;

        if ( non_item && !ChrList.lst[object].isitem )
        {
            break;
        }

        // check for a list with a loop. shouldn't happen, but...
        if ( !INGAME_CHR( object_next ) || object_next == original_object )
        {
            break;
        }

        object = object_next;
    }

    return object;
}

//--------------------------------------------------------------------------------------------
bool_t chr_can_mount( const CHR_REF ichr_a, const CHR_REF ichr_b )
{
    bool_t is_valid_rider_a, is_valid_mount_b, has_ride_anim;
    int action_mi;

    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    // make sure that A is valid
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    pcap_a = chr_get_pcap( ichr_a );
    if ( NULL == pcap_a ) return bfalse;

    // make sure that B is valid
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    pcap_b = chr_get_pcap( ichr_b );
    if ( NULL == pcap_b ) return bfalse;

    action_mi = mad_get_action_ref( chr_get_imad( ichr_a ), ACTION_MI );
    has_ride_anim = ( ACTION_COUNT != action_mi && !ACTION_IS_TYPE( action_mi, D ) );

    is_valid_rider_a = !pchr_a->isitem && pchr_a->alive && ( 0 == pchr_a->flyheight ) &&
                       !INGAME_CHR( pchr_a->attachedto ) && has_ride_anim;

    is_valid_mount_b = pchr_b->ismount && pchr_b->alive &&
                       pcap_b->slotvalid[SLOT_LEFT] && !INGAME_CHR( pchr_b->holdingwhich[SLOT_LEFT] );

    return is_valid_rider_a && is_valid_mount_b;
}

//--------------------------------------------------------------------------------------------
Uint32 chr_get_framefx( chr_t * pchr )
{
    if ( !DEFINED_PCHR( pchr ) ) return 0;

    return chr_instance_get_framefx( &( pchr->inst ) );
};

//--------------------------------------------------------------------------------------------
egoboo_rv chr_invalidate_child_instances( chr_t * pchr )
{
    int cnt;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    // invalidate vlst_cache of everything in this character's holdingwhich array
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        CHR_REF iitem = pchr->holdingwhich[cnt];
        if ( !INGAME_CHR( iitem ) ) continue;

        // invalidate the matrix_cache
        ChrList.lst[iitem].inst.matrix_cache.valid = bfalse;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_set_action( chr_t * pchr, int action, bool_t action_ready, bool_t override_action )
{
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egoboo_rv )chr_instance_set_action( &( pchr->inst ), action, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_start_anim( chr_t * pchr, int action, bool_t action_ready, bool_t override_action )
{
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egoboo_rv )chr_instance_start_anim( &( pchr->inst ), action, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_set_anim( chr_t * pchr, int action, int frame, bool_t action_ready, bool_t override_action )
{
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egoboo_rv )chr_instance_set_anim( &( pchr->inst ), action, frame, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_increment_action( chr_t * pchr )
{
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egoboo_rv )chr_instance_increment_action( &( pchr->inst ) );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_increment_frame( chr_t * pchr )
{
    egoboo_rv retval;
    mad_t * pmad;
    int mount_action;
    CHR_REF imount;
    bool_t needs_keep;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;
    imount = pchr->attachedto;

    pmad = chr_get_pmad( GET_REF_PCHR( pchr ) );
    if ( NULL == pmad ) return rv_error;

    // do we need to keep this animation?
    needs_keep = bfalse;

    if ( !INGAME_CHR( imount ) )
    {
        imount = ( CHR_REF )MAX_CHR;
        mount_action = ACTION_DA;
    }
    else
    {
        // determine what kind of action we are going to substitute for a riding character
        if ( INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) || INGAME_CHR( pchr->holdingwhich[SLOT_RIGHT] ) )
        {
            // if the character is holding anything, make the animation
            // ACTION_MH == "sitting" so that it does not look so silly

            mount_action = mad_get_action_ref( pchr->inst.imad, ACTION_MH );
            if ( ACTION_MH != mount_action )
            {
                // no real sitting animation. set the animation to keep
                needs_keep = btrue;
            }
        }
        else
        {
            // if it is not holding anything, go for the riding animation
            mount_action = mad_get_action_ref( pchr->inst.imad, ACTION_MI );
            if ( ACTION_MI != mount_action )
            {
                // no real riding animation. set the animation to keep
                needs_keep = btrue;
            }
        }
    }

    retval = ( egoboo_rv )chr_instance_increment_frame( &( pchr->inst ), pmad, imount, mount_action );
    if ( rv_success != retval ) return retval;

    // BB@> this did not work as expected...
    // set keep if needed
    //if ( needs_keep )
    //{
    //    chr_instance_set_action_keep( &( pchr->inst ), btrue );
    //}

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_play_action( chr_t * pchr, int action, bool_t action_ready )
{
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = ( egoboo_rv )chr_instance_play_action( &( pchr->inst ), action, action_ready );
    if ( rv_success != retval ) return retval;

    // if the instance is invalid, invalidate everything that depends on this object
    if ( !pchr->inst.save.valid )
    {
        chr_invalidate_child_instances( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
MAD_REF chr_get_imad( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return ( MAD_REF )MAX_MAD;
    pchr = ChrList.lst + ichr;

    // try to repair a bad model if it exists
    if ( !LOADED_MAD( pchr->inst.imad ) )
    {
        MAD_REF imad_tmp = pro_get_imad( pchr->profile_ref );
        if ( LOADED_MAD( imad_tmp ) )
        {
            if ( chr_instance_set_mad( &( pchr->inst ), imad_tmp ) )
            {
                chr_update_collision_size( pchr, btrue );
            }
        }
        if ( !LOADED_MAD( pchr->inst.imad ) ) return ( MAD_REF )MAX_MAD;
    }

    return pchr->inst.imad;
}

//--------------------------------------------------------------------------------------------
mad_t * chr_get_pmad( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    // try to repair a bad model if it exists
    if ( !LOADED_MAD( pchr->inst.imad ) )
    {
        MAD_REF imad_tmp = pro_get_imad( pchr->profile_ref );
        if ( LOADED_MAD( imad_tmp ) )
        {
            chr_instance_set_mad( &( pchr->inst ), imad_tmp );
        }
    }

    if ( !LOADED_MAD( pchr->inst.imad ) ) return NULL;

    return MadStack.lst + pchr->inst.imad;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t chr_update_pos( chr_t * pchr )
{
    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    pchr->onwhichgrid   = mesh_get_grid( PMesh, pchr->pos.x, pchr->pos.y );
    pchr->onwhichblock  = mesh_get_block( PMesh, pchr->pos.x, pchr->pos.y );

    // update whether the current character position is safe
    chr_update_safe( pchr, bfalse );

    // update the breadcrumb list
    chr_update_breadcrumb( pchr, bfalse );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_set_pos( chr_t * pchr, fvec3_base_t pos )
{
    bool_t retval = bfalse;

    if ( !ALLOCATED_PCHR( pchr ) ) return retval;

    retval = btrue;

    if (( pos[kX] != pchr->pos.v[kX] ) || ( pos[kY] != pchr->pos.v[kY] ) || ( pos[kZ] != pchr->pos.v[kZ] ) )
    {
        memmove( pchr->pos.v, pos, sizeof( fvec3_base_t ) );

        retval = chr_update_pos( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_set_maxaccel( chr_t * pchr, float new_val )
{
    bool_t retval = bfalse;
    float ftmp;

    if ( !ALLOCATED_PCHR( pchr ) ) return retval;

    ftmp = pchr->maxaccel / pchr->maxaccel_reset;
    pchr->maxaccel_reset = new_val;
    pchr->maxaccel = ftmp * pchr->maxaccel_reset;

    return btrue;
}

//--------------------------------------------------------------------------------------------
chr_t * chr_set_ai_state( chr_t * pchr, int state )
{
    if ( !DEFINED_PCHR( pchr ) ) return pchr;

    pchr->ai.state = state;

    chr_update_hide( pchr );

    return pchr;
}

//--------------------------------------------------------------------------------------------
bool_t chr_calc_grip_cv( chr_t * pmount, int grip_offset, oct_bb_t * grip_cv_ptr, fvec3_base_t grip_origin, fvec3_base_t grip_up, bool_t shift_origin )
{
    /// @details BB@> use a standard size for the grip

    // take the character size from the adventurer model
    const float default_chr_height = 88.0f;
    const float default_chr_radius = 22.0f;

    int              cnt;
    chr_instance_t * pmount_inst;
    oct_bb_t         tmp_cv = OCT_BB_INIT_VALS;

    int     grip_count;
    Uint16  grip_verts[GRIP_VERTS];
    fvec4_t grip_points[GRIP_VERTS];
    fvec4_t grip_nupoints[GRIP_VERTS];
    bumper_t bmp;

    if ( !DEFINED_PCHR( pmount ) ) return bfalse;

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
        if ( vert_stt < 0 ) return bfalse;

        if ( rv_error == chr_instance_update_vertices( pmount_inst, vert_stt, vert_stt + grip_offset, bfalse ) )
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
    if ( NULL == grip_up )
    {
        // we only need one vertex
        TransformVertices( &( pmount_inst->matrix ), grip_points, grip_nupoints, 1 );
    }
    else
    {
        // transform all the vertices
        TransformVertices( &( pmount_inst->matrix ), grip_points, grip_nupoints, GRIP_VERTS );
    }

    // find the up vector, if needed
    if ( NULL != grip_up )
    {
        fvec3_t grip_vecs[3];

        // determine the grip vectors
        for ( cnt = 0; cnt < 3; cnt++ )
        {
            grip_vecs[cnt] = fvec3_sub( grip_nupoints[cnt + 1].v, grip_nupoints[0].v );
        }

        // grab the grip's "up" vector
        fvec3_base_assign( grip_up, fvec3_normalize( grip_vecs[2].v ) );
    }

    // save the origin, if necessary
    if ( NULL != grip_origin )
    {
        fvec3_base_copy( grip_origin, grip_nupoints[0].v );
    }

    // add in the "origin" of the grip, if necessary
    if ( NULL != grip_cv_ptr )
    {
        oct_bb_add_fvec3( &tmp_cv, grip_nupoints[0].v, grip_cv_ptr );
    }

    return btrue;
}

