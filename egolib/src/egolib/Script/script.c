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

/// @file  egolib/Script/script.c
/// @brief Implements the game's scripting language.
/// @details

#include "egolib/Script/script.h"
#include "game/script_compile.h"
#include "game/script_implementation.h"
#include "game/script_functions.h"
#include "egolib/Graphics/mad.h"
#include "egolib/AI/AStar.h"
#include "game/game.h"
#include "game/network.h"
#include "game/player.h"
#include "game/Entities/_Include.hpp"
#include "game/char.h"
#include "game/Core/GameEngine.hpp"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Module/Module.hpp"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool scr_increment_pos( script_info_t * pself );
static bool scr_set_pos( script_info_t * pself, size_t position );

static Uint8 scr_run_function( script_state_t * pstate, ai_state_t& aiState, script_info_t *pscript );
static void  scr_set_operand( script_state_t * pstate, Uint8 variable );
static void  scr_run_operand( script_state_t * pstate, ai_state_t& aiState, script_info_t * pscript );

static bool scr_run_operation( script_state_t * pstate, ai_state_t& aiState, script_info_t *pscript );
static bool scr_run_function_call( script_state_t * pstate, ai_state_t& aiState, script_info_t *pscript );

/// A counter to measure the time of an invocation of a script function.
/// Its window size is 1 as the duration spend in the invocation is added to an histogram (see below).
static std::shared_ptr<Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive>> g_scriptFunctionClock = nullptr;
/// @todo Data points (avg. runtimes) sorted into categories (script functions): This is a histogram
///       and can be implemented as such.
static int    _script_function_calls[SCRIPT_FUNCTIONS_COUNT];
static double _script_function_times[SCRIPT_FUNCTIONS_COUNT];

static PRO_REF script_error_model = INVALID_PRO_REF;
static const char * script_error_classname = "UNKNOWN";

static bool _scripting_system_initialized = false;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scripting_system_begin()
{
	if (!_scripting_system_initialized) {
		g_scriptFunctionClock = std::make_shared<Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive>>("script function clock", 1);
		for (size_t i = 0; i < SCRIPT_FUNCTIONS_COUNT; ++i) {
			_script_function_calls[i] = 0;
			_script_function_times[i] = 0.0F;
		}
		_scripting_system_initialized = true;
	}
}

