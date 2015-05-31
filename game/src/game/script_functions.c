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

/// @file game/script_functions.c
/// @brief implementation of the internal egoscript functions
/// @details These are the c-functions that are called to implement the egoscript commands.
/// At some point, the non-trivial commands in this file will be broken out to discrete functions
/// and wrapped by some external language (like Lua) using something like SWIG, and a cross compiler written.
/// The current code is about 3/4 of the way toward this goal.
/// The functions below will then be replaced with stub calls to the "real" functions.

#include "game/script_functions.h"
#include "game/script_implementation.h"
#include "egolib/Graphics/mad.h"
#include "game/link.h"
#include "game/input.h"
#include "game/network.h"
#include "game/game.h"
#include "game/player.h"
#include "game/graphic_billboard.h"
#include "game/renderer_2d.h"
#include "game/script_implementation.h"
#include "game/char.h"
#include "game/Inventory.hpp"
#include "game/Entities/_Include.hpp"
#include "game/mesh.h"
#include "game/Core/GameEngine.hpp"
#include "game/Module/Passage.hpp"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Module/Module.hpp"
#include "game/GameStates/VictoryScreen.hpp"
#include "game/Entities/_Include.hpp"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// turn off this annoying warning
#if defined _MSC_VER
#    pragma warning(disable : 4189) // local variable is initialized but not referenced
#endif

#define SCRIPT_FUNCTION_BEGIN() \
    Object * pchr; \
    Uint8 returncode = true; \
    if( NULL == pstate || NULL == pself || !_gameObjects.exists(pself->index) ) return false; \
    pchr = _gameObjects.get( pself->index ); \
    const std::shared_ptr<ObjectProfile> &ppro = ProfileSystem::get().getProfile( pchr->profile_ref ); \
    if(!ppro) return false;

#define SCRIPT_FUNCTION_END() \
    return returncode;

#define FUNCTION_BEGIN() \
    Uint8 returncode = true; \
    if( nullptr == ( pchr ) ) return false;

#define FUNCTION_END() \
    return returncode;

#define SET_TARGET_0(ITARGET)         pself->target = ITARGET;
#define SET_TARGET_1(ITARGET,PTARGET) if( NULL != PTARGET ) { PTARGET = _gameObjects.get(ITARGET); }
#define SET_TARGET(ITARGET,PTARGET)   SET_TARGET_0( ITARGET ); SET_TARGET_1(ITARGET,PTARGET)

#define SCRIPT_REQUIRE_TARGET(PTARGET) \
    if( !_gameObjects.exists(pself->target) ) return false; \
    PTARGET = _gameObjects.get( pself->target );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// @defgroup _bitwise_functions_ Bitwise Scripting Functions
