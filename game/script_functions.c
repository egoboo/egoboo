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

/// @file script_functions.c
/// @brief implementation of the internal egoscript functions
/// @details These are the c-functions that are called to implement the egoscript commsnds
/// At some pont, the non-trivial commands in this file will be broken out to discrete functions
/// and wrapped by some external language (like Lua) using something like SWIG, and a cross compiler written.
/// The current code is about 3/4 of the way toward this goal.
/// The functions below will then be replaced with stub calls to the "real" functions.

#include "script_functions.h"

#include "profile.h"
#include "enchant.h"
#include "char.h"
#include "particle.h"
#include "mad.h"

#include "link.h"
#include "camera.h"
#include "passage.h"
#include "graphic.h"
#include "input.h"
#include "network.h"
#include "game.h"
#include "log.h"

#include "spawn_file.h"
#include "quest.h"

#include "SDL_extensions.h"

#include "egoboo_strutil.h"
#include "egoboo_setup.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// turn off this annoying warning
#if defined _MSC_VER
#    pragma warning(disable : 4189) // local variable is initialized but not referenced
#endif

#define SCRIPT_FUNCTION_BEGIN() \
    chr_t * pchr; \
    pro_t * ppro; \
    Uint16 sTmp = 0; \
    Uint8 returncode = btrue; \
    if( NULL == pstate || NULL == pself || !ALLOCATED_CHR(pself->index) ) return bfalse; \
    pchr = ChrList.lst + pself->index; \
    if( !LOADED_PRO(pchr->iprofile) ) return bfalse; \
    ppro = ProList.lst + pchr->iprofile;

#define SCRIPT_FUNCTION_END() \
    return returncode;

#define FUNCTION_BEGIN() \
    Uint16 sTmp = 0; \
    Uint8 returncode = btrue; \
    if( !ALLOCATED_PCHR( pchr ) ) return bfalse;

#define FUNCTION_END() \
    return returncode;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// @defgroup _bitwise_functions_ Bitwise Scripting Functions
