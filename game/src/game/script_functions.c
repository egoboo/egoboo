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

#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/script_functions.h"
#include "game/script_implementation.h"
#include "game/GameStates/PlayingState.hpp"
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
#include "game/Physics/PhysicalConstants.hpp"
#include "game/Physics/ObjectPhysics.h"

#include "game/GUI/MiniMap.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// turn off this annoying warning
#if defined _MSC_VER
#    pragma warning(disable : 4189) // local variable is initialized but not referenced
#endif

#define SCRIPT_FUNCTION_BEGIN() \
    Uint8 returncode = true; \
    if( !_currentModule->getObjectHandler().exists(self.index) ) return false; \
    Object *pchr = _currentModule->getObjectHandler().get( self.index ); \
    const std::shared_ptr<ObjectProfile> &ppro = pchr->getProfile(); \
    if(!ppro) return false;

#define SCRIPT_FUNCTION_END() \
    return returncode;

#define FUNCTION_BEGIN() \
    Uint8 returncode = true; \
    if( nullptr == ( pchr ) ) return false;

#define FUNCTION_END() \
    return returncode;

#define SET_TARGET_0(ITARGET)         self.target = ITARGET.get();
#define SET_TARGET_1(ITARGET,PTARGET) if( NULL != PTARGET ) { PTARGET = _currentModule->getObjectHandler().get(ITARGET); }
#define SET_TARGET(ITARGET,PTARGET)   SET_TARGET_0( ITARGET ); SET_TARGET_1(ITARGET,PTARGET)

#define SCRIPT_REQUIRE_TARGET(PTARGET) \
    if( !_currentModule->getObjectHandler().exists(self.target) ) return false; \
    PTARGET = _currentModule->getObjectHandler().get( self.target );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// @defgroup _bitwise_functions_ Bitwise Scripting Functions