void scripting_system_end()
{
    if (_scripting_system_initialized) {
		vfs_FILE *target = vfs_openAppend("/debug/script_function_timing.txt");
		if (nullptr != target) {
            for (size_t i = 0; i < SCRIPT_FUNCTIONS_COUNT; ++i) {
                if (_script_function_calls[i] > 0) {
					vfs_printf(target, "function == %d\tname == \"%s\"\tcalls == %d\ttime == %lf\n",
						       static_cast<int>(i), script_function_names[i], _script_function_calls[i], _script_function_times[i]);
                }
            }
            vfs_close(target);
        }
		g_scriptFunctionClock = nullptr;
        _scripting_system_initialized = false;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scr_run_chr_script(Object *pchr) {

	// Make sure that this module is initialized.
	scripting_system_begin();

	// Do not run scripts of terminated entities.
	if (pchr->isTerminated()) {
		return;
	}
	ai_state_t& aiState = pchr->ai;
	script_info_t *pscript = &pchr->getProfile()->getAIScript();

	// Has the time for this character to die come and gone?
	if (aiState.poof_time >= 0 && aiState.poof_time <= (Sint32)update_wld) {
		return;
	}

	// Grab the "changed" value from the last time the script was run.
	if (aiState.changed) {
		SET_BIT(aiState.alert, ALERTIF_CHANGED);
		aiState.changed = false;
	}

	Ego::Time::ClockScope<Ego::Time::ClockPolicy::NonRecursive> scope(*aiState._clock);

	// debug a certain script
	// debug_scripts = ( 385 == pself->index && 76 == pchr->profile_ref );

	// target_old is set to the target every time the script is run
	aiState.target_old = aiState.target;

	// Make life easier
	script_error_classname = "UNKNOWN";
	script_error_model = pchr->profile_ref;
	if (script_error_model < INVALID_PRO_REF)
	{
		script_error_classname = ProfileSystem::get().getProfile(script_error_model)->getClassName().c_str();
	}

	if (debug_scripts && debug_script_file) {
		vfs_FILE * scr_file = debug_script_file;

		vfs_printf(scr_file, "\n\n--------\n%s\n", pscript->name);
		vfs_printf(scr_file, "%d - %s\n", REF_TO_INT(script_error_model), script_error_classname);

		// who are we related to?
		vfs_printf(scr_file, "\tindex  == %d\n", REF_TO_INT(aiState.index));
		vfs_printf(scr_file, "\ttarget == %d\n", REF_TO_INT(aiState.target));
		vfs_printf(scr_file, "\towner  == %d\n", REF_TO_INT(aiState.owner));
		vfs_printf(scr_file, "\tchild  == %d\n", REF_TO_INT(aiState.child));

		// some local storage
		vfs_printf(scr_file, "\talert     == %x\n", aiState.alert);
		vfs_printf(scr_file, "\tstate     == %d\n", aiState.state);
		vfs_printf(scr_file, "\tcontent   == %d\n", aiState.content);
		vfs_printf(scr_file, "\ttimer     == %d\n", aiState.timer);
		vfs_printf(scr_file, "\tupdate_wld == %d\n", update_wld);

		// ai memory from the last event
		vfs_printf(scr_file, "\tbumplast       == %d\n", REF_TO_INT(aiState.bumplast));
		vfs_printf(scr_file, "\tattacklast     == %d\n", REF_TO_INT(aiState.attacklast));
		vfs_printf(scr_file, "\thitlast        == %d\n", REF_TO_INT(aiState.hitlast));
		vfs_printf(scr_file, "\tdirectionlast  == %d\n", aiState.directionlast);
		vfs_printf(scr_file, "\tdamagetypelast == %d\n", aiState.damagetypelast);
		vfs_printf(scr_file, "\tlastitemused   == %d\n", REF_TO_INT(aiState.lastitemused));
		vfs_printf(scr_file, "\ttarget_old     == %d\n", REF_TO_INT(aiState.target_old));

		// message handling
		vfs_printf(scr_file, "\torder == %d\n", aiState.order_value);
		vfs_printf(scr_file, "\tcounter == %d\n", aiState.order_counter);

		// waypoints
		vfs_printf(scr_file, "\twp_tail == %d\n", aiState.wp_lst.tail);
		vfs_printf(scr_file, "\twp_head == %d\n\n", aiState.wp_lst.head);
	}

	// Clear the button latches.
	if (!VALID_PLA(pchr->is_which_player)) {
		RESET_BIT_FIELD(pchr->latch.b);
	}

	// Reset the target if it can't be seen.
	if (aiState.target != aiState.index) {
		const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[aiState.target];
		if (target && !pchr->canSeeObject(target)) {
			aiState.target = aiState.index;
		}
	}

	// Reset the script state.
	script_state_t my_state;
	script_state_init(my_state);

	// Reset the ai.
	aiState.terminate = false;
	pscript->indent = 0;

	// Run the AI Script.
	scr_set_pos(pscript, 0);
	while (!aiState.terminate && pscript->position < pscript->length) {
		// This is used by the Else function
		// it only keeps track of functions.
		pscript->indent_last = pscript->indent;
		pscript->indent = GET_DATA_BITS(pscript->data[pscript->position]);

		// Was it a function.
		if (HAS_SOME_BITS(pscript->data[pscript->position], FUNCTION_BIT)) {
			if (!scr_run_function_call(&my_state, aiState, pscript)) {
				break;
			}
		}
		else {
			if (!scr_run_operation(&my_state, aiState, pscript)) {
				break;
			}
		}
	}

	// Set latches
	if (!VALID_PLA(pchr->is_which_player)) {
		float latch2;

		ai_state_ensure_wp(aiState);

		if (pchr->isMount() && _currentModule->getObjectHandler().exists(pchr->holdingwhich[SLOT_LEFT])) {
			// Mount
			pchr->latch.x = _currentModule->getObjectHandler().get(pchr->holdingwhich[SLOT_LEFT])->latch.x;
			pchr->latch.y = _currentModule->getObjectHandler().get(pchr->holdingwhich[SLOT_LEFT])->latch.y;
		}
		else if (aiState.wp_valid) {
			// Normal AI
			pchr->latch.x = (aiState.wp[kX] - pchr->getPosX()) / (GRID_ISIZE << 1);
			pchr->latch.y = (aiState.wp[kY] - pchr->getPosY()) / (GRID_ISIZE << 1);
		}
		else {
			// AI, but no valid waypoints
			pchr->latch.x = 0;
			pchr->latch.y = 0;
		}

		latch2 = pchr->latch.x * pchr->latch.x + pchr->latch.y * pchr->latch.y;
		if (latch2 > 1.0f) {
			float scale = 1.0f / std::sqrt(latch2);
			pchr->latch.x *= scale;
			pchr->latch.y *= scale;
		}
	}

	// Clear alerts for next time around
	RESET_BIT_FIELD(aiState.alert);
}
void scr_run_chr_script( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function lets one character do AI stuff

    // Make sure that this module is initialized.
    scripting_system_begin();

	if (!_currentModule->getObjectHandler().exists(character))  {
		return;
	}
	Object *pchr = _currentModule->getObjectHandler().get(character);
	return scr_run_chr_script(pchr);
}

//--------------------------------------------------------------------------------------------
bool scr_run_function_call( script_state_t * pstate, ai_state_t& aiState, script_info_t *pscript )
{
    Uint8  functionreturn;

    // check for valid pointers
    if ( NULL == pstate) return false;

    // check for valid execution pointer
    if ( pscript->position >= pscript->length ) return false;

    // Run the function
	functionreturn = scr_run_function(pstate, aiState, pscript);

    // move the execution pointer to the jump code
    scr_increment_pos( pscript );
    if ( functionreturn )
    {
        // move the execution pointer to the next opcode
        scr_increment_pos( pscript );
    }
    else
    {
        // use the jump code to jump to the right location
        size_t new_index = pscript->data[pscript->position];

        // make sure the value is valid
        EGOBOO_ASSERT( new_index <= pscript->length );

        // actually do the jump
        scr_set_pos( pscript, new_index );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool scr_run_operation( script_state_t * pstate, ai_state_t& aiState, script_info_t * pscript )
{
    const char * variable;
    Uint32 var_value, operand_count, i;

    // check for valid pointers
    if ( NULL == pstate || NULL == pscript ) return false;

    // check for valid execution pointer
    if ( pscript->position >= pscript->length ) return false;

    var_value = pscript->data[pscript->position] & VALUE_BITS;

    // debug stuff
    variable = "UNKNOWN";
    if ( debug_scripts && debug_script_file )
    {

        for ( i = 0; i < pscript->indent; i++ ) { vfs_printf( debug_script_file, "  " ); }

        for ( i = 0; i < MAX_OPCODE; i++ )
        {
            if ( 'V' == OpList.ary[i].cType && var_value == OpList.ary[i].iValue )
            {
                variable = OpList.ary[i].cName;
                break;
            };
        }

        vfs_printf( debug_script_file, "%s = ", variable );
    }

    // Get the number of operands
    scr_increment_pos( pscript );
    operand_count = pscript->data[pscript->position];

    // Now run the operation
    pstate->operationsum = 0;
    for ( i = 0; i < operand_count && pscript->position < pscript->length; i++ )
    {
        scr_increment_pos( pscript );
		scr_run_operand(pstate, aiState, pscript);
    }
    if ( debug_scripts && debug_script_file )
    {
        vfs_printf( debug_script_file, " == %d \n", pstate->operationsum );
    }

    // Save the results in the register that called the arithmetic
    scr_set_operand( pstate, var_value );

    // go to the next opcode
    scr_increment_pos( pscript );

    return true;
}

//--------------------------------------------------------------------------------------------
Uint8 scr_run_function( script_state_t * pstate, ai_state_t& aiState, script_info_t * pscript )
{
    /// @author BB
    /// @details This is about half-way to what is needed for Lua integration

    // Mask out the indentation
    Uint32 valuecode = pscript->data[pscript->position] & VALUE_BITS;

    // Assume that the function will pass, as most do
    Uint8 returncode = true;
    if ( MAX_OPCODE == valuecode )
    {
        log_message( "SCRIPT ERROR: scr_run_function() - model == %d, class name == \"%s\" - Unknown opcode found!\n", REF_TO_INT( script_error_model ), script_error_classname );
        return false;
    }

    // debug stuff
    if ( debug_scripts && debug_script_file )
    {
        Uint32 i;

        for ( i = 0; i < pscript->indent; i++ ) { vfs_printf( debug_script_file,  "  " ); }

        for ( i = 0; i < MAX_OPCODE; i++ )
        {
            if ( 'F' == OpList.ary[i].cType && valuecode == OpList.ary[i].iValue )
            {
                vfs_printf( debug_script_file,  "%s\n", OpList.ary[i].cName );
                break;
            };
        }
    }

    if ( valuecode > SCRIPT_FUNCTIONS_COUNT )
    {
    }
    else
    {
		
		{ 
			Ego::Time::ClockScope<Ego::Time::ClockPolicy::NonRecursive> scope(*g_scriptFunctionClock);
            // Figure out which function to run
            switch ( valuecode )
            {
				case FIFSPAWNED: returncode = scr_Spawned(pstate, &aiState); break;
				case FIFTIMEOUT: returncode = scr_TimeOut(pstate, &aiState); break;
				case FIFATWAYPOINT: returncode = scr_AtWaypoint(pstate, &aiState); break;
				case FIFATLASTWAYPOINT: returncode = scr_AtLastWaypoint(pstate, &aiState); break;
				case FIFATTACKED: returncode = scr_Attacked(pstate, &aiState); break;
				case FIFBUMPED: returncode = scr_Bumped(pstate, &aiState); break;
				case FIFORDERED: returncode = scr_Ordered(pstate, &aiState); break;
				case FIFCALLEDFORHELP: returncode = scr_CalledForHelp(pstate, &aiState); break;
				case FSETCONTENT: returncode = scr_set_Content(pstate, &aiState); break;
				case FIFKILLED: returncode = scr_Killed(pstate, &aiState); break;
				case FIFTARGETKILLED: returncode = scr_TargetKilled(pstate, &aiState); break;
				case FCLEARWAYPOINTS: returncode = scr_ClearWaypoints(pstate, &aiState); break;
				case FADDWAYPOINT: returncode = scr_AddWaypoint(pstate, &aiState); break;
				case FFINDPATH: returncode = scr_FindPath(pstate, &aiState); break;
				case FCOMPASS: returncode = scr_Compass(pstate, &aiState); break;
				case FGETTARGETARMORPRICE: returncode = scr_get_TargetArmorPrice(pstate, &aiState); break;
				case FSETTIME: returncode = scr_set_Time(pstate, &aiState); break;
				case FGETCONTENT: returncode = scr_get_Content(pstate, &aiState); break;
				case FJOINTARGETTEAM: returncode = scr_JoinTargetTeam(pstate, &aiState); break;
				case FSETTARGETTONEARBYENEMY: returncode = scr_set_TargetToNearbyEnemy(pstate, &aiState); break;
				case FSETTARGETTOTARGETLEFTHAND: returncode = scr_set_TargetToTargetLeftHand(pstate, &aiState); break;
				case FSETTARGETTOTARGETRIGHTHAND: returncode = scr_set_TargetToTargetRightHand(pstate, &aiState); break;
				case FSETTARGETTOWHOEVERATTACKED: returncode = scr_set_TargetToWhoeverAttacked(pstate, &aiState); break;
				case FSETTARGETTOWHOEVERBUMPED: returncode = scr_set_TargetToWhoeverBumped(pstate, &aiState); break;
				case FSETTARGETTOWHOEVERCALLEDFORHELP: returncode = scr_set_TargetToWhoeverCalledForHelp(pstate, &aiState); break;
				case FSETTARGETTOOLDTARGET: returncode = scr_set_TargetToOldTarget(pstate, &aiState); break;
				case FSETTURNMODETOVELOCITY: returncode = scr_set_TurnModeToVelocity(pstate, &aiState); break;
				case FSETTURNMODETOWATCH: returncode = scr_set_TurnModeToWatch(pstate, &aiState); break;
				case FSETTURNMODETOSPIN: returncode = scr_set_TurnModeToSpin(pstate, &aiState); break;
				case FSETBUMPHEIGHT: returncode = scr_set_BumpHeight(pstate, &aiState); break;
				case FIFTARGETHASID: returncode = scr_TargetHasID(pstate, &aiState); break;
				case FIFTARGETHASITEMID: returncode = scr_TargetHasItemID(pstate, &aiState); break;
				case FIFTARGETHOLDINGITEMID: returncode = scr_TargetHoldingItemID(pstate, &aiState); break;
				case FIFTARGETHASSKILLID: returncode = scr_TargetHasSkillID(pstate, &aiState); break;
				case FELSE: returncode = scr_Else(pstate, &aiState); break;
				case FRUN: returncode = scr_Run(pstate, &aiState); break;
				case FWALK: returncode = scr_Walk(pstate, &aiState); break;
				case FSNEAK: returncode = scr_Sneak(pstate, &aiState); break;
				case FDOACTION: returncode = scr_DoAction(pstate, &aiState); break;
				case FKEEPACTION: returncode = scr_KeepAction(pstate, &aiState); break;
				case FISSUEORDER: returncode = scr_IssueOrder(pstate, &aiState); break;
				case FDROPWEAPONS: returncode = scr_DropWeapons(pstate, &aiState); break;
				case FTARGETDOACTION: returncode = scr_TargetDoAction(pstate, &aiState); break;
				case FOPENPASSAGE: returncode = scr_OpenPassage(pstate, &aiState); break;
				case FCLOSEPASSAGE: returncode = scr_ClosePassage(pstate, &aiState); break;
				case FIFPASSAGEOPEN: returncode = scr_PassageOpen(pstate, &aiState); break;
				case FGOPOOF: returncode = scr_GoPoof(pstate, &aiState); break;
				case FCOSTTARGETITEMID: returncode = scr_CostTargetItemID(pstate, &aiState); break;
				case FDOACTIONOVERRIDE: returncode = scr_DoActionOverride(pstate, &aiState); break;
				case FIFHEALED: returncode = scr_Healed(pstate, &aiState); break;
				case FSENDMESSAGE: returncode = scr_SendPlayerMessage(pstate, &aiState); break;
				case FCALLFORHELP: returncode = scr_CallForHelp(pstate, &aiState); break;
				case FADDIDSZ: returncode = scr_AddIDSZ(pstate, &aiState); break;
				case FSETSTATE: returncode = scr_set_State(pstate, &aiState); break;
				case FGETSTATE: returncode = scr_get_State(pstate, &aiState); break;
				case FIFSTATEIS: returncode = scr_StateIs(pstate, &aiState); break;
				case FIFTARGETCANOPENSTUFF: returncode = scr_TargetCanOpenStuff(pstate, &aiState); break;
				case FIFGRABBED: returncode = scr_Grabbed(pstate, &aiState); break;
				case FIFDROPPED: returncode = scr_Dropped(pstate, &aiState); break;
				case FSETTARGETTOWHOEVERISHOLDING: returncode = scr_set_TargetToWhoeverIsHolding(pstate, &aiState); break;
				case FDAMAGETARGET: returncode = scr_DamageTarget(pstate, &aiState); break;
				case FIFXISLESSTHANY: returncode = scr_XIsLessThanY(pstate, &aiState); break;
				case FSETWEATHERTIME: returncode = scr_set_WeatherTime(pstate, &aiState); break;
				case FGETBUMPHEIGHT: returncode = scr_get_BumpHeight(pstate, &aiState); break;
				case FIFREAFFIRMED: returncode = scr_Reaffirmed(pstate, &aiState); break;
				case FUNKEEPACTION: returncode = scr_UnkeepAction(pstate, &aiState); break;
				case FIFTARGETISONOTHERTEAM: returncode = scr_TargetIsOnOtherTeam(pstate, &aiState); break;
				case FIFTARGETISONHATEDTEAM: returncode = scr_TargetIsOnHatedTeam(pstate, &aiState); break;
				case FPRESSLATCHBUTTON: returncode = scr_PressLatchButton(pstate, &aiState); break;
				case FSETTARGETTOTARGETOFLEADER: returncode = scr_set_TargetToTargetOfLeader(pstate, &aiState); break;
				case FIFLEADERKILLED: returncode = scr_LeaderKilled(pstate, &aiState); break;
				case FBECOMELEADER: returncode = scr_BecomeLeader(pstate, &aiState); break;
				case FCHANGETARGETARMOR: returncode = scr_ChangeTargetArmor(pstate, &aiState); break;
				case FGIVEMONEYTOTARGET: returncode = scr_GiveMoneyToTarget(pstate, &aiState); break;
				case FDROPKEYS: returncode = scr_DropKeys(pstate, &aiState); break;
				case FIFLEADERISALIVE: returncode = scr_LeaderIsAlive(pstate, &aiState); break;
				case FIFTARGETISOLDTARGET: returncode = scr_TargetIsOldTarget(pstate, &aiState); break;
				case FSETTARGETTOLEADER: returncode = scr_set_TargetToLeader(pstate, &aiState); break;
				case FSPAWNCHARACTER: returncode = scr_SpawnCharacter(pstate, &aiState); break;
				case FRESPAWNCHARACTER: returncode = scr_RespawnCharacter(pstate, &aiState); break;
				case FCHANGETILE: returncode = scr_ChangeTile(pstate, &aiState); break;
				case FIFUSED: returncode = scr_Used(pstate, &aiState); break;
				case FDROPMONEY: returncode = scr_DropMoney(pstate, &aiState); break;
				case FSETOLDTARGET: returncode = scr_set_OldTarget(pstate, &aiState); break;
				case FDETACHFROMHOLDER: returncode = scr_DetachFromHolder(pstate, &aiState); break;
				case FIFTARGETHASVULNERABILITYID: returncode = scr_TargetHasVulnerabilityID(pstate, &aiState); break;
				case FCLEANUP: returncode = scr_CleanUp(pstate, &aiState); break;
				case FIFCLEANEDUP: returncode = scr_CleanedUp(pstate, &aiState); break;
				case FIFSITTING: returncode = scr_Sitting(pstate, &aiState); break;
				case FIFTARGETISHURT: returncode = scr_TargetIsHurt(pstate, &aiState); break;
				case FIFTARGETISAPLAYER: returncode = scr_TargetIsAPlayer(pstate, &aiState); break;
				case FPLAYSOUND: returncode = scr_PlaySound(pstate, &aiState); break;
				case FSPAWNPARTICLE: returncode = scr_SpawnParticle(pstate, &aiState); break;
				case FIFTARGETISALIVE: returncode = scr_TargetIsAlive(pstate, &aiState); break;
				case FSTOP: returncode = scr_Stop(pstate, &aiState); break;
				case FDISAFFIRMCHARACTER: returncode = scr_DisaffirmCharacter(pstate, &aiState); break;
				case FREAFFIRMCHARACTER: returncode = scr_ReaffirmCharacter(pstate, &aiState); break;
				case FIFTARGETISSELF: returncode = scr_TargetIsSelf(pstate, &aiState); break;
				case FIFTARGETISMALE: returncode = scr_TargetIsMale(pstate, &aiState); break;
				case FIFTARGETISFEMALE: returncode = scr_TargetIsFemale(pstate, &aiState); break;
				case FSETTARGETTOSELF: returncode = scr_set_TargetToSelf(pstate, &aiState); break;
				case FSETTARGETTORIDER: returncode = scr_set_TargetToRider(pstate, &aiState); break;
				case FGETATTACKTURN: returncode = scr_get_AttackTurn(pstate, &aiState); break;
				case FGETDAMAGETYPE: returncode = scr_get_DamageType(pstate, &aiState); break;
				case FBECOMESPELL: returncode = scr_BecomeSpell(pstate, &aiState); break;
				case FBECOMESPELLBOOK: returncode = scr_BecomeSpellbook(pstate, &aiState); break;
				case FIFSCOREDAHIT: returncode = scr_ScoredAHit(pstate, &aiState); break;
				case FIFDISAFFIRMED: returncode = scr_Disaffirmed(pstate, &aiState); break;
				case FTRANSLATEORDER: returncode = scr_TranslateOrder(pstate, &aiState); break;
				case FSETTARGETTOWHOEVERWASHIT: returncode = scr_set_TargetToWhoeverWasHit(pstate, &aiState); break;
				case FSETTARGETTOWIDEENEMY: returncode = scr_set_TargetToWideEnemy(pstate, &aiState); break;
				case FIFCHANGED: returncode = scr_Changed(pstate, &aiState); break;
				case FIFINWATER: returncode = scr_InWater(pstate, &aiState); break;
				case FIFBORED: returncode = scr_Bored(pstate, &aiState); break;
				case FIFTOOMUCHBAGGAGE: returncode = scr_TooMuchBaggage(pstate, &aiState); break;
				case FIFGROGGED: returncode = scr_Grogged(pstate, &aiState); break;
				case FIFDAZED: returncode = scr_Dazed(pstate, &aiState); break;
				case FIFTARGETHASSPECIALID: returncode = scr_TargetHasSpecialID(pstate, &aiState); break;
				case FPRESSTARGETLATCHBUTTON: returncode = scr_PressTargetLatchButton(pstate, &aiState); break;
				case FIFINVISIBLE: returncode = scr_Invisible(pstate, &aiState); break;
				case FIFARMORIS: returncode = scr_ArmorIs(pstate, &aiState); break;
				case FGETTARGETGROGTIME: returncode = scr_get_TargetGrogTime(pstate, &aiState); break;
				case FGETTARGETDAZETIME: returncode = scr_get_TargetDazeTime(pstate, &aiState); break;
				case FSETDAMAGETYPE: returncode = scr_set_DamageType(pstate, &aiState); break;
				case FSETWATERLEVEL: returncode = scr_set_WaterLevel(pstate, &aiState); break;
				case FENCHANTTARGET: returncode = scr_EnchantTarget(pstate, &aiState); break;
				case FENCHANTCHILD: returncode = scr_EnchantChild(pstate, &aiState); break;
				case FTELEPORTTARGET: returncode = scr_TeleportTarget(pstate, &aiState); break;
				case FGIVEEXPERIENCETOTARGET: returncode = scr_add_TargetExperience(pstate, &aiState); break;
				case FINCREASEAMMO: returncode = scr_IncreaseAmmo(pstate, &aiState); break;
				case FUNKURSETARGET: returncode = scr_UnkurseTarget(pstate, &aiState); break;
				case FGIVEEXPERIENCETOTARGETTEAM: returncode = scr_add_TargetTeamExperience(pstate, &aiState); break;
				case FIFUNARMED: returncode = scr_Unarmed(pstate, &aiState); break;
				case FRESTOCKTARGETAMMOIDALL: returncode = scr_RestockTargetAmmoIDAll(pstate, &aiState); break;
				case FRESTOCKTARGETAMMOIDFIRST: returncode = scr_RestockTargetAmmoIDFirst(pstate, &aiState); break;
				case FFLASHTARGET: returncode = scr_FlashTarget(pstate, &aiState); break;
				case FSETREDSHIFT: returncode = scr_set_RedShift(pstate, &aiState); break;
				case FSETGREENSHIFT: returncode = scr_set_GreenShift(pstate, &aiState); break;
				case FSETBLUESHIFT: returncode = scr_set_BlueShift(pstate, &aiState); break;
				case FSETLIGHT: returncode = scr_set_Light(pstate, &aiState); break;
				case FSETALPHA: returncode = scr_set_Alpha(pstate, &aiState); break;
				case FIFHITFROMBEHIND: returncode = scr_HitFromBehind(pstate, &aiState); break;
				case FIFHITFROMFRONT: returncode = scr_HitFromFront(pstate, &aiState); break;
				case FIFHITFROMLEFT: returncode = scr_HitFromLeft(pstate, &aiState); break;
				case FIFHITFROMRIGHT: returncode = scr_HitFromRight(pstate, &aiState); break;
				case FIFTARGETISONSAMETEAM: returncode = scr_TargetIsOnSameTeam(pstate, &aiState); break;
				case FKILLTARGET: returncode = scr_KillTarget(pstate, &aiState); break;
				case FUNDOENCHANT: returncode = scr_UndoEnchant(pstate, &aiState); break;
				case FGETWATERLEVEL: returncode = scr_get_WaterLevel(pstate, &aiState); break;
				case FCOSTTARGETMANA: returncode = scr_CostTargetMana(pstate, &aiState); break;
				case FIFTARGETHASANYID: returncode = scr_TargetHasAnyID(pstate, &aiState); break;
				case FSETBUMPSIZE: returncode = scr_set_BumpSize(pstate, &aiState); break;
				case FIFNOTDROPPED: returncode = scr_NotDropped(pstate, &aiState); break;
				case FIFYISLESSTHANX: returncode = scr_YIsLessThanX(pstate, &aiState); break;
				case FSETFLYHEIGHT: returncode = scr_set_FlyHeight(pstate, &aiState); break;
				case FIFBLOCKED: returncode = scr_Blocked(pstate, &aiState); break;
				case FIFTARGETISDEFENDING: returncode = scr_TargetIsDefending(pstate, &aiState); break;
				case FIFTARGETISATTACKING: returncode = scr_TargetIsAttacking(pstate, &aiState); break;
				case FIFSTATEIS0: returncode = scr_StateIs0(pstate, &aiState); break;
				case FIFSTATEIS1: returncode = scr_StateIs1(pstate, &aiState); break;
				case FIFSTATEIS2: returncode = scr_StateIs2(pstate, &aiState); break;
				case FIFSTATEIS3: returncode = scr_StateIs3(pstate, &aiState); break;
				case FIFSTATEIS4: returncode = scr_StateIs4(pstate, &aiState); break;
				case FIFSTATEIS5: returncode = scr_StateIs5(pstate, &aiState); break;
				case FIFSTATEIS6: returncode = scr_StateIs6(pstate, &aiState); break;
				case FIFSTATEIS7: returncode = scr_StateIs7(pstate, &aiState); break;
				case FIFCONTENTIS: returncode = scr_ContentIs(pstate, &aiState); break;
				case FSETTURNMODETOWATCHTARGET: returncode = scr_set_TurnModeToWatchTarget(pstate, &aiState); break;
				case FIFSTATEISNOT: returncode = scr_StateIsNot(pstate, &aiState); break;
				case FIFXISEQUALTOY: returncode = scr_XIsEqualToY(pstate, &aiState); break;
				case FDEBUGMESSAGE: returncode = scr_DebugMessage(pstate, &aiState); break;
				case FBLACKTARGET: returncode = scr_BlackTarget(pstate, &aiState); break;
				case FSENDMESSAGENEAR: returncode = scr_SendMessageNear(pstate, &aiState); break;
				case FIFHITGROUND: returncode = scr_HitGround(pstate, &aiState); break;
				case FIFNAMEISKNOWN: returncode = scr_NameIsKnown(pstate, &aiState); break;
				case FIFUSAGEISKNOWN: returncode = scr_UsageIsKnown(pstate, &aiState); break;
				case FIFHOLDINGITEMID: returncode = scr_HoldingItemID(pstate, &aiState); break;
				case FIFHOLDINGRANGEDWEAPON: returncode = scr_HoldingRangedWeapon(pstate, &aiState); break;
				case FIFHOLDINGMELEEWEAPON: returncode = scr_HoldingMeleeWeapon(pstate, &aiState); break;
				case FIFHOLDINGSHIELD: returncode = scr_HoldingShield(pstate, &aiState); break;
				case FIFKURSED: returncode = scr_Kursed(pstate, &aiState); break;
				case FIFTARGETISKURSED: returncode = scr_TargetIsKursed(pstate, &aiState); break;
				case FIFTARGETISDRESSEDUP: returncode = scr_TargetIsDressedUp(pstate, &aiState); break;
				case FIFOVERWATER: returncode = scr_OverWater(pstate, &aiState); break;
				case FIFTHROWN: returncode = scr_Thrown(pstate, &aiState); break;
				case FMAKENAMEKNOWN: returncode = scr_MakeNameKnown(pstate, &aiState); break;
				case FMAKEUSAGEKNOWN: returncode = scr_MakeUsageKnown(pstate, &aiState); break;
				case FSTOPTARGETMOVEMENT: returncode = scr_StopTargetMovement(pstate, &aiState); break;
				case FSETXY: returncode = scr_set_XY(pstate, &aiState); break;
				case FGETXY: returncode = scr_get_XY(pstate, &aiState); break;
				case FADDXY: returncode = scr_AddXY(pstate, &aiState); break;
				case FMAKEAMMOKNOWN: returncode = scr_MakeAmmoKnown(pstate, &aiState); break;
				case FSPAWNATTACHEDPARTICLE: returncode = scr_SpawnAttachedParticle(pstate, &aiState); break;
				case FSPAWNEXACTPARTICLE: returncode = scr_SpawnExactParticle(pstate, &aiState); break;
				case FACCELERATETARGET: returncode = scr_AccelerateTarget(pstate, &aiState); break;
				case FIFDISTANCEISMORETHANTURN: returncode = scr_distanceIsMoreThanTurn(pstate, &aiState); break;
				case FIFCRUSHED: returncode = scr_Crushed(pstate, &aiState); break;
				case FMAKECRUSHVALID: returncode = scr_MakeCrushValid(pstate, &aiState); break;
				case FSETTARGETTOLOWESTTARGET: returncode = scr_set_TargetToLowestTarget(pstate, &aiState); break;
				case FIFNOTPUTAWAY: returncode = scr_NotPutAway(pstate, &aiState); break;
				case FIFTAKENOUT: returncode = scr_TakenOut(pstate, &aiState); break;
				case FIFAMMOOUT: returncode = scr_AmmoOut(pstate, &aiState); break;
				case FPLAYSOUNDLOOPED: returncode = scr_PlaySoundLooped(pstate, &aiState); break;
				case FSTOPSOUND: returncode = scr_StopSound(pstate, &aiState); break;
				case FHEALSELF: returncode = scr_HealSelf(pstate, &aiState); break;
				case FEQUIP: returncode = scr_Equip(pstate, &aiState); break;
				case FIFTARGETHASITEMIDEQUIPPED: returncode = scr_TargetHasItemIDEquipped(pstate, &aiState); break;
				case FSETOWNERTOTARGET: returncode = scr_set_OwnerToTarget(pstate, &aiState); break;
				case FSETTARGETTOOWNER: returncode = scr_set_TargetToOwner(pstate, &aiState); break;
				case FSETFRAME: returncode = scr_set_Frame(pstate, &aiState); break;
				case FBREAKPASSAGE: returncode = scr_BreakPassage(pstate, &aiState); break;
				case FSETRELOADTIME: returncode = scr_set_ReloadTime(pstate, &aiState); break;
				case FSETTARGETTOWIDEBLAHID: returncode = scr_set_TargetToWideBlahID(pstate, &aiState); break;
				case FPOOFTARGET: returncode = scr_PoofTarget(pstate, &aiState); break;
				case FCHILDDOACTIONOVERRIDE: returncode = scr_ChildDoActionOverride(pstate, &aiState); break;
				case FSPAWNPOOF: returncode = scr_SpawnPoof(pstate, &aiState); break;
				case FSETSPEEDPERCENT: returncode = scr_set_SpeedPercent(pstate, &aiState); break;
				case FSETCHILDSTATE: returncode = scr_set_ChildState(pstate, &aiState); break;
				case FSPAWNATTACHEDSIZEDPARTICLE: returncode = scr_SpawnAttachedSizedParticle(pstate, &aiState); break;
				case FCHANGEARMOR: returncode = scr_ChangeArmor(pstate, &aiState); break;
				case FSHOWTIMER: returncode = scr_ShowTimer(pstate, &aiState); break;
				case FIFFACINGTARGET: returncode = scr_FacingTarget(pstate, &aiState); break;
				case FPLAYSOUNDVOLUME: returncode = scr_PlaySoundVolume(pstate, &aiState); break;
				case FSPAWNATTACHEDFACEDPARTICLE: returncode = scr_SpawnAttachedFacedParticle(pstate, &aiState); break;
				case FIFSTATEISODD: returncode = scr_StateIsOdd(pstate, &aiState); break;
				case FSETTARGETTODISTANTENEMY: returncode = scr_set_TargetToDistantEnemy(pstate, &aiState); break;
				case FTELEPORT: returncode = scr_Teleport(pstate, &aiState); break;
				case FGIVESTRENGTHTOTARGET: returncode = scr_add_TargetStrength(pstate, &aiState); break;
				case FGIVEINTELLECTTOTARGET:
				case FGIVEINTELLIGENCETOTARGET: returncode = scr_add_TargetIntelligence(pstate, &aiState); break;
				case FGIVEDEXTERITYTOTARGET: returncode = scr_add_TargetDexterity(pstate, &aiState); break;
				case FGIVELIFETOTARGET: returncode = scr_add_TargetLife(pstate, &aiState); break;
				case FGIVEMANATOTARGET: returncode = scr_add_TargetMana(pstate, &aiState); break;
				case FSHOWMAP: returncode = scr_ShowMap(pstate, &aiState); break;
				case FSHOWYOUAREHERE: returncode = scr_ShowYouAreHere(pstate, &aiState); break;
				case FSHOWBLIPXY: returncode = scr_ShowBlipXY(pstate, &aiState); break;
				case FHEALTARGET: returncode = scr_HealTarget(pstate, &aiState); break;
				case FPUMPTARGET: returncode = scr_PumpTarget(pstate, &aiState); break;
				case FCOSTAMMO: returncode = scr_CostAmmo(pstate, &aiState); break;
				case FMAKESIMILARNAMESKNOWN: returncode = scr_MakeSimilarNamesKnown(pstate, &aiState); break;
				case FSPAWNATTACHEDHOLDERPARTICLE: returncode = scr_SpawnAttachedHolderParticle(pstate, &aiState); break;
				case FSETTARGETRELOADTIME: returncode = scr_set_TargetReloadTime(pstate, &aiState); break;
				case FSETFOGLEVEL: returncode = scr_set_FogLevel(pstate, &aiState); break;
				case FGETFOGLEVEL: returncode = scr_get_FogLevel(pstate, &aiState); break;
				case FSETFOGTAD: returncode = scr_set_FogTAD(pstate, &aiState); break;
				case FSETFOGBOTTOMLEVEL: returncode = scr_set_FogBottomLevel(pstate, &aiState); break;
				case FGETFOGBOTTOMLEVEL: returncode = scr_get_FogBottomLevel(pstate, &aiState); break;
				case FCORRECTACTIONFORHAND: returncode = scr_CorrectActionForHand(pstate, &aiState); break;
				case FIFTARGETISMOUNTED: returncode = scr_TargetIsMounted(pstate, &aiState); break;
				case FSPARKLEICON: returncode = scr_SparkleIcon(pstate, &aiState); break;
				case FUNSPARKLEICON: returncode = scr_UnsparkleIcon(pstate, &aiState); break;
				case FGETTILEXY: returncode = scr_get_TileXY(pstate, &aiState); break;
				case FSETTILEXY: returncode = scr_set_TileXY(pstate, &aiState); break;
				case FSETSHADOWSIZE: returncode = scr_set_ShadowSize(pstate, &aiState); break;
				case FORDERTARGET: returncode = scr_OrderTarget(pstate, &aiState); break;
				case FSETTARGETTOWHOEVERISINPASSAGE: returncode = scr_set_TargetToWhoeverIsInPassage(pstate, &aiState); break;
				case FIFCHARACTERWASABOOK: returncode = scr_CharacterWasABook(pstate, &aiState); break;
				case FSETENCHANTBOOSTVALUES: returncode = scr_set_EnchantBoostValues(pstate, &aiState); break;
				case FSPAWNCHARACTERXYZ: returncode = scr_SpawnCharacterXYZ(pstate, &aiState); break;
				case FSPAWNEXACTCHARACTERXYZ: returncode = scr_SpawnExactCharacterXYZ(pstate, &aiState); break;
				case FCHANGETARGETCLASS: returncode = scr_ChangeTargetClass(pstate, &aiState); break;
				case FPLAYFULLSOUND: returncode = scr_PlayFullSound(pstate, &aiState); break;
				case FSPAWNEXACTCHASEPARTICLE: returncode = scr_SpawnExactChaseParticle(pstate, &aiState); break;
				case FCREATEORDER: returncode = scr_CreateOrder(pstate, &aiState); break;
				case FORDERSPECIALID: returncode = scr_OrderSpecialID(pstate, &aiState); break;
				case FUNKURSETARGETINVENTORY: returncode = scr_UnkurseTargetInventory(pstate, &aiState); break;
				case FIFTARGETISSNEAKING: returncode = scr_TargetIsSneaking(pstate, &aiState); break;
				case FDROPITEMS: returncode = scr_DropItems(pstate, &aiState); break;
				case FRESPAWNTARGET: returncode = scr_RespawnTarget(pstate, &aiState); break;
				case FTARGETDOACTIONSETFRAME: returncode = scr_TargetDoActionSetFrame(pstate, &aiState); break;
				case FIFTARGETCANSEEINVISIBLE: returncode = scr_TargetCanSeeInvisible(pstate, &aiState); break;
				case FSETTARGETTONEARESTBLAHID: returncode = scr_set_TargetToNearestBlahID(pstate, &aiState); break;
				case FSETTARGETTONEARESTENEMY: returncode = scr_set_TargetToNearestEnemy(pstate, &aiState); break;
				case FSETTARGETTONEARESTFRIEND: returncode = scr_set_TargetToNearestFriend(pstate, &aiState); break;
				case FSETTARGETTONEARESTLIFEFORM: returncode = scr_set_TargetToNearestLifeform(pstate, &aiState); break;
				case FFLASHPASSAGE: returncode = scr_FlashPassage(pstate, &aiState); break;
				case FFINDTILEINPASSAGE: returncode = scr_FindTileInPassage(pstate, &aiState); break;
				case FIFHELDINLEFTHAND: returncode = scr_HeldInLeftHand(pstate, &aiState); break;
				case FNOTANITEM: returncode = scr_NotAnItem(pstate, &aiState); break;
				case FSETCHILDAMMO: returncode = scr_set_ChildAmmo(pstate, &aiState); break;
				case FIFHITVULNERABLE: returncode = scr_HitVulnerable(pstate, &aiState); break;
				case FIFTARGETISFLYING: returncode = scr_TargetIsFlying(pstate, &aiState); break;
				case FIDENTIFYTARGET: returncode = scr_IdentifyTarget(pstate, &aiState); break;
				case FBEATMODULE: returncode = scr_BeatModule(pstate, &aiState); break;
				case FENDMODULE: returncode = scr_EndModule(pstate, &aiState); break;
				case FDISABLEEXPORT: returncode = scr_DisableExport(pstate, &aiState); break;
				case FENABLEEXPORT: returncode = scr_EnableExport(pstate, &aiState); break;
				case FGETTARGETSTATE: returncode = scr_get_TargetState(pstate, &aiState); break;
				case FIFEQUIPPED: returncode = scr_Equipped(pstate, &aiState); break;
				case FDROPTARGETMONEY: returncode = scr_DropTargetMoney(pstate, &aiState); break;
				case FGETTARGETCONTENT: returncode = scr_get_TargetContent(pstate, &aiState); break;
				case FDROPTARGETKEYS: returncode = scr_DropTargetKeys(pstate, &aiState); break;
				case FJOINTEAM: returncode = scr_JoinTeam(pstate, &aiState); break;
				case FTARGETJOINTEAM: returncode = scr_TargetJoinTeam(pstate, &aiState); break;
				case FCLEARMUSICPASSAGE: returncode = scr_ClearMusicPassage(pstate, &aiState); break;
				case FCLEARENDMESSAGE: returncode = scr_ClearEndMessage(pstate, &aiState); break;
				case FADDENDMESSAGE: returncode = scr_AddEndMessage(pstate, &aiState); break;
				case FPLAYMUSIC: returncode = scr_PlayMusic(pstate, &aiState); break;
				case FSETMUSICPASSAGE: returncode = scr_set_MusicPassage(pstate, &aiState); break;
				case FMAKECRUSHINVALID: returncode = scr_MakeCrushInvalid(pstate, &aiState); break;
				case FSTOPMUSIC: returncode = scr_StopMusic(pstate, &aiState); break;
				case FFLASHVARIABLE: returncode = scr_FlashVariable(pstate, &aiState); break;
				case FACCELERATEUP: returncode = scr_AccelerateUp(pstate, &aiState); break;
				case FFLASHVARIABLEHEIGHT: returncode = scr_FlashVariableHeight(pstate, &aiState); break;
				case FSETDAMAGETIME: returncode = scr_set_DamageTime(pstate, &aiState); break;
				case FIFSTATEIS8: returncode = scr_StateIs8(pstate, &aiState); break;
				case FIFSTATEIS9: returncode = scr_StateIs9(pstate, &aiState); break;
				case FIFSTATEIS10: returncode = scr_StateIs10(pstate, &aiState); break;
				case FIFSTATEIS11: returncode = scr_StateIs11(pstate, &aiState); break;
				case FIFSTATEIS12: returncode = scr_StateIs12(pstate, &aiState); break;
				case FIFSTATEIS13: returncode = scr_StateIs13(pstate, &aiState); break;
				case FIFSTATEIS14: returncode = scr_StateIs14(pstate, &aiState); break;
				case FIFSTATEIS15: returncode = scr_StateIs15(pstate, &aiState); break;
				case FIFTARGETISAMOUNT: returncode = scr_TargetIsAMount(pstate, &aiState); break;
				case FIFTARGETISAPLATFORM: returncode = scr_TargetIsAPlatform(pstate, &aiState); break;
				case FADDSTAT: returncode = scr_AddStat(pstate, &aiState); break;
				case FDISENCHANTTARGET: returncode = scr_DisenchantTarget(pstate, &aiState); break;
				case FDISENCHANTALL: returncode = scr_DisenchantAll(pstate, &aiState); break;
				case FSETVOLUMENEARESTTEAMMATE: returncode = scr_set_VolumeNearestTeammate(pstate, &aiState); break;
				case FADDSHOPPASSAGE: returncode = scr_AddShopPassage(pstate, &aiState); break;
				case FTARGETPAYFORARMOR: returncode = scr_TargetPayForArmor(pstate, &aiState); break;
				case FJOINEVILTEAM: returncode = scr_JoinEvilTeam(pstate, &aiState); break;
				case FJOINNULLTEAM: returncode = scr_JoinNullTeam(pstate, &aiState); break;
				case FJOINGOODTEAM: returncode = scr_JoinGoodTeam(pstate, &aiState); break;
				case FPITSKILL: returncode = scr_PitsKill(pstate, &aiState); break;
				case FSETTARGETTOPASSAGEID: returncode = scr_set_TargetToPassageID(pstate, &aiState); break;
				case FMAKENAMEUNKNOWN: returncode = scr_MakeNameUnknown(pstate, &aiState); break;
				case FSPAWNEXACTPARTICLEENDSPAWN: returncode = scr_SpawnExactParticleEndSpawn(pstate, &aiState); break;
				case FSPAWNPOOFSPEEDSPACINGDAMAGE: returncode = scr_SpawnPoofSpeedSpacingDamage(pstate, &aiState); break;
				case FGIVEEXPERIENCETOGOODTEAM: returncode = scr_add_GoodTeamExperience(pstate, &aiState); break;
				case FDONOTHING: returncode = scr_DoNothing(pstate, &aiState); break;
				case FGROGTARGET: returncode = scr_GrogTarget(pstate, &aiState); break;
				case FDAZETARGET: returncode = scr_DazeTarget(pstate, &aiState); break;
				case FENABLERESPAWN: returncode = scr_EnableRespawn(pstate, &aiState); break;
				case FDISABLERESPAWN: returncode = scr_DisableRespawn(pstate, &aiState); break;
				case FDISPELTARGETENCHANTID: returncode = scr_DispelTargetEnchantID(pstate, &aiState); break;
				case FIFHOLDERBLOCKED: returncode = scr_HolderBlocked(pstate, &aiState); break;

				case FIFTARGETHASNOTFULLMANA: returncode = scr_TargetHasNotFullMana(pstate, &aiState); break;
				case FENABLELISTENSKILL: returncode = scr_EnableListenSkill(pstate, &aiState); break;
				case FSETTARGETTOLASTITEMUSED: returncode = scr_set_TargetToLastItemUsed(pstate, &aiState); break;
				case FFOLLOWLINK: returncode = scr_FollowLink(pstate, &aiState); break;
				case FIFOPERATORISLINUX: returncode = scr_OperatorIsLinux(pstate, &aiState); break;
				case FIFTARGETISAWEAPON: returncode = scr_TargetIsAWeapon(pstate, &aiState); break;
				case FIFSOMEONEISSTEALING: returncode = scr_SomeoneIsStealing(pstate, &aiState); break;
				case FIFTARGETISASPELL: returncode = scr_TargetIsASpell(pstate, &aiState); break;
				case FIFBACKSTABBED: returncode = scr_Backstabbed(pstate, &aiState); break;
				case FGETTARGETDAMAGETYPE: returncode = scr_get_TargetDamageType(pstate, &aiState); break;
				case FADDQUEST: returncode = scr_AddQuest(pstate, &aiState); break;
				case FBEATQUESTALLPLAYERS: returncode = scr_BeatQuestAllPlayers(pstate, &aiState); break;
				case FIFTARGETHASQUEST: returncode = scr_TargetHasQuest(pstate, &aiState); break;
				case FSETQUESTLEVEL: returncode = scr_set_QuestLevel(pstate, &aiState); break;
				case FADDQUESTALLPLAYERS: returncode = scr_AddQuestAllPlayers(pstate, &aiState); break;
				case FADDBLIPALLENEMIES: returncode = scr_AddBlipAllEnemies(pstate, &aiState); break;
				case FPITSFALL: returncode = scr_PitsFall(pstate, &aiState); break;
				case FIFTARGETISOWNER: returncode = scr_TargetIsOwner(pstate, &aiState); break;
				case FEND: returncode = scr_End(pstate, &aiState); break;

				case FSETSPEECH:           returncode = scr_set_Speech(pstate, &aiState);           break;
				case FSETMOVESPEECH:       returncode = scr_set_MoveSpeech(pstate, &aiState);       break;
				case FSETSECONDMOVESPEECH: returncode = scr_set_SecondMoveSpeech(pstate, &aiState); break;
				case FSETATTACKSPEECH:     returncode = scr_set_AttackSpeech(pstate, &aiState);     break;
				case FSETASSISTSPEECH:     returncode = scr_set_AssistSpeech(pstate, &aiState);     break;
				case FSETTERRAINSPEECH:    returncode = scr_set_TerrainSpeech(pstate, &aiState);    break;
				case FSETSELECTSPEECH:     returncode = scr_set_SelectSpeech(pstate, &aiState);     break;

				case FTAKEPICTURE:           returncode = scr_TakePicture(pstate, &aiState);         break;
				case FIFOPERATORISMACINTOSH: returncode = scr_OperatorIsMacintosh(pstate, &aiState); break;
				case FIFMODULEHASIDSZ:       returncode = scr_ModuleHasIDSZ(pstate, &aiState);       break;
				case FMORPHTOTARGET:         returncode = scr_MorphToTarget(pstate, &aiState);       break;
				case FGIVEMANAFLOWTOTARGET:  returncode = scr_add_TargetManaFlow(pstate, &aiState); break;
				case FGIVEMANARETURNTOTARGET:returncode = scr_add_TargetManaReturn(pstate, &aiState); break;
				case FSETMONEY:              returncode = scr_set_Money(pstate, &aiState);           break;
				case FIFTARGETCANSEEKURSES:  returncode = scr_TargetCanSeeKurses(pstate, &aiState);  break;
				case FSPAWNATTACHEDCHARACTER:returncode = scr_SpawnAttachedCharacter(pstate, &aiState); break;
				case FKURSETARGET:           returncode = scr_KurseTarget(pstate, &aiState);            break;
				case FSETCHILDCONTENT:       returncode = scr_set_ChildContent(pstate, &aiState);    break;
				case FSETTARGETTOCHILD:      returncode = scr_set_TargetToChild(pstate, &aiState);   break;
				case FSETDAMAGETHRESHOLD:     returncode = scr_set_DamageThreshold(pstate, &aiState);   break;
				case FACCELERATETARGETUP:    returncode = scr_AccelerateTargetUp(pstate, &aiState); break;
				case FSETTARGETAMMO:         returncode = scr_set_TargetAmmo(pstate, &aiState); break;
				case FENABLEINVICTUS:        returncode = scr_EnableInvictus(pstate, &aiState); break;
				case FDISABLEINVICTUS:       returncode = scr_DisableInvictus(pstate, &aiState); break;
				case FTARGETDAMAGESELF:      returncode = scr_TargetDamageSelf(pstate, &aiState); break;
				case FSETTARGETSIZE:         returncode = scr_set_TargetSize(pstate, &aiState); break;
				case FIFTARGETISFACINGSELF:  returncode = scr_TargetIsFacingSelf(pstate, &aiState); break;

				case FDRAWBILLBOARD:                 returncode = scr_DrawBillboard(pstate, &aiState); break;
				case FSETTARGETTOFIRSTBLAHINPASSAGE: returncode = scr_set_TargetToBlahInPassage(pstate, &aiState); break;
				case FIFLEVELUP:                     returncode = scr_LevelUp(pstate, &aiState); break;
				case FGIVESKILLTOTARGET:             returncode = scr_add_TargetSkill(pstate, &aiState); break;
				case FSETTARGETTONEARBYMELEEWEAPON:  returncode = scr_set_TargetToNearbyMeleeWeapon(pstate, &aiState); break;

                    // if none of the above, skip the line and log an error
                default:
                    log_message( "SCRIPT ERROR: scr_run_function() - ai script \"%s\" - unhandled script function %d\n", pscript->name, valuecode );
                    returncode = false;
                    break;
            }

        }

        _script_function_calls[valuecode] += 1;
        _script_function_times[valuecode] += g_scriptFunctionClock->lst();
    }

    return returncode;
}

//--------------------------------------------------------------------------------------------
void scr_set_operand( script_state_t * pstate, Uint8 variable )
{
    /// @author ZZ
    /// @details This function sets one of the tmp* values for scripted AI
    switch ( variable )
    {
        case VARTMPX:
            pstate->x = pstate->operationsum;
            break;

        case VARTMPY:
            pstate->y = pstate->operationsum;
            break;

        case VARTMPDISTANCE:
            pstate->distance = pstate->operationsum;
            break;

        case VARTMPTURN:
            pstate->turn = pstate->operationsum;
            break;

        case VARTMPARGUMENT:
            pstate->argument = pstate->operationsum;
            break;

        default:
            log_warning( "scr_set_operand() - cannot assign a number to index %d\n", variable );
            break;
    }
}

//--------------------------------------------------------------------------------------------
void scr_run_operand( script_state_t * pstate, ai_state_t& aiState, script_info_t * pscript )
{
    /// @author ZZ
    /// @details This function does the scripted arithmetic in OPERATOR, OPERAND pscriptrs

    const char * varname, * op;

    STRING buffer = EMPTY_CSTR;
    Uint8  variable;
    Uint8  operation;

    Uint32 iTmp;

    Object * pchr = NULL, * ptarget = NULL, * powner = NULL;

	if (!_currentModule->getObjectHandler().exists(aiState.index)) return;
	pchr = _currentModule->getObjectHandler().get(aiState.index);

	if (_currentModule->getObjectHandler().exists(aiState.target))
    {
		ptarget = _currentModule->getObjectHandler().get(aiState.target);
    }

	if (_currentModule->getObjectHandler().exists(aiState.owner))
    {
		powner = _currentModule->getObjectHandler().get(aiState.owner);
    }

    // get the operator
    iTmp      = 0;
    varname   = buffer;
    operation = GET_DATA_BITS( pscript->data[pscript->position] );
    if ( HAS_SOME_BITS( pscript->data[pscript->position], FUNCTION_BIT ) )
    {
        // Get the working opcode from a constant, constants are all but high 5 bits
        iTmp = pscript->data[pscript->position] & VALUE_BITS;
        if ( debug_scripts ) snprintf( buffer, SDL_arraysize( buffer ), "%d", iTmp );
    }
    else
    {
        // Get the variable opcode from a register
        variable = pscript->data[pscript->position] & VALUE_BITS;

        switch ( variable )
        {
            case VARTMPX:
                varname = "TMPX";
                iTmp = pstate->x;
                break;

            case VARTMPY:
                varname = "TMPY";
                iTmp = pstate->y;
                break;

            case VARTMPDISTANCE:
                varname = "TMPDISTANCE";
                iTmp = pstate->distance;
                break;

            case VARTMPTURN:
                varname = "TMPTURN";
                iTmp = pstate->turn;
                break;

            case VARTMPARGUMENT:
                varname = "TMPARGUMENT";
                iTmp = pstate->argument;
                break;

            case VARRAND:
                varname = "RAND";
                iTmp = Random::next(std::numeric_limits<uint16_t>::max());
                break;

            case VARSELFX:
                varname = "SELFX";
                iTmp = pchr->getPosX();
                break;

            case VARSELFY:
                varname = "SELFY";
                iTmp = pchr->getPosY();
                break;

            case VARSELFTURN:
                varname = "SELFTURN";
                iTmp = pchr->ori.facing_z;
                break;

            case VARSELFCOUNTER:
                varname = "SELFCOUNTER";
				iTmp = aiState.order_counter;
                break;

            case VARSELFORDER:
                varname = "SELFORDER";
				iTmp = aiState.order_value;
                break;

            case VARSELFMORALE:
                varname = "SELFMORALE";
                iTmp = _currentModule->getTeamList()[pchr->team_base].getMorale();
                break;

            case VARSELFLIFE:
                varname = "SELFLIFE";
                iTmp = pchr->life;
                break;

            case VARTARGETX:
                varname = "TARGETX";
                iTmp = ( nullptr == ptarget ) ? 0 : ptarget->getPosX();
                break;

            case VARTARGETY:
                varname = "TARGETY";
                iTmp = ( nullptr == ptarget ) ? 0 : ptarget->getPosY();
                break;

            case VARTARGETDISTANCE:
                varname = "TARGETDISTANCE";
                if ( nullptr == ptarget )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = std::abs(ptarget->getPosX() - pchr->getPosX())
                         + std::abs(ptarget->getPosY() - pchr->getPosY());
                }
                break;

            case VARTARGETTURN:
                varname = "TARGETTURN";
                iTmp = ( nullptr == ptarget ) ? 0 : ptarget->ori.facing_z;
                break;

            case VARLEADERX:
            {
                varname = "LEADERX";
                iTmp = pchr->getPosX();
                std::shared_ptr<Object> leader = _currentModule->getTeamList()[pchr->team].getLeader();
                if ( leader )
                    iTmp = leader->getPosX();
                break;
            }

            case VARLEADERY:
            {
                varname = "LEADERY";
                iTmp = pchr->getPosY();
                std::shared_ptr<Object> leader = _currentModule->getTeamList()[pchr->team].getLeader();
                if ( leader )
                    iTmp = leader->getPosY();

                break; 
            }

            case VARLEADERDISTANCE:
                {
                    varname = "LEADERDISTANCE";

                    std::shared_ptr<Object> pleader = _currentModule->getTeamList()[pchr->team].getLeader();
                    if ( !pleader )
                    {
                        iTmp = 0x7FFFFFFF;
                    }
                    else
                    {
                        iTmp = std::abs(pleader->getPosX() - pchr->getPosX())
                             + std::abs(pleader->getPosY() - pchr->getPosY());
                    }
                }
                break;

            case VARLEADERTURN:
                varname = "LEADERTURN";
                iTmp = pchr->ori.facing_z;
                if ( _currentModule->getTeamList()[pchr->team].getLeader() )
                    iTmp = _currentModule->getTeamList()[pchr->team].getLeader()->ori.facing_z;

                break;

            case VARGOTOX:
                varname = "GOTOX";

				ai_state_ensure_wp(aiState);

				if (!aiState.wp_valid)
                {
                    iTmp = pchr->getPosX();
                }
                else
                {
					iTmp = aiState.wp[kX];
                }
                break;

            case VARGOTOY:
                varname = "GOTOY";

				ai_state_ensure_wp(aiState);

				if (!aiState.wp_valid)
                {
                    iTmp = pchr->getPosY();
                }
                else
                {
					iTmp = aiState.wp[kY];
                }
                break;

            case VARGOTODISTANCE:
                varname = "GOTODISTANCE";

				ai_state_ensure_wp(aiState);

				if (!aiState.wp_valid)
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
					iTmp = std::abs(aiState.wp[kX] - pchr->getPosX())
						 + std::abs(aiState.wp[kY] - pchr->getPosY());
                }
                break;

            case VARTARGETTURNTO:
                varname = "TARGETTURNTO";
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = vec_to_facing( ptarget->getPosX() - pchr->getPosX() , ptarget->getPosY() - pchr->getPosY() );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARPASSAGE:
                varname = "PASSAGE";
				iTmp = aiState.passage;
                break;

            case VARWEIGHT:
                varname = "WEIGHT";
                iTmp = pchr->holdingweight;
                break;

            case VARSELFALTITUDE:
                varname = "SELFALTITUDE";
                iTmp = pchr->getPosZ() - pchr->enviro.floor_level;
                break;

            case VARSELFID:
                varname = "SELFID";
				iTmp = chr_get_idsz(aiState.index, IDSZ_TYPE);
                break;

            case VARSELFHATEID:
                varname = "SELFHATEID";
				iTmp = chr_get_idsz(aiState.index, IDSZ_HATE);
                break;

            case VARSELFMANA:
                varname = "SELFMANA";
                iTmp = pchr->mana;
                if ( pchr->canchannel )  iTmp += pchr->life;

                break;

            case VARTARGETSTR:
                varname = "TARGETSTR";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::MIGHT));
                break;

            case VARTARGETWIS:
                varname = "TARGETWIS";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::INTELLECT));
                break;

            case VARTARGETINT:
                varname = "TARGETINT";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::INTELLECT));
                break;

            case VARTARGETDEX:
                varname = "TARGETDEX";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::AGILITY));
                break;

            case VARTARGETLIFE:
                varname = "TARGETLIFE";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->life;
                break;

            case VARTARGETMANA:
                varname = "TARGETMANA";
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = ptarget->mana;
                    if ( ptarget->canchannel ) iTmp += ptarget->life;
                }

                break;

            case VARTARGETLEVEL:
                varname = "TARGETLEVEL";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->experiencelevel;
                break;

            case VARTARGETSPEEDX:
                varname = "TARGETSPEEDX";
                iTmp = ( NULL == ptarget ) ? 0 : std::abs(ptarget->vel[kX]);
                break;

            case VARTARGETSPEEDY:
                varname = "TARGETSPEEDY";
                iTmp = ( NULL == ptarget ) ? 0 : std::abs(ptarget->vel[kY]);
                break;

            case VARTARGETSPEEDZ:
                varname = "TARGETSPEEDZ";
                iTmp = ( NULL == ptarget ) ? 0 : std::abs(ptarget->vel[kZ]);
                break;

            case VARSELFSPAWNX:
                varname = "SELFSPAWNX";
                iTmp = pchr->pos_stt[kX];
                break;

            case VARSELFSPAWNY:
                varname = "SELFSPAWNY";
                iTmp = pchr->pos_stt[kY];
                break;

            case VARSELFSTATE:
                varname = "SELFSTATE";
				iTmp = aiState.state;
                break;

            case VARSELFCONTENT:
                varname = "SELFCONTENT";
				iTmp = aiState.content;
                break;

            case VARSELFSTR:
                varname = "SELFSTR";
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::MIGHT));
                break;

            case VARSELFWIS:
                varname = "SELFWIS";
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::INTELLECT));
                break;

            case VARSELFINT:
                varname = "SELFINT";
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::INTELLECT));
                break;

            case VARSELFDEX:
                varname = "SELFDEX";
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::AGILITY));
                break;

            case VARSELFMANAFLOW:
                varname = "SELFMANAFLOW";
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::SPELL_POWER));
                break;

            case VARTARGETMANAFLOW:
                varname = "TARGETMANAFLOW";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::SPELL_POWER));
                break;

            case VARSELFATTACHED:
                varname = "SELFATTACHED";
				iTmp = number_of_attached_particles(aiState.index);
                break;

            case VARSWINGTURN:
                varname = "SWINGTURN";
                {
					std::shared_ptr<Camera> camera = CameraSystem::get()->getCameraByChrID(aiState.index);

                    iTmp = 0;
                    if ( camera )
                    {
                        iTmp = camera->getSwing() << 2;
                    }
                }
                break;

            case VARXYDISTANCE:
                varname = "XYDISTANCE";
                iTmp = std::sqrt( pstate->x * pstate->x + pstate->y * pstate->y );
                break;

            case VARSELFZ:
                varname = "SELFZ";
                iTmp = pchr->getPosZ();
                break;

            case VARTARGETALTITUDE:
                varname = "TARGETALTITUDE";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->getPosZ() - ptarget->enviro.floor_level;
                break;

            case VARTARGETZ:
                varname = "TARGETZ";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->getPosZ();
                break;

            case VARSELFINDEX:
                varname = "SELFINDEX";
				iTmp = REF_TO_INT(aiState.index);
                break;

            case VAROWNERX:
                varname = "OWNERX";
                iTmp = ( NULL == powner ) ? 0 : powner->getPosX();
                break;

            case VAROWNERY:
                varname = "OWNERY";
                iTmp = ( NULL == powner ) ? 0 : powner->getPosY();
                break;

            case VAROWNERTURN:
                varname = "OWNERTURN";
                iTmp = ( NULL == powner ) ? 0 : powner->ori.facing_z;
                break;

            case VAROWNERDISTANCE:
                varname = "OWNERDISTANCE";
                if ( NULL == powner )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = std::abs(powner->getPosX() - pchr->getPosX())
                         + std::abs(powner->getPosY() - pchr->getPosY());
                }
                break;

            case VAROWNERTURNTO:
                varname = "OWNERTURNTO";
                if ( NULL == powner )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = vec_to_facing( powner->getPosX() - pchr->getPosX() , powner->getPosY() - pchr->getPosY() );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARXYTURNTO:
                varname = "XYTURNTO";
                iTmp = vec_to_facing( pstate->x - pchr->getPosX() , pstate->y - pchr->getPosY() );
                iTmp = CLIP_TO_16BITS( iTmp );
                break;

            case VARSELFMONEY:
                varname = "SELFMONEY";
                iTmp = pchr->money;
                break;

            case VARSELFACCEL:
                varname = "SELFACCEL";
                iTmp = ( pchr->maxaccel_reset * 100.0f );
                break;

            case VARTARGETEXP:
                varname = "TARGETEXP";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->experience;
                break;

            case VARSELFAMMO:
                varname = "SELFAMMO";
                iTmp = pchr->ammo;
                break;

            case VARTARGETAMMO:
                varname = "TARGETAMMO";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->ammo;
                break;

            case VARTARGETMONEY:
                varname = "TARGETMONEY";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->money;
                break;

            case VARTARGETTURNAWAY:
                varname = "TARGETTURNAWAY";
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = vec_to_facing( ptarget->getPosX() - pchr->getPosX() , ptarget->getPosY() - pchr->getPosY() );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARSELFLEVEL:
                varname = "SELFLEVEL";
                iTmp = pchr->experiencelevel;
                break;

            case VARTARGETRELOADTIME:
                varname = "TARGETRELOADTIME";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->reload_timer;
                break;

            case VARSPAWNDISTANCE:
                varname = "SPAWNDISTANCE";
                iTmp = std::abs( pchr->pos_stt[kX] - pchr->getPosX() )
                     + std::abs( pchr->pos_stt[kY] - pchr->getPosY() );
                break;

            case VARTARGETMAXLIFE:
                varname = "TARGETMAXLIFE";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::MAX_LIFE));
                break;

            case VARTARGETTEAM:
                varname = "TARGETTEAM";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->team;
                //iTmp = REF_TO_INT( chr_get_iteam( pself->target ) );
                break;

            case VARTARGETARMOR:
                varname = "TARGETARMOR";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->skin;
                break;

            case VARDIFFICULTY:
                varname = "DIFFICULTY";
                iTmp = static_cast<uint32_t>(egoboo_config_t::get().game_difficulty.getValue());
                break;

            case VARTIMEHOURS:
                varname = "TIMEHOURS";
                iTmp = Ego::Time::LocalTime().getHours();
                break;

            case VARTIMEMINUTES:
                varname = "TIMEMINUTES";
                iTmp = Ego::Time::LocalTime().getMinutes();
                break;

            case VARTIMESECONDS:
                varname = "TIMESECONDS";
                iTmp = Ego::Time::LocalTime().getSeconds();
                break;

            case VARDATEMONTH:
                varname = "DATEMONTH";
                iTmp = Ego::Time::LocalTime().getMonth() + 1; /// @todo The addition of +1 should be removed and
				                                              /// the whole Ego::Time::LocalTime class should be
				                                              /// made available via EgoScript. However, EgoScript
				                                              /// is not yet ready for that ... not yet.
                break;

            case VARDATEDAY:
                varname = "DATEDAY";
                iTmp = Ego::Time::LocalTime().getDayOfMonth();
                break;

            default:
                log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - Unknown variable found!\n", REF_TO_INT( script_error_model ), script_error_classname );
                break;
        }
    }

    // Now do the math
    op = "UNKNOWN";
    switch ( operation )
    {
        case OPADD:
            op = "ADD";
            pstate->operationsum += iTmp;
            break;

        case OPSUB:
            op = "SUB";
            pstate->operationsum -= iTmp;
            break;

        case OPAND:
            op = "AND";
            pstate->operationsum &= iTmp;
            break;

        case OPSHR:
            op = "SHR";
            pstate->operationsum >>= iTmp;
            break;

        case OPSHL:
            op = "SHL";
            pstate->operationsum <<= iTmp;
            break;

        case OPMUL:
            op = "MUL";
            pstate->operationsum *= iTmp;
            break;

        case OPDIV:
            op = "DIV";
            if ( iTmp != 0 )
            {
                pstate->operationsum = (( float )pstate->operationsum ) / iTmp;
            }
            else
            {
                log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - Cannot divide by zero!\n", REF_TO_INT( script_error_model ), script_error_classname );
            }
            break;

        case OPMOD:
            op = "MOD";
            if ( iTmp != 0 )
            {
                pstate->operationsum %= iTmp;
            }
            else
            {
                log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - Cannot modulo by zero!\n", REF_TO_INT( script_error_model ), script_error_classname );
            }
            break;

        default:
            log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - unknown op\n", REF_TO_INT( script_error_model ), script_error_classname );
            break;
    }

    if ( debug_scripts && debug_script_file )
    {
        vfs_printf( debug_script_file, "%s %s(%d) ", op, varname, iTmp );
    }
}