/// @details These functions may be necessary to export the bitwise functions for handling alerts to
///  scripting languages where there is no support for bitwise operators (Lua, tcl, ...)

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_set_AlertBit( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Sets the bit in the 32-bit integer self.alert indexed by pstate->argument

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument >= 0 && pstate->argument < 32 )
    {
        pself->alert |= ( 1 << pstate->argument );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_ClearAlertBit( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Clears the bit in the 32-bit integer self.alert indexed by pstate->argument

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument >= 0 && pstate->argument < 32 )
    {
        pself->alert &= ~( 1 << pstate->argument );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_TestAlertBit( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Tests to see if the the bit in the 32-bit integer self.alert indexed by pstate->argument is non-zero

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument >= 0 && pstate->argument < 32 )
    {
        returncode = HAS_SOME_BITS( pself->alert,  1 << pstate->argument );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_set_Alert( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Sets one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pself->alert |= pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_ClearAlert( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Clears one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pself->alert &= ~pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_TestAlert( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Tests one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_set_Bit( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Sets the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->y >= 0 && pstate->y < 32 )
    {
        pstate->x |= ( 1 << pstate->y );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_ClearBit( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Clears the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->y >= 0 && pstate->y < 32 )
    {
        pstate->x &= ~( 1 << pstate->y );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_TestBit( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Tests the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->y >= 0 && pstate->y < 32 )
    {
        returncode = HAS_SOME_BITS( pstate->x, 1 << pstate->y );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_set_Bits( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Adds the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    pstate->x |= pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_ClearBits( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Clears the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    pstate->x &= ~pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
/// @ingroup _bitwise_functions_
Uint8 scr_TestBits( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> Tests the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pstate->x, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 scr_Spawned( script_state_t * pstate, ai_state_t * pself )
{
    // IfSpawned()
    /// @details ZZ@> This function proceeds if the character was spawned this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if it's a new character
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_SPAWNED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TimeOut( script_state_t * pstate, ai_state_t * pself )
{
    // IfTimeOut()
    /// @details ZZ@> This function proceeds if the character's aitime is 0.  Use
    /// in conjunction with set_Time

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if time alert is set
    returncode = ( update_wld > pself->timer );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AtWaypoint( script_state_t * pstate, ai_state_t * pself )
{
    // IfAtWaypoint()
    /// @details ZZ@> This function proceeds if the character reached its waypoint this
    /// update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character reached a waypoint
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_ATWAYPOINT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AtLastWaypoint( script_state_t * pstate, ai_state_t * pself )
{
    // IfAtLastWaypoint()
    /// @details ZZ@> This function proceeds if the character reached its last waypoint this
    /// update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character reached its last waypoint
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_ATLASTWAYPOINT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Attacked( script_state_t * pstate, ai_state_t * pself )
{
    // IfAttacked()
    /// @details ZZ@> This function proceeds if the character ( an item ) was put in its
    /// owner's pocket this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was damaged
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_ATTACKED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Bumped( script_state_t * pstate, ai_state_t * pself )
{
    // IfBumped()
    /// @details ZZ@> This function proceeds if the character was bumped by another character
    /// this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was bumped
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_BUMPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Ordered( script_state_t * pstate, ai_state_t * pself )
{
    // IfOrdered()
    /// @details ZZ@> This function proceeds if the character got an order from another
    /// character on its team this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was ordered
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_ORDERED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CalledForHelp( script_state_t * pstate, ai_state_t * pself )
{
    // IfCalledForHelp()
    /// @details ZZ@> This function proceeds if one of the character's teammates was nearly
    /// killed this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was called for help
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CALLEDFORHELP );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Content( script_state_t * pstate, ai_state_t * pself )
{
    // SetContent( tmpargument )
    /// @details ZZ@> This function sets the content variable.  Used in conjunction with
    /// GetContent.  Content is preserved from update to update

    SCRIPT_FUNCTION_BEGIN();

    // Set the content
    pself->content = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Killed( script_state_t * pstate, ai_state_t * pself )
{
    // IfKilled()
    /// @details ZZ@> This function proceeds if the character was killed this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character's been killed
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_KILLED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetKilled( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetKilled()
    /// @details ZZ@> This function proceeds if the character's target from last update was
    /// killed during this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character's target has just died or is already dead
    returncode = ( HAS_SOME_BITS( pself->alert, ALERTIF_TARGETKILLED ) || !ChrList.lst[pself->target].alive );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearWaypoints( script_state_t * pstate, ai_state_t * pself )
{
    // ClearWaypoints()
    /// @details ZZ@> This function is used to move a character around.  Do this before
    /// AddWaypoint

    SCRIPT_FUNCTION_BEGIN();

    returncode = waypoint_list_clear( &(pself->wp_lst) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddWaypoint( script_state_t * pstate, ai_state_t * pself )
{
    // AddWaypoint( tmpx = "x position", tmpy = "y position" )
    /// @details ZZ@> This function tells the character where to move next

    fvec2_t pos;

    SCRIPT_FUNCTION_BEGIN();

    // init the vector with the desired position
    pos.x = pstate->x;
    pos.y = pstate->y;

    // is this a safe position?
    returncode = bfalse;
    if( !mesh_hitawall( PMesh, pos.v, pchr->bump.size, pchr->stoppedby, NULL ) )
    {
        // yes it is safe. add it.
        returncode = waypoint_list_push( &(pself->wp_lst), pstate->x, pstate->y );
    }
    else
    {
        // no it is not safe. what to do? nothing, or add the current position?
        //returncode = waypoint_list_push( &(pself->wp_lst), pchr->pos.x, pchr->pos.y );
    }

    if( returncode )
    {
        // make sure we update the waypoint, since the list changed
        pself->wp_valid = waypoint_list_peek( &(pself->wp_lst), pself->wp ); 
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FindPath( script_state_t * pstate, ai_state_t * pself )
{
    // FindPath
    /// @details ZF@> This modifies the eay the character moves relative to its target. There is no pth finding at this time.
    /// @todo scr_FindPath doesn't really work yet

    SCRIPT_FUNCTION_BEGIN();

    // Yep this is it
    if ( pself->target != pself->index )
    {
        float fx, fy;

        if ( pstate->distance != MOVE_FOLLOW )
        {
            fx = ChrList.lst[ pself->target ].pos.x;
            fy = ChrList.lst[ pself->target ].pos.y;
        }
        else
        {
            fx = generate_randmask( -512, 1023 ) + ChrList.lst[ pself->target ].pos.x;
            fy = generate_randmask( -512, 1023 ) + ChrList.lst[ pself->target ].pos.y;
        }

        pstate->turn = vec_to_facing( fx - pchr->pos.x , fy - pchr->pos.y );

        if ( pstate->distance == MOVE_RETREAT )
        {
            // flip around to the other direction and add in some randomness
            pstate->turn += ATK_BEHIND + generate_randmask( -8192, 16383 );
        }
        pstate->turn = CLIP_TO_16BITS( pstate->turn );

        if ( pstate->distance == MOVE_CHARGE || pstate->distance == MOVE_RETREAT )
        {
            reset_character_accel( pself->index ); // Force 100% speed
        }

        // Then add the waypoint
        returncode = waypoint_list_push( &(pself->wp_lst), fx, fy );

        if( returncode )
        {
            // return the new position
            pstate->x = fx;
            pstate->y = fy;

            if( returncode )
            {
                // make sure we update the waypoint, since the list changed
                pself->wp_valid = waypoint_list_peek( &(pself->wp_lst), pself->wp ); 
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Compass( script_state_t * pstate, ai_state_t * pself )
{
    // Compass( tmpturn = "rotation", tmpdistance = "radius" )
    /// @details ZZ@> This function modifies tmpx and tmpy, depending on the setting of
    /// tmpdistance and tmpturn.  It acts like one of those Compass thing
    /// with the two little needle legs

    SCRIPT_FUNCTION_BEGIN();

    pstate->x -= turntocos[( pstate->turn >> 2 ) & TRIG_TABLE_MASK ] * pstate->distance;
    pstate->y -= turntosin[( pstate->turn >> 2 ) & TRIG_TABLE_MASK ] * pstate->distance;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetArmorPrice( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx = GetTargetArmorPrice( tmpargument = "skin" )
    /// @details ZZ@> This function returns the cost of the desired skin upgrade, setting
    /// tmpx to the price

    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    pcap = chr_get_pcap( pself->target );

    returncode = bfalse;
    if ( NULL != pcap )
    {
        sTmp = pstate->argument % MAX_SKIN;

        pstate->x = pcap->skincost[sTmp];
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Time( script_state_t * pstate, ai_state_t * pself )
{
    // SetTime( tmpargument = "time" )
    /// @details ZZ@> This function sets the character's ai timer.  50 clicks per second.
    /// Used in conjunction with IfTimeOut

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->argument > -1 )
    {
        pself->timer = update_wld + pstate->argument;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_Content( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetContent()
    /// @details ZZ@> This function sets tmpargument to the character's content variable.
    /// Used in conjunction with set_Content, or as a NOP to space out an Else

    SCRIPT_FUNCTION_BEGIN();

    // Get the content
    pstate->argument = pself->content;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinTargetTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinTargetTeam()
    /// @details ZZ@> This function lets a character join a different team.  Used
    /// mostly for pets

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ACTIVE_CHR( pself->target ) )
    {
        switch_team( pself->index, ChrList.lst[pself->target].team );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearbyEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearbyEnemy()
    /// @details ZZ@> This function sets the target to a nearby enemy, failing if there are none

    Uint16 ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = _get_chr_target( pchr, NEARBY, TARGET_ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse, bfalse );

    returncode = ( ichr != pself->index ) && ACTIVE_CHR( ichr );

    if ( returncode )
    {
        pself->target = ichr;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetLeftHand( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToTargetLeftHand()
    /// @details ZZ@> This function sets the target to the item in the target's left hand,
    /// failing if the target has no left hand item

    SCRIPT_FUNCTION_BEGIN();

    sTmp = ChrList.lst[pself->target].holdingwhich[SLOT_LEFT];
    returncode = bfalse;
    if ( ACTIVE_CHR( sTmp ) )
    {
        pself->target = sTmp;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetRightHand( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToTargetRightHand()
    /// @details ZZ@> This function sets the target to the item in the target's right hand,
    /// failing if the target has no right hand item

    SCRIPT_FUNCTION_BEGIN();

    sTmp = ChrList.lst[pself->target].holdingwhich[SLOT_RIGHT];
    returncode = bfalse;
    if ( ACTIVE_CHR( sTmp ) )
    {
        pself->target = sTmp;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverAttacked( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverAttacked()
    /// @details ZZ@> This function sets the target to whoever attacked the character last, failing for damage tiles

    SCRIPT_FUNCTION_BEGIN();
    if ( ACTIVE_CHR( pself->attacklast ) )
    {
        pself->target = pself->attacklast;
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverBumped( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverBumped()
    /// @details ZZ@> This function sets the target to whoever bumped the character last. It never fails

    SCRIPT_FUNCTION_BEGIN();

    pself->target = pself->bumplast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverCalledForHelp( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverCalledForHelp()
    /// @details ZZ@> This function sets the target to whoever called for help last.

    SCRIPT_FUNCTION_BEGIN();

    pself->target = TeamList[pchr->team].sissy;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToOldTarget( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToOldTarget()
    /// @details ZZ@> This function sets the target to the target from last update, used to
    /// undo other set_Target functions

    SCRIPT_FUNCTION_BEGIN();

    pself->target = pself->target_old;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToVelocity( script_state_t * pstate, ai_state_t * pself )
{
    // SetTurnModeToVelocity()
    /// @details ZZ@> This function sets the character's movement mode to the default

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_VELOCITY;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToWatch( script_state_t * pstate, ai_state_t * pself )
{
    // SetTurnModeToWatch()
    /// @details ZZ@> This function makes the character look at its next waypoint, usually
    /// used with close waypoints or the Stop function

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_WATCH;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToSpin( script_state_t * pstate, ai_state_t * pself )
{
    // SetTurnModeToSpin()
    /// @details ZZ@> This function makes the character spin around in a circle, usually
    /// used for magical items and such

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_SPIN;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BumpHeight( script_state_t * pstate, ai_state_t * pself )
{
    // SetBumpHeight( tmpargument = "height" )
    /// @details ZZ@> This function makes the character taller or shorter, usually used when
    /// the character dies

    SCRIPT_FUNCTION_BEGIN();

    chr_set_height( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the target has either a parent or type IDSZ
    /// matching tmpargument.

    SCRIPT_FUNCTION_BEGIN();

    returncode = chr_is_type_idsz( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasItemID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasItemID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the target has a matching item in his/her
    /// pockets or hands.

    Uint16 item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_has_item_idsz( pself->target, ( IDSZ ) pstate->argument, bfalse, NULL );

    returncode = ACTIVE_CHR( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHoldingItemID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHoldingItemID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the target has a matching item in his/her
    /// hands.  It also sets tmpargument to the proper latch button to press
    /// to use that item

    Uint16 item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_holding_idsz( pself->target, pstate->argument );

    returncode = ACTIVE_CHR( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasSkillID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasSkillID( tmpargument = "skill idsz" )
    /// @details ZZ@> This function proceeds if ID matches tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 != check_skills( pself->target, ( IDSZ )pstate->argument ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Else( script_state_t * pstate, ai_state_t * pself )
{
    // Else
    /// @details ZZ@> This function fails if the last function was more indented

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->indent >= pself->indent_last );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Run( script_state_t * pstate, ai_state_t * pself )
{
    // Run()
    /// @details ZZ@> This function sets the character's maximum acceleration to its
    /// actual maximum

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Walk( script_state_t * pstate, ai_state_t * pself )
{
    // Walk()
    /// @details ZZ@> This function sets the character's maximum acceleration to 66%
    /// of its actual maximum

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );
    pchr->maxaccel *= 0.66f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Sneak( script_state_t * pstate, ai_state_t * pself )
{
    // Sneak()
    /// @details ZZ@> This function sets the character's maximum acceleration to 33%
    /// of its actual maximum

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );
    pchr->maxaccel *= 0.33f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoAction( script_state_t * pstate, ai_state_t * pself )
{
    // DoAction( tmpargument = "action" )
    /// @details ZZ@> This function makes the character do a given action if it isn't doing
    /// anything better.  Fails if the action is invalid or if the character is doing
    /// something else already

    int action;

    SCRIPT_FUNCTION_BEGIN();

    action = mad_get_action( pchr->inst.imad, pstate->argument );

    returncode = bfalse;
    if ( action < ACTION_COUNT && pchr->inst.action_ready )
    {
        if ( MadList[pchr->inst.imad].action_valid[action] )
        {
            pchr->inst.action_which = action;
            pchr->inst.ilip         = 0;
            pchr->inst.flip         = 0;
            pchr->inst.frame_lst    = pchr->inst.frame_nxt;
            pchr->inst.frame_nxt    = MadList[pchr->inst.imad].action_stt[action];
            pchr->inst.action_ready = bfalse;

            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KeepAction( script_state_t * pstate, ai_state_t * pself )
{
    // KeepAction()
    /// @details ZZ@> This function makes the character's animation stop on its last frame
    /// and stay there.  Usually used for dropped items

    SCRIPT_FUNCTION_BEGIN();

    pchr->inst.action_keep = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IssueOrder( script_state_t * pstate, ai_state_t * pself )
{
    // IssueOrder( tmpargument = "order"  )
    /// @details ZZ@> This function tells all of the character's teammates to do something,
    /// though each teammate needs to interpret the order using _Ordered in
    /// its own script.

    SCRIPT_FUNCTION_BEGIN();

    issue_order( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropWeapons( script_state_t * pstate, ai_state_t * pself )
{
    // DropWeapons()
    /// @details ZZ@> This function drops the character's in-hand items.  It will also
    /// buck the rider if the character is a mount

    SCRIPT_FUNCTION_BEGIN();

    // This funtion drops the character's in hand items/riders
    sTmp = pchr->holdingwhich[SLOT_LEFT];
    if ( ACTIVE_CHR( sTmp ) )
    {
        detach_character_from_mount( sTmp, btrue, btrue );
        if ( pchr->ismount )
        {
            ChrList.lst[sTmp].vel.z = DISMOUNTZVEL;
            ChrList.lst[sTmp].pos.z += DISMOUNTZVEL;
            ChrList.lst[sTmp].jumptime = JUMPDELAY;
        }
    }

    sTmp = pchr->holdingwhich[SLOT_RIGHT];
    if ( ACTIVE_CHR( sTmp ) )
    {
        detach_character_from_mount( sTmp, btrue, btrue );
        if ( pchr->ismount )
        {
            ChrList.lst[sTmp].vel.z = DISMOUNTZVEL;
            ChrList.lst[sTmp].pos.z += DISMOUNTZVEL;
            ChrList.lst[sTmp].jumptime = JUMPDELAY;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDoAction( script_state_t * pstate, ai_state_t * pself )
{
    // TargetDoAction( tmpargument = "action" )
    /// @details ZZ@> The function makes the target start a new action, if it is valid for the model
    /// It will fail if the action is invalid or if the target is doing
    /// something else already

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ACTIVE_CHR( pself->target ) && ChrList.lst[pself->target].alive )
    {
        int action;
        chr_t * ptarget;

        ptarget = ChrList.lst + pself->target;

        action = mad_get_action( ptarget->inst.imad, pstate->argument );
        if ( action < ACTION_COUNT && ptarget->inst.action_ready )
        {
            if ( MadList[ptarget->inst.imad].action_valid[action] )
            {
                ptarget->inst.action_which = action;
                ptarget->inst.action_ready = bfalse;

                ptarget->inst.ilip         = 0;
                ptarget->inst.flip         = 0;
                ptarget->inst.frame_lst    = ptarget->inst.frame_nxt;
                ptarget->inst.frame_nxt    = MadList[ptarget->inst.imad].action_stt[action];

                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OpenPassage( script_state_t * pstate, ai_state_t * pself )
{
    // OpenPassage( tmpargument = "passage" )

    /// @details ZZ@> This function opens the passage specified by tmpargument, failing if the
    /// passage was already open.
    /// Passage areas are defined in passage.txt and set in spawn.txt for the given character

    SCRIPT_FUNCTION_BEGIN();

    returncode = open_passage( pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClosePassage( script_state_t * pstate, ai_state_t * pself )
{
    // ClosePassage( tmpargument = "passage" )
    /// @details ZZ@> This function closes the passage specified by tmpargument, proceeding
    /// if the passage isn't blocked.  Crushable characters within the passage
    /// are crushed.

    SCRIPT_FUNCTION_BEGIN();

    returncode = close_passage( pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PassageOpen( script_state_t * pstate, ai_state_t * pself )
{
    // IfPassageOpen( tmpargument = "passage" )
    /// @details ZZ@> This function proceeds if the given passage is valid and open to movement
    /// Used mostly by door characters to tell them when to run their open animation.

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( VALID_PASSAGE( pstate->argument ) )
    {
        returncode = PassageStack.lst[pstate->argument].open;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GoPoof( script_state_t * pstate, ai_state_t * pself )
{
    // GoPoof()
    /// @details ZZ@> This function flags the character to be removed from the game entirely.
    /// This doesn't work on players

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( !pchr->isplayer )
    {
        returncode = btrue;
        pself->poof_time = update_wld;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostTargetItemID( script_state_t * pstate, ai_state_t * pself )
{
    // CostTargetItemID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the target has a matching item, and poofs
    /// that item.
    /// For one use keys and such

    Uint16 item, pack_last;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->target;
    item = chr_has_item_idsz( sTmp, ( IDSZ ) pstate->argument, bfalse, &pack_last );

    returncode = bfalse;
    if ( ACTIVE_CHR( item ) )
    {
        returncode = btrue;

        if ( ChrList.lst[item].ammo > 1 )
        {
            // Cost one ammo
            ChrList.lst[item].ammo--;
        }
        else
        {
            // Poof the item
            if ( ACTIVE_CHR( pack_last ) && ChrList.lst[item].pack_ispacked )
            {
                // Remove from the pack
                ChrList.lst[pack_last].pack_next = ChrList.lst[item].pack_next;
                ChrList.lst[sTmp].pack_count--;

                ChrList.lst[item].pack_ispacked = bfalse;
                ChrList.lst[item].pack_next     = MAX_CHR;
            }
            else if ( ACTIVE_CHR( pack_last ) && !ChrList.lst[item].pack_ispacked )
            {
                // this is corrupt data == trouble
                // treat it as the normal case. if it causes errors, we'll fix them later
                ChrList.lst[pack_last].pack_next = ChrList.lst[item].pack_next;
                ChrList.lst[sTmp].pack_count--;

                ChrList.lst[item].pack_ispacked = bfalse;
                ChrList.lst[item].pack_next     = MAX_CHR;
            }
            else
            {
                // Drop from hand
                detach_character_from_mount( item, btrue, bfalse );
            }

            // get rid of the character, no matter what
            chr_request_terminate( item );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoActionOverride( script_state_t * pstate, ai_state_t * pself )
{
    // DoActionOverride( tmpargument = "action" )
    /// @details ZZ@> This function makes the character do a given action no matter what
    /// It will fail if the action is invalid

    int action;

    SCRIPT_FUNCTION_BEGIN();

    action = mad_get_action( pchr->inst.imad, pstate->argument );

    returncode = bfalse;
    if ( action < ACTION_COUNT )
    {
        if ( MadList[pchr->inst.imad].action_valid[action] )
        {
            pchr->inst.action_which = action;
            pchr->inst.action_ready = bfalse;

            pchr->inst.ilip         = 0;
            pchr->inst.flip         = 0;
            pchr->inst.frame_lst    = pchr->inst.frame_nxt;
            pchr->inst.frame_nxt    = MadList[pchr->inst.imad].action_stt[action];

            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Healed( script_state_t * pstate, ai_state_t * pself )
{
    // IfHealed()
    /// @details ZZ@> This function proceeds if the character was healed by a healing particle

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was healed
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_HEALED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SendPlayerMessage( script_state_t * pstate, ai_state_t * pself )
{
    // SendPlayerMessage( tmpargument = "message number" )
    /// @details ZZ@> This function sends a message to the players

    SCRIPT_FUNCTION_BEGIN();

    returncode = _display_message( pself->index, pchr->iprofile, pstate->argument, pstate );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CallForHelp( script_state_t * pstate, ai_state_t * pself )
{
    // CallForHelp()
    /// @details ZZ@> This function calls all of the character's teammates for help.  The
    /// teammates must use IfCalledForHelp in their scripts

    SCRIPT_FUNCTION_BEGIN();

    call_for_help( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddIDSZ( script_state_t * pstate, ai_state_t * pself )
{
    // AddIDSZ( tmpargument = "idsz" )
    /// @details ZZ@> This function slaps an expansion IDSZ onto the  menu.txt file.
    /// Used to show completion of special quests for a given module

    SCRIPT_FUNCTION_BEGIN();

    module_add_idsz( pickedmodule_name, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_State( script_state_t * pstate, ai_state_t * pself )
{
    // SetState( tmpargument = "state" )
    /// @details ZZ@> This function sets the character's state.
    /// VERY IMPORTANT. State is preserved from update to update

    SCRIPT_FUNCTION_BEGIN();

    // set the state
    pself->state = pstate->argument;

    // determine whether the object is hidden
    chr_update_hide( pchr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_State( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetState()
    /// @details ZZ@> This function reads the character's state variable

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = pself->state;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs( script_state_t * pstate, ai_state_t * pself )
{
    // IfStateIs( tmpargument = "state" )
    /// @details ZZ@> This function proceeds if the character's state equals tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->argument == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanOpenStuff( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetCanOpenStuff()
    /// @details ZZ@> This function proceeds if the target can open stuff ( set in data.txt )
    /// Used by chests and buttons and such so only "smart" creatures can operate
    /// them

    SCRIPT_FUNCTION_BEGIN();

    if ( ChrList.lst[pself->target].ismount && ChrList.lst[ChrList.lst[pself->target].holdingwhich[SLOT_LEFT]].openstuff ) returncode = btrue;
    else returncode = ChrList.lst[pself->target].openstuff;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Grabbed( script_state_t * pstate, ai_state_t * pself )
{
    // IfGrabbed()
    /// @details ZZ@> This function proceeds if the character was grabbed (picked up) this update.
    /// Used mostly by item characters

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_GRABBED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Dropped( script_state_t * pstate, ai_state_t * pself )
{
    // IfDropped()
    /// @details ZZ@> This function proceeds if the character was dropped this update.
    /// Used mostly by item characters

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_DROPPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverIsHolding( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverIsHolding()
    /// @details ZZ@> This function sets the target to the character's holder or mount,
    /// failing if the character has no mount or holder

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        pself->target = pchr->attachedto;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DamageTarget( script_state_t * pstate, ai_state_t * pself )
{
    // DamageTarget( tmpargument = "damage" )
    /// @details ZZ@> This function applies little bit of love to the character's target.
    /// The amount is set in tmpargument

    IPair tmp_damage;

    SCRIPT_FUNCTION_BEGIN();

    tmp_damage.base = pstate->argument;
    tmp_damage.rand = 1;

    damage_character( pself->target, ATK_FRONT, tmp_damage, pchr->damagetargettype, pchr->team, pself->index, DAMFX_NBLOC, btrue );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_XIsLessThanY( script_state_t * pstate, ai_state_t * pself )
{
    // IfXIsLessThanY( tmpx, tmpy )
    /// @details ZZ@> This function proceeds if tmpx is less than tmpy.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->x < pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_WeatherTime( script_state_t * pstate, ai_state_t * pself )
{
    // SetWeatherTime( tmpargument = "time" )
    /// @details ZZ@> This function can be used to slow down or speed up or stop rain and
    /// other weather effects

    SCRIPT_FUNCTION_BEGIN();

    // Set the weather timer
    weather.timer_reset = pstate->argument;
    weather.time = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_BumpHeight( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetBumpHeight()
    /// @details ZZ@> This function sets tmpargument to the character's height

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = pchr->bump.height;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Reaffirmed( script_state_t * pstate, ai_state_t * pself )
{
    // IfReaffirmed()
    /// @details ZZ@> This function proceeds if the character was damaged by its reaffirm
    /// damage type.  Used to relight the torch.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_REAFFIRMED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkeepAction( script_state_t * pstate, ai_state_t * pself )
{
    // UnkeepAction()
    /// @details ZZ@> This function is the opposite of KeepAction. It makes the current animation resume.

    SCRIPT_FUNCTION_BEGIN();

    pchr->inst.action_keep = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnOtherTeam( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsOnOtherTeam()
    /// @details ZZ@> This function proceeds if the target is on another team

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList.lst[pself->target].alive && chr_get_iteam( pself->target ) != pchr->team );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnHatedTeam( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsOnHatedTeam()
    /// @details ZZ@> This function proceeds if the target is on an enemy team

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList.lst[pself->target].alive && TeamList[pchr->team].hatesteam[chr_get_iteam( pself->target )] && !ChrList.lst[pself->target].invictus );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PressLatchButton( script_state_t * pstate, ai_state_t * pself )
{
    // PressLatchButton( tmpargument = "latch bits" )
    /// @details ZZ@> This function sets the character latch buttons

    SCRIPT_FUNCTION_BEGIN();

    pchr->latch.b = pchr->latch.b | pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetOfLeader( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToTargetOfLeader()
    /// @details ZZ@> This function sets the character's target to the target of its leader,
    /// or it fails with no change if the leader is dead

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( TeamList[pchr->team].leader != NOLEADER )
    {
        pself->target = ChrList.lst[TeamList[pchr->team].leader].ai.target;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_LeaderKilled( script_state_t * pstate, ai_state_t * pself )
{
    // IfLeaderKilled()
    /// @details ZZ@> This function proceeds if the team's leader died this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_LEADERKILLED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeLeader( script_state_t * pstate, ai_state_t * pself )
{
    // BecomeLeader()
    /// @details ZZ@> This function makes the character the leader of the team

    SCRIPT_FUNCTION_BEGIN();

    TeamList[pchr->team].leader = pself->index;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTargetArmor( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeTargetArmor( tmpargument = "armor" )

    /// @details ZZ@> This function sets the target's armor type and returns the old type
    /// as tmpargument and the new type as tmpx

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = ChrList.lst[pself->target].skin;
    pstate->x = change_armor( pself->target, pstate->argument );
    pstate->argument = iTmp;  // The character's old armor

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveMoneyToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveMoneyToTarget( tmpargument = "money" )
    /// @details ZZ@> This function increases the target's money, while decreasing the
    /// character's own money.  tmpargument is set to the amount transferred

    int tTmp;
    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = pchr->money;
    tTmp = ChrList.lst[pself->target].money;
    iTmp -= pstate->argument;
    tTmp += pstate->argument;
    if ( iTmp < 0 ) { tTmp += iTmp;  pstate->argument += iTmp;  iTmp = 0; }
    if ( tTmp < 0 ) { iTmp += tTmp;  pstate->argument += tTmp;  tTmp = 0; }
    if ( iTmp > MAXMONEY ) { iTmp = MAXMONEY; }
    if ( tTmp > MAXMONEY ) { tTmp = MAXMONEY; }

    pchr->money = iTmp;
    ChrList.lst[pself->target].money = tTmp;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropKeys( script_state_t * pstate, ai_state_t * pself )
{
    // DropKeys()
    /// @details ZZ@> This function drops all of the keys in the character's inventory.
    /// This does NOT drop keys in the character's hands.

    SCRIPT_FUNCTION_BEGIN();

    drop_keys( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_LeaderIsAlive( script_state_t * pstate, ai_state_t * pself )
{
    // IfLeaderIsAlive()
    /// @details ZZ@> This function proceeds if the team has a leader

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( TeamList[pchr->team].leader != NOLEADER );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOldTarget( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsOldTarget()
    /// @details ZZ@> This function proceeds if the target is the same as it was last update

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->target == pself->target_old );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLeader( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToLeader()
    /// @details ZZ@> This function sets the target to the leader, proceeding if their is
    /// a valid leader for the character's team

    SCRIPT_FUNCTION_BEGIN();

    if ( TeamList[pchr->team].leader == NOLEADER )
    {
        returncode = bfalse;
    }
    else
    {
        pself->target = TeamList[pchr->team].leader;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnCharacter( tmpx = "x", tmpy = "y", tmpturn = "turn", tmpdistance = "speed" )

    /// @details ZZ@> This function spawns a character of the same type as the spawner.
    /// This function spawns a character, failing if x,y is invalid
    /// This is horribly complicated to use, so see ANIMATE.OBJ for an example
    /// tmpx and tmpy give the coodinates, tmpturn gives the new character's
    /// direction, and tmpdistance gives the new character's initial velocity

    int tTmp;
    fvec3_t   pos;

    SCRIPT_FUNCTION_BEGIN();

    pos.x = pstate->x;
    pos.y = pstate->y;
    pos.z = 0;

    sTmp = spawn_one_character( pos, pchr->iprofile, pchr->team, 0, CLIP_TO_16BITS( pstate->turn ), NULL, MAX_CHR );
    if ( !ACTIVE_CHR( sTmp ) )
    {
        if ( sTmp > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "Object %s failed to spawn a copy of itself\n", pchr->obj_base._name );
        }
    }
    else
    {
        chr_t * pchild = ChrList.lst + sTmp;

        // was the child spawned in a "safe" spot?
        if ( !pchild->safe_valid )
        {
            chr_request_terminate( sTmp );
            sTmp = MAX_CHR;
        }
        else
        {
            pself->child = sTmp;

            tTmp = ( pchr->turn_z + ATK_BEHIND ) >> 2;
            pchild->vel.x += turntocos[ tTmp & TRIG_TABLE_MASK ] * pstate->distance;
            pchild->vel.y += turntosin[ tTmp & TRIG_TABLE_MASK ] * pstate->distance;

            pchild->iskursed = pchr->iskursed;  // BB> inherit this from your spawner
            pchild->ai.passage = pself->passage;
            pchild->ai.owner   = pself->owner;
        }
    }

    returncode = ACTIVE_CHR( sTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RespawnCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // RespawnCharacter()
    /// @details ZZ@> This function respawns the character at its starting location.
    /// Often used with the Clean functions

    SCRIPT_FUNCTION_BEGIN();

    respawn_character( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTile( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeTile( tmpargument = "tile type")
    /// @details ZZ@> This function changes the tile under the character to the new tile type,
    /// which is highly module dependent

    SCRIPT_FUNCTION_BEGIN();

    returncode = mesh_set_texture( PMesh, pchr->onwhichfan, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Used( script_state_t * pstate, ai_state_t * pself )
{
    // IfUsed()
    /// @details ZZ@> This function proceeds if the character was used by its holder or rider.
    /// Character's cannot be used if their reload time is greater than 0

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_USED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropMoney( script_state_t * pstate, ai_state_t * pself )
{
    // DropMoney( tmpargument = "money" )
    /// @details ZZ@> This function drops a certain amount of money, if the character has that
    /// much

    SCRIPT_FUNCTION_BEGIN();

    drop_money( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_OldTarget( script_state_t * pstate, ai_state_t * pself )
{
    // SetOldTarget()
    /// @details ZZ@> This function sets the old target to the current target.  To allow
    /// greater manipulations of the target

    SCRIPT_FUNCTION_BEGIN();

    pself->target_old = pself->target;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DetachFromHolder( script_state_t * pstate, ai_state_t * pself )
{
    // DetachFromHolder()
    /// @details ZZ@> This function drops the character or makes it get off its mount
    /// Can be used to make slippery weapons, or to make certain characters
    /// incapable of wielding certain weapons. "A troll can't grab a torch"

    SCRIPT_FUNCTION_BEGIN();

    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        detach_character_from_mount( pself->index, btrue, btrue );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasVulnerabilityID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasVulnerabilityID( tmpargument = "vulnerability idsz" )
    /// @details ZZ@> This function proceeds if the target is vulnerable to the given IDSZ.

    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    pcap = chr_get_pcap( pself->target );
    if ( NULL != pcap )
    {
        returncode = ( pcap->idsz[IDSZ_VULNERABILITY] == ( IDSZ ) pstate->argument );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CleanUp( script_state_t * pstate, ai_state_t * pself )
{
    // CleanUp()
    /// @details ZZ@> This function tells all the dead characters on the team to clean
    /// themselves up.  Usually done by the boss creature every second or so

    SCRIPT_FUNCTION_BEGIN();

    issue_clean( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CleanedUp( script_state_t * pstate, ai_state_t * pself )
{
    // IfCleanedUp()
    /// @details ZZ@> This function proceeds if the character is dead and if the boss told it
    /// to clean itself up

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CLEANEDUP );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Sitting( script_state_t * pstate, ai_state_t * pself )
{
    // IfSitting()
    /// @details ZZ@> This function proceeds if the character is riding a mount

    SCRIPT_FUNCTION_BEGIN();

    returncode = ACTIVE_CHR( pchr->attachedto );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsHurt( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsHurt()
    /// @details ZZ@> This function passes only if the target is hurt and alive

    SCRIPT_FUNCTION_BEGIN();
    if ( !ChrList.lst[pself->target].alive || ChrList.lst[pself->target].life > ChrList.lst[pself->target].lifemax - HURTDAMAGE )
        returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAPlayer( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAPlayer()
    /// @details ZZ@> This function proceeds if the target is controlled by a human ( may not be local )

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList.lst[pself->target].isplayer;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySound( script_state_t * pstate, ai_state_t * pself )
{
    // PlaySound( tmpargument = "sound" )
    /// @details ZZ@> This function plays one of the character's sounds.
    /// The sound fades out depending on its distance from the viewer

    SCRIPT_FUNCTION_BEGIN();

    if ( pchr->pos_old.z > PITNOSOUND && VALID_SND( pstate->argument ) )
    {
        sound_play_chunk( pchr->pos_old, chr_get_chunk_ptr( pchr, pstate->argument ) );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnParticle(tmpargument = "particle", tmpdistance = "character vertex", tmpx = "offset x", tmpy = "offset y" )
    /// @details ZZ@> This function spawns a particle, offset from the character's location

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->index;
    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        sTmp = pchr->attachedto;
    }

    tTmp = spawn_one_particle( pchr->pos, pchr->turn_z, pchr->iprofile, pstate->argument, pself->index, pstate->distance, pchr->team, sTmp, TOTAL_MAX_PRT, 0, MAX_CHR );

    returncode = ACTIVE_PRT( tTmp );
    if ( returncode )
    {
        prt_t * pprt = PrtList.lst + tTmp;

        // Detach the particle
        place_particle_at_vertex( tTmp, pself->index, pstate->distance );
        pprt->attachedto_ref = MAX_CHR;

        // Correct X, Y, Z spacing
        pprt->pos.x += pstate->x;
        pprt->pos.y += pstate->y;
        pprt->pos.z += PipStack.lst[pprt->pip_ref].zspacing_pair.base;

        // Don't spawn in walls
        if ( __prthitawall( pprt, NULL ) )
        {
            pprt->pos.x = pchr->pos.x;
            if ( __prthitawall( pprt, NULL ) )
            {
                pprt->pos.y = pchr->pos.y;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAlive( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAlive()
    /// @details ZZ@> This function proceeds if the target is alive

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList.lst[pself->target].alive;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Stop( script_state_t * pstate, ai_state_t * pself )
{
    // Stop()
    /// @details ZZ@> This function sets the character's maximum acceleration to 0.  Used
    /// along with Walk and Run and Sneak

    SCRIPT_FUNCTION_BEGIN();

    pchr->maxaccel = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisaffirmCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // DisaffirmCharacter()
    /// @details ZZ@> This function removes all the attached particles from a character
    /// ( stuck arrows, flames, etc )

    SCRIPT_FUNCTION_BEGIN();

    disaffirm_attached_particles( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ReaffirmCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // ReaffirmCharacter()
    /// @details ZZ@> This function makes sure it has all of its reaffirmation particles
    /// attached to it. Used to make the torch light again

    SCRIPT_FUNCTION_BEGIN();

    reaffirm_attached_particles( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsSelf( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsSelf()
    /// @details ZZ@> This function proceeds if the character is targeting itself

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->target == pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsMale( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsMale()
    /// @details ZZ@> This function proceeds only if the target is male

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList.lst[pself->target].gender == GENDER_MALE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsFemale( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsFemale()
    /// @details ZZ@> This function proceeds if the target is female

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList.lst[pself->target].gender == GENDER_FEMALE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToSelf( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToSelf()
    /// @details ZZ@> This function sets the target to the character itself

    SCRIPT_FUNCTION_BEGIN();

    pself->target = pself->index;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToRider( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToRider()
    /// @details ZZ@> This function sets the target to whoever is riding the character (left/only grip),
    /// failing if there is no rider

    SCRIPT_FUNCTION_BEGIN();
    if ( !ACTIVE_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
    {
        returncode = bfalse;
    }
    else
    {
        pself->target = pchr->holdingwhich[SLOT_LEFT];
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_AttackTurn( script_state_t * pstate, ai_state_t * pself )
{
    // tmpturn = GetAttackTurn()
    /// @details ZZ@> This function sets tmpturn to the direction from which the last attack
    /// came. Not particularly useful in most cases, but it could be.

    SCRIPT_FUNCTION_BEGIN();

    pstate->turn = pself->directionlast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_DamageType( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetDamageType()
    /// @details ZZ@> This function sets tmpargument to the damage type of the last attack that
    /// hit the character

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = pself->damagetypelast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeSpell( script_state_t * pstate, ai_state_t * pself )
{
    // BecomeSpell()
    /// @details ZZ@> This function turns a spellbook character into a spell based on its
    /// content.
    /// TOO COMPLICATED TO EXPLAIN.  SHOULDN'T EVER BE NEEDED BY YOU.

    int iskin;
    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    // get the spellbook's skin
    iskin = pchr->skin;

    // change the spellbook to a spell effect
    change_character( pself->index, pself->content, 0, LEAVENONE );

    // set the spell effect parameters
    pchr->money    = iskin;
    pself->content = 0;
    pself->state   = 0;

    // have to do this every time pself->state is modified
    chr_update_hide( pchr );

    // set the book icon of the spell effect if it is not already set
    pcap = chr_get_pcap( pself->index );
    if ( NULL != pcap )
    {
        pcap->spelleffect_type = iskin;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeSpellbook( script_state_t * pstate, ai_state_t * pself )
{
    // BecomeSpellbook()
    //
    /// @details ZZ@> This function turns a spell character into a spellbook and sets the content accordingly.
    /// TOO COMPLICATED TO EXPLAIN. Just copy the spells that already exist, and don't change
    /// them too much

    Uint16  old_profile;
    cap_t * pcap;
    mad_t * pmad;

    SCRIPT_FUNCTION_BEGIN();

    // convert the spell effect to a spellbook
    old_profile = pchr->iprofile;
    change_character( pself->index, SPELLBOOK, pchr->money % MAX_SKIN, LEAVENONE );

    // Reset the spellbook state so it doesn't burn up
    pself->state   = 0;
    pself->content = old_profile;

    // set the spellbook animations
    pcap = pro_get_pcap( pchr->iprofile );
    pmad = chr_get_pmad( pself->index );
    if ( NULL != pcap && NULL != pmad )
    {
        //Do dropped animation
        int tmp_action = mad_get_action( pchr->inst.imad, ACTION_JB );
        if ( ACTION_COUNT != tmp_action && pmad->action_valid[tmp_action] )
        {
            pchr->inst.action_which = tmp_action;
            pchr->inst.action_ready = bfalse;
            //pchr->inst.action_keep = btrue;

            pchr->inst.ilip = 0;
            pchr->inst.flip = 0;
            pchr->inst.frame_lst = pchr->inst.frame_nxt;
            pchr->inst.frame_nxt = pmad->action_stt[tmp_action];
        }

        returncode = btrue;
    }

    // have to do this every time pself->state is modified
    chr_update_hide( pchr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ScoredAHit( script_state_t * pstate, ai_state_t * pself )
{
    // IfScoredAHit()
    /// @details ZZ@> This function proceeds if the character damaged another character this
    /// update. If it's a held character it also sets the target to whoever was hit

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character scored a hit
    if ( !ACTIVE_CHR( pchr->attachedto ) || ChrList.lst[pchr->attachedto].ismount )
    {
        returncode = HAS_SOME_BITS( pself->alert, ALERTIF_SCOREDAHIT );
    }

    // Proceed only if the holder scored a hit with the character
    else if ( ChrList.lst[pchr->attachedto].ai.lastitemused == pself->index )
    {
        returncode = HAS_SOME_BITS( ChrList.lst[pchr->attachedto].ai.alert, ALERTIF_SCOREDAHIT );
        if ( returncode ) pself->target = ChrList.lst[pchr->attachedto].ai.hitlast;
    }
    else returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Disaffirmed( script_state_t * pstate, ai_state_t * pself )
{
    // IfDisaffirmed()
    /// @details ZZ@> This function proceeds if the character was disaffirmed.
    /// This doesn't seem useful anymore.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_DISAFFIRMED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TranslateOrder( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx,tmpy,tmpargument = TranslateOrder()
    /// @details ZZ@> This function translates a packed order into understandable values.
    /// See CreateOrder for more.  This function sets tmpx, tmpy, tmpargument,
    /// and sets the target ( if valid )

    SCRIPT_FUNCTION_BEGIN();

    sTmp = CLIP_TO_16BITS( pself->order_value >> 24 );
    if ( ACTIVE_CHR( sTmp ) )
    {
        pself->target = sTmp;
    }

    pstate->x        = (( pself->order_value >> 14 ) & 0x03FF ) << 6;
    pstate->y        = (( pself->order_value >>  4 ) & 0x03FF ) << 6;
    pstate->argument = (( pself->order_value >>  0 ) & 0x000F );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverWasHit( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverWasHit()
    /// @details ZZ@> This function sets the target to whoever was hit by the character last

    SCRIPT_FUNCTION_BEGIN();

    pself->target = pself->hitlast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWideEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWideEnemy()
    /// @details ZZ@> This function sets the target to an enemy in the vicinity around the
    /// character, failing if there are none

    Uint16 ichr;
    SCRIPT_FUNCTION_BEGIN();

    ichr = _get_chr_target( pchr, WIDE, TARGET_ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse, bfalse );

    returncode = ( ichr != pself->index ) && ACTIVE_CHR( ichr );

    if ( returncode )
    {
        pself->target = ichr;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Changed( script_state_t * pstate, ai_state_t * pself )
{
    // IfChanged()
    /// @details ZZ@> This function proceeds if the character was polymorphed.
    /// Needed for morph spells and such

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CHANGED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_InWater( script_state_t * pstate, ai_state_t * pself )
{
    // IfInWater()
    /// @details ZZ@> This function proceeds if the character has just entered into some water
    /// this update ( and the water is really water, not fog or another effect )

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_INWATER );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Bored( script_state_t * pstate, ai_state_t * pself )
{
    // IfBored()
    /// @details ZZ@> This function proceeds if the character has been standing idle too long

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_BORED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TooMuchBaggage( script_state_t * pstate, ai_state_t * pself )
{
    // IfTooMuchBaggage()
    /// @details ZZ@> This function proceeds if the character tries to put an item in his/her
    /// pockets, but the character already has 6 items in the inventory.
    /// Used to tell the players what's going on.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_TOOMUCHBAGGAGE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Grogged( script_state_t * pstate, ai_state_t * pself )
{
    // IfGrogged()
    /// @details ZZ@> This function proceeds if the character has been grogged ( a type of
    /// confusion ) this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_GROGGED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Dazed( script_state_t * pstate, ai_state_t * pself )
{
    // IfDazed()
    /// @details ZZ@> This function proceeds if the character has been dazed ( a type of
    /// confusion ) this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_DAZED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasSpecialID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasSpecialID( tmpargument = "special idsz" )
    /// @details ZZ@> This function proceeds if the character has a special IDSZ ( in data.txt )

    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    pcap = chr_get_pcap( pself->target );
    if ( NULL != pcap )
    {
        returncode = ( pcap->idsz[IDSZ_SPECIAL] == ( IDSZ ) pstate->argument );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PressTargetLatchButton( script_state_t * pstate, ai_state_t * pself )
{
    // PressTargetLatchButton( tmpargument = "latch bits" )
    /// @details ZZ@> This function mimics joystick button presses for the target.
    /// For making items force their own usage and such

    SCRIPT_FUNCTION_BEGIN();

    ChrList.lst[pself->target].latch.b = ChrList.lst[pself->target].latch.b | pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Invisible( script_state_t * pstate, ai_state_t * pself )
{
    // IfInvisible()
    /// @details ZZ@> This function proceeds if the character is invisible

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->inst.alpha <= INVISIBLE;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ArmorIs( script_state_t * pstate, ai_state_t * pself )
{
    // IfArmorIs( tmpargument = "skin" )
    /// @details ZZ@> This function proceeds if the character's skin type equals tmpargument

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    tTmp = pchr->skin;
    returncode = ( tTmp == pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetGrogTime( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTargetGrogTime()
    /// @details ZZ@> This function sets tmpargument to the number of updates before the
    /// character is ungrogged, proceeding if the number is greater than 0

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = ChrList.lst[pchr->ai.target].grogtime;
    returncode = ( pstate->argument != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetDazeTime( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTargetDazeTime()
    /// @details ZZ@> This function sets tmpargument to the number of updates before the
    /// character is undazed, proceeding if the number is greater than 0

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = ChrList.lst[pchr->ai.target].dazetime;
    returncode = ( pstate->argument != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageType( script_state_t * pstate, ai_state_t * pself )
{
    // SetDamageType( tmpargument = "damage type" )
    /// @details ZZ@> This function lets a weapon change the type of damage it inflicts

    SCRIPT_FUNCTION_BEGIN();

    pchr->damagetargettype = pstate->argument % DAMAGE_COUNT;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_WaterLevel( script_state_t * pstate, ai_state_t * pself )
{
    // SetWaterLevel( tmpargument = "level" )
    /// @details ZZ@> This function raises or lowers the water in the module

    int iTmp;
    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( pstate->argument / 10.0f ) - water.douse_level;
    water.surface_level += fTmp;
    water.douse_level += fTmp;

    for ( iTmp = 0; iTmp < MAXWATERLAYER; iTmp++ )
    {
        water.layer[iTmp].z += fTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnchantTarget( script_state_t * pstate, ai_state_t * pself )
{
    // EnchantTarget()
    /// @details ZZ@> This function enchants the target with the enchantment given
    /// in enchant.txt. Make sure you use set_OwnerToTarget before doing this.

    SCRIPT_FUNCTION_BEGIN();

    sTmp = spawn_one_enchant( pself->owner, pself->target, pself->index, MAX_ENC, MAX_PROFILE );
    returncode = ACTIVE_ENC( sTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnchantChild( script_state_t * pstate, ai_state_t * pself )
{
    // EnchantChild()
    /// @details ZZ@> This function can be used with SpawnCharacter to enchant the
    /// newly spawned character with the enchantment
    /// given in enchant.txt. Make sure you use set_OwnerToTarget before doing this.

    SCRIPT_FUNCTION_BEGIN();

    sTmp = spawn_one_enchant( pself->owner, pself->child, pself->index, MAX_ENC, MAX_PROFILE );
    returncode = ACTIVE_ENC( sTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TeleportTarget( script_state_t * pstate, ai_state_t * pself )
{
    // TeleportTarget( tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function teleports the target to the X, Y location, failing if the
    /// location is off the map or blocked

    SCRIPT_FUNCTION_BEGIN();

    returncode = chr_teleport( pself->target, pstate->x, pstate->y, pstate->distance, pstate->turn );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveExperienceToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveExperienceToTarget( tmpargument = "amount", tmpdistance = "type" )
    /// @details ZZ@> This function gives the target some experience, xptype from distance,
    /// amount from argument.

    SCRIPT_FUNCTION_BEGIN();

    give_experience( pself->target, pstate->argument, pstate->distance, bfalse );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IncreaseAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // IncreaseAmmo()
    /// @details ZZ@> This function increases the character's ammo by 1

    SCRIPT_FUNCTION_BEGIN();
    if ( pchr->ammo < pchr->ammomax )
    {
        pchr->ammo++;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkurseTarget( script_state_t * pstate, ai_state_t * pself )
{
    // UnkurseTarget()
    /// @details ZZ@> This function unkurses the target

    SCRIPT_FUNCTION_BEGIN();

    ChrList.lst[pself->target].iskursed = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveExperienceToTargetTeam( script_state_t * pstate, ai_state_t * pself )
{
    // GiveExperienceToTargetTeam( tmpargument = "amount", tmpdistance = "type" )
    /// @details ZZ@> This function gives experience to everyone on the target's team

    SCRIPT_FUNCTION_BEGIN();

    give_team_experience( chr_get_iteam( pself->target ), pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Unarmed( script_state_t * pstate, ai_state_t * pself )
{
    // IfUnarmed()
    /// @details ZZ@> This function proceeds if the character is holding no items in hand.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( !ACTIVE_CHR( pchr->holdingwhich[SLOT_LEFT] ) && !ACTIVE_CHR( pchr->holdingwhich[SLOT_RIGHT] ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RestockTargetAmmoIDAll( script_state_t * pstate, ai_state_t * pself )
{
    // RestockTargetAmmoIDAll( tmpargument = "idsz" )
    /// @details ZZ@> This function restocks the ammo of every item the character is holding,
    /// if the item matches the ID given ( parent or child type )

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = 0;  // Amount of ammo given
    sTmp = ChrList.lst[pself->target].holdingwhich[SLOT_LEFT];
    iTmp += restock_ammo( sTmp, pstate->argument );
    sTmp = ChrList.lst[pself->target].holdingwhich[SLOT_RIGHT];
    iTmp += restock_ammo( sTmp, pstate->argument );
    sTmp = ChrList.lst[pself->target].pack_next;

    while ( sTmp != MAX_CHR )
    {
        iTmp += restock_ammo( sTmp, pstate->argument );
        sTmp = ChrList.lst[sTmp].pack_next;
    }

    pstate->argument = iTmp;
    returncode = ( iTmp != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RestockTargetAmmoIDFirst( script_state_t * pstate, ai_state_t * pself )
{
    // RestockTargetAmmoIDFirst( tmpargument = "idsz" )
    /// @details ZZ@> This function restocks the ammo of the first item the character is holding,
    /// if the item matches the ID given ( parent or child type )

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = 0;  // Amount of ammo given
    sTmp = ChrList.lst[pself->target].holdingwhich[SLOT_LEFT];
    iTmp += restock_ammo( sTmp, pstate->argument );
    if ( iTmp == 0 )
    {
        sTmp = ChrList.lst[pself->target].holdingwhich[SLOT_RIGHT];
        iTmp += restock_ammo( sTmp, pstate->argument );
        if ( iTmp == 0 )
        {
            sTmp = ChrList.lst[pself->target].pack_next;

            while ( sTmp != MAX_CHR && iTmp == 0 )
            {
                iTmp += restock_ammo( sTmp, pstate->argument );
                sTmp = ChrList.lst[sTmp].pack_next;
            }
        }
    }

    pstate->argument = iTmp;
    returncode = ( iTmp != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashTarget( script_state_t * pstate, ai_state_t * pself )
{
    // FlashTarget()
    /// @details ZZ@> This function makes the target flash

    SCRIPT_FUNCTION_BEGIN();

    flash_character( pself->target, 255 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_RedShift( script_state_t * pstate, ai_state_t * pself )
{
    // SetRedShift( tmpargument = "red darkening" )
    /// @details ZZ@> This function sets the character's red shift ( 0 - 3 ), higher values
    /// making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    chr_set_redshift( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_GreenShift( script_state_t * pstate, ai_state_t * pself )
{
    // SetGreenShift( tmpargument = "green darkening" )
    /// @details ZZ@> This function sets the character's green shift ( 0 - 3 ), higher values
    /// making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    chr_set_grnshift( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BlueShift( script_state_t * pstate, ai_state_t * pself )
{
    // SetBlueShift( tmpargument = "blue darkening" )
    /// @details ZZ@> This function sets the character's blue shift ( 0 - 3 ), higher values
    /// making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    chr_set_grnshift( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Light( script_state_t * pstate, ai_state_t * pself )
{
    // SetLight( tmpargument = "lighness" )
    /// @details ZZ@> This function alters the character's transparency ( 0 - 254 )
    /// 255 = no transparency

    SCRIPT_FUNCTION_BEGIN();

    chr_set_light( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Alpha( script_state_t * pstate, ai_state_t * pself )
{
    // SetAlpha( tmpargument = "alpha" )
    /// @details ZZ@> This function alters the character's transparency ( 0 - 255 )
    /// 255 = no transparency

    SCRIPT_FUNCTION_BEGIN();

    chr_set_alpha( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromBehind( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitFromBehind()
    /// @details ZZ@> This function proceeds if the last attack to the character came from behind

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= ATK_BEHIND - 8192 && pself->directionlast < ATK_BEHIND + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromFront( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitFromFront()
    /// @details ZZ@> This function proceeds if the last attack to the character came
    /// from the front

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= ATK_LEFT + 8192 || pself->directionlast < ATK_FRONT + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromLeft( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitFromLeft()
    /// @details ZZ@> This function proceeds if the last attack to the character came
    /// from the left

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= ATK_LEFT - 8192 && pself->directionlast < ATK_LEFT + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromRight( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitFromRight()
    /// @details ZZ@> This function proceeds if the last attack to the character came
    /// from the right

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= ATK_RIGHT - 8192 && pself->directionlast < ATK_RIGHT + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnSameTeam( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsOnSameTeam()
    /// @details ZZ@> This function proceeds if the target is on the character's team

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( chr_get_iteam( pself->target ) == pchr->team )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KillTarget( script_state_t * pstate, ai_state_t * pself )
{
    // KillTarget()
    /// @details ZZ@> This function kills the target

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->index;

    //Weapons don't kill people, people kill people...
    if ( ACTIVE_CHR( pchr->attachedto ) && !ChrList.lst[pchr->attachedto].ismount )
    {
        sTmp = pchr->attachedto;
    }

    kill_character( pself->target, sTmp, bfalse );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UndoEnchant( script_state_t * pstate, ai_state_t * pself )
{
    // UndoEnchant()
    /// @details ZZ@> This function removes the last enchantment spawned by the character,
    /// proceeding if an enchantment was removed

    SCRIPT_FUNCTION_BEGIN();

    // clean up the enchant list before doing anything
    pchr->undoenchant = cleanup_enchant_list( pchr->undoenchant );

    returncode = remove_enchant( pchr->undoenchant );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_WaterLevel( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetWaterLevel()
    /// @details ZZ@> This function sets tmpargument to the current douse level for the water * 10.
    /// A waterlevel in wawalight of 85 would set tmpargument to 850

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = water.douse_level * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostTargetMana( script_state_t * pstate, ai_state_t * pself )
{
    // CostTargetMana( tmpargument = "amount" )
    /// @details ZZ@> This function costs the target a specific amount of mana, proceeding
    /// if the target was able to pay the price.  The amounts are 8.8-bit fixed point

    SCRIPT_FUNCTION_BEGIN();

    returncode = cost_mana( pself->target, pstate->argument, pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasAnyID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasAnyID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the target has any IDSZ that matches the given one

    SCRIPT_FUNCTION_BEGIN();

    returncode = chr_has_idsz( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BumpSize( script_state_t * pstate, ai_state_t * pself )
{
    // SetBumpSize( tmpargument = "size" )
    /// @details ZZ@> This function sets the how wide the character is

    SCRIPT_FUNCTION_BEGIN();

    chr_set_width( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotDropped( script_state_t * pstate, ai_state_t * pself )
{
    // IfNotDropped()
    /// @details ZZ@> This function proceeds if the character is kursed and another character
    /// was holding it and tried to drop it

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_NOTDROPPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_YIsLessThanX( script_state_t * pstate, ai_state_t * pself )
{
    // IfYIsLessThanX()
    /// @details ZZ@> This function proceeds if tmpy is less than tmpx

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->y < pstate->x );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FlyHeight( script_state_t * pstate, ai_state_t * pself )
{
    // SetFlyHeight( tmpargument = "height" )
    /// @details ZZ@> This function makes the character fly ( or fall to ground if 0 )

    SCRIPT_FUNCTION_BEGIN();

    pchr->flyheight = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Blocked( script_state_t * pstate, ai_state_t * pself )
{
    // IfBlocked()
    /// @details ZZ@> This function proceeds if the character blocked the attack of another
    /// character this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_BLOCKED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsDefending( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsDefending()
    /// @details ZZ@> This function proceeds if the target is holding up a shield or similar
    /// defense

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList.lst[pself->target].inst.action_which >= ACTION_PA && ChrList.lst[pself->target].inst.action_which <= ACTION_PD );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAttacking( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAttacking()
    /// @details ZZ@> This function proceeds if the target is doing an attack action

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList.lst[pself->target].inst.action_which >= ACTION_UA && ChrList.lst[pself->target].inst.action_which <= ACTION_FD );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs0( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs1( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 1 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs2( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 2 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs3( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 3 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs4( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 4 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs5( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 5 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs6( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 6 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs7( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 7 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ContentIs( script_state_t * pstate, ai_state_t * pself )
{
    // IfContentIs( tmpargument = "test" )
    /// @details ZZ@> This function proceeds if the content matches tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->argument == pself->content );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToWatchTarget( script_state_t * pstate, ai_state_t * pself )
{
    // SetTurnModeToWatchTarget()
    /// @details ZZ@> This function makes the character face its target, no matter what
    /// direction it is moving in.  Undo this with set_TurnModeToVelocity

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_WATCHTARGET;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIsNot( script_state_t * pstate, ai_state_t * pself )
{
    // IfStateIsNot( tmpargument = "test" )
    /// @details ZZ@> This function proceeds if the character's state does not equal tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->argument != pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_XIsEqualToY( script_state_t * pstate, ai_state_t * pself )
{
    // These functions proceed if tmpx and tmpy are the same

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->x == pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DebugMessage( script_state_t * pstate, ai_state_t * pself )
{
    // DebugMessage()
    /// @details ZZ@> This function spits out some useful numbers

    SCRIPT_FUNCTION_BEGIN();

    debug_printf( "aistate %d, aicontent %d, target %d", pself->state, pself->content, pself->target );
    debug_printf( "tmpx %d, tmpy %d", pstate->x, pstate->y );
    debug_printf( "tmpdistance %d, tmpturn %d", pstate->distance, pstate->turn );
    debug_printf( "tmpargument %d, selfturn %d", pstate->argument, pchr->turn_z );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BlackTarget( script_state_t * pstate, ai_state_t * pself )
{
    // BlackTarget()
    /// @details ZZ@>  The opposite of FlashTarget, causing the target to turn black

    SCRIPT_FUNCTION_BEGIN();

    flash_character( pself->target, 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SendMessageNear( script_state_t * pstate, ai_state_t * pself )
{
    // SendMessageNear( tmpargument = "message" )
    /// @details ZZ@> This function sends a message if the camera is in the nearby area

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = ABS( pchr->pos_old.x - PCamera->track_pos.x ) + ABS( pchr->pos_old.y - PCamera->track_pos.y );
    if ( iTmp < MSGDISTANCE )
    {
        returncode = _display_message( pself->index, pchr->iprofile, pstate->argument, pstate );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitGround( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitGround()
    /// @details ZZ@> This function proceeds if a character hit the ground this update.
    /// Used to determine when to play the sound for a dropped item

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_HITGROUND );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NameIsKnown( script_state_t * pstate, ai_state_t * pself )
{
    // IfNameIsKnown()
    /// @details ZZ@> This function proceeds if the character's name is known

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->nameknown;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UsageIsKnown( script_state_t * pstate, ai_state_t * pself )
{
    // IfUsageIsKnown()
    /// @details ZZ@> This function proceeds if the character's usage is known

    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    pcap = pro_get_pcap( pchr->iprofile );

    returncode = bfalse;
    if ( NULL != pcap )
    {
        returncode = pcap->usageknown;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingItemID( script_state_t * pstate, ai_state_t * pself )
{
    // IfHoldingItemID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it

    Uint16 item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_holding_idsz( pself->index, pstate->argument );

    returncode = ACTIVE_CHR( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingRangedWeapon( script_state_t * pstate, ai_state_t * pself )
{
    // IfHoldingRangedWeapon()
    /// @details ZZ@> This function passes if the character is holding a ranged weapon, returning
    /// the latch to press to use it.  This also checks ammo/ammoknown.

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    pstate->argument = 0;

    // Check right hand
    tTmp = pchr->holdingwhich[SLOT_RIGHT];
    if ( ACTIVE_CHR( tTmp ) )
    {
        cap_t * pcap = chr_get_pcap( tTmp );

        if ( NULL != pcap && pcap->isranged && ( ChrList.lst[tTmp].ammomax == 0 || ( ChrList.lst[tTmp].ammo != 0 && ChrList.lst[tTmp].ammoknown ) ) )
        {
            if ( pstate->argument == 0 || ( update_wld & 1 ) )
            {
                pstate->argument = LATCHBUTTON_RIGHT;
                returncode = btrue;
            }
        }
    }

    if ( !returncode )
    {
        // Check left hand
        tTmp = pchr->holdingwhich[SLOT_LEFT];
        if ( ACTIVE_CHR( tTmp ) )
        {
            cap_t * pcap = chr_get_pcap( tTmp );

            if ( NULL != pcap && pcap->isranged && ( ChrList.lst[tTmp].ammomax == 0 || ( ChrList.lst[tTmp].ammo != 0 && ChrList.lst[tTmp].ammoknown ) ) )
            {
                pstate->argument = LATCHBUTTON_LEFT;
                returncode = btrue;
            }
        }

    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingMeleeWeapon( script_state_t * pstate, ai_state_t * pself )
{
    // IfHoldingMeleeWeapon()
    /// @details ZZ@> This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    pstate->argument = 0;

    if ( !returncode )
    {
        // Check right hand
        sTmp = pchr->holdingwhich[SLOT_RIGHT];
        if ( ACTIVE_CHR( sTmp ) )
        {
            cap_t * pcap = chr_get_pcap( sTmp );

            if ( NULL != pcap && !pcap->isranged && pcap->weaponaction != ACTION_PA )
            {
                if ( pstate->argument == 0 || ( update_wld & 1 ) )
                {
                    pstate->argument = LATCHBUTTON_RIGHT;
                    returncode = btrue;
                }
            }
        }
    }

    if ( !returncode )
    {
        // Check left hand
        sTmp = pchr->holdingwhich[SLOT_LEFT];
        if ( ACTIVE_CHR( sTmp ) )
        {
            cap_t * pcap = chr_get_pcap( sTmp );

            if ( NULL != pcap && !pcap->isranged && pcap->weaponaction != ACTION_PA )
            {
                pstate->argument = LATCHBUTTON_LEFT;
                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingShield( script_state_t * pstate, ai_state_t * pself )
{
    // IfHoldingShield()
    /// @details ZZ@> This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it. The button will need to be held down.

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    pstate->argument = 0;

    if ( !returncode )
    {
        // Check right hand
        sTmp = pchr->holdingwhich[SLOT_RIGHT];
        if ( ACTIVE_CHR( sTmp ) )
        {
            cap_t * pcap = chr_get_pcap( sTmp );

            if ( NULL != pcap && pcap->weaponaction == ACTION_PA )
            {
                pstate->argument = LATCHBUTTON_RIGHT;
                returncode = btrue;
            }
        }
    }

    if ( !returncode )
    {
        // Check left hand
        sTmp = pchr->holdingwhich[SLOT_LEFT];
        if ( ACTIVE_CHR( sTmp ) )
        {
            cap_t * pcap = chr_get_pcap( sTmp );

            if ( NULL != pcap && pcap->weaponaction == ACTION_PA )
            {
                pstate->argument = LATCHBUTTON_LEFT;
                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Kursed( script_state_t * pstate, ai_state_t * pself )
{
    // IfKursed()
    /// @details ZZ@> This function proceeds if the character is kursed

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->iskursed;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsKursed( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsKursed()
    /// @details ZZ@> This function proceeds if the target is kursed

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList.lst[pself->target].iskursed;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsDressedUp( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsDressedUp()
    /// @details ZZ@> This function proceeds if the target is dressed in fancy clothes

    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    pcap = pro_get_pcap( pchr->iprofile );

    returncode = bfalse;
    if ( NULL != pcap )
    {
        returncode = HAS_SOME_BITS( pcap->skindressy, 1 << pchr->skin );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OverWater( script_state_t * pstate, ai_state_t * pself )
{
    // IfOverWater()
    /// @details ZZ@> This function proceeds if the character is on a water tile

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( VALID_TILE( PMesh, pchr->onwhichfan ) )
    {
        returncode = (( 0 != mesh_test_fx( PMesh, pchr->onwhichfan, MPDFX_WATER ) ) && water.is_water );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Thrown( script_state_t * pstate, ai_state_t * pself )
{
    // IfThrown()
    /// @details ZZ@> This function proceeds if the character was thrown this update.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_THROWN );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeNameKnown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeNameKnown()
    /// @details ZZ@> This function makes the name of the character known, for identifying
    /// weapons and spells and such

    SCRIPT_FUNCTION_BEGIN();

    pchr->nameknown = btrue;
    //           pchr->icon = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeUsageKnown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeUsageKnown()
    /// @details ZZ@> This function makes the usage known for this type of object
    /// For XP gains from using an unknown potion or such

    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    pcap = pro_get_pcap( pchr->iprofile );

    returncode = bfalse;
    if ( NULL != pcap )
    {
        pcap->usageknown = btrue;
        returncode       = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopTargetMovement( script_state_t * pstate, ai_state_t * pself )
{
    // StopTargetMovement()
    /// @details ZZ@> This function makes the target stop moving temporarily
    /// Sets the target's x and y velocities to 0, and
    /// sets the z velocity to 0 if the character is moving upwards.
    /// This is a special function for the IronBall object

    SCRIPT_FUNCTION_BEGIN();

    ChrList.lst[pself->target].vel.x = 0;
    ChrList.lst[pself->target].vel.y = 0;
    if ( ChrList.lst[pself->target].vel.z > 0 ) ChrList.lst[pself->target].vel.z = gravity;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_XY( script_state_t * pstate, ai_state_t * pself )
{
    // SetXY( tmpargument = "index", tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function sets one of the 8 permanent storage variable slots
    /// ( each of which holds an x,y pair )

    SCRIPT_FUNCTION_BEGIN();

    pself->x[pstate->argument&STOR_AND] = pstate->x;
    pself->y[pstate->argument&STOR_AND] = pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_XY( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx,tmpy = GetXY( tmpargument = "index" )
    /// @details ZZ@> This function reads one of the 8 permanent storage variable slots,
    /// setting tmpx and tmpy accordingly

    SCRIPT_FUNCTION_BEGIN();

    pstate->x = pself->x[pstate->argument&STOR_AND];
    pstate->y = pself->y[pstate->argument&STOR_AND];

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddXY( script_state_t * pstate, ai_state_t * pself )
{
    // AddXY( tmpargument = "index", tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function alters the contents of one of the 8 permanent storage
    /// slots

    SCRIPT_FUNCTION_BEGIN();

    pself->x[pstate->argument&STOR_AND] += pstate->x;
    pself->y[pstate->argument&STOR_AND] += pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeAmmoKnown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeAmmoKnown()
    /// @details ZZ@> This function makes the character's ammo known ( for items )

    SCRIPT_FUNCTION_BEGIN();

    pchr->ammoknown = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedParticle( tmpargument = "particle", tmpdistance = "vertex" )
    /// @details ZZ@> This function spawns a particle attached to the character

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->index;
    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        sTmp = pchr->attachedto;
    }

    tTmp = spawn_one_particle( pchr->pos, pchr->turn_z, pchr->iprofile, pstate->argument, pself->index, pstate->distance, pchr->team, sTmp, TOTAL_MAX_PRT, 0, MAX_CHR );

    returncode = ACTIVE_PRT( tTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnExactParticle( tmpargument = "particle", tmpx = "x", tmpy = "y", tmpdistance = "z" )
    /// @details ZZ@> This function spawns a particle at a specific x, y, z position

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->index;
    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        sTmp = pchr->attachedto;
    }

    {
        fvec3_t   vtmp = VECT3( pstate->x, pstate->y, pstate->distance );
        tTmp = spawn_one_particle( vtmp, pchr->turn_z, pchr->iprofile, pstate->argument, MAX_CHR, 0, pchr->team, sTmp, TOTAL_MAX_PRT, 0, MAX_CHR );
    }

    returncode = ACTIVE_PRT( tTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateTarget( script_state_t * pstate, ai_state_t * pself )
{
    // AccelerateTarget( tmpx = "acc x", tmpy = "acc y" )
    /// @details ZZ@> This function changes the x and y speeds of the target

    SCRIPT_FUNCTION_BEGIN();

    ChrList.lst[pself->target].vel.x += pstate->x;
    ChrList.lst[pself->target].vel.y += pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_distanceIsMoreThanTurn( script_state_t * pstate, ai_state_t * pself )
{
    // IfdistanceIsMoreThanTurn()
    /// @details ZZ@> This function proceeds tmpdistance is greater than tmpturn

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->distance > pstate->turn );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Crushed( script_state_t * pstate, ai_state_t * pself )
{
    // IfCrushed()
    /// @details ZZ@> This function proceeds if the character was crushed in a passage this
    /// update.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CRUSHED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeCrushValid( script_state_t * pstate, ai_state_t * pself )
{
    // MakeCrushValid()
    /// @details ZZ@> This function makes a character able to be crushed by closing doors
    /// and such

    SCRIPT_FUNCTION_BEGIN();

    pchr->canbecrushed = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLowestTarget( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToLowestTarget()
    /// @details ZZ@> This function sets the target to the absolute bottom character.
    /// The holder of the target, or the holder of the holder of the target, or
    /// the holder of the holder of ther holder of the target, etc.   This function never fails

    SCRIPT_FUNCTION_BEGIN();

    pself->target = chr_get_lowest_attachment( pself->target, bfalse );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotPutAway( script_state_t * pstate, ai_state_t * pself )
{
    // IfNotPutAway()
    /// @details ZZ@> This function proceeds if the character couldn't be put into another
    /// character's pockets for some reason.
    /// It might be kursed or too big or something

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_NOTPUTAWAY );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TakenOut( script_state_t * pstate, ai_state_t * pself )
{
    // IfTakenOut()
    /// @details ZZ@> This function proceeds if the character is equiped in another's inventory,
    /// and the holder tried to unequip it ( take it out of pack ), but the
    /// item was kursed and didn't cooperate

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_TAKENOUT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AmmoOut( script_state_t * pstate, ai_state_t * pself )
{
    // IfAmmoOut()
    /// @details ZZ@> This function proceeds if the character itself has no ammo left.
    /// This is for crossbows and such, not archers.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pchr->ammo == 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySoundLooped( script_state_t * pstate, ai_state_t * pself )
{
    // PlaySoundLooped( tmpargument = "sound", tmpdistance = "frequency" )

    /// @details ZZ@> This function starts playing a continuous sound

    Mix_Chunk * new_chunk;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    new_chunk = chr_get_chunk_ptr( pchr, pstate->argument );

    if ( NULL == new_chunk )
    {
        looped_stop_object_sounds( pself->index );       // Stop existing sound loop (if any)
    }
    else
    {
        Mix_Chunk * playing_chunk = NULL;

        // check whatever might be playing on the channel now
        if ( INVALID_SOUND != pchr->loopedsound_channel )
        {
            playing_chunk = Mix_GetChunk( pchr->loopedsound_channel );
        }

        if ( playing_chunk != new_chunk )
        {
            pchr->loopedsound_channel = sound_play_chunk_looped( pchr->pos_old, new_chunk, -1, pself->index );
        }
    }

    returncode = ( -1 != pchr->loopedsound_channel );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopSound( script_state_t * pstate, ai_state_t * pself )
{
    // StopSound( tmpargument = "sound" )
    /// @details ZZ@> This function stops the playing of a continuous sound!

    SCRIPT_FUNCTION_BEGIN();

    looped_stop_object_sounds( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HealSelf( script_state_t * pstate, ai_state_t * pself )
{
    // HealSelf()
    /// @details ZZ@> This function gives life back to the character.
    /// Values given as 8.8-bit fixed point
    /// This does NOT remove [HEAL] enchants ( poisons )
    /// This does not set the ALERTIF_HEALED alert

    SCRIPT_FUNCTION_BEGIN();

    heal_character( pself->index, pself->index, pstate->argument, btrue );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Equip( script_state_t * pstate, ai_state_t * pself )
{
    // Equip()
    /// @details ZZ@> This function flags the character as being equipped.
    /// This is used by equipment items when they are placed in the inventory

    SCRIPT_FUNCTION_BEGIN();

    pchr->isequipped = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasItemIDEquipped( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasItemIDEquipped( tmpargument = "item idsz" )
    /// @details ZZ@> This function proceeds if the target already wearing a matching item

    Uint16 item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_has_inventory_idsz( pself->target, pstate->argument, btrue, NULL );

    returncode = ACTIVE_CHR( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_OwnerToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // SetOwnerToTarget()
    /// @details ZZ@> This function must be called before enchanting anything.
    /// The owner is the character that pays the sustain costs and such for the enchantment

    SCRIPT_FUNCTION_BEGIN();

    pself->owner = pself->target;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToOwner( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToOwner()
    /// @details ZZ@> This function sets the target to whoever was previously declared as the
    /// owner.

    SCRIPT_FUNCTION_BEGIN();

    pself->target = pself->owner;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Frame( script_state_t * pstate, ai_state_t * pself )
{
    // SetFrame( tmpargument = "frame" )
    /// @details ZZ@> This function sets the current .MD2 frame for the character.  Values are * 4

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pstate->argument & 3;
    iTmp = pstate->argument >> 2;
    chr_set_frame( pself->index, ACTION_DA, iTmp, sTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BreakPassage( script_state_t * pstate, ai_state_t * pself )
{
    // BreakPassage( tmpargument = "passage", tmpturn = "tile type", tmpdistance = "number of frames", tmpx = "borken tile", tmpy = "tile fx bits" )

    /// @details ZZ@> This function makes the tiles fall away ( turns into damage terrain )
    /// This function causes the tiles of a passage to increment if stepped on.
    /// tmpx and tmpy are both set to the location of whoever broke the tile if
    /// the function passed.

    SCRIPT_FUNCTION_BEGIN();

    returncode = _break_passage( pstate->y, pstate->x, pstate->distance, pstate->turn, pstate->argument, &( pstate->x ), &( pstate->y ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ReloadTime( script_state_t * pstate, ai_state_t * pself )
{
    // SetReloadTime( tmpargument = "time" )
    /// @details ZZ@> This function stops a character from being used for a while.  Used
    /// by weapons to slow down their attack rate.  50 clicks per second.

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->argument > 0 ) pchr->reloadtime = pstate->argument;
    else pchr->reloadtime = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWideBlahID( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWideBlahID( tmpargument = "idsz", tmpdistance = "blah bits" )
    /// @details ZZ@> This function sets the target to a character that matches the description,
    /// and who is located in the general vicinity of the character

    Uint16 ichr;
    TARGET_TYPE blahteam;

    SCRIPT_FUNCTION_BEGIN();

    blahteam = TARGET_NONE;
    returncode = bfalse;

    // Determine which team to target
    if (( pstate->distance >> 3 ) & 1 ) blahteam = TARGET_ALL;
    if (( pstate->distance >> 2 ) & 1 ) blahteam = TARGET_FRIEND;
    if (( pstate->distance >> 1 ) & 1 ) blahteam = ( TARGET_FRIEND  == blahteam ) ? TARGET_ALL : TARGET_ENEMY;

    // Try to find one
    ichr = _get_chr_target( pchr, WIDE, blahteam, ( pstate->distance >> 3 ) & 1 , ( pstate->distance ) & 1,
                            pstate->argument, ( pstate->distance >> 4 ) & 1, ( pstate->distance >> 5 ) & 1 );

    returncode = ( ichr != pself->index ) && ACTIVE_CHR( ichr );

    if ( returncode )
    {
        pself->target = ichr;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PoofTarget( script_state_t * pstate, ai_state_t * pself )
{
    // PoofTarget()
    /// @details ZZ@> This function removes the target from the game, failing if the
    /// target is a player

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( !ChrList.lst[pself->target].isplayer )
    {
        returncode = btrue;
        if ( pself->target == pself->index )
        {
            // Poof self later
            pself->poof_time = update_wld + 1;
        }
        else
        {
            // Poof others now
            ChrList.lst[pself->target].ai.poof_time = update_wld;
            pself->target = pself->index;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChildDoActionOverride( script_state_t * pstate, ai_state_t * pself )
{
    // ChildDoActionOverride( tmpargument = action )

    /// @details ZZ@> This function lets a character set the action of the last character
    /// it spawned.  It also sets the current frame to the first frame of the
    /// action ( no interpolation from last frame ). If the cation is not valid for the model,
    /// the function will fail

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ACTIVE_CHR( pself->child ) )
    {
        int action;
        chr_t * pchild = ChrList.lst + pself->child;

        action = mad_get_action( pchild->inst.imad, pstate->argument );
        if ( action < ACTION_COUNT )
        {
            if ( MadList[pchild->inst.imad].action_valid[action] )
            {
                pchild->inst.action_which = action;
                pchild->inst.action_ready = bfalse;

                pchild->inst.ilip         = 0;
                pchild->inst.flip         = 0;
                pchild->inst.frame_nxt    = MadList[pchild->inst.imad].action_stt[action];
                pchild->inst.frame_lst    = pchild->inst.frame_nxt;

                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnPoof( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnPoof
    /// @details ZZ@> This function makes a lovely little poof at the character's location.
    /// The poof form and particle types are set in data.txt

    SCRIPT_FUNCTION_BEGIN();

    spawn_poof( pself->index, pchr->iprofile );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SpeedPercent( script_state_t * pstate, ai_state_t * pself )
{
    // SetSpeedPercent( tmpargument = "percent" )
    /// @details ZZ@> This function acts like Run or Walk, except it allows the explicit
    /// setting of the speed

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );
    pchr->maxaccel = pchr->maxaccel * pstate->argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildState( script_state_t * pstate, ai_state_t * pself )
{
    // SetChildState( tmpargument = "state" )
    /// @details ZZ@> This function lets a character set the state of the last character it
    /// spawned

    SCRIPT_FUNCTION_BEGIN();

    ChrList.lst[pself->child].ai.state = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedSizedParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedSizedParticle( tmpargument = "particle", tmpdistance = "vertex", tmpturn = "size" )
    /// @details ZZ@> This function spawns a particle of the specific size attached to the
    /// character. For spell charging effects

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->index;
    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        sTmp = pchr->attachedto;
    }

    tTmp = spawn_one_particle( pchr->pos, pchr->turn_z, pchr->iprofile, pstate->argument, pself->index, pstate->distance, pchr->team, sTmp, TOTAL_MAX_PRT, 0, MAX_CHR );

    returncode = ACTIVE_PRT( tTmp );

    if ( returncode )
    {
        PrtList.lst[tTmp].size = pstate->turn;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeArmor( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeArmor( tmpargument = "time" )
    /// @details ZZ@> This function changes the character's armor.
    /// Sets tmpargument as the old type and tmpx as the new type

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    pstate->x = pstate->argument;
    iTmp = pchr->skin;
    pstate->x = change_armor( pself->index, pstate->argument );
    pstate->argument = iTmp;  // The character's old armor

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowTimer( script_state_t * pstate, ai_state_t * pself )
{
    // ShowTimer( tmpargument = "time" )
    /// @details ZZ@> This function sets the value displayed by the module timer.
    /// For races and such.  50 clicks per second

    SCRIPT_FUNCTION_BEGIN();

    timeron = btrue;
    timervalue = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FacingTarget( script_state_t * pstate, ai_state_t * pself )
{
    // IfFacingTarget()
    /// @details ZZ@> This function proceeds if the character is more or less facing its
    /// target

    SCRIPT_FUNCTION_BEGIN();

    sTmp = vec_to_facing( ChrList.lst[pself->target].pos.x - pchr->pos.x , ChrList.lst[pself->target].pos.y - pchr->pos.y );
    sTmp -= pchr->turn_z;
    returncode = ( sTmp > 55535 || sTmp < 10000 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySoundVolume( script_state_t * pstate, ai_state_t * pself )
{
    // PlaySoundVolume( argument = "sound", distance = "volume" )
    /// @details ZZ@> This function sets the volume of a sound and plays it

    int iTmp;
    int volume;

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->distance >= 0 )
    {
        volume = pstate->distance;
        iTmp = INVALID_SOUND;
        if ( VALID_SND( pstate->argument ) )
        {
            iTmp = sound_play_chunk( pchr->pos_old, chr_get_chunk_ptr( pchr, pstate->argument ) );
        }

        if ( INVALID_SOUND != iTmp )
        {
            Mix_Volume( iTmp, pstate->distance );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedFacedParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedFacedParticle(  tmpargument = "particle", tmpdistance = "vertex", tmpturn = "turn" )

    /// @details ZZ@> This function spawns a particle attached to the character, facing the
    /// same direction given by tmpturn

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->index;
    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        sTmp = pchr->attachedto;
    }

    tTmp = spawn_one_particle( pchr->pos, CLIP_TO_16BITS( pstate->turn ), pchr->iprofile, pstate->argument, pself->index, pstate->distance, pchr->team, sTmp, TOTAL_MAX_PRT, 0, MAX_CHR );

    returncode = ACTIVE_PRT( tTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIsOdd( script_state_t * pstate, ai_state_t * pself )
{
    // IfStateIsOdd()
    /// @details ZZ@> This function proceeds if the character's state is 1, 3, 5, 7, etc.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->state & 1 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToDistantEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToDistantEnemy( tmpdistance = "distance" )
    /// @details ZZ@> This function finds a character within a certain distance of the
    /// character, failing if there are none

    Uint16 ichr;
    SCRIPT_FUNCTION_BEGIN();

    ichr = _get_chr_target( pchr, pstate->distance, TARGET_ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse, bfalse );

    returncode = ( ichr != pself->index ) && ACTIVE_CHR( ichr );

    if ( returncode )
    {
        pself->target = ichr;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Teleport( script_state_t * pstate, ai_state_t * pself )
{
    // Teleport( tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function teleports the character to a new location, failing if
    /// the location is blocked or off the map

    SCRIPT_FUNCTION_BEGIN();

    returncode = chr_teleport( pself->index, pstate->x, pstate->y, pchr->pos.z, pchr->turn_z );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveStrengthToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveStrengthToTarget()
    // Permanently boost the target's strength

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList.lst[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList.lst[pself->target].strength, PERFECTSTAT, &iTmp );
        ChrList.lst[pself->target].strength += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveWisdomToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveWisdomToTarget()
    // Permanently boost the target's wisdom

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList.lst[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList.lst[pself->target].wisdom, PERFECTSTAT, &iTmp );
        ChrList.lst[pself->target].wisdom += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveIntelligenceToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveIntelligenceToTarget()
    // Permanently boost the target's intelligence

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList.lst[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList.lst[pself->target].intelligence, PERFECTSTAT, &iTmp );
        ChrList.lst[pself->target].intelligence += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveDexterityToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveDexterityToTarget()
    // Permanently boost the target's dexterity

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList.lst[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList.lst[pself->target].dexterity, PERFECTSTAT, &iTmp );
        ChrList.lst[pself->target].dexterity += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveLifeToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveLifeToTarget()
    /// @details ZZ@> Permanently boost the target's life

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList.lst[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( LOWSTAT, ChrList.lst[pself->target].lifemax, PERFECTBIG, &iTmp );
        ChrList.lst[pself->target].lifemax += iTmp;
        if ( iTmp < 0 )
        {
            getadd( 1, ChrList.lst[pself->target].life, PERFECTBIG, &iTmp );
        }

        ChrList.lst[pself->target].life += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveManaToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveManaToTarget()
    /// @details ZZ@> Permanently boost the target's mana

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList.lst[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList.lst[pself->target].manamax, PERFECTBIG, &iTmp );
        ChrList.lst[pself->target].manamax += iTmp;
        if ( iTmp < 0 )
        {
            getadd( 0, ChrList.lst[pself->target].mana, PERFECTBIG, &iTmp );
        }

        ChrList.lst[pself->target].mana += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowMap( script_state_t * pstate, ai_state_t * pself )
{
    // ShowMap()
    /// @details ZZ@> This function shows the module's map.
    /// Fails if map already visible

    SCRIPT_FUNCTION_BEGIN();
    if ( mapon )  returncode = bfalse;

    mapon = mapvalid;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowYouAreHere( script_state_t * pstate, ai_state_t * pself )
{
    // ShowYouAreHere()
    /// @details ZZ@> This function shows the blinking white blip on the map that represents the
    /// camera location

    SCRIPT_FUNCTION_BEGIN();

    youarehereon = mapvalid;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowBlipXY( script_state_t * pstate, ai_state_t * pself )
{
    // ShowBlipXY( tmpx = "x", tmpy = "y", tmpargument = "color" )

    /// @details ZZ@> This function draws a blip on the map, and must be done each update

    SCRIPT_FUNCTION_BEGIN();

    // Add a blip
    if ( numblip < MAXBLIP )
    {
        if ( pstate->x > 0 && pstate->x < PMesh->info.edge_x && pstate->y > 0 && pstate->y < PMesh->info.edge_y )
        {
            if ( pstate->argument < COLOR_MAX && pstate->argument >= 0 )
            {
                blipx[numblip] = pstate->x * MAPSIZE / PMesh->info.edge_x;
                blipy[numblip] = pstate->y * MAPSIZE / PMesh->info.edge_y;
                blipc[numblip] = pstate->argument;
                numblip++;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HealTarget( script_state_t * pstate, ai_state_t * pself )
{
    // HealTarget( tmpargument = "amount" )
    /// @details ZZ@> This function gives some life back to the target.
    /// Values are 8.8-bit fixed point. Any enchantments that are removed by [HEAL], like poison, go away

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( heal_character( pself->target, pself->index, pstate->argument, bfalse ) )
    {
        Uint16 enc_now, enc_next;
        eve_t * peve;

        returncode = btrue;

        // clean up the enchant list before doing anything
        ChrList.lst[pself->target].firstenchant = cleanup_enchant_list( ChrList.lst[pself->target].firstenchant );

        // Check all enchants to see if they are removed
        enc_now = ChrList.lst[pself->target].firstenchant;
        while ( enc_now != MAX_ENC )
        {
            IDSZ test = MAKE_IDSZ( 'H', 'E', 'A', 'L' );
            enc_next  = EncList.lst[enc_now].nextenchant_ref;

            peve = enc_get_peve( enc_now );
            if ( NULL != peve && test == peve->removedbyidsz )
            {
                remove_enchant( enc_now );
            }

            enc_now = enc_next;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PumpTarget( script_state_t * pstate, ai_state_t * pself )
{
    // PumpTarget( tmpargument = "amount" )
    /// @details ZZ@> This function gives some mana back to the target.
    /// Values are 8.8-bit fixed point

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // Give some mana to the target
    if ( ChrList.lst[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList.lst[pself->target].mana, ChrList.lst[pself->target].manamax, &iTmp );
        ChrList.lst[pself->target].mana += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // CostAmmo()
    /// @details ZZ@> This function costs the character 1 point of ammo

    SCRIPT_FUNCTION_BEGIN();
    if ( pchr->ammo > 0 )
    {
        pchr->ammo--;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeSimilarNamesKnown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeSimilarNamesKnown()
    /// @details ZZ@> This function makes the names of similar objects known.
    /// Checks all 6 IDSZ types to make sure they match.

    int tTmp;
    int iTmp;
    cap_t * pcap_chr;

    SCRIPT_FUNCTION_BEGIN();

    pcap_chr = pro_get_pcap( pchr->iprofile );
    if ( NULL == pcap_chr ) return bfalse;

    for ( iTmp = 0; iTmp < MAX_CHR; iTmp++ )
    {
        cap_t * pcap_test;

        pcap_test = chr_get_pcap( iTmp );
        if ( NULL == pcap_test ) continue;

        sTmp = btrue;
        for ( tTmp = 0; tTmp < IDSZ_COUNT; tTmp++ )
        {
            if ( pcap_chr->idsz[tTmp] != pcap_test->idsz[tTmp] )
            {
                sTmp = bfalse;
            }
        }

        if ( sTmp )
        {
            ChrList.lst[iTmp].nameknown = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedHolderParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedHolderParticle( tmpargument = "particle", tmpdistance = "vertex" )

    /// @details ZZ@> This function spawns a particle attached to the character's holder, or to the character if no holder

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->index;
    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        sTmp = pchr->attachedto;
    }

    tTmp = spawn_one_particle( pchr->pos, pchr->turn_z, pchr->iprofile, pstate->argument, sTmp, pstate->distance, pchr->team, sTmp, TOTAL_MAX_PRT, 0, MAX_CHR );

    returncode = ACTIVE_PRT( tTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetReloadTime( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetReloadTime( tmpargument = "time" )

    /// @details ZZ@> This function sets the target's reload time
    /// This function stops the target from attacking for a while.

    SCRIPT_FUNCTION_BEGIN();
    if ( pstate->argument > 0 )
        ChrList.lst[pself->target].reloadtime = pstate->argument;
    else ChrList.lst[pself->target].reloadtime = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogLevel( script_state_t * pstate, ai_state_t * pself )
{
    // SetFogLevel( tmpargument = "level" )
    /// @details ZZ@> This function sets the level of the module's fog.
    /// Values are * 10
    /// !!BAD!! DOESN'T WORK !!BAD!!

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( pstate->argument / 10.0f ) - fog.top;
    fog.top += fTmp;
    fog.distance += fTmp;
    fog.on = cfg.fog_allowed;
    if ( fog.distance < 1.0f )  fog.on = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_FogLevel( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetFogLevel()
    /// @details ZZ@> This function sets tmpargument to the level of the module's fog.
    /// Values are * 10

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = fog.top * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogTAD( script_state_t * pstate, ai_state_t * pself )
{
    /// @details ZZ@> This function sets the color of the module's fog.
    /// TAD stands for <turn, argument, distance> == <red, green, blue>.
    /// Makes sense, huh?
    /// !!BAD!! DOESN'T WORK !!BAD!!

    SCRIPT_FUNCTION_BEGIN();

    fog.red = pstate->turn;
    fog.grn = pstate->argument;
    fog.blu = pstate->distance;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogBottomLevel( script_state_t * pstate, ai_state_t * pself )
{
    // SetFogBottomLevel( tmpargument = "level" )

    /// @details ZZ@> This function sets the level of the module's fog.
    /// Values are * 10

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( pstate->argument / 10.0f ) - fog.bottom;
    fog.bottom += fTmp;
    fog.distance -= fTmp;
    fog.on = cfg.fog_allowed;
    if ( fog.distance < 1.0f )  fog.on = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_FogBottomLevel( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetFogBottomLevel()

    /// @details ZZ@> This function sets tmpargument to the level of the module's fog.
    /// Values are * 10

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = fog.bottom * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CorrectActionForHand( script_state_t * pstate, ai_state_t * pself )
{
    // CorrectActionForHand( tmpargument = "action" )
    /// @details ZZ@> This function changes tmpargument according to which hand the character
    /// is held in It turns ZA into ZA, ZB, ZC, or ZD.
    /// USAGE:  wizards casting spells

    SCRIPT_FUNCTION_BEGIN();
    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        if ( pchr->inwhich_slot == SLOT_LEFT )
        {
            // A or B
            pstate->argument = generate_randmask( pstate->argument, 1 );
        }
        else
        {
            // C or D
            pstate->argument = generate_randmask( pstate->argument + 2, 1 );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsMounted( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsMounted()
    /// @details ZZ@> This function proceeds if the target is riding a mount

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    sTmp = ChrList.lst[pself->target].attachedto;

    if ( ACTIVE_CHR( sTmp ) )
    {
        returncode = ChrList.lst[sTmp].ismount;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SparkleIcon( script_state_t * pstate, ai_state_t * pself )
{
    // SparkleIcon( tmpargument = "color" )
    /// @details ZZ@> This function starts little sparklies going around the character's icon

    SCRIPT_FUNCTION_BEGIN();
    if ( pstate->argument < COLOR_MAX )
    {
        if ( pstate->argument < -1 || pstate->argument >= COLOR_MAX )
        {
            pchr->sparkle = NOSPARKLE;
        }
        else
        {
            pchr->sparkle = pstate->argument;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnsparkleIcon( script_state_t * pstate, ai_state_t * pself )
{
    // UnsparkleIcon()
    /// @details ZZ@> This function stops little sparklies going around the character's icon

    SCRIPT_FUNCTION_BEGIN();

    pchr->sparkle = NOSPARKLE;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TileXY( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTileXY( tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function sets tmpargument to the tile type at the specified
    /// coordinates

    Uint32 iTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    iTmp = mesh_get_tile( PMesh, pstate->x, pstate->y );
    if ( VALID_TILE( PMesh, iTmp ) )
    {
        returncode = btrue;
        pstate->argument = CLIP_TO_08BITS( PMesh->mmem.tile_list[iTmp].img );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TileXY( script_state_t * pstate, ai_state_t * pself )
{
    // scr_set_TileXY( tmpargument = "tile type", tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function changes the tile type at the specified coordinates

    Uint32 iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp       = mesh_get_tile( PMesh, pstate->x, pstate->y );
    returncode = mesh_set_texture( PMesh, iTmp, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ShadowSize( script_state_t * pstate, ai_state_t * pself )
{
    // SetShadowSize( tmpargument = "size" )
    /// @details ZZ@> This function makes the character's shadow bigger or smaller

    SCRIPT_FUNCTION_BEGIN();

    pchr->shadowsize     = pstate->argument * pchr->fat;
    pchr->shadowsizesave = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OrderTarget( script_state_t * pstate, ai_state_t * pself )
{
    // OrderTarget( tmpargument = "order" )
    /// @details ZZ@> This function issues an order to the given target
    /// Be careful in using this, always checking IDSZ first

    SCRIPT_FUNCTION_BEGIN();

    if ( !ACTIVE_CHR( pself->target ) )
    {
        returncode = bfalse;
    }
    else
    {
        returncode = ai_add_order( &( ChrList.lst[pself->target].ai ), pstate->argument, 0 );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverIsInPassage( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverIsInPassage()
    /// @details ZZ@> This function sets the target to whoever is blocking the given passage
    /// This function lets passage rectangles be used as event triggers

    SCRIPT_FUNCTION_BEGIN();

    sTmp = who_is_blocking_passage( pstate->argument, btrue, btrue, bfalse, bfalse, 0 );
    returncode = bfalse;
    if ( ACTIVE_CHR( sTmp ) )
    {
        pself->target = sTmp;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CharacterWasABook( script_state_t * pstate, ai_state_t * pself )
{
    // IfCharacterWasABook()
    /// @details ZZ@> This function proceeds if the base model is the same as the current
    /// model or if the base model is SPELLBOOK
    /// USAGE: USED BY THE MORPH SPELL. Not much use elsewhere

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pchr->basemodel == SPELLBOOK ||
                   pchr->basemodel == pchr->iprofile );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_EnchantBoostValues( script_state_t * pstate, ai_state_t * pself )
{
    // SetEnchantBoostValues( tmpargument = "owner mana regen", tmpdistance = "owner life regen", tmpx = "target mana regen", tmpy = "target life regen" )
    /// @details ZZ@> This function sets the mana and life drains for the last enchantment
    /// spawned by this character.
    /// Values are 8.8-bit fixed point

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = pchr->undoenchant;

    returncode = bfalse;
    if ( ACTIVE_ENC( iTmp ) )
    {
        EncList.lst[iTmp].ownermana = pstate->argument;
        EncList.lst[iTmp].ownerlife = pstate->distance;
        EncList.lst[iTmp].targetmana = pstate->x;
        EncList.lst[iTmp].targetlife = pstate->y;

        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnCharacterXYZ( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnCharacterXYZ( tmpx = "x", tmpy = "y", tmpdistance = "z", tmpturn = "turn" )
    /// @details ZZ@> This function spawns a character of the same type at a specific location, failing if x,y,z is invalid

    fvec3_t   pos;

    SCRIPT_FUNCTION_BEGIN();

    pos.x = pstate->x;
    pos.y = pstate->y;
    pos.z = pstate->distance;

    sTmp = spawn_one_character( pos, pchr->iprofile, pchr->team, 0, CLIP_TO_16BITS( pstate->turn ), NULL, MAX_CHR );

    if ( !ACTIVE_CHR( sTmp ) )
    {
        if ( sTmp > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "Object %s failed to spawn a copy of itself\n", pchr->obj_base._name );
        }
    }
    else
    {
        chr_t * pchild = ChrList.lst + sTmp;

        // was the child spawned in a "safe" spot?
        if ( !pchild->safe_valid )
        {
            chr_request_terminate( sTmp );
            sTmp = MAX_CHR;
        }
        else
        {
            pself->child = sTmp;

            pchild->iskursed   = pchr->iskursed;  // BB> inherit this from your spawner
            pchild->ai.passage = pself->passage;
            pchild->ai.owner   = pself->owner;
        }
    }

    returncode = ACTIVE_CHR( sTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactCharacterXYZ( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnCharacterXYZ( tmpargument = "slot", tmpx = "x", tmpy = "y", tmpdistance = "z", tmpturn = "turn" )
    /// @details ZZ@> This function spawns a character at a specific location, using a
    /// specific model type, failing if x,y,z is invalid
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS,
    /// AS THE MODEL SLOTS MAY VARY FROM MODULE TO MODULE.

    fvec3_t   pos;

    SCRIPT_FUNCTION_BEGIN();

    pos.x = pstate->x;
    pos.y = pstate->y;
    pos.z = pstate->distance;

    sTmp = spawn_one_character( pos, pstate->argument, pchr->team, 0, CLIP_TO_16BITS( pstate->turn ), NULL, MAX_CHR );
    if ( !ACTIVE_CHR( sTmp ) )
    {
        if ( sTmp > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            cap_t * pcap = pro_get_pcap( pchr->iprofile );

            log_warning( "Object \"%s\"(\"%s\") failed to spawn profile index %d\n", pchr->obj_base._name, NULL == pcap ? "INVALID" : pcap->classname, pstate->argument );
        }
    }
    else
    {
        chr_t * pchild = ChrList.lst + sTmp;

        // was the child spawned in a "safe" spot?
        if ( !pchild->safe_valid )
        {
            chr_request_terminate( sTmp );
            sTmp = MAX_CHR;
        }
        else
        {
            pself->child = sTmp;

            pchild->iskursed   = pchr->iskursed;  // BB> inherit this from your spawner
            pchild->ai.passage = pself->passage;
            pchild->ai.owner   = pself->owner;
        }
    }

    returncode = ACTIVE_CHR( sTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTargetClass( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeTargetClass( tmpargument = "slot" )

    /// @details ZZ@> This function changes the target character's model slot.
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS, AS THE MODEL SLOTS MAY VARY FROM
    /// MODULE TO MODULE.
    /// USAGE: This is intended as a way to incorporate more player classes into the game.

    SCRIPT_FUNCTION_BEGIN();

    change_character_full( pself->target, pstate->argument, 0, LEAVEALL );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlayFullSound( script_state_t * pstate, ai_state_t * pself )
{
    // PlayFullSound( tmpargument = "sound", tmpdistance = "frequency" )
    /// @details ZZ@> This function plays one of the character's sounds .
    /// The sound will be heard at full volume by all players (Victory music)

    SCRIPT_FUNCTION_BEGIN();

    if ( VALID_SND( pstate->argument ) )
    {
        sound_play_chunk( PCamera->track_pos, chr_get_chunk_ptr( pchr, pstate->argument ) );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactChaseParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnExactChaseParticle( tmpargument = "particle", tmpx = "x", tmpy = "y", tmpdistance = "z" )
    /// @details ZZ@> This function spawns a particle at a specific x, y, z position,
    /// that will home in on the character's target

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->index;
    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        sTmp = pchr->attachedto;
    }

    {
        fvec3_t   vtmp = VECT3( pstate->x, pstate->y, pstate->distance );
        tTmp = spawn_one_particle( vtmp, pchr->turn_z, pchr->iprofile, pstate->argument, MAX_CHR, 0, pchr->team, sTmp, TOTAL_MAX_PRT, 0, MAX_CHR );
    }

    returncode = ACTIVE_PRT( tTmp );

    if ( returncode )
    {
        PrtList.lst[tTmp].target_ref = pself->target;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CreateOrder( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = CreateOrder( tmpx = "value1", tmpy = "value2", tmpargument = "order" )

    /// @details ZZ@> This function compresses tmpx, tmpy, tmpargument ( 0 - 15 ), and the
    /// character's target into tmpargument.  This new tmpargument can then
    /// be issued as an order to teammates.  TranslateOrder will undo the
    /// compression

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->target << 24;
    sTmp |= (( pstate->x >> 6 ) & 1023 ) << 14;
    sTmp |= (( pstate->y >> 6 ) & 1023 ) << 4;
    sTmp |= ( pstate->argument & 15 );
    pstate->argument = sTmp;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OrderSpecialID( script_state_t * pstate, ai_state_t * pself )
{
    // OrderSpecialID( tmpargument = "compressed order", tmpdistance = "idsz" )
    /// @details ZZ@> This function orders all characters with the given special IDSZ.

    SCRIPT_FUNCTION_BEGIN();

    issue_special_order( pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkurseTargetInventory( script_state_t * pstate, ai_state_t * pself )
{
    // UnkurseTargetInventory()
    /// @details ZZ@> This function unkurses all items held and in the pockets of the target

    SCRIPT_FUNCTION_BEGIN();

    sTmp = ChrList.lst[pself->target].holdingwhich[SLOT_LEFT];
    if ( ACTIVE_CHR( sTmp ) )
    {
        ChrList.lst[sTmp].iskursed = bfalse;
    }

    sTmp = ChrList.lst[pself->target].holdingwhich[SLOT_RIGHT];
    if ( ACTIVE_CHR( sTmp ) )
    {
        ChrList.lst[sTmp].iskursed = bfalse;
    }

    sTmp = ChrList.lst[pself->target].pack_next;
    while ( sTmp != MAX_CHR )
    {
        if ( ACTIVE_CHR( sTmp ) )
        {
            ChrList.lst[sTmp].iskursed = bfalse;
        }

        sTmp = ChrList.lst[sTmp].pack_next;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsSneaking( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsSneaking()
    /// @details ZZ@> This function proceeds if the target is doing ACTION_WA or ACTION_DA

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList.lst[pself->target].inst.action_which == ACTION_DA || ChrList.lst[pself->target].inst.action_which == ACTION_WA );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropItems( script_state_t * pstate, ai_state_t * pself )
{
    // DropItems()
    /// @details ZZ@> This function drops all of the items the character is holding

    SCRIPT_FUNCTION_BEGIN();

    drop_all_items( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RespawnTarget( script_state_t * pstate, ai_state_t * pself )
{
    // RespawnTarget()
    /// @details ZZ@> This function respawns the target at its current location

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->target;

    ChrList.lst[sTmp].pos_old = ChrList.lst[sTmp].pos;
    respawn_character( sTmp );
    ChrList.lst[sTmp].pos = ChrList.lst[sTmp].pos_old;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDoActionSetFrame( script_state_t * pstate, ai_state_t * pself )
{
    // TargetDoActionSetFrame( tmpargument = "action" )
    /// @details ZZ@> This function starts the target doing the given action, and also sets
    /// the starting frame to the first of the animation ( so there is no
    /// interpolation 'cause it looks awful in some circumstances )
    /// It will fail if the action is invalid

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ACTIVE_CHR( pself->target ) )
    {
        int action;
        chr_t * ptarget = ChrList.lst + pself->target;

        action = mad_get_action( ptarget->inst.imad, pstate->argument );
        if ( action < ACTION_COUNT )
        {
            if ( MadList[ptarget->inst.imad].action_valid[action] )
            {
                ptarget->inst.action_which = action;
                ptarget->inst.action_ready = bfalse;

                ptarget->inst.ilip         = 0;
                ptarget->inst.flip         = 0;
                ptarget->inst.frame_nxt    = MadList[ptarget->inst.imad].action_stt[action];
                ptarget->inst.frame_lst    = ptarget->inst.frame_nxt;

                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanSeeInvisible( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetCanSeeInvisible()
    /// @details ZZ@> This function proceeds if the target can see invisible

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList.lst[pself->target].see_invisible_level;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestBlahID( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearestBlahID( tmpargument = "idsz", tmpdistance = "blah bits" )

    /// @details ZZ@> This function finds the NEAREST ( exact ) character that fits the given
    /// parameters, failing if it finds none

    Uint16 ichr;
    TARGET_TYPE blahteam;

    SCRIPT_FUNCTION_BEGIN();

    blahteam = TARGET_NONE;
    returncode = bfalse;

    // Determine which team to target
    if (( pstate->distance >> 3 ) & 1 ) blahteam = TARGET_ALL;
    if (( pstate->distance >> 2 ) & 1 ) blahteam = TARGET_FRIEND;
    if (( pstate->distance >> 1 ) & 1 ) blahteam = ( TARGET_FRIEND  == blahteam ) ? TARGET_ALL : TARGET_ENEMY;

    // Try to find one
    ichr = _get_chr_target( pchr, NEAREST, blahteam, (( pstate->distance >> 3 ) & 1 ),
                            (( pstate->distance ) & 1 ), pstate->argument, (( pstate->distance >> 4 ) & 1 ), ( pstate->distance >> 5 ) & 1 );

    returncode = ( ichr != pself->index ) && ACTIVE_CHR( ichr );

    if ( returncode )
    {
        pself->target = ichr;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearestEnemy()
    /// @details ZZ@> This function finds the NEAREST ( exact ) enemy, failing if it finds none

    Uint16 ichr;
    SCRIPT_FUNCTION_BEGIN();

    ichr = _get_chr_target( pchr, 0, TARGET_ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse, bfalse );

    returncode = ( ichr != pself->index ) && ACTIVE_CHR( ichr );

    if ( returncode )
    {
        pself->target = ichr;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestFriend( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearestFriend()
    /// @details ZZ@> This function finds the NEAREST ( exact ) friend, failing if it finds none

    Uint16 ichr;
    SCRIPT_FUNCTION_BEGIN();

    ichr = _get_chr_target( pchr, 0, TARGET_FRIEND, bfalse, bfalse, IDSZ_NONE, bfalse, bfalse );

    returncode = ( ichr != pself->index ) && ACTIVE_CHR( ichr );

    if ( returncode )
    {
        pself->target = ichr;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestLifeform( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearestLifeform()

    /// @details ZZ@> This function finds the NEAREST ( exact ) friend or enemy, failing if it
    /// finds none

    Uint16 ichr;
    SCRIPT_FUNCTION_BEGIN();

    ichr = _get_chr_target( pchr, 0, TARGET_ALL, bfalse, bfalse, IDSZ_NONE, bfalse, bfalse );

    returncode = ( ichr != pself->index ) && ACTIVE_CHR( ichr );

    if ( returncode )
    {
        pself->target = ichr;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashPassage( script_state_t * pstate, ai_state_t * pself )
{
    // FlashPassage( tmpargument = "passage", tmpdistance = "color" )

    /// @details ZZ@> This function makes the given passage light or dark.
    /// Usage: For debug purposes

    SCRIPT_FUNCTION_BEGIN();

    flash_passage( pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FindTileInPassage( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx, tmpy = FindTileInPassage( tmpargument = "passage", tmpdistance = "tile type", tmpx, tmpy )

    /// @details ZZ@> This function finds all tiles of the specified type that lie within the
    /// given passage.  Call multiple times to find multiple tiles.  tmpx and
    /// tmpy will be set to the middle of the found tile if one is found, or
    /// both will be set to 0 if no tile is found.
    /// tmpx and tmpy are required and set on return

    SCRIPT_FUNCTION_BEGIN();

    returncode = _find_tile_in_passage( pstate->x, pstate->y, pstate->distance, pstate->argument, &( pstate->x ), &( pstate->y ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HeldInLeftHand( script_state_t * pstate, ai_state_t * pself )
{
    // IfHeldInLeftHand()
    /// @details ZZ@> This function passes if another character is holding the character in its
    /// left hand.
    /// Usage: Used mostly by enchants that target the item of the other hand

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    sTmp = pchr->attachedto;
    if ( ACTIVE_CHR( sTmp ) )
    {
        returncode = ( ChrList.lst[sTmp].holdingwhich[SLOT_LEFT] == pself->index );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotAnItem( script_state_t * pstate, ai_state_t * pself )
{
    // NotAnItem()
    /// @details ZZ@> This function makes the character a non-item character.
    /// Usage: Used for spells that summon creatures

    SCRIPT_FUNCTION_BEGIN();

    pchr->isitem = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // SetChildAmmo( tmpargument = "none" )
    /// @details ZZ@> This function sets the ammo of the last character spawned by this character

    SCRIPT_FUNCTION_BEGIN();

    ChrList.lst[pself->child].ammo = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitVulnerable( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitVulnerable()
    /// @details ZZ@> This function proceeds if the character was hit by a weapon of its
    /// vulnerability IDSZ.
    /// For example, a werewolf gets hit by a [SILV] bullet.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_HITVULNERABLE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsFlying( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsFlying()
    /// @details ZZ@> This function proceeds if the character target is flying

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList.lst[pself->target].flyheight > 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IdentifyTarget( script_state_t * pstate, ai_state_t * pself )
{
    // IdentifyTarget()
    /// @details ZZ@> This function reveals the target's name, ammo, and usage
    /// Proceeds if the target was unknown

    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    sTmp = pself->target;
    if ( ChrList.lst[sTmp].ammomax != 0 )  ChrList.lst[sTmp].ammoknown = btrue;
    if ( 0 == strcmp( "Blah", ChrList.lst[sTmp].Name ) )
    {
        returncode = !ChrList.lst[sTmp].nameknown;
        ChrList.lst[sTmp].nameknown = btrue;
    }

    pcap = chr_get_pcap( pself->target );
    if ( NULL != pcap )
    {
        pcap->usageknown = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BeatModule( script_state_t * pstate, ai_state_t * pself )
{
    // BeatModule()
    /// @details ZZ@> This function displays the Module Ended message

    SCRIPT_FUNCTION_BEGIN();

    PMod->beat = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EndModule( script_state_t * pstate, ai_state_t * pself )
{
    // EndModule()
    /// @details ZZ@> This function presses the Escape key

    SCRIPT_FUNCTION_BEGIN();

    // This tells the game to quit
    EProc->escape_requested = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableExport( script_state_t * pstate, ai_state_t * pself )
{
    // DisableExport()
    /// @details ZZ@> This function turns export off

    SCRIPT_FUNCTION_BEGIN();

    PMod->exportvalid = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableExport( script_state_t * pstate, ai_state_t * pself )
{
    // EnableExport()
    /// @details ZZ@> This function turns export on

    SCRIPT_FUNCTION_BEGIN();

    PMod->exportvalid = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetState( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTargetState()
    /// @details ZZ@> This function sets tmpargument to the state of the target

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = ChrList.lst[pself->target].ai.state;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Equipped( script_state_t * pstate, ai_state_t * pself )
{
    // This proceeds if the character is equipped

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->isequipped;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropTargetMoney( script_state_t * pstate, ai_state_t * pself )
{
    // DropTargetMoney( tmpargument = "amount" )
    /// @details ZZ@> This function drops some of the target's money

    SCRIPT_FUNCTION_BEGIN();

    drop_money( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetContent( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTargetContent()
    // This sets tmpargument to the current Target's content value

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = ChrList.lst[pself->target].ai.content;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropTargetKeys( script_state_t * pstate, ai_state_t * pself )
{
    // DropTargetKeys()
    /// @details ZZ@> This function makes the Target drops keys in inventory (Not inhand)

    SCRIPT_FUNCTION_BEGIN();

    drop_keys( pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinTeam( tmpargument = "team" )
    /// @details ZZ@> This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetJoinTeam( script_state_t * pstate, ai_state_t * pself )
{
    // TargetJoinTeam( tmpargument = "team" )
    /// @details ZZ@> This makes the Target join a Team specified in tmpargument (A = 0, 23 = Z, etc.)

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearMusicPassage( script_state_t * pstate, ai_state_t * pself )
{
    // ClearMusicPassage( tmpargument = "passage" )
    /// @details ZZ@> This clears the music for a specified passage

    SCRIPT_FUNCTION_BEGIN();

    PassageStack.lst[pstate->argument].music = NO_MUSIC;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearEndMessage( script_state_t * pstate, ai_state_t * pself )
{
    // ClearEndMessage()
    /// @details ZZ@> This function empties the end-module text buffer

    SCRIPT_FUNCTION_BEGIN();

    endtext[0] = CSTR_END;
    endtext_carat = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddEndMessage( script_state_t * pstate, ai_state_t * pself )
{
    // AddEndMessage( tmpargument = "message" )
    /// @details ZZ@> This function appends a message to the end-module text buffer

    SCRIPT_FUNCTION_BEGIN();

    returncode = _append_end_text( pchr,  pstate->argument, pstate );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlayMusic( script_state_t * pstate, ai_state_t * pself )
{
    // PlayMusic( tmpargument = "song number", tmpdistance = "fade time (msec)" )
    /// @details ZZ@> This function begins playing a new track of music

    SCRIPT_FUNCTION_BEGIN();

    if ( snd.musicvalid && ( songplaying != pstate->argument ) )
    {
        sound_play_song( pstate->argument, pstate->distance, -1 );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_MusicPassage( script_state_t * pstate, ai_state_t * pself )
{
    // SetMusicPassage( tmpargument = "passage", tmpturn = "type", tmpdistance = "repetitions" )

    /// @details ZZ@> This function makes the given passage play music if a player enters it
    /// tmpargument is the passage to set and tmpdistance is the music track to play.

    SCRIPT_FUNCTION_BEGIN();

    PassageStack.lst[pstate->argument].music = pstate->distance;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeCrushInvalid( script_state_t * pstate, ai_state_t * pself )
{
    // MakeCrushInvalid()
    /// @details ZZ@> This function makes doors unable to close on this object

    SCRIPT_FUNCTION_BEGIN();

    pchr->canbecrushed = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopMusic( script_state_t * pstate, ai_state_t * pself )
{
    // StopMusic()
    /// @details ZZ@> This function stops the interactive music

    SCRIPT_FUNCTION_BEGIN();

    sound_stop_song();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashVariable( script_state_t * pstate, ai_state_t * pself )
{
    // FlashVariable( tmpargument = "amount" )

    /// @details ZZ@> This function makes the character flash according to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    flash_character( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateUp( script_state_t * pstate, ai_state_t * pself )
{
    // AccelerateUp( tmpargument = "acc z" )
    /// @details ZZ@> This function makes the character accelerate up and down

    SCRIPT_FUNCTION_BEGIN();

    pchr->vel.z += pstate->argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashVariableHeight( script_state_t * pstate, ai_state_t * pself )
{
    // FlashVariableHeight( tmpturn = "intensity bottom", tmpx = "bottom", tmpdistance = "intensity top", tmpy = "top" )
    /// @details ZZ@> This function makes the character flash, feet one color, head another.

    SCRIPT_FUNCTION_BEGIN();

    flash_character_height( pself->index, CLIP_TO_16BITS( pstate->turn ), pstate->x, pstate->distance, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageTime( script_state_t * pstate, ai_state_t * pself )
{
    // SetDamageTime( tmpargument = "time" )
    /// @details ZZ@> This function makes the character invincible for a little while

    SCRIPT_FUNCTION_BEGIN();

    pchr->damagetime = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs8( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 8 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs9( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 9 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs10( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 10 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs11( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 11 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs12( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 12 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs13( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 13 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs14( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 14 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs15( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 15 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAMount( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAMount()
    /// @details ZZ@> This function passes if the Target is a mountable character

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList.lst[pself->target].ismount;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAPlatform( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAPlatform()
    /// @details ZZ@> This function passes if the Target is a platform character

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList.lst[pself->target].platform;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddStat( script_state_t * pstate, ai_state_t * pself )
{
    // AddStat()
    /// @details ZZ@> This function turns on an NPC's status display

    SCRIPT_FUNCTION_BEGIN();

    if ( !pchr->StatusList_on )
    {
        statlist_add( pself->index );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisenchantTarget( script_state_t * pstate, ai_state_t * pself )
{
    // DisenchantTarget()
    /// @details ZZ@> This function removes all enchantments on the Target character, proceeding
    /// if there were any, failing if not

    SCRIPT_FUNCTION_BEGIN();

    returncode = MAX_ENC != ChrList.lst[pself->target].firstenchant;

    disenchant_character( pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisenchantAll( script_state_t * pstate, ai_state_t * pself )
{
    // DisenchantAll()
    /// @details ZZ@> This function removes all enchantments in the game

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    for ( iTmp = 0; iTmp < MAX_ENC; iTmp++ )
    {
        remove_enchant( iTmp );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_VolumeNearestTeammate( script_state_t * pstate, ai_state_t * pself )
{
    // SetVolumeNearestTeammate( tmpargument = "sound", tmpdistance = "distance" )
    /// @details ZZ@> This function lets insects buzz correctly.  The closest Team member
    /// is used to determine the overall sound level.

    SCRIPT_FUNCTION_BEGIN();

    /*PORT
    if(moduleactive && pstate->distance >= 0)
    {
    // Find the closest Teammate
    iTmp = 10000;
    sTmp = 0;
    while(sTmp < MAX_CHR)
    {
    if(ACTIVE_CHR(sTmp) && ChrList.lst[sTmp].alive && ChrList.lst[sTmp].Team == pchr->Team)
    {
    distance = ABS(PCamera->trackx-ChrList.lst[sTmp].pos_old.x)+ABS(PCamera->tracky-ChrList.lst[sTmp].pos_old.y);
    if(distance < iTmp)  iTmp = distance;
    }
    sTmp++;
    }
    distance=iTmp+pstate->distance;
    volume = -distance;
    volume = volume<<VOLSHIFT;
    if(volume < VOLMIN) volume = VOLMIN;
    iTmp = CapList[pro_get_icap(pchr->iprofile)].wavelist[pstate->argument];
    if(iTmp < numsound && iTmp >= 0 && soundon)
    {
    lpDSBuffer[iTmp]->SetVolume(volume);
    }
    }
    */

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddShopPassage( script_state_t * pstate, ai_state_t * pself )
{
    // AddShopPassage( tmpargument = "passage" )
    /// @details ZZ@> This function makes a passage behave as a shop area, as long as the
    /// character is alive.

    SCRIPT_FUNCTION_BEGIN();

    add_shop_passage( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetPayForArmor( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx, tmpy = TargetPayForArmor( tmpargument = "skin" )

    /// @details ZZ@> This function costs the Target the appropriate amount of money for the
    /// given armor type.  Passes if the character has enough, and fails if not.
    /// Does trade-in bonus automatically.  tmpy is always set to cost of requested
    /// skin tmpx is set to amount needed after trade-in ( 0 for pass ).

    int iTmp;
    cap_t * pcap;
    chr_t * ptarget;

    SCRIPT_FUNCTION_BEGIN();

    if ( !ACTIVE_CHR( pself->target ) ) return bfalse;

    ptarget = ChrList.lst + pself->target;

    pcap = chr_get_pcap( pself->target );         // The Target's model
    if ( NULL == pcap )  return bfalse;

    iTmp = pcap->skincost[pstate->argument&3];
    pstate->y = iTmp;                             // Cost of new skin
    iTmp -= pcap->skincost[ptarget->skin];        // Refund
    if ( iTmp > ptarget->money )
    {
        // Not enough.
        pstate->x = iTmp - ptarget->money;        // Amount needed
        returncode = bfalse;
    }
    else
    {
        // Pay for it.  Cost may be negative after refund.
        ptarget->money -= iTmp;
        if ( ptarget->money > MAXMONEY )  ptarget->money = MAXMONEY;

        pstate->x = 0;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinEvilTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinEvilTeam()
    /// @details ZZ@> This function adds the character to the evil Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, TEAM_EVIL );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinNullTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinNullTeam()
    /// @details ZZ@> This function adds the character to the null Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, TEAM_NULL );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinGoodTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinGoodTeam()
    /// @details ZZ@> This function adds the character to the good Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, TEAM_GOOD );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PitsKill( script_state_t * pstate, ai_state_t * pself )
{
    // PitsKill()
    /// @details ZZ@> This function activates pit deaths for when characters fall below a
    /// certain altitude.

    SCRIPT_FUNCTION_BEGIN();

    pits.kill = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToPassageID( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToPassageID( tmpargument = "passage", tmpdistance = "idsz" )
    /// @details ZZ@> This function finds a character who is both in the passage and who has
    /// an item with the given IDSZ

    SCRIPT_FUNCTION_BEGIN();

    sTmp = who_is_blocking_passage( pstate->argument, bfalse, bfalse, bfalse, btrue, pstate->distance );
    returncode = bfalse;
    if ( ACTIVE_CHR( sTmp ) )
    {
        pself->target = sTmp;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeNameUnknown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeNameUnknown()
    /// @details ZZ@> This function makes the name of an item/character unknown.
    /// Usage: Use if you have subspawning of creatures from a book.

    SCRIPT_FUNCTION_BEGIN();

    pchr->nameknown = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactParticleEndSpawn( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnExactParticleEndSpawn( tmpargument = "particle", tmpturn = "state", tmpx = "x", tmpy = "y", tmpdistance = "z" )

    /// @details ZZ@> This function spawns a particle at a specific x, y, z position.
    /// When the particle ends, a character is spawned at its final location.
    /// The character is the same type of whatever spawned the particle.

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->index;
    if ( ACTIVE_CHR( pchr->attachedto ) )
    {
        sTmp = pchr->attachedto;
    }

    {
        fvec3_t   vtmp = VECT3( pstate->x, pstate->y, pstate->distance );
        tTmp = spawn_one_particle( vtmp, pchr->turn_z, pchr->iprofile, pstate->argument, MAX_CHR, 0, pchr->team, sTmp, TOTAL_MAX_PRT, 0, MAX_CHR );
    }

    returncode = ACTIVE_PRT( tTmp );

    if ( returncode )
    {
        PrtList.lst[sTmp].spawncharacterstate = pstate->turn;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnPoofSpeedSpacingDamage( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnPoofSpeedSpacingDamage( tmpx = "xy speed", tmpy = "xy spacing", tmpargument = "damage" )

    /// @details ZZ@> This function makes a lovely little poof at the character's location,
    /// adjusting the xy speed and spacing and the base damage first
    /// Temporarily adjust the values for the particle type

    int   tTmp, iTmp;
    float fTmp;

    cap_t * pcap;
    pip_t * ppip;

    SCRIPT_FUNCTION_BEGIN();

    pcap = pro_get_pcap( pchr->iprofile );

    if ( NULL == pcap ) return bfalse;

    ppip = pro_get_ppip( pchr->iprofile, pcap->gopoofprt_pip );

    returncode = bfalse;
    if ( NULL != ppip )
    {
        // save some values
        iTmp = ppip->xyvel_pair.base;
        tTmp = ppip->xyspacing_pair.base;
        fTmp = ppip->damage.from;

        // set some values
        ppip->xyvel_pair.base     = pstate->x;
        ppip->xyspacing_pair.base = pstate->y;
        ppip->damage.from         = FP8_TO_FLOAT( pstate->argument );

        spawn_poof( pself->index, pchr->iprofile );

        // Restore the saved values
        ppip->xyvel_pair.base     = iTmp;
        ppip->xyspacing_pair.base = tTmp;
        ppip->damage.from         = fTmp;

        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveExperienceToGoodTeam( script_state_t * pstate, ai_state_t * pself )
{
    // GiveExperienceToGoodTeam(  tmpargument = "amount", tmpdistance = "type" )
    /// @details ZZ@> This function gives experience to everyone on the G Team

    SCRIPT_FUNCTION_BEGIN();

    give_team_experience( TEAM_GOOD, pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoNothing( script_state_t * pstate, ai_state_t * pself )
{
    // DoNothing()
    /// @details ZZ@> This function does nothing
    /// Use this for debugging or in combination with a Else function

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GrogTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GrogTarget( tmpargument = "amount" )
    /// @details ZF@> This function grogs the Target for a duration equal to tmpargument

    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    pcap = chr_get_pcap( pself->target );

    returncode = bfalse;
    if ( NULL != pcap && pcap->canbegrogged && ACTIVE_CHR( pself->target ) )
    {
        ChrList.lst[pself->target].grogtime += pstate->argument;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DazeTarget( script_state_t * pstate, ai_state_t * pself )
{
    // DazeTarget( tmpargument = "amount" )
    /// @details ZF@> This function dazes the Target for a duration equal to tmpargument

    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    if ( !ACTIVE_CHR( pself->target ) ) return bfalse;

    pcap = chr_get_pcap( pself->target );

    returncode = bfalse;
    if ( NULL != pcap && pcap->canbedazed )
    {
        ChrList.lst[pself->target].dazetime += pstate->argument;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableRespawn( script_state_t * pstate, ai_state_t * pself )
{
    // EnableRespawn()
    /// @details ZF@> This function turns respawn with JUMP button on

    SCRIPT_FUNCTION_BEGIN();

    PMod->respawnvalid = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableRespawn( script_state_t * pstate, ai_state_t * pself )
{
    // DisableRespawn()
    /// @details ZF@> This function turns respawn with JUMP button off

    SCRIPT_FUNCTION_BEGIN();

    PMod->respawnvalid = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HolderBlocked( script_state_t * pstate, ai_state_t * pself )
{
    // IfHolderBlocked()
    /// @details ZZ@> This function passes if the holder blocked an attack

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( HAS_SOME_BITS( ChrList.lst[pchr->attachedto].ai.alert, ALERTIF_BLOCKED ) )
    {
        returncode = btrue;
        pself->target = ChrList.lst[pchr->attachedto].ai.attacklast;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasNotFullMana( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasNotFullMana()
    /// @details ZZ@> This function passes only if the Target is not at max mana and alive

    SCRIPT_FUNCTION_BEGIN();

    if ( !ChrList.lst[pself->target].alive || ChrList.lst[pself->target].mana > ChrList.lst[pself->target].manamax - HURTDAMAGE )
        returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableListenSkill( script_state_t * pstate, ai_state_t * pself )
{
    // EnableListenSkill()
    /// @details ZF@> This function increases range from which sound can be heard by 33%

    SCRIPT_FUNCTION_BEGIN();

    local_listening = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLastItemUsed( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToLastItemUsed()
    /// @details ZF@> This sets the Target to the last item the character used

    SCRIPT_FUNCTION_BEGIN();

    if ( pself->lastitemused == pself->index ) returncode = bfalse;
    else pself->target = pself->lastitemused;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FollowLink( script_state_t * pstate, ai_state_t * pself )
{
    // FollowLink( tmpargument = "index of next module name" )
    /// @details BB@> Skips to the next module!

    int message_number, message_index;
    char * ptext;

    SCRIPT_FUNCTION_BEGIN();

    message_number = ppro->message_start + pstate->argument;
    message_index  = MessageOffset.lst[message_number];

    ptext = message_buffer + message_index;

    returncode = link_follow_modname( ptext, btrue );
    if ( !returncode )
    {
        debug_printf( "That's too scary for %s", pchr->Name );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OperatorIsLinux( script_state_t * pstate, ai_state_t * pself )
{
    // IfOperatorIsLinux()
    /// @details ZF@> Proceeds if running on linux

    SCRIPT_FUNCTION_BEGIN();

#ifdef __unix__
    returncode = btrue;
#else
    returncode = bfalse;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAWeapon( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAWeapon()
    /// @details ZF@> Proceeds if the AI Target Is a melee or ranged weapon

    cap_t * pcap;

    SCRIPT_FUNCTION_BEGIN();

    if ( !ACTIVE_CHR( pself->target ) ) return bfalse;

    pcap = chr_get_pcap( pself->target );
    if ( NULL == pcap ) return bfalse;

    returncode = pcap->isranged || chr_has_idsz( pself->target, MAKE_IDSZ( 'X', 'W', 'E', 'P' ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SomeoneIsStealing( script_state_t * pstate, ai_state_t * pself )
{
    // IfSomeoneIsStealing()
    /// @details ZF@> This function passes if someone stealed from it's shop

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->order_value == SHOP_STOLEN && pself->order_counter == SHOP_THEFT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsASpell( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsASpell()
    /// @details ZF@> roceeds if the AI Target has any particle with the [IDAM] or [WDAM] expansion

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    for ( iTmp = 0; iTmp < MAX_PIP_PER_PROFILE; iTmp++ )
    {
        pip_t * ppip = pro_get_ppip( pchr->iprofile, iTmp );
        if ( NULL == ppip ) continue;

        if ( ppip->intdamagebonus || ppip->wisdamagebonus )
        {
            returncode = btrue;
            break;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Backstabbed( script_state_t * pstate, ai_state_t * pself )
{
    // IfBackstabbed()
    /// @details ZF@> Proceeds if HitFromBehind, target has [DISA] skill and damage dealt is physical
    /// automatically fails if character has code of conduct

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( !pchr->hascodeofconduct && HAS_SOME_BITS( pself->alert, ALERTIF_ATTACKED ) )
    {
        if ( pself->directionlast >= ATK_BEHIND - 8192 && pself->directionlast < ATK_BEHIND + 8192 )
        {
            if ( check_skills( pself->attacklast, MAKE_IDSZ( 'S', 'T', 'A', 'B' ) ) )
            {
                sTmp = pself->damagetypelast;
                if ( sTmp == DAMAGE_CRUSH || sTmp == DAMAGE_POKE || sTmp == DAMAGE_SLASH ) returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetDamageType( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTargetDamageType()
    /// @details ZF@> This function gets the last type of damage for the Target

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = ChrList.lst[pself->target].ai.damagetypelast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddQuest( script_state_t * pstate, ai_state_t * pself )
{
    // AddQuest( tmpargument = "quest idsz" )
    /// @details ZF@> This function adds a quest idsz set in tmpargument into the Targets quest.txt

    SCRIPT_FUNCTION_BEGIN();

    if ( ChrList.lst[pself->target].isplayer )
    {
        quest_add_idsz( chr_get_dir_name( pself->target ), pstate->argument );
        returncode = btrue;
    }
    else returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BeatQuestAllPlayers( script_state_t * pstate, ai_state_t * pself )
{
    // IfBeatQuestAllPlayers()
    /// @details ZF@> This function marks a IDSZ in the targets quest.txt as beaten

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    for ( iTmp = 0; iTmp < MAXPLAYER; iTmp++ )
    {
        Uint16 ichr;
        if ( !PlaList[iTmp].valid ) continue;

        ichr = PlaList[iTmp].index;
        if ( !ACTIVE_ENC( ichr ) ) continue;

        if ( ChrList.lst[ichr].isplayer )
        {
            if ( QUEST_BEATEN == quest_modify_idsz( chr_get_dir_name( ichr ), ( IDSZ )pstate->argument, 0 ) )
            {
                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasQuest( script_state_t * pstate, ai_state_t * pself )
{
    // tmpdistance = IfTargetHasQuest( tmpargument = "quest idsz )
    /// @details ZF@> This function proceeds if the Target has the unfinIshed quest specIfied in tmpargument
    /// and sets tmpdistance to the Quest Level of the specIfied quest.

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ChrList.lst[pself->target].isplayer )
    {
        iTmp = quest_check( chr_get_dir_name( pself->target ), pstate->argument );
        if ( iTmp > QUEST_BEATEN )
        {
            returncode       = btrue;
            pstate->distance = iTmp;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_QuestLevel( script_state_t * pstate, ai_state_t * pself )
{
    // SetQuestLevel( tmpargument = "idsz", distance = "adjustment" )
    /// @details ZF@> This function modIfies the quest level for a specIfic quest IDSZ
    /// tmpargument specIfies quest idsz and tmpdistance the adjustment (which may be negative)

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ChrList.lst[pself->target].isplayer && pstate->distance != 0 )
    {
        if ( quest_modify_idsz( chr_get_dir_name( pself->target ), pstate->argument, pstate->distance ) > QUEST_NONE ) returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddQuestAllPlayers( script_state_t * pstate, ai_state_t * pself )
{
    // AddQuestAllPlayers( tmpargument = "quest idsz" )
    /// @details ZF@> This function adds a quest idsz set in tmpargument into all local player's quest logs
    /// The quest level Is set to tmpdistance if the level Is not already higher or QUEST_BEATEN

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    for ( iTmp = 0; iTmp < MAXPLAYER; iTmp++ )
    {
        Uint16 ichr;

        if ( !PlaList[iTmp].valid ) continue;

        ichr = PlaList[iTmp].index;
        if ( !ACTIVE_CHR( ichr ) ) continue;

        if ( ChrList.lst[ichr].isplayer )
        {
            // Try to add it if not already there or beaten
            quest_add_idsz( chr_get_dir_name( ichr ) , pstate->argument );

            // Not beaten yet, set level to tmpdistance
            returncode = QUEST_NONE != quest_modify_idsz( chr_get_dir_name( ichr ), pstate->argument, pstate->distance );
        }

    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddBlipAllEnemies( script_state_t * pstate, ai_state_t * pself )
{
    // AddBlipAllEnemies()
    /// @details ZF@> show all enemies on the minimap who match the IDSZ given in tmpargument
    /// it show only the enemies of the AI Target

    SCRIPT_FUNCTION_BEGIN();

    if ( ACTIVE_CHR( pself->target ) )
    {
        local_senseenemiesTeam = chr_get_iteam( pself->target );
        local_senseenemiesID   = pstate->argument;
    }
    else
    {
        local_senseenemiesTeam = TEAM_MAX;
        local_senseenemiesID   = IDSZ_NONE;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PitsFall( script_state_t * pstate, ai_state_t * pself )
{
    // PitsFall( tmpx = "teleprt x", tmpy = "teleprt y", tmpdistance = "teleprt z" )
    /// @details ZF@> This function activates pit teleportation.

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->x > EDGE && pstate->y > EDGE && pstate->x < PMesh->info.edge_x - EDGE && pstate->y < PMesh->info.edge_y - EDGE )
    {
        pits.teleport = btrue;
        pits.teleport_pos.x = pstate->x;
        pits.teleport_pos.y = pstate->y;
        pits.teleport_pos.z = pstate->distance;
    }
    else
    {
        pits.kill = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOwner( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsOwner()
    /// @details ZF@> This function proceeds only if the Target is the character's owner

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList.lst[pself->target].alive && pself->owner == pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedCharacter( tmpargument = "profile", tmpx = "x", tmpy = "y", tmpdistance = "z" )

    /// @details ZF@> This function spawns a character defined in tmpargument to the characters AI target using
    /// the slot specified in tmpdistance (LEFT, RIGHT or INVENTORY). Fails if the inventory or
    /// grip specified is full or already in use.
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS,
    /// AS THE MODEL SLOTS MAY VARY FROM MODULE TO MODULE.

    fvec3_t   pos;

    SCRIPT_FUNCTION_BEGIN();

    pos.x = pstate->x;
    pos.y = pstate->y;
    pos.z = pstate->distance;

    sTmp = spawn_one_character( pos, pstate->argument, pchr->team, 0, FACE_NORTH, NULL, MAX_CHR );
    if ( !ACTIVE_CHR( sTmp ) )
    {
        if ( sTmp > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            cap_t * pcap = pro_get_pcap( pchr->iprofile );

            log_warning( "Object \"%s\"(\"%s\") failed to spawn profile index %d\n", pchr->obj_base._name, NULL == pcap ? "IVALID" : pcap->classname, pstate->argument );
        }
    }
    else
    {
        chr_t * pchild = ChrList.lst + sTmp;

        Uint8 grip = CLIP( pstate->distance, ATTACH_INVENTORY, ATTACH_RIGHT );

        if ( grip == ATTACH_INVENTORY )
        {
            // Inventory character
            if ( inventory_add_item( sTmp, pself->target ) )
            {
                pchild->ai.alert |= ALERTIF_GRABBED;  // Make spellbooks change
                pchild->attachedto = pself->target;  // Make grab work
                let_character_think( sTmp );  // Empty the grabbed messages

                pchild->attachedto = MAX_CHR;  // Fix grab

                //Set some AI values
                pself->child = sTmp;
                pchild->ai.passage = pself->passage;
                pchild->ai.owner   = pself->owner;
            }

            //No more room!
            else
            {
                chr_request_terminate( sTmp );
                sTmp = MAX_CHR;
            }
        }
        else if ( grip == ATTACH_LEFT || grip == ATTACH_RIGHT )
        {
            if ( !ACTIVE_CHR( ChrList.lst[pself->target].holdingwhich[grip] ) )
            {
                // Wielded character
                grip_offset_t grip_off = ( ATTACH_LEFT == grip ) ? GRIP_LEFT : GRIP_RIGHT;
                attach_character_to_mount( sTmp, pself->target, grip_off );

                // Handle the "grabbed" messages
                let_character_think( sTmp );

                //Set some AI values
                pself->child = sTmp;
                pchild->ai.passage = pself->passage;
                pchild->ai.owner   = pself->owner;
            }

            //Grip is already used
            else
            {
                chr_request_terminate( sTmp );
                sTmp = MAX_CHR;
            }
        }
        else
        {
            // we have been given an invalid attachment point.
            // still allow the character to spawn if it is not in an invalid area

            // technically this should never occur since we are limiting the attachment points above
            if ( !pchild->safe_valid )
            {
                chr_request_terminate( sTmp );
                sTmp = MAX_CHR;
            }
        }
    }

    returncode = ACTIVE_CHR( sTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToChild( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToChild()
    /// @details ZF@> This function sets the target to the character it spawned last (also called it's "child")

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ACTIVE_CHR( pself->child ) )
    {
        pself->target = pself->child;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageThreshold( script_state_t * pstate, ai_state_t * pself )
{
    // SetDamageThreshold()
    /// @details ZF@> This sets the damage treshold for this character. Damage below the treshold is ignored

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->argument > 0 ) pchr->damagethreshold = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_End( script_state_t * pstate, ai_state_t * pself )
{
    // End()
    /// @details ZZ@> This Is the last function in a script

    SCRIPT_FUNCTION_BEGIN();

    pself->terminate = btrue;
    returncode       = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TakePicture( script_state_t * pstate, ai_state_t * pself )
{
    // TakePicture()
    /// @details ZF@> This function proceeds only if the screenshot was successful

    SCRIPT_FUNCTION_BEGIN();

    returncode = dump_screenshot();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Speech( script_state_t * pstate, ai_state_t * pself )
{
    // SetSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets all of the RTS speech registers to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    for ( sTmp = SPEECH_BEGIN; sTmp <= SPEECH_END; sTmp++ )
    {
        pchr->soundindex[sTmp] = pstate->argument;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_MoveSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetMoveSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS move speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_MOVE] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SecondMoveSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetSecondMoveSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS movealt speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_MOVEALT] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_AttackSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetAttacksSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS attack speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_ATTACK] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_AssistSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetAssistSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS assist speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_ASSIST] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TerrainSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetTerrainSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS terrain speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_TERRAIN] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SelectSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetSelectSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS select speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_SELECT] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OperatorIsMacintosh( script_state_t * pstate, ai_state_t * pself )
{
    // IfOperatorIsMacintosh()
    /// @details ZF@> Proceeds if the current running OS is mac

    SCRIPT_FUNCTION_BEGIN();

#ifdef __APPLE__
    returncode = btrue;
#else
    returncode = bfalse;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ModuleHasIDSZ( script_state_t * pstate, ai_state_t * pself )
{
    // IfModuleHasIDSZ( tmpargument = "message number with module name", tmpdistance = "idsz" )

    /// @details ZF@> Proceeds if the specified module has the required IDSZ specified in tmpdistance
    /// The module folder name to be checked is a string from message.txt

    SCRIPT_FUNCTION_BEGIN();

    /// @todo use message.txt to send the module name
    returncode = module_has_idsz( PMod->loadname, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MorphToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // MorphToTarget()
    /// @details ZF@> This morphs the character into the target
    /// Also set size and keeps the previous AI type

    SCRIPT_FUNCTION_BEGIN();

    if ( !ACTIVE_CHR( pself->target ) ) return bfalse;

    change_character( pself->index, ChrList.lst[pself->target].basemodel, ChrList.lst[pself->target].skin, LEAVEALL );

    // let the resizing take some time
    pchr->fat_goto      = ChrList.lst[pself->target].fat;
    pchr->fat_goto_time = SIZETIME;

    // change back to our original AI
    pchr->ai.type      = ProList.lst[pchr->basemodel].iai;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveManaFlowToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveManaFlowToTarget()
    /// @details ZF@> Permanently boost the target's mana flow

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList.lst[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList.lst[pself->target].manaflow, PERFECTSTAT, &iTmp );
        ChrList.lst[pself->target].manaflow += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveManaReturnToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveManaReturnToTarget()
    /// @details ZF@> Permanently boost the target's mana return

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList.lst[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList.lst[pself->target].manareturn, PERFECTSTAT, &iTmp );
        ChrList.lst[pself->target].manareturn += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Money( script_state_t * pstate, ai_state_t * pself )
{
    // SetMoney()
    /// @details ZF@> Permanently sets the money for the character to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->argument >= 0 )
    {
        if ( pstate->argument > MAXMONEY ) pchr->money = MAXMONEY;
        else pchr->money = pstate->argument;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanSeeKurses( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetCanSeeKurses()
    /// @details ZF@> Proceeds if the target can see kursed stuff.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList.lst[pself->target].canseekurse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DispelTargetEnchantID( script_state_t * pstate, ai_state_t * pself )
{
    // DispelEnchantID( tmpargument = "idsz" )
    /// @details ZF@> This function removes all enchants from the target who match the specified RemovedByIDSZ

    SCRIPT_FUNCTION_BEGIN();

    if ( ChrList.lst[pself->target].alive )
    {
        // Check all enchants to see if they are removed

        eve_t * peve;
        Uint16 enc_now, enc_next;

        IDSZ idsz = pstate->argument;

        // clean up the enchant list before doing anything
        ChrList.lst[pself->target].firstenchant = cleanup_enchant_list( ChrList.lst[pself->target].firstenchant );

        enc_now = ChrList.lst[pself->target].firstenchant;
        while ( enc_now != MAX_ENC )
        {
            enc_next = EncList.lst[enc_now].nextenchant_ref;

            peve = enc_get_peve( enc_now );
            if ( NULL != peve && idsz == peve->removedbyidsz )
            {
                remove_enchant( enc_now );
            }

            enc_now = enc_next;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KurseTarget( script_state_t * pstate, ai_state_t * pself )
{
    // KurseTarget()
    /// @details ZF@> This makes the target kursed

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ChrList.lst[pself->target].isitem && !ChrList.lst[pself->target].iskursed )
    {
        ChrList.lst[pself->target].iskursed = btrue;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildContent( script_state_t * pstate, ai_state_t * pself )
{
    // SetChildContent( tmpargument = "content" )
    /// @details ZF@> This function lets a character set the content of the last character it
    /// spawned last

    SCRIPT_FUNCTION_BEGIN();

    ChrList.lst[pself->child].ai.content = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateTargetUp( script_state_t * pstate, ai_state_t * pself )
{
    // AccelerateTargetUp( tmpargument = "acc z" )
    /// @details ZF@> This function makes the target accelerate up and down

    SCRIPT_FUNCTION_BEGIN();

    ChrList.lst[pself->target].vel.z += pstate->argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetAmmo( tmpargument = "none" )
    /// @details ZF@> This function sets the ammo of the character's current AI target

    SCRIPT_FUNCTION_BEGIN();

    ChrList.lst[pself->target].ammo = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableInvictus( script_state_t * pstate, ai_state_t * pself )
{
    // EnableInvictus()
    /// @details ZF@> This function makes the character invulerable

    SCRIPT_FUNCTION_BEGIN();

    pchr->invictus = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableInvictus( script_state_t * pstate, ai_state_t * pself )
{
    // DisableInvictus()
    /// @details ZF@> This function makes the character not invulerable

    SCRIPT_FUNCTION_BEGIN();

    pchr->invictus = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDamageSelf( script_state_t * pstate, ai_state_t * pself )
{
    // TargetDamageSelf( tmpargument = "damage" )
    /// @details ZF@> This function applies little bit of hate from the character's target to
    /// the character itself. The amount is set in tmpargument

    IPair tmp_damage;

    SCRIPT_FUNCTION_BEGIN();

    tmp_damage.base = pstate->argument;
    tmp_damage.rand = 1;

    damage_character( pself->index, ATK_FRONT, tmp_damage, pstate->distance, chr_get_iteam( pself->target ), pself->target, DAMFX_NBLOC, btrue );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetSize( script_state_t * pstate, ai_state_t * pself )
{
    // SetSize( tmpargument = "percent" )
    /// @details ZF@> This changes the AI target's size

    SCRIPT_FUNCTION_BEGIN();

    ChrList.lst[pself->target].fat_goto *= pstate->argument / 100.0f;
    ChrList.lst[pself->target].fat_goto_time += SIZETIME;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestQuestID( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearestQuestID()
    /// @details ZF@> This function finds the NEAREST ( exact ) player who has the specified quest
    float  longdist = 0xFFFFFFFF;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    //A special version of the get_chr() function
    for ( sTmp = 0; sTmp < MAXPLAYER; sTmp++ )
    {
        Uint16 ichr_test = PlaList[sTmp].index;
        chr_t * ptst;
        int iTmp;
        fvec3_t   diff;
        float  dist2;

        if ( !ACTIVE_CHR( ichr_test ) ) continue;
        ptst = ChrList.lst + ichr_test;

        //Only valid targets
        if ( !check_target( pchr, ichr_test, TARGET_ALL, bfalse, bfalse, IDSZ_NONE, bfalse, btrue ) )
        {
            continue;
        }

        //Calculate distance
        diff  = fvec3_sub( pchr->pos.v, ptst->pos.v );
        dist2 = fvec3_dot_product( diff.v, diff.v );

        //Do they have the specified quest?
        iTmp = quest_check( chr_get_dir_name( ichr_test ), pstate->argument );
        if ( iTmp > QUEST_BEATEN && dist2 < longdist )
        {
            pself->target = ichr_test;
            returncode = btrue;
            longdist = dist2;
        }
    }


    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 _break_passage( int mesh_fx_or, int become, int frames, int starttile, int passage, int *ptilex, int *ptiley )
{
    /// @details ZZ@> This function breaks the tiles of a passage if there is a character standing
    ///               on 'em.  Turns the tiles into damage terrain if it reaches last frame.

    Uint16 endtile;
    Uint32 fan;
    int useful, character;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    // limit the start tile the the 256 tile images that we have
    starttile = CLIP_TO_08BITS( starttile );

    // same with the end tile
    endtile   =  starttile + frames - 1;
    endtile = CLIP( endtile, 0, 255 );

    useful = bfalse;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        chr_t * pchr;
        float lerp_z;

        if ( !ACTIVE_CHR( character ) ) continue;
        pchr = ChrList.lst + character;

        // nothing in packs
        if ( pchr->pack_ispacked || ACTIVE_CHR( pchr->attachedto ) ) continue;

        // nothing flying
        if ( 0 != pchr->flyheight ) continue;

        lerp_z = ( pchr->pos.z - pchr->enviro.floor_level ) / DAMAGERAISE;
        lerp_z = 1.0f - CLIP( lerp_z, 0.0f, 1.0f );

        if ( pchr->phys.weight * lerp_z <= 20 ) continue;

        fan = mesh_get_tile( PMesh, pchr->pos.x, pchr->pos.y );
        if ( VALID_TILE( PMesh, fan ) )
        {
            int img      = PMesh->mmem.tile_list[fan].img & 0x00FF;
            int highbits = PMesh->mmem.tile_list[fan].img & 0xFF00;

            if ( img >= starttile && img < endtile )
            {
                if ( object_is_in_passage( passage, pchr->pos.x, pchr->pos.y, pchr->bump.size ) )
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

            if ( PMesh->mmem.tile_list[fan].img != ( img | highbits ) )
            {
                mesh_set_texture( PMesh, fan, img | highbits );
            }
        }
    }

    return useful;
}

//--------------------------------------------------------------------------------------------
Uint8 _append_end_text( chr_t * pchr, const int message, script_state_t * pstate )
{
    /// @details ZZ@> This function appends a message to the end-module text

    int read, message_offset, ichr;

    FUNCTION_BEGIN();

    if ( !LOADED_PRO( pchr->iprofile ) ) return bfalse;

    message_offset = ProList.lst[pchr->iprofile].message_start + message;
    ichr           = GET_INDEX_PCHR( pchr );

    if ( message_offset < MessageOffset.count )
    {
        char * src, * src_end;
        char * dst, * dst_end;

        // Copy the message_offset
        read = MessageOffset.lst[message_offset];

        src     = message_buffer + read;
        src_end = message_buffer + MESSAGEBUFFERSIZE;

        dst     = endtext + endtext_carat;
        dst_end = endtext + MAXENDTEXT - 1;

        expand_escape_codes( ichr, pstate, src, src_end, dst, dst_end );

        endtext_carat = strlen( endtext );
    }

    str_add_linebreaks( endtext, strlen( endtext ), 30 );

    FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 _find_tile_in_passage( const int x0, const int y0, const int tiletype, const int passage, int *px1, int *py1 )
{
    /// @details ZZ@> This function finds the next tile in the passage, x0 and y0
    ///    must be set first, and are set on a find.  Returns btrue or bfalse
    ///    depending on if it finds one or not

    int x, y;
    Uint32 fan;
    passage_t * ppass;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;
    ppass = PassageStack.lst + passage;

    // Do the first row
    x = x0 >> TILE_BITS;
    y = y0 >> TILE_BITS;

    if ( x < ppass->area.left )  x = ppass->area.left;
    if ( y < ppass->area.top )  y = ppass->area.top;

    if ( y < ppass->area.bottom )
    {
        for ( /*nothing*/; x <= ppass->area.right; x++ )
        {
            fan = mesh_get_tile_int( PMesh, x, y );

            if ( VALID_TILE( PMesh, fan ) )
            {
                if ( CLIP_TO_08BITS( PMesh->mmem.tile_list[fan].img ) == tiletype )
                {
                    *px1 = ( x << TILE_BITS ) + 64;
                    *py1 = ( y << TILE_BITS ) + 64;
                    return btrue;
                }

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

            if ( VALID_TILE( PMesh, fan ) )
            {

                if ( CLIP_TO_08BITS( PMesh->mmem.tile_list[fan].img ) == tiletype )
                {
                    *px1 = ( x << TILE_BITS ) + 64;
                    *py1 = ( y << TILE_BITS ) + 64;
                    return btrue;
                }
            }
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 _get_chr_target( chr_t * pchr, Uint32 max_dist, TARGET_TYPE target_type, bool_t target_items, bool_t target_dead, IDSZ target_idsz, bool_t exclude_idsz, bool_t target_players )
{
    /// @details ZF@> This is the new improved AI targeting system. Also includes distance in the Z direction.
    ///     If max_dist is 0 then it searches without a max limit.

    float max_dist2;

    if ( !ACTIVE_PCHR( pchr ) ) return MAX_CHR;

    if ( TARGET_NONE == target_type ) return MAX_CHR;

    max_dist2 = max_dist * max_dist;

    return chr_find_target( pchr, max_dist2, target_type, target_items, target_dead, target_idsz, exclude_idsz, target_players );
}

//--------------------------------------------------------------------------------------------
Uint8 _display_message( int ichr, int iprofile, int message, script_state_t * pstate )
{
    /// @details ZZ@> This function sticks a message_offset in the display queue and sets its timer

    int slot, read;
    int message_offset;
    Uint8 retval;

    message_offset = ProList.lst[iprofile].message_start + message;

    retval = 0;
    if ( message_offset < MessageOffset.count )
    {
        char * src, * src_end;
        char * dst, * dst_end;

        slot = DisplayMsg_get_free();
        DisplayMsg.lst[slot].time = cfg.message_duration;

        // Copy the message_offset
        read = MessageOffset.lst[message_offset];

        src     = message_buffer + read;
        src_end = message_buffer + MESSAGEBUFFERSIZE;

        dst     = DisplayMsg.lst[slot].textdisplay;
        dst_end = DisplayMsg.lst[slot].textdisplay + MESSAGESIZE - 1;

        expand_escape_codes( ichr, pstate, src, src_end, dst, dst_end );

        *dst_end = CSTR_END;

        retval = 1;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// Uint8 scr_get_SkillLevel( script_state_t * pstate, ai_state_t * pself )
// {
//    // tmpargument = GetSkillLevel()
//    /// @details ZZ@> This function sets tmpargument to the shield profiency level of the Target
//   SCRIPT_FUNCTION_BEGIN();
//   pstate->argument = CapList[pchr->attachedto].shieldprofiency;
//   SCRIPT_FUNCTION_END();
// }