/// @details These functions may be necessary to export the bitwise functions for handling alerts to
///  scripting languages where there is no support for bitwise operators (Lua, tcl, ...)

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_SetAlertBit( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Sets the bit in the 32-bit integer self.alert indexed by state.argument

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( state.argument >= 0 && state.argument < 32 )
    {
        SET_BIT( self.alert, 1 << state.argument );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearAlertBit( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Clears the bit in the 32-bit integer self.alert indexed by state.argument

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( state.argument >= 0 && state.argument < 32 )
    {
        UNSET_BIT( self.alert, 1 << state.argument );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestAlertBit( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Tests to see if the the bit in the 32-bit integer self.alert indexed by state.argument is non-zero

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( state.argument >= 0 && state.argument < 32 )
    {
        returncode = HAS_SOME_BITS( self.alert,  1 << state.argument );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_SetAlert( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Sets one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    SET_BIT( self.alert, state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearAlert( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Clears one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    UNSET_BIT( self.alert, state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestAlert( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Tests one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_SetBit( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Sets the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( state.y >= 0 && state.y < 32 )
    {
        SET_BIT( state.x, 1 << state.y );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearBit( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Clears the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( state.y >= 0 && state.y < 32 )
    {
        UNSET_BIT( state.x, 1 << state.y );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestBit( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Tests the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( state.y >= 0 && state.y < 32 )
    {
        returncode = HAS_SOME_BITS( state.x, 1 << state.y );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_SetBits( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Adds the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    SET_BIT( state.x, state.y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearBits( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Clears the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    UNSET_BIT( state.x, state.y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestBits( script_state_t& state, ai_state_t& self )
{
    /// @author BB
    /// @details Tests the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( state.x, state.y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 scr_IfSpawned( script_state_t& state, ai_state_t& self )
{
    // IfSpawned()
    /// @author ZZ
    /// @details This function proceeds if the character was spawned this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if it's a new character
    returncode = HAS_SOME_BITS( self.alert, ALERTIF_SPAWNED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTimeOut( script_state_t& state, ai_state_t& self )
{
    // IfTimeOut()
    /// @author ZZ
    /// @details This function proceeds if the character's aitime is 0.  Use
    /// in conjunction with set_Time

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if time alert is set
    returncode = ( update_wld > self.timer );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfAtWaypoint( script_state_t& state, ai_state_t& self )
{
    // IfAtWaypoint()
    /// @author ZZ
    /// @details This function proceeds if the character reached its waypoint this
    /// update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character reached a waypoint
    returncode = HAS_SOME_BITS( self.alert, ALERTIF_ATWAYPOINT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfAtLastWaypoint( script_state_t& state, ai_state_t& self )
{
    // IfAtLastWaypoint()
    /// @author ZZ
    /// @details This function proceeds if the character reached its last waypoint this
    /// update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character reached its last waypoint
    returncode = HAS_SOME_BITS( self.alert, ALERTIF_ATLASTWAYPOINT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfAttacked( script_state_t& state, ai_state_t& self )
{
    // IfAttacked()
    /// @author ZZ
    /// @details This function proceeds if the character ( an item ) was put in its
    /// owner's pocket this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was damaged
    returncode = HAS_SOME_BITS( self.alert, ALERTIF_ATTACKED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfBumped( script_state_t& state, ai_state_t& self )
{
    // IfBumped()
    /// @author ZZ
    /// @details This function proceeds if the character was bumped by another character
    /// this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was bumped
    returncode = HAS_SOME_BITS( self.alert, ALERTIF_BUMPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfOrdered( script_state_t& state, ai_state_t& self )
{
    // IfOrdered()
    /// @author ZZ
    /// @details This function proceeds if the character got an order from another
    /// character on its team this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was ordered
    returncode = HAS_SOME_BITS( self.alert, ALERTIF_ORDERED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfCalledForHelp( script_state_t& state, ai_state_t& self )
{
    // IfCalledForHelp()
    /// @author ZZ
    /// @details This function proceeds if one of the character's teammates was nearly
    /// killed this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was called for help
    returncode = HAS_SOME_BITS( self.alert, ALERTIF_CALLEDFORHELP );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetContent( script_state_t& state, ai_state_t& self )
{
    // SetContent( tmpargument )
    /// @author ZZ
    /// @details This function sets the content variable.  Used in conjunction with
    /// GetContent.  Content is preserved from update to update

    SCRIPT_FUNCTION_BEGIN();

    // Set the content
    self.content = state.argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfKilled( script_state_t& state, ai_state_t& self )
{
    // IfKilled()
    /// @author ZZ
    /// @details This function proceeds if the character was killed this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character's been killed
    returncode = HAS_SOME_BITS( self.alert, ALERTIF_KILLED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetKilled( script_state_t& state, ai_state_t& self )
{
    // IfTargetKilled()
    /// @author ZZ
    /// @details This function proceeds if the character's target from last update was
    /// killed during this update

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    // Proceed only if the character's target has just died or is already dead
    returncode = ( HAS_SOME_BITS( self.alert, ALERTIF_TARGETKILLED ) || !pself_target->isAlive() );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearWaypoints( script_state_t& state, ai_state_t& self )
{
    // ClearWaypoints()
    /// @author ZZ
    /// @details This function is used to move a character around.  Do this before
    /// AddWaypoint

    SCRIPT_FUNCTION_BEGIN();

	returncode = true;
	waypoint_list_clear(self.wp_lst);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddWaypoint( script_state_t& state, ai_state_t& self )
{
    // AddWaypoint( tmpx = "x position", tmpy = "y position" )
    /// @author ZZ
    /// @details This function tells the character where to move next

    SCRIPT_FUNCTION_BEGIN();

    returncode = AddWaypoint( self.wp_lst, self.index, state.x, state.y );

    if ( returncode )
    {
        // make sure we update the waypoint, since the list changed
        ai_state_t::get_wp( self );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FindPath( script_state_t& state, ai_state_t& self )
{
    // FindPath
    /// @author ZF
    /// @details Ported the A* path finding algorithm by birdsey and heavily modified it
    /// This function adds enough waypoints to get from one point to another

    bool used_astar;

    SCRIPT_FUNCTION_BEGIN();

    //Too soon since last try?
    if ( self.astar_timer > update_wld ) return false;

    returncode = FindPath( self.wp_lst, pchr, state.x, state.y, &used_astar );

    if ( used_astar )
    {
        // limit the rate of AStar calculations to be once every half second.
        self.astar_timer = update_wld + ( ONESECOND / 2 );
    }

    //Make sure the waypoint list is updated
	ai_state_t::get_wp( self );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Compass( script_state_t& state, ai_state_t& self )
{
    // Compass( tmpturn = "rotation", tmpdistance = "radius" )
    /// @author ZZ
    /// @details This function modifies tmpx and tmpy, depending on the setting of
    /// tmpdistance and tmpturn.  It acts like one of those Compass thing
    /// with the two little needle legs

	Vector2f loc_pos;

    SCRIPT_FUNCTION_BEGIN();

    loc_pos[XX] = state.x;
    loc_pos[YY] = state.y;

    returncode = Compass( loc_pos, state.turn, state.distance );

    // update the position
    if ( returncode )
    {
        state.x = loc_pos[XX];
        state.y = loc_pos[YY];
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetTargetArmorPrice( script_state_t& state, ai_state_t& self )
{
    // tmpx = GetTargetArmorPrice( tmpargument = "skin" )
    /// @author ZZ
    /// @details This function returns the cost of the desired skin upgrade, setting
    /// tmpx to the price

    Object *ptarget;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( ptarget );

    int value = ptarget->getProfile()->getSkinInfo(state.argument).cost;

    if ( value > 0 )
    {
        state.x  = value;
        returncode = true;
    }
    else
    {
        state.x  = 0;
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTime( script_state_t& state, ai_state_t& self )
{
    // SetTime( tmpargument = "time" )
    /// @author ZZ
    /// @details This function sets the character's ai timer.  50 clicks per second.
    /// Used in conjunction with IfTimeOut

    SCRIPT_FUNCTION_BEGIN();

    self.timer = UpdateTime( self.timer, state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetContent( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetContent()
    /// @author ZZ
    /// @details This function sets tmpargument to the character's content variable.
    /// Used in conjunction with set_Content, or as a NOP to space out an Else

    SCRIPT_FUNCTION_BEGIN();

    // Get the content
    state.argument = self.content;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinTargetTeam( script_state_t& state, ai_state_t& self )
{
    // JoinTargetTeam()
    /// @author ZZ
    /// @details This function lets a character join a different team.  Used
    /// mostly for pets

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = false;
    if ( _currentModule->getObjectHandler().exists( self.target ) )
    {
        pchr->setTeam(pself_target->team);
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToNearbyEnemy( script_state_t& state, ai_state_t& self )
{
    // SetTargetToNearbyEnemy()
    /// @author ZZ
    /// @details This function sets the target to a nearby enemy, failing if there are none

    SCRIPT_FUNCTION_BEGIN();

    auto ichr = chr_find_target(pchr, NEARBY, IDSZ_NONE, TARGET_ENEMIES);

    if ( _currentModule->getObjectHandler().exists(ichr) )
    {
        self.target = ichr.get();
        returncode = true;
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToTargetLeftHand( script_state_t& state, ai_state_t& self )
{
    // SetTargetToTargetLeftHand()
    /// @author ZZ
    /// @details This function sets the target to the item in the target's left hand,
    /// failing if the target has no left hand item

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    auto ichr = ObjectRef(pself_target->holdingwhich[SLOT_LEFT]);
    returncode = false;
    if ( _currentModule->getObjectHandler().exists( ichr ) )
    {
        SET_TARGET( ichr, pself_target );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToTargetRightHand( script_state_t& state, ai_state_t& self )
{
    // SetTargetToTargetRightHand()
    /// @author ZZ
    /// @details This function sets the target to the item in the target's right hand,
    /// failing if the target has no right hand item

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    auto ichr = ObjectRef(pself_target->holdingwhich[SLOT_RIGHT]);
    returncode = false;
    if ( _currentModule->getObjectHandler().exists( ichr ) )
    {
        SET_TARGET( ichr, pself_target );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToWhoeverAttacked( script_state_t& state, ai_state_t& self )
{
    // SetTargetToWhoeverAttacked()
    /// @author ZZ
    /// @details This function sets the target to whoever attacked the character last, failing for damage tiles

    SCRIPT_FUNCTION_BEGIN();

    if ( _currentModule->getObjectHandler().exists( self.attacklast ) )
    {
        SET_TARGET_0(ObjectRef(self.attacklast));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToWhoeverBumped( script_state_t& state, ai_state_t& self )
{
    // SetTargetToWhoeverBumped()
    /// @author ZZ
    /// @details This function sets the target to whoever bumped the character last. It never fails

    SCRIPT_FUNCTION_BEGIN();

    if ( _currentModule->getObjectHandler().exists( self.bumplast ) )
    {
        SET_TARGET_0(ObjectRef(self.bumplast));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToWhoeverCalledForHelp( script_state_t& state, ai_state_t& self )
{
    // SetTargetToWhoeverCalledForHelp()
    /// @author ZZ
    /// @details This function sets the target to whoever called for help last.

    SCRIPT_FUNCTION_BEGIN();

    if ( VALID_TEAM_RANGE( pchr->team ) )
    {
        std::shared_ptr<Object> sissy = pchr->getTeam().getSissy();

        if ( sissy )
        {
            SET_TARGET_0(sissy->getObjRef());
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
Uint8 scr_SetTargetToOldTarget( script_state_t& state, ai_state_t& self )
{
    // SetTargetToOldTarget()
    /// @author ZZ
    /// @details This function sets the target to the target from last update, used to
    /// undo other set_Target functions

    SCRIPT_FUNCTION_BEGIN();

    if ( _currentModule->getObjectHandler().exists( self.target_old ) )
    {
        SET_TARGET_0(ObjectRef(self.target_old));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTurnModeToVelocity( script_state_t& state, ai_state_t& self )
{
    // SetTurnModeToVelocity()
    /// @author ZZ
    /// @details This function sets the character's movement mode to the default

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_VELOCITY;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTurnModeToWatch( script_state_t& state, ai_state_t& self )
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
Uint8 scr_SetTurnModeToSpin( script_state_t& state, ai_state_t& self )
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
Uint8 scr_SetBumpHeight( script_state_t& state, ai_state_t& self )
{
    // SetBumpHeight( tmpargument = "height" )
    /// @author ZZ
    /// @details This function makes the character taller or shorter, usually used when
    /// the character dies

    SCRIPT_FUNCTION_BEGIN();

    pchr->setBumpHeight(state.argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetHasID( script_state_t& state, ai_state_t& self )
{
    // IfTargetHasID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target has either a parent or type IDSZ
    /// matching tmpargument.

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[self.target];
    if(target) {
        returncode = target->getProfile()->hasTypeIDSZ(state.argument);
    }
    else {
        returncode = false;
    }


    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetHasItemID( script_state_t& state, ai_state_t& self )
{
    // IfTargetHasItemID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target has a matching item in his/her
    /// pockets or hands.

    SCRIPT_FUNCTION_BEGIN();

    Object *pself_target;
    SCRIPT_REQUIRE_TARGET(pself_target);

    //Assume nothing is found
    returncode = false;

    //Check hands
    if (nullptr != pself_target->isWieldingItemIDSZ(state.argument)) {
        returncode = true;
    }

    //Check inventory
    if (!returncode) {
        if (ObjectRef::Invalid != Inventory::findItem(pself_target->getObjRef(), state.argument, false)) {
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetHoldingItemID( script_state_t& state, ai_state_t& self )
{
    // IfTargetHoldingItemID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target has a matching item in his/her
    /// hands.  It also sets tmpargument to the proper latch button to press
    /// to use that item

    Object *pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET(pself_target);

    returncode = (pself_target->isWieldingItemIDSZ(state.argument) != nullptr);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetHasSkillID( script_state_t& state, ai_state_t& self )
{
    // IfTargetHasSkillID( tmpargument = "skill idsz" )
    /// @author ZZ
    /// @details This function proceeds if ID matches tmpargument

    Object *pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->hasSkillIDSZ(state.argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Else( script_state_t& state, ai_state_t& self )
{
    // Else
    /// @author ZZ
    /// @details This function fails if the last function was more indented

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ppro->getAIScript().indent >= ppro->getAIScript().indent_last );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Run( script_state_t& state, ai_state_t& self )
{
    // Run()
    /// @author ZZ
    /// @details This function sets the character's maximum acceleration to its
    /// actual maximum

    SCRIPT_FUNCTION_BEGIN();

    self.maxSpeed = 1.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Walk( script_state_t& state, ai_state_t& self )
{
    // Walk()
    /// @author ZZ
    /// @details This function sets the character's maximum acceleration to 66%
    /// of its actual maximum

    SCRIPT_FUNCTION_BEGIN();

    self.maxSpeed = 0.66f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Sneak( script_state_t& state, ai_state_t& self )
{
    // Sneak()
    /// @author ZZ
    /// @details This function sets the character's maximum acceleration to 33%
    /// of its actual maximum

    SCRIPT_FUNCTION_BEGIN();

    self.maxSpeed = 0.33f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoAction( script_state_t& state, ai_state_t& self )
{
    // DoAction( tmpargument = "action" )
    /// @author ZZ
    /// @details This function makes the character do a given action if it isn't doing
    /// anything better.  Fails if the action is invalid or if the character is doing
    /// something else already

    int action;

    SCRIPT_FUNCTION_BEGIN();

    action = pchr->getProfile()->getModel()->getAction( state.argument );

    returncode = false;
    if ( rv_success == chr_start_anim( pchr, action, false, false ) )
    {
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KeepAction( script_state_t& state, ai_state_t& self )
{
    // KeepAction()
    /// @author ZZ
    /// @details This function makes the character's animation stop on its last frame
    /// and stay there.  Usually used for dropped items

    SCRIPT_FUNCTION_BEGIN();

    chr_instance_t::set_action_keep(pchr->inst, true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IssueOrder( script_state_t& state, ai_state_t& self )
{
    // IssueOrder( tmpargument = "order"  )
    /// @author ZZ
    /// @details This function tells all of the character's teammates to do something,
    /// though each teammate needs to interpret the order using IfOrdered in
    /// its own script.

    SCRIPT_FUNCTION_BEGIN();

    issue_order( self.index, state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropWeapons( script_state_t& state, ai_state_t& self )
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

    const std::shared_ptr<Object> &rightItem = pchr->getRightHandItem();
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
Uint8 scr_TargetDoAction( script_state_t& state, ai_state_t& self )
{
    // TargetDoAction( tmpargument = "action" )
    /// @author ZZ
    /// @details The function makes the target start a new action, if it is valid for the model
    /// It will fail if the action is invalid or if the target is doing
    /// something else already

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( _currentModule->getObjectHandler().exists( self.target ) )
    {
        Object * pself_target = _currentModule->getObjectHandler().get( self.target );

        if ( pself_target->isAlive() )
        {
            int action = pself_target->getProfile()->getModel()->getAction( state.argument );

            if ( rv_success == chr_start_anim( pself_target, action, false, false ) )
            {
                returncode = true;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OpenPassage( script_state_t& state, ai_state_t& self )
{
    // OpenPassage( tmpargument = "passage" )

    /// @author ZZ
    /// @details This function opens the passage specified by tmpargument, failing if the
    /// passage was already open.
    /// Passage areas are defined in passage.txt and set in spawn.txt for the given character

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = _currentModule->getPassageByID(state.argument);
    
    returncode = false;
    if(passage) {
        returncode = true;
        passage->open();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClosePassage( script_state_t& state, ai_state_t& self )
{
    // ClosePassage( tmpargument = "passage" )
    /// @author ZZ
    /// @details This function closes the passage specified by tmpargument, proceeding
    /// if the passage isn't blocked.  Crushable characters within the passage
    /// are crushed.

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = _currentModule->getPassageByID(state.argument);

    returncode = false;
    if(passage) {
        returncode = passage->close();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfPassageOpen( script_state_t& state, ai_state_t& self )
{
    // IfPassageOpen( tmpargument = "passage" )
    /// @author ZZ
    /// @details This function proceeds if the given passage is valid and open to movement
    /// Used mostly by door characters to tell them when to run their open animation.

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = _currentModule->getPassageByID(state.argument);

    returncode = false;
    if(passage) {
        returncode = passage->isOpen();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GoPoof( script_state_t& state, ai_state_t& self )
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
        self.poof_time = update_wld;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostTargetItemID( script_state_t& state, ai_state_t& self )
{
    // CostTargetItemID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target has a matching item, and poofs
    /// that item.
    /// For one use keys and such

    Object *ptarget;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( ptarget );

    //first check both hands
    const IDSZ idsz = static_cast<IDSZ>(state.argument);
    std::shared_ptr<Object> pitem = ptarget->isWieldingItemIDSZ(idsz);

    //need to search inventory as well?
    if (!pitem)
    {
        for(const std::shared_ptr<Object> &inventoryItem : ptarget->getInventory().iterate())
        {
            //matching idsz?
            if ( inventoryItem->getProfile()->hasTypeIDSZ(idsz) ) {
                pitem = inventoryItem;
                break;
            }
        }
    }

    returncode = false;
    if ( pitem )
    {
        returncode = true;

        // Cost one ammo
        if ( pitem->ammo > 1 )
        {
            pitem->ammo--;
        }

        // Poof the item
        else
        {
            if ( pitem->isInsideInventory() )
            {
                // Remove from the pack
                pchr->getInventory().removeItem(pitem, true);
            }
            else
            {
                // Drop from hand
                pitem->detatchFromHolder(true, false);
            }

            // get rid of the character, no matter what
            pitem->requestTerminate();
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoActionOverride( script_state_t& state, ai_state_t& self )
{
    // DoActionOverride( tmpargument = "action" )
    /// @author ZZ
    /// @details This function makes the character do a given action no matter what
    /// It will fail if the action is invalid

    int action;

    SCRIPT_FUNCTION_BEGIN();

    action = pchr->getProfile()->getModel()->getAction(state.argument);

    returncode = false;
    if ( rv_success == chr_start_anim( pchr, action, false, true ) )
    {
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHealed( script_state_t& state, ai_state_t& self )
{
    // IfHealed()
    /// @author ZZ
    /// @details This function proceeds if the character was healed by a healing particle

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was healed
    returncode = HAS_SOME_BITS( self.alert, ALERTIF_HEALED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SendMessage( script_state_t& state, ai_state_t& self )
{
    // SendMessage( tmpargument = "message number" )
    /// @author ZZ
    /// @details This function sends a message to the players

    SCRIPT_FUNCTION_BEGIN();

    returncode = _display_message( self.index, pchr->getProfileID(), state.argument, &state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CallForHelp( script_state_t& state, ai_state_t& self )
{
    // CallForHelp()
    /// @author ZZ
    /// @details This function calls all of the character's teammates for help.  The
    /// teammates must use IfCalledForHelp in their scripts

    SCRIPT_FUNCTION_BEGIN();

    pchr->getTeam().callForHelp(_currentModule->getObjectHandler()[self.index]);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddIDSZ( script_state_t& state, ai_state_t& self )
{
    // AddIDSZ( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function slaps an expansion IDSZ onto the menu.txt file.
    /// Used to show completion of special quests for a given module

    SCRIPT_FUNCTION_BEGIN();

    if ( ModuleProfile::moduleAddIDSZ(_currentModule->getPath().c_str(), state.argument, 0, NULL) )
    {
        // invalidate any module list so that we will reload them
        //module_list_valid = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetState( script_state_t& state, ai_state_t& self )
{
    // SetState( tmpargument = "state" )
    /// @author ZZ
    /// @details This function sets the character's state.
    /// VERY IMPORTANT. State is preserved from update to update

    SCRIPT_FUNCTION_BEGIN();

    pchr->ai.state = state.argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetState( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetState()
    /// @author ZZ
    /// @details This function reads the character's state variable

    SCRIPT_FUNCTION_BEGIN();

    state.argument = self.state;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs( script_state_t& state, ai_state_t& self )
{
    // IfStateIs( tmpargument = "state" )
    /// @author ZZ
    /// @details This function proceeds if the character's state equals tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( state.argument == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetCanOpenStuff( script_state_t& state, ai_state_t& self )
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
        const std::shared_ptr<Object> &rider = pself_target->getLeftHandItem();

        if (rider)
        {
            // can the rider open stuff
            returncode = rider->getProfile()->canOpenStuff();
        }
    }

    if ( !returncode )
    {
        // if a rider can't openstuff, can the target openstuff?
        returncode = pself_target->getProfile()->canOpenStuff();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfGrabbed( script_state_t& state, ai_state_t& self )
{
    // IfGrabbed()
    /// @author ZZ
    /// @details This function proceeds if the character was grabbed (picked up) this update.
    /// Used mostly by item characters

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_GRABBED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfDropped( script_state_t& state, ai_state_t& self )
{
    // IfDropped()
    /// @author ZZ
    /// @details This function proceeds if the character was dropped this update.
    /// Used mostly by item characters

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_DROPPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToWhoeverIsHolding( script_state_t& state, ai_state_t& self )
{
    // SetTargetToWhoeverIsHolding()
    /// @author ZZ
    /// @details This function sets the target to the character's holder or mount,
    /// failing if the character has no mount or holder

    SCRIPT_FUNCTION_BEGIN();

    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        SET_TARGET_0(ObjectRef(pchr->attachedto));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DamageTarget( script_state_t& state, ai_state_t& self )
{
    // DamageTarget( tmpargument = "damage" )
    /// @author ZZ
    /// @details This function applies little bit of love to the character's target.
    /// The amount is set in tmpargument

    IPair tmp_damage;

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[self.target];
    if(!target) {
        return false;
    }

    tmp_damage.base = state.argument;
    tmp_damage.rand = 1;

    target->damage(ATK_FRONT, tmp_damage, static_cast<DamageType>(pchr->damagetarget_damagetype), 
        pchr->team, _currentModule->getObjectHandler()[self.index], DAMFX_NBLOC, true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfXIsLessThanY( script_state_t& state, ai_state_t& self )
{
    // IfXIsLessThanY( tmpx, tmpy )
    /// @author ZZ
    /// @details This function proceeds if tmpx is less than tmpy.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( state.x < state.y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetWeatherTime( script_state_t& state, ai_state_t& self )
{
    // SetWeatherTime( tmpargument = "time" )
    /// @author ZZ
    /// @details This function can be used to slow down or speed up or stop rain and
    /// other weather effects

    SCRIPT_FUNCTION_BEGIN();

    // Set the weather timer
    weather.timer_reset = state.argument;
    weather.time = state.argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetBumpHeight( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetBumpHeight()
    /// @author ZZ
    /// @details This function sets tmpargument to the character's height

    SCRIPT_FUNCTION_BEGIN();

    state.argument = pchr->bump.height;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfReaffirmed( script_state_t& state, ai_state_t& self )
{
    // IfReaffirmed()
    /// @author ZZ
    /// @details This function proceeds if the character was damaged by its reaffirm
    /// damage type.  Used to relight the torch.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_REAFFIRMED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkeepAction( script_state_t& state, ai_state_t& self )
{
    // UnkeepAction()
    /// @author ZZ
    /// @details This function is the opposite of KeepAction. It makes the current animation resume.

    SCRIPT_FUNCTION_BEGIN();

    chr_instance_t::set_action_keep(pchr->inst, false);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsOnOtherTeam( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsOnOtherTeam()
    /// @author ZZ
    /// @details This function proceeds if the target is on another team

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->isAlive() &&  pself_target->getTeam() != pchr->getTeam() );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsOnHatedTeam( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsOnHatedTeam()
    /// @author ZZ
    /// @details This function proceeds if the target is on an enemy team

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->isAlive() && pchr->getTeam().hatesTeam(pself_target->getTeam()) && !pself_target->isInvincible() );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PressLatchButton( script_state_t& state, ai_state_t& self )
{
    // PressLatchButton( tmpargument = "latch bits" )
    /// @author ZZ
    /// @details This function sets the character latch buttons

    SCRIPT_FUNCTION_BEGIN();

    if(state.argument >= LATCHBUTTON_LEFT && state.argument < LATCHBUTTON_RESPAWN)
    {
        pchr->latch.b[state.argument] = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToTargetOfLeader( script_state_t& state, ai_state_t& self )
{
    // SetTargetToTargetOfLeader()
    /// @author ZZ
    /// @details This function sets the character's target to the target of its leader,
    /// or it fails with no change if the leader is dead

    SCRIPT_FUNCTION_BEGIN();

    if ( VALID_TEAM_RANGE( pchr->team ) )
    {
        const std::shared_ptr<Object> &leader = _currentModule->getTeamList()[pchr->team].getLeader();

        if ( leader )
        {
            auto itarget = ObjectRef(leader->ai.target);

            if ( _currentModule->getObjectHandler().exists( itarget ) )
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
Uint8 scr_IfLeaderKilled( script_state_t& state, ai_state_t& self )
{
    // IfLeaderKilled()
    /// @author ZZ
    /// @details This function proceeds if the team's leader died this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_LEADERKILLED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeLeader( script_state_t& state, ai_state_t& self )
{
    // BecomeLeader()
    /// @author ZZ
    /// @details This function makes the character the leader of the team

    SCRIPT_FUNCTION_BEGIN();

    _currentModule->getTeamList()[pchr->team].setLeader(_currentModule->getObjectHandler()[self.index]);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTargetArmor( script_state_t& state, ai_state_t& self )
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
    state.x = pself_target->setSkin(state.argument);

    state.argument = iTmp;  // The character's old armor

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveMoneyToTarget( script_state_t& state, ai_state_t& self )
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
    iTmp = pchr->money - state.argument;
    iTmp = CLIP( iTmp, 0, MAXMONEY );

    // limit the range of the target's money
    tTmp = pself_target->money + state.argument;
    tTmp = CLIP( tTmp, 0, MAXMONEY );

    // recover the possible transfer values
    iTmp = iTmp + state.argument;
    tTmp = tTmp - state.argument;

    // limit the transfer values
    if ( state.argument < 0 )
    {
        state.argument = std::max( iTmp, tTmp );
    }
    else
    {
        state.argument = std::min( iTmp, tTmp );
    }

    pchr->money         = pchr->money + state.argument;
    pself_target->money = pself_target->money + state.argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropKeys( script_state_t& state, ai_state_t& self )
{
    // DropKeys()
    /// @author ZZ
    /// @details This function drops all of the keys in the character's inventory.
    /// This does NOT drop keys in the character's hands.

    SCRIPT_FUNCTION_BEGIN();

    pchr->dropKeys();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfLeaderIsAlive( script_state_t& state, ai_state_t& self )
{
    // IfLeaderIsAlive()
    /// @author ZZ
    /// @details This function proceeds if the team has a leader

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( _currentModule->getTeamList()[pchr->team].getLeader() != nullptr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsOldTarget( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsOldTarget()
    /// @author ZZ
    /// @details This function proceeds if the target is the same as it was last update

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( self.target == self.target_old );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToLeader( script_state_t& state, ai_state_t& self )
{
    // SetTargetToLeader()
    /// @author ZZ
    /// @details This function sets the target to the leader, proceeding if their is
    /// a valid leader for the character's team

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( VALID_TEAM_RANGE( pchr->team ) )
    {
        const std::shared_ptr<Object> &leader = _currentModule->getTeamList()[pchr->team].getLeader();
        if ( leader )
        {
            SET_TARGET_0(leader->getObjRef());
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnCharacter( script_state_t& state, ai_state_t& self )
{
    // SpawnCharacter( tmpx = "x", tmpy = "y", tmpturn = "turn", tmpdistance = "speed" )

    /// @author ZZ
    /// @details This function spawns a character of the same type as the spawner.
    /// This function spawns a character, failing if x,y is invalid
    /// This is horribly complicated to use, so see ANIMATE.OBJ for an example
    /// tmpx and tmpy give the coodinates, tmpturn gives the new character's
    /// direction, and tmpdistance gives the new character's initial velocity

    SCRIPT_FUNCTION_BEGIN();

	Vector3f pos = Vector3f(float(state.x), float(state.y), 0.0f);

    std::shared_ptr<Object> pchild = _currentModule->spawnObject(pos, pchr->getProfileID(), pchr->team, 0, CLIP_TO_16BITS( state.turn ), "", INVALID_CHR_REF);
    returncode = pchild != nullptr;

    if ( !returncode )
    {
		Log::get().warn( "Object %s failed to spawn a copy of itself\n", pchr->getName().c_str() );
    }
    else
    {
        // was the child spawned in a "safe" spot?
        if (!pchild->hasSafePosition()) {
			Log::get().warn( "Object %s failed to spawn a copy of itself (no safe location)\n", pchr->getName().c_str() );
            pchild->requestTerminate();
        }
        else
        {
            TURN_T turn;
            self.child = pchild->getObjRef().get();

            turn = TO_TURN( pchr->ori.facing_z + ATK_BEHIND );
            pchild->vel[kX] += turntocos[ turn ] * state.distance;
            pchild->vel[kY] += turntosin[ turn ] * state.distance;

            pchild->iskursed = pchr->iskursed;  /// @note BB@> inherit this from your spawner
            pchild->ai.passage = self.passage;
            pchild->ai.owner   = self.owner;

            pchild->dismount_timer  = PHYS_DISMOUNT_TIME;
            pchild->dismount_object = self.index;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RespawnCharacter( script_state_t& state, ai_state_t& self )
{
    // RespawnCharacter()
    /// @author ZZ
    /// @details This function respawns the character at its starting location.
    /// Often used with the Clean functions

    SCRIPT_FUNCTION_BEGIN();

    pchr->respawn();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTile( script_state_t& state, ai_state_t& self )
{
    // ChangeTile( tmpargument = "tile type")
    /// @author ZZ
    /// @details This function changes the tile under the character to the new tile type,
    /// which is highly module dependent

    SCRIPT_FUNCTION_BEGIN();

	auto mesh = _currentModule->getMeshPointer();
	if (!mesh) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == mesh");
	}
    returncode = mesh->set_texture( pchr->getTile(), state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfUsed( script_state_t& state, ai_state_t& self )
{
    // IfUsed()
    /// @author ZZ
    /// @details This function proceeds if the character was used by its holder or rider.
    /// Character's cannot be used if their reload time is greater than 0

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_USED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropMoney( script_state_t& state, ai_state_t& self )
{
    // DropMoney( tmpargument = "money" )
    /// @author ZZ
    /// @details This function drops a certain amount of money, if the character has that
    /// much

    SCRIPT_FUNCTION_BEGIN();

    pchr->dropMoney(state.argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetOldTarget( script_state_t& state, ai_state_t& self )
{
    // SetOldTarget()
    /// @author ZZ
    /// @details This function sets the old target to the current target.  To allow
    /// greater manipulations of the target

    SCRIPT_FUNCTION_BEGIN();

    self.target_old = self.target;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DetachFromHolder( script_state_t& state, ai_state_t& self )
{
    // DetachFromHolder()
    /// @author ZZ
    /// @details This function drops the character or makes it get off its mount
    /// Can be used to make slippery weapons, or to make certain characters
    /// incapable of wielding certain weapons. "A troll can't grab a torch"

    SCRIPT_FUNCTION_BEGIN();

    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
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
Uint8 scr_IfTargetHasVulnerabilityID( script_state_t& state, ai_state_t& self )
{
    // IfTargetHasVulnerabilityID( tmpargument = "vulnerability idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target is vulnerable to the given IDSZ.
    
    Object *pself_target;
    
    SCRIPT_FUNCTION_BEGIN();
    
    SCRIPT_REQUIRE_TARGET(pself_target);
    
    returncode = pself_target->getProfile()->getIDSZ(IDSZ_VULNERABILITY) == static_cast<IDSZ>(state.argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CleanUp( script_state_t& state, ai_state_t& self )
{
    // CleanUp()
    /// @author ZZ
    /// @details This function tells all the dead characters on the team to clean
    /// themselves up.  Usually done by the boss creature every second or so

    SCRIPT_FUNCTION_BEGIN();

    for(const std::shared_ptr<Object> &listener : _currentModule->getObjectHandler().iterator())
    {
        if ( pchr->getTeam() != listener->getTeam() ) continue;

        if ( !listener->isAlive() )
        {
            listener->ai.timer  = update_wld + 2;  // Don't let it think too much...
        }

        SET_BIT( listener->ai.alert, ALERTIF_CLEANEDUP );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfCleanedUp( script_state_t& state, ai_state_t& self )
{
    // IfCleanedUp()
    /// @author ZZ
    /// @details This function proceeds if the character is dead and if the boss told it
    /// to clean itself up

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_CLEANEDUP );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfSitting( script_state_t& state, ai_state_t& self )
{
    // IfSitting()
    /// @author ZZ
    /// @details This function proceeds if the character is riding a mount

    SCRIPT_FUNCTION_BEGIN();

    returncode = _currentModule->getObjectHandler().exists( pchr->attachedto );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsHurt( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsHurt()
    /// @author ZZ
    /// @details This function passes only if the target is hurt and alive

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !pself_target->isAlive() || pself_target->getLife() > pself_target->getAttribute(Ego::Attribute::MAX_LIFE) - FP8_TO_FLOAT(HURTDAMAGE) )
        returncode = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsAPlayer( script_state_t& state, ai_state_t& self )
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
Uint8 scr_PlaySound( script_state_t& state, ai_state_t& self )
{
    // PlaySound( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function plays one of the character's sounds.
    /// The sound fades out depending on its distance from the viewer

    SCRIPT_FUNCTION_BEGIN();

    if ( pchr->getOldPosition()[kZ] > PITNOSOUND )
    {
        AudioSystem::get().playSound(pchr->getOldPosition(), ppro->getSoundID(state.argument));
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnParticle( script_state_t& state, ai_state_t& self )
{
    // SpawnParticle(tmpargument = "particle", tmpdistance = "character vertex", tmpx = "offset x", tmpy = "offset y" )
    /// @author ZZ
    /// @details This function spawns a particle, offset from the character's location

    SCRIPT_FUNCTION_BEGIN();

	CHR_REF ichr = self.index;
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    //If we are a mount, our rider is the owner of this particle
    if ( pchr->isMount() && _currentModule->getObjectHandler().exists( pchr->holdingwhich[SLOT_LEFT] ) )
    {
        ichr = pchr->holdingwhich[SLOT_LEFT];
    }

    std::shared_ptr<Ego::Particle> particle = ParticleHandler::get().spawnLocalParticle(pchr->getPosition(), 
                                                   pchr->ori.facing_z, 
                                                   pchr->getProfileID(),
                                                   LocalParticleProfileRef(state.argument), self.index,
                                                   state.distance, pchr->team, ichr, INVALID_PRT_REF, 0,
                                                   INVALID_CHR_REF );

    returncode = (particle != nullptr);
    if ( returncode )
    {
        // attach the particle
        particle->placeAtVertex(_currentModule->getObjectHandler()[self.index], state.distance);
        particle->attach(INVALID_CHR_REF);

		Vector3f tmp_pos = particle->getPosition();

        // Correct X, Y, Z spacing
        tmp_pos[kZ] += particle->getProfile()->spacing_vrt_pair.base;

        // Don't spawn in walls
        tmp_pos[kX] += state.x;
        if (EMPTY_BIT_FIELD != particle->test_wall(tmp_pos))
        {
            tmp_pos[kX] = particle->getPosX();

            tmp_pos[kY] += state.y;
            if (EMPTY_BIT_FIELD != particle->test_wall(tmp_pos))
            {
                tmp_pos[kY] = particle->getPosY();
            }
        }

        particle->setPosition(tmp_pos);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsAlive( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsAlive()
    /// @author ZZ
    /// @details This function proceeds if the target is alive

    SCRIPT_FUNCTION_BEGIN();

	Object *pself_target;
    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->isAlive();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Stop( script_state_t& state, ai_state_t& self )
{
    // Stop()
    /// @author ZZ
    /// @details This function sets the character's maximum acceleration to 0.  Used
    /// along with Walk and Run and Sneak

    SCRIPT_FUNCTION_BEGIN();

    self.maxSpeed = 0.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisaffirmCharacter( script_state_t& state, ai_state_t& self )
{
    // DisaffirmCharacter()
    /// @author ZZ
    /// @details This function removes all the attached particles from a character
    /// ( stuck arrows, flames, etc )

    SCRIPT_FUNCTION_BEGIN();

    disaffirm_attached_particles(ObjectRef(self.index));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ReaffirmCharacter( script_state_t& state, ai_state_t& self )
{
    // ReaffirmCharacter()
    /// @author ZZ
    /// @details This function makes sure it has all of its reaffirmation particles
    /// attached to it. Used to make the torch light again

    SCRIPT_FUNCTION_BEGIN();

    reaffirm_attached_particles(ObjectRef(self.index));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsSelf( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsSelf()
    /// @author ZZ
    /// @details This function proceeds if the character is targeting itself

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( self.target == self.index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsMale( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsMale()
    /// @author ZZ
    /// @details This function proceeds only if the target is male

    SCRIPT_FUNCTION_BEGIN();

	Object *pself_target;
    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->gender == GENDER_MALE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsFemale( script_state_t& state, ai_state_t& self )
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
Uint8 scr_SetTargetToSelf( script_state_t& state, ai_state_t& self )
{
    // SetTargetToSelf()
    /// @author ZZ
    /// @details This function sets the target to the character itself

    SCRIPT_FUNCTION_BEGIN();

    SET_TARGET_0(ObjectRef(self.index));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToRider( script_state_t& state, ai_state_t& self )
{
    // SetTargetToRider()
    /// @author ZZ
    /// @details This function sets the target to whoever is riding the character (left/only grip),
    /// failing if there is no rider

    SCRIPT_FUNCTION_BEGIN();

    if ( _currentModule->getObjectHandler().exists( pchr->holdingwhich[SLOT_LEFT] ) )
    {
        SET_TARGET_0(ObjectRef(pchr->holdingwhich[SLOT_LEFT]));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetAttackTurn( script_state_t& state, ai_state_t& self )
{
    // tmpturn = GetAttackTurn()
    /// @author ZZ
    /// @details This function sets tmpturn to the direction from which the last attack
    /// came. Not particularly useful in most cases, but it could be.

    SCRIPT_FUNCTION_BEGIN();

    state.turn = self.directionlast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetDamageType( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetDamageType()
    /// @author ZZ
    /// @details This function sets tmpargument to the damage type of the last attack that
    /// hit the character

    SCRIPT_FUNCTION_BEGIN();

    state.argument = self.damagetypelast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeSpell( script_state_t& state, ai_state_t& self )
{
    // BecomeSpell()
    /// @author ZZ
    /// @details This function turns a spellbook character into a spell based on its
    /// content.
    /// TOO COMPLICATED TO EXPLAIN.  SHOULDN'T EVER BE NEEDED BY YOU.

    SCRIPT_FUNCTION_BEGIN();

    // change the spellbook to a spell effect
    pchr->disenchant();
    pchr->polymorphObject(self.content, 0);

    // set the spell effect parameters
    self.content = 0;
    self.state = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeSpellbook( script_state_t& state, ai_state_t& self )
{
    // BecomeSpellbook()
    //
    /// @author ZZ
    /// @details This function turns a spell character into a spellbook and sets the content accordingly.
    /// TOO COMPLICATED TO EXPLAIN. Just copy the spells that already exist, and don't change
    /// them too much

    SCRIPT_FUNCTION_BEGIN();

    // convert the spell effect to a spellbook
    PRO_REF old_profile = pchr->getProfileID();
    pchr->disenchant();
    pchr->polymorphObject(SPELLBOOK, ppro->getSpellEffectType());

    // Reset the spellbook state so it doesn't burn up
    self.state = 0;
    self.content = REF_TO_INT( old_profile );

    // set the spellbook animations
    // Do dropped animation
    if (rv_success == chr_start_anim(pchr, pchr->getProfile()->getModel()->getAction(ACTION_JB), false, true))
    {
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfScoredAHit( script_state_t& state, ai_state_t& self )
{
    // IfScoredAHit()
    /// @author ZZ
    /// @details This function proceeds if the character damaged another character this
    /// update. If it's a held character it also sets the target to whoever was hit

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character scored a hit
//    if ( !_currentModule->getObjectHandler().exists( pchr->attachedto ) || _currentModule->getObjectHandler().get(pchr->attachedto).ismount )
//    {
    returncode = HAS_SOME_BITS( self.alert, ALERTIF_SCOREDAHIT );
//    }

    // Proceed only if the holder scored a hit with the character
    /*    else if ( _currentModule->getObjectHandler().get(pchr->attachedto).ai.lastitemused == pself->index )
        {
            returncode = HAS_SOME_BITS( _currentModule->getObjectHandler().get(pchr->attachedto).ai.alert, ALERTIF_SCOREDAHIT );
        }
        else returncode = false;*/

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfDisaffirmed( script_state_t& state, ai_state_t& self )
{
    // IfDisaffirmed()
    /// @author ZZ
    /// @details This function proceeds if the character was disaffirmed.
    /// This doesn't seem useful anymore.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_DISAFFIRMED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TranslateOrder( script_state_t& state, ai_state_t& self )
{
    // tmpx,tmpy,tmpargument = TranslateOrder()
    /// @author ZZ
    /// @details This function translates a packed order into understandable values.
    /// See CreateOrder for more.  This function sets tmpx, tmpy, tmpargument,
    /// and sets the target ( if valid )

    SCRIPT_FUNCTION_BEGIN();

    auto ichr = ObjectRef(CLIP_TO_16BITS( self.order_value >> 24 ));

    if ( _currentModule->getObjectHandler().exists( ichr ) )
    {
        SET_TARGET_0( ichr );

        state.x        = (( self.order_value >> 14 ) & 0x03FF ) << 6;
        state.y        = (( self.order_value >>  4 ) & 0x03FF ) << 6;
        state.argument = (( self.order_value >>  0 ) & 0x000F );
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToWhoeverWasHit( script_state_t& state, ai_state_t& self )
{
    // SetTargetToWhoeverWasHit()
    /// @author ZZ
    /// @details This function sets the target to whoever was hit by the character last

    SCRIPT_FUNCTION_BEGIN();

    if ( _currentModule->getObjectHandler().exists( self.hitlast ) )
    {
        SET_TARGET_0(ObjectRef(self.hitlast));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToWideEnemy( script_state_t& state, ai_state_t& self )
{
    // SetTargetToWideEnemy()
    /// @author ZZ
    /// @details This function sets the target to an enemy in the vicinity around the
    /// character, failing if there are none

    SCRIPT_FUNCTION_BEGIN();

    auto ichr = chr_find_target( pchr, WIDE, IDSZ_NONE, TARGET_ENEMIES );

    if ( _currentModule->getObjectHandler().exists( ichr ) )
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
Uint8 scr_IfChanged( script_state_t& state, ai_state_t& self )
{
    // IfChanged()
    /// @author ZZ
    /// @details This function proceeds if the character was polymorphed.
    /// Needed for morph spells and such

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_CHANGED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfInWater( script_state_t& state, ai_state_t& self )
{
    // IfInWater()
    /// @author ZZ
    /// @details This function proceeds if the character has just entered into some water
    /// this update ( and the water is really water, not fog or another effect )

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_INWATER );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfBored( script_state_t& state, ai_state_t& self )
{
    // IfBored()
    /// @author ZZ
    /// @details This function proceeds if the character has been standing idle too long

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_BORED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTooMuchBaggage( script_state_t& state, ai_state_t& self )
{
    // IfTooMuchBaggage()
    /// @author ZZ
    /// @details This function proceeds if the character tries to put an item in his/her
    /// pockets, but the character already has 6 items in the inventory.
    /// Used to tell the players what's going on.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_TOOMUCHBAGGAGE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfGrogged( script_state_t& state, ai_state_t& self )
{
    // IfGrogged()
    /// @author ZZ
    /// @details This function proceeds if the character has been grogged ( a type of
    /// confusion ) this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = _currentModule->getObjectHandler().get(self.index)->grog_timer > 0 && HAS_SOME_BITS( self.alert, ALERTIF_CONFUSED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfDazed( script_state_t& state, ai_state_t& self )
{
    // IfDazed()
    /// @author ZZ
    /// @details This function proceeds if the character has been dazed ( a type of
    /// confusion ) this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = _currentModule->getObjectHandler().get(self.index)->daze_timer > 0 && HAS_SOME_BITS( self.alert, ALERTIF_CONFUSED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetHasSpecialID( script_state_t& state, ai_state_t& self )
{
    // IfTargetHasSpecialID( tmpargument = "special idsz" )
    /// @author ZZ
    /// @details This function proceeds if the character has a special IDSZ ( in data.txt )

    Object *pself_target;
    
    SCRIPT_FUNCTION_BEGIN();
    
    SCRIPT_REQUIRE_TARGET(pself_target);

    returncode = pself_target->getProfile()->getIDSZ(IDSZ_SPECIAL) == static_cast<IDSZ>(state.argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PressTargetLatchButton( script_state_t& state, ai_state_t& self )
{
    // PressTargetLatchButton( tmpargument = "latch bits" )
    /// @author ZZ
    /// @details This function mimics joystick button presses for the target.
    /// For making items force their own usage and such

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if(state.argument >= LATCHBUTTON_LEFT && state.argument < LATCHBUTTON_RESPAWN)
    {
        pself_target->latch.b[state.argument] = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfInvisible( script_state_t& state, ai_state_t& self )
{
    // IfInvisible()
    /// @author ZZ
    /// @details This function proceeds if the character is invisible

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->inst.alpha <= INVISIBLE;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfArmorIs( script_state_t& state, ai_state_t& self )
{
    // IfArmorIs( tmpargument = "skin" )
    /// @author ZZ
    /// @details This function proceeds if the character's skin type equals tmpargument

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    tTmp = pchr->skin;
    returncode = ( tTmp == state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetTargetGrogTime( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetTargetGrogTime()
    /// @author ZZ
    /// @details This function sets tmpargument to the number of updates before the
    /// character is ungrogged, proceeding if the number is greater than 0

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    state.argument = pself_target->grog_timer;

    returncode = ( 0 != state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetTargetDazeTime( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetTargetDazeTime()
    /// @author ZZ
    /// @details This function sets tmpargument to the number of updates before the
    /// character is undazed, proceeding if the number is greater than 0

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    state.argument = pself_target->daze_timer;

    returncode = ( 0 != state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetDamageType( script_state_t& state, ai_state_t& self )
{
    // SetDamageType( tmpargument = "damage type" )
    /// @author ZZ
    /// @details This function lets a weapon change the type of damage it inflicts

    SCRIPT_FUNCTION_BEGIN();

    pchr->damagetarget_damagetype = static_cast<DamageType>(state.argument % DAMAGE_COUNT);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetWaterLevel( script_state_t& state, ai_state_t& self )
{
    // SetWaterLevel( tmpargument = "level" )
    /// @author ZZ
    /// @details This function raises or lowers the water in the module

    SCRIPT_FUNCTION_BEGIN();

    water.set_douse_level(state.argument / 10.0f);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnchantTarget( script_state_t& state, ai_state_t& self )
{
    // EnchantTarget()
    /// @author ZZ
    /// @details This function enchants the target with the enchantment given
    /// in enchant.txt. Make sure you use set_OwnerToTarget before doing this.

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> target = _currentModule->getObjectHandler()[self.target];
    if(target) {
        returncode = target->addEnchant(pchr->getProfile()->getEnchantRef(), pchr->getProfileID(), _currentModule->getObjectHandler()[self.owner], _currentModule->getObjectHandler()[pchr->getObjRef()]) != nullptr;
    }   
    else {
        returncode = false;
    } 

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnchantChild( script_state_t& state, ai_state_t& self )
{
    // EnchantChild()
    /// @author ZZ
    /// @details This function can be used with SpawnCharacter to enchant the
    /// newly spawned character with the enchantment
    /// given in enchant.txt. Make sure you use set_OwnerToTarget before doing this.

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> child = _currentModule->getObjectHandler()[self.child];
    if(child) {
        returncode = child->addEnchant(pchr->getProfile()->getEnchantRef(), pchr->getProfileID(), _currentModule->getObjectHandler()[self.owner], _currentModule->getObjectHandler()[pchr->getObjRef()]) != nullptr;
    }   
    else {
        returncode = false;
    } 

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TeleportTarget( script_state_t& state, ai_state_t& self )
{
    // TeleportTarget( tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function teleports the target to the X, Y location, failing if the
    /// location is off the map or blocked

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[self.target];
    if(!target) {
        return false;
    }

    returncode = target->teleport(Vector3f(float(state.x), float(state.y), float(state.distance)), state.turn);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveExperienceToTarget( script_state_t& state, ai_state_t& self )
{
    // GiveExperienceToTarget( tmpargument = "amount", tmpdistance = "type" )
    /// @author ZZ
    /// @details This function gives the target some experience, xptype from distance,
    /// amount from argument.

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[self.target];
    if(!target) {
        return false;
    }

    target->giveExperience(state.argument, static_cast<XPType>(state.distance), false);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IncreaseAmmo( script_state_t& state, ai_state_t& self )
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
Uint8 scr_UnkurseTarget( script_state_t& state, ai_state_t& self )
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
Uint8 scr_GiveExperienceToTargetTeam( script_state_t& state, ai_state_t& self )
{
    // GiveExperienceToTargetTeam( tmpargument = "amount", tmpdistance = "type" )
    /// @author ZZ
    /// @details This function gives experience to everyone on the target's team

    SCRIPT_FUNCTION_BEGIN();

    if(state.distance < XP_COUNT && state.distance >= 0) {
        pchr->getTeam().giveTeamExperience(state.argument, static_cast<XPType>(state.distance));
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfUnarmed( script_state_t& state, ai_state_t& self )
{
    // IfUnarmed()
    /// @author ZZ
    /// @details This function proceeds if the character is holding no items in hand.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( !_currentModule->getObjectHandler().exists( pchr->holdingwhich[SLOT_LEFT] ) && !_currentModule->getObjectHandler().exists( pchr->holdingwhich[SLOT_RIGHT] ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RestockTargetAmmoIDAll( script_state_t& state, ai_state_t& self )
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
    iTmp += RestockAmmo( ichr, state.argument );

    ichr = pself_target->holdingwhich[SLOT_RIGHT];
    iTmp += RestockAmmo( ichr, state.argument );

    for(const std::shared_ptr<Object> pitem : pchr->getInventory().iterate())
    {
        iTmp += RestockAmmo( pitem->getObjRef().get(), state.argument );
    }

    state.argument = iTmp;
    returncode = ( iTmp != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RestockTargetAmmoIDFirst( script_state_t& state, ai_state_t& self )
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
    iTmp += RestockAmmo(ichr, state.argument);
    
    if (iTmp == 0)
    {
        ichr = pself_target->holdingwhich[SLOT_RIGHT];
        iTmp += RestockAmmo(ichr, state.argument);
    }

    if (iTmp == 0)
    {
        for(const std::shared_ptr<Object> pitem : pchr->getInventory().iterate())
        {
            iTmp += RestockAmmo( pitem->getObjRef().get(), state.argument );
            if ( 0 != iTmp ) break;
        }
    }

    state.argument = iTmp;
    returncode = ( iTmp != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashTarget( script_state_t& state, ai_state_t& self )
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
Uint8 scr_SetRedShift( script_state_t& state, ai_state_t& self )
{
    // SetRedShift( tmpargument = "red darkening" )
    /// @author ZZ
    /// @details This function sets the character's red shift ( 0 - 3 ), higher values
    /// making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    pchr->setBaseAttribute(Ego::Attribute::RED_SHIFT, Ego::Math::constrain(state.argument, 0, 6));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetGreenShift( script_state_t& state, ai_state_t& self )
{
    // SetGreenShift( tmpargument = "green darkening" )
    /// @author ZZ
    /// @details This function sets the character's green shift ( 0 - 3 ), higher values
    /// making the character less green and darker

    SCRIPT_FUNCTION_BEGIN();

    pchr->setBaseAttribute(Ego::Attribute::GREEN_SHIFT, Ego::Math::constrain(state.argument, 0, 6));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetBlueShift( script_state_t& state, ai_state_t& self )
{
    // SetBlueShift( tmpargument = "blue darkening" )
    /// @author ZZ
    /// @details This function sets the character's blue shift ( 0 - 3 ), higher values
    /// making the character less blue and darker

    SCRIPT_FUNCTION_BEGIN();

    pchr->setBaseAttribute(Ego::Attribute::BLUE_SHIFT, Ego::Math::constrain(state.argument, 0, 6));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetLight( script_state_t& state, ai_state_t& self )
{
    // SetLight( tmpargument = "lighness" )
    /// @author ZZ
    /// @details This function alters the character's transparency ( 0 - 254 )
    /// 255 = no transparency

    SCRIPT_FUNCTION_BEGIN();

    pchr->setLight(state.argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetAlpha( script_state_t& state, ai_state_t& self )
{
    // SetAlpha( tmpargument = "alpha" )
    /// @author ZZ
    /// @details This function alters the character's transparency ( 0 - 255 )
    /// 255 = no transparency

    SCRIPT_FUNCTION_BEGIN();

    pchr->setAlpha(state.argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHitFromBehind( script_state_t& state, ai_state_t& self )
{
    // IfHitFromBehind()
    /// @author ZZ
    /// @details This function proceeds if the last attack to the character came from behind

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( self.directionlast >= ATK_BEHIND - 8192 && self.directionlast < ATK_BEHIND + 8192 )
        returncode = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHitFromFront( script_state_t& state, ai_state_t& self )
{
    // IfHitFromFront()
    /// @author ZZ
    /// @details This function proceeds if the last attack to the character came
    /// from the front

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( self.directionlast >= ATK_LEFT + 8192 || self.directionlast < ATK_FRONT + 8192 )
        returncode = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHitFromLeft( script_state_t& state, ai_state_t& self )
{
    // IfHitFromLeft()
    /// @author ZZ
    /// @details This function proceeds if the last attack to the character came
    /// from the left

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( self.directionlast >= ATK_LEFT - 8192 && self.directionlast < ATK_LEFT + 8192 )
        returncode = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHitFromRight( script_state_t& state, ai_state_t& self )
{
    // IfHitFromRight()
    /// @author ZZ
    /// @details This function proceeds if the last attack to the character came
    /// from the right

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( self.directionlast >= ATK_RIGHT - 8192 && self.directionlast < ATK_RIGHT + 8192 )
        returncode = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsOnSameTeam( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsOnSameTeam()
    /// @author ZZ
    /// @details This function proceeds if the target is on the character's team

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[self.target];
    if(target) {
        returncode = target->getTeam() == pchr->getTeam();
    }
    else {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KillTarget( script_state_t& state, ai_state_t& self )
{
    // KillTarget()
    /// @author ZZ
    /// @details This function kills the target

    SCRIPT_FUNCTION_BEGIN();

	CHR_REF ichr = self.index;

    //Weapons don't kill people, people kill people...
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) && !_currentModule->getObjectHandler().get(pchr->attachedto)->isMount() )
    {
        ichr = pchr->attachedto;
    }

    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[self.target];
    if(target) {
        target->kill(_currentModule->getObjectHandler()[ichr], false);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UndoEnchant( script_state_t& state, ai_state_t& self )
{
    // UndoEnchant()
    /// @author ZZ
    /// @details This function removes the last enchantment spawned by the character,
    /// proceeding if an enchantment was removed

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Ego::Enchantment> lastEnchant = pchr->getLastEnchantmentSpawned();
    if(lastEnchant == nullptr || lastEnchant->isTerminated()) {
        returncode = false;
    }
    else {
        returncode = true;
        lastEnchant->requestTerminate();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetWaterLevel( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetWaterLevel()
    /// @author ZZ
    /// @details This function sets tmpargument to the current douse level for the water * 10.
    /// A waterlevel in wawalight of 85 would set tmpargument to 850

    SCRIPT_FUNCTION_BEGIN();

    state.argument = water._douse_level * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostTargetMana( script_state_t& state, ai_state_t& self )
{
    // CostTargetMana( tmpargument = "amount" )
    /// @author ZZ
    /// @details This function costs the target a specific amount of mana, proceeding
    /// if the target was able to pay the price.  The amounts are 8.8 fixed point

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> target = _currentModule->getObjectHandler()[self.target];
    if(target) {
        returncode = target->costMana(state.argument, self.index);
    }
    else {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetHasAnyID( script_state_t& state, ai_state_t& self )
{
    // IfTargetHasAnyID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target has any IDSZ that matches the given one

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> target = _currentModule->getObjectHandler()[self.target];
    if(target) {
        returncode = target->getProfile()->hasIDSZ(state.argument);
    }
    else {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetBumpSize( script_state_t& state, ai_state_t& self )
{
    // SetBumpSize( tmpargument = "size" )
    /// @author ZZ
    /// @details This function sets the how wide the character is

    SCRIPT_FUNCTION_BEGIN();

    pchr->setBumpWidth(state.argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfNotDropped( script_state_t& state, ai_state_t& self )
{
    // IfNotDropped()
    /// @author ZZ
    /// @details This function proceeds if the character is kursed and another character
    /// was holding it and tried to drop it

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_NOTDROPPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfYIsLessThanX( script_state_t& state, ai_state_t& self )
{
    // IfYIsLessThanX()
    /// @author ZZ
    /// @details This function proceeds if tmpy is less than tmpx

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( state.y < state.x );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetFlyHeight( script_state_t& state, ai_state_t& self )
{
    // SetFlyHeight( tmpargument = "height" )
    /// @author ZZ
    /// @details This function makes the character fly ( or fall to ground if 0 )

    SCRIPT_FUNCTION_BEGIN();

    pchr->setBaseAttribute(Ego::Attribute::FLY_TO_HEIGHT, std::max(0, state.argument));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfBlocked( script_state_t& state, ai_state_t& self )
{
    // IfBlocked()
    /// @author ZZ
    /// @details This function proceeds if the character blocked the attack of another
    /// character this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_BLOCKED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsDefending( script_state_t& state, ai_state_t& self )
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
Uint8 scr_IfTargetIsAttacking( script_state_t& state, ai_state_t& self )
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
Uint8 scr_IfStateIs0( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs1( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 1 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs2( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 2 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs3( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 3 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs4( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 4 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs5( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 5 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs6( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 6 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs7( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 7 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfContentIs( script_state_t& state, ai_state_t& self )
{
    // IfContentIs( tmpargument = "test" )
    /// @author ZZ
    /// @details This function proceeds if the content matches tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( state.argument == self.content );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTurnModeToWatchTarget( script_state_t& state, ai_state_t& self )
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
Uint8 scr_IfStateIsNot( script_state_t& state, ai_state_t& self )
{
    // IfStateIsNot( tmpargument = "test" )
    /// @author ZZ
    /// @details This function proceeds if the character's state does not equal tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( state.argument != self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfXIsEqualToY( script_state_t& state, ai_state_t& self )
{
    // These functions proceed if tmpx and tmpy are the same

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( state.x == state.y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DebugMessage( script_state_t& state, ai_state_t& self )
{
    // DebugMessage()
    /// @author ZZ
    /// @details This function spits out some useful numbers

    SCRIPT_FUNCTION_BEGIN();

    DisplayMsg_printf( "aistate %d, aicontent %d, target %d", self.state, self.content, REF_TO_INT( self.target ) );
    DisplayMsg_printf( "tmpx %d, tmpy %d", state.x, state.y );
    DisplayMsg_printf( "tmpdistance %d, tmpturn %d", state.distance, state.turn );
    DisplayMsg_printf( "tmpargument %d, selfturn %d", state.argument, pchr->ori.facing_z );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BlackTarget( script_state_t& state, ai_state_t& self )
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
Uint8 scr_SendMessageNear( script_state_t& state, ai_state_t& self )
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
        iTmp = std::fabs( pchr->getOldPosition()[kX] - camera->getTrackPosition()[kX] ) + std::fabs( pchr->getOldPosition()[kY] - camera->getTrackPosition()[kY] );

        if ( -1 == min_distance || iTmp < min_distance )
        {
            min_distance = iTmp;
        }
    }

    if ( min_distance < MSGDISTANCE )
    {
        returncode = _display_message( self.index, pchr->getProfileID(), state.argument, &state );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHitGround( script_state_t& state, ai_state_t& self )
{
    // IfHitGround()
    /// @author ZZ
    /// @details This function proceeds if a character hit the ground this update.
    /// Used to determine when to play the sound for a dropped item

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_HITGROUND );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfNameIsKnown( script_state_t& state, ai_state_t& self )
{
    // IfNameIsKnown()
    /// @author ZZ
    /// @details This function proceeds if the character's name is known

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->nameknown;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfUsageIsKnown( script_state_t& state, ai_state_t& self )
{
    // IfUsageIsKnown()
    /// @author ZZ
    /// @details This function proceeds if the character's usage is known

    SCRIPT_FUNCTION_BEGIN();

    returncode = ppro->isUsageKnown();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHoldingItemID( script_state_t& state, ai_state_t& self )
{
    // IfHoldingItemID( tmpargument = "idsz" )
    /// @author ZZ
    /// @details This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it

    SCRIPT_FUNCTION_BEGIN();

    returncode = (pchr->isWieldingItemIDSZ(state.argument) != nullptr);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHoldingRangedWeapon( script_state_t& state, ai_state_t& self )
{
    // IfHoldingRangedWeapon()
    /// @author ZZ
    /// @details This function passes if the character is holding a ranged weapon, returning
    /// the latch to press to use it.  This also checks ammo.

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    state.argument = 0;

    // Check right hand
    const std::shared_ptr<Object> &rightHandItem = _currentModule->getObjectHandler()[pchr->holdingwhich[SLOT_RIGHT]];

    if (rightHandItem)
    {
        if ( rightHandItem->getProfile()->isRangedWeapon() && (0 == rightHandItem->ammomax || (0 != rightHandItem->ammo)))
        {
            state.argument = LATCHBUTTON_RIGHT;
            returncode = true;
        }
    }

    //50% chance to check left hand even though we have already found one in our right hand
    if ( !returncode || Random::nextBool() )
    {
        // Check left hand
        const std::shared_ptr<Object> &leftHandItem = _currentModule->getObjectHandler()[pchr->holdingwhich[SLOT_LEFT]];
        if (leftHandItem)
        {
            if ( leftHandItem->getProfile()->isRangedWeapon() && (0 == leftHandItem->ammomax || (0 != leftHandItem->ammo)))
            {
                state.argument = LATCHBUTTON_LEFT;
                returncode = true;
            }
        }

    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHoldingMeleeWeapon( script_state_t& state, ai_state_t& self )
{
    // IfHoldingMeleeWeapon()
    /// @author ZZ
    /// @details This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    state.argument = 0;

    if ( !returncode )
    {
        // Check right hand
        const std::shared_ptr<Object> &rightItem = pchr->getRightHandItem();
        if (rightItem)
        {
            if ( !rightItem->getProfile()->isRangedWeapon() && rightItem->getProfile()->getWeaponAction() != ACTION_PA )
            {
                if ( 0 == state.argument || ( update_wld & 1 ) )
                {
                    state.argument = LATCHBUTTON_RIGHT;
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
                state.argument = LATCHBUTTON_LEFT;
                returncode = true;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHoldingShield( script_state_t& state, ai_state_t& self )
{
    // IfHoldingShield()
    /// @author ZZ
    /// @details This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it. The button will need to be held down.

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    state.argument = 0;

    if ( !returncode )
    {
        // Check right hand
        const std::shared_ptr<Object> &rightItem = pchr->getRightHandItem();
        if ( rightItem )
        {
            if ( rightItem->getProfile()->getWeaponAction() == ACTION_PA )
            {
                state.argument = LATCHBUTTON_RIGHT;
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
                state.argument = LATCHBUTTON_LEFT;
                returncode = true;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfKursed( script_state_t& state, ai_state_t& self )
{
    // IfKursed()
    /// @author ZZ
    /// @details This function proceeds if the character is kursed

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->iskursed;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsKursed( script_state_t& state, ai_state_t& self )
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
Uint8 scr_IfTargetIsDressedUp( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsDressedUp()
    /// @author ZZ
    /// @details This function proceeds if the target is dressed in fancy clothes

    SCRIPT_FUNCTION_BEGIN();

    returncode = ppro->getSkinInfo(pchr->skin).dressy;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfOverWater( script_state_t& state, ai_state_t& self )
{
    // IfOverWater()
    /// @author ZZ
    /// @details This function proceeds if the character is on a water tile

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->isOverWater(true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfThrown( script_state_t& state, ai_state_t& self )
{
    // IfThrown()
    /// @author ZZ
    /// @details This function proceeds if the character was thrown this update.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_THROWN );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeNameKnown( script_state_t& state, ai_state_t& self )
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
Uint8 scr_MakeUsageKnown( script_state_t& state, ai_state_t& self )
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
Uint8 scr_StopTargetMovement( script_state_t& state, ai_state_t& self )
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
    if ( pself_target->vel[kZ] > 0 ) pself_target->vel[kZ] = Ego::Physics::g_environment.gravity;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetXY( script_state_t& state, ai_state_t& self )
{
    // SetXY( tmpargument = "index", tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function sets one of the 8 permanent storage variable slots
    /// ( each of which holds an x,y pair )

    SCRIPT_FUNCTION_BEGIN();

    self.x[state.argument&STOR_AND] = state.x;
    self.y[state.argument&STOR_AND] = state.y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetXY( script_state_t& state, ai_state_t& self )
{
    // tmpx,tmpy = GetXY( tmpargument = "index" )
    /// @author ZZ
    /// @details This function reads one of the 8 permanent storage variable slots,
    /// setting tmpx and tmpy accordingly

    SCRIPT_FUNCTION_BEGIN();

    state.x = self.x[state.argument&STOR_AND];
    state.y = self.y[state.argument&STOR_AND];

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddXY( script_state_t& state, ai_state_t& self )
{
    // AddXY( tmpargument = "index", tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function alters the contents of one of the 8 permanent storage
    /// slots

    SCRIPT_FUNCTION_BEGIN();

    self.x[state.argument&STOR_AND] += state.x;
    self.y[state.argument&STOR_AND] += state.y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeAmmoKnown( script_state_t& state, ai_state_t& self )
{
    // MakeAmmoKnown()
    /// @author ZZ
    /// @details This function makes the character's ammo known ( for items )

    SCRIPT_FUNCTION_BEGIN();

    pchr->ammoknown = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedParticle( script_state_t& state, ai_state_t& self )
{
    // SpawnAttachedParticle( tmpargument = "particle", tmpdistance = "vertex" )
    /// @author ZZ
    /// @details This function spawns a particle attached to the character

    SCRIPT_FUNCTION_BEGIN();

    //If we are a weapon, our holder is the owner of this particle
	CHR_REF ichr = self.index;
	CHR_REF iholder = chr_get_lowest_attachment( ichr, true );
    if ( _currentModule->getObjectHandler().exists( iholder ) )
    {
        ichr = iholder;
    }

    returncode = nullptr != ParticleHandler::get().spawnLocalParticle(pchr->getPosition(), pchr->ori.facing_z, pchr->getProfileID(),
                                                                      LocalParticleProfileRef(state.argument), self.index,
                                                                      state.distance, pchr->team, ichr, INVALID_PRT_REF, 0,
                                                                      INVALID_CHR_REF);
    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactParticle( script_state_t& state, ai_state_t& self )
{
    // SpawnExactParticle( tmpargument = "particle", tmpx = "x", tmpy = "y", tmpdistance = "z" )
    /// @author ZZ
    /// @details This function spawns a particle at a specific x, y, z position

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = self.index;
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    {
		Vector3f vtmp =
			Vector3f
            (
				float(state.x),
				float(state.y),
				float(state.distance)
            );

        returncode = nullptr != ParticleHandler::get().spawnLocalParticle(vtmp, pchr->ori.facing_z, pchr->getProfileID(),
                                                         LocalParticleProfileRef(state.argument),
                                                         INVALID_CHR_REF, 0, pchr->team, ichr,
                                                         INVALID_PRT_REF, 0, INVALID_CHR_REF);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateTarget( script_state_t& state, ai_state_t& self )
{
    // AccelerateTarget( tmpx = "acc x", tmpy = "acc y" )
    /// @author ZZ
    /// @details This function changes the x and y speeds of the target

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->vel[kX] += state.x;
    pself_target->vel[kY] += state.y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfDistanceIsMoreThanTurn( script_state_t& state, ai_state_t& self )
{
    // IfDistanceIsMoreThanTurn()
    /// @author ZZ
    /// @details This function proceeds tmpdistance is greater than tmpturn

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( state.distance > state.turn );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfCrushed( script_state_t& state, ai_state_t& self )
{
    // IfCrushed()
    /// @author ZZ
    /// @details This function proceeds if the character was crushed in a passage this
    /// update.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_CRUSHED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeCrushValid( script_state_t& state, ai_state_t& self )
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
Uint8 scr_SetTargetToLowestTarget( script_state_t& state, ai_state_t& self )
{
    // SetTargetToLowestTarget()
    /// @author ZZ
    /// @details This function sets the target to the absolute bottom character.
    /// The holder of the target, or the holder of the holder of the target, or
    /// the holder of the holder of ther holder of the target, etc.   This function never fails

    SCRIPT_FUNCTION_BEGIN();

	auto itarget = chr_get_lowest_attachment( self.target, false );

    if ( _currentModule->getObjectHandler().exists( itarget ) )
    {
        SET_TARGET_0(ObjectRef(itarget));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfNotPutAway( script_state_t& state, ai_state_t& self )
{
    // IfNotPutAway()
    /// @author ZZ
    /// @details This function proceeds if the character couldn't be put into another
    /// character's pockets for some reason.
    /// It might be kursed or too big or something

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_NOTPUTAWAY );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTakenOut( script_state_t& state, ai_state_t& self )
{
    // IfTakenOut()
    /// @author ZZ
    /// @details This function proceeds if the character is equiped in another's inventory,
    /// and the holder tried to unequip it ( take it out of pack ), but the
    /// item was kursed and didn't cooperate

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_TAKENOUT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfAmmoOut( script_state_t& state, ai_state_t& self )
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
Uint8 scr_PlaySoundLooped( script_state_t& state, ai_state_t& self )
{
    // PlaySoundLooped( tmpargument = "sound", tmpdistance = "frequency" )

    /// @author ZZ
    /// @details This function starts playing a continuous sound

    SCRIPT_FUNCTION_BEGIN();

    SoundID sound = ppro->getSoundID(state.argument);
    
    if ( INVALID_SOUND_ID == sound )
    {
        // Stop existing sound loop (if any)
        AudioSystem::get().stopObjectLoopingSounds(self.index);
    }
    else
    {
        // check whatever might be playing on the channel now
        //ZF> TODO: check if character is already playing a looped sound first!
        AudioSystem::get().playSoundLooped(sound, self.index);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopSound( script_state_t& state, ai_state_t& self )
{
    // StopSound( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function stops the playing of a continuous sound!

    SCRIPT_FUNCTION_BEGIN();

    AudioSystem::get().stopObjectLoopingSounds(self.index, ppro->getSoundID(state.argument));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HealSelf( script_state_t& state, ai_state_t& self )
{
    // HealSelf()
    /// @author ZZ
    /// @details This function gives life back to the character.
    /// Values given as 8.8 fixed point
    /// This does NOT remove [HEAL] enchants ( poisons )
    /// This does not set the ALERTIF_HEALED alert

    SCRIPT_FUNCTION_BEGIN();

    pchr->heal(_currentModule->getObjectHandler()[pchr->getObjRef()], state.argument, true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Equip( script_state_t& state, ai_state_t& self )
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
Uint8 scr_IfTargetHasItemIDEquipped( script_state_t& state, ai_state_t& self )
{
    // IfTargetHasItemIDEquipped( tmpargument = "item idsz" )
    /// @author ZZ
    /// @details This function proceeds if the target already wearing a matching item

    SCRIPT_FUNCTION_BEGIN();

	auto iitem = Inventory::findItem( ObjectRef(self.target), state.argument, true );

    returncode = _currentModule->getObjectHandler().exists(iitem);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetOwnerToTarget( script_state_t& state, ai_state_t& self )
{
    // SetOwnerToTarget()
    /// @author ZZ
    /// @details This function must be called before enchanting anything.
    /// The owner is the character that pays the sustain costs and such for the enchantment

    SCRIPT_FUNCTION_BEGIN();

    self.owner = self.target;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToOwner( script_state_t& state, ai_state_t& self )
{
    // SetTargetToOwner()
    /// @author ZZ
    /// @details This function sets the target to whoever was previously declared as the
    /// owner.

    SCRIPT_FUNCTION_BEGIN();

    if ( _currentModule->getObjectHandler().exists( self.owner ) )
    {
        SET_TARGET_0(ObjectRef(self.owner));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetFrame( script_state_t& state, ai_state_t& self )
{
    // SetFrame( tmpargument = "frame" )
    /// @author ZZ
    /// @details This function sets the current .MD2 frame for the character.  Values are * 4

    SCRIPT_FUNCTION_BEGIN();

    uint16_t ilip   = state.argument & 3;
    int frame_along = state.argument >> 2;

    // resolve the requested action to a action that is valid for this model (if possible)
    const int action = pchr->getProfile()->getModel()->getAction(ACTION_DA);

    // set the action
    if ( rv_success == chr_set_action(pchr, action, true, true) )
    {
        // the action is set. now set the frame info.
        // pass along the imad in case the pchr->inst is not using this same mad
        // (corrupted data?)
        returncode = chr_instance_t::set_frame_full(pchr->inst, frame_along, ilip, pchr->getProfile()->getModel());
    }


    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BreakPassage( script_state_t& state, ai_state_t& self )
{
    // BreakPassage( tmpargument = "passage", tmpturn = "tile type", tmpdistance = "number of frames", tmpx = "borken tile", tmpy = "tile fx bits" )

    /// @author ZZ
    /// @details This function makes the tiles fall away ( turns into damage terrain )
    /// This function causes the tiles of a passage to increment if stepped on.
    /// tmpx and tmpy are both set to the location of whoever broke the tile if
    /// the function passed.

    SCRIPT_FUNCTION_BEGIN();

    returncode = BreakPassage( state.y, state.x, state.distance, state.turn, ( PASS_REF )state.argument, &( state.x ), &( state.y ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetReloadTime( script_state_t& state, ai_state_t& self )
{
    // SetReloadTime( tmpargument = "time" )
    /// @author ZZ
    /// @details This function stops a character from being used for a while.  Used
    /// by weapons to slow down their attack rate.  50 clicks per second.

    SCRIPT_FUNCTION_BEGIN();

    pchr->reload_timer = std::max( 0, state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToWideBlahID( script_state_t& state, ai_state_t& self )
{
    // SetTargetToWideBlahID( tmpargument = "idsz", tmpdistance = "blah bits" )
    /// @author ZZ
    /// @details This function sets the target to a character that matches the description,
    /// and who is located in the general vicinity of the character

    SCRIPT_FUNCTION_BEGIN();

    // Try to find one
    auto ichr = chr_find_target( pchr, WIDE, state.argument, state.distance );

    if ( _currentModule->getObjectHandler().exists( ichr ) )
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
Uint8 scr_PoofTarget( script_state_t& state, ai_state_t& self )
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
        if ( self.target == self.index )
        {
            // Poof self later
            self.poof_time = update_wld + 1;
        }
        else
        {
            // Poof others now
            pself_target->ai.poof_time = update_wld;

            SET_TARGET(ObjectRef(self.index), pself_target );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChildDoActionOverride( script_state_t& state, ai_state_t& self )
{
    // ChildDoActionOverride( tmpargument = action )

    /// @author ZZ
    /// @details This function lets a character set the action of the last character
    /// it spawned.  It also sets the current frame to the first frame of the
    /// action ( no interpolation from last frame ). If the cation is not valid for the model,
    /// the function will fail

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( _currentModule->getObjectHandler().exists( self.child ) )
    {
        Object * pchild = _currentModule->getObjectHandler().get( self.child );

        int action = pchild->getProfile()->getModel()->getAction(state.argument);

        if ( rv_success == chr_start_anim( pchild, action, false, true ) )
        {
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnPoof( script_state_t& state, ai_state_t& self )
{
    // SpawnPoof
    /// @author ZZ
    /// @details This function makes a lovely little poof at the character's location.
    /// The poof form and particle types are set in data.txt

    SCRIPT_FUNCTION_BEGIN();

    ParticleHandler::get().spawnPoof(pchr->toSharedPointer());

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetSpeedPercent( script_state_t& state, ai_state_t& self )
{
    // SetSpeedPercent( tmpargument = "percent" )
    /// @author ZZ
    /// @details This function acts like Run or Walk, except it allows the explicit
    /// setting of the speed

    SCRIPT_FUNCTION_BEGIN();

    self.maxSpeed = std::max(0.0f, state.argument / 100.0f);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetChildState( script_state_t& state, ai_state_t& self )
{
    // SetChildState( tmpargument = "state" )
    /// @author ZZ
    /// @details This function lets a character set the state of the last character it
    /// spawned

    SCRIPT_FUNCTION_BEGIN();

    if (INVALID_CHR_REF != self.child)
    {
        _currentModule->getObjectHandler()[self.child]->ai.state = state.argument;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedSizedParticle( script_state_t& state, ai_state_t& self )
{
    // SpawnAttachedSizedParticle( tmpargument = "particle", tmpdistance = "vertex", tmpturn = "size" )
    /// @author ZZ
    /// @details This function spawns a particle of the specific size attached to the
    /// character. For spell charging effects

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = self.index;
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    std::shared_ptr<Ego::Particle> particle = ParticleHandler::get().spawnLocalParticle(pchr->getPosition(), pchr->ori.facing_z, 
                                                                                        pchr->getProfileID(), LocalParticleProfileRef(state.argument), self.index,
                                                                                        state.distance, pchr->team, ichr, INVALID_PRT_REF, 0,
                                                                                        INVALID_CHR_REF);

    returncode = (particle != nullptr);

    if ( returncode )
    {
        particle->setSize(state.turn);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeArmor( script_state_t& state, ai_state_t& self )
{
    // ChangeArmor( tmpargument = "time" )
    /// @author ZZ
    /// @details This function changes the character's armor.
    /// Sets tmpargument as the old type and tmpx as the new type

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    state.x = state.argument;
    iTmp = pchr->skin;
    pchr->setSkin(state.argument);
    state.x = pchr->skin;
    state.argument = iTmp;  // The character's old armor

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowTimer( script_state_t& state, ai_state_t& self )
{
    // ShowTimer( tmpargument = "time" )
    /// @author ZZ
    /// @details This function sets the value displayed by the module timer.
    /// For races and such.  50 clicks per second

    SCRIPT_FUNCTION_BEGIN();

    timeron = true;
    timervalue = state.argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfFacingTarget( script_state_t& state, ai_state_t& self )
{
    // IfFacingTarget()
    /// @author ZZ
    /// @details This function proceeds if the character is more or less facing its
    /// target

    Object *  pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pchr->isFacingLocation(pself_target->getPosX(), pself_target->getPosY());

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySoundVolume( script_state_t& state, ai_state_t& self )
{
    // PlaySoundVolume( argument = "sound", distance = "volume" )
    /// @author ZZ
    /// @details This function sets the volume of a sound and plays it

    SCRIPT_FUNCTION_BEGIN();

    if ( state.distance > 0 )
    {
        int channel = AudioSystem::get().playSound(pchr->getOldPosition(), ppro->getSoundID(state.argument));

        if ( channel != INVALID_SOUND_CHANNEL )
        {
            Mix_Volume( channel, ( 128*state.distance ) / 100 );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedFacedParticle( script_state_t& state, ai_state_t& self )
{
    // SpawnAttachedFacedParticle(  tmpargument = "particle", tmpdistance = "vertex", tmpturn = "turn" )

    /// @author ZZ
    /// @details This function spawns a particle attached to the character, facing the
    /// same direction given by tmpturn

    SCRIPT_FUNCTION_BEGIN();

	CHR_REF ichr = self.index;
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    returncode = nullptr != ParticleHandler::get().spawnLocalParticle(pchr->getPosition(), CLIP_TO_16BITS( state.turn ),
                                                                      pchr->getProfileID(), LocalParticleProfileRef(state.argument),
                                                                      self.index, state.distance, pchr->team, ichr, INVALID_PRT_REF,
                                                                      0, INVALID_CHR_REF);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIsOdd( script_state_t& state, ai_state_t& self )
{
    // IfStateIsOdd()
    /// @author ZZ
    /// @details This function proceeds if the character's state is 1, 3, 5, 7, etc.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( self.state & 1 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToDistantEnemy( script_state_t& state, ai_state_t& self )
{
    // SetTargetToDistantEnemy( tmpdistance = "distance" )
    /// @author ZZ
    /// @details This function finds a character within a certain distance of the
    /// character, failing if there are none

    SCRIPT_FUNCTION_BEGIN();

    auto ichr = chr_find_target( pchr, state.distance, IDSZ_NONE, TARGET_ENEMIES );

    if ( _currentModule->getObjectHandler().exists( ichr ) )
    {
        SET_TARGET_0(ObjectRef(ichr));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Teleport( script_state_t& state, ai_state_t& self )
{
    // Teleport( tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function teleports the character to a new location, failing if
    /// the location is blocked or off the map

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->teleport(Vector3f(float(state.x), float(state.y), pchr->getPosZ()), pchr->ori.facing_z);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveStrengthToTarget( script_state_t& state, ai_state_t& self )
{
    // GiveStrengthToTarget()
    // Permanently boost the target's strength

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->isAlive() )
    {
        pself_target->increaseBaseAttribute(Ego::Attribute::MIGHT, FP8_TO_FLOAT(state.argument));
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveIntelligenceToTarget( script_state_t& state, ai_state_t& self )
{
    // GiveIntelligenceToTarget()
    // Permanently boost the target's intelligence

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->isAlive() )
    {
        pself_target->increaseBaseAttribute(Ego::Attribute::INTELLECT, FP8_TO_FLOAT(state.argument));
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveDexterityToTarget( script_state_t& state, ai_state_t& self )
{
    // GiveDexterityToTarget()
    // Permanently boost the target's dexterity

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->isAlive() )
    {
        pself_target->increaseBaseAttribute(Ego::Attribute::AGILITY, FP8_TO_FLOAT(state.argument));
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveLifeToTarget( script_state_t& state, ai_state_t& self )
{
    // GiveLifeToTarget()
    /// @author ZZ
    /// @details Permanently boost the target's life

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->isAlive() )
    {
        pself_target->increaseBaseAttribute(Ego::Attribute::MAX_LIFE, FP8_TO_FLOAT(state.argument));
        pself_target->heal(_currentModule->getObjectHandler()[pchr->getObjRef()], state.argument, true);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveManaToTarget( script_state_t& state, ai_state_t& self )
{
    // GiveManaToTarget()
    /// @author ZZ
    /// @details Permanently boost the target's mana

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->isAlive() )
    {
        pself_target->increaseBaseAttribute(Ego::Attribute::MAX_MANA, FP8_TO_FLOAT(state.argument));
        pself_target->costMana(-state.argument, INVALID_CHR_REF);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowMap( script_state_t& state, ai_state_t& self )
{
    // ShowMap()
    /// @author ZZ
    /// @details This function shows the module's map.
    /// Fails if map already visible

    SCRIPT_FUNCTION_BEGIN();
    if(_gameEngine->getActivePlayingState()->getMiniMap()->isVisible()) returncode = false;

    _gameEngine->getActivePlayingState()->getMiniMap()->setVisible(true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowYouAreHere( script_state_t& state, ai_state_t& self )
{
    // ShowYouAreHere()
    /// @author ZZ
    /// @details This function shows the blinking white blip on the map that represents the
    /// camera location

    SCRIPT_FUNCTION_BEGIN();

    _gameEngine->getActivePlayingState()->getMiniMap()->setShowPlayerPosition(true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowBlipXY( script_state_t& state, ai_state_t& self )
{
    // ShowBlipXY( tmpx = "x", tmpy = "y", tmpargument = "color" )

    /// @author ZZ
    /// @details This function draws a blip on the map, and must be done each update
    SCRIPT_FUNCTION_BEGIN();

    // Add a blip
    if ( state.argument >= 0 )
    {
        //_gameEngine->getActivePlayingState()->getMiniMap()->addBlip(state.x, state.y, static_cast<HUDColors>(state.argument % COLOR_MAX));
        _gameEngine->getActivePlayingState()->getMiniMap()->addBlip(state.x, state.y, _currentModule->getObjectHandler()[pchr->getObjRef()]);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HealTarget( script_state_t& state, ai_state_t& self )
{
    // HealTarget( tmpargument = "amount" )
    /// @author ZZ
    /// @details This function gives some life back to the target.
    /// Values are 8.8 fixed point. Any enchantments that are removed by [HEAL], like poison, go away

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[self.target];
    if(!target) {
        return false;
    }

    returncode = false;
    if ( target->heal(_currentModule->getObjectHandler()[self.index], state.argument, false) )
    {
        returncode = true;
        target->removeEnchantsWithIDSZ(MAKE_IDSZ('H', 'E', 'A', 'L'));
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PumpTarget( script_state_t& state, ai_state_t& self )
{
    // PumpTarget( tmpargument = "amount" )
    /// @author ZZ
    /// @details This function gives some mana back to the target.
    /// Values are 8.8 fixed point

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->isAlive() && state.argument > 0)
    {
        pself_target->costMana(-state.argument, pchr->getObjRef().get());
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostAmmo( script_state_t& state, ai_state_t& self )
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
Uint8 scr_MakeSimilarNamesKnown( script_state_t& state, ai_state_t& self )
{
    // MakeSimilarNamesKnown()
    /// @author ZZ
    /// @details This function makes the names of similar objects known.
    /// Checks all 6 IDSZ types to make sure they match.

    int tTmp;
    Uint16 sTmp = 0;

    SCRIPT_FUNCTION_BEGIN();

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
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
Uint8 scr_SpawnAttachedHolderParticle( script_state_t& state, ai_state_t& self )
{
    // SpawnAttachedHolderParticle( tmpargument = "particle", tmpdistance = "vertex" )

    /// @author ZZ
    /// @details This function spawns a particle attached to the character's holder, or to the character if no holder

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = self.index;
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    returncode = nullptr != ParticleHandler::get().spawnLocalParticle(pchr->getPosition(), pchr->ori.facing_z, pchr->getProfileID(),
                                                                      LocalParticleProfileRef(state.argument), ichr,
                                                                      state.distance, pchr->team, ichr, INVALID_PRT_REF, 0,
                                                                      INVALID_CHR_REF);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetReloadTime( script_state_t& state, ai_state_t& self )
{
    // SetTargetReloadTime( tmpargument = "time" )

    /// @author ZZ
    /// @details This function sets the target's reload time
    /// This function stops the target from attacking for a while.

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( state.argument > 0 )
    {
        pself_target->reload_timer = CLIP( state.argument, 0, 0xFFFF );
    }
    else
    {
        pself_target->reload_timer = 0;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetFogLevel( script_state_t& state, ai_state_t& self )
{
    // SetFogLevel( tmpargument = "level" )
    /// @author ZZ
    /// @details This function sets the level of the module's fog.
    /// Values are * 10
    /// !!BAD!! DOESN'T WORK !!BAD!!

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( state.argument / 10.0f ) - fog._top;
    fog._top += fTmp;
    fog._distance += fTmp;
    fog._on = egoboo_config_t::get().graphic_fog_enable.getValue();
	if (fog._distance < 1.0f)  fog._on = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetFogLevel( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetFogLevel()
    /// @author ZZ
    /// @details This function sets tmpargument to the level of the module's fog.
    /// Values are * 10

    SCRIPT_FUNCTION_BEGIN();

    state.argument = fog._top * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetFogTAD( script_state_t& state, ai_state_t& self )
{
    /// @author ZZ
    /// @details This function sets the color of the module's fog.
    /// TAD stands for <turn, argument, distance> == <red, green, blue>.
    /// Makes sense, huh?
    /// !!BAD!! DOESN'T WORK !!BAD!!

    SCRIPT_FUNCTION_BEGIN();

	fog._red = CLIP(state.turn, 0, 0xFF);
	fog._grn = CLIP(state.argument, 0, 0xFF);
	fog._blu = CLIP(state.distance, 0, 0xFF);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetFogBottomLevel( script_state_t& state, ai_state_t& self )
{
    // SetFogBottomLevel( tmpargument = "level" )

    /// @author ZZ
    /// @details This function sets the level of the module's fog.
    /// Values are * 10

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

	fTmp = (state.argument / 10.0f) - fog._bottom;
    fog._bottom += fTmp;
    fog._distance -= fTmp;
    fog._on = egoboo_config_t::get().graphic_fog_enable.getValue();
	if (fog._distance < 1.0f)  fog._on = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetFogBottomLevel( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetFogBottomLevel()

    /// @author ZZ
    /// @details This function sets tmpargument to the level of the module's fog.
    /// Values are * 10

    SCRIPT_FUNCTION_BEGIN();

    state.argument = fog._bottom * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CorrectActionForHand( script_state_t& state, ai_state_t& self )
{
    // CorrectActionForHand( tmpargument = "action" )
    /// @author ZZ
    /// @details This function changes tmpargument according to which hand the character
    /// is held in It turns ZA into ZA, ZB, ZC, or ZD.
    /// USAGE:  wizards casting spells

    SCRIPT_FUNCTION_BEGIN();
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        if ( pchr->inwhich_slot == SLOT_LEFT )
        {
            // A or B
            state.argument += Random::next(1);
        }
        else
        {
            // C or D
            state.argument += 2 + Random::next(1);
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsMounted( script_state_t& state, ai_state_t& self )
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
    if ( _currentModule->getObjectHandler().exists( ichr ) )
    {
        returncode = _currentModule->getObjectHandler().get(ichr)->isMount();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SparkleIcon( script_state_t& state, ai_state_t& self )
{
    // SparkleIcon( tmpargument = "color" )
    /// @author ZZ
    /// @details This function starts little sparklies going around the character's icon

    SCRIPT_FUNCTION_BEGIN();
    if ( state.argument < COLOR_MAX )
    {
        if ( state.argument < -1 )
        {
            pchr->sparkle = NOSPARKLE;
        }
        else
        {
            pchr->sparkle = state.argument % COLOR_MAX;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnsparkleIcon( script_state_t& state, ai_state_t& self )
{
    // UnsparkleIcon()
    /// @author ZZ
    /// @details This function stops little sparklies going around the character's icon

    SCRIPT_FUNCTION_BEGIN();

    pchr->sparkle = NOSPARKLE;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetTileXY( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetTileXY( tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function sets tmpargument to the tile type at the specified
    /// coordinates

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    TileIndex idx = _currentModule->getMeshPointer()->getTileIndex(PointWorld(state.x, state.y));

    const ego_tile_info_t& ptr = _currentModule->getMeshPointer()->getTileInfo(idx);
    returncode = true;
    state.argument = ptr._img & TILE_LOWER_MASK;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTileXY( script_state_t& state, ai_state_t& self )
{
    // SetTileXY( tmpargument = "tile type", tmpx = "x", tmpy = "y" )
    /// @author ZZ
    /// @details This function changes the tile type at the specified coordinates

    SCRIPT_FUNCTION_BEGIN();

	auto mesh = _currentModule->getMeshPointer();
	if (!mesh) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == mesh");
	}

    TileIndex index = mesh->getTileIndex(PointWorld(state.x, state.y));
    returncode = mesh->set_texture( index, state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetShadowSize( script_state_t& state, ai_state_t& self )
{
    // SetShadowSize( tmpargument = "size" )
    /// @author ZZ
    /// @details This function makes the character's shadow bigger or smaller

    SCRIPT_FUNCTION_BEGIN();

    pchr->shadow_size     = state.argument * pchr->fat;
    pchr->shadow_size_save = state.argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OrderTarget( script_state_t& state, ai_state_t& self )
{
    // OrderTarget( tmpargument = "order" )
    /// @author ZZ
    /// @details This function issues an order to the given target
    /// Be careful in using this, always checking IDSZ first

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !_currentModule->getObjectHandler().exists( self.target ) )
    {
        returncode = false;
    }
    else
    {
        returncode = ai_state_t::add_order(pself_target->ai, state.argument, 0);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToWhoeverIsInPassage( script_state_t& state, ai_state_t& self )
{
    // SetTargetToWhoeverIsInPassage()
    /// @author ZZ
    /// @details This function sets the target to whoever is blocking the given passage
    /// This function lets passage rectangles be used as event triggers

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = _currentModule->getPassageByID(state.argument);

    returncode = false;
    if(passage)
    {
        auto objRef = passage->whoIsBlockingPassage(ObjectRef(self.index), IDSZ_NONE, TARGET_SELF | TARGET_FRIENDS | TARGET_ENEMIES, IDSZ_NONE);

        if (_currentModule->getObjectHandler().exists(objRef))
        {
            SET_TARGET_0(objRef);
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfCharacterWasABook( script_state_t& state, ai_state_t& self )
{
    // IfCharacterWasABook()
    /// @author ZZ
    /// @details This function proceeds if the base model is the same as the current
    /// model or if the base model is SPELLBOOK
    /// USAGE: USED BY THE MORPH SPELL. Not much use elsewhere

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pchr->basemodel_ref == SPELLBOOK ||
                   pchr->basemodel_ref == pchr->getProfileID() );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetEnchantBoostValues( script_state_t& state, ai_state_t& self )
{
    // SetEnchantBoostValues( tmpargument = "owner mana regen", tmpdistance = "owner life regen", tmpx = "target mana regen", tmpy = "target life regen" )
    /// @author ZZ
    /// @details This function sets the mana and life drains for the last enchantment
    /// spawned by this character.
    /// Values are 8.8 fixed point

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if(!pchr->getActiveEnchants().empty()) {
        const std::shared_ptr<Ego::Enchantment> &enchant = pchr->getActiveEnchants().front();
        if(!enchant->isTerminated()) {
            enchant->setBoostValues(FP8_TO_FLOAT(state.argument), FP8_TO_FLOAT(state.distance), FP8_TO_FLOAT(state.x), FP8_TO_FLOAT(state.y));
            returncode = true;            
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnCharacterXYZ( script_state_t& state, ai_state_t& self )
{
    // SpawnCharacterXYZ( tmpx = "x", tmpy = "y", tmpdistance = "z", tmpturn = "turn" )
    /// @author ZZ
    /// @details This function spawns a character of the same type at a specific location, failing if x,y,z is invalid

    SCRIPT_FUNCTION_BEGIN();

	Vector3f pos = Vector3f(float(state.x), float(state.y), float(state.distance));

    std::shared_ptr<Object> pchild = _currentModule->spawnObject( pos, pchr->getProfileID(), pchr->team, 0, CLIP_TO_16BITS( state.turn ), "", INVALID_CHR_REF );
    if (pchild == nullptr)
    {
		Log::get().warn("%s:%d: object %s failed to spawn a copy of itself\n", __FILE__, __LINE__, pchr->getName().c_str() );
        returncode = false;
    }
    else
    {
        self.child = pchild->getObjRef().get();

        pchild->iskursed   = pchr->iskursed;  /// @note BB@> inherit this from your spawner
        pchild->ai.passage = self.passage;
        pchild->ai.owner   = self.owner;

        pchild->dismount_timer  = PHYS_DISMOUNT_TIME;
        pchild->dismount_object = self.index;
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactCharacterXYZ( script_state_t& state, ai_state_t& self )
{
    // SpawnCharacterXYZ( tmpargument = "slot", tmpx = "x", tmpy = "y", tmpdistance = "z", tmpturn = "turn" )
    /// @author ZZ
    /// @details This function spawns a character at a specific location, using a
    /// specific model type, failing if x,y,z is invalid
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS,
    /// AS THE MODEL SLOTS MAY VARY FROM MODULE TO MODULE.

    SCRIPT_FUNCTION_BEGIN();

	Vector3f pos =
		Vector3f
        (
			float(state.x),
			float(state.y),
			float(state.distance)
        );

    const std::shared_ptr<Object> pchild = _currentModule->spawnObject(pos, static_cast<PRO_REF>(state.argument), pchr->team, 0, CLIP_TO_16BITS(state.turn), "", INVALID_CHR_REF);

    if ( !pchild )
    {
        returncode = false;
    }
    else
    {
        // was the child spawned in a "safe" spot?
        if (!pchild->hasSafePosition())
        {
			Log::get().warn( "Object %s failed to spawn object (no safe location)\n", pchr->getName().c_str() );
            pchr->requestTerminate();
            returncode = false;
        }
        else
        {
            self.child = pchild->getObjRef().get();

            pchild->iskursed   = pchr->iskursed;  /// @note BB@> inherit this from your spawner
            pchild->ai.passage = self.passage;
            pchild->ai.owner   = self.owner;

            pchild->dismount_timer  = PHYS_DISMOUNT_TIME;
            pchild->dismount_object = self.index;
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTargetClass( script_state_t& state, ai_state_t& self )
{
    // ChangeTargetClass( tmpargument = "slot" )

    /// @author ZZ
    /// @details This function changes the target character's model slot.
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS, AS THE MODEL SLOTS MAY VARY FROM
    /// MODULE TO MODULE.
    /// USAGE: This is intended as a way to incorporate more player classes into the game.

    SCRIPT_FUNCTION_BEGIN();

    const PRO_REF profileID = static_cast<PRO_REF>(state.argument);

    /// @details This function polymorphs a character permanently so that it can be exported properly
    /// A character turned into a frog with this function will also export as a frog!
    if(ProfileSystem::get().isValidProfileID(profileID)) 
    {
        //Change the object
        pchr->polymorphObject(profileID, 0);

        // set the base model to the new model, too
        pchr->basemodel_ref = profileID;

        returncode = true;
    }
    else {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlayFullSound( script_state_t& state, ai_state_t& self )
{
    // PlayFullSound( tmpargument = "sound", tmpdistance = "frequency" )
    /// @author ZZ
    /// @details This function plays one of the character's sounds .
    /// The sound will be heard at full volume by all players (Victory music)

    SCRIPT_FUNCTION_BEGIN();

    AudioSystem::get().playSoundFull(ppro->getSoundID(state.argument));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactChaseParticle( script_state_t& state, ai_state_t& self )
{
    // SpawnExactChaseParticle( tmpargument = "particle", tmpx = "x", tmpy = "y", tmpdistance = "z" )
    /// @author ZZ
    /// @details This function spawns a particle at a specific x, y, z position,
    /// that will home in on the character's target

    std::shared_ptr<Ego::Particle> particle;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = self.index;
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    {
		Vector3f vtmp =
			Vector3f
            (
				float(state.x),
				float(state.y),
				float(state.distance)
            );

        particle = ParticleHandler::get().spawnLocalParticle(vtmp, pchr->ori.facing_z, pchr->getProfileID(),
                                                             LocalParticleProfileRef(state.argument),
                                                             INVALID_CHR_REF, 0, pchr->team, ichr, INVALID_PRT_REF,
                                                             0, INVALID_CHR_REF);
    }

    returncode = (particle != nullptr);

    if ( returncode )
    {
        particle->setTarget(self.target);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CreateOrder( script_state_t& state, ai_state_t& self )
{
    // tmpargument = CreateOrder( tmpx = "value1", tmpy = "value2", tmpargument = "order" )

    /// @author ZZ
    /// @details This function compresses tmpx, tmpy, tmpargument ( 0 - 15 ), and the
    /// character's target into tmpargument.  This new tmpargument can then
    /// be issued as an order to teammates.  TranslateOrder will undo the
    /// compression

    Uint16 sTmp = 0;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = ( REF_TO_INT( self.target ) & 0x00FF ) << 24;
    sTmp |= (( state.x >> 6 ) & 0x03FF ) << 14;
    sTmp |= (( state.y >> 6 ) & 0x03FF ) << 4;
    sTmp |= ( state.argument & 0x000F );
    state.argument = sTmp;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OrderSpecialID( script_state_t& state, ai_state_t& self )
{
    // OrderSpecialID( tmpargument = "compressed order", tmpdistance = "idsz" )
    /// @author ZZ
    /// @details This function orders all characters with the given special IDSZ.

    SCRIPT_FUNCTION_BEGIN();

    issue_special_order( state.argument, state.distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkurseTargetInventory( script_state_t& state, ai_state_t& self )
{
    // UnkurseTargetInventory()
    /// @author ZZ
    /// @details This function unkurses all items held and in the pockets of the target

    CHR_REF ichr;
    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ichr = pself_target->holdingwhich[SLOT_LEFT];
    if ( _currentModule->getObjectHandler().exists( ichr ) )
    {
        _currentModule->getObjectHandler().get(ichr)->iskursed = false;
    }

    ichr = pself_target->holdingwhich[SLOT_RIGHT];
    if ( _currentModule->getObjectHandler().exists( ichr ) )
    {
        _currentModule->getObjectHandler().get(ichr)->iskursed = false;
    }

    for(const std::shared_ptr<Object> pitem : pchr->getInventory().iterate())
    {
        pitem->iskursed = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsSneaking( script_state_t& state, ai_state_t& self )
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
Uint8 scr_DropItems( script_state_t& state, ai_state_t& self )
{
    // DropItems()
    /// @author ZZ
    /// @details This function drops all of the items the character is holding

    SCRIPT_FUNCTION_BEGIN();

    pchr->dropAllItems();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RespawnTarget( script_state_t& state, ai_state_t& self )
{
    // RespawnTarget()
    /// @author ZZ
    /// @details This function respawns the target at its current location

    SCRIPT_FUNCTION_BEGIN();

	Object *pself_target;
    SCRIPT_REQUIRE_TARGET( pself_target );
	Vector3f save_pos = pself_target->getPosition();
    pself_target->respawn();
    pself_target->setPosition(save_pos);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDoActionSetFrame( script_state_t& state, ai_state_t& self )
{
    // TargetDoActionSetFrame( tmpargument = "action" )
    /// @author ZZ
    /// @details This function starts the target doing the given action, and also sets
    /// the starting frame to the first of the animation ( so there is no
    /// interpolation 'cause it looks awful in some circumstances )
    /// It will fail if the action is invalid

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    if ( _currentModule->getObjectHandler().exists( self.target ) )
    {
        int action;
        Object * pself_target = _currentModule->getObjectHandler().get( self.target );

        action = pself_target->getProfile()->getModel()->getAction(state.argument );

        if ( rv_success == chr_start_anim( pself_target, action, false, true ) )
        {
            // remove the interpolation
            chr_instance_t::remove_interpolation(pself_target->inst);

            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetCanSeeInvisible( script_state_t& state, ai_state_t& self )
{
    // IfTargetCanSeeInvisible()
    /// @author ZZ
    /// @details This function proceeds if the target can see invisible

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->canSeeInvisible();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToNearestBlahID( script_state_t& state, ai_state_t& self )
{
    // SetTargetToNearestBlahID( tmpargument = "idsz", tmpdistance = "blah bits" )

    /// @author ZZ
    /// @details This function finds the NEAREST ( exact ) character that fits the given
    /// parameters, failing if it finds none

    SCRIPT_FUNCTION_BEGIN();

    // Try to find one
    auto ichr = chr_find_target( pchr, NEAREST, state.argument, state.distance );

    if ( _currentModule->getObjectHandler().exists( ichr ) )
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
Uint8 scr_SetTargetToNearestEnemy( script_state_t& state, ai_state_t& self )
{
    // SetTargetToNearestEnemy()
    /// @author ZZ
    /// @details This function finds the NEAREST ( exact ) enemy, failing if it finds none

    SCRIPT_FUNCTION_BEGIN();

    auto ichr = chr_find_target( pchr, NEAREST, IDSZ_NONE, TARGET_ENEMIES );

    if ( _currentModule->getObjectHandler().exists( ichr ) )
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
Uint8 scr_SetTargetToNearestFriend( script_state_t& state, ai_state_t& self )
{
    // SetTargetToNearestFriend()
    /// @author ZZ
    /// @details This function finds the NEAREST ( exact ) friend, failing if it finds none

    SCRIPT_FUNCTION_BEGIN();

    auto ichr = chr_find_target( pchr, NEAREST, IDSZ_NONE, TARGET_FRIENDS );

    if ( _currentModule->getObjectHandler().exists( ichr ) )
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
Uint8 scr_SetTargetToNearestLifeform( script_state_t& state, ai_state_t& self )
{
    // SetTargetToNearestLifeform()

    /// @author ZZ
    /// @details This function finds the NEAREST ( exact ) friend or enemy, failing if it
    /// finds none

    SCRIPT_FUNCTION_BEGIN();

    auto ichr = chr_find_target( pchr, NEAREST, IDSZ_NONE, TARGET_ITEMS | TARGET_FRIENDS | TARGET_ENEMIES );

    if ( _currentModule->getObjectHandler().exists( ichr ) )
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
Uint8 scr_FlashPassage( script_state_t& state, ai_state_t& self )
{
    // FlashPassage( tmpargument = "passage", tmpdistance = "color" )

    /// @author ZZ
    /// @details This function makes the given passage light or dark.
    /// Usage: For debug purposes

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = _currentModule->getPassageByID(state.argument);
    if(passage) {
        passage->flashColor(state.distance);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FindTileInPassage( script_state_t& state, ai_state_t& self )
{
    // tmpx, tmpy = FindTileInPassage( tmpargument = "passage", tmpdistance = "tile type", tmpx, tmpy )

    /// @author ZZ
    /// @details This function finds all tiles of the specified type that lie within the
    /// given passage.  Call multiple times to find multiple tiles.  tmpx and
    /// tmpy will be set to the middle of the found tile if one is found, or
    /// both will be set to 0 if no tile is found.
    /// tmpx and tmpy are required and set on return

    SCRIPT_FUNCTION_BEGIN();

    returncode = FindTileInPassage( state.x, state.y, state.distance, ( PASS_REF )state.argument, &( state.x ), &( state.y ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHeldInLeftHand( script_state_t& state, ai_state_t& self )
{
    // IfHeldInLeftHand()
    /// @author ZZ
    /// @details This function passes if another character is holding the character in its
    /// left hand.
    /// Usage: Used mostly by enchants that target the item of the other hand

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    const std::shared_ptr<Object> holder = _currentModule->getObjectHandler()[pchr->attachedto];
    if (holder)
    {
        returncode = holder->holdingwhich[SLOT_LEFT] == pchr->getObjRef().get();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotAnItem( script_state_t& state, ai_state_t& self )
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
Uint8 scr_SetChildAmmo( script_state_t& state, ai_state_t& self )
{
    // SetChildAmmo( tmpargument = "none" )
    /// @author ZZ
    /// @details This function sets the ammo of the last character spawned by this character

    SCRIPT_FUNCTION_BEGIN();

    _currentModule->getObjectHandler().get(self.child)->ammo = CLIP( state.argument, 0, 0xFFFF );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHitVulnerable( script_state_t& state, ai_state_t& self )
{
    // IfHitVulnerable()
    /// @author ZZ
    /// @details This function proceeds if the character was hit by a weapon of its
    /// vulnerability IDSZ.
    /// For example, a werewolf gets hit by a [SILV] bullet.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_HITVULNERABLE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsFlying( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsFlying()
    /// @author ZZ
    /// @details This function proceeds if the character target is flying

    SCRIPT_FUNCTION_BEGIN();

	Object *pself_target;
    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->isFlying();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IdentifyTarget( script_state_t& state, ai_state_t& self )
{
    // IdentifyTarget()
    /// @author ZZ
    /// @details This function reveals the target's name, ammo, and usage
    /// Proceeds if the target was unknown

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
	CHR_REF ichr = self.target;
    if ( _currentModule->getObjectHandler().get(ichr)->ammomax != 0 )  _currentModule->getObjectHandler().get(ichr)->ammoknown = true;


    returncode = !_currentModule->getObjectHandler().get(ichr)->nameknown;
    _currentModule->getObjectHandler().get(ichr)->nameknown = true;
    ppro->makeUsageKnown();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BeatModule( script_state_t& state, ai_state_t& self )
{
    // BeatModule()
    /// @author ZZ
    /// @details This function displays the Module Ended message

    SCRIPT_FUNCTION_BEGIN();

    _currentModule->beatModule();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EndModule( script_state_t& state, ai_state_t& self )
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
Uint8 scr_DisableExport( script_state_t& state, ai_state_t& self )
{
    // DisableExport()
    /// @author ZZ
    /// @details This function turns export off

    SCRIPT_FUNCTION_BEGIN();

    _currentModule->setExportValid(false);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableExport( script_state_t& state, ai_state_t& self )
{
    // EnableExport()
    /// @author ZZ
    /// @details This function turns export on

    SCRIPT_FUNCTION_BEGIN();

    _currentModule->setExportValid(true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetTargetState( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetTargetState()
    /// @author ZZ
    /// @details This function sets tmpargument to the state of the target

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    state.argument = pself_target->ai.state;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfEquipped( script_state_t& state, ai_state_t& self )
{
    // This proceeds if the character is equipped

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->isequipped;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropTargetMoney( script_state_t& state, ai_state_t& self )
{
    // DropTargetMoney( tmpargument = "amount" )
    /// @author ZZ
    /// @details This function drops some of the target's money

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->dropMoney(state.argument);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetTargetContent( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetTargetContent()
    // This sets tmpargument to the current Target's content value

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    state.argument = pself_target->ai.content;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropTargetKeys( script_state_t& state, ai_state_t& self )
{
    // DropTargetKeys()
    /// @author ZZ
    /// @details This function makes the Target drops keys in inventory (Not inhand)

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->dropKeys();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinTeam( script_state_t& state, ai_state_t& self )
{
    // JoinTeam( tmpargument = "team" )
    /// @author ZZ
    /// @details This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)

    SCRIPT_FUNCTION_BEGIN();

    pchr->setTeam(static_cast<TEAM_REF>(state.argument));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetJoinTeam( script_state_t& state, ai_state_t& self )
{
    // TargetJoinTeam( tmpargument = "team" )
    /// @author ZZ
    /// @details This makes the Target join a Team specified in tmpargument (A = 0, 25 = Z, etc.)

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[self.target];
    if(target) {
        target->setTeam(static_cast<TEAM_REF>(state.argument));
        returncode = true;
    }
    else {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearMusicPassage( script_state_t& state, ai_state_t& self )
{
    // ClearMusicPassage( tmpargument = "passage" )
    /// @author ZZ
    /// @details This clears the music for a specified passage

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = _currentModule->getPassageByID(state.argument);
    if(passage) {
        passage->setMusic(Passage::NO_MUSIC);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearEndMessage( script_state_t& state, ai_state_t& self )
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
Uint8 scr_AddEndMessage( script_state_t& state, ai_state_t& self )
{
    // AddEndMessage( tmpargument = "message" )
    /// @author ZZ
    /// @details This function appends a message to the end-module text buffer

    SCRIPT_FUNCTION_BEGIN();

    returncode = AddEndMessage( pchr,  state.argument, &state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlayMusic( script_state_t& state, ai_state_t& self )
{
    // PlayMusic( tmpargument = "song number", tmpdistance = "fade time (msec)" )
    /// @author ZZ
    /// @details This function begins playing a new track of music

    SCRIPT_FUNCTION_BEGIN();

    int fadeTime = state.distance;
    if(fadeTime < 0) fadeTime = 0;

    AudioSystem::get().playMusic(state.argument, fadeTime);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetMusicPassage( script_state_t& state, ai_state_t& self )
{
    // SetMusicPassage( tmpargument = "passage", tmpturn = "type", tmpdistance = "repetitions" )

    /// @author ZZ
    /// @details This function makes the given passage play music if a player enters it
    /// tmpargument is the passage to set and tmpdistance is the music track to play.

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = _currentModule->getPassageByID(state.argument);
    if(passage) {
        passage->setMusic(state.distance);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeCrushInvalid( script_state_t& state, ai_state_t& self )
{
    // MakeCrushInvalid()
    /// @author ZZ
    /// @details This function makes doors unable to close on this object

    SCRIPT_FUNCTION_BEGIN();

    pchr->canbecrushed = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopMusic( script_state_t& state, ai_state_t& self )
{
    // StopMusic()
    /// @author ZZ
    /// @details This function stops the interactive music

    SCRIPT_FUNCTION_BEGIN();

    AudioSystem::get().stopMusic();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashVariable( script_state_t& state, ai_state_t& self )
{
    // FlashVariable( tmpargument = "amount" )

    /// @author ZZ
    /// @details This function makes the character flash according to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    FlashObject( pchr, state.argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateUp( script_state_t& state, ai_state_t& self )
{
    // AccelerateUp( tmpargument = "acc z" )
    /// @author ZZ
    /// @details This function makes the character accelerate up and down

    SCRIPT_FUNCTION_BEGIN();

    pchr->vel[kZ] += state.argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashVariableHeight( script_state_t& state, ai_state_t& self )
{
    // FlashVariableHeight( tmpturn = "intensity bottom", tmpx = "bottom", tmpdistance = "intensity top", tmpy = "top" )
    /// @author ZZ
    /// @details This function makes the character flash, feet one color, head another.
    ///          This function sets a character's lighting depending on vertex height...
    ///          Can make feet dark and head light...

    SCRIPT_FUNCTION_BEGIN();

    const uint8_t valuelow = CLIP_TO_16BITS(state.turn);
    const int16_t low = state.x;
    const uint8_t valuehigh = state.distance;
    const int16_t high = state.y;

    for (size_t cnt = 0; cnt < pchr->inst.vrt_count; cnt++)
    {
        int16_t z = pchr->inst.vrt_lst[cnt].pos[ZZ];

        if ( z < low )
        {
            pchr->inst.vrt_lst[cnt].col[RR] =
                pchr->inst.vrt_lst[cnt].col[GG] =
                    pchr->inst.vrt_lst[cnt].col[BB] = valuelow;
        }
        else if ( z > high )
        {
            pchr->inst.vrt_lst[cnt].col[RR] =
                pchr->inst.vrt_lst[cnt].col[GG] =
                    pchr->inst.vrt_lst[cnt].col[BB] = valuehigh;
        }
        else if ( high != low )
        {
            uint8_t valuemid = ( valuehigh * ( z - low ) / ( high - low ) ) +
                             ( valuelow * ( high - z ) / ( high - low ) );

            pchr->inst.vrt_lst[cnt].col[RR] =
                pchr->inst.vrt_lst[cnt].col[GG] =
                    pchr->inst.vrt_lst[cnt].col[BB] =  valuemid;
        }
        else
        {
            // z == high == low
            uint8_t valuemid = ( valuehigh + valuelow ) * 0.5f;

            pchr->inst.vrt_lst[cnt].col[RR] =
                pchr->inst.vrt_lst[cnt].col[GG] =
                    pchr->inst.vrt_lst[cnt].col[BB] =  valuemid;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetDamageTime( script_state_t& state, ai_state_t& self )
{
    // SetDamageTime( tmpargument = "time" )
    /// @author ZZ
    /// @details This function makes the character invincible for a little while

    SCRIPT_FUNCTION_BEGIN();

    pchr->damage_timer = CLIP( state.argument, 0, 0xFFFF );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs8( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 8 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs9( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 9 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs10( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 10 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs11( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 11 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs12( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 12 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs13( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 13 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs14( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 14 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStateIs15( script_state_t& state, ai_state_t& self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 15 == self.state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsAMount( script_state_t& state, ai_state_t& self )
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
Uint8 scr_IfTargetIsAPlatform( script_state_t& state, ai_state_t& self )
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
Uint8 scr_AddStat( script_state_t& state, ai_state_t& self )
{
    // AddStat()
    /// @author ZZ
    /// @details This function turns on an NPC's status display

    SCRIPT_FUNCTION_BEGIN();

    _gameEngine->getActivePlayingState()->addStatusMonitor( _currentModule->getObjectHandler()[self.index] );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisenchantTarget( script_state_t& state, ai_state_t& self )
{
    // DisenchantTarget()
    /// @author ZZ
    /// @details This function removes all enchantments on the Target character, proceeding
    /// if there were any, failing if not

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->disenchant();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisenchantAll( script_state_t& state, ai_state_t& self )
{
    // DisenchantAll()
    /// @author ZZ
    /// @details This function removes all enchantments in the game

    SCRIPT_FUNCTION_BEGIN();

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator()) {
        for(const std::shared_ptr<Ego::Enchantment> &enchant : object->getActiveEnchants()) {
            enchant->requestTerminate();
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetVolumeNearestTeammate( script_state_t& state, ai_state_t& self )
{
    // SetVolumeNearestTeammate( tmpargument = "sound", tmpdistance = "distance" )
    /// @author ZZ
    /// @details This function lets insects buzz correctly.  The closest Team member
    /// is used to determine the overall sound level.

    SCRIPT_FUNCTION_BEGIN();

    //ZF> TODO: Not implemented

    /*PORT
    if(moduleactive && state.distance >= 0)
    {
    // Find the closest Teammate
    iTmp = 10000;
    sTmp = 0;
    while(sTmp < OBJECTS_MAX)
    {
    if(_currentModule->getObjectHandler().exists(sTmp) && ChrList.lst[sTmp].alive && ChrList.lst[sTmp].Team == pchr->Team)
    {
    distance = ABS(PCamera->track.x-ChrList.lst[sTmp].getOldPosition().x)+ABS(PCamera->track.y-ChrList.lst[sTmp].getOldPosition().y);
    if(distance < iTmp)  iTmp = distance;
    }
    sTmp++;
    }
    distance=iTmp+state.distance;
    volume = -distance;
    volume = volume<<VOLSHIFT;
    if(volume < VOLMIN) volume = VOLMIN;
    iTmp = CapStack.lst[pro_get_icap(pchr->getProfileID())].wavelist[pstate->argument];
    if(iTmp < numsound && iTmp >= 0 && soundon)
    {
    lpDSBuffer[iTmp]->SetVolume(volume);
    }
    }
    */

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddShopPassage( script_state_t& state, ai_state_t& self )
{
    // AddShopPassage( tmpargument = "passage" )
    /// @author ZZ
    /// @details This function makes a passage behave as a shop area, as long as the
    /// character is alive.

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = _currentModule->getPassageByID(state.argument);
    if(passage) {
        passage->makeShop(ObjectRef(self.index));
        returncode = true;
    }
    else {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetPayForArmor( script_state_t& state, ai_state_t& self )
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

    if ( !_currentModule->getObjectHandler().exists( self.target ) ) return false;

    pself_target = _currentModule->getObjectHandler().get( self.target );


    iTmp = pself_target->getProfile()->getSkinInfo(state.argument).cost;
    state.y = iTmp;                                       // Cost of new skin

    iTmp -= pself_target->getProfile()->getSkinInfo(pself_target->skin).cost;     // Refund for old skin

    if ( iTmp > pself_target->money )
    {
        // Not enough.
        state.x = iTmp - pself_target->money;        // Amount needed
        returncode = false;
    }
    else
    {
        // Pay for it.  Cost may be negative after refund.
        pself_target->money = Ego::Math::constrain<int16_t>(pself_target->money - iTmp, 0, MAXMONEY);
        state.x = 0;
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinEvilTeam( script_state_t& state, ai_state_t& self )
{
    // JoinEvilTeam()
    /// @author ZZ
    /// @details This function adds the character to the evil Team.

    SCRIPT_FUNCTION_BEGIN();

    pchr->setTeam(static_cast<TEAM_REF>(Team::TEAM_EVIL));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinNullTeam( script_state_t& state, ai_state_t& self )
{
    // JoinNullTeam()
    /// @author ZZ
    /// @details This function adds the character to the null Team.

    SCRIPT_FUNCTION_BEGIN();

    pchr->setTeam(static_cast<TEAM_REF>(Team::TEAM_NULL));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinGoodTeam( script_state_t& state, ai_state_t& self )
{
    // JoinGoodTeam()
    /// @author ZZ
    /// @details This function adds the character to the good Team.

    SCRIPT_FUNCTION_BEGIN();

    pchr->setTeam(static_cast<TEAM_REF>(Team::TEAM_GOOD));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PitsKill( script_state_t& state, ai_state_t& self )
{
    // PitsKill()
    /// @author ZZ
    /// @details This function activates pit deaths for when characters fall below a
    /// certain altitude.

    SCRIPT_FUNCTION_BEGIN();

    g_pits.kill = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToPassageID( script_state_t& state, ai_state_t& self )
{
    // SetTargetToPassageID( tmpargument = "passage", tmpdistance = "idsz" )
    /// @author ZZ
    /// @details This function finds a character who is both in the passage and who has
    /// an item with the given IDSZ

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = _currentModule->getPassageByID(state.argument);

    returncode = false;
    if(passage) {
        ObjectRef objRef = passage->whoIsBlockingPassage(ObjectRef(self.index), IDSZ_NONE, TARGET_SELF | TARGET_FRIENDS | TARGET_ENEMIES, state.distance);
        if ( _currentModule->getObjectHandler().exists(objRef) )
        {
            SET_TARGET_0(objRef);
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeNameUnknown( script_state_t& state, ai_state_t& self )
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
Uint8 scr_SpawnExactParticleEndSpawn( script_state_t& state, ai_state_t& self )
{
    // SpawnExactParticleEndSpawn( tmpargument = "particle", tmpturn = "state", tmpx = "x", tmpy = "y", tmpdistance = "z" )

    /// @author ZZ
    /// @details This function spawns a particle at a specific x, y, z position.
    /// When the particle ends, a character is spawned at its final location.
    /// The character is the same type of whatever spawned the particle.

    std::shared_ptr<Ego::Particle> particle;

    SCRIPT_FUNCTION_BEGIN();

	CHR_REF ichr = self.index;
    if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    {
		Vector3f vtmp =
			Vector3f
            (
				float(state.x),
				float(state.y),
				float(state.distance)
            );

        particle = ParticleHandler::get().spawnLocalParticle(vtmp, pchr->ori.facing_z, pchr->getProfileID(),
                                                             LocalParticleProfileRef(state.argument),
                                                             INVALID_CHR_REF, 0, pchr->team, ichr, INVALID_PRT_REF,
                                                             0, INVALID_CHR_REF);
    }

    returncode = (particle != nullptr);

    if ( returncode )
    {
        particle->endspawn_characterstate = state.turn;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnPoofSpeedSpacingDamage( script_state_t& state, ai_state_t& self )
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
    std::shared_ptr<pip_t> ppip = PipStack.get_ptr(particleRef);

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
        ppip->vel_hrz_pair.base     = state.x;
        ppip->spacing_hrz_pair.base = state.y;
        ppip->damage.from           = FP8_TO_FLOAT( state.argument );
        ppip->damage.to             = ppip->damage.from + damage_rand;

        ParticleHandler::get().spawnPoof(pchr->toSharedPointer());


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
Uint8 scr_GiveExperienceToGoodTeam( script_state_t& state, ai_state_t& self )
{
    // GiveExperienceToGoodTeam(  tmpargument = "amount", tmpdistance = "type" )
    /// @author ZZ
    /// @details This function gives experience to everyone on the G Team

    SCRIPT_FUNCTION_BEGIN();

    if(state.distance < XP_COUNT)
    {

        _currentModule->getTeamList()[Team::TEAM_GOOD].giveTeamExperience(state.argument, static_cast<XPType>(state.distance) );
    }


    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoNothing( script_state_t& state, ai_state_t& self )
{
    // DoNothing()
    /// @author ZF
    /// @details This function does nothing
    /// Use this for debugging or in combination with a Else function

    return true;
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GrogTarget( script_state_t& state, ai_state_t& self )
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
        int timer_val = pself_target->grog_timer + state.argument;
        pself_target->grog_timer = std::max( 0, timer_val );
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DazeTarget( script_state_t& state, ai_state_t& self )
{
    // DazeTarget( tmpargument = "amount" )
    /// @author ZF
    /// @details This function dazes the Target for a duration equal to tmpargument

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    // Characters who manage to daze themselves are to ignore their daze immunity
    returncode = false;
    if ( pself_target->getProfile()->canBeDazed() || self.index == self.target )
    {
        int timer_val = pself_target->daze_timer + state.argument;
        pself_target->daze_timer = std::max( 0, timer_val );

        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableRespawn( script_state_t& state, ai_state_t& self )
{
    // EnableRespawn()
    /// @author ZF
    /// @details This function turns respawn with JUMP button on

    SCRIPT_FUNCTION_BEGIN();

    _currentModule->setRespawnValid(true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableRespawn( script_state_t& state, ai_state_t& self )
{
    // DisableRespawn()
    /// @author ZF
    /// @details This function turns respawn with JUMP button off

    SCRIPT_FUNCTION_BEGIN();

    _currentModule->setRespawnValid(false);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfHolderBlocked( script_state_t& state, ai_state_t& self )
{
    // IfHolderBlocked()
    /// @author ZF
    /// @details This function passes if the holder blocked an attack

    CHR_REF iattached;

    SCRIPT_FUNCTION_BEGIN();

    iattached = pchr->attachedto;

    if ( _currentModule->getObjectHandler().exists( iattached ) )
    {
        BIT_FIELD bits = _currentModule->getObjectHandler().get(iattached)->ai.alert;

        if ( HAS_SOME_BITS( bits, ALERTIF_BLOCKED ) )
        {
            auto iattacked = ObjectRef(_currentModule->getObjectHandler().get(iattached)->ai.attacklast);

            if ( _currentModule->getObjectHandler().exists( iattacked ) )
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
Uint8 scr_IfTargetHasNotFullMana( script_state_t& state, ai_state_t& self )
{
    // IfTargetHasNotFullMana()
    /// @author ZF
    /// @details This function passes only if the Target is not at max mana and alive

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !pself_target->isAlive() || pself_target->getMana() > pself_target->getAttribute(Ego::Attribute::MAX_MANA) - FP8_TO_FLOAT(HURTDAMAGE) )
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableListenSkill( script_state_t& state, ai_state_t& self )
{
    // EnableListenSkill()
    /// @author ZF
    /// @details This function increases range from which sound can be heard by 33%

    SCRIPT_FUNCTION_BEGIN();

    {
		Log::get().warn("deprecated script function used: EnableListenSkill! (%s)\n", pchr->getProfile()->getClassName().c_str());
    }

    returncode = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToLastItemUsed( script_state_t& state, ai_state_t& self )
{
    // SetTargetToLastItemUsed()
    /// @author ZF
    /// @details This sets the Target to the last item the character used

    SCRIPT_FUNCTION_BEGIN();

    if ( self.lastitemused != self.index && _currentModule->getObjectHandler().exists( self.lastitemused ) )
    {
        SET_TARGET_0(ObjectRef(self.lastitemused));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FollowLink( script_state_t& state, ai_state_t& self )
{
    // FollowLink( tmpargument = "index of next module name" )
    /// @author BB
    /// @details Skips to the next module!

    SCRIPT_FUNCTION_BEGIN();

    if ( !ppro->isValidMessageID(state.argument) ) return false;

    returncode = link_follow_modname( ppro->getMessage(state.argument).c_str(), true );
    if ( !returncode )
    {
        DisplayMsg_printf( "That's too scary for %s", pchr->getName().c_str() );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfOperatorIsLinux( script_state_t& state, ai_state_t& self )
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
Uint8 scr_IfTargetIsAWeapon( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsAWeapon()
    /// @author ZF
    /// @details Proceeds if the AI Target Is a melee or ranged weapon

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[self.target];
    if(target) {
        returncode = target->getProfile()->isRangedWeapon() || target->getProfile()->hasIDSZ(MAKE_IDSZ('X', 'W', 'E', 'P'));
    }
    else {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfSomeoneIsStealing( script_state_t& state, ai_state_t& self )
{
    // IfSomeoneIsStealing()
    /// @author ZF
    /// @details This function passes if someone stealed from it's shop

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( self.order_value == Passage::SHOP_STOLEN && self.order_counter == Passage::SHOP_THEFT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsASpell( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsASpell()
    /// @author ZF
    /// @details roceeds if the AI Target has any particle with the [IDAM] expansion

    SCRIPT_FUNCTION_BEGIN();

    returncode = false;
    for (LocalParticleProfileRef iTmp(0); iTmp.get() < MAX_PIP_PER_PROFILE; ++iTmp)
    {
        std::shared_ptr<pip_t> ppip = ProfileSystem::get().pro_get_ppip(pchr->getProfileID(), iTmp);
        if (!ppip) continue;

        if ( ppip->_intellectDamageBonus )
        {
            returncode = true;
            break;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfBackstabbed( script_state_t& state, ai_state_t& self )
{
    // IfBackstabbed()
    /// @author ZF
    /// @details Proceeds if HitFromBehind, target has [STAB] skill and damage dealt is physical
    /// automatically fails if attacker has a code of conduct

    SCRIPT_FUNCTION_BEGIN();

    //Now check if it really was backstabbed
    returncode = false;
    if ( HAS_SOME_BITS( self.alert, ALERTIF_ATTACKED ) )
    {
        //Who is the dirty backstabber?
        Object * pattacker = _currentModule->getObjectHandler().get( self.attacklast );
        if (!pattacker || pattacker->isTerminated()) return false;

        //Only if hit from behind
        if ( self.directionlast >= ATK_BEHIND - 8192 && self.directionlast < ATK_BEHIND + 8192 )
        {
            //And require the backstab skill
            if ( pattacker->hasPerk(Ego::Perks::BACKSTAB) )
            {
                //Finally we require it to be physical damage!
                if (DamageType_isPhysical(self.damagetypelast))
                {
                    returncode = true;
                }
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GetTargetDamageType( script_state_t& state, ai_state_t& self )
{
    // tmpargument = GetTargetDamageType()
    /// @author ZF
    /// @details This function gets the last type of damage for the Target

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    state.argument = pself_target->ai.damagetypelast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddQuest( script_state_t& state, ai_state_t& self )
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

        result = quest_log_add( ppla->quest_log, state.argument, state.distance );
    }

    returncode = ( rv_success == result );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BeatQuestAllPlayers( script_state_t& state, ai_state_t& self )
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
        if ( !_currentModule->getObjectHandler().exists( ichr ) ) continue;

        if ( QUEST_BEATEN == quest_log_adjust_level( ppla->quest_log, static_cast<IDSZ>(state.argument), QUEST_MAXVAL ) )
        {
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetHasQuest( script_state_t& state, ai_state_t& self )
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

        quest_level = quest_log_get_level( ppla->quest_log, state.argument );
    }

    // only find active quests
    if ( quest_level >= 0 )
    {
        state.distance = quest_level;
        returncode       = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetQuestLevel( script_state_t& state, ai_state_t& self )
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
    if ( VALID_PLA( ipla ) && 0 != state.distance )
    {
        player_t * ppla        = PlaStack.get_ptr( ipla );

        int quest_level = quest_log_adjust_level( ppla->quest_log, state.argument, state.distance );

        returncode = QUEST_NONE != quest_level;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddQuestAllPlayers( script_state_t& state, ai_state_t& self )
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

        if ( !ppla->valid || !_currentModule->getObjectHandler().exists( ppla->index ) ) continue;
        player_count++;

        // Try to add it or replace it if this one is higher
        quest_level = quest_log_add( ppla->quest_log, state.argument, state.distance );
        if ( QUEST_NONE != quest_level ) success_count++;
    }

    returncode = ( player_count > 0 ) && ( success_count >= player_count );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddBlipAllEnemies( script_state_t& state, ai_state_t& self )
{
    // AddBlipAllEnemies()
    /// @author ZF
    /// @details show all enemies on the minimap who match the IDSZ given in tmpargument
    /// it show only the enemies of the AI Target

    SCRIPT_FUNCTION_BEGIN();

    if ( _currentModule->getObjectHandler().exists( self.target ) )
    {
        local_stats.sense_enemies_team = _currentModule->getObjectHandler()[self.target]->getTeam().toRef();
        local_stats.sense_enemies_idsz = state.argument;
    }
    else
    {
        local_stats.sense_enemies_team = ( TEAM_REF )Team::TEAM_MAX;
        local_stats.sense_enemies_idsz = IDSZ_NONE;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PitsFall( script_state_t& state, ai_state_t& self )
{
    // PitsFall( tmpx = "teleprt x", tmpy = "teleprt y", tmpdistance = "teleprt z" )
    /// @author ZF
    /// @details This function activates pit teleportation.

    SCRIPT_FUNCTION_BEGIN();

    if ( state.x > EDGE && state.y > EDGE && state.x < _currentModule->getMeshPointer()->_gmem._edge_x - EDGE && state.y < _currentModule->getMeshPointer()->_gmem._edge_y - EDGE )
    {
        g_pits.teleport = true;
        g_pits.teleport_pos[kX] = state.x;
        g_pits.teleport_pos[kY] = state.y;
        g_pits.teleport_pos[kZ] = state.distance;
    }
    else
    {
        g_pits.kill = true;          //make it kill instead
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsOwner( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsOwner()
    /// @author ZF
    /// @details This function proceeds only if the Target is the character's owner

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->isAlive() && self.owner == self.target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedCharacter( script_state_t& state, ai_state_t& self )
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

	Vector3f pos = Vector3f(float(state.x), float(state.y), float(state.distance));

    std::shared_ptr<Object> pchild = _currentModule->spawnObject(pos, (PRO_REF)state.argument, pchr->team, 0, FACE_NORTH, "", INVALID_CHR_REF);
    returncode = pchild != nullptr;

    if ( !returncode )
    {
		Log::get().warn("%s:%d: object \"%s\"(\"%s\") failed to spawn profile index %d\n", __FILE__, __LINE__, \
			            pchr->getName().c_str(), pchr->getProfile()->getClassName().c_str(), state.argument);
    }
    else
    {
        Uint8 grip = Ego::Math::constrain<int>(state.distance, ATTACH_INVENTORY, ATTACH_RIGHT);

        if ( grip == ATTACH_INVENTORY )
        {
            // Inventory character
            if ( Inventory::add_item( ObjectRef(self.target), pchild->getObjRef(), pchr->getInventory().getFirstFreeSlotNumber(), true ) )
            {
                SET_BIT( pchild->ai.alert, ALERTIF_GRABBED );  // Make spellbooks change
                pchild->attachedto = self.target;  // Make grab work
                scr_run_chr_script( pchild->getObjRef().get() );  // Empty the grabbed messages

                pchild->attachedto = INVALID_CHR_REF;  // Fix grab

                //Set some AI values
                self.child = pchild->getObjRef().get();
                pchild->ai.passage = self.passage;
                pchild->ai.owner   = self.owner;
            }

            //No more room!
            else
            {
                pchild->requestTerminate();
            }
        }
        else if ( grip == ATTACH_LEFT || grip == ATTACH_RIGHT )
        {
            if ( !_currentModule->getObjectHandler().exists( pself_target->holdingwhich[grip] ) )
            {
                // Wielded character
                grip_offset_t grip_off = ( ATTACH_LEFT == grip ) ? GRIP_LEFT : GRIP_RIGHT;

                if ( rv_success == attach_character_to_mount( pchild->getObjRef(), ObjectRef(self.target), grip_off ) )
                {
                    // Handle the "grabbed" messages
                    scr_run_chr_script( pchild->getObjRef().get() );
                }

                //Set some AI values
                self.child = pchild->getObjRef().get();
                pchild->ai.passage = self.passage;
                pchild->ai.owner   = self.owner;
            }

            //Grip is already used
            else
            {
                pchild->requestTerminate();
            }
        }
        else
        {
            // we have been given an invalid attachment point.
            // still allow the character to spawn if it is not in an invalid area

            //Set some AI values
            self.child = pchild->getObjRef().get();
            pchild->ai.passage = self.passage;
            pchild->ai.owner   = self.owner;                
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToChild( script_state_t& state, ai_state_t& self )
{
    // SetTargetToChild()
    /// @author ZF
    /// @details This function sets the target to the character it spawned last (also called it's "child")

    SCRIPT_FUNCTION_BEGIN();

    if ( _currentModule->getObjectHandler().exists( self.child ) )
    {
        SET_TARGET_0(ObjectRef(self.child));
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetDamageThreshold( script_state_t& state, ai_state_t& self )
{
    // SetDamageThreshold()
    /// @author ZF
    /// @details This sets the damage treshold for this character. Damage below the threshold is ignored

    SCRIPT_FUNCTION_BEGIN();

    if ( state.argument > 0 ) pchr->damage_threshold = state.argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_End( script_state_t& state, ai_state_t& self )
{
    // End()
    /// @author ZZ
    /// @details This Is the last function in a script

    SCRIPT_FUNCTION_BEGIN();

    self.terminate = true;
    returncode = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TakePicture( script_state_t& state, ai_state_t& self )
{
    // TakePicture()
    /// @author ZF
    /// @details This function proceeds only if the screenshot was successful

    SCRIPT_FUNCTION_BEGIN();

    returncode = dump_screenshot();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetSpeech( script_state_t& state, ai_state_t& self )
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
        pchr->sound_index[sTmp] = state.argument;
    }
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetMoveSpeech( script_state_t& state, ai_state_t& self )
{
    // SetMoveSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS move speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_MOVE] = state.argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetSecondMoveSpeech( script_state_t& state, ai_state_t& self )
{
    // SetSecondMoveSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS movealt speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();
    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_MOVEALT] = state.argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetAttackSpeech( script_state_t& state, ai_state_t& self )
{
    // SetAttacksSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS attack speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();
    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_ATTACK] = state.argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetAssistSpeech( script_state_t& state, ai_state_t& self )
{
    // SetAssistSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS assist speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();
    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_ASSIST] = state.argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTerrainSpeech( script_state_t& state, ai_state_t& self )
{
    // SetTerrainSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS terrain speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();
    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_TERRAIN] = state.argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetSelectSpeech( script_state_t& state, ai_state_t& self )
{
    // SetSelectSpeech( tmpargument = "sound" )
    /// @author ZZ
    /// @details This function sets the RTS select speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();
    //ZF> no longer supported
#if 0
    pchr->sound_index[SPEECH_SELECT] = state.argument;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfOperatorIsMacintosh( script_state_t& state, ai_state_t& self )
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
Uint8 scr_IfModuleHasIDSZ( script_state_t& state, ai_state_t& self )
{
    // IfModuleHasIDSZ( tmpargument = "message number with module name", tmpdistance = "idsz" )

    /// @author ZF
    /// @details Proceeds if the specified module has the required IDSZ specified in tmpdistance
    /// The module folder name to be checked is a string from message.txt

    SCRIPT_FUNCTION_BEGIN();

    ///use message.txt to send the module name
    if ( !ppro->isValidMessageID(state.argument) ) return false;

    STRING buffer;
    strncpy(buffer, ppro->getMessage(state.argument).c_str(), SDL_arraysize(buffer));

    returncode = ModuleProfile::moduleHasIDSZ( _currentModule->getName().c_str(), state.distance, 0, buffer);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MorphToTarget( script_state_t& state, ai_state_t& self )
{
    // MorphToTarget()
    /// @author ZF
    /// @details This morphs the character into the target
    /// Also set size and keeps the previous AI type

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !_currentModule->getObjectHandler().exists( self.target ) ) return false;

    pchr->polymorphObject(pself_target->basemodel_ref, pself_target->skin);

    // let the resizing take some time
    pchr->fat_goto      = pself_target->fat;
    pchr->fat_goto_time = SIZETIME;

    // change back to our original AI (keep our old AI script)
//    pself->type      = ProList.lst[pchr->basemodel_ref].iai;      //TODO: this no longer works (is it even needed?)

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveManaFlowToTarget( script_state_t& state, ai_state_t& self )
{
    // GiveManaFlowToTarget()
    /// @author ZF
    /// @details Permanently boost the target's mana flow

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->isAlive() )
    {
        pself_target->increaseBaseAttribute(Ego::Attribute::SPELL_POWER, FP8_TO_FLOAT(state.argument));
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveManaReturnToTarget( script_state_t& state, ai_state_t& self )
{
    // GiveManaReturnToTarget()
    /// @author ZF
    /// @details Permanently boost the target's mana return

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->isAlive() )
    {
        pself_target->increaseBaseAttribute(Ego::Attribute::MANA_REGEN, FP8_TO_FLOAT(state.argument));
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetMoney( script_state_t& state, ai_state_t& self )
{
    // SetMoney()
    /// @author ZF
    /// @details Permanently sets the money for the character to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->money = CLIP( state.argument, 0, MAXMONEY );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetCanSeeKurses( script_state_t& state, ai_state_t& self )
{
    // IfTargetCanSeeKurses()
    /// @author ZF
    /// @details Proceeds if the target can see kursed stuff.

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->getAttribute(Ego::Attribute::SENSE_KURSES) > 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DispelTargetEnchantID( script_state_t& state, ai_state_t& self )
{
    // DispelEnchantID( tmpargument = "idsz" )
    /// @author ZF
    /// @details This function removes all enchants from the target who match the specified RemovedByIDSZ

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = false;
    if ( pself_target->isAlive() )
    {
        // Check all enchants to see if they are removed
        pself_target->removeEnchantsWithIDSZ(state.argument);
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KurseTarget( script_state_t& state, ai_state_t& self )
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
Uint8 scr_SetChildContent( script_state_t& state, ai_state_t& self )
{
    // SetChildContent( tmpargument = "content" )
    /// @author ZF
    /// @details This function lets a character set the content of the last character it
    /// spawned last

    SCRIPT_FUNCTION_BEGIN();

    _currentModule->getObjectHandler().get(self.child)->ai.content = state.argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateTargetUp( script_state_t& state, ai_state_t& self )
{
    // AccelerateTargetUp( tmpargument = "acc z" )
    /// @author ZF
    /// @details This function makes the target accelerate up and down

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->vel[kZ] += state.argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetAmmo( script_state_t& state, ai_state_t& self )
{
    // SetTargetAmmo( tmpargument = "ammo" )
    /// @author ZF
    /// @details This function sets the ammo of the character's current AI target

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->ammo = std::min( state.argument, (int)pself_target->ammomax );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableInvictus( script_state_t& state, ai_state_t& self )
{
    // EnableInvictus()
    /// @author ZF
    /// @details This function makes the character invulerable

    SCRIPT_FUNCTION_BEGIN();

    pchr->invictus = true;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableInvictus( script_state_t& state, ai_state_t& self )
{
    // DisableInvictus()
    /// @author ZF
    /// @details This function makes the character not invulerable

    SCRIPT_FUNCTION_BEGIN();

    pchr->invictus = false;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDamageSelf( script_state_t& state, ai_state_t& self )
{
    // TargetDamageSelf( tmpargument = "damage" )
    /// @author ZF
    /// @details This function applies little bit of hate from the character's target to
    /// the character itself. The amount is set in tmpargument

    IPair tmp_damage;

    SCRIPT_FUNCTION_BEGIN();

    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[self.target];
    if(!target) {
        return false;
    }

    tmp_damage.base = state.argument;
    tmp_damage.rand = 1;

    pchr->damage(ATK_FRONT, tmp_damage, static_cast<DamageType>(state.distance), target->getTeam().toRef(), target, DAMFX_NBLOC, true);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetSize( script_state_t& state, ai_state_t& self )
{
    // SetTargetSize( tmpargument = "percent" )
    /// @author ZF
    /// @details This changes the AI target's size

    Object * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->fat_goto *= state.argument / 100.0f;
    pself_target->fat_goto_time += SIZETIME;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DrawBillboard( script_state_t& state, ai_state_t& self )
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

    if ( !ppro->isValidMessageID(state.argument) ) return false;

    auto* tint = &tint_white;
    //Figure out which color to use
    switch ( state.turn )
    {
        default:
        case COLOR_WHITE:   tint = &tint_white;   break;
        case COLOR_RED:     tint = &tint_red;     break;
        case COLOR_PURPLE:  tint = &tint_purple;  break;
        case COLOR_YELLOW:  tint = &tint_yellow;  break;
        case COLOR_GREEN:   tint = &tint_green;   break;
        case COLOR_BLUE:    tint = &tint_blue;    break;
    }

    returncode = NULL != chr_make_text_billboard(ObjectRef(self.index), ppro->getMessage(state.argument).c_str(), text_color, *tint, state.distance, Billboard::Flags::Fade);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToBlahInPassage( script_state_t& state, ai_state_t& self )
{
    // SetTargetToBlahInPassage()
    /// @author ZF
    /// @details This function sets the target to whatever object with the specified bits
    /// in tmpdistance is blocking the given passage. This function lets passage rectangles be used as event triggers

    SCRIPT_FUNCTION_BEGIN();

    std::shared_ptr<Passage> passage = _currentModule->getPassageByID(state.argument);
    returncode = false;
    if(passage) {
        auto objRef = passage->whoIsBlockingPassage(ObjectRef(self.index), state.turn, TARGET_SELF | state.distance, IDSZ_NONE );

        if ( _currentModule->getObjectHandler().exists(objRef) )
        {
            SET_TARGET_0(objRef);
            returncode = true;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfTargetIsFacingSelf( script_state_t& state, ai_state_t& self )
{
    // IfTargetIsFacingSelf()
    /// @author ZF
    /// @details This function proceeds if the target is more or less facing the character

    SCRIPT_FUNCTION_BEGIN();

	Object *pself_target;
    SCRIPT_REQUIRE_TARGET( pself_target );

	FACING_T sTmp = 0;
    sTmp = vec_to_facing( pchr->getPosX() - pself_target->getPosX() , pchr->getPosY() - pself_target->getPosY() );
    sTmp -= pself_target->ori.facing_z;
    returncode = ( sTmp > 55535 || sTmp < 10000 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfLevelUp( script_state_t& state, ai_state_t& self )
{
    // IfLevelUp()
    /// @author ZF
    /// @details This function proceeds if the character gained a new level this update
    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( self.alert, ALERTIF_LEVELUP );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveSkillToTarget( script_state_t& state, ai_state_t& self )
{
    // GiveSkillToTarget( tmpargument = "skill_IDSZ" )
    /// @author ZF
    /// @details This function permanently gives the target character a Perk

    SCRIPT_FUNCTION_BEGIN();

	Object *ptarget;
    SCRIPT_REQUIRE_TARGET( ptarget );

    //IDSZ to Perk
    switch(state.argument)
    {
        case MAKE_IDSZ( 'A', 'W', 'E', 'P' ): ptarget->addPerk(Ego::Perks::WEAPON_PROFICIENCY); break;
        case MAKE_IDSZ( 'P', 'O', 'I', 'S' ): ptarget->addPerk(Ego::Perks::POISONRY); break;
        case MAKE_IDSZ( 'C', 'K', 'U', 'R' ): ptarget->addPerk(Ego::Perks::SENSE_KURSES); break;
        case MAKE_IDSZ( 'R', 'E', 'A', 'D' ): ptarget->addPerk(Ego::Perks::LITERACY); break;
        case MAKE_IDSZ( 'W', 'M', 'A', 'G' ): ptarget->addPerk(Ego::Perks::ARCANE_MAGIC); break;
        case MAKE_IDSZ( 'H', 'M', 'A', 'G' ): ptarget->addPerk(Ego::Perks::DIVINE_MAGIC); break;
        case MAKE_IDSZ( 'T', 'E', 'C', 'H' ): ptarget->addPerk(Ego::Perks::USE_TECHNOLOGICAL_ITEMS); break;
        case MAKE_IDSZ( 'D', 'I', 'S', 'A' ): ptarget->addPerk(Ego::Perks::TRAP_LORE); break;
        case MAKE_IDSZ( 'S', 'T', 'A', 'B' ): ptarget->addPerk(Ego::Perks::BACKSTAB); break;
        case MAKE_IDSZ( 'D', 'A', 'R', 'K' ): ptarget->addPerk(Ego::Perks::NIGHT_VISION); break;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetToNearbyMeleeWeapon( script_state_t& state, ai_state_t& self )
{
    CHR_REF best_target;

    SCRIPT_FUNCTION_BEGIN();

    best_target = FindWeapon( pchr, WIDE, MAKE_IDSZ( 'X', 'W', 'E', 'P' ), false, true );

    //Did we find anything good?
    if ( _currentModule->getObjectHandler().exists( best_target ) )
    {
        self.target = best_target;
        returncode = true;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableStealth( script_state_t& state, ai_state_t& self )
{
    // EnableStealth()
    /// @author ZF
    /// @details Makes the object enter stealth mode. Returns true if it is now hidden from others.

    SCRIPT_FUNCTION_BEGIN();

    if(pchr->isStealthed()) {
        returncode = false;
    }
    else {
        returncode = pchr->activateStealth();
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableStealth( script_state_t& state, ai_state_t& self )
{
    // DisableStealth()
    /// @author ZF
    /// @details Makes the object exit stealth mode. Returns true if it exited stealth mode.

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->isStealthed();
    pchr->deactivateStealth();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfStealthed( script_state_t& state, ai_state_t& self )
{
    // IfStealthed()
    /// @author ZF
    /// @details Returns true if the Object is currently in stealth mode and not detected

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->isStealthed();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
uint8_t scr_SetTargetToDistantFriend( script_state_t& state, ai_state_t& self )
{
    // SetTargetToDistantFriend( tmpdistance = "distance" )
    /// @author ZF
    /// @details This function finds a character within a certain distance of the
    /// character, failing if there are none

    SCRIPT_FUNCTION_BEGIN();

    auto ichr = chr_find_target(pchr, state.distance, IDSZ_NONE, TARGET_FRIENDS);

    if (_currentModule->getObjectHandler().exists(ichr))
    {
        SET_TARGET_0(ichr);
    }
    else
    {
        returncode = false;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
uint8_t scr_DisplayCharge(script_state_t& state, ai_state_t& self)
{
    // DisplayCharge( tmpargument = "progress", tmpdistance = "max progress", tmpturn = "pip width" )
    /// @author ZF
    /// @details Draws a special progress bar this update frame

    SCRIPT_FUNCTION_BEGIN();

    //We ourselves must be a player or our holder must be one
    std::shared_ptr<Object> player = _currentModule->getObjectHandler()[pchr->getObjRef()];
    if(!player->isPlayer() && player->isBeingHeld()) {
        player = _currentModule->getObjectHandler()[pchr->attachedto];
    }

    //Only do this for players
    if(!player->isPlayer()) {
        returncode = false;
    }

    //Validate arguments
    else if(state.distance <= 0)  {
        returncode = false;
    }

    //Render it!
    else {        
        returncode = true;

        player_t * ppla = PlaStack.get_ptr(player->is_which_player);
        ppla->_currentCharge = std::min(state.argument, state.argument);
        ppla->_maxCharge = state.distance;
        ppla->_chargeTick = state.turn;
        ppla->_chargeBarFrame = update_wld + 10;
    }

    SCRIPT_FUNCTION_END();
}
