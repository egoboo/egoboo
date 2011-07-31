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

/// @file script_implementation.c
/// @brief implementation of the internal egoscript functions
/// @details These are the c-functions that are called to implement the egoscript commsnds
/// At some pont, the non-trivial commands in this file will be broken out to discrete functions
/// and wrapped by some external language (like Lua) using something like SWIG, and a cross compiler written.
/// The current code is about 3/4 of the way toward this goal.
/// The functions below will then be replaced with stub calls to the "real" functions.

#include "script_implementation.h"

#include "game.h"
#include "AStar.h"
#include "passage.h"

#include "ChrList.inl"
#include "mesh.inl"
#include "profile.inl"

#include <egolib/egoboo_setup.h>
#include <egolib/strutil.h>
#include <egolib/_math.inl>

//--------------------------------------------------------------------------------------------
// wrap generic bitwise conversion macros
//--------------------------------------------------------------------------------------------
BIT_FIELD bool_to_bit( bool_t val )
{
    return BOOL_TO_BIT( val );
}

//--------------------------------------------------------------------------------------------
bool_t bit_to_bool( BIT_FIELD val )
{
    return BIT_TO_BOOL( val );
}

//--------------------------------------------------------------------------------------------
Uint8  BIT_FIELD_clip_to_08_bits( BIT_FIELD val )  { return val & 0xFF;      }

//--------------------------------------------------------------------------------------------
Uint16 BIT_FIELD_clip_to_16_bits( BIT_FIELD val )  { return val & 0xFFFF;     }

//--------------------------------------------------------------------------------------------
Uint32 BIT_FIELD_clip_to_24_bits( BIT_FIELD val )  { return val & 0xFFFFFF;   }

//--------------------------------------------------------------------------------------------
Uint32 BIT_FIELD_clip_to_32_bits( BIT_FIELD val )  { return val & 0xFFFFFFFF; }

//--------------------------------------------------------------------------------------------
// wrap the BIT_FIELD macros, since lua doesn't recognize bitwise functions
//--------------------------------------------------------------------------------------------