/// @details These functions may be necessary to export the bitwise functions for handling alerts to
///  scripting languages where there is no support for bitwise operators (Lua, tcl, ...)

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_set_AlertBit( script_state_t * pstate, ai_state_t * pself )
{
    /// @author BB
    /// @details Sets the bit in the 32-bit integer self.alert indexed by pstate->argument

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( pstate->argument >= 0 && pstate->argument < 32 )
    {
        SET_BIT( pself->alert, 1 << pstate->argument );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearAlertBit( script_state_t * pstate, ai_state_t * pself )
{
    /// @author BB
    /// @details Clears the bit in the 32-bit integer self.alert indexed by pstate->argument

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( pstate->argument >= 0 && pstate->argument < 32 )
    {
        UNSET_BIT( pself->alert, 1 << pstate->argument );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestAlertBit( script_state_t * pstate, ai_state_t * pself )
{
    /// @author BB
    /// @details Tests to see if the the bit in the 32-bit integer self.alert indexed by pstate->argument is non-zero

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
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
    /// @author BB
    /// @details Sets one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    SET_BIT( pself->alert, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearAlert( script_state_t * pstate, ai_state_t * pself )
{
    /// @author BB
    /// @details Clears one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    UNSET_BIT( pself->alert, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestAlert( script_state_t * pstate, ai_state_t * pself )
{
    /// @author BB
    /// @details Tests one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_set_Bit( script_state_t * pstate, ai_state_t * pself )
{
    /// @author BB
    /// @details Sets the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( pstate->y >= 0 && pstate->y < 32 )
    {
        SET_BIT( pstate->x, 1 << pstate->y );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearBit( script_state_t * pstate, ai_state_t * pself )
{
    /// @author BB
    /// @details Clears the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( pstate->y >= 0 && pstate->y < 32 )
    {
        UNSET_BIT( pstate->x, 1 << pstate->y );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestBit( script_state_t * pstate, ai_state_t * pself )
{
    /// @author BB
    /// @details Tests the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
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
    /// @author BB
    /// @details Adds the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    SET_BIT( pstate->x, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearBits( script_state_t * pstate, ai_state_t * pself )
{
    /// @author BB
    /// @details Clears the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    UNSET_BIT( pstate->x, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestBits( script_state_t * pstate, ai_state_t * pself )
{
    /// @author BB
    /// @details Tests the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pstate->x, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 scr_Spawned( script_state_t * pstate, ai_state_t * pself )
{
    // IfSpawned()
    /// @author ZZ
    /// @details This function proceeds if the character was spawned this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if it's a new character
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_SPAWNED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TimeOut( script_state_t * pstate, ai_state_t * pself )
{
    // IfTimeOut()
    /// @author ZZ
    /// @details This function proceeds if the character's aitime is 0.  Use
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
    /// @author ZZ
    /// @details This function proceeds if the character reached its waypoint this
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
    /// @author ZZ
    /// @details This function proceeds if the character reached its last waypoint this
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
    /// @author ZZ
    /// @details This function proceeds if the character ( an item ) was put in its
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
    /// @author ZZ
    /// @details This function proceeds if the character was bumped by another character
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
    /// @author ZZ
    /// @details This function proceeds if the character got an order from another
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
    /// @author ZZ
    /// @details This function proceeds if one of the character's teammates was nearly
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
    /// @author ZZ
    /// @details This function sets the content variable.  Used in conjunction with
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
    /// @author ZZ
    /// @details This function proceeds if the character was killed this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character's been killed
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_KILLED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetKilled( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetKilled()
    /// @author ZZ
    /// @details This function proceeds if the character's target from last update was
    /// killed during this update

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    // Proceed only if the character's target has just died or is already dead
    returncode = ( HAS_SOME_BITS( pself->alert, ALERTIF_TARGETKILLED ) || !pself_target->alive );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearWaypoints( script_state_t * pstate, ai_state_t * pself )
{
    // ClearWaypoints()
    /// @author ZZ
    /// @details This function is used to move a character around.  Do this before
    /// AddWaypoint

    SCRIPT_FUNCTION_BEGIN();

    returncode = waypoint_list_clear( &( pself->wp_lst ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddWaypoint( script_state_t * pstate, ai_state_t * pself )
{
    // AddWaypoint( tmpx = "x position", tmpy = "y position" )
    /// @author ZZ
    /// @details This function tells the character where to move next

    SCRIPT_FUNCTION_BEGIN();

    returncode = AddWaypoint( &( pself->wp_lst ), pself->index, pstate->x, pstate->y );

    if ( returncode )
    {
        // make sure we update the waypoint, since the list changed
        ai_state_get_wp( pself );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FindPath( script_state_t * pstate, ai_state_t * pself )
{
    // FindPath
    /// @author ZF
    /// @details Ported the A* path finding algorithm by birdsey and heavily modified it
    /// This function adds enough waypoints to get from one point to another

    bool used_astar;

    SCRIPT_FUNCTION_BEGIN();

    //Too soon since last try?
    if ( pself->astar_timer > update_wld ) return false;

    returncode = FindPath( &( pself->wp_lst ), pchr, pstate->x, pstate->y, &used_astar );

    if ( used_astar )
    {
        // limit the rate of AStar calculations to be once every half second.
        pself->astar_timer = update_wld + ( ONESECOND / 2 );
    }

    //Make sure the waypoint list is updated
    ai_state_get_wp( pself );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Compass( script_state_t * pstate, ai_state_t * pself )
{
    // Compass( tmpturn = "rotation", tmpdistance = "radius" )
    /// @author ZZ
    /// @details This function modifies tmpx and tmpy, depending on the setting of
    /// tmpdistance and tmpturn.  It acts like one of those Compass thing
    /// with the two little needle legs

    fvec2_t loc_pos;

    SCRIPT_FUNCTION_BEGIN();

    loc_pos[XX] = pstate->x;
    loc_pos[YY] = pstate->y;

    returncode = Compass( loc_pos, pstate->turn, pstate->distance );

    // update the position
    if ( returncode )
    {
        pstate->x = loc_pos[XX];
        pstate->y = loc_pos[YY];
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetArmorPrice( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx = GetTargetArmorPrice( tmpargument = "skin" )
    /// @author ZZ
    /// @details This function returns the cost of the desired skin upgrade, setting
    /// tmpx to the price

    int value;
    Object *ptarget;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( ptarget );

    value = GetArmorPrice( ptarget, pstate->argument );

    if ( value > 0 )
    {
        pstate->x  = value;
        returncode = true;
    }
    else
    {
        pstate->x  = 0;
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Time( script_state_t * pstate, ai_state_t * pself )
{
    // SetTime( tmpargument = "time" )
    /// @author ZZ
    /// @details This function sets the character's ai timer.  50 clicks per second.
    /// Used in conjunction with IfTimeOut

    SCRIPT_FUNCTION_BEGIN();

    pself->timer = UpdateTime( pself->timer, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_Content( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetContent()
    /// @author ZZ
    /// @details This function sets tmpargument to the character's content variable.
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
    /// @author ZZ
    /// @details This function lets a character join a different team.  Used
    /// mostly for pets

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = false;
    if ( _gameObjects.exists( pself->target ) )
    {
        switch_team( pself->index, pself_target->team );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearbyEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearbyEnemy()
    /// @author ZZ
    /// @details This function sets the target to a nearby enemy, failing if there are none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, NEARBY, IDSZ_NONE, TARGET_ENEMIES );

    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetLeftHand( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToTargetLeftHand()
    /// @author ZZ
    /// @details This function sets the target to the item in the target's left hand,
    /// failing if the target has no left hand item

    CHR_REF ichr;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ichr = pself_target->holdingwhich[SLOT_LEFT];
    returncode = false;
    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET( ichr, pself_target );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetRightHand( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToTargetRightHand()
    /// @author ZZ
    /// @details This function sets the target to the item in the target's right hand,
    /// failing if the target has no right hand item

    CHR_REF ichr;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ichr = pself_target->holdingwhich[SLOT_RIGHT];
    returncode = false;
    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET( ichr, pself_target );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverAttacked( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverAttacked()
    /// @author ZZ
    /// @details This function sets the target to whoever attacked the character last, failing for damage tiles

    SCRIPT_FUNCTION_BEGIN();

    if ( _gameObjects.exists( pself->attacklast ) )
    {
        SET_TARGET_0( pself->attacklast );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverBumped( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverBumped()
    /// @author ZZ
    /// @details This function sets the target to whoever bumped the character last. It never fails

    SCRIPT_FUNCTION_BEGIN();

    if ( _gameObjects.exists( pself->bumplast ) )
    {
        SET_TARGET_0( pself->bumplast );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverCalledForHelp( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverCalledForHelp()
    /// @author ZZ
    /// @details This function sets the target to whoever called for help last.

    SCRIPT_FUNCTION_BEGIN();

    if ( VALID_TEAM_RANGE( pchr->team ) )
    {
        CHR_REF isissy = TeamStack[pchr->team].sissy;

        if ( _gameObjects.exists( isissy ) )
        {
            SET_TARGET_0( isissy );
        }
        else
        {
            returncode = false;
        }
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToOldTarget( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToOldTarget()
    /// @author ZZ
    /// @details This function sets the target to the target from last update, used to
    /// undo other set_Target functions

    SCRIPT_FUNCTION_BEGIN();

    if ( _gameObjects.exists( pself->target_old ) )
    {
        SET_TARGET_0( pself->target_old );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToVelocity( script_state_t * pstate, ai_state_t * pself )
{
    // SetTurnModeToVelocity()
    /// @author ZZ
    /// @details This function sets the character's movement mode to the default

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_VELOCITY;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToWatch( script_state_t * pstate, ai_state_t * pself )
{
    // SetTurnModeToWatch()
    /// @author ZZ
    /// @details This function makes the character look at its next waypoint, usually
    /// used with close waypoints or the Stop function

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_WATCH;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToSpin( script_state_t * pstate, ai_state_t * pself )
{
    // SetTurnModeToSpin()
    /// @author ZZ
    /// @details This function makes the character spin around in a circle, usually
    /// used for magical items and such

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_SPIN;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BumpHeight( script_state_t * pstate, ai_state_t * pself )
{
    // SetBumpHeight( tmpargument = "height" )
    /// @author ZZ
    /// @details This function makes the character taller or shorter, usually used when
    /// the character dies

    SCRIPT_FUNCTION_BEGIN();

    pchr->setBumpHeight(pstate->argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target has either a parent or type IDSZ
    /// matching tmpargument.

    SCRIPT_FUNCTION_BEGIN();

    returncode = chr_is_type_idsz( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasItemID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasItemID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target has a matching item in his/her
    /// pockets or hands.

    CHR_REF item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_has_item_idsz( pself->target, ( IDSZ ) pstate->argument, false );

    returncode = _gameObjects.exists( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHoldingItemID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHoldingItemID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target has a matching item in his/her
    /// hands.  It also sets tmpargument to the proper latch button to press
    /// to use that item

    CHR_REF item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_holding_idsz( pself->target, pstate->argument );

    returncode = _gameObjects.exists( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasSkillID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasSkillID( tmpargument = "skill idsz" )
    /// @author ZZ
    /// @details This function proceeds if ID matches tmpargument

    Object *pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( 0 != chr_get_skill( pself_target, ( IDSZ )pstate->argument ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Else( script_state_t * pstate, ai_state_t * pself )
{
    // Else
    /// @author ZZ
    /// @details This function fails if the last function was more indented

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ppro->getAIScript().indent >= ppro->getAIScript().indent_last );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Run( script_state_t * pstate, ai_state_t * pself )
{
    // Run()
    /// @author ZZ
    /// @details This function sets the character's maximum acceleration to its
    /// actual maximum

    SCRIPT_FUNCTION_BEGIN();

    pchr->resetAcceleration();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Walk( script_state_t * pstate, ai_state_t * pself )
{
    // Walk()
    /// @author ZZ
    /// @details This function sets the character's maximum acceleration to 66%
    /// of its actual maximum

    SCRIPT_FUNCTION_BEGIN();

    pchr->resetAcceleration();

    pchr->maxaccel      = pchr->maxaccel_reset * 0.66f;
    pchr->movement_bits = CHR_MOVEMENT_BITS_WALK;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Sneak( script_state_t * pstate, ai_state_t * pself )
{
    // Sneak()
    /// @author ZZ
    /// @details This function sets the character's maximum acceleration to 33%
    /// of its actual maximum

    SCRIPT_FUNCTION_BEGIN();

    pchr->resetAcceleration();

    pchr->maxaccel      = pchr->maxaccel_reset * 0.33f;
    pchr->movement_bits = CHR_MOVEMENT_BITS_SNEAK | CHR_MOVEMENT_BITS_STOP;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoAction( script_state_t * pstate, ai_state_t * pself )
{
    // DoAction( tmpargument = "action" )
    /// @author ZZ
    /// @details This function makes the character do a given action if it isn't doing
    /// anything better.  Fails if the action is invalid or if the character is doing
    /// something else already

    int action;

    SCRIPT_FUNCTION_BEGIN();

    action = mad_get_action_ref( pchr->inst.imad, pstate->argument );

    returncode = false;
    if ( rv_success == chr_start_anim( pchr, action, false, false ) )
    {
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KeepAction( script_state_t * pstate, ai_state_t * pself )
{
    // KeepAction()
    /// @author ZZ
    /// @details This function makes the character's animation stop on its last frame
    /// and stay there.  Usually used for dropped items

    SCRIPT_FUNCTION_BEGIN();

    chr_instance_set_action_keep( &( pchr->inst ), true );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IssueOrder( script_state_t * pstate, ai_state_t * pself )
{
    // IssueOrder( tmpargument = "order"  )
    /// @author ZZ
    /// @details This function tells all of the character's teammates to do something,
    /// though each teammate needs to interpret the order using IfOrdered in
    /// its own script.

    SCRIPT_FUNCTION_BEGIN();

    issue_order( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropWeapons( script_state_t * pstate, ai_state_t * pself )
{
    // DropWeapons()
    /// @author ZZ
    /// @details This function drops the character's in-hand items.  It will also
    /// buck the rider if the character is a mount

    SCRIPT_FUNCTION_BEGIN();

    // This funtion drops the character's in hand items/riders
    const std::shared_ptr<Object> &leftItem = pchr->getLeftHandItem();
    if (leftItem)
    {
        leftItem->detatchFromHolder(true, true);
        if ( pchr->isMount() )
        {
            leftItem->vel[kZ]    = DISMOUNTZVEL;
            leftItem->jump_timer = JUMPDELAY;
            leftItem->movePosition(0.0f, 0.0f, DISMOUNTZVEL);
        }
    }

    const std::shared_ptr<Object> &rightItem = pchr->getLeftHandItem();
    if (rightItem)
    {
        rightItem->detatchFromHolder(true, true);
        if ( pchr->isMount() )
        {
            rightItem->vel[kZ]    = DISMOUNTZVEL;
            rightItem->jump_timer = JUMPDELAY;
            rightItem->movePosition(0.0f, 0.0f, DISMOUNTZVEL);
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDoAction( script_state_t * pstate, ai_state_t * pself )
{
    // TargetDoAction( tmpargument = "action" )
    /// @author ZZ
    /// @details The function makes the target start a new action, if it is valid for the model
    /// It will fail if the action is invalid or if the target is doing
    /// something else already

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( _gameObjects.exists( pself->target ) )
    {
        Object * pself_target = _gameObjects.get( pself->target );

        if ( pself_target->alive )
        {
            int action = mad_get_action_ref( pself_target->inst.imad, pstate->argument );

            if ( rv_success == chr_start_anim( pself_target, action, false, false ) )
            {
                returncode = true;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OpenPassage( script_state_t * pstate, ai_state_t * pself )
{
    // OpenPassage( tmpargument = "passage" )

    /// @author ZZ
    /// @details This function opens the passage specified by tmpargument, failing if the
    /// passage was already open.
    /// Passage areas are defined in passage.txt and set in spawn.txt for the given character

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = PMod->getPassageByID(pstate->argument);
    
    returncode = false;
    if(passage) {
        returncode = true;
        passage->open();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClosePassage( script_state_t * pstate, ai_state_t * pself )
{
    // ClosePassage( tmpargument = "passage" )
    /// @author ZZ
    /// @details This function closes the passage specified by tmpargument, proceeding
    /// if the passage isn't blocked.  Crushable characters within the passage
    /// are crushed.

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = PMod->getPassageByID(pstate->argument);

    returncode = false;
    if(passage) {
        returncode = passage->close();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PassageOpen( script_state_t * pstate, ai_state_t * pself )
{
    // IfPassageOpen( tmpargument = "passage" )
    /// @author ZZ
    /// @details This function proceeds if the given passage is valid and open to movement
    /// Used mostly by door characters to tell them when to run their open animation.

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = PMod->getPassageByID(pstate->argument);

    returncode = false;
    if(passage) {
        returncode = passage->isOpen();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GoPoof( script_state_t * pstate, ai_state_t * pself )
{
    // GoPoof()
    /// @author ZZ
    /// @details This function flags the character to be removed from the game entirely.
    /// This doesn't work on players

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( !VALID_PLA( pchr->is_which_player ) )
    {
        returncode = true;
        pself->poof_time = update_wld;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostTargetItemID( script_state_t * pstate, ai_state_t * pself )
{
    // CostTargetItemID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target has a matching item, and poofs
    /// that item.
    /// For one use keys and such

    CHR_REF item;
    Object *pitem, *ptarget;
    size_t cnt = INVALID_CHR_REF;
    IDSZ idsz;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( ptarget );

    //first check both hands
    idsz = ( IDSZ ) pstate->argument;
    item = chr_holding_idsz( pself->target, idsz );

    //need to search inventory as well?
    if ( !_gameObjects.exists( item ) )
    {
        for ( cnt = 0; cnt < MAXINVENTORY; cnt++ )
        {
            item = ptarget->inventory[cnt];

            //only valid items
            if ( !_gameObjects.exists( item ) ) continue;
            pitem = _gameObjects.get( item );

            //matching idsz?
            if ( chr_is_type_idsz( item, idsz ) ) break;
        }

        //did we fail?
        if ( cnt == MAXINVENTORY ) item = INVALID_CHR_REF;
    }

    returncode = false;
    if ( _gameObjects.exists( item ) )
    {
        pitem = _gameObjects.get( item );
        returncode = true;

        // Cost one ammo
        if ( pitem->ammo > 1 )
        {
            pitem->ammo--;
        }

        // Poof the item
        else
        {
            if ( _gameObjects.exists( pitem->inwhich_inventory ) && cnt < MAXINVENTORY )
            {
                // Remove from the pack
                Inventory::remove_item( pchr->ai.index, cnt, true );
            }
            else
            {
                // Drop from hand
                pitem->detatchFromHolder(true, false);
            }

            // get rid of the character, no matter what
            _gameObjects.remove( item );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoActionOverride( script_state_t * pstate, ai_state_t * pself )
{
    // DoActionOverride( tmpargument = "action" )
    /// @author ZZ
    /// @details This function makes the character do a given action no matter what
    /// It will fail if the action is invalid

    int action;

    SCRIPT_FUNCTION_BEGIN();

    action = mad_get_action_ref( pchr->inst.imad, pstate->argument );

    returncode = false;
    if ( rv_success == chr_start_anim( pchr, action, false, true ) )
    {
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Healed( script_state_t * pstate, ai_state_t * pself )
{
    // IfHealed()
    /// @author ZZ
    /// @details This function proceeds if the character was healed by a healing particle

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was healed
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_HEALED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SendPlayerMessage( script_state_t * pstate, ai_state_t * pself )
{
    // SendPlayerMessage( tmpargument = "message number" )
    /// @author ZZ
    /// @details This function sends a message to the players

    SCRIPT_FUNCTION_BEGIN();

    returncode = _display_message( pself->index, pchr->profile_ref, pstate->argument, pstate );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CallForHelp( script_state_t * pstate, ai_state_t * pself )
{
    // CallForHelp()
    /// @author ZZ
    /// @details This function calls all of the character's teammates for help.  The
    /// teammates must use IfCalledForHelp in their scripts

    SCRIPT_FUNCTION_BEGIN();

    pchr->callForHelp();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddIDSZ( script_state_t * pstate, ai_state_t * pself )
{
    // AddIDSZ( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function slaps an expansion IDSZ onto the menu.txt file.
    /// Used to show completion of special quests for a given module

    SCRIPT_FUNCTION_BEGIN();

    if ( ModuleProfile::moduleAddIDSZ(PMod->getPath().c_str(), pstate->argument, 0, NULL) )
    {
        // invalidate any module list so that we will reload them
        //module_list_valid = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_State( script_state_t * pstate, ai_state_t * pself )
{
    // SetState( tmpargument = "state" )
    /// @author ZZ
    /// @details This function sets the character's state.
    /// VERY IMPORTANT. State is preserved from update to update

    SCRIPT_FUNCTION_BEGIN();

    // set the state - this function updates the is_hidden
    chr_set_ai_state( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_State( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetState()
    /// @author ZZ
    /// @details This function reads the character's state variable

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = pself->state;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs( script_state_t * pstate, ai_state_t * pself )
{
    // IfStateIs( tmpargument = "state" )
    /// @author ZZ
    /// @details This function proceeds if the character's state equals tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->argument == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanOpenStuff( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetCanOpenStuff()
    /// @author ZZ
    /// @details This function proceeds if the target can open stuff ( set in data.txt )
    /// Used by chests and buttons and such so only "smart" creatures can operate
    /// them

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();
    returncode = false;

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->isMount() )
    {
        CHR_REF iheld = pself_target->holdingwhich[SLOT_LEFT];

        if ( _gameObjects.exists( iheld ) )
        {
            // can the rider open the
            returncode = _gameObjects.get(iheld)->openstuff;
        }
    }

    if ( !returncode )
    {
        // if a rider can't openstuff, can the target openstuff?
        returncode = pself_target->openstuff;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Grabbed( script_state_t * pstate, ai_state_t * pself )
{
    // IfGrabbed()
    /// @author ZZ
    /// @details This function proceeds if the character was grabbed (picked up) this update.
    /// Used mostly by item characters

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_GRABBED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Dropped( script_state_t * pstate, ai_state_t * pself )
{
    // IfDropped()
    /// @author ZZ
    /// @details This function proceeds if the character was dropped this update.
    /// Used mostly by item characters

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_DROPPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverIsHolding( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverIsHolding()
    /// @author ZZ
    /// @details This function sets the target to the character's holder or mount,
    /// failing if the character has no mount or holder

    SCRIPT_FUNCTION_BEGIN();

    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        SET_TARGET_0( pchr->attachedto );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DamageTarget( script_state_t * pstate, ai_state_t * pself )
{
    // DamageTarget( tmpargument = "damage" )
    /// @author ZZ
    /// @details This function applies little bit of love to the character's target.
    /// The amount is set in tmpargument

    IPair tmp_damage;

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _gameObjects[pself->target];
    if(!target) {
        return false;
    }

    tmp_damage.base = pstate->argument;
    tmp_damage.rand = 1;

    target->damage(ATK_FRONT, tmp_damage, static_cast<DamageType>(pchr->damagetarget_damagetype), 
        pchr->team, _gameObjects[pself->index], DAMFX_NBLOC, true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_XIsLessThanY( script_state_t * pstate, ai_state_t * pself )
{
    // IfXIsLessThanY( tmpx, tmpy )
    /// @author ZZ
    /// @details This function proceeds if tmpx is less than tmpy.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->x < pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_WeatherTime( script_state_t * pstate, ai_state_t * pself )
{
    // SetWeatherTime( tmpargument = "time" )
    /// @author ZZ
    /// @details This function can be used to slow down or speed up or stop rain and
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
    /// @author ZZ
    /// @details This function sets tmpargument to the character's height

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = pchr->bump.height;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Reaffirmed( script_state_t * pstate, ai_state_t * pself )
{
    // IfReaffirmed()
    /// @author ZZ
    /// @details This function proceeds if the character was damaged by its reaffirm
    /// damage type.  Used to relight the torch.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_REAFFIRMED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkeepAction( script_state_t * pstate, ai_state_t * pself )
{
    // UnkeepAction()
    /// @author ZZ
    /// @details This function is the opposite of KeepAction. It makes the current animation resume.

    SCRIPT_FUNCTION_BEGIN();

    chr_instance_set_action_keep( &( pchr->inst ), false );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnOtherTeam( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsOnOtherTeam()
    /// @author ZZ
    /// @details This function proceeds if the target is on another team

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->alive && chr_get_iteam( pself->target ) != pchr->team );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnHatedTeam( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsOnHatedTeam()
    /// @author ZZ
    /// @details This function proceeds if the target is on an enemy team

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->alive && team_hates_team( pchr->team, chr_get_iteam( pself->target ) ) && !pself_target->invictus );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PressLatchButton( script_state_t * pstate, ai_state_t * pself )
{
    // PressLatchButton( tmpargument = "latch bits" )
    /// @author ZZ
    /// @details This function sets the character latch buttons

    SCRIPT_FUNCTION_BEGIN();

    if(pstate->argument >= LATCHBUTTON_LEFT && pstate->argument < LATCHBUTTON_RESPAWN)
    {
        pchr->latch.b[pstate->argument] = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetOfLeader( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToTargetOfLeader()
    /// @author ZZ
    /// @details This function sets the character's target to the target of its leader,
    /// or it fails with no change if the leader is dead

    SCRIPT_FUNCTION_BEGIN();

    if ( VALID_TEAM_RANGE( pchr->team ) )
    {
        const std::shared_ptr<Object> &leader = TeamStack[pchr->team].getLeader();

        if ( leader )
        {
            CHR_REF itarget = leader->ai.target;

            if ( _gameObjects.exists( itarget ) )
            {
                SET_TARGET_0( itarget );
            }
            else
            {
                returncode = false;
            }
        }
        else
        {
            returncode = false;
        }
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_LeaderKilled( script_state_t * pstate, ai_state_t * pself )
{
    // IfLeaderKilled()
    /// @author ZZ
    /// @details This function proceeds if the team's leader died this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_LEADERKILLED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeLeader( script_state_t * pstate, ai_state_t * pself )
{
    // BecomeLeader()
    /// @author ZZ
    /// @details This function makes the character the leader of the team

    SCRIPT_FUNCTION_BEGIN();

    TeamStack[pchr->team].setLeader(_gameObjects[pself->index]);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTargetArmor( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeTargetArmor( tmpargument = "armor" )

    /// @author ZZ
    /// @details This function sets the target's armor type and returns the old type
    /// as tmpargument and the new type as tmpx

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    iTmp = pself_target->skin;
    pstate->x = change_armor( pself->target, pstate->argument );

    pstate->argument = iTmp;  // The character's old armor

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveMoneyToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveMoneyToTarget( tmpargument = "money" )
    /// @author ZZ
    /// @details This function increases the target's money, while decreasing the
    /// character's own money.  tmpargument is set to the amount transferred
    /// @note BB@> I would like to use getadd_int() here, but it is not really suited to two variables

    int tTmp, iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    //squash out-or-range values
    pchr->money = CLIP( pchr->money, (Sint16)0, (Sint16)MAXMONEY );
    pself_target->money = CLIP( pself_target->money, (Sint16)0, (Sint16)MAXMONEY );

    // limit the range of the character's money
    iTmp = pchr->money - pstate->argument;
    iTmp = CLIP( iTmp, 0, MAXMONEY );

    // limit the range of the target's money
    tTmp = pself_target->money + pstate->argument;
    tTmp = CLIP( tTmp, 0, MAXMONEY );

    // recover the possible transfer values
    iTmp = iTmp + pstate->argument;
    tTmp = tTmp - pstate->argument;

    // limit the transfer values
    if ( pstate->argument < 0 )
    {
        pstate->argument = std::max( iTmp, tTmp );
    }
    else
    {
        pstate->argument = std::min( iTmp, tTmp );
    }

    pchr->money         = pchr->money + pstate->argument;
    pself_target->money = pself_target->money + pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropKeys( script_state_t * pstate, ai_state_t * pself )
{
    // DropKeys()
    /// @author ZZ
    /// @details This function drops all of the keys in the character's inventory.
    /// This does NOT drop keys in the character's hands.

    SCRIPT_FUNCTION_BEGIN();

    drop_keys( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_LeaderIsAlive( script_state_t * pstate, ai_state_t * pself )
{
    // IfLeaderIsAlive()
    /// @author ZZ
    /// @details This function proceeds if the team has a leader

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( TeamStack[pchr->team].getLeader() != nullptr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOldTarget( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsOldTarget()
    /// @author ZZ
    /// @details This function proceeds if the target is the same as it was last update

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->target == pself->target_old );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLeader( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToLeader()
    /// @author ZZ
    /// @details This function sets the target to the leader, proceeding if their is
    /// a valid leader for the character's team

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( VALID_TEAM_RANGE( pchr->team ) )
    {
        const std::shared_ptr<Object> &leader = TeamStack[pchr->team].getLeader();
        if ( leader )
        {
            SET_TARGET_0( leader->getCharacterID() );
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnCharacter( tmpx = "x", tmpy = "y", tmpturn = "turn", tmpdistance = "speed" )

    /// @author ZZ
    /// @details This function spawns a character of the same type as the spawner.
    /// This function spawns a character, failing if x,y is invalid
    /// This is horribly complicated to use, so see ANIMATE.OBJ for an example
    /// tmpx and tmpy give the coodinates, tmpturn gives the new character's
    /// direction, and tmpdistance gives the new character's initial velocity

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    fvec3_t pos = fvec3_t(pstate->x, pstate->y, 0);

    ichr = spawn_one_character(pos, pchr->profile_ref, pchr->team, 0, CLIP_TO_16BITS( pstate->turn ), NULL, INVALID_CHR_REF);
    returncode = _gameObjects.exists( ichr );

    if ( !returncode )
    {
        if ( ichr > PMod->getImportAmount() * MAX_IMPORT_PER_PLAYER )
        {
            log_warning( "Object %s failed to spawn a copy of itself\n", pchr->Name );
        }
    }
    else
    {
        Object * pchild = _gameObjects.get( ichr );

        // was the child spawned in a "safe" spot?
        if (!chr_get_safe( pchild))
        {
            _gameObjects.remove( ichr );
            ichr = INVALID_CHR_REF;
        }
        else
        {
            TURN_T turn;
            pself->child = ichr;

            turn = TO_TURN( pchr->ori.facing_z + ATK_BEHIND );
            pchild->vel[kX] += turntocos[ turn ] * pstate->distance;
            pchild->vel[kY] += turntosin[ turn ] * pstate->distance;

            pchild->iskursed = pchr->iskursed;  /// @note BB@> inherit this from your spawner
            pchild->ai.passage = pself->passage;
            pchild->ai.owner   = pself->owner;

            pchild->dismount_timer  = PHYS_DISMOUNT_TIME;
            pchild->dismount_object = pself->index;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RespawnCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // RespawnCharacter()
    /// @author ZZ
    /// @details This function respawns the character at its starting location.
    /// Often used with the Clean functions

    SCRIPT_FUNCTION_BEGIN();

    respawn_character( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTile( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeTile( tmpargument = "tile type")
    /// @author ZZ
    /// @details This function changes the tile under the character to the new tile type,
    /// which is highly module dependent

    SCRIPT_FUNCTION_BEGIN();

    returncode = ego_mesh_set_texture( PMesh, pchr->getTile(), pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Used( script_state_t * pstate, ai_state_t * pself )
{
    // IfUsed()
    /// @author ZZ
    /// @details This function proceeds if the character was used by its holder or rider.
    /// Character's cannot be used if their reload time is greater than 0

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_USED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropMoney( script_state_t * pstate, ai_state_t * pself )
{
    // DropMoney( tmpargument = "money" )
    /// @author ZZ
    /// @details This function drops a certain amount of money, if the character has that
    /// much

    SCRIPT_FUNCTION_BEGIN();

    drop_money( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_OldTarget( script_state_t * pstate, ai_state_t * pself )
{
    // SetOldTarget()
    /// @author ZZ
    /// @details This function sets the old target to the current target.  To allow
    /// greater manipulations of the target

    SCRIPT_FUNCTION_BEGIN();

    pself->target_old = pself->target;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DetachFromHolder( script_state_t * pstate, ai_state_t * pself )
{
    // DetachFromHolder()
    /// @author ZZ
    /// @details This function drops the character or makes it get off its mount
    /// Can be used to make slippery weapons, or to make certain characters
    /// incapable of wielding certain weapons. "A troll can't grab a torch"

    SCRIPT_FUNCTION_BEGIN();

    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        pchr->detatchFromHolder(true, true);
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasVulnerabilityID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasVulnerabilityID( tmpargument = "vulnerability idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target is vulnerable to the given IDSZ.
    
    Object *pself_target;
    
    SCRIPT_FUNCTION_BEGIN();
    
    SCRIPT_REQUIRE_TARGET(pself_target);
    
    returncode = pself_target->getProfile()->getIDSZ(IDSZ_VULNERABILITY) == static_cast<IDSZ>(pstate->argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CleanUp( script_state_t * pstate, ai_state_t * pself )
{
    // CleanUp()
    /// @author ZZ
    /// @details This function tells all the dead characters on the team to clean
    /// themselves up.  Usually done by the boss creature every second or so

    SCRIPT_FUNCTION_BEGIN();

    issue_clean( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CleanedUp( script_state_t * pstate, ai_state_t * pself )
{
    // IfCleanedUp()
    /// @author ZZ
    /// @details This function proceeds if the character is dead and if the boss told it
    /// to clean itself up

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CLEANEDUP );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Sitting( script_state_t * pstate, ai_state_t * pself )
{
    // IfSitting()
    /// @author ZZ
    /// @details This function proceeds if the character is riding a mount

    SCRIPT_FUNCTION_BEGIN();

    returncode = _gameObjects.exists( pchr->attachedto );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsHurt( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsHurt()
    /// @author ZZ
    /// @details This function passes only if the target is hurt and alive

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !pself_target->alive || pself_target->life > pself_target->life_max - HURTDAMAGE )
        returncode = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAPlayer( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAPlayer()
    /// @author ZZ
    /// @details This function proceeds if the target is controlled by a human ( may not be local )

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = VALID_PLA( pself_target->is_which_player );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySound( script_state_t * pstate, ai_state_t * pself )
{
    // PlaySound( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function plays one of the character's sounds.
    /// The sound fades out depending on its distance from the viewer

    SCRIPT_FUNCTION_BEGIN();

    if ( pchr->pos_old[kZ] > PITNOSOUND )
    {
        AudioSystem::get().playSound(pchr->pos_old, ppro->getSoundID(pstate->argument));
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnParticle(tmpargument = "particle", tmpdistance = "character vertex", tmpx = "offset x", tmpy = "offset y" )
    /// @author ZZ
    /// @details This function spawns a particle, offset from the character's location

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    //If we are a mount, our rider is the owner of this particle
    if ( pchr->isMount() && _gameObjects.exists( pchr->holdingwhich[SLOT_LEFT] ) )
    {
        ichr = pchr->holdingwhich[SLOT_LEFT];
    }

    iprt = ParticleHandler::get().spawn_one_particle(pchr->getPosition(), pchr->ori.facing_z, pchr->profile_ref,
                                                     LocalParticleProfileRef(pstate->argument), pself->index,
                                                     pstate->distance, pchr->team, ichr, INVALID_PRT_REF, 0,
                                                     INVALID_CHR_REF );

    returncode = DEFINED_PRT( iprt );
    if ( returncode )
    {
        fvec3_t tmp_pos;
        prt_t *pprt = ParticleHandler::get().get_ptr( iprt );

        // attach the particle
        place_particle_at_vertex( pprt, pself->index, pstate->distance );
        pprt->attachedto_ref = INVALID_CHR_REF;

        tmp_pos = pprt->getPosition();

        // Correct X, Y, Z spacing
        tmp_pos[kZ] += PipStack.get_ptr(pprt->pip_ref)->spacing_vrt_pair.base;

        // Don't spawn in walls
        tmp_pos[kX] += pstate->x;
        if (EMPTY_BIT_FIELD != pprt->test_wall(tmp_pos, nullptr))
        {
            tmp_pos[kX] = pprt->pos[kX];

            tmp_pos[kY] += pstate->y;
            if (EMPTY_BIT_FIELD != pprt->test_wall(tmp_pos, nullptr))
            {
                tmp_pos[kY] = pprt->pos[kY];
            }
        }

        pprt->setPosition(tmp_pos);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAlive( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAlive()
    /// @author ZZ
    /// @details This function proceeds if the target is alive

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->alive;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Stop( script_state_t * pstate, ai_state_t * pself )
{
    // Stop()
    /// @author ZZ
    /// @details This function sets the character's maximum acceleration to 0.  Used
    /// along with Walk and Run and Sneak

    SCRIPT_FUNCTION_BEGIN();

    pchr->maxaccel      = 0;
    pchr->movement_bits = CHR_MOVEMENT_BITS_STOP;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisaffirmCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // DisaffirmCharacter()
    /// @author ZZ
    /// @details This function removes all the attached particles from a character
    /// ( stuck arrows, flames, etc )

    SCRIPT_FUNCTION_BEGIN();

    disaffirm_attached_particles( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ReaffirmCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // ReaffirmCharacter()
    /// @author ZZ
    /// @details This function makes sure it has all of its reaffirmation particles
    /// attached to it. Used to make the torch light again

    SCRIPT_FUNCTION_BEGIN();

    reaffirm_attached_particles( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsSelf( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsSelf()
    /// @author ZZ
    /// @details This function proceeds if the character is targeting itself

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->target == pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsMale( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsMale()
    /// @author ZZ
    /// @details This function proceeds only if the target is male

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->gender == GENDER_MALE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsFemale( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsFemale()
    /// @author ZZ
    /// @details This function proceeds if the target is female

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->gender == GENDER_FEMALE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToSelf( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToSelf()
    /// @author ZZ
    /// @details This function sets the target to the character itself

    SCRIPT_FUNCTION_BEGIN();

    SET_TARGET_0( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToRider( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToRider()
    /// @author ZZ
    /// @details This function sets the target to whoever is riding the character (left/only grip),
    /// failing if there is no rider

    SCRIPT_FUNCTION_BEGIN();

    if ( _gameObjects.exists( pchr->holdingwhich[SLOT_LEFT] ) )
    {
        SET_TARGET_0( pchr->holdingwhich[SLOT_LEFT] );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_AttackTurn( script_state_t * pstate, ai_state_t * pself )
{
    // tmpturn = GetAttackTurn()
    /// @author ZZ
    /// @details This function sets tmpturn to the direction from which the last attack
    /// came. Not particularly useful in most cases, but it could be.

    SCRIPT_FUNCTION_BEGIN();

    pstate->turn = pself->directionlast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_DamageType( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetDamageType()
    /// @author ZZ
    /// @details This function sets tmpargument to the damage type of the last attack that
    /// hit the character

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = pself->damagetypelast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeSpell( script_state_t * pstate, ai_state_t * pself )
{
    // BecomeSpell()
    /// @author ZZ
    /// @details This function turns a spellbook character into a spell based on its
    /// content.
    /// TOO COMPLICATED TO EXPLAIN.  SHOULDN'T EVER BE NEEDED BY YOU.

    int iskin;

    SCRIPT_FUNCTION_BEGIN();

    // get the spellbook's skin
    iskin = pchr->skin;

    // change the spellbook to a spell effect
    change_character( pself->index, ( PRO_REF )pself->content, 0, ENC_LEAVE_NONE );

    // set the spell effect parameters
    pself->content = 0;
    chr_set_ai_state( pchr, 0 );

    // have to do this every time pself->state is modified
    chr_update_hide( pchr );

    // set the book icon of the spell effect if it is not already set
    /*
    //ZF> TODO: is this needed? what does it actually do?
    pcap = chr_get_pcap( pself->index );
    if ( NULL != pcap )
    {
        iskin = ( iskin < 0 ) ? NO_SKIN_OVERRIDE : iskin;
        iskin = ( iskin > SKINS_PEROBJECT_MAX ) ? SKINS_PEROBJECT_MAX : iskin;

        pcap->spelleffect_type = iskin;
    }
    */

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeSpellbook( script_state_t * pstate, ai_state_t * pself )
{
    // BecomeSpellbook()
    //
    /// @author ZZ
    /// @details This function turns a spell character into a spellbook and sets the content accordingly.
    /// TOO COMPLICATED TO EXPLAIN. Just copy the spells that already exist, and don't change
    /// them too much

    PRO_REF  old_profile;
    mad_t * pmad;
    int iskin;

    SCRIPT_FUNCTION_BEGIN();

    // Figure out what this spellbook looks like
    iskin = ppro->getSpellEffectType();
    if ( iskin < 0 ) iskin = 0;

    // convert the spell effect to a spellbook
    old_profile = pchr->profile_ref;
    change_character( pself->index, (PRO_REF)SPELLBOOK, iskin, ENC_LEAVE_NONE );

    // Reset the spellbook state so it doesn't burn up
    chr_set_ai_state(pchr, 0);
    pself->content = REF_TO_INT( old_profile );

    // set the spellbook animations
    pmad = chr_get_pmad(pself->index);

    if ( NULL != pmad )
    {
        // Do dropped animation
        int tmp_action = mad_get_action_ref(pchr->inst.imad, ACTION_JB);

        if (rv_success == chr_start_anim(pchr, tmp_action, false, true))
        {
            returncode = true;
        }
    }

    // have to do this every time pself->state is modified
    chr_update_hide(pchr);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ScoredAHit( script_state_t * pstate, ai_state_t * pself )
{
    // IfScoredAHit()
    /// @author ZZ
    /// @details This function proceeds if the character damaged another character this
    /// update. If it's a held character it also sets the target to whoever was hit

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character scored a hit
//    if ( !_gameObjects.exists( pchr->attachedto ) || _gameObjects.get(pchr->attachedto).ismount )
//    {
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_SCOREDAHIT );
//    }

    // Proceed only if the holder scored a hit with the character
    /*    else if ( _gameObjects.get(pchr->attachedto).ai.lastitemused == pself->index )
        {
            returncode = HAS_SOME_BITS( _gameObjects.get(pchr->attachedto).ai.alert, ALERTIF_SCOREDAHIT );
        }
        else returncode = false;*/

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Disaffirmed( script_state_t * pstate, ai_state_t * pself )
{
    // IfDisaffirmed()
    /// @author ZZ
    /// @details This function proceeds if the character was disaffirmed.
    /// This doesn't seem useful anymore.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_DISAFFIRMED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TranslateOrder( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx,tmpy,tmpargument = TranslateOrder()
    /// @author ZZ
    /// @details This function translates a packed order into understandable values.
    /// See CreateOrder for more.  This function sets tmpx, tmpy, tmpargument,
    /// and sets the target ( if valid )

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = CLIP_TO_16BITS( pself->order_value >> 24 );

    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET_0( ichr );

        pstate->x        = (( pself->order_value >> 14 ) & 0x03FF ) << 6;
        pstate->y        = (( pself->order_value >>  4 ) & 0x03FF ) << 6;
        pstate->argument = (( pself->order_value >>  0 ) & 0x000F );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverWasHit( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverWasHit()
    /// @author ZZ
    /// @details This function sets the target to whoever was hit by the character last

    SCRIPT_FUNCTION_BEGIN();

    if ( _gameObjects.exists( pself->hitlast ) )
    {
        SET_TARGET_0( pself->hitlast );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWideEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWideEnemy()
    /// @author ZZ
    /// @details This function sets the target to an enemy in the vicinity around the
    /// character, failing if there are none

    CHR_REF ichr;
    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, WIDE, IDSZ_NONE, TARGET_ENEMIES );

    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Changed( script_state_t * pstate, ai_state_t * pself )
{
    // IfChanged()
    /// @author ZZ
    /// @details This function proceeds if the character was polymorphed.
    /// Needed for morph spells and such

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CHANGED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_InWater( script_state_t * pstate, ai_state_t * pself )
{
    // IfInWater()
    /// @author ZZ
    /// @details This function proceeds if the character has just entered into some water
    /// this update ( and the water is really water, not fog or another effect )

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_INWATER );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Bored( script_state_t * pstate, ai_state_t * pself )
{
    // IfBored()
    /// @author ZZ
    /// @details This function proceeds if the character has been standing idle too long

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_BORED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TooMuchBaggage( script_state_t * pstate, ai_state_t * pself )
{
    // IfTooMuchBaggage()
    /// @author ZZ
    /// @details This function proceeds if the character tries to put an item in his/her
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
    /// @author ZZ
    /// @details This function proceeds if the character has been grogged ( a type of
    /// confusion ) this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = _gameObjects.get(pself->index)->grog_timer > 0 && HAS_SOME_BITS( pself->alert, ALERTIF_CONFUSED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Dazed( script_state_t * pstate, ai_state_t * pself )
{
    // IfDazed()
    /// @author ZZ
    /// @details This function proceeds if the character has been dazed ( a type of
    /// confusion ) this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = _gameObjects.get(pself->index)->daze_timer > 0 && HAS_SOME_BITS( pself->alert, ALERTIF_CONFUSED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasSpecialID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasSpecialID( tmpargument = "special idsz" )
    /// @author ZZ
    /// @details This function proceeds if the character has a special IDSZ ( in data.txt )

    Object *pself_target;
    
    SCRIPT_FUNCTION_BEGIN();
    
    SCRIPT_REQUIRE_TARGET(pself_target);

    returncode = pself_target->getProfile()->getIDSZ(IDSZ_SPECIAL) == static_cast<IDSZ>(pstate->argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PressTargetLatchButton( script_state_t * pstate, ai_state_t * pself )
{
    // PressTargetLatchButton( tmpargument = "latch bits" )
    /// @author ZZ
    /// @details This function mimics joystick button presses for the target.
    /// For making items force their own usage and such

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if(pstate->argument >= LATCHBUTTON_LEFT && pstate->argument < LATCHBUTTON_RESPAWN)
    {
        pself_target->latch.b[pstate->argument] = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Invisible( script_state_t * pstate, ai_state_t * pself )
{
    // IfInvisible()
    /// @author ZZ
    /// @details This function proceeds if the character is invisible

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->inst.alpha <= INVISIBLE;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ArmorIs( script_state_t * pstate, ai_state_t * pself )
{
    // IfArmorIs( tmpargument = "skin" )
    /// @author ZZ
    /// @details This function proceeds if the character's skin type equals tmpargument

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
    /// @author ZZ
    /// @details This function sets tmpargument to the number of updates before the
    /// character is ungrogged, proceeding if the number is greater than 0

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pstate->argument = pself_target->grog_timer;

    returncode = ( 0 != pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetDazeTime( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTargetDazeTime()
    /// @author ZZ
    /// @details This function sets tmpargument to the number of updates before the
    /// character is undazed, proceeding if the number is greater than 0

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pstate->argument = pself_target->daze_timer;

    returncode = ( 0 != pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageType( script_state_t * pstate, ai_state_t * pself )
{
    // SetDamageType( tmpargument = "damage type" )
    /// @author ZZ
    /// @details This function lets a weapon change the type of damage it inflicts

    SCRIPT_FUNCTION_BEGIN();

    pchr->damagetarget_damagetype = static_cast<DamageType>(pstate->argument % DAMAGE_COUNT);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_WaterLevel( script_state_t * pstate, ai_state_t * pself )
{
    // SetWaterLevel( tmpargument = "level" )
    /// @author ZZ
    /// @details This function raises or lowers the water in the module

    SCRIPT_FUNCTION_BEGIN();

    returncode = water_instance_set_douse_level( &water, pstate->argument / 10.0f );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnchantTarget( script_state_t * pstate, ai_state_t * pself )
{
    // EnchantTarget()
    /// @author ZZ
    /// @details This function enchants the target with the enchantment given
    /// in enchant.txt. Make sure you use set_OwnerToTarget before doing this.

    ENC_REF iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = EnchantHandler::get().spawn_one_enchant( pself->owner, pself->target, pself->index, INVALID_ENC_REF, INVALID_PRO_REF );
    returncode = DEFINED_ENC( iTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnchantChild( script_state_t * pstate, ai_state_t * pself )
{
    // EnchantChild()
    /// @author ZZ
    /// @details This function can be used with SpawnCharacter to enchant the
    /// newly spawned character with the enchantment
    /// given in enchant.txt. Make sure you use set_OwnerToTarget before doing this.

    ENC_REF iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = EnchantHandler::get().spawn_one_enchant( pself->owner, pself->child, pself->index, INVALID_ENC_REF, INVALID_PRO_REF );
    returncode = DEFINED_ENC( iTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TeleportTarget( script_state_t * pstate, ai_state_t * pself )
{
    // TeleportTarget( tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function teleports the target to the X, Y location, failing if the
    /// location is off the map or blocked

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _gameObjects[pself->target];
    if(!target) {
        return false;
    }

    returncode = target->teleport(fvec3_t(pstate->x, pstate->y, pstate->distance), pstate->turn);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetExperience( script_state_t * pstate, ai_state_t * pself )
{
    // GiveExperienceToTarget( tmpargument = "amount", tmpdistance = "type" )
    /// @author ZZ
    /// @details This function gives the target some experience, xptype from distance,
    /// amount from argument.

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _gameObjects[pself->target];
    if(!target) {
        return false;
    }

    target->giveExperience(pstate->argument, static_cast<XPType>(pstate->distance), false);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IncreaseAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // IncreaseAmmo()
    /// @author ZZ
    /// @details This function increases the character's ammo by 1

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
    /// @author ZZ
    /// @details This function unkurses the target

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->iskursed = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetTeamExperience( script_state_t * pstate, ai_state_t * pself )
{
    // GiveExperienceToTargetTeam( tmpargument = "amount", tmpdistance = "type" )
    /// @author ZZ
    /// @details This function gives experience to everyone on the target's team

    SCRIPT_FUNCTION_BEGIN();

    if(pstate->distance < XP_COUNT && pstate->distance >= 0) {
        give_team_experience(chr_get_iteam(pself->target), pstate->argument, static_cast<XPType>(pstate->distance) );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Unarmed( script_state_t * pstate, ai_state_t * pself )
{
    // IfUnarmed()
    /// @author ZZ
    /// @details This function proceeds if the character is holding no items in hand.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( !_gameObjects.exists( pchr->holdingwhich[SLOT_LEFT] ) && !_gameObjects.exists( pchr->holdingwhich[SLOT_RIGHT] ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RestockTargetAmmoIDAll( script_state_t * pstate, ai_state_t * pself )
{
    // RestockTargetAmmoIDAll( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function restocks the ammo of every item the character is holding,
    /// if the item matches the ID given ( parent or child type )

    CHR_REF ichr;
    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    iTmp = 0;  // Amount of ammo given

    ichr = pself_target->holdingwhich[SLOT_LEFT];
    iTmp += restock_ammo( ichr, pstate->argument );

    ichr = pself_target->holdingwhich[SLOT_RIGHT];
    iTmp += restock_ammo( ichr, pstate->argument );

    PACK_BEGIN_LOOP( pchr->inventory, pitem, item )
    {
        iTmp += restock_ammo( item, pstate->argument );
    }
    PACK_END_LOOP();

    pstate->argument = iTmp;
    returncode = ( iTmp != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RestockTargetAmmoIDFirst( script_state_t * pstate, ai_state_t * pself )
{
    // RestockTargetAmmoIDFirst( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function restocks the ammo of the first item the character is holding,
    /// if the item matches the ID given ( parent or child type )

    int     iTmp;
    int     ichr;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    iTmp = 0;  // Amount of ammo given
    
    ichr = pself_target->holdingwhich[SLOT_LEFT];
    iTmp += restock_ammo(ichr, pstate->argument);
    
    if (iTmp == 0)
    {
        ichr = pself_target->holdingwhich[SLOT_RIGHT];
        iTmp += restock_ammo(ichr, pstate->argument);
    }

    if (iTmp == 0)
    {
        PACK_BEGIN_LOOP( pself_target->inventory, pitem, item )
        {
            iTmp += restock_ammo( item, pstate->argument );
            if ( 0 != iTmp ) break;
        }
        PACK_END_LOOP()
    }

    pstate->argument = iTmp;
    returncode = ( iTmp != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashTarget( script_state_t * pstate, ai_state_t * pself )
{
    // FlashTarget()
    /// @author ZZ
    /// @details This function makes the target flash

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    FlashObject( pself_target, 255 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_RedShift( script_state_t * pstate, ai_state_t * pself )
{
    // SetRedShift( tmpargument = "red darkening" )
    /// @author ZZ
    /// @details This function sets the character's red shift ( 0 - 3 ), higher values
    /// making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    chr_set_redshift( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_GreenShift( script_state_t * pstate, ai_state_t * pself )
{
    // SetGreenShift( tmpargument = "green darkening" )
    /// @author ZZ
    /// @details This function sets the character's green shift ( 0 - 3 ), higher values
    /// making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    chr_set_grnshift( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BlueShift( script_state_t * pstate, ai_state_t * pself )
{
    // SetBlueShift( tmpargument = "blue darkening" )
    /// @author ZZ
    /// @details This function sets the character's blue shift ( 0 - 3 ), higher values
    /// making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    chr_set_grnshift( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Light( script_state_t * pstate, ai_state_t * pself )
{
    // SetLight( tmpargument = "lighness" )
    /// @author ZZ
    /// @details This function alters the character's transparency ( 0 - 254 )
    /// 255 = no transparency

    SCRIPT_FUNCTION_BEGIN();

    pchr->setLight(pstate->argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Alpha( script_state_t * pstate, ai_state_t * pself )
{
    // SetAlpha( tmpargument = "alpha" )
    /// @author ZZ
    /// @details This function alters the character's transparency ( 0 - 255 )
    /// 255 = no transparency

    SCRIPT_FUNCTION_BEGIN();

    pchr->setAlpha(pstate->argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromBehind( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitFromBehind()
    /// @author ZZ
    /// @details This function proceeds if the last attack to the character came from behind

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( pself->directionlast >= ATK_BEHIND - 8192 && pself->directionlast < ATK_BEHIND + 8192 )
        returncode = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromFront( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitFromFront()
    /// @author ZZ
    /// @details This function proceeds if the last attack to the character came
    /// from the front

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( pself->directionlast >= ATK_LEFT + 8192 || pself->directionlast < ATK_FRONT + 8192 )
        returncode = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromLeft( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitFromLeft()
    /// @author ZZ
    /// @details This function proceeds if the last attack to the character came
    /// from the left

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( pself->directionlast >= ATK_LEFT - 8192 && pself->directionlast < ATK_LEFT + 8192 )
        returncode = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromRight( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitFromRight()
    /// @author ZZ
    /// @details This function proceeds if the last attack to the character came
    /// from the right

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( pself->directionlast >= ATK_RIGHT - 8192 && pself->directionlast < ATK_RIGHT + 8192 )
        returncode = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnSameTeam( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsOnSameTeam()
    /// @author ZZ
    /// @details This function proceeds if the target is on the character's team

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( chr_get_iteam( pself->target ) == pchr->team )
        returncode = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KillTarget( script_state_t * pstate, ai_state_t * pself )
{
    // KillTarget()
    /// @author ZZ
    /// @details This function kills the target

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;

    //Weapons don't kill people, people kill people...
    if ( _gameObjects.exists( pchr->attachedto ) && !_gameObjects.get(pchr->attachedto)->isMount() )
    {
        ichr = pchr->attachedto;
    }

    const std::shared_ptr<Object> &target = _gameObjects[pself->target];
    if(target) {
        target->kill(_gameObjects[ichr], false);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UndoEnchant( script_state_t * pstate, ai_state_t * pself )
{
    // UndoEnchant()
    /// @author ZZ
    /// @details This function removes the last enchantment spawned by the character,
    /// proceeding if an enchantment was removed

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_ENC( pchr->undoenchant ) )
    {
        returncode = remove_enchant( pchr->undoenchant, NULL );
    }
    else
    {
        pchr->undoenchant = INVALID_ENC_REF;
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_WaterLevel( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetWaterLevel()
    /// @author ZZ
    /// @details This function sets tmpargument to the current douse level for the water * 10.
    /// A waterlevel in wawalight of 85 would set tmpargument to 850

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = water.douse_level * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostTargetMana( script_state_t * pstate, ai_state_t * pself )
{
    // CostTargetMana( tmpargument = "amount" )
    /// @author ZZ
    /// @details This function costs the target a specific amount of mana, proceeding
    /// if the target was able to pay the price.  The amounts are 8.8 fixed point

    SCRIPT_FUNCTION_BEGIN();

    returncode = cost_mana( pself->target, pstate->argument, pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasAnyID( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasAnyID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target has any IDSZ that matches the given one

    SCRIPT_FUNCTION_BEGIN();

    returncode = chr_has_idsz( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BumpSize( script_state_t * pstate, ai_state_t * pself )
{
    // SetBumpSize( tmpargument = "size" )
    /// @author ZZ
    /// @details This function sets the how wide the character is

    SCRIPT_FUNCTION_BEGIN();

    pchr->setBumpWidth(pstate->argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotDropped( script_state_t * pstate, ai_state_t * pself )
{
    // IfNotDropped()
    /// @author ZZ
    /// @details This function proceeds if the character is kursed and another character
    /// was holding it and tried to drop it

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_NOTDROPPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_YIsLessThanX( script_state_t * pstate, ai_state_t * pself )
{
    // IfYIsLessThanX()
    /// @author ZZ
    /// @details This function proceeds if tmpy is less than tmpx

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->y < pstate->x );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FlyHeight( script_state_t * pstate, ai_state_t * pself )
{
    // SetFlyHeight( tmpargument = "height" )
    /// @author ZZ
    /// @details This function makes the character fly ( or fall to ground if 0 )

    SCRIPT_FUNCTION_BEGIN();

    pchr->flyheight = std::max( 0, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Blocked( script_state_t * pstate, ai_state_t * pself )
{
    // IfBlocked()
    /// @author ZZ
    /// @details This function proceeds if the character blocked the attack of another
    /// character this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_BLOCKED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsDefending( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsDefending()
    /// @author ZZ
    /// @details This function proceeds if the target is holding up a shield or similar
    /// defense

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ACTION_IS_TYPE( pself_target->inst.action_which, P );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAttacking( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAttacking()
    /// @author ZZ
    /// @details This function proceeds if the target is doing an attack action

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->isAttacking();

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
    /// @author ZZ
    /// @details This function proceeds if the content matches tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->argument == pself->content );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToWatchTarget( script_state_t * pstate, ai_state_t * pself )
{
    // SetTurnModeToWatchTarget()
    /// @author ZZ
    /// @details This function makes the character face its target, no matter what
    /// direction it is moving in.  Undo this with set_TurnModeToVelocity

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_WATCHTARGET;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIsNot( script_state_t * pstate, ai_state_t * pself )
{
    // IfStateIsNot( tmpargument = "test" )
    /// @author ZZ
    /// @details This function proceeds if the character's state does not equal tmpargument

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
    /// @author ZZ
    /// @details This function spits out some useful numbers

    SCRIPT_FUNCTION_BEGIN();

    DisplayMsg_printf( "aistate %d, aicontent %d, target %d", pself->state, pself->content, REF_TO_INT( pself->target ) );
    DisplayMsg_printf( "tmpx %d, tmpy %d", pstate->x, pstate->y );
    DisplayMsg_printf( "tmpdistance %d, tmpturn %d", pstate->distance, pstate->turn );
    DisplayMsg_printf( "tmpargument %d, selfturn %d", pstate->argument, pchr->ori.facing_z );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BlackTarget( script_state_t * pstate, ai_state_t * pself )
{
    // BlackTarget()
    /// @author ZZ
    /// @details  The opposite of FlashTarget, causing the target to turn black

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    FlashObject( pself_target, 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SendMessageNear( script_state_t * pstate, ai_state_t * pself )
{
    // SendMessageNear( tmpargument = "message" )
    /// @author ZZ
    /// @details This function sends a message if the camera is in the nearby area

    int iTmp, min_distance;

    SCRIPT_FUNCTION_BEGIN();

    // iterate over all cameras and find the minimum distance
    min_distance = -1;
    for(std::shared_ptr<Camera> camera : CameraSystem::get()->getCameraList())
    {
        iTmp = std::fabs( pchr->pos_old[kX] - camera->getTrackPosition()[kX] ) + std::fabs( pchr->pos_old[kY] - camera->getTrackPosition()[kY] );

        if ( -1 == min_distance || iTmp < min_distance )
        {
            min_distance = iTmp;
        }
    }

    if ( min_distance < MSGDISTANCE )
    {
        returncode = _display_message( pself->index, pchr->profile_ref, pstate->argument, pstate );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitGround( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitGround()
    /// @author ZZ
    /// @details This function proceeds if a character hit the ground this update.
    /// Used to determine when to play the sound for a dropped item

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_HITGROUND );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NameIsKnown( script_state_t * pstate, ai_state_t * pself )
{
    // IfNameIsKnown()
    /// @author ZZ
    /// @details This function proceeds if the character's name is known

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->nameknown;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UsageIsKnown( script_state_t * pstate, ai_state_t * pself )
{
    // IfUsageIsKnown()
    /// @author ZZ
    /// @details This function proceeds if the character's usage is known

    SCRIPT_FUNCTION_BEGIN();

    returncode = ppro->isUsageKnown();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingItemID( script_state_t * pstate, ai_state_t * pself )
{
    // IfHoldingItemID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it

    CHR_REF item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_holding_idsz( pself->index, pstate->argument );

    returncode = _gameObjects.exists( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingRangedWeapon( script_state_t * pstate, ai_state_t * pself )
{
    // IfHoldingRangedWeapon()
    /// @author ZZ
    /// @details This function passes if the character is holding a ranged weapon, returning
    /// the latch to press to use it.  This also checks ammo.

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    pstate->argument = 0;

    // Check right hand
    const std::shared_ptr<Object> &rightHandItem = _gameObjects[pchr->holdingwhich[SLOT_RIGHT]];

    if (rightHandItem)
    {
        if ( rightHandItem->getProfile()->isRangedWeapon() && (0 == rightHandItem->ammomax || (0 != rightHandItem->ammo)))
        {
            pstate->argument = LATCHBUTTON_RIGHT;
            returncode = true;
        }
    }

    //50% chance to check left hand even though we have already found one in our right hand
    if ( !returncode || Random::nextBool() )
    {
        // Check left hand
        const std::shared_ptr<Object> &leftHandItem = _gameObjects[pchr->holdingwhich[SLOT_LEFT]];
        if (leftHandItem)
        {
            if ( leftHandItem->getProfile()->isRangedWeapon() && (0 == leftHandItem->ammomax || (0 != leftHandItem->ammo)))
            {
                pstate->argument = LATCHBUTTON_LEFT;
                returncode = true;
            }
        }

    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingMeleeWeapon( script_state_t * pstate, ai_state_t * pself )
{
    // IfHoldingMeleeWeapon()
    /// @author ZZ
    /// @details This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    pstate->argument = 0;

    if ( !returncode )
    {
        // Check right hand
        const std::shared_ptr<Object> &rightItem = pchr->getRightHandItem();
        if (rightItem)
        {
            if ( !rightItem->getProfile()->isRangedWeapon() && rightItem->getProfile()->getWeaponAction() != ACTION_PA )
            {
                if ( 0 == pstate->argument || ( update_wld & 1 ) )
                {
                    pstate->argument = LATCHBUTTON_RIGHT;
                    returncode = true;
                }
            }
        }
    }

    if ( !returncode )
    {
        // Check left hand
        const std::shared_ptr<Object> &leftItem = pchr->getLeftHandItem();
        if (leftItem)
        {
            if ( !leftItem->getProfile()->isRangedWeapon() && leftItem->getProfile()->getWeaponAction() != ACTION_PA )
            {
                pstate->argument = LATCHBUTTON_LEFT;
                returncode = true;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingShield( script_state_t * pstate, ai_state_t * pself )
{
    // IfHoldingShield()
    /// @author ZZ
    /// @details This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it. The button will need to be held down.

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    pstate->argument = 0;

    if ( !returncode )
    {
        // Check right hand
        const std::shared_ptr<Object> &rightItem = pchr->getRightHandItem();
        if ( rightItem )
        {
            if ( rightItem->getProfile()->getWeaponAction() == ACTION_PA )
            {
                pstate->argument = LATCHBUTTON_RIGHT;
                returncode = true;
            }
        }
    }

    if ( !returncode )
    {
        // Check left hand
        const std::shared_ptr<Object> &leftItem = pchr->getLeftHandItem();
        if ( leftItem )
        {     
            if ( leftItem->getProfile()->getWeaponAction() == ACTION_PA )
            {
                pstate->argument = LATCHBUTTON_LEFT;
                returncode = true;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Kursed( script_state_t * pstate, ai_state_t * pself )
{
    // IfKursed()
    /// @author ZZ
    /// @details This function proceeds if the character is kursed

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->iskursed;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsKursed( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsKursed()
    /// @author ZZ
    /// @details This function proceeds if the target is kursed

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->iskursed;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsDressedUp( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsDressedUp()
    /// @author ZZ
    /// @details This function proceeds if the target is dressed in fancy clothes

    SCRIPT_FUNCTION_BEGIN();

    returncode = ppro->getSkinInfo(pchr->skin).dressy;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OverWater( script_state_t * pstate, ai_state_t * pself )
{
    // IfOverWater()
    /// @author ZZ
    /// @details This function proceeds if the character is on a water tile

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->isOverWater(true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Thrown( script_state_t * pstate, ai_state_t * pself )
{
    // IfThrown()
    /// @author ZZ
    /// @details This function proceeds if the character was thrown this update.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_THROWN );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeNameKnown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeNameKnown()
    /// @author ZZ
    /// @details This function makes the name of the character known, for identifying
    /// weapons and spells and such

    SCRIPT_FUNCTION_BEGIN();

    pchr->nameknown = true;
    //           pchr->icon = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeUsageKnown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeUsageKnown()
    /// @author ZZ
    /// @details This function makes the usage known for this type of object
    /// For XP gains from using an unknown potion or such

    SCRIPT_FUNCTION_BEGIN();

    ppro->makeUsageKnown();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopTargetMovement( script_state_t * pstate, ai_state_t * pself )
{
    // StopTargetMovement()
    /// @author ZZ
    /// @details This function makes the target stop moving temporarily
    /// Sets the target's x and y velocities to 0, and
    /// sets the z velocity to 0 if the character is moving upwards.
    /// This is a special function for the IronBall object

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->vel[kX] = 0;
    pself_target->vel[kY] = 0;
    if ( pself_target->vel[kZ] > 0 ) pself_target->vel[kZ] = Physics::g_environment.gravity;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_XY( script_state_t * pstate, ai_state_t * pself )
{
    // SetXY( tmpargument = "index", tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function sets one of the 8 permanent storage variable slots
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
    /// @author ZZ
    /// @details This function reads one of the 8 permanent storage variable slots,
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
    /// @author ZZ
    /// @details This function alters the contents of one of the 8 permanent storage
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
    /// @author ZZ
    /// @details This function makes the character's ammo known ( for items )

    SCRIPT_FUNCTION_BEGIN();

    pchr->ammoknown = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedParticle( tmpargument = "particle", tmpdistance = "vertex" )
    /// @author ZZ
    /// @details This function spawns a particle attached to the character

    CHR_REF ichr, iholder;
    PRT_REF iprt;

    SCRIPT_FUNCTION_BEGIN();

    //If we are a weapon, our holder is the owner of this particle
    ichr    = pself->index;
    iholder = chr_get_lowest_attachment( ichr, true );
    if ( _gameObjects.exists( iholder ) )
    {
        ichr = iholder;
    }

    iprt = ParticleHandler::get().spawn_one_particle(pchr->getPosition(), pchr->ori.facing_z, pchr->profile_ref,
                                                     LocalParticleProfileRef(pstate->argument), pself->index,
                                                     pstate->distance, pchr->team, ichr, INVALID_PRT_REF, 0,
                                                     INVALID_CHR_REF);
    returncode = DEFINED_PRT( iprt );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnExactParticle( tmpargument = "particle", tmpx = "x", tmpy = "y", tmpdistance = "z" )
    /// @author ZZ
    /// @details This function spawns a particle at a specific x, y, z position

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    {
        fvec3_t vtmp =
            fvec3_t
            (
            pstate->x,
            pstate->y,
            pstate->distance
            );

        iprt = ParticleHandler::get().spawn_one_particle(vtmp, pchr->ori.facing_z, pchr->profile_ref,
                                                         LocalParticleProfileRef(pstate->argument),
                                                         INVALID_CHR_REF, 0, pchr->team, ichr,
                                                         INVALID_PRT_REF, 0, INVALID_CHR_REF);
    }

    returncode = DEFINED_PRT( iprt );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateTarget( script_state_t * pstate, ai_state_t * pself )
{
    // AccelerateTarget( tmpx = "acc x", tmpy = "acc y" )
    /// @author ZZ
    /// @details This function changes the x and y speeds of the target

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->vel[kX] += pstate->x;
    pself_target->vel[kY] += pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_distanceIsMoreThanTurn( script_state_t * pstate, ai_state_t * pself )
{
    // IfdistanceIsMoreThanTurn()
    /// @author ZZ
    /// @details This function proceeds tmpdistance is greater than tmpturn

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->distance > pstate->turn );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Crushed( script_state_t * pstate, ai_state_t * pself )
{
    // IfCrushed()
    /// @author ZZ
    /// @details This function proceeds if the character was crushed in a passage this
    /// update.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CRUSHED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeCrushValid( script_state_t * pstate, ai_state_t * pself )
{
    // MakeCrushValid()
    /// @author ZZ
    /// @details This function makes a character able to be crushed by closing doors
    /// and such

    SCRIPT_FUNCTION_BEGIN();

    pchr->canbecrushed = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLowestTarget( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToLowestTarget()
    /// @author ZZ
    /// @details This function sets the target to the absolute bottom character.
    /// The holder of the target, or the holder of the holder of the target, or
    /// the holder of the holder of ther holder of the target, etc.   This function never fails

    CHR_REF itarget;

    SCRIPT_FUNCTION_BEGIN();

    itarget = chr_get_lowest_attachment( pself->target, false );

    if ( _gameObjects.exists( itarget ) )
    {
        SET_TARGET_0( itarget );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotPutAway( script_state_t * pstate, ai_state_t * pself )
{
    // IfNotPutAway()
    /// @author ZZ
    /// @details This function proceeds if the character couldn't be put into another
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
    /// @author ZZ
    /// @details This function proceeds if the character is equiped in another's inventory,
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
    /// @author ZZ
    /// @details This function proceeds if the character itself has no ammo left.
    /// This is for crossbows and such, not archers.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 == pchr->ammo );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySoundLooped( script_state_t * pstate, ai_state_t * pself )
{
    // PlaySoundLooped( tmpargument = "sound", tmpdistance = "frequency" )

    /// @author ZZ
    /// @details This function starts playing a continuous sound

    SCRIPT_FUNCTION_BEGIN();

    SoundID sound = ppro->getSoundID(pstate->argument);
    
    if ( INVALID_SOUND_ID == sound )
    {
        // Stop existing sound loop (if any)
        AudioSystem::get().stopObjectLoopingSounds(pself->index);
    }
    else
    {
        // check whatever might be playing on the channel now
        //ZF> TODO: check if character is already playing a looped sound first!
        AudioSystem::get().playSoundLooped(sound, pself->index);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopSound( script_state_t * pstate, ai_state_t * pself )
{
    // StopSound( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function stops the playing of a continuous sound!

    SCRIPT_FUNCTION_BEGIN();

    AudioSystem::get().stopObjectLoopingSounds(pself->index, ppro->getSoundID(pstate->argument));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HealSelf( script_state_t * pstate, ai_state_t * pself )
{
    // HealSelf()
    /// @author ZZ
    /// @details This function gives life back to the character.
    /// Values given as 8.8 fixed point
    /// This does NOT remove [HEAL] enchants ( poisons )
    /// This does not set the ALERTIF_HEALED alert

    SCRIPT_FUNCTION_BEGIN();

    pchr->heal(_gameObjects[pchr->getCharacterID()], pstate->argument, true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Equip( script_state_t * pstate, ai_state_t * pself )
{
    // Equip()
    /// @author ZZ
    /// @details This function flags the character as being equipped.
    /// This is used by equipment items when they are placed in the inventory

    SCRIPT_FUNCTION_BEGIN();

    pchr->isequipped = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasItemIDEquipped( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasItemIDEquipped( tmpargument = "item idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target already wearing a matching item

    CHR_REF item;

    SCRIPT_FUNCTION_BEGIN();

    item = Inventory::findItem( pself->target, pstate->argument, true );

    returncode = _gameObjects.exists( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_OwnerToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // SetOwnerToTarget()
    /// @author ZZ
    /// @details This function must be called before enchanting anything.
    /// The owner is the character that pays the sustain costs and such for the enchantment

    SCRIPT_FUNCTION_BEGIN();

    pself->owner = pself->target;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToOwner( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToOwner()
    /// @author ZZ
    /// @details This function sets the target to whoever was previously declared as the
    /// owner.

    SCRIPT_FUNCTION_BEGIN();

    if ( _gameObjects.exists( pself->owner ) )
    {
        SET_TARGET_0( pself->owner );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Frame( script_state_t * pstate, ai_state_t * pself )
{
    // SetFrame( tmpargument = "frame" )
    /// @author ZZ
    /// @details This function sets the current .MD2 frame for the character.  Values are * 4

    int    frame_along = 0;
    Uint16 ilip        = 0;

    SCRIPT_FUNCTION_BEGIN();

    ilip        = pstate->argument & 3;
    frame_along = pstate->argument >> 2;
    chr_set_frame( pself->index, ACTION_DA, frame_along, ilip );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BreakPassage( script_state_t * pstate, ai_state_t * pself )
{
    // BreakPassage( tmpargument = "passage", tmpturn = "tile type", tmpdistance = "number of frames", tmpx = "borken tile", tmpy = "tile fx bits" )

    /// @author ZZ
    /// @details This function makes the tiles fall away ( turns into damage terrain )
    /// This function causes the tiles of a passage to increment if stepped on.
    /// tmpx and tmpy are both set to the location of whoever broke the tile if
    /// the function passed.

    SCRIPT_FUNCTION_BEGIN();

    returncode = BreakPassage( pstate->y, pstate->x, pstate->distance, pstate->turn, ( PASS_REF )pstate->argument, &( pstate->x ), &( pstate->y ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ReloadTime( script_state_t * pstate, ai_state_t * pself )
{
    // SetReloadTime( tmpargument = "time" )
    /// @author ZZ
    /// @details This function stops a character from being used for a while.  Used
    /// by weapons to slow down their attack rate.  50 clicks per second.

    SCRIPT_FUNCTION_BEGIN();

    pchr->reload_timer = std::max( 0, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWideBlahID( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWideBlahID( tmpargument = "idsz", tmpdistance = "blah bits" )
    /// @author ZZ
    /// @details This function sets the target to a character that matches the description,
    /// and who is located in the general vicinity of the character

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    // Try to find one
    ichr = chr_find_target( pchr, WIDE, pstate->argument, pstate->distance );

    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET_0( ichr );
        returncode = true;
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PoofTarget( script_state_t * pstate, ai_state_t * pself )
{
    // PoofTarget()
    /// @author ZZ
    /// @details This function removes the target from the game, failing if the
    /// target is a player

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = false;
    if ( INVALID_PLA( pself_target->is_which_player ) )             //Do not poof players
    {
        returncode = true;
        if ( pself->target == pself->index )
        {
            // Poof self later
            pself->poof_time = update_wld + 1;
        }
        else
        {
            // Poof others now
            pself_target->ai.poof_time = update_wld;

            SET_TARGET( pself->index, pself_target );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChildDoActionOverride( script_state_t * pstate, ai_state_t * pself )
{
    // ChildDoActionOverride( tmpargument = action )

    /// @author ZZ
    /// @details This function lets a character set the action of the last character
    /// it spawned.  It also sets the current frame to the first frame of the
    /// action ( no interpolation from last frame ). If the cation is not valid for the model,
    /// the function will fail

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( _gameObjects.exists( pself->child ) )
    {
        int action;

        Object * pchild = _gameObjects.get( pself->child );

        action = mad_get_action_ref( pchild->inst.imad, pstate->argument );

        if ( rv_success == chr_start_anim( pchild, action, false, true ) )
        {
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnPoof( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnPoof
    /// @author ZZ
    /// @details This function makes a lovely little poof at the character's location.
    /// The poof form and particle types are set in data.txt

    SCRIPT_FUNCTION_BEGIN();

    spawn_poof( pself->index, pchr->profile_ref );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SpeedPercent( script_state_t * pstate, ai_state_t * pself )
{
    // SetSpeedPercent( tmpargument = "percent" )
    /// @author ZZ
    /// @details This function acts like Run or Walk, except it allows the explicit
    /// setting of the speed

    float fvalue;

    SCRIPT_FUNCTION_BEGIN();

    pchr->resetAcceleration();

    fvalue = pstate->argument / 100.0f;
    fvalue = std::max( 0.0f, fvalue );

    pchr->maxaccel = pchr->maxaccel_reset * fvalue;

    if ( pchr->maxaccel < 0.33f )
    {
        // only sneak
        pchr->movement_bits = CHR_MOVEMENT_BITS_SNEAK | CHR_MOVEMENT_BITS_STOP;
    }
    else
    {
        // everything but sneak
        pchr->movement_bits = ( unsigned )( ~CHR_MOVEMENT_BITS_SNEAK );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildState( script_state_t * pstate, ai_state_t * pself )
{
    // SetChildState( tmpargument = "state" )
    /// @author ZZ
    /// @details This function lets a character set the state of the last character it
    /// spawned

    SCRIPT_FUNCTION_BEGIN();

    if ( VALID_CHR_RANGE( pself->child ) )
    {
        chr_set_ai_state( _gameObjects.get( pself->child ), pstate->argument );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedSizedParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedSizedParticle( tmpargument = "particle", tmpdistance = "vertex", tmpturn = "size" )
    /// @author ZZ
    /// @details This function spawns a particle of the specific size attached to the
    /// character. For spell charging effects

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    iprt = ParticleHandler::get().spawn_one_particle(pchr->getPosition(), pchr->ori.facing_z, pchr->profile_ref,
                                                     LocalParticleProfileRef(pstate->argument), pself->index,
                                                     pstate->distance, pchr->team, ichr, INVALID_PRT_REF, 0,
                                                     INVALID_CHR_REF);

    returncode = DEFINED_PRT( iprt );

    if ( returncode )
    {
        returncode = ParticleHandler::get().get_ptr(iprt)->set_size(pstate->turn);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeArmor( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeArmor( tmpargument = "time" )
    /// @author ZZ
    /// @details This function changes the character's armor.
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
    /// @author ZZ
    /// @details This function sets the value displayed by the module timer.
    /// For races and such.  50 clicks per second

    SCRIPT_FUNCTION_BEGIN();

    timeron = true;
    timervalue = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FacingTarget( script_state_t * pstate, ai_state_t * pself )
{
    // IfFacingTarget()
    /// @author ZZ
    /// @details This function proceeds if the character is more or less facing its
    /// target

    FACING_T sTmp = 0;
    Object *  pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pchr->isFacingLocation(pself_target->getPosX(), pself_target->getPosY());

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySoundVolume( script_state_t * pstate, ai_state_t * pself )
{
    // PlaySoundVolume( argument = "sound", distance = "volume" )
    /// @author ZZ
    /// @details This function sets the volume of a sound and plays it

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->distance > 0 )
    {
        int channel = AudioSystem::get().playSound(pchr->pos_old, ppro->getSoundID(pstate->argument));

        if ( channel != INVALID_SOUND_CHANNEL )
        {
            Mix_Volume( channel, ( 128*pstate->distance ) / 100 );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedFacedParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedFacedParticle(  tmpargument = "particle", tmpdistance = "vertex", tmpturn = "turn" )

    /// @author ZZ
    /// @details This function spawns a particle attached to the character, facing the
    /// same direction given by tmpturn

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    iprt = ParticleHandler::get().spawn_one_particle(pchr->getPosition(), CLIP_TO_16BITS( pstate->turn ),
                                                     pchr->profile_ref, LocalParticleProfileRef(pstate->argument),
                                                     pself->index, pstate->distance, pchr->team, ichr, INVALID_PRT_REF,
                                                     0, INVALID_CHR_REF);

    returncode = DEFINED_PRT( iprt );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIsOdd( script_state_t * pstate, ai_state_t * pself )
{
    // IfStateIsOdd()
    /// @author ZZ
    /// @details This function proceeds if the character's state is 1, 3, 5, 7, etc.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->state & 1 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToDistantEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToDistantEnemy( tmpdistance = "distance" )
    /// @author ZZ
    /// @details This function finds a character within a certain distance of the
    /// character, failing if there are none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, pstate->distance, IDSZ_NONE, TARGET_ENEMIES );

    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Teleport( script_state_t * pstate, ai_state_t * pself )
{
    // Teleport( tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function teleports the character to a new location, failing if
    /// the location is blocked or off the map

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->teleport(fvec3_t(pstate->x, pstate->y, pchr->getPosZ()), pchr->ori.facing_z);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetStrength( script_state_t * pstate, ai_state_t * pself )
{
    // GiveStrengthToTarget()
    // Permanently boost the target's strength

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd_int( 0, pself_target->strength, PERFECTSTAT, &iTmp );
        pself_target->strength += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetWisdom( script_state_t * pstate, ai_state_t * pself )
{
    // GiveWisdomToTarget()
    // Permanently boost the target's wisdom

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd_int( 0, pself_target->wisdom, PERFECTSTAT, &iTmp );
        pself_target->wisdom += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetIntelligence( script_state_t * pstate, ai_state_t * pself )
{
    // GiveIntelligenceToTarget()
    // Permanently boost the target's intelligence

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd_int( 0, pself_target->intelligence, PERFECTSTAT, &iTmp );
        pself_target->intelligence += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetDexterity( script_state_t * pstate, ai_state_t * pself )
{
    // GiveDexterityToTarget()
    // Permanently boost the target's dexterity

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd_int( 0, pself_target->dexterity, PERFECTSTAT, &iTmp );
        pself_target->dexterity += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetLife( script_state_t * pstate, ai_state_t * pself )
{
    // GiveLifeToTarget()
    /// @author ZZ
    /// @details Permanently boost the target's life

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd_int( LOWSTAT, pself_target->life_max, PERFECTBIG, &iTmp );
        pself_target->life_max += iTmp;
        if ( iTmp < 0 )
        {
            getadd_int( 1, pself_target->life, PERFECTBIG, &iTmp );
        }

        pself_target->life += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetMana( script_state_t * pstate, ai_state_t * pself )
{
    // GiveManaToTarget()
    /// @author ZZ
    /// @details Permanently boost the target's mana

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd_int( 0, pself_target->mana_max, PERFECTBIG, &iTmp );
        pself_target->mana_max += iTmp;
        if ( iTmp < 0 )
        {
            getadd_int( 0, pself_target->mana, PERFECTBIG, &iTmp );
        }

        pself_target->mana += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowMap( script_state_t * pstate, ai_state_t * pself )
{
    // ShowMap()
    /// @author ZZ
    /// @details This function shows the module's map.
    /// Fails if map already visible

    SCRIPT_FUNCTION_BEGIN();
    if ( mapon )  returncode = false;

    mapon = mapvalid;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowYouAreHere( script_state_t * pstate, ai_state_t * pself )
{
    // ShowYouAreHere()
    /// @author ZZ
    /// @details This function shows the blinking white blip on the map that represents the
    /// camera location

    SCRIPT_FUNCTION_BEGIN();

    youarehereon = mapvalid;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowBlipXY( script_state_t * pstate, ai_state_t * pself )
{
    // ShowBlipXY( tmpx = "x", tmpy = "y", tmpargument = "color" )

    /// @author ZZ
    /// @details This function draws a blip on the map, and must be done each update
    SCRIPT_FUNCTION_BEGIN();

    // Add a blip
    if ( blip_count < MAXBLIP )
    {
        if ( pstate->x > 0 && pstate->x < PMesh->gmem.edge_x && pstate->y > 0 && pstate->y < PMesh->gmem.edge_y )
        {
            if ( pstate->argument >= 0 )
            {
                blip_x[blip_count] = pstate->x;
                blip_y[blip_count] = pstate->y;
                blip_c[blip_count] = pstate->argument % COLOR_MAX;
                blip_count++;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HealTarget( script_state_t * pstate, ai_state_t * pself )
{
    // HealTarget( tmpargument = "amount" )
    /// @author ZZ
    /// @details This function gives some life back to the target.
    /// Values are 8.8 fixed point. Any enchantments that are removed by [HEAL], like poison, go away

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _gameObjects[pself->target];
    if(!target) {
        return false;
    }

    returncode = false;
    if ( target->heal(_gameObjects[pself->index], pstate->argument, false) )
    {
        returncode = true;
        remove_all_enchants_with_idsz(pself->target, MAKE_IDSZ('H', 'E', 'A', 'L'));
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PumpTarget( script_state_t * pstate, ai_state_t * pself )
{
    // PumpTarget( tmpargument = "amount" )
    /// @author ZZ
    /// @details This function gives some mana back to the target.
    /// Values are 8.8 fixed point

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd_int( 0, pself_target->mana, pself_target->mana_max, &iTmp );
        pself_target->mana += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // CostAmmo()
    /// @author ZZ
    /// @details This function costs the character 1 point of ammo

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
    /// @author ZZ
    /// @details This function makes the names of similar objects known.
    /// Checks all 6 IDSZ types to make sure they match.

    int tTmp;
    Uint16 sTmp = 0;

    SCRIPT_FUNCTION_BEGIN();

    for(const std::shared_ptr<Object> &object : _gameObjects.iterator())
    {

        sTmp = true;
        for ( tTmp = 0; tTmp < IDSZ_COUNT; tTmp++ )
        {
            if ( ppro->getIDSZ(tTmp) != object->getProfile()->getIDSZ(tTmp) )
            {
                sTmp = false;
            }
        }

        if ( sTmp )
        {
            object->nameknown = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedHolderParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedHolderParticle( tmpargument = "particle", tmpdistance = "vertex" )

    /// @author ZZ
    /// @details This function spawns a particle attached to the character's holder, or to the character if no holder

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    iprt = ParticleHandler::get().spawn_one_particle(pchr->getPosition(), pchr->ori.facing_z, pchr->profile_ref,
                                                     LocalParticleProfileRef(pstate->argument), ichr,
                                                     pstate->distance, pchr->team, ichr, INVALID_PRT_REF, 0,
                                                     INVALID_CHR_REF);

    returncode = DEFINED_PRT( iprt );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetReloadTime( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetReloadTime( tmpargument = "time" )

    /// @author ZZ
    /// @details This function sets the target's reload time
    /// This function stops the target from attacking for a while.

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pstate->argument > 0 )
    {
        pself_target->reload_timer = CLIP( pstate->argument, 0, 0xFFFF );
    }
    else
    {
        pself_target->reload_timer = 0;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogLevel( script_state_t * pstate, ai_state_t * pself )
{
    // SetFogLevel( tmpargument = "level" )
    /// @author ZZ
    /// @details This function sets the level of the module's fog.
    /// Values are * 10
    /// !!BAD!! DOESN'T WORK !!BAD!!

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( pstate->argument / 10.0f ) - fog.top;
    fog.top += fTmp;
    fog.distance += fTmp;
    fog.on = egoboo_config_t::get().graphic_fog_enable.getValue();
    if ( fog.distance < 1.0f )  fog.on = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_FogLevel( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetFogLevel()
    /// @author ZZ
    /// @details This function sets tmpargument to the level of the module's fog.
    /// Values are * 10

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = fog.top * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogTAD( script_state_t * pstate, ai_state_t * pself )
{
    /// @author ZZ
    /// @details This function sets the color of the module's fog.
    /// TAD stands for <turn, argument, distance> == <red, green, blue>.
    /// Makes sense, huh?
    /// !!BAD!! DOESN'T WORK !!BAD!!

    SCRIPT_FUNCTION_BEGIN();

    fog.red = CLIP( pstate->turn, 0, 0xFF );
    fog.grn = CLIP( pstate->argument, 0, 0xFF );
    fog.blu = CLIP( pstate->distance, 0, 0xFF );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogBottomLevel( script_state_t * pstate, ai_state_t * pself )
{
    // SetFogBottomLevel( tmpargument = "level" )

    /// @author ZZ
    /// @details This function sets the level of the module's fog.
    /// Values are * 10

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( pstate->argument / 10.0f ) - fog.bottom;
    fog.bottom += fTmp;
    fog.distance -= fTmp;
    fog.on = egoboo_config_t::get().graphic_fog_enable.getValue();
    if ( fog.distance < 1.0f )  fog.on = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_FogBottomLevel( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetFogBottomLevel()

    /// @author ZZ
    /// @details This function sets tmpargument to the level of the module's fog.
    /// Values are * 10

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = fog.bottom * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CorrectActionForHand( script_state_t * pstate, ai_state_t * pself )
{
    // CorrectActionForHand( tmpargument = "action" )
    /// @author ZZ
    /// @details This function changes tmpargument according to which hand the character
    /// is held in It turns ZA into ZA, ZB, ZC, or ZD.
    /// USAGE:  wizards casting spells

    SCRIPT_FUNCTION_BEGIN();
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        if ( pchr->inwhich_slot == SLOT_LEFT )
        {
            // A or B
            pstate->argument += Random::next(1);
        }
        else
        {
            // C or D
            pstate->argument += 2 + Random::next(1);
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsMounted( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsMounted()
    /// @author ZZ
    /// @details This function proceeds if the target is riding a mount

    CHR_REF ichr;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = false;

    ichr = pself_target->attachedto;
    if ( _gameObjects.exists( ichr ) )
    {
        returncode = _gameObjects.get(ichr)->isMount();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SparkleIcon( script_state_t * pstate, ai_state_t * pself )
{
    // SparkleIcon( tmpargument = "color" )
    /// @author ZZ
    /// @details This function starts little sparklies going around the character's icon

    SCRIPT_FUNCTION_BEGIN();
    if ( pstate->argument < COLOR_MAX )
    {
        if ( pstate->argument < -1 )
        {
            pchr->sparkle = NOSPARKLE;
        }
        else
        {
            pchr->sparkle = pstate->argument % COLOR_MAX;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnsparkleIcon( script_state_t * pstate, ai_state_t * pself )
{
    // UnsparkleIcon()
    /// @author ZZ
    /// @details This function stops little sparklies going around the character's icon

    SCRIPT_FUNCTION_BEGIN();

    pchr->sparkle = NOSPARKLE;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TileXY( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTileXY( tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function sets tmpargument to the tile type at the specified
    /// coordinates

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    TileIndex idx = ego_mesh_t::get_grid(PMesh, PointWorld(pstate->x, pstate->y));

    ego_tile_info_t *ptr = ego_mesh_t::get_ptile(PMesh, idx);
    if (ptr)
    {
        returncode = true;
        pstate->argument = ptr->img & TILE_LOWER_MASK;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TileXY( script_state_t * pstate, ai_state_t * pself )
{
    // scr_set_TileXY( tmpargument = "tile type", tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function changes the tile type at the specified coordinates

    SCRIPT_FUNCTION_BEGIN();

    TileIndex index = ego_mesh_t::get_grid(PMesh, PointWorld(pstate->x, pstate->y));
    returncode = ego_mesh_set_texture( PMesh, index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ShadowSize( script_state_t * pstate, ai_state_t * pself )
{
    // SetShadowSize( tmpargument = "size" )
    /// @author ZZ
    /// @details This function makes the character's shadow bigger or smaller

    SCRIPT_FUNCTION_BEGIN();

    pchr->shadow_size     = pstate->argument * pchr->fat;
    pchr->shadow_size_save = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OrderTarget( script_state_t * pstate, ai_state_t * pself )
{
    // OrderTarget( tmpargument = "order" )
    /// @author ZZ
    /// @details This function issues an order to the given target
    /// Be careful in using this, always checking IDSZ first

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !_gameObjects.exists( pself->target ) )
    {
        returncode = false;
    }
    else
    {
        returncode = ai_add_order( &( pself_target->ai ), pstate->argument, 0 );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverIsInPassage( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToWhoeverIsInPassage()
    /// @author ZZ
    /// @details This function sets the target to whoever is blocking the given passage
    /// This function lets passage rectangles be used as event triggers

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = PMod->getPassageByID(pstate->argument);

    returncode = false;
    if(passage)
    {
        ichr = passage->whoIsBlockingPassage(pself->index, IDSZ_NONE, TARGET_SELF | TARGET_FRIENDS | TARGET_ENEMIES, IDSZ_NONE);

        if ( _gameObjects.exists( ichr ) )
        {
            SET_TARGET_0( ichr );
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CharacterWasABook( script_state_t * pstate, ai_state_t * pself )
{
    // IfCharacterWasABook()
    /// @author ZZ
    /// @details This function proceeds if the base model is the same as the current
    /// model or if the base model is SPELLBOOK
    /// USAGE: USED BY THE MORPH SPELL. Not much use elsewhere

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pchr->basemodel_ref == SPELLBOOK ||
                   pchr->basemodel_ref == pchr->profile_ref );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_EnchantBoostValues( script_state_t * pstate, ai_state_t * pself )
{
    // SetEnchantBoostValues( tmpargument = "owner mana regen", tmpdistance = "owner life regen", tmpx = "target mana regen", tmpy = "target life regen" )
    /// @author ZZ
    /// @details This function sets the mana and life drains for the last enchantment
    /// spawned by this character.
    /// Values are 8.8 fixed point

    ENC_REF iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = pchr->undoenchant;

    returncode = false;
    if ( INGAME_ENC( iTmp ) )
    {
        EnchantHandler::get().get_ptr(iTmp)->owner_mana = pstate->argument;
        EnchantHandler::get().get_ptr(iTmp)->owner_life = pstate->distance;
        EnchantHandler::get().get_ptr(iTmp)->target_mana = pstate->x;
        EnchantHandler::get().get_ptr(iTmp)->target_life = pstate->y;

        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnCharacterXYZ( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnCharacterXYZ( tmpx = "x", tmpy = "y", tmpdistance = "z", tmpturn = "turn" )
    /// @author ZZ
    /// @details This function spawns a character of the same type at a specific location, failing if x,y,z is invalid

    SCRIPT_FUNCTION_BEGIN();

    fvec3_t pos = fvec3_t(pstate->x, pstate->y, pstate->distance);

    CHR_REF ichr = spawn_one_character( pos, pchr->profile_ref, pchr->team, 0, CLIP_TO_16BITS( pstate->turn ), NULL, INVALID_CHR_REF );
    returncode = _gameObjects.exists( ichr );

    if ( !returncode )
    {
        if ( ichr > PMod->getImportAmount() * MAX_IMPORT_PER_PLAYER )
        {
            log_warning( "Object %s failed to spawn a copy of itself\n", pchr->Name );
        }
    }
    else
    {
        Object * pchild = _gameObjects.get( ichr );

        // was the child spawned in a "safe" spot?
        if (!chr_get_safe(pchild))
        {
            _gameObjects.remove( ichr );
            ichr = INVALID_CHR_REF;
        }
        else
        {
            pself->child = ichr;

            pchild->iskursed   = pchr->iskursed;  /// @note BB@> inherit this from your spawner
            pchild->ai.passage = pself->passage;
            pchild->ai.owner   = pself->owner;

            pchild->dismount_timer  = PHYS_DISMOUNT_TIME;
            pchild->dismount_object = pself->index;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactCharacterXYZ( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnCharacterXYZ( tmpargument = "slot", tmpx = "x", tmpy = "y", tmpdistance = "z", tmpturn = "turn" )
    /// @author ZZ
    /// @details This function spawns a character at a specific location, using a
    /// specific model type, failing if x,y,z is invalid
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS,
    /// AS THE MODEL SLOTS MAY VARY FROM MODULE TO MODULE.

    SCRIPT_FUNCTION_BEGIN();

    fvec3_t pos =
        fvec3_t
        (
        pstate->x,
        pstate->y,
        pstate->distance
        );

    CHR_REF ichr = spawn_one_character(pos, static_cast<PRO_REF>(pstate->argument), pchr->team, 0, CLIP_TO_16BITS(pstate->turn), nullptr, INVALID_CHR_REF);
    const std::shared_ptr<Object> &pchild = _gameObjects[ichr];

    if ( !pchild )
    {
        if ( ichr > PMod->getImportAmount() * MAX_IMPORT_PER_PLAYER )
        {
            log_warning( "Object \"%s\"(\"%s\") failed to spawn profile index %d\n", pchr->Name, ppro->getClassName().c_str(), pstate->argument );
        }
        returncode = false;
    }
    else
    {
        // was the child spawned in a "safe" spot?
        if (!chr_get_safe(pchild.get()))
        {
            pchr->requestTerminate();
            returncode = false;
        }
        else
        {
            pself->child = ichr;

            pchild->iskursed   = pchr->iskursed;  /// @note BB@> inherit this from your spawner
            pchild->ai.passage = pself->passage;
            pchild->ai.owner   = pself->owner;

            pchild->dismount_timer  = PHYS_DISMOUNT_TIME;
            pchild->dismount_object = pself->index;
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTargetClass( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeTargetClass( tmpargument = "slot" )

    /// @author ZZ
    /// @details This function changes the target character's model slot.
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS, AS THE MODEL SLOTS MAY VARY FROM
    /// MODULE TO MODULE.
    /// USAGE: This is intended as a way to incorporate more player classes into the game.

    SCRIPT_FUNCTION_BEGIN();

    change_character_full( pself->target, ( PRO_REF )pstate->argument, 0, ENC_LEAVE_ALL );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlayFullSound( script_state_t * pstate, ai_state_t * pself )
{
    // PlayFullSound( tmpargument = "sound", tmpdistance = "frequency" )
    /// @author ZZ
    /// @details This function plays one of the character's sounds .
    /// The sound will be heard at full volume by all players (Victory music)

    SCRIPT_FUNCTION_BEGIN();

    AudioSystem::get().playSoundFull(ppro->getSoundID(pstate->argument));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactChaseParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnExactChaseParticle( tmpargument = "particle", tmpx = "x", tmpy = "y", tmpdistance = "z" )
    /// @author ZZ
    /// @details This function spawns a particle at a specific x, y, z position,
    /// that will home in on the character's target

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    {
        fvec3_t vtmp =
            fvec3_t
            (
            pstate->x,
            pstate->y,
            pstate->distance
            );

        iprt = ParticleHandler::get().spawn_one_particle(vtmp, pchr->ori.facing_z, pchr->profile_ref,
                                                         LocalParticleProfileRef(pstate->argument),
                                                         INVALID_CHR_REF, 0, pchr->team, ichr, INVALID_PRT_REF,
                                                         0, INVALID_CHR_REF);
    }

    returncode = DEFINED_PRT( iprt );

    if ( returncode )
    {
        ParticleHandler::get().get_ptr(iprt)->target_ref = pself->target;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CreateOrder( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = CreateOrder( tmpx = "value1", tmpy = "value2", tmpargument = "order" )

    /// @author ZZ
    /// @details This function compresses tmpx, tmpy, tmpargument ( 0 - 15 ), and the
    /// character's target into tmpargument.  This new tmpargument can then
    /// be issued as an order to teammates.  TranslateOrder will undo the
    /// compression

    Uint16 sTmp = 0;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = ( REF_TO_INT( pself->target ) & 0x00FF ) << 24;
    sTmp |= (( pstate->x >> 6 ) & 0x03FF ) << 14;
    sTmp |= (( pstate->y >> 6 ) & 0x03FF ) << 4;
    sTmp |= ( pstate->argument & 0x000F );
    pstate->argument = sTmp;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OrderSpecialID( script_state_t * pstate, ai_state_t * pself )
{
    // OrderSpecialID( tmpargument = "compressed order", tmpdistance = "idsz" )
    /// @author ZZ
    /// @details This function orders all characters with the given special IDSZ.

    SCRIPT_FUNCTION_BEGIN();

    issue_special_order( pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkurseTargetInventory( script_state_t * pstate, ai_state_t * pself )
{
    // UnkurseTargetInventory()
    /// @author ZZ
    /// @details This function unkurses all items held and in the pockets of the target

    CHR_REF ichr;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ichr = pself_target->holdingwhich[SLOT_LEFT];
    if ( _gameObjects.exists( ichr ) )
    {
        _gameObjects.get(ichr)->iskursed = false;
    }

    ichr = pself_target->holdingwhich[SLOT_RIGHT];
    if ( _gameObjects.exists( ichr ) )
    {
        _gameObjects.get(ichr)->iskursed = false;
    }

    PACK_BEGIN_LOOP( pself_target->inventory, pitem, item )
    {
        pitem->iskursed = false;
    }
    PACK_END_LOOP();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsSneaking( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsSneaking()
    /// @author ZZ
    /// @details This function proceeds if the target is doing ACTION_WA or ACTION_DA

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->inst.action_which == ACTION_DA || pself_target->inst.action_which == ACTION_WA );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropItems( script_state_t * pstate, ai_state_t * pself )
{
    // DropItems()
    /// @author ZZ
    /// @details This function drops all of the items the character is holding

    SCRIPT_FUNCTION_BEGIN();

    drop_all_items( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RespawnTarget( script_state_t * pstate, ai_state_t * pself )
{
    // RespawnTarget()
    /// @author ZZ
    /// @details This function respawns the target at its current location

    Object * pself_target;
    fvec3_t save_pos;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );
    save_pos = pself_target->getPosition();
    respawn_character( pself->target );
    pself_target->setPosition(save_pos);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDoActionSetFrame( script_state_t * pstate, ai_state_t * pself )
{
    // TargetDoActionSetFrame( tmpargument = "action" )
    /// @author ZZ
    /// @details This function starts the target doing the given action, and also sets
    /// the starting frame to the first of the animation ( so there is no
    /// interpolation 'cause it looks awful in some circumstances )
    /// It will fail if the action is invalid

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( _gameObjects.exists( pself->target ) )
    {
        int action;
        Object * pself_target = _gameObjects.get( pself->target );

        action = mad_get_action_ref( pself_target->inst.imad, pstate->argument );

        if ( rv_success == chr_start_anim( pself_target, action, false, true ) )
        {
            // remove the interpolation
            chr_instance_remove_interpolation( &( pself_target->inst ) );

            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanSeeInvisible( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetCanSeeInvisible()
    /// @author ZZ
    /// @details This function proceeds if the target can see invisible

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->see_invisible_level > 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestBlahID( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearestBlahID( tmpargument = "idsz", tmpdistance = "blah bits" )

    /// @author ZZ
    /// @details This function finds the NEAREST ( exact ) character that fits the given
    /// parameters, failing if it finds none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    // Try to find one
    ichr = chr_find_target( pchr, NEAREST, pstate->argument, pstate->distance );

    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearestEnemy()
    /// @author ZZ
    /// @details This function finds the NEAREST ( exact ) enemy, failing if it finds none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, NEAREST, IDSZ_NONE, TARGET_ENEMIES );

    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestFriend( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearestFriend()
    /// @author ZZ
    /// @details This function finds the NEAREST ( exact ) friend, failing if it finds none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, NEAREST, IDSZ_NONE, TARGET_FRIENDS );

    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestLifeform( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToNearestLifeform()

    /// @author ZZ
    /// @details This function finds the NEAREST ( exact ) friend or enemy, failing if it
    /// finds none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, NEAREST, IDSZ_NONE, TARGET_ITEMS | TARGET_FRIENDS | TARGET_ENEMIES );

    if ( _gameObjects.exists( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashPassage( script_state_t * pstate, ai_state_t * pself )
{
    // FlashPassage( tmpargument = "passage", tmpdistance = "color" )

    /// @author ZZ
    /// @details This function makes the given passage light or dark.
    /// Usage: For debug purposes

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = PMod->getPassageByID(pstate->argument);
    if(passage) {
        passage->flashColor(pstate->distance);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FindTileInPassage( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx, tmpy = FindTileInPassage( tmpargument = "passage", tmpdistance = "tile type", tmpx, tmpy )

    /// @author ZZ
    /// @details This function finds all tiles of the specified type that lie within the
    /// given passage.  Call multiple times to find multiple tiles.  tmpx and
    /// tmpy will be set to the middle of the found tile if one is found, or
    /// both will be set to 0 if no tile is found.
    /// tmpx and tmpy are required and set on return

    SCRIPT_FUNCTION_BEGIN();

    returncode = FindTileInPassage( pstate->x, pstate->y, pstate->distance, ( PASS_REF )pstate->argument, &( pstate->x ), &( pstate->y ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HeldInLeftHand( script_state_t * pstate, ai_state_t * pself )
{
    // IfHeldInLeftHand()
    /// @author ZZ
    /// @details This function passes if another character is holding the character in its
    /// left hand.
    /// Usage: Used mostly by enchants that target the item of the other hand

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    ichr = pchr->attachedto;
    if ( _gameObjects.exists( ichr ) )
    {
        returncode = ( _gameObjects.get(ichr)->holdingwhich[SLOT_LEFT] == pself->index );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotAnItem( script_state_t * pstate, ai_state_t * pself )
{
    // NotAnItem()
    /// @author ZZ
    /// @details This function makes the character a non-item character.
    /// Usage: Used for spells that summon creatures

    SCRIPT_FUNCTION_BEGIN();

    pchr->isitem = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // SetChildAmmo( tmpargument = "none" )
    /// @author ZZ
    /// @details This function sets the ammo of the last character spawned by this character

    SCRIPT_FUNCTION_BEGIN();

    _gameObjects.get(pself->child)->ammo = CLIP( pstate->argument, 0, 0xFFFF );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitVulnerable( script_state_t * pstate, ai_state_t * pself )
{
    // IfHitVulnerable()
    /// @author ZZ
    /// @details This function proceeds if the character was hit by a weapon of its
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
    /// @author ZZ
    /// @details This function proceeds if the character target is flying

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->flyheight > 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IdentifyTarget( script_state_t * pstate, ai_state_t * pself )
{
    // IdentifyTarget()
    /// @author ZZ
    /// @details This function reveals the target's name, ammo, and usage
    /// Proceeds if the target was unknown

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    ichr = pself->target;
    if ( _gameObjects.get(ichr)->ammomax != 0 )  _gameObjects.get(ichr)->ammoknown = true;


    returncode = !_gameObjects.get(ichr)->nameknown;
    _gameObjects.get(ichr)->nameknown = true;
    ppro->makeUsageKnown();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BeatModule( script_state_t * pstate, ai_state_t * pself )
{
    // BeatModule()
    /// @author ZZ
    /// @details This function displays the Module Ended message

    SCRIPT_FUNCTION_BEGIN();

    PMod->beatModule();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EndModule( script_state_t * pstate, ai_state_t * pself )
{
    // EndModule()
    /// @author ZZ
    /// @details This function presses the Escape key

    SCRIPT_FUNCTION_BEGIN();

    // This tells the game to quit
    _gameEngine->pushGameState(std::make_shared<VictoryScreen>(nullptr, true));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableExport( script_state_t * pstate, ai_state_t * pself )
{
    // DisableExport()
    /// @author ZZ
    /// @details This function turns export off

    SCRIPT_FUNCTION_BEGIN();

    PMod->setExportValid(false);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableExport( script_state_t * pstate, ai_state_t * pself )
{
    // EnableExport()
    /// @author ZZ
    /// @details This function turns export on

    SCRIPT_FUNCTION_BEGIN();

    PMod->setExportValid(true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetState( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTargetState()
    /// @author ZZ
    /// @details This function sets tmpargument to the state of the target

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pstate->argument = pself_target->ai.state;

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
    /// @author ZZ
    /// @details This function drops some of the target's money

    SCRIPT_FUNCTION_BEGIN();

    drop_money( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetContent( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTargetContent()
    // This sets tmpargument to the current Target's content value

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pstate->argument = pself_target->ai.content;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropTargetKeys( script_state_t * pstate, ai_state_t * pself )
{
    // DropTargetKeys()
    /// @author ZZ
    /// @details This function makes the Target drops keys in inventory (Not inhand)

    SCRIPT_FUNCTION_BEGIN();

    drop_keys( pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinTeam( tmpargument = "team" )
    /// @author ZZ
    /// @details This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, ( TEAM_REF )pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetJoinTeam( script_state_t * pstate, ai_state_t * pself )
{
    // TargetJoinTeam( tmpargument = "team" )
    /// @author ZZ
    /// @details This makes the Target join a Team specified in tmpargument (A = 0, 25 = Z, etc.)

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->target, ( TEAM_REF )pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearMusicPassage( script_state_t * pstate, ai_state_t * pself )
{
    // ClearMusicPassage( tmpargument = "passage" )
    /// @author ZZ
    /// @details This clears the music for a specified passage

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = PMod->getPassageByID(pstate->argument);
    if(passage) {
        passage->setMusic(Passage::NO_MUSIC);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearEndMessage( script_state_t * pstate, ai_state_t * pself )
{
    // ClearEndMessage()
    /// @author ZZ
    /// @details This function empties the end-module text buffer

    SCRIPT_FUNCTION_BEGIN();

    endtext[0] = CSTR_END;
    endtext_carat = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddEndMessage( script_state_t * pstate, ai_state_t * pself )
{
    // AddEndMessage( tmpargument = "message" )
    /// @author ZZ
    /// @details This function appends a message to the end-module text buffer

    SCRIPT_FUNCTION_BEGIN();

    returncode = AddEndMessage( pchr,  pstate->argument, pstate );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlayMusic( script_state_t * pstate, ai_state_t * pself )
{
    // PlayMusic( tmpargument = "song number", tmpdistance = "fade time (msec)" )
    /// @author ZZ
    /// @details This function begins playing a new track of music

    SCRIPT_FUNCTION_BEGIN();

    int fadeTime = pstate->distance;
    if(fadeTime < 0) fadeTime = 0;

    AudioSystem::get().playMusic(pstate->argument, fadeTime);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_MusicPassage( script_state_t * pstate, ai_state_t * pself )
{
    // SetMusicPassage( tmpargument = "passage", tmpturn = "type", tmpdistance = "repetitions" )

    /// @author ZZ
    /// @details This function makes the given passage play music if a player enters it
    /// tmpargument is the passage to set and tmpdistance is the music track to play.

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = PMod->getPassageByID(pstate->argument);
    if(passage) {
        passage->setMusic(pstate->distance);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeCrushInvalid( script_state_t * pstate, ai_state_t * pself )
{
    // MakeCrushInvalid()
    /// @author ZZ
    /// @details This function makes doors unable to close on this object

    SCRIPT_FUNCTION_BEGIN();

    pchr->canbecrushed = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopMusic( script_state_t * pstate, ai_state_t * pself )
{
    // StopMusic()
    /// @author ZZ
    /// @details This function stops the interactive music

    SCRIPT_FUNCTION_BEGIN();

    AudioSystem::get().stopMusic();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashVariable( script_state_t * pstate, ai_state_t * pself )
{
    // FlashVariable( tmpargument = "amount" )

    /// @author ZZ
    /// @details This function makes the character flash according to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    FlashObject( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateUp( script_state_t * pstate, ai_state_t * pself )
{
    // AccelerateUp( tmpargument = "acc z" )
    /// @author ZZ
    /// @details This function makes the character accelerate up and down

    SCRIPT_FUNCTION_BEGIN();

    pchr->vel[kZ] += pstate->argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashVariableHeight( script_state_t * pstate, ai_state_t * pself )
{
    // FlashVariableHeight( tmpturn = "intensity bottom", tmpx = "bottom", tmpdistance = "intensity top", tmpy = "top" )
    /// @author ZZ
    /// @details This function makes the character flash, feet one color, head another.

    SCRIPT_FUNCTION_BEGIN();

    flash_character_height( pself->index, CLIP_TO_16BITS( pstate->turn ), pstate->x, pstate->distance, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageTime( script_state_t * pstate, ai_state_t * pself )
{
    // SetDamageTime( tmpargument = "time" )
    /// @author ZZ
    /// @details This function makes the character invincible for a little while

    SCRIPT_FUNCTION_BEGIN();

    pchr->damage_timer = CLIP( pstate->argument, 0, 0xFFFF );

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
    /// @author ZZ
    /// @details This function passes if the Target is a mountable character

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->isMount();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAPlatform( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAPlatform()
    /// @author ZZ
    /// @details This function passes if the Target is a platform character

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->platform;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddStat( script_state_t * pstate, ai_state_t * pself )
{
    // AddStat()
    /// @author ZZ
    /// @details This function turns on an NPC's status display

    SCRIPT_FUNCTION_BEGIN();

    if ( !pchr->show_stats )
    {
        statlist_add( pself->index );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisenchantTarget( script_state_t * pstate, ai_state_t * pself )
{
    // DisenchantTarget()
    /// @author ZZ
    /// @details This function removes all enchantments on the Target character, proceeding
    /// if there were any, failing if not

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( INVALID_ENC_REF != pself_target->firstenchant );

    disenchant_character( pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisenchantAll( script_state_t * pstate, ai_state_t * pself )
{
    // DisenchantAll()
    /// @author ZZ
    /// @details This function removes all enchantments in the game

    ENC_REF iTmp;

    SCRIPT_FUNCTION_BEGIN();

    for ( iTmp = 0; iTmp < ENCHANTS_MAX; iTmp++ )
    {
        remove_enchant( iTmp, NULL );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_VolumeNearestTeammate( script_state_t * pstate, ai_state_t * pself )
{
    // SetVolumeNearestTeammate( tmpargument = "sound", tmpdistance = "distance" )
    /// @author ZZ
    /// @details This function lets insects buzz correctly.  The closest Team member
    /// is used to determine the overall sound level.

    SCRIPT_FUNCTION_BEGIN();

    //ZF> TODO: Not implemented

    /*PORT
    if(moduleactive && pstate->distance >= 0)
    {
    // Find the closest Teammate
    iTmp = 10000;
    sTmp = 0;
    while(sTmp < OBJECTS_MAX)
    {
    if(_gameObjects.exists(sTmp) && ChrList.lst[sTmp].alive && ChrList.lst[sTmp].Team == pchr->Team)
    {
    distance = ABS(PCamera->track.x-ChrList.lst[sTmp].pos_old.x)+ABS(PCamera->track.y-ChrList.lst[sTmp].pos_old.y);
    if(distance < iTmp)  iTmp = distance;
    }
    sTmp++;
    }
    distance=iTmp+pstate->distance;
    volume = -distance;
    volume = volume<<VOLSHIFT;
    if(volume < VOLMIN) volume = VOLMIN;
    iTmp = CapStack.lst[pro_get_icap(pchr->profile_ref)].wavelist[pstate->argument];
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
    /// @author ZZ
    /// @details This function makes a passage behave as a shop area, as long as the
    /// character is alive.

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = PMod->getPassageByID(pstate->argument);
    if(passage) {
        passage->makeShop(pself->index);
        returncode = true;
    }
    else {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetPayForArmor( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx, tmpy = TargetPayForArmor( tmpargument = "skin" )

    /// @author ZZ
    /// @details This function costs the Target the appropriate amount of money for the
    /// given armor type.  Passes if the character has enough, and fails if not.
    /// Does trade-in bonus automatically.  tmpy is always set to cost of requested
    /// skin tmpx is set to amount needed after trade-in ( 0 for pass ).

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !_gameObjects.exists( pself->target ) ) return false;

    pself_target = _gameObjects.get( pself->target );


    iTmp = ppro->getSkinInfo(pstate->argument).cost;
    pstate->y = iTmp;                                       // Cost of new skin

    iTmp -= ppro->getSkinInfo(pself_target->skin).cost;     // Refund for old skin

    if ( iTmp > pself_target->money )
    {
        // Not enough.
        pstate->x = iTmp - pself_target->money;        // Amount needed
        returncode = false;
    }
    else
    {
        // Pay for it.  Cost may be negative after refund.
        pself_target->money = pself_target->money - iTmp;
        pself_target->money = CLIP( pself_target->money, (Sint16)0, (Sint16)MAXMONEY );

        pstate->x = 0;
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinEvilTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinEvilTeam()
    /// @author ZZ
    /// @details This function adds the character to the evil Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, ( TEAM_REF )TEAM_EVIL );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinNullTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinNullTeam()
    /// @author ZZ
    /// @details This function adds the character to the null Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, ( TEAM_REF )TEAM_NULL );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinGoodTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinGoodTeam()
    /// @author ZZ
    /// @details This function adds the character to the good Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, ( TEAM_REF )TEAM_GOOD );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PitsKill( script_state_t * pstate, ai_state_t * pself )
{
    // PitsKill()
    /// @author ZZ
    /// @details This function activates pit deaths for when characters fall below a
    /// certain altitude.

    SCRIPT_FUNCTION_BEGIN();

    pits.kill = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToPassageID( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToPassageID( tmpargument = "passage", tmpdistance = "idsz" )
    /// @author ZZ
    /// @details This function finds a character who is both in the passage and who has
    /// an item with the given IDSZ

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = PMod->getPassageByID(pstate->argument);

    returncode = false;
    if(passage) {
        CHR_REF ichr = passage->whoIsBlockingPassage(pself->index, IDSZ_NONE, TARGET_SELF | TARGET_FRIENDS | TARGET_ENEMIES, pstate->distance);
        if ( _gameObjects.exists( ichr ) )
        {
            SET_TARGET_0( ichr );
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeNameUnknown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeNameUnknown()
    /// @author ZZ
    /// @details This function makes the name of an item/character unknown.
    /// Usage: Use if you have subspawning of creatures from a book.

    SCRIPT_FUNCTION_BEGIN();

    pchr->nameknown = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactParticleEndSpawn( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnExactParticleEndSpawn( tmpargument = "particle", tmpturn = "state", tmpx = "x", tmpy = "y", tmpdistance = "z" )

    /// @author ZZ
    /// @details This function spawns a particle at a specific x, y, z position.
    /// When the particle ends, a character is spawned at its final location.
    /// The character is the same type of whatever spawned the particle.

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( _gameObjects.exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    {
        fvec3_t vtmp =
            fvec3_t
            (
            pstate->x,
            pstate->y,
            pstate->distance
            );

        iprt = ParticleHandler::get().spawn_one_particle(vtmp, pchr->ori.facing_z, pchr->profile_ref,
                                                         LocalParticleProfileRef(pstate->argument),
                                                         INVALID_CHR_REF, 0, pchr->team, ichr, INVALID_PRT_REF,
                                                         0, INVALID_CHR_REF);
    }

    returncode = DEFINED_PRT( iprt );

    if ( returncode )
    {
        ParticleHandler::get().get_ptr(iprt)->endspawn_characterstate = pstate->turn;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnPoofSpeedSpacingDamage( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnPoofSpeedSpacingDamage( tmpx = "xy speed", tmpy = "xy spacing", tmpargument = "damage" )

    /// @author ZZ
    /// @details This function makes a lovely little poof at the character's location,
    /// adjusting the xy speed and spacing and the base damage first
    /// Temporarily adjust the values for the particle type

    int   tTmp, iTmp;
    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    PIP_REF particleRef = ppro->getParticlePoofProfile();
    if ( INVALID_PRT_REF == particleRef ) return false;
    pip_t * ppip = PipStack.get_ptr(particleRef);

    returncode = false;
    if ( NULL != ppip )
    {
        /// @note BB@> if we do not change both the ppip->damage.from AND the ppip->damage.to
        /// an error will be generated down the line...

        float damage_rand = ppip->damage.to - ppip->damage.from;

        // save some values
        iTmp = ppip->vel_hrz_pair.base;
        tTmp = ppip->spacing_hrz_pair.base;
        fTmp = ppip->damage.from;

        // set some values
        ppip->vel_hrz_pair.base     = pstate->x;
        ppip->spacing_hrz_pair.base = pstate->y;
        ppip->damage.from           = FP8_TO_FLOAT( pstate->argument );
        ppip->damage.to             = ppip->damage.from + damage_rand;

        spawn_poof( pself->index, pchr->profile_ref );

        // Restore the saved values
        ppip->vel_hrz_pair.base     = iTmp;
        ppip->spacing_hrz_pair.base = tTmp;
        ppip->damage.from           = fTmp;
        ppip->damage.to             = ppip->damage.from + damage_rand;

        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_GoodTeamExperience( script_state_t * pstate, ai_state_t * pself )
{
    // GiveExperienceToGoodTeam(  tmpargument = "amount", tmpdistance = "type" )
    /// @author ZZ
    /// @details This function gives experience to everyone on the G Team

    SCRIPT_FUNCTION_BEGIN();

    if(pstate->distance < XP_COUNT)
    {
        give_team_experience(static_cast<TEAM_REF>(TEAM_GOOD), pstate->argument, static_cast<XPType>(pstate->distance) );
    }


    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoNothing( script_state_t * pstate, ai_state_t * pself )
{
    // DoNothing()
    /// @author ZF
    /// @details This function does nothing
    /// Use this for debugging or in combination with a Else function

    return true;
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GrogTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GrogTarget( tmpargument = "amount" )
    /// @author ZF
    /// @details This function grogs the Target for a duration equal to tmpargument

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = false;
    if ( pself_target->getProfile()->canBeGrogged() )
    {
        int timer_val = pself_target->grog_timer + pstate->argument;
        pself_target->grog_timer = std::max( 0, timer_val );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DazeTarget( script_state_t * pstate, ai_state_t * pself )
{
    // DazeTarget( tmpargument = "amount" )
    /// @author ZF
    /// @details This function dazes the Target for a duration equal to tmpargument

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    // Characters who manage to daze themselves are to ignore their daze immunity
    returncode = false;
    if ( pself_target->getProfile()->canBeDazed() || pself->index == pself->target )
    {
        int timer_val = pself_target->daze_timer + pstate->argument;
        pself_target->daze_timer = std::max( 0, timer_val );

        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableRespawn( script_state_t * pstate, ai_state_t * pself )
{
    // EnableRespawn()
    /// @author ZF
    /// @details This function turns respawn with JUMP button on

    SCRIPT_FUNCTION_BEGIN();

    PMod->setRespawnValid(true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableRespawn( script_state_t * pstate, ai_state_t * pself )
{
    // DisableRespawn()
    /// @author ZF
    /// @details This function turns respawn with JUMP button off

    SCRIPT_FUNCTION_BEGIN();

    PMod->setRespawnValid(false);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HolderBlocked( script_state_t * pstate, ai_state_t * pself )
{
    // IfHolderBlocked()
    /// @author ZF
    /// @details This function passes if the holder blocked an attack

    CHR_REF iattached;

    SCRIPT_FUNCTION_BEGIN();

    iattached = pchr->attachedto;

    if ( _gameObjects.exists( iattached ) )
    {
        BIT_FIELD bits = _gameObjects.get(iattached)->ai.alert;

        if ( HAS_SOME_BITS( bits, ALERTIF_BLOCKED ) )
        {
            CHR_REF iattacked = _gameObjects.get(iattached)->ai.attacklast;

            if ( _gameObjects.exists( iattacked ) )
            {
                SET_TARGET_0( iattacked );
            }
            else
            {
                returncode = false;
            }
        }
        else
        {
            returncode = false;
        }
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasNotFullMana( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetHasNotFullMana()
    /// @author ZF
    /// @details This function passes only if the Target is not at max mana and alive

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !pself_target->alive || pself_target->mana > pself_target->mana_max - HURTDAMAGE )
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableListenSkill( script_state_t * pstate, ai_state_t * pself )
{
    // EnableListenSkill()
    /// @author ZF
    /// @details This function increases range from which sound can be heard by 33%

    SCRIPT_FUNCTION_BEGIN();

    {
        mad_t * pmad = chr_get_pmad( pself->index );

        log_warning( "Depacrated script function used: EnableListenSkill! (%s)\n",
                     ( NULL == pmad ) ? "UNKNOWN" : pmad->name );
    }

    returncode = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLastItemUsed( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToLastItemUsed()
    /// @author ZF
    /// @details This sets the Target to the last item the character used

    SCRIPT_FUNCTION_BEGIN();

    if ( pself->lastitemused != pself->index && _gameObjects.exists( pself->lastitemused ) )
    {
        SET_TARGET_0( pself->lastitemused );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FollowLink( script_state_t * pstate, ai_state_t * pself )
{
    // FollowLink( tmpargument = "index of next module name" )
    /// @author BB
    /// @details Skips to the next module!

    SCRIPT_FUNCTION_BEGIN();

    if ( !ppro->isValidMessageID(pstate->argument) ) return false;

    returncode = link_follow_modname( ppro->getMessage(pstate->argument).c_str(), true );
    if ( !returncode )
    {
        DisplayMsg_printf( "That's too scary for %s", pchr->Name );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OperatorIsLinux( script_state_t * pstate, ai_state_t * pself )
{
    // IfOperatorIsLinux()
    /// @author ZF
    /// @details Proceeds if running on linux

    SCRIPT_FUNCTION_BEGIN();

#if defined(ID_LINUX)
    returncode = true;
#else
    returncode = false;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAWeapon( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsAWeapon()
    /// @author ZF
    /// @details Proceeds if the AI Target Is a melee or ranged weapon

    SCRIPT_FUNCTION_BEGIN();

    if ( !_gameObjects.exists( pself->target ) ) return false;

    returncode = _gameObjects[pself->target]->getProfile()->isRangedWeapon() || chr_has_idsz(pself->target, MAKE_IDSZ('X', 'W', 'E', 'P'));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SomeoneIsStealing( script_state_t * pstate, ai_state_t * pself )
{
    // IfSomeoneIsStealing()
    /// @author ZF
    /// @details This function passes if someone stealed from it's shop

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->order_value == Passage::SHOP_STOLEN && pself->order_counter == Passage::SHOP_THEFT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsASpell( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsASpell()
    /// @author ZF
    /// @details roceeds if the AI Target has any particle with the [IDAM] or [WDAM] expansion

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    for (LocalParticleProfileRef iTmp(0); iTmp.get() < MAX_PIP_PER_PROFILE; ++iTmp)
    {
        pip_t * ppip = ProfileSystem::get().pro_get_ppip(pchr->profile_ref, iTmp);
        if ( NULL == ppip ) continue;

        if ( ppip->damageBoni._intelligence || ppip->damageBoni._wisdom )
        {
            returncode = true;
            break;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Backstabbed( script_state_t * pstate, ai_state_t * pself )
{
    // IfBackstabbed()
    /// @author ZF
    /// @details Proceeds if HitFromBehind, target has [STAB] skill and damage dealt is physical
    /// automatically fails if attacker has a code of conduct

    SCRIPT_FUNCTION_BEGIN();

    //Now check if it really was backstabbed
    returncode = false;
    if ( HAS_SOME_BITS( pself->alert, ALERTIF_ATTACKED ) )
    {
        //Who is the dirty backstabber?
        Object * pattacker = _gameObjects.get( pself->attacklast );
        if ( !ACTIVE_PCHR( pattacker ) ) return false;

        //Only if hit from behind
        if ( pself->directionlast >= ATK_BEHIND - 8192 && pself->directionlast < ATK_BEHIND + 8192 )
        {
            //And require the backstab skill
            if ( chr_get_skill( pattacker, MAKE_IDSZ( 'S', 'T', 'A', 'B' ) ) )
            {
                //Finally we require it to be physical damage!
                if (DamageType_isPhysical(pself->damagetypelast))
                {
                    returncode = true;
                }
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetDamageType( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = GetTargetDamageType()
    /// @author ZF
    /// @details This function gets the last type of damage for the Target

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pstate->argument = pself_target->ai.damagetypelast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddQuest( script_state_t * pstate, ai_state_t * pself )
{
    // AddQuest( tmpargument = "quest idsz" )
    /// @author ZF
    /// @details This function adds a quest idsz set in tmpargument into the targets quest.txt to 0

    egolib_rv result = rv_fail;
    Object * pself_target;
    PLA_REF ipla;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ipla = pself_target->is_which_player;
    if ( VALID_PLA( ipla ) )
    {
        player_t * ppla = PlaStack.get_ptr( ipla );

        result = quest_log_add( ppla->quest_log, SDL_arraysize( ppla->quest_log ), pstate->argument, pstate->distance );
    }

    returncode = ( rv_success == result );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BeatQuestAllPlayers( script_state_t * pstate, ai_state_t * pself )
{
    // BeatQuestAllPlayers()
    /// @author ZF
    /// @details This function marks a IDSZ in the targets quest.txt as beaten
    ///               returns true if at least one quest got marked as beaten.

    PLA_REF ipla;

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF ichr;
        player_t * ppla = PlaStack.get_ptr( ipla );

        if ( !ppla->valid ) continue;

        ichr = ppla->index;
        if ( !_gameObjects.exists( ichr ) ) continue;

        if ( QUEST_BEATEN == quest_log_adjust_level( ppla->quest_log, SDL_arraysize( ppla->quest_log ), ( IDSZ )pstate->argument, QUEST_MAXVAL ) )
        {
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasQuest( script_state_t * pstate, ai_state_t * pself )
{
    // tmpdistance = IfTargetHasQuest( tmpargument = "quest idsz )
    /// @author ZF
    /// @details This function proceeds if the Target has the unfinIshed quest specified in tmpargument
    /// and sets tmpdistance to the Quest Level of the specified quest.

    int     quest_level = QUEST_NONE;
    Object * pself_target = NULL;
    PLA_REF ipla;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = false;

    ipla = pself_target->is_which_player;
    if ( VALID_PLA( ipla ) )
    {
        player_t * ppla = PlaStack.get_ptr( ipla );

        quest_level = quest_log_get_level( ppla->quest_log, SDL_arraysize( ppla->quest_log ), pstate->argument );
    }

    // only find active quests
    if ( quest_level >= 0 )
    {
        pstate->distance = quest_level;
        returncode       = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_QuestLevel( script_state_t * pstate, ai_state_t * pself )
{
    // SetQuestLevel( tmpargument = "idsz", distance = "adjustment" )
    /// @author ZF
    /// @details This function modifies the quest level for a specific quest IDSZ
    /// tmpargument specifies quest idsz (tmpargument) and the adjustment (tmpdistance, which may be negative)

    Object * pself_target;
    PLA_REF ipla;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = false;
    ipla = pself_target->is_which_player;
    if ( VALID_PLA( ipla ) && 0 != pstate->distance )
    {
        int        quest_level = QUEST_NONE;
        player_t * ppla        = PlaStack.get_ptr( ipla );

        quest_level = quest_log_adjust_level( ppla->quest_log, SDL_arraysize( ppla->quest_log ), pstate->argument, pstate->distance );

        returncode = QUEST_NONE != quest_level;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddQuestAllPlayers( script_state_t * pstate, ai_state_t * pself )
{
    // AddQuestAllPlayers( tmpargument = "quest idsz" )
    /// @author ZF
    /// @details This function adds a quest idsz set in tmpargument into all local player's quest logs
    /// The quest level Is set to tmpdistance if the level Is not already higher

    PLA_REF ipla;
    int success_count, player_count;

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    for ( player_count = 0, success_count = 0, ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        int quest_level;
        player_t * ppla = PlaStack.get_ptr( ipla );

        if ( !ppla->valid || !_gameObjects.exists( ppla->index ) ) continue;
        player_count++;

        // Try to add it or replace it if this one is higher
        quest_level = quest_log_add( ppla->quest_log, SDL_arraysize( ppla->quest_log ), pstate->argument, pstate->distance );
        if ( QUEST_NONE != quest_level ) success_count++;
    }

    returncode = ( player_count > 0 ) && ( success_count >= player_count );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddBlipAllEnemies( script_state_t * pstate, ai_state_t * pself )
{
    // AddBlipAllEnemies()
    /// @author ZF
    /// @details show all enemies on the minimap who match the IDSZ given in tmpargument
    /// it show only the enemies of the AI Target

    SCRIPT_FUNCTION_BEGIN();

    if ( _gameObjects.exists( pself->target ) )
    {
        local_stats.sense_enemies_team = chr_get_iteam( pself->target );
        local_stats.sense_enemies_idsz = pstate->argument;
    }
    else
    {
        local_stats.sense_enemies_team = ( TEAM_REF )TEAM_MAX;
        local_stats.sense_enemies_idsz = IDSZ_NONE;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PitsFall( script_state_t * pstate, ai_state_t * pself )
{
    // PitsFall( tmpx = "teleprt x", tmpy = "teleprt y", tmpdistance = "teleprt z" )
    /// @author ZF
    /// @details This function activates pit teleportation.

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->x > EDGE && pstate->y > EDGE && pstate->x < PMesh->gmem.edge_x - EDGE && pstate->y < PMesh->gmem.edge_y - EDGE )
    {
        pits.teleport = true;
        pits.teleport_pos[kX] = pstate->x;
        pits.teleport_pos[kY] = pstate->y;
        pits.teleport_pos[kZ] = pstate->distance;
    }
    else
    {
        pits.kill = true;          //make it kill instead
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOwner( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsOwner()
    /// @author ZF
    /// @details This function proceeds only if the Target is the character's owner

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->alive && pself->owner == pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedCharacter( tmpargument = "profile", tmpx = "x", tmpy = "y", tmpdistance = "z" )

    /// @author ZF
    /// @details This function spawns a character defined in tmpargument to the characters AI target using
    /// the slot specified in tmpdistance (LEFT, RIGHT or INVENTORY). Fails if the inventory or
    /// grip specified is full or already in use.
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS,
    /// AS THE MODEL SLOTS MAY VARY FROM MODULE TO MODULE.
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    fvec3_t pos = fvec3_t(pstate->x, pstate->y, pstate->distance);

    CHR_REF ichr = spawn_one_character(pos, (PRO_REF)pstate->argument, pchr->team, 0, FACE_NORTH, NULL, INVALID_CHR_REF);
    returncode = _gameObjects.exists( ichr );

    if ( !returncode )
    {
        if ( ichr > PMod->getImportAmount() * MAX_IMPORT_PER_PLAYER )
        {
            log_warning("Object \"%s\"(\"%s\") failed to spawn profile index %d\n", pchr->Name, ProfileSystem::get().getProfile(pchr->profile_ref)->getClassName().c_str(), pstate->argument);
        }
    }
    else
    {
        Object * pchild = _gameObjects.get( ichr );

        Uint8 grip = CLIP( pstate->distance, (int)ATTACH_INVENTORY, (int)ATTACH_RIGHT );

        if ( grip == ATTACH_INVENTORY )
        {
            // Inventory character
            if ( Inventory::add_item( pself->target, ichr, MAXINVENTORY, true ) )
            {
                SET_BIT( pchild->ai.alert, ALERTIF_GRABBED );  // Make spellbooks change
                pchild->attachedto = pself->target;  // Make grab work
                scr_run_chr_script( ichr );  // Empty the grabbed messages

                pchild->attachedto = INVALID_CHR_REF;  // Fix grab

                //Set some AI values
                pself->child = ichr;
                pchild->ai.passage = pself->passage;
                pchild->ai.owner   = pself->owner;
            }

            //No more room!
            else
            {
                _gameObjects.remove( ichr );
                ichr = INVALID_CHR_REF;
            }
        }
        else if ( grip == ATTACH_LEFT || grip == ATTACH_RIGHT )
        {
            if ( !_gameObjects.exists( pself_target->holdingwhich[grip] ) )
            {
                // Wielded character
                grip_offset_t grip_off = ( ATTACH_LEFT == grip ) ? GRIP_LEFT : GRIP_RIGHT;

                if ( rv_success == attach_character_to_mount( ichr, pself->target, grip_off ) )
                {
                    // Handle the "grabbed" messages
                    scr_run_chr_script( ichr );
                }

                //Set some AI values
                pself->child = ichr;
                pchild->ai.passage = pself->passage;
                pchild->ai.owner   = pself->owner;
            }

            //Grip is already used
            else
            {
                _gameObjects.remove( ichr );
                ichr = INVALID_CHR_REF;
            }
        }
        else
        {
            // we have been given an invalid attachment point.
            // still allow the character to spawn if it is not in an invalid area

            // technically this should never occur since we are limiting the attachment points above
            if (!chr_get_safe(pchild))
            {
                _gameObjects.remove( ichr );
                ichr = INVALID_CHR_REF;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToChild( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToChild()
    /// @author ZF
    /// @details This function sets the target to the character it spawned last (also called it's "child")

    SCRIPT_FUNCTION_BEGIN();

    if ( _gameObjects.exists( pself->child ) )
    {
        SET_TARGET_0( pself->child );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageThreshold( script_state_t * pstate, ai_state_t * pself )
{
    // SetDamageThreshold()
    /// @author ZF
    /// @details This sets the damage treshold for this character. Damage below the threshold is ignored

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->argument > 0 ) pchr->damage_threshold = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_End( script_state_t * pstate, ai_state_t * pself )
{
    // End()
    /// @author ZZ
    /// @details This Is the last function in a script

    SCRIPT_FUNCTION_BEGIN();

    pself->terminate = true;
    returncode       = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TakePicture( script_state_t * pstate, ai_state_t * pself )
{
    // TakePicture()
    /// @author ZF
    /// @details This function proceeds only if the screenshot was successful

    SCRIPT_FUNCTION_BEGIN();

    returncode = dump_screenshot();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Speech( script_state_t * pstate, ai_state_t * pself )
{
    // SetSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets all of the RTS speech registers to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    //ZF> no longer supported
#if 0
    Uint16 sTmp = 0;
    for ( sTmp = SPEECH_BEGIN; sTmp <= SPEECH_END; sTmp++ )
    {
        pchr->sound_index[sTmp] = pstate->argument;
    }
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_MoveSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetMoveSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS move speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_MOVE] = pstate->argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SecondMoveSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetSecondMoveSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS movealt speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();
    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_MOVEALT] = pstate->argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_AttackSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetAttacksSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS attack speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();
    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_ATTACK] = pstate->argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_AssistSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetAssistSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS assist speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();
    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_ASSIST] = pstate->argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TerrainSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetTerrainSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS terrain speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();
    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_TERRAIN] = pstate->argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SelectSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // SetSelectSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS select speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();
    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_SELECT] = pstate->argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OperatorIsMacintosh( script_state_t * pstate, ai_state_t * pself )
{
    // IfOperatorIsMacintosh()
    /// @author ZF
    /// @details Proceeds if the current running OS is mac

    SCRIPT_FUNCTION_BEGIN();

#if defined(ID_OSX)
    returncode = true;
#else
    returncode = false;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ModuleHasIDSZ( script_state_t * pstate, ai_state_t * pself )
{
    // IfModuleHasIDSZ( tmpargument = "message number with module name", tmpdistance = "idsz" )

    /// @author ZF
    /// @details Proceeds if the specified module has the required IDSZ specified in tmpdistance
    /// The module folder name to be checked is a string from message.txt

    SCRIPT_FUNCTION_BEGIN();

    ///use message.txt to send the module name
    if ( !ppro->isValidMessageID(pstate->argument) ) return false;

    STRING buffer;
    strncpy(buffer, ppro->getMessage(pstate->argument).c_str(), SDL_arraysize(buffer));

    returncode = ModuleProfile::moduleHasIDSZ( PMod->getName().c_str(), pstate->distance, 0, buffer);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MorphToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // MorphToTarget()
    /// @author ZF
    /// @details This morphs the character into the target
    /// Also set size and keeps the previous AI type

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !_gameObjects.exists( pself->target ) ) return false;

    change_character( pself->index, pself_target->basemodel_ref, pself_target->skin, ENC_LEAVE_ALL );

    // let the resizing take some time
    pchr->fat_goto      = pself_target->fat;
    pchr->fat_goto_time = SIZETIME;

    // change back to our original AI
//    pself->type      = ProList.lst[pchr->basemodel_ref].iai;      //TODO: this no longer works (is it even needed?)

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetManaFlow( script_state_t * pstate, ai_state_t * pself )
{
    // GiveManaFlowToTarget()
    /// @author ZF
    /// @details Permanently boost the target's mana flow

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd_int( 0, pself_target->mana_flow, PERFECTSTAT, &iTmp );
        pself_target->mana_flow += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetManaReturn( script_state_t * pstate, ai_state_t * pself )
{
    // GiveManaReturnToTarget()
    /// @author ZF
    /// @details Permanently boost the target's mana return

    int iTmp;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd_int( 0, pself_target->mana_return, PERFECTSTAT, &iTmp );
        pself_target->mana_return += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Money( script_state_t * pstate, ai_state_t * pself )
{
    // SetMoney()
    /// @author ZF
    /// @details Permanently sets the money for the character to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->money = CLIP( pstate->argument, 0, MAXMONEY );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanSeeKurses( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetCanSeeKurses()
    /// @author ZF
    /// @details Proceeds if the target can see kursed stuff.

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->see_kurse_level > 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DispelTargetEnchantID( script_state_t * pstate, ai_state_t * pself )
{
    // DispelEnchantID( tmpargument = "idsz" )
    /// @author ZF
    /// @details This function removes all enchants from the target who match the specified RemovedByIDSZ

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = false;
    if ( pself_target->alive )
    {
        // Check all enchants to see if they are removed
        returncode = remove_all_enchants_with_idsz( pself->target, pstate->argument );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KurseTarget( script_state_t * pstate, ai_state_t * pself )
{
    // KurseTarget()
    /// @author ZF
    /// @details This makes the target kursed

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = false;
    if ( pself_target->isitem && !pself_target->iskursed )
    {
        pself_target->iskursed = true;
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildContent( script_state_t * pstate, ai_state_t * pself )
{
    // SetChildContent( tmpargument = "content" )
    /// @author ZF
    /// @details This function lets a character set the content of the last character it
    /// spawned last

    SCRIPT_FUNCTION_BEGIN();

    _gameObjects.get(pself->child)->ai.content = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateTargetUp( script_state_t * pstate, ai_state_t * pself )
{
    // AccelerateTargetUp( tmpargument = "acc z" )
    /// @author ZF
    /// @details This function makes the target accelerate up and down

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->vel[kZ] += pstate->argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetAmmo( tmpargument = "ammo" )
    /// @author ZF
    /// @details This function sets the ammo of the character's current AI target

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->ammo = std::min( pstate->argument, (int)pself_target->ammomax );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableInvictus( script_state_t * pstate, ai_state_t * pself )
{
    // EnableInvictus()
    /// @author ZF
    /// @details This function makes the character invulerable

    SCRIPT_FUNCTION_BEGIN();

    pchr->invictus = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableInvictus( script_state_t * pstate, ai_state_t * pself )
{
    // DisableInvictus()
    /// @author ZF
    /// @details This function makes the character not invulerable

    SCRIPT_FUNCTION_BEGIN();

    pchr->invictus = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDamageSelf( script_state_t * pstate, ai_state_t * pself )
{
    // TargetDamageSelf( tmpargument = "damage" )
    /// @author ZF
    /// @details This function applies little bit of hate from the character's target to
    /// the character itself. The amount is set in tmpargument

    IPair tmp_damage;

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _gameObjects[pself->target];
    if(!target) {
        return false;
    }

    tmp_damage.base = pstate->argument;
    tmp_damage.rand = 1;

    pchr->damage(ATK_FRONT, tmp_damage, static_cast<DamageType>(pstate->distance), target->getTeam(), target, DAMFX_NBLOC, true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetSize( script_state_t * pstate, ai_state_t * pself )
{
    // SetSize( tmpargument = "percent" )
    /// @author ZF
    /// @details This changes the AI target's size

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->fat_goto *= pstate->argument / 100.0f;
    pself_target->fat_goto_time += SIZETIME;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DrawBillboard( script_state_t * pstate, ai_state_t * pself )
{
    // DrawBillboard( tmpargument = "message", tmpdistance = "duration", tmpturn = "color" )
    /// @author ZF
    /// @details This function draws one of those billboards above the character

    const auto text_color = Ego::Math::Colour4f::parse(0xFF, 0xFF, 0xFF, 0xFF);

    //List of avalible colours
    const auto tint_red  = Ego::Math::Colour4f{ 1.00f, 0.25f, 0.25f, 1.00f };
    const auto tint_purple = Ego::Math::Colour4f{ 0.88f, 0.75f, 1.00f, 1.00f };
    const auto tint_white = Ego::Math::Colour4f{ 1.00f, 1.00f, 1.00f, 1.00f };
    const auto tint_yellow = Ego::Math::Colour4f{ 1.00f, 1.00f, 0.75f, 1.00f };
    const auto tint_green = Ego::Math::Colour4f{ 0.25f, 1.00f, 0.25f, 1.00f };
    const auto tint_blue = Ego::Math::Colour4f{ 0.25f, 0.25f, 1.00f, 1.00f };

    SCRIPT_FUNCTION_BEGIN();

    if ( !ppro->isValidMessageID(pstate->argument) ) return false;

    auto* tint = &tint_white;
    //Figure out which color to use
    switch ( pstate->turn )
    {
        default:
        case COLOR_WHITE:   tint = &tint_white;   break;
        case COLOR_RED:     tint = &tint_red;     break;
        case COLOR_PURPLE:  tint = &tint_purple;  break;
        case COLOR_YELLOW:  tint = &tint_yellow;  break;
        case COLOR_GREEN:   tint = &tint_green;   break;
        case COLOR_BLUE:    tint = &tint_blue;    break;
    }

    returncode = NULL != chr_make_text_billboard(pself->index, ppro->getMessage(pstate->argument).c_str(), text_color, *tint, pstate->distance, Billboard::Flags::Fade);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToBlahInPassage( script_state_t * pstate, ai_state_t * pself )
{
    // SetTargetToBlahInPassage()
    /// @author ZF
    /// @details This function sets the target to whatever object with the specified bits
    /// in tmpdistance is blocking the given passage. This function lets passage rectangles be used as event triggers

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = PMod->getPassageByID(pstate->argument);
    returncode = false;
    if(passage) {
        CHR_REF ichr = passage->whoIsBlockingPassage(pself->index, pstate->turn, TARGET_SELF | pstate->distance, IDSZ_NONE );

        if ( _gameObjects.exists( ichr ) )
        {
            SET_TARGET_0( ichr );
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsFacingSelf( script_state_t * pstate, ai_state_t * pself )
{
    // IfTargetIsFacingSelf()
    /// @author ZF
    /// @details This function proceeds if the target is more or less facing the character
    FACING_T sTmp = 0;
    Object *  pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    sTmp = vec_to_facing( pchr->getPosX() - pself_target->getPosX() , pchr->getPosY() - pself_target->getPosY() );
    sTmp -= pself_target->ori.facing_z;
    returncode = ( sTmp > 55535 || sTmp < 10000 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_LevelUp( script_state_t * pstate, ai_state_t * pself )
{
    // IfLevelUp()
    /// @author ZF
    /// @details This function proceeds if the character gained a new level this update
    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_LEVELUP );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_add_TargetSkill( script_state_t * pstate, ai_state_t * pself )
{
    // GiveSkillToTarget( tmpargument = "skill_IDSZ", tmpdistance = "skill_level" )
    /// @author ZF
    /// @details This function permanently gives the target character a skill

    Object *ptarget;
    egolib_rv rv;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( ptarget );

    rv = idsz_map_add( ptarget->skills, SDL_arraysize( ptarget->skills ), pstate->argument, pstate->distance );

    returncode = ( rv_success == rv );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearbyMeleeWeapon( script_state_t * pstate, ai_state_t * pself )
{
    CHR_REF best_target;

    SCRIPT_FUNCTION_BEGIN();

    best_target = FindWeapon( pchr, WIDE, MAKE_IDSZ( 'X', 'W', 'E', 'P' ), false, true );

    //Did we find anything good?
    if ( _gameObjects.exists( best_target ) )
    {
        pself->target = best_target;
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}