//--------------------------------------------------------------------------------------------
bool scr_increment_pos( script_info_t * pscript )
{
    if ( NULL == pscript ) return false;
    if ( pscript->position >= pscript->length ) return false;

    pscript->position++;

    return true;
}

//--------------------------------------------------------------------------------------------
bool scr_set_pos( script_info_t * pscript, size_t position )
{
    if ( NULL == pscript ) return false;
    if ( position >= pscript->length ) return false;

    pscript->position = position;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool ai_state_get_wp( ai_state_t * pself )
{
    // try to load up the top waypoint

    if ( NULL == pself || !_currentModule->getObjectHandler().exists( pself->index ) ) return false;

    pself->wp_valid = waypoint_list_peek( &( pself->wp_lst ), pself->wp );

    return true;
}

//--------------------------------------------------------------------------------------------
bool ai_state_ensure_wp(ai_state_t& self)
{
    // is the current waypoint is not valid, try to load up the top waypoint

	if (!_currentModule->getObjectHandler().exists(self.index)) {
		return false;
	}
	if (self.wp_valid) {
		return true;
	}
    return ai_state_get_wp(&self);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void set_alerts( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function polls some alert conditions

    // invalid characters do not think
	if (!_currentModule->getObjectHandler().exists(character)) {
		return;
	}
	Object *pchr = _currentModule->getObjectHandler().get(character);
	ai_state_t& aiState = pchr->ai;

	if (waypoint_list_empty(&(aiState.wp_lst))) {
		return;
	}

    // let's let mounts get alert updates...
    // imagine a mount, like a racecar, that needs to make sure that it follows X
    // waypoints around a track or something

    // mounts do not get alerts
    // if ( _currentModule->getObjectHandler().exists(pchr->attachedto) ) return;

    // is the current waypoint is not valid, try to load up the top waypoint
    ai_state_ensure_wp(aiState);

    bool at_waypoint = false;
	if (aiState.wp_valid)
    {
		at_waypoint = (std::abs(pchr->getPosX() - aiState.wp[kX]) < WAYTHRESH) &&
			          (std::abs(pchr->getPosY() - aiState.wp[kY]) < WAYTHRESH);
    }

    if ( at_waypoint )
    {
		SET_BIT(aiState.alert, ALERTIF_ATWAYPOINT);

		if (waypoint_list_finished(&(aiState.wp_lst)))
        {
            // we are now at the last waypoint
            // if the object can be alerted to last waypoint, do it
            // this test needs to be done because the ALERTIF_ATLASTWAYPOINT
            // doubles for "at last waypoint" and "not put away"
            if ( !pchr->getProfile()->isEquipment() )
            {
				SET_BIT(aiState.alert, ALERTIF_ATLASTWAYPOINT);
            }

            // !!!!restart the waypoint list, do not clear them!!!!
			waypoint_list_reset(&(aiState.wp_lst));

            // load the top waypoint
			ai_state_get_wp(&aiState);
        }
		else if (waypoint_list_advance(&(aiState.wp_lst)))
        {
            // load the top waypoint
			ai_state_get_wp(&aiState);
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_order( const CHR_REF character, Uint32 value )
{
    /// @author ZZ
    /// @details This function issues an value for help to all teammates
    int counter = 0;

    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[character];

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if ( object->isTerminated() ) continue;

        if ( object->getTeam() == pchr->getTeam() )
        {
            ai_state_add_order(object->ai, value, counter);
            counter++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_special_order( Uint32 value, IDSZ idsz )
{
    /// @author ZZ
    /// @details This function issues an order to all characters with the a matching special IDSZ
    int counter = 0;

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if ( object->isTerminated() ) continue;

        if ( idsz == object->getProfile()->getIDSZ(IDSZ_SPECIAL) )
        {
            ai_state_add_order(object->ai, value, counter);
            counter++;
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ai_state_t::ai_state_t() {
	_clock = std::make_shared<Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive>>("", 8);
	poof_time = -1;
	changed = false;
	terminate = false;

	// who are we related to?
	index = INVALID_CHR_REF;
	target = INVALID_CHR_REF;
	owner = INVALID_CHR_REF;
	child = INVALID_CHR_REF;

	// some local storage
	alert = 0;
	state = 0;
	content = 0;
	passage = 0;
	timer = 0;
	for (size_t i = 0; i < STOR_COUNT; ++i) {
		x[i] = 0;
		y[i] = 0;
	}

	// ai memory from the last event
	bumplast = INVALID_CHR_REF;
	bumplast_time = 0;

	attacklast = INVALID_CHR_REF;
	hitlast = INVALID_CHR_REF;
	directionlast = 0;
	damagetypelast = DamageType::DAMAGE_DIRECT;
	lastitemused = INVALID_CHR_REF;
	target_old = INVALID_CHR_REF;

	// message handling
	order_value = 0;
	order_counter = 0;

	// waypoints
	wp_valid = false;
	wp_lst.head = wp_lst.tail = 0;
	astar_timer = 0;
}

ai_state_t::~ai_state_t() {
	_clock = nullptr;
}

//--------------------------------------------------------------------------------------------
ai_state_t *ai_state_reset(ai_state_t *self)
{
	self->_clock->reinit();

	self->poof_time = -1;
	self->changed = false;
	self->terminate = false;

	// who are we related to?
	self->index = INVALID_CHR_REF;
	self->target = INVALID_CHR_REF;
	self->owner = INVALID_CHR_REF;
	self->child = INVALID_CHR_REF;

	// some local storage
	self->alert = 0;         ///< Alerts for AI script
	self->state = 0;
	self->content = 0;
	self->passage = 0;
	self->timer = 0;
	for (size_t i = 0; i < STOR_COUNT; ++i) {
		self->x[i] = 0;
		self->y[i] = 0;
	}

	// ai memory from the last event
	self->bumplast = INVALID_CHR_REF;
	self->bumplast_time = 0;

	self->attacklast = INVALID_CHR_REF;
	self->hitlast = INVALID_CHR_REF;
	self->directionlast = 0;
	self->damagetypelast = DamageType::DAMAGE_DIRECT;
	self->lastitemused = INVALID_CHR_REF;
	self->target_old = INVALID_CHR_REF;

	// message handling
	self->order_value = 0;
	self->order_counter = 0;

	// waypoints
	self->wp_valid = false;
	self->wp_lst.head = self->wp_lst.tail = 0;
	self->astar_timer = 0;

	return self;
}

//--------------------------------------------------------------------------------------------
bool ai_state_add_order(ai_state_t& self, Uint32 value, Uint16 counter)
{
    // this function is only truely valid if there is no other order
	bool retval = HAS_NO_BITS(self.alert, ALERTIF_ORDERED);

	SET_BIT(self.alert, ALERTIF_ORDERED);
    self.order_value   = value;
    self.order_counter = counter;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ai_state_set_changed(ai_state_t& self)
{
    /// @author BB
    /// @details do something tricky here

    bool retval = false;

	if (HAS_NO_BITS(self.alert, ALERTIF_CHANGED))
    {
		SET_BIT(self.alert, ALERTIF_CHANGED);
        retval = true;
    }

	if (!self.changed)
    {
		self.changed = true;
        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ai_state_set_bumplast(ai_state_t& self, const CHR_REF ichr)
{
    /// @author BB
    /// @details bumping into a chest can initiate whole loads of update messages.
    ///     Try to throttle the rate that new "bump" messages can be passed to the ai

	if (!_currentModule->getObjectHandler().exists(ichr)) {
		return false;
	}

    // 5 bumps per second?
	if (self.bumplast != ichr || update_wld > self.bumplast_time + GameEngine::GAME_TARGET_UPS / 5) {
		self.bumplast_time = update_wld;
		SET_BIT(self.alert, ALERTIF_BUMPED);
    }
	self.bumplast = ichr;

    return true;
}

//--------------------------------------------------------------------------------------------
void ai_state_spawn(ai_state_t *self, const CHR_REF index, const PRO_REF iobj, Uint16 rank)
{
    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[index];
	self = ai_state_reset(self);

	if (!self || !pchr) {
		return;
	}

	self->index = index;
	self->alert = ALERTIF_SPAWNED;
	self->state = pchr->getProfile()->getStateOverride();
	self->content = pchr->getProfile()->getContentOverride();
	self->passage = 0;
	self->target = index;
	self->owner = index;
	self->child = index;
	self->target_old = index;

	self->bumplast = index;
	self->hitlast = index;

	self->order_counter = rank;
	self->order_value = 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void script_state_init(script_state_t& self)
{
	self.x = 0;
	self.y = 0;
	self.turn = 0;
	self.distance = 0;
	self.argument = 0;
	self.operationsum = 0;
}