BIT_FIELD BIT_FIELD_fill( BIT_FIELD val )
{
    return FULL_BIT_FIELD;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_empty( BIT_FIELD val )
{
    return EMPTY_BIT_FIELD;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_set_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    SET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_unset_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    UNSET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
bool_t BIT_FIELD_has_some_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_SOME_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
bool_t BIT_FIELD_has_all_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_ALL_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
bool_t BIT_FIELD_has_no_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_NO_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
bool_t BIT_FIELD_missing_bits( BIT_FIELD val, BIT_FIELD test )
{
    return HAS_SOME_BITS( val, test ) && !HAS_ALL_BITS( val, test );
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_set_one_bit( BIT_FIELD val, size_t which )
{
    const size_t bit_count = 8 * sizeof( BIT_FIELD );

    BIT_FIELD new_val = val;

    if ( which >= 0 && which < bit_count )
    {
        new_val = SET_BIT( new_val, 1 << which );
    }

    return new_val;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_clear_one_bit( BIT_FIELD val, size_t which )
{
    const size_t bit_count = 8 * sizeof( BIT_FIELD );

    BIT_FIELD new_val = val;

    if ( which >= 0 && which < bit_count )
    {
        new_val = UNSET_BIT( new_val, 1 << which );
    }

    return new_val;
}

//--------------------------------------------------------------------------------------------
bool_t BIT_FIELD_test_one_bit( BIT_FIELD val, size_t which )
{
    const size_t bit_count = 8 * sizeof( BIT_FIELD );

    bool_t retval = bfalse;

    if ( which >= 0 && which < bit_count )
    {
        retval = HAS_ALL_BITS( val, 1 << which );
    }

    return retval;
}
//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_set_all_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    SET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD BIT_FIELD_clear_all_bits( BIT_FIELD val, BIT_FIELD bits )
{
    BIT_FIELD new_val = val;

    UNSET_BIT( new_val, bits );

    return new_val;
}

//--------------------------------------------------------------------------------------------
bool_t BIT_FIELD_test_all_bits( BIT_FIELD val, BIT_FIELD bits )
{
    return HAS_ALL_BITS( val, bits );
}

//--------------------------------------------------------------------------------------------
// waypoint_list_t
//--------------------------------------------------------------------------------------------
bool_t waypoint_list_peek( waypoint_list_t * plst, waypoint_t wp )
{
    int index;

    // is the list valid?
    if ( NULL == plst || plst->tail >= MAXWAY ) return bfalse;

    // is the list is empty?
    if ( 0 == plst->head ) return bfalse;

    if ( plst->tail > plst->head )
    {
        // fix the tail
        plst->tail = plst->head;

        // we have passed the last waypoint
        // just tell them the previous waypoint
        index = plst->tail - 1;
    }
    else if ( plst->tail == plst->head )
    {
        // we have passed the last waypoint
        // just tell them the previous waypoint
        index = plst->tail - 1;
    }
    else
    {
        // tell them the current waypoint
        index = plst->tail;
    }

    wp[kX] = plst->pos[index][kX];
    wp[kY] = plst->pos[index][kY];
    wp[kZ] = plst->pos[index][kZ];

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_push( waypoint_list_t * plst, int x, int y )
{
    /// @details BB@> Add a waypoint to the waypoint list

    if ( NULL == plst ) return bfalse;

    // add the value
    plst->pos[plst->head][kX] = x;
    plst->pos[plst->head][kY] = y;
    plst->pos[plst->head][kZ] = 0;

    // do not let the list overflow
    plst->head++;
    if ( plst->head >= MAXWAY ) plst->head = MAXWAY - 1;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_reset( waypoint_list_t * plst )
{
    /// @details BB@> reset the waypoint list to the beginning

    if ( NULL == plst ) return bfalse;

    plst->tail = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_clear( waypoint_list_t * plst )
{
    /// @details BB@> Clear out all waypoints

    if ( NULL == plst ) return bfalse;

    plst->tail = 0;
    plst->head = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_empty( waypoint_list_t * plst )
{
    if ( NULL == plst ) return btrue;

    return 0 == plst->head;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_finished( waypoint_list_t * plst )
{
    if ( NULL == plst || 0 == plst->head ) return btrue;

    return plst->tail == plst->head;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_advance( waypoint_list_t * plst )
{
    bool_t retval;

    if ( NULL == plst ) return bfalse;

    retval = bfalse;
    if ( plst->tail > plst->head )
    {
        // fix the tail
        plst->tail = plst->head;
    }
    else if ( plst->tail < plst->head )
    {
        // advance the tail
        plst->tail++;
        retval = btrue;
    }

    // clamp the tail to valid values
    if ( plst->tail >= MAXWAY ) plst->tail = MAXWAY - 1;

    return retval;
}

//--------------------------------------------------------------------------------------------
// line_of_sight_info_t
//--------------------------------------------------------------------------------------------

bool_t line_of_sight_do( line_of_sight_info_t * plos )
{
    bool_t mesh_hit = bfalse, chr_hit = bfalse;
    mesh_hit = line_of_sight_with_mesh( plos );

    /*if ( mesh_hit )
    {
        plos->x1 = (plos->collide_x + 0.5f) * GRID_FSIZE;
        plos->y1 = (plos->collide_y + 0.5f) * GRID_FSIZE;
    }

    chr_hit = line_of_sight_with_characters( plos );
    */

    return mesh_hit || chr_hit;
}

//--------------------------------------------------------------------------------------------
bool_t line_of_sight_with_mesh( line_of_sight_info_t * plos )
{
    Uint32 fan_last;

    int Dx, Dy;
    int ix, ix_stt, ix_end;
    int iy, iy_stt, iy_end;

    int Dbig, Dsmall;
    int ibig, ibig_stt, ibig_end;
    int ismall, ismall_stt, ismall_end;
    int dbig, dsmall;
    int TwoDsmall, TwoDsmallMinusTwoDbig, TwoDsmallMinusDbig;

    bool_t steep;

    if ( NULL == plos ) return bfalse;

    //is there any point of these calculations?
    if ( EMPTY_BIT_FIELD == plos->stopped_by ) return bfalse;

    ix_stt = FLOOR( plos->x0 / GRID_FSIZE );
    ix_end = FLOOR( plos->x1 / GRID_FSIZE );

    iy_stt = FLOOR( plos->y0 / GRID_FSIZE );
    iy_end = FLOOR( plos->y1 / GRID_FSIZE );

    Dx = plos->x1 - plos->x0;
    Dy = plos->y1 - plos->y0;

    steep = ( ABS( Dy ) >= ABS( Dx ) );

    // determine which are the big and small values
    if ( steep )
    {
        ibig_stt = iy_stt;
        ibig_end = iy_end;

        ismall_stt = ix_stt;
        ismall_end = ix_end;
    }
    else
    {
        ibig_stt = ix_stt;
        ibig_end = ix_end;

        ismall_stt = iy_stt;
        ismall_end = iy_end;
    }

    // set up the big loop variables
    dbig = 1;
    Dbig = ibig_end - ibig_stt;
    if ( Dbig < 0 )
    {
        dbig = -1;
        Dbig = -Dbig;
        ibig_end--;
    }
    else
    {
        ibig_end++;
    }

    // set up the small loop variables
    dsmall = 1;
    Dsmall = ismall_end - ismall_stt;
    if ( Dsmall < 0 )
    {
        dsmall = -1;
        Dsmall = -Dsmall;
    }

    // pre-compute some common values
    TwoDsmall             = 2 * Dsmall;
    TwoDsmallMinusTwoDbig = TwoDsmall - 2 * Dbig;
    TwoDsmallMinusDbig    = TwoDsmall - Dbig;

    fan_last = INVALID_TILE;
    for ( ibig = ibig_stt, ismall = ismall_stt;  ibig != ibig_end;  ibig += dbig )
    {
        Uint32 fan;

        if ( steep )
        {
            ix = ismall;
            iy = ibig;
        }
        else
        {
            ix = ibig;
            iy = ismall;
        }

        // check to see if the "ray" collides with the mesh
        fan = mesh_get_tile_int( PMesh, ix, iy );
        if ( INVALID_TILE != fan && fan != fan_last )
        {
            Uint32 collide_fx = mesh_test_fx( PMesh, fan, plos->stopped_by );
            // collide the ray with the mesh

            if ( EMPTY_BIT_FIELD != collide_fx )
            {
                plos->collide_x  = ix;
                plos->collide_y  = iy;
                plos->collide_fx = collide_fx;

                return btrue;
            }

            fan_last = fan;
        }

        // go to the next step
        if ( TwoDsmallMinusDbig > 0 )
        {
            TwoDsmallMinusDbig += TwoDsmallMinusTwoDbig;
            ismall             += dsmall;
        }
        else
        {
            TwoDsmallMinusDbig += TwoDsmall;
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t line_of_sight_with_characters( line_of_sight_info_t * plos )
{

    if ( NULL == plos ) return bfalse;

    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        // do line/character intersection
    }
    CHR_END_LOOP();

    return bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t AddWaypoint( waypoint_list_t * plst, CHR_REF ichr, float pos_x, float pos_y )
{
    // AddWaypoint( tmpx = "x position", tmpy = "y position" )
    /// @details ZZ@> This function tells the character where to move next

    bool_t returncode;

    if ( NULL == plst ) return bfalse;

#if defined(_DEBUG) && defined(DEBUG_WAYPOINTS)
    cap_t * pcap;
    fvec2_t loc_pos;
    fvec3_t nrm;
    float   pressure;

    // init the vector with the desired position
    loc_pos.x = pos_x;
    loc_pos.y = pos_y;

    // is this a safe position?
    returncode = bfalse;

    pcap = chr_get_pcap( ichr );
    if ( NULL != pcap )
    {
        if ( CAP_INFINITE_WEIGHT == pcap->weight || !mesh_hit_wall( PMesh, loc_pos.v, pchr->bump.size, pchr->stoppedby, nrm.v, &pressure, NULL ) )
        {
            // yes it is safe. add it.
            returncode = waypoint_list_push( plst, pos_x, pos_y );
        }
        else
        {
            // no it is not safe. what to do? nothing, or add the current position?
            //returncode = waypoint_list_push( plst, pchr->loc_pos.x, pchr->loc_pos.y );

            log_warning( "scr_AddWaypoint() - failed to add a waypoint because object was \"inside\" a wall.\n"
                         "\tcharacter %d (\"%s\", \"%s\")\n"
                         "\tWaypoint index %d\n"
                         "\tWaypoint location (in tiles) <%f,%f>\n"
                         "\tWall normal <%1.4f,%1.4f>\n"
                         "\tPressure %f\n",
                         ichr, pchr->Name, pcap->name,
                         plst->head,
                         loc_pos.x / GRID_FSIZE, loc_pos.y / GRID_FSIZE,
                         nrm.x, nrm.y,
                         SQRT( pressure ) / GRID_FSIZE );
        }
    }
#else
    returncode = waypoint_list_push( plst, pos_x, pos_y );
#endif

    return returncode;
}

//--------------------------------------------------------------------------------------------
bool_t FindPath( waypoint_list_t * plst, chr_t * pchr, float dst_x, float dst_y, bool_t * used_astar_ptr )
{
    // FindPath
    /// @details ZF@> Ported the A* path finding algorithm by birdsey and heavily modified it
    // This function adds enough waypoints to get from one point to another

    int src_ix, src_iy;
    int dst_ix, dst_iy;
    line_of_sight_info_t los_info;
    bool_t straight_line;
    bool_t returncode = bfalse;

    if ( NULL != used_astar_ptr )
    {
        *used_astar_ptr = bfalse;
    }

    //Our current position
    src_ix = ( int )pchr->pos.x / GRID_ISIZE;
    src_iy = ( int )pchr->pos.y / GRID_ISIZE;

    //Destination position
    dst_ix = dst_x / GRID_ISIZE;
    dst_iy = dst_y / GRID_ISIZE;

    //Always clear any old waypoints
    waypoint_list_clear( plst );

    //Don't do need to do anything if there is no need to move
    if ( src_ix == dst_ix && src_iy == dst_iy ) return bfalse;

    returncode = bfalse;

    //setup line of sight data for source
    los_info.stopped_by = pchr->stoppedby;
    los_info.x0 = pchr->pos.x;
    los_info.y0 = pchr->pos.y;
    los_info.z0 = 0;

    //setup line of sight to target
    los_info.x1 = dst_x;
    los_info.y1 = dst_y;
    los_info.z1 = 0;

    // test for the simple case... a straight line
    straight_line = !line_of_sight_do( &los_info );

    if ( !straight_line )
    {
#ifdef DEBUG_ASTAR
        printf( "Finding a path from %d,%d to %d,%d: \n", src_ix, src_iy, dst_ix, dst_iy );
#endif
        //Try to find a path with the AStar algorithm
        if ( AStar_find_path( PMesh, pchr->stoppedby, src_ix, src_iy, dst_ix, dst_iy ) )
        {
            returncode = AStar_get_path( dst_x, dst_y, plst );
        }

        if ( NULL != used_astar_ptr )
        {
            *used_astar_ptr = btrue;
        }
    }

    //failed to find a path
    if ( !returncode )
    {
        // just use a straight line path
        waypoint_list_push( plst, dst_x, dst_y );
    }

    return returncode;
}

//--------------------------------------------------------------------------------------------
bool_t Compass( fvec2_base_t pos, int facing, float distance )
{
    // Compass( tmpturn = "rotation", tmpdistance = "radius" )

    /// @details ZZ@> This function modifies tmpx and tmpy, depending on the setting of
    /// tmpdistance and tmpturn.  It acts like one of those Compass thing
    /// with the two little needle legs

    TURN_T turn;

    turn = TO_TURN( facing );

    pos[XX] -= turntocos[ turn ] * distance;
    pos[YY] -= turntosin[ turn ] * distance;

    return btrue;
}

//--------------------------------------------------------------------------------------------
int GetArmorPrice( chr_t * pchr, int skin )
{
    // tmpx = GetTargetArmorPrice( tmpargument = "skin" )
    /// @details ZZ@> This function returns the cost of the desired skin upgrade, setting
    /// tmpx to the price

    cap_t * pcap;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return -1;

    skin %= MAX_SKIN;

    return pcap->skincost[skin];
}

//--------------------------------------------------------------------------------------------
Uint32 UpdateTime( Uint32 time_val, int delay )
{
    // UpdateTime( tmpargument = "time" )
    /// @details ZZ@> This function sets the character's ai timer.  50 clicks per second.
    /// Used in conjunction with IfTimeOut

    Uint32 new_time_val;

    if ( delay <= 0 )
    {
        new_time_val = time_val;
    }
    else
    {
        new_time_val = update_wld + delay;
    }

    return new_time_val;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 _break_passage( int mesh_fx_or, int become, int frames, int starttile, const PASS_REF passage, int *ptilex, int *ptiley )
{
    /// @details ZZ@> This function breaks the tiles of a passage if there is a character standing
    ///               on 'em.  Turns the tiles into damage terrain if it reaches last frame.

    Uint32 endtile;
    Uint32 fan;
    bool_t       useful;
    ego_tile_info_t * ptile = NULL;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    // limit the start tile the the 256 tile images that we have
    starttile = CLIP_TO_08BITS( starttile );

    // same with the end tile
    endtile   =  starttile + frames - 1;
    endtile = CLIP( endtile, 0, 255 );

    useful = bfalse;
    CHR_BEGIN_LOOP_ACTIVE( character, pchr )
    {
        float lerp_z;

        // nothing in packs
        if ( IS_ATTACHED_CHR( pchr->ai.index ) ) continue;

        // nothing flying
        if ( 0 != pchr->flyheight ) continue;

        lerp_z = ( pchr->pos.z - pchr->enviro.floor_level ) / DAMAGERAISE;
        lerp_z = 1.0f - CLIP( lerp_z, 0.0f, 1.0f );

        if ( pchr->phys.weight * lerp_z <= 20 ) continue;

        fan = mesh_get_grid( PMesh, pchr->pos.x, pchr->pos.y );

        ptile = mesh_get_ptile( PMesh, fan );
        if ( NULL != ptile )
        {
            Uint16 img      = ptile->img & TILE_LOWER_MASK;
            Uint16 highbits = ptile->img & TILE_UPPER_MASK;

            if ( img >= starttile && img < endtile )
            {
                if ( object_is_in_passage(( PASS_REF )passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
                {
                    // Remember where the hit occured.
                    *ptilex = pchr->pos.x;
                    *ptiley = pchr->pos.y;

                    useful = btrue;

                    // Change the tile image
                    img++;
                }
            }

            if ( img == endtile )
            {
                useful = mesh_add_fx( PMesh, fan, mesh_fx_or );

                if ( become != 0 )
                {
                    img = become;
                }
            }

            if ( ptile->img != ( img | highbits ) )
            {
                mesh_set_texture( PMesh, fan, img | highbits );
            }
        }
    }
    CHR_END_LOOP();

    return useful;
}

//--------------------------------------------------------------------------------------------
Uint8 _append_end_text( chr_t * pchr, const int message_index, script_state_t * pstate )
{
    /// @details ZZ@> This function appends a message to the end-module text

    size_t length;
    CHR_REF ichr;
    pro_t *ppro;
    char * dst, * dst_end;

    Uint8 returncode = btrue;

    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    if ( !IS_VALID_MESSAGE_PRO( pchr->profile_ref, message_index ) ) return bfalse;
    ppro = ProList_get_ptr( pchr->profile_ref );

    ichr           = GET_REF_PCHR( pchr );
    length = strlen( ppro->message[message_index] );

    dst     = endtext + endtext_carat;
    dst_end = endtext + MAXENDTEXT - 1;

    expand_escape_codes( ichr, pstate, ppro->message[message_index], ppro->message[message_index] + length, dst, dst_end );
    endtext_carat = strlen( endtext );

    str_add_linebreaks( endtext, strlen( endtext ), 30 );

    return returncode;
}

//--------------------------------------------------------------------------------------------
Uint8 _find_grid_in_passage( const int x0, const int y0, const int tiletype, const PASS_REF passage, int *px1, int *py1 )
{
    /// @details ZZ@> This function finds the next tile in the passage, x0 and y0
    ///    must be set first, and are set on a find.  Returns btrue or bfalse
    ///    depending on if it finds one or not

    int x, y;
    Uint32 fan;
    passage_t  * ppass = NULL;
    ego_tile_info_t * ptile = NULL;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;
    ppass = PassageStack_get_ptr( passage );

    // Do the first row
    x = x0 / GRID_ISIZE;
    y = y0 / GRID_ISIZE;

    if ( x < ppass->area.left )  x = ppass->area.left;
    if ( y < ppass->area.top )  y = ppass->area.top;

    if ( y < ppass->area.bottom )
    {
        for ( /*nothing*/; x <= ppass->area.right; x++ )
        {
            fan = mesh_get_tile_int( PMesh, x, y );

            ptile = mesh_get_ptile( PMesh, fan );
            if ( NULL != ptile && tiletype == ( ptile->img & TILE_LOWER_MASK ) )
            {
                *px1 = ( x * GRID_ISIZE ) + 64;
                *py1 = ( y * GRID_ISIZE ) + 64;
                return btrue;
            }
        }
        y++;
    }

    // Do all remaining rows
    for ( /* nothing */; y <= ppass->area.bottom; y++ )
    {
        for ( x = ppass->area.left; x <= ppass->area.right; x++ )
        {
            fan = mesh_get_tile_int( PMesh, x, y );

            ptile = mesh_get_ptile( PMesh, fan );
            if ( NULL != ptile && tiletype == ( ptile->img & TILE_LOWER_MASK ) )
            {
                *px1 = x * GRID_ISIZE + 64;
                *py1 = y * GRID_ISIZE + 64;
                return btrue;
            }
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint8 _display_message( const CHR_REF ichr, const PRO_REF iprofile, const int message, script_state_t * pstate )
{
    /// @details ZZ@> This function sticks a message_offset in the display queue and sets its timer

    int slot;
    char * dst, * dst_end;
    size_t length;
    pro_t *ppro;

    if ( !IS_VALID_MESSAGE_PRO( iprofile, message ) ) return bfalse;
    ppro = ProList_get_ptr( iprofile );

    slot = DisplayMsg_get_free();
    DisplayMsg.ary[slot].time = cfg.message_duration;

    length = strlen( ppro->message[message] );

    dst     = DisplayMsg.ary[slot].textdisplay;
    dst_end = DisplayMsg.ary[slot].textdisplay + MESSAGESIZE - 1;

    expand_escape_codes( ichr, pstate, ppro->message[message], ppro->message[message] + length, dst, dst_end );

    *dst_end = CSTR_END;

    return btrue;
}
