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

/// @file script.c
/// @brief Implements the game's scripting language.
/// @details

#include "script.h"
#include "script_compile.h"
#include "script_functions.h"

#include "mad.h"

#include "log.h"
#include "camera.h"
#include "game.h"
#include "network.h"

#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo_math.h"

#include "profile.inl"
#include "char.inl"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static const char *  script_error_classname = "UNKNOWN";
static PRO_REF       script_error_model     = ( PRO_REF )MAX_PROFILE;
static const char *  script_error_name      = "UNKNOWN";
static REF_T         script_error_index     = ( Uint16 )( ~0 );

static bool_t scr_increment_exe( ai_state_t * pself );
static bool_t scr_set_exe( ai_state_t * pself, size_t offset );

// static Uint8 run_function_obsolete( script_state_t * pstate, ai_state_t * pself );
static Uint8 scr_run_function( script_state_t * pstate, ai_state_t * pself );
static void  scr_set_operand( script_state_t * pstate, Uint8 variable );
static void  scr_run_operand( script_state_t * pstate, ai_state_t * pself );

static bool_t scr_run_operation( script_state_t * pstate, ai_state_t * pself );
static bool_t scr_run_function_call( script_state_t * pstate, ai_state_t * pself );

PROFILE_DECLARE( script_function )

static int    _script_function_calls[SCRIPT_FUNCTIONS_COUNT];
static double _script_function_times[SCRIPT_FUNCTIONS_COUNT];

static bool_t _scripting_system_initialized = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scripting_system_begin()
{
    if ( !_scripting_system_initialized )
    {
        int cnt;

        PROFILE_INIT( script_function );

        for ( cnt = 0; cnt < SCRIPT_FUNCTIONS_COUNT; cnt++ )
        {
            _script_function_calls[cnt] = 0;
            _script_function_times[cnt] = 0.0F;
        }

        _scripting_system_initialized = btrue;
    }
}

//--------------------------------------------------------------------------------------------
void scripting_system_end()
{
    if ( _scripting_system_initialized )
    {
        PROFILE_FREE( script_function );

#if (DEBUG_SCRIPT_LEVEL > 1 ) && defined(DEBUG_PROFILE) && defined(_DEBUG)
        {
            FILE * ftmp = fopen( vfs_resolveWriteFilename( "/debug/script_function_timing.txt" ), "a+" );

            if ( NULL != ftmp )
            {
                int cnt;

                for ( cnt = 0; cnt < SCRIPT_FUNCTIONS_COUNT; cnt++ )
                {
                    if ( _script_function_calls[cnt] > 0 )
                    {
                        fprintf( ftmp, "function == %d\tname == \"%s\"\tcalls == %d\ttime == %lf\n",
                                 cnt, script_function_names[cnt], _script_function_calls[cnt], _script_function_times[cnt] );
                    }
                }

                fflush( ftmp );
                fclose( ftmp );
            }
        }
#endif

        _scripting_system_initialized = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scr_run_chr_script( const CHR_REF character )
{
    /// @details ZZ@> This function lets one character do AI stuff

    script_state_t   my_state;
    chr_t          * pchr;
    ai_state_t     * pself;

    // make sure that this module is initialized
    scripting_system_begin();

    if ( !INGAME_CHR( character ) )  return;
    pchr  = ChrList.lst + character;
    pself = &( pchr->ai );

    // has the time for this character to die come and gone?
    if ( pself->poof_time >= 0 && pself->poof_time <= ( Sint32 )update_wld ) return;

    // grab the "changed" value from the last time the script was run
    if ( pself->changed )
    {
        SET_BIT( pself->alert, ALERTIF_CHANGED );
        pself->changed = bfalse;
    }

    PROFILE_BEGIN_STRUCT( pself );

    // debug a certain script
    // debug_scripts = ( 385 == pself->index && 76 == pchr->profile_ref );

    // target_old is set to the target every time the script is run
    pself->target_old = pself->target;

    // Make life easier
    script_error_classname = "UNKNOWN";
    script_error_model     = pchr->profile_ref;
    script_error_index     = ( Uint16 )( ~0 );
    script_error_name      = "UNKNOWN";
    if ( script_error_model < MAX_PROFILE )
    {
        CAP_REF icap = pro_get_icap( script_error_model );

        script_error_classname = CapStack.lst[ icap ].classname;

        script_error_index = ProList.lst[script_error_model].iai;
        if ( script_error_index < MAX_AI )
        {
            script_error_name = AisStorage.ary[script_error_index].szName;
        }
    }

    if ( debug_scripts )
    {
        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;

        fprintf( scr_file,  "\n\n--------\n%d - %s\n", script_error_index, script_error_name );
        fprintf( scr_file,  "%d - %s\n", REF_TO_INT( script_error_model ), script_error_classname );

        // who are we related to?
        fprintf( scr_file,  "\tindex  == %d\n", REF_TO_INT( pself->index ) );
        fprintf( scr_file,  "\ttarget == %d\n", REF_TO_INT( pself->target ) );
        fprintf( scr_file,  "\towner  == %d\n", REF_TO_INT( pself->owner ) );
        fprintf( scr_file,  "\tchild  == %d\n", REF_TO_INT( pself->child ) );

        // some local storage
        fprintf( scr_file,  "\talert     == %x\n", pself->alert );
        fprintf( scr_file,  "\tstate     == %d\n", pself->state );
        fprintf( scr_file,  "\tcontent   == %d\n", pself->content );
        fprintf( scr_file,  "\ttimer     == %d\n", pself->timer );
        fprintf( scr_file,  "\tupdate_wld == %d\n", update_wld );

        // ai memory from the last event
        fprintf( scr_file,  "\tbumplast       == %d\n", REF_TO_INT( pself->bumplast ) );
        fprintf( scr_file,  "\tattacklast     == %d\n", REF_TO_INT( pself->attacklast ) );
        fprintf( scr_file,  "\thitlast        == %d\n", REF_TO_INT( pself->hitlast ) );
        fprintf( scr_file,  "\tdirectionlast  == %d\n", pself->directionlast );
        fprintf( scr_file,  "\tdamagetypelast == %d\n", pself->damagetypelast );
        fprintf( scr_file,  "\tlastitemused   == %d\n", REF_TO_INT( pself->lastitemused ) );
        fprintf( scr_file,  "\ttarget_old     == %d\n", REF_TO_INT( pself->target_old ) );

        // message handling
        fprintf( scr_file,  "\torder == %d\n", pself->order_value );
        fprintf( scr_file,  "\tcounter == %d\n", pself->order_counter );

        // waypoints
        fprintf( scr_file,  "\twp_tail == %d\n", pself->wp_lst.tail );
        fprintf( scr_file,  "\twp_head == %d\n\n", pself->wp_lst.head );
    }

    // Clear the button latches
    if ( !VALID_PLA( pchr->is_which_player ) )
    {
        RESET_BIT_FIELD( pchr->latch.b );
    }

    // Reset the target if it can't be seen
    if (( pself->target != pself->index ) && !chr_can_see_object( character, pself->target ) )
    {
        pself->target = pself->index;
    }

    // reset the script state
    memset( &my_state, 0, sizeof( my_state ) );

    // reset the ai
    pself->terminate = bfalse;
    pself->indent    = 0;
    pself->exe_stt   = AisStorage.ary[pself->type].iStartPosition;
    pself->exe_end   = AisStorage.ary[pself->type].iEndPosition;

    // Run the AI Script
    scr_set_exe( pself, pself->exe_stt );
    while ( !pself->terminate && pself->exe_pos < pself->exe_end )
    {
        // This is used by the Else function
        // it only keeps track of functions
        pself->indent_last = pself->indent;
        pself->indent = GET_DATA_BITS( pself->opcode );

        // Was it a function
        if ( HAS_SOME_BITS( pself->opcode, FUNCTION_BIT ) )
        {
            if ( !scr_run_function_call( &my_state, pself ) )
            {
                break;
            }
        }
        else
        {
            if ( !scr_run_operation( &my_state, pself ) )
            {
                break;
            }
        }
    }

    // Set latches
    if ( !VALID_PLA( pchr->is_which_player ) )
    {
        float latch2;

        ai_state_ensure_wp( pself );

        if ( pchr->ismount && INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
        {
            // Mount
            pchr->latch.x = ChrList.lst[pchr->holdingwhich[SLOT_LEFT]].latch.x;
            pchr->latch.y = ChrList.lst[pchr->holdingwhich[SLOT_LEFT]].latch.y;
        }
        else if ( pself->wp_valid )
        {
            // Normal AI
            pchr->latch.x = ( pself->wp[kX] - pchr->pos.x ) / ( GRID_ISIZE << 2 );
            pchr->latch.y = ( pself->wp[kY] - pchr->pos.y ) / ( GRID_ISIZE << 2 );
        }
        else
        {
            // AI, but no valid waypoints
            pchr->latch.x = 0;
            pchr->latch.y = 0;
        }

        latch2 = pchr->latch.x * pchr->latch.x + pchr->latch.y * pchr->latch.y;
        if ( latch2 > 1.0f )
        {
            float scale = 1.0f / SQRT( latch2 );
            pchr->latch.x *= scale;
            pchr->latch.y *= scale;
        }
    }

    // Clear alerts for next time around
    RESET_BIT_FIELD( pself->alert );

    PROFILE_END2_STRUCT( pself );
}

//--------------------------------------------------------------------------------------------
bool_t scr_run_function_call( script_state_t * pstate, ai_state_t * pself )
{
    Uint8  functionreturn;

    // check for valid pointers
    if ( NULL == pstate || NULL == pself ) return bfalse;

    // check for valid execution pointer
    if ( pself->exe_pos < pself->exe_stt || pself->exe_pos >= pself->exe_end ) return bfalse;

    // Run the function
    functionreturn = scr_run_function( pstate, pself );

    // move the execution pointer to the jump code
    scr_increment_exe( pself );
    if ( functionreturn )
    {
        // move the execution pointer to the next opcode
        scr_increment_exe( pself );
    }
    else
    {
        // use the jump code to jump to the right location
        size_t new_index = pself->opcode;

        // make sure the value is valid
        EGOBOO_ASSERT( new_index < AISMAXCOMPILESIZE && new_index >= pself->exe_stt && new_index <= pself->exe_end );

        // actually do the jump
        scr_set_exe( pself, new_index );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t scr_run_operation( script_state_t * pstate, ai_state_t * pself )
{
    const char * variable;
    Uint32 var_value, operand_count, i;

    // check for valid pointers
    if ( NULL == pstate || NULL == pself ) return bfalse;

    // check for valid execution pointer
    if ( pself->exe_pos < pself->exe_stt || pself->exe_pos >= pself->exe_end ) return bfalse;

    var_value = pself->opcode & VALUE_BITS;

    // debug stuff
    variable = "UNKNOWN";
    if ( debug_scripts )
    {
        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;

        for ( i = 0; i < pself->indent; i++ ) { fprintf( scr_file, "  " ); }

        for ( i = 0; i < MAX_OPCODE; i++ )
        {
            if ( 'V' == OpList.ary[i].cType && var_value == OpList.ary[i].iValue )
            {
                variable = OpList.ary[i].cName;
                break;
            };
        }

        fprintf( scr_file, "%s = ", variable );
    }

    // Get the number of operands
    scr_increment_exe( pself );
    operand_count = pself->opcode;

    // Now run the operation
    pstate->operationsum = 0;
    for ( i = 0; i < operand_count && pself->exe_pos < pself->exe_end; i++ )
    {
        scr_increment_exe( pself );
        scr_run_operand( pstate, pself );
    }
    if ( debug_scripts )
    {
        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;
        fprintf( scr_file, " == %d \n", pstate->operationsum );
    }

    // Save the results in the register that called the arithmetic
    scr_set_operand( pstate, var_value );

    // go to the next opcode
    scr_increment_exe( pself );

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint8 scr_run_function( script_state_t * pstate, ai_state_t * pself )
{
    /// @details BB@> This is about half-way to what is needed for Lua integration

    // Mask out the indentation
    Uint32 valuecode = pself->opcode & VALUE_BITS;

    // Assume that the function will pass, as most do
    Uint8 returncode = btrue;
    if ( MAX_OPCODE == valuecode )
    {
        log_message( "SCRIPT ERROR: scr_run_function() - model == %d, class name == \"%s\" - Unknown opcode found!\n", REF_TO_INT( script_error_model ), script_error_classname );
        return bfalse;
    }

    // debug stuff
    if ( debug_scripts )
    {
        Uint32 i;
        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;

        for ( i = 0; i < pself->indent; i++ ) { fprintf( scr_file,  "  " ); }

        for ( i = 0; i < MAX_OPCODE; i++ )
        {
            if ( 'F' == OpList.ary[i].cType && valuecode == OpList.ary[i].iValue )
            {
                fprintf( scr_file,  "%s\n", OpList.ary[i].cName );
                break;
            };
        }
    }

    if ( valuecode > SCRIPT_FUNCTIONS_COUNT )
    {
    }
    else
    {
        PROFILE_RESET( script_function );

        PROFILE_BEGIN( script_function )
        {
            // Figure out which function to run
            switch ( valuecode )
            {
                case FIFSPAWNED: returncode = scr_Spawned( pstate, pself ); break;
                case FIFTIMEOUT: returncode = scr_TimeOut( pstate, pself ); break;
                case FIFATWAYPOINT: returncode = scr_AtWaypoint( pstate, pself ); break;
                case FIFATLASTWAYPOINT: returncode = scr_AtLastWaypoint( pstate, pself ); break;
                case FIFATTACKED: returncode = scr_Attacked( pstate, pself ); break;
                case FIFBUMPED: returncode = scr_Bumped( pstate, pself ); break;
                case FIFORDERED: returncode = scr_Ordered( pstate, pself ); break;
                case FIFCALLEDFORHELP: returncode = scr_CalledForHelp( pstate, pself ); break;
                case FSETCONTENT: returncode = scr_set_Content( pstate, pself ); break;
                case FIFKILLED: returncode = scr_Killed( pstate, pself ); break;
                case FIFTARGETKILLED: returncode = scr_TargetKilled( pstate, pself ); break;
                case FCLEARWAYPOINTS: returncode = scr_ClearWaypoints( pstate, pself ); break;
                case FADDWAYPOINT: returncode = scr_AddWaypoint( pstate, pself ); break;
                case FFINDPATH: returncode = scr_FindPath( pstate, pself ); break;
                case FCOMPASS: returncode = scr_Compass( pstate, pself ); break;
                case FGETTARGETARMORPRICE: returncode = scr_get_TargetArmorPrice( pstate, pself ); break;
                case FSETTIME: returncode = scr_set_Time( pstate, pself ); break;
                case FGETCONTENT: returncode = scr_get_Content( pstate, pself ); break;
                case FJOINTARGETTEAM: returncode = scr_JoinTargetTeam( pstate, pself ); break;
                case FSETTARGETTONEARBYENEMY: returncode = scr_set_TargetToNearbyEnemy( pstate, pself ); break;
                case FSETTARGETTOTARGETLEFTHAND: returncode = scr_set_TargetToTargetLeftHand( pstate, pself ); break;
                case FSETTARGETTOTARGETRIGHTHAND: returncode = scr_set_TargetToTargetRightHand( pstate, pself ); break;
                case FSETTARGETTOWHOEVERATTACKED: returncode = scr_set_TargetToWhoeverAttacked( pstate, pself ); break;
                case FSETTARGETTOWHOEVERBUMPED: returncode = scr_set_TargetToWhoeverBumped( pstate, pself ); break;
                case FSETTARGETTOWHOEVERCALLEDFORHELP: returncode = scr_set_TargetToWhoeverCalledForHelp( pstate, pself ); break;
                case FSETTARGETTOOLDTARGET: returncode = scr_set_TargetToOldTarget( pstate, pself ); break;
                case FSETTURNMODETOVELOCITY: returncode = scr_set_TurnModeToVelocity( pstate, pself ); break;
                case FSETTURNMODETOWATCH: returncode = scr_set_TurnModeToWatch( pstate, pself ); break;
                case FSETTURNMODETOSPIN: returncode = scr_set_TurnModeToSpin( pstate, pself ); break;
                case FSETBUMPHEIGHT: returncode = scr_set_BumpHeight( pstate, pself ); break;
                case FIFTARGETHASID: returncode = scr_TargetHasID( pstate, pself ); break;
                case FIFTARGETHASITEMID: returncode = scr_TargetHasItemID( pstate, pself ); break;
                case FIFTARGETHOLDINGITEMID: returncode = scr_TargetHoldingItemID( pstate, pself ); break;
                case FIFTARGETHASSKILLID: returncode = scr_TargetHasSkillID( pstate, pself ); break;
                case FELSE: returncode = scr_Else( pstate, pself ); break;
                case FRUN: returncode = scr_Run( pstate, pself ); break;
                case FWALK: returncode = scr_Walk( pstate, pself ); break;
                case FSNEAK: returncode = scr_Sneak( pstate, pself ); break;
                case FDOACTION: returncode = scr_DoAction( pstate, pself ); break;
                case FKEEPACTION: returncode = scr_KeepAction( pstate, pself ); break;
                case FISSUEORDER: returncode = scr_IssueOrder( pstate, pself ); break;
                case FDROPWEAPONS: returncode = scr_DropWeapons( pstate, pself ); break;
                case FTARGETDOACTION: returncode = scr_TargetDoAction( pstate, pself ); break;
                case FOPENPASSAGE: returncode = scr_OpenPassage( pstate, pself ); break;
                case FCLOSEPASSAGE: returncode = scr_ClosePassage( pstate, pself ); break;
                case FIFPASSAGEOPEN: returncode = scr_PassageOpen( pstate, pself ); break;
                case FGOPOOF: returncode = scr_GoPoof( pstate, pself ); break;
                case FCOSTTARGETITEMID: returncode = scr_CostTargetItemID( pstate, pself ); break;
                case FDOACTIONOVERRIDE: returncode = scr_DoActionOverride( pstate, pself ); break;
                case FIFHEALED: returncode = scr_Healed( pstate, pself ); break;
                case FSENDMESSAGE: returncode = scr_SendPlayerMessage( pstate, pself ); break;
                case FCALLFORHELP: returncode = scr_CallForHelp( pstate, pself ); break;
                case FADDIDSZ: returncode = scr_AddIDSZ( pstate, pself ); break;
                case FSETSTATE: returncode = scr_set_State( pstate, pself ); break;
                case FGETSTATE: returncode = scr_get_State( pstate, pself ); break;
                case FIFSTATEIS: returncode = scr_StateIs( pstate, pself ); break;
                case FIFTARGETCANOPENSTUFF: returncode = scr_TargetCanOpenStuff( pstate, pself ); break;
                case FIFGRABBED: returncode = scr_Grabbed( pstate, pself ); break;
                case FIFDROPPED: returncode = scr_Dropped( pstate, pself ); break;
                case FSETTARGETTOWHOEVERISHOLDING: returncode = scr_set_TargetToWhoeverIsHolding( pstate, pself ); break;
                case FDAMAGETARGET: returncode = scr_DamageTarget( pstate, pself ); break;
                case FIFXISLESSTHANY: returncode = scr_XIsLessThanY( pstate, pself ); break;
                case FSETWEATHERTIME: returncode = scr_set_WeatherTime( pstate, pself ); break;
                case FGETBUMPHEIGHT: returncode = scr_get_BumpHeight( pstate, pself ); break;
                case FIFREAFFIRMED: returncode = scr_Reaffirmed( pstate, pself ); break;
                case FUNKEEPACTION: returncode = scr_UnkeepAction( pstate, pself ); break;
                case FIFTARGETISONOTHERTEAM: returncode = scr_TargetIsOnOtherTeam( pstate, pself ); break;
                case FIFTARGETISONHATEDTEAM: returncode = scr_TargetIsOnHatedTeam( pstate, pself ); break;
                case FPRESSLATCHBUTTON: returncode = scr_PressLatchButton( pstate, pself ); break;
                case FSETTARGETTOTARGETOFLEADER: returncode = scr_set_TargetToTargetOfLeader( pstate, pself ); break;
                case FIFLEADERKILLED: returncode = scr_LeaderKilled( pstate, pself ); break;
                case FBECOMELEADER: returncode = scr_BecomeLeader( pstate, pself ); break;
                case FCHANGETARGETARMOR: returncode = scr_ChangeTargetArmor( pstate, pself ); break;
                case FGIVEMONEYTOTARGET: returncode = scr_GiveMoneyToTarget( pstate, pself ); break;
                case FDROPKEYS: returncode = scr_DropKeys( pstate, pself ); break;
                case FIFLEADERISALIVE: returncode = scr_LeaderIsAlive( pstate, pself ); break;
                case FIFTARGETISOLDTARGET: returncode = scr_TargetIsOldTarget( pstate, pself ); break;
                case FSETTARGETTOLEADER: returncode = scr_set_TargetToLeader( pstate, pself ); break;
                case FSPAWNCHARACTER: returncode = scr_SpawnCharacter( pstate, pself ); break;
                case FRESPAWNCHARACTER: returncode = scr_RespawnCharacter( pstate, pself ); break;
                case FCHANGETILE: returncode = scr_ChangeTile( pstate, pself ); break;
                case FIFUSED: returncode = scr_Used( pstate, pself ); break;
                case FDROPMONEY: returncode = scr_DropMoney( pstate, pself ); break;
                case FSETOLDTARGET: returncode = scr_set_OldTarget( pstate, pself ); break;
                case FDETACHFROMHOLDER: returncode = scr_DetachFromHolder( pstate, pself ); break;
                case FIFTARGETHASVULNERABILITYID: returncode = scr_TargetHasVulnerabilityID( pstate, pself ); break;
                case FCLEANUP: returncode = scr_CleanUp( pstate, pself ); break;
                case FIFCLEANEDUP: returncode = scr_CleanedUp( pstate, pself ); break;
                case FIFSITTING: returncode = scr_Sitting( pstate, pself ); break;
                case FIFTARGETISHURT: returncode = scr_TargetIsHurt( pstate, pself ); break;
                case FIFTARGETISAPLAYER: returncode = scr_TargetIsAPlayer( pstate, pself ); break;
                case FPLAYSOUND: returncode = scr_PlaySound( pstate, pself ); break;
                case FSPAWNPARTICLE: returncode = scr_SpawnParticle( pstate, pself ); break;
                case FIFTARGETISALIVE: returncode = scr_TargetIsAlive( pstate, pself ); break;
                case FSTOP: returncode = scr_Stop( pstate, pself ); break;
                case FDISAFFIRMCHARACTER: returncode = scr_DisaffirmCharacter( pstate, pself ); break;
                case FREAFFIRMCHARACTER: returncode = scr_ReaffirmCharacter( pstate, pself ); break;
                case FIFTARGETISSELF: returncode = scr_TargetIsSelf( pstate, pself ); break;
                case FIFTARGETISMALE: returncode = scr_TargetIsMale( pstate, pself ); break;
                case FIFTARGETISFEMALE: returncode = scr_TargetIsFemale( pstate, pself ); break;
                case FSETTARGETTOSELF: returncode = scr_set_TargetToSelf( pstate, pself ); break;
                case FSETTARGETTORIDER: returncode = scr_set_TargetToRider( pstate, pself ); break;
                case FGETATTACKTURN: returncode = scr_get_AttackTurn( pstate, pself ); break;
                case FGETDAMAGETYPE: returncode = scr_get_DamageType( pstate, pself ); break;
                case FBECOMESPELL: returncode = scr_BecomeSpell( pstate, pself ); break;
                case FBECOMESPELLBOOK: returncode = scr_BecomeSpellbook( pstate, pself ); break;
                case FIFSCOREDAHIT: returncode = scr_ScoredAHit( pstate, pself ); break;
                case FIFDISAFFIRMED: returncode = scr_Disaffirmed( pstate, pself ); break;
                case FTRANSLATEORDER: returncode = scr_TranslateOrder( pstate, pself ); break;
                case FSETTARGETTOWHOEVERWASHIT: returncode = scr_set_TargetToWhoeverWasHit( pstate, pself ); break;
                case FSETTARGETTOWIDEENEMY: returncode = scr_set_TargetToWideEnemy( pstate, pself ); break;
                case FIFCHANGED: returncode = scr_Changed( pstate, pself ); break;
                case FIFINWATER: returncode = scr_InWater( pstate, pself ); break;
                case FIFBORED: returncode = scr_Bored( pstate, pself ); break;
                case FIFTOOMUCHBAGGAGE: returncode = scr_TooMuchBaggage( pstate, pself ); break;
                case FIFGROGGED: returncode = scr_Grogged( pstate, pself ); break;
                case FIFDAZED: returncode = scr_Dazed( pstate, pself ); break;
                case FIFTARGETHASSPECIALID: returncode = scr_TargetHasSpecialID( pstate, pself ); break;
                case FPRESSTARGETLATCHBUTTON: returncode = scr_PressTargetLatchButton( pstate, pself ); break;
                case FIFINVISIBLE: returncode = scr_Invisible( pstate, pself ); break;
                case FIFARMORIS: returncode = scr_ArmorIs( pstate, pself ); break;
                case FGETTARGETGROGTIME: returncode = scr_get_TargetGrogTime( pstate, pself ); break;
                case FGETTARGETDAZETIME: returncode = scr_get_TargetDazeTime( pstate, pself ); break;
                case FSETDAMAGETYPE: returncode = scr_set_DamageType( pstate, pself ); break;
                case FSETWATERLEVEL: returncode = scr_set_WaterLevel( pstate, pself ); break;
                case FENCHANTTARGET: returncode = scr_EnchantTarget( pstate, pself ); break;
                case FENCHANTCHILD: returncode = scr_EnchantChild( pstate, pself ); break;
                case FTELEPORTTARGET: returncode = scr_TeleportTarget( pstate, pself ); break;
                case FGIVEEXPERIENCETOTARGET: returncode = scr_GiveExperienceToTarget( pstate, pself ); break;
                case FINCREASEAMMO: returncode = scr_IncreaseAmmo( pstate, pself ); break;
                case FUNKURSETARGET: returncode = scr_UnkurseTarget( pstate, pself ); break;
                case FGIVEEXPERIENCETOTARGETTEAM: returncode = scr_GiveExperienceToTargetTeam( pstate, pself ); break;
                case FIFUNARMED: returncode = scr_Unarmed( pstate, pself ); break;
                case FRESTOCKTARGETAMMOIDALL: returncode = scr_RestockTargetAmmoIDAll( pstate, pself ); break;
                case FRESTOCKTARGETAMMOIDFIRST: returncode = scr_RestockTargetAmmoIDFirst( pstate, pself ); break;
                case FFLASHTARGET: returncode = scr_FlashTarget( pstate, pself ); break;
                case FSETREDSHIFT: returncode = scr_set_RedShift( pstate, pself ); break;
                case FSETGREENSHIFT: returncode = scr_set_GreenShift( pstate, pself ); break;
                case FSETBLUESHIFT: returncode = scr_set_BlueShift( pstate, pself ); break;
                case FSETLIGHT: returncode = scr_set_Light( pstate, pself ); break;
                case FSETALPHA: returncode = scr_set_Alpha( pstate, pself ); break;
                case FIFHITFROMBEHIND: returncode = scr_HitFromBehind( pstate, pself ); break;
                case FIFHITFROMFRONT: returncode = scr_HitFromFront( pstate, pself ); break;
                case FIFHITFROMLEFT: returncode = scr_HitFromLeft( pstate, pself ); break;
                case FIFHITFROMRIGHT: returncode = scr_HitFromRight( pstate, pself ); break;
                case FIFTARGETISONSAMETEAM: returncode = scr_TargetIsOnSameTeam( pstate, pself ); break;
                case FKILLTARGET: returncode = scr_KillTarget( pstate, pself ); break;
                case FUNDOENCHANT: returncode = scr_UndoEnchant( pstate, pself ); break;
                case FGETWATERLEVEL: returncode = scr_get_WaterLevel( pstate, pself ); break;
                case FCOSTTARGETMANA: returncode = scr_CostTargetMana( pstate, pself ); break;
                case FIFTARGETHASANYID: returncode = scr_TargetHasAnyID( pstate, pself ); break;
                case FSETBUMPSIZE: returncode = scr_set_BumpSize( pstate, pself ); break;
                case FIFNOTDROPPED: returncode = scr_NotDropped( pstate, pself ); break;
                case FIFYISLESSTHANX: returncode = scr_YIsLessThanX( pstate, pself ); break;
                case FSETFLYHEIGHT: returncode = scr_set_FlyHeight( pstate, pself ); break;
                case FIFBLOCKED: returncode = scr_Blocked( pstate, pself ); break;
                case FIFTARGETISDEFENDING: returncode = scr_TargetIsDefending( pstate, pself ); break;
                case FIFTARGETISATTACKING: returncode = scr_TargetIsAttacking( pstate, pself ); break;
                case FIFSTATEIS0: returncode = scr_StateIs0( pstate, pself ); break;
                case FIFSTATEIS1: returncode = scr_StateIs1( pstate, pself ); break;
                case FIFSTATEIS2: returncode = scr_StateIs2( pstate, pself ); break;
                case FIFSTATEIS3: returncode = scr_StateIs3( pstate, pself ); break;
                case FIFSTATEIS4: returncode = scr_StateIs4( pstate, pself ); break;
                case FIFSTATEIS5: returncode = scr_StateIs5( pstate, pself ); break;
                case FIFSTATEIS6: returncode = scr_StateIs6( pstate, pself ); break;
                case FIFSTATEIS7: returncode = scr_StateIs7( pstate, pself ); break;
                case FIFCONTENTIS: returncode = scr_ContentIs( pstate, pself ); break;
                case FSETTURNMODETOWATCHTARGET: returncode = scr_set_TurnModeToWatchTarget( pstate, pself ); break;
                case FIFSTATEISNOT: returncode = scr_StateIsNot( pstate, pself ); break;
                case FIFXISEQUALTOY: returncode = scr_XIsEqualToY( pstate, pself ); break;
                case FDEBUGMESSAGE: returncode = scr_DebugMessage( pstate, pself ); break;
                case FBLACKTARGET: returncode = scr_BlackTarget( pstate, pself ); break;
                case FSENDMESSAGENEAR: returncode = scr_SendMessageNear( pstate, pself ); break;
                case FIFHITGROUND: returncode = scr_HitGround( pstate, pself ); break;
                case FIFNAMEISKNOWN: returncode = scr_NameIsKnown( pstate, pself ); break;
                case FIFUSAGEISKNOWN: returncode = scr_UsageIsKnown( pstate, pself ); break;
                case FIFHOLDINGITEMID: returncode = scr_HoldingItemID( pstate, pself ); break;
                case FIFHOLDINGRANGEDWEAPON: returncode = scr_HoldingRangedWeapon( pstate, pself ); break;
                case FIFHOLDINGMELEEWEAPON: returncode = scr_HoldingMeleeWeapon( pstate, pself ); break;
                case FIFHOLDINGSHIELD: returncode = scr_HoldingShield( pstate, pself ); break;
                case FIFKURSED: returncode = scr_Kursed( pstate, pself ); break;
                case FIFTARGETISKURSED: returncode = scr_TargetIsKursed( pstate, pself ); break;
                case FIFTARGETISDRESSEDUP: returncode = scr_TargetIsDressedUp( pstate, pself ); break;
                case FIFOVERWATER: returncode = scr_OverWater( pstate, pself ); break;
                case FIFTHROWN: returncode = scr_Thrown( pstate, pself ); break;
                case FMAKENAMEKNOWN: returncode = scr_MakeNameKnown( pstate, pself ); break;
                case FMAKEUSAGEKNOWN: returncode = scr_MakeUsageKnown( pstate, pself ); break;
                case FSTOPTARGETMOVEMENT: returncode = scr_StopTargetMovement( pstate, pself ); break;
                case FSETXY: returncode = scr_set_XY( pstate, pself ); break;
                case FGETXY: returncode = scr_get_XY( pstate, pself ); break;
                case FADDXY: returncode = scr_AddXY( pstate, pself ); break;
                case FMAKEAMMOKNOWN: returncode = scr_MakeAmmoKnown( pstate, pself ); break;
                case FSPAWNATTACHEDPARTICLE: returncode = scr_SpawnAttachedParticle( pstate, pself ); break;
                case FSPAWNEXACTPARTICLE: returncode = scr_SpawnExactParticle( pstate, pself ); break;
                case FACCELERATETARGET: returncode = scr_AccelerateTarget( pstate, pself ); break;
                case FIFDISTANCEISMORETHANTURN: returncode = scr_distanceIsMoreThanTurn( pstate, pself ); break;
                case FIFCRUSHED: returncode = scr_Crushed( pstate, pself ); break;
                case FMAKECRUSHVALID: returncode = scr_MakeCrushValid( pstate, pself ); break;
                case FSETTARGETTOLOWESTTARGET: returncode = scr_set_TargetToLowestTarget( pstate, pself ); break;
                case FIFNOTPUTAWAY: returncode = scr_NotPutAway( pstate, pself ); break;
                case FIFTAKENOUT: returncode = scr_TakenOut( pstate, pself ); break;
                case FIFAMMOOUT: returncode = scr_AmmoOut( pstate, pself ); break;
                case FPLAYSOUNDLOOPED: returncode = scr_PlaySoundLooped( pstate, pself ); break;
                case FSTOPSOUND: returncode = scr_StopSound( pstate, pself ); break;
                case FHEALSELF: returncode = scr_HealSelf( pstate, pself ); break;
                case FEQUIP: returncode = scr_Equip( pstate, pself ); break;
                case FIFTARGETHASITEMIDEQUIPPED: returncode = scr_TargetHasItemIDEquipped( pstate, pself ); break;
                case FSETOWNERTOTARGET: returncode = scr_set_OwnerToTarget( pstate, pself ); break;
                case FSETTARGETTOOWNER: returncode = scr_set_TargetToOwner( pstate, pself ); break;
                case FSETFRAME: returncode = scr_set_Frame( pstate, pself ); break;
                case FBREAKPASSAGE: returncode = scr_BreakPassage( pstate, pself ); break;
                case FSETRELOADTIME: returncode = scr_set_ReloadTime( pstate, pself ); break;
                case FSETTARGETTOWIDEBLAHID: returncode = scr_set_TargetToWideBlahID( pstate, pself ); break;
                case FPOOFTARGET: returncode = scr_PoofTarget( pstate, pself ); break;
                case FCHILDDOACTIONOVERRIDE: returncode = scr_ChildDoActionOverride( pstate, pself ); break;
                case FSPAWNPOOF: returncode = scr_SpawnPoof( pstate, pself ); break;
                case FSETSPEEDPERCENT: returncode = scr_set_SpeedPercent( pstate, pself ); break;
                case FSETCHILDSTATE: returncode = scr_set_ChildState( pstate, pself ); break;
                case FSPAWNATTACHEDSIZEDPARTICLE: returncode = scr_SpawnAttachedSizedParticle( pstate, pself ); break;
                case FCHANGEARMOR: returncode = scr_ChangeArmor( pstate, pself ); break;
                case FSHOWTIMER: returncode = scr_ShowTimer( pstate, pself ); break;
                case FIFFACINGTARGET: returncode = scr_FacingTarget( pstate, pself ); break;
                case FPLAYSOUNDVOLUME: returncode = scr_PlaySoundVolume( pstate, pself ); break;
                case FSPAWNATTACHEDFACEDPARTICLE: returncode = scr_SpawnAttachedFacedParticle( pstate, pself ); break;
                case FIFSTATEISODD: returncode = scr_StateIsOdd( pstate, pself ); break;
                case FSETTARGETTODISTANTENEMY: returncode = scr_set_TargetToDistantEnemy( pstate, pself ); break;
                case FTELEPORT: returncode = scr_Teleport( pstate, pself ); break;
                case FGIVESTRENGTHTOTARGET: returncode = scr_GiveStrengthToTarget( pstate, pself ); break;
                case FGIVEWISDOMTOTARGET: returncode = scr_GiveWisdomToTarget( pstate, pself ); break;
                case FGIVEINTELLIGENCETOTARGET: returncode = scr_GiveIntelligenceToTarget( pstate, pself ); break;
                case FGIVEDEXTERITYTOTARGET: returncode = scr_GiveDexterityToTarget( pstate, pself ); break;
                case FGIVELIFETOTARGET: returncode = scr_GiveLifeToTarget( pstate, pself ); break;
                case FGIVEMANATOTARGET: returncode = scr_GiveManaToTarget( pstate, pself ); break;
                case FSHOWMAP: returncode = scr_ShowMap( pstate, pself ); break;
                case FSHOWYOUAREHERE: returncode = scr_ShowYouAreHere( pstate, pself ); break;
                case FSHOWBLIPXY: returncode = scr_ShowBlipXY( pstate, pself ); break;
                case FHEALTARGET: returncode = scr_HealTarget( pstate, pself ); break;
                case FPUMPTARGET: returncode = scr_PumpTarget( pstate, pself ); break;
                case FCOSTAMMO: returncode = scr_CostAmmo( pstate, pself ); break;
                case FMAKESIMILARNAMESKNOWN: returncode = scr_MakeSimilarNamesKnown( pstate, pself ); break;
                case FSPAWNATTACHEDHOLDERPARTICLE: returncode = scr_SpawnAttachedHolderParticle( pstate, pself ); break;
                case FSETTARGETRELOADTIME: returncode = scr_set_TargetReloadTime( pstate, pself ); break;
                case FSETFOGLEVEL: returncode = scr_set_FogLevel( pstate, pself ); break;
                case FGETFOGLEVEL: returncode = scr_get_FogLevel( pstate, pself ); break;
                case FSETFOGTAD: returncode = scr_set_FogTAD( pstate, pself ); break;
                case FSETFOGBOTTOMLEVEL: returncode = scr_set_FogBottomLevel( pstate, pself ); break;
                case FGETFOGBOTTOMLEVEL: returncode = scr_get_FogBottomLevel( pstate, pself ); break;
                case FCORRECTACTIONFORHAND: returncode = scr_CorrectActionForHand( pstate, pself ); break;
                case FIFTARGETISMOUNTED: returncode = scr_TargetIsMounted( pstate, pself ); break;
                case FSPARKLEICON: returncode = scr_SparkleIcon( pstate, pself ); break;
                case FUNSPARKLEICON: returncode = scr_UnsparkleIcon( pstate, pself ); break;
                case FGETTILEXY: returncode = scr_get_TileXY( pstate, pself ); break;
                case FSETTILEXY: returncode = scr_set_TileXY( pstate, pself ); break;
                case FSETSHADOWSIZE: returncode = scr_set_ShadowSize( pstate, pself ); break;
                case FORDERTARGET: returncode = scr_OrderTarget( pstate, pself ); break;
                case FSETTARGETTOWHOEVERISINPASSAGE: returncode = scr_set_TargetToWhoeverIsInPassage( pstate, pself ); break;
                case FIFCHARACTERWASABOOK: returncode = scr_CharacterWasABook( pstate, pself ); break;
                case FSETENCHANTBOOSTVALUES: returncode = scr_set_EnchantBoostValues( pstate, pself ); break;
                case FSPAWNCHARACTERXYZ: returncode = scr_SpawnCharacterXYZ( pstate, pself ); break;
                case FSPAWNEXACTCHARACTERXYZ: returncode = scr_SpawnExactCharacterXYZ( pstate, pself ); break;
                case FCHANGETARGETCLASS: returncode = scr_ChangeTargetClass( pstate, pself ); break;
                case FPLAYFULLSOUND: returncode = scr_PlayFullSound( pstate, pself ); break;
                case FSPAWNEXACTCHASEPARTICLE: returncode = scr_SpawnExactChaseParticle( pstate, pself ); break;
                case FCREATEORDER: returncode = scr_CreateOrder( pstate, pself ); break;
                case FORDERSPECIALID: returncode = scr_OrderSpecialID( pstate, pself ); break;
                case FUNKURSETARGETINVENTORY: returncode = scr_UnkurseTargetInventory( pstate, pself ); break;
                case FIFTARGETISSNEAKING: returncode = scr_TargetIsSneaking( pstate, pself ); break;
                case FDROPITEMS: returncode = scr_DropItems( pstate, pself ); break;
                case FRESPAWNTARGET: returncode = scr_RespawnTarget( pstate, pself ); break;
                case FTARGETDOACTIONSETFRAME: returncode = scr_TargetDoActionSetFrame( pstate, pself ); break;
                case FIFTARGETCANSEEINVISIBLE: returncode = scr_TargetCanSeeInvisible( pstate, pself ); break;
                case FSETTARGETTONEARESTBLAHID: returncode = scr_set_TargetToNearestBlahID( pstate, pself ); break;
                case FSETTARGETTONEARESTENEMY: returncode = scr_set_TargetToNearestEnemy( pstate, pself ); break;
                case FSETTARGETTONEARESTFRIEND: returncode = scr_set_TargetToNearestFriend( pstate, pself ); break;
                case FSETTARGETTONEARESTLIFEFORM: returncode = scr_set_TargetToNearestLifeform( pstate, pself ); break;
                case FFLASHPASSAGE: returncode = scr_FlashPassage( pstate, pself ); break;
                case FFINDTILEINPASSAGE: returncode = scr_FindTileInPassage( pstate, pself ); break;
                case FIFHELDINLEFTHAND: returncode = scr_HeldInLeftHand( pstate, pself ); break;
                case FNOTANITEM: returncode = scr_NotAnItem( pstate, pself ); break;
                case FSETCHILDAMMO: returncode = scr_set_ChildAmmo( pstate, pself ); break;
                case FIFHITVULNERABLE: returncode = scr_HitVulnerable( pstate, pself ); break;
                case FIFTARGETISFLYING: returncode = scr_TargetIsFlying( pstate, pself ); break;
                case FIDENTIFYTARGET: returncode = scr_IdentifyTarget( pstate, pself ); break;
                case FBEATMODULE: returncode = scr_BeatModule( pstate, pself ); break;
                case FENDMODULE: returncode = scr_EndModule( pstate, pself ); break;
                case FDISABLEEXPORT: returncode = scr_DisableExport( pstate, pself ); break;
                case FENABLEEXPORT: returncode = scr_EnableExport( pstate, pself ); break;
                case FGETTARGETSTATE: returncode = scr_get_TargetState( pstate, pself ); break;
                case FIFEQUIPPED: returncode = scr_Equipped( pstate, pself ); break;
                case FDROPTARGETMONEY: returncode = scr_DropTargetMoney( pstate, pself ); break;
                case FGETTARGETCONTENT: returncode = scr_get_TargetContent( pstate, pself ); break;
                case FDROPTARGETKEYS: returncode = scr_DropTargetKeys( pstate, pself ); break;
                case FJOINTEAM: returncode = scr_JoinTeam( pstate, pself ); break;
                case FTARGETJOINTEAM: returncode = scr_TargetJoinTeam( pstate, pself ); break;
                case FCLEARMUSICPASSAGE: returncode = scr_ClearMusicPassage( pstate, pself ); break;
                case FCLEARENDMESSAGE: returncode = scr_ClearEndMessage( pstate, pself ); break;
                case FADDENDMESSAGE: returncode = scr_AddEndMessage( pstate, pself ); break;
                case FPLAYMUSIC: returncode = scr_PlayMusic( pstate, pself ); break;
                case FSETMUSICPASSAGE: returncode = scr_set_MusicPassage( pstate, pself ); break;
                case FMAKECRUSHINVALID: returncode = scr_MakeCrushInvalid( pstate, pself ); break;
                case FSTOPMUSIC: returncode = scr_StopMusic( pstate, pself ); break;
                case FFLASHVARIABLE: returncode = scr_FlashVariable( pstate, pself ); break;
                case FACCELERATEUP: returncode = scr_AccelerateUp( pstate, pself ); break;
                case FFLASHVARIABLEHEIGHT: returncode = scr_FlashVariableHeight( pstate, pself ); break;
                case FSETDAMAGETIME: returncode = scr_set_DamageTime( pstate, pself ); break;
                case FIFSTATEIS8: returncode = scr_StateIs8( pstate, pself ); break;
                case FIFSTATEIS9: returncode = scr_StateIs9( pstate, pself ); break;
                case FIFSTATEIS10: returncode = scr_StateIs10( pstate, pself ); break;
                case FIFSTATEIS11: returncode = scr_StateIs11( pstate, pself ); break;
                case FIFSTATEIS12: returncode = scr_StateIs12( pstate, pself ); break;
                case FIFSTATEIS13: returncode = scr_StateIs13( pstate, pself ); break;
                case FIFSTATEIS14: returncode = scr_StateIs14( pstate, pself ); break;
                case FIFSTATEIS15: returncode = scr_StateIs15( pstate, pself ); break;
                case FIFTARGETISAMOUNT: returncode = scr_TargetIsAMount( pstate, pself ); break;
                case FIFTARGETISAPLATFORM: returncode = scr_TargetIsAPlatform( pstate, pself ); break;
                case FADDSTAT: returncode = scr_AddStat( pstate, pself ); break;
                case FDISENCHANTTARGET: returncode = scr_DisenchantTarget( pstate, pself ); break;
                case FDISENCHANTALL: returncode = scr_DisenchantAll( pstate, pself ); break;
                case FSETVOLUMENEARESTTEAMMATE: returncode = scr_set_VolumeNearestTeammate( pstate, pself ); break;
                case FADDSHOPPASSAGE: returncode = scr_AddShopPassage( pstate, pself ); break;
                case FTARGETPAYFORARMOR: returncode = scr_TargetPayForArmor( pstate, pself ); break;
                case FJOINEVILTEAM: returncode = scr_JoinEvilTeam( pstate, pself ); break;
                case FJOINNULLTEAM: returncode = scr_JoinNullTeam( pstate, pself ); break;
                case FJOINGOODTEAM: returncode = scr_JoinGoodTeam( pstate, pself ); break;
                case FPITSKILL: returncode = scr_PitsKill( pstate, pself ); break;
                case FSETTARGETTOPASSAGEID: returncode = scr_set_TargetToPassageID( pstate, pself ); break;
                case FMAKENAMEUNKNOWN: returncode = scr_MakeNameUnknown( pstate, pself ); break;
                case FSPAWNEXACTPARTICLEENDSPAWN: returncode = scr_SpawnExactParticleEndSpawn( pstate, pself ); break;
                case FSPAWNPOOFSPEEDSPACINGDAMAGE: returncode = scr_SpawnPoofSpeedSpacingDamage( pstate, pself ); break;
                case FGIVEEXPERIENCETOGOODTEAM: returncode = scr_GiveExperienceToGoodTeam( pstate, pself ); break;
                case FDONOTHING: returncode = scr_DoNothing( pstate, pself ); break;
                case FGROGTARGET: returncode = scr_GrogTarget( pstate, pself ); break;
                case FDAZETARGET: returncode = scr_DazeTarget( pstate, pself ); break;
                case FENABLERESPAWN: returncode = scr_EnableRespawn( pstate, pself ); break;
                case FDISABLERESPAWN: returncode = scr_DisableRespawn( pstate, pself ); break;
                case FDISPELTARGETENCHANTID: returncode = scr_DispelTargetEnchantID( pstate, pself ); break;
                case FIFHOLDERBLOCKED: returncode = scr_HolderBlocked( pstate, pself ); break;
                    // case FGETSKILLLEVEL: returncode = scr_get_SkillLevel( pstate, pself ); break;
                case FIFTARGETHASNOTFULLMANA: returncode = scr_TargetHasNotFullMana( pstate, pself ); break;
                case FENABLELISTENSKILL: returncode = scr_EnableListenSkill( pstate, pself ); break;
                case FSETTARGETTOLASTITEMUSED: returncode = scr_set_TargetToLastItemUsed( pstate, pself ); break;
                case FFOLLOWLINK: returncode = scr_FollowLink( pstate, pself ); break;
                case FIFOPERATORISLINUX: returncode = scr_OperatorIsLinux( pstate, pself ); break;
                case FIFTARGETISAWEAPON: returncode = scr_TargetIsAWeapon( pstate, pself ); break;
                case FIFSOMEONEISSTEALING: returncode = scr_SomeoneIsStealing( pstate, pself ); break;
                case FIFTARGETISASPELL: returncode = scr_TargetIsASpell( pstate, pself ); break;
                case FIFBACKSTABBED: returncode = scr_Backstabbed( pstate, pself ); break;
                case FGETTARGETDAMAGETYPE: returncode = scr_get_TargetDamageType( pstate, pself ); break;
                case FADDQUEST: returncode = scr_AddQuest( pstate, pself ); break;
                case FBEATQUESTALLPLAYERS: returncode = scr_BeatQuestAllPlayers( pstate, pself ); break;
                case FIFTARGETHASQUEST: returncode = scr_TargetHasQuest( pstate, pself ); break;
                case FSETQUESTLEVEL: returncode = scr_set_QuestLevel( pstate, pself ); break;
                case FADDQUESTALLPLAYERS: returncode = scr_AddQuestAllPlayers( pstate, pself ); break;
                case FADDBLIPALLENEMIES: returncode = scr_AddBlipAllEnemies( pstate, pself ); break;
                case FPITSFALL: returncode = scr_PitsFall( pstate, pself ); break;
                case FIFTARGETISOWNER: returncode = scr_TargetIsOwner( pstate, pself ); break;
                case FEND: returncode = scr_End( pstate, pself ); break;

                case FSETSPEECH:           returncode = scr_set_Speech( pstate, pself );           break;
                case FSETMOVESPEECH:       returncode = scr_set_MoveSpeech( pstate, pself );       break;
                case FSETSECONDMOVESPEECH: returncode = scr_set_SecondMoveSpeech( pstate, pself ); break;
                case FSETATTACKSPEECH:     returncode = scr_set_AttackSpeech( pstate, pself );     break;
                case FSETASSISTSPEECH:     returncode = scr_set_AssistSpeech( pstate, pself );     break;
                case FSETTERRAINSPEECH:    returncode = scr_set_TerrainSpeech( pstate, pself );    break;
                case FSETSELECTSPEECH:     returncode = scr_set_SelectSpeech( pstate, pself );     break;

                case FTAKEPICTURE:           returncode = scr_TakePicture( pstate, pself );         break;
                case FIFOPERATORISMACINTOSH: returncode = scr_OperatorIsMacintosh( pstate, pself ); break;
                case FIFMODULEHASIDSZ:       returncode = scr_ModuleHasIDSZ( pstate, pself );       break;
                case FMORPHTOTARGET:         returncode = scr_MorphToTarget( pstate, pself );       break;
                case FGIVEMANAFLOWTOTARGET:  returncode = scr_GiveManaFlowToTarget( pstate, pself ); break;
                case FGIVEMANARETURNTOTARGET:returncode = scr_GiveManaReturnToTarget( pstate, pself ); break;
                case FSETMONEY:              returncode = scr_set_Money( pstate, pself );           break;
                case FIFTARGETCANSEEKURSES:  returncode = scr_TargetCanSeeKurses( pstate, pself );  break;
                case FSPAWNATTACHEDCHARACTER:returncode = scr_SpawnAttachedCharacter( pstate, pself ); break;
                case FKURSETARGET:           returncode = scr_KurseTarget( pstate, pself );            break;
                case FSETCHILDCONTENT:       returncode = scr_set_ChildContent( pstate, pself );    break;
                case FSETTARGETTOCHILD:      returncode = scr_set_TargetToChild( pstate, pself );   break;
                case FSETDAMAGETHRESHOLD:     returncode = scr_set_DamageThreshold( pstate, pself );   break;
                case FACCELERATETARGETUP:    returncode = scr_AccelerateTargetUp( pstate, pself ); break;
                case FSETTARGETAMMO:         returncode = scr_set_TargetAmmo( pstate, pself ); break;
                case FENABLEINVICTUS:        returncode = scr_EnableInvictus( pstate, pself ); break;
                case FDISABLEINVICTUS:       returncode = scr_DisableInvictus( pstate, pself ); break;
                case FTARGETDAMAGESELF:      returncode = scr_TargetDamageSelf( pstate, pself ); break;
                case FSETTARGETSIZE:         returncode = scr_SetTargetSize( pstate, pself ); break;
                case FIFTARGETISFACINGSELF:  returncode = scr_TargetIsFacingSelf( pstate, pself ); break;
                case FDRAWBILLBOARD:         returncode = scr_DrawBillboard( pstate, pself ); break;
                case FSETTARGETTOFIRSTBLAHINPASSAGE: returncode = scr_set_TargetToBlahInPassage( pstate, pself ); break;

                case FIFLEVELUP:            returncode = scr_LevelUp( pstate, pself ); break;
                case FGIVESKILLTOTARGET:    returncode = scr_GiveSkillToTarget( pstate, pself ); break;

                    // if none of the above, skip the line and log an error
                default:
                    log_message( "SCRIPT ERROR: scr_run_function() - ai script %d - unhandled script function %d\n", pself->type, valuecode );
                    returncode = bfalse;
                    break;
            }

        }
        PROFILE_END2( script_function );

        _script_function_calls[valuecode] += 1;
        _script_function_times[valuecode] += clktime_script_function;
    }

    return returncode;
}

//--------------------------------------------------------------------------------------------
void scr_set_operand( script_state_t * pstate, Uint8 variable )
{
    /// @details ZZ@> This function sets one of the tmp* values for scripted AI
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
            log_warning( "scr_set_operand() - cannot assign a number to index %d", variable );
            break;
    }
}

//--------------------------------------------------------------------------------------------
void scr_run_operand( script_state_t * pstate, ai_state_t * pself )
{
    /// @details ZZ@> This function does the scripted arithmetic in OPERATOR, OPERAND pairs

    const char * varname, * op;

    STRING buffer = EMPTY_CSTR;
    Uint8  variable;
    Uint8  operation;

    Uint32 iTmp;

    chr_t * pchr = NULL, * ptarget = NULL, * powner = NULL;

    if ( !DEFINED_CHR( pself->index ) ) return;
    pchr = ChrList.lst + pself->index;

    if ( DEFINED_CHR( pself->target ) )
    {
        ptarget = ChrList.lst + pself->target;
    }

    if ( DEFINED_CHR( pself->owner ) )
    {
        powner = ChrList.lst + pself->owner;
    }

    // get the operator
    iTmp      = 0;
    varname   = buffer;
    operation = GET_DATA_BITS( pself->opcode );
    if ( HAS_SOME_BITS( pself->opcode, FUNCTION_BIT ) )
    {
        // Get the working opcode from a constant, constants are all but high 5 bits
        iTmp = pself->opcode & VALUE_BITS;
        if ( debug_scripts ) snprintf( buffer, SDL_arraysize( buffer ), "%d", iTmp );
    }
    else
    {
        // Get the variable opcode from a register
        variable = pself->opcode & VALUE_BITS;

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
                iTmp = RANDIE;
                break;

            case VARSELFX:
                varname = "SELFX";
                iTmp = pchr->pos.x;
                break;

            case VARSELFY:
                varname = "SELFY";
                iTmp = pchr->pos.y;
                break;

            case VARSELFTURN:
                varname = "SELFTURN";
                iTmp = pchr->ori.facing_z;
                break;

            case VARSELFCOUNTER:
                varname = "SELFCOUNTER";
                iTmp = pself->order_counter;
                break;

            case VARSELFORDER:
                varname = "SELFORDER";
                iTmp = pself->order_value;
                break;

            case VARSELFMORALE:
                varname = "SELFMORALE";
                iTmp = TeamStack.lst[pchr->baseteam].morale;
                break;

            case VARSELFLIFE:
                varname = "SELFLIFE";
                iTmp = pchr->life;
                break;

            case VARTARGETX:
                varname = "TARGETX";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->pos.x;
                break;

            case VARTARGETY:
                varname = "TARGETY";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->pos.y;
                break;

            case VARTARGETDISTANCE:
                varname = "TARGETDISTANCE";
                if ( NULL == ptarget )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = ABS( ptarget->pos.x - pchr->pos.x ) + ABS( ptarget->pos.y - pchr->pos.y );
                }
                break;

            case VARTARGETTURN:
                varname = "TARGETTURN";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->ori.facing_z;
                break;

            case VARLEADERX:
                varname = "LEADERX";
                iTmp = pchr->pos.x;
                if ( TeamStack.lst[pchr->team].leader != NOLEADER )
                    iTmp = team_get_pleader( pchr->team )->pos.x;

                break;

            case VARLEADERY:
                varname = "LEADERY";
                iTmp = pchr->pos.y;
                if ( TeamStack.lst[pchr->team].leader != NOLEADER )
                    iTmp = team_get_pleader( pchr->team )->pos.y;

                break;

            case VARLEADERDISTANCE:
                {
                    chr_t * pleader;
                    varname = "LEADERDISTANCE";

                    pleader = team_get_pleader( pchr->team );

                    if ( NULL == pleader )
                    {
                        iTmp = 0x7FFFFFFF;
                    }
                    else
                    {
                        iTmp = ABS( pleader->pos.x - pchr->pos.x ) + ABS( pleader->pos.y - pchr->pos.y );
                    }
                }
                break;

            case VARLEADERTURN:
                varname = "LEADERTURN";
                iTmp = pchr->ori.facing_z;
                if ( TeamStack.lst[pchr->team].leader != NOLEADER )
                    iTmp = team_get_pleader( pchr->team )->ori.facing_z;

                break;

            case VARGOTOX:
                varname = "GOTOX";

                ai_state_ensure_wp( pself );

                if ( !pself->wp_valid )
                {
                    iTmp = pchr->pos.x;
                }
                else
                {
                    iTmp = pself->wp[kX];
                }
                break;

            case VARGOTOY:
                varname = "GOTOY";

                ai_state_ensure_wp( pself );

                if ( !pself->wp_valid )
                {
                    iTmp = pchr->pos.y;
                }
                else
                {
                    iTmp = pself->wp[kY];
                }
                break;

            case VARGOTODISTANCE:
                varname = "GOTODISTANCE";

                ai_state_ensure_wp( pself );

                if ( !pself->wp_valid )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = ABS( pself->wp[kX] - pchr->pos.x ) +
                           ABS( pself->wp[kY] - pchr->pos.y );
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
                    iTmp = vec_to_facing( ptarget->pos.x - pchr->pos.x , ptarget->pos.y - pchr->pos.y );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARPASSAGE:
                varname = "PASSAGE";
                iTmp = pself->passage;
                break;

            case VARWEIGHT:
                varname = "WEIGHT";
                iTmp = pchr->holdingweight;
                break;

            case VARSELFALTITUDE:
                varname = "SELFALTITUDE";
                iTmp = pchr->pos.z - pchr->enviro.floor_level;
                break;

            case VARSELFID:
                varname = "SELFID";
                iTmp = chr_get_idsz( pself->index, IDSZ_TYPE );
                break;

            case VARSELFHATEID:
                varname = "SELFHATEID";
                iTmp = chr_get_idsz( pself->index, IDSZ_HATE );
                break;

            case VARSELFMANA:
                varname = "SELFMANA";
                iTmp = pchr->mana;
                if ( pchr->canchannel )  iTmp += pchr->life;

                break;

            case VARTARGETSTR:
                varname = "TARGETSTR";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->strength;
                break;

            case VARTARGETWIS:
                varname = "TARGETWIS";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->wisdom;
                break;

            case VARTARGETINT:
                varname = "TARGETINT";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->intelligence;
                break;

            case VARTARGETDEX:
                varname = "TARGETDEX";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->dexterity;
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
                iTmp = ( NULL == ptarget ) ? 0 : ABS( ptarget->vel.x );
                break;

            case VARTARGETSPEEDY:
                varname = "TARGETSPEEDY";
                iTmp = ( NULL == ptarget ) ? 0 : ABS( ptarget->vel.y );
                break;

            case VARTARGETSPEEDZ:
                varname = "TARGETSPEEDZ";
                iTmp = ( NULL == ptarget ) ? 0 : ABS( ptarget->vel.z );
                break;

            case VARSELFSPAWNX:
                varname = "SELFSPAWNX";
                iTmp = pchr->pos_stt.x;
                break;

            case VARSELFSPAWNY:
                varname = "SELFSPAWNY";
                iTmp = pchr->pos_stt.y;
                break;

            case VARSELFSTATE:
                varname = "SELFSTATE";
                iTmp = pself->state;
                break;

            case VARSELFCONTENT:
                varname = "SELFCONTENT";
                iTmp = pself->content;
                break;

            case VARSELFSTR:
                varname = "SELFSTR";
                iTmp = pchr->strength;
                break;

            case VARSELFWIS:
                varname = "SELFWIS";
                iTmp = pchr->wisdom;
                break;

            case VARSELFINT:
                varname = "SELFINT";
                iTmp = pchr->intelligence;
                break;

            case VARSELFDEX:
                varname = "SELFDEX";
                iTmp = pchr->dexterity;
                break;

            case VARSELFMANAFLOW:
                varname = "SELFMANAFLOW";
                iTmp = pchr->manaflow;
                break;

            case VARTARGETMANAFLOW:
                varname = "TARGETMANAFLOW";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->manaflow;
                break;

            case VARSELFATTACHED:
                varname = "SELFATTACHED";
                iTmp = number_of_attached_particles( pself->index );
                break;

            case VARSWINGTURN:
                varname = "SWINGTURN";
                iTmp = PCamera->swing << 2;
                break;

            case VARXYDISTANCE:
                varname = "XYDISTANCE";
                iTmp = SQRT( pstate->x * pstate->x + pstate->y * pstate->y );
                break;

            case VARSELFZ:
                varname = "SELFZ";
                iTmp = pchr->pos.z;
                break;

            case VARTARGETALTITUDE:
                varname = "TARGETALTITUDE";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->pos.z - ptarget->enviro.floor_level;
                break;

            case VARTARGETZ:
                varname = "TARGETZ";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->pos.z;
                break;

            case VARSELFINDEX:
                varname = "SELFINDEX";
                iTmp = REF_TO_INT( pself->index );
                break;

            case VAROWNERX:
                varname = "OWNERX";
                iTmp = ( NULL == powner ) ? 0 : powner->pos.x;
                break;

            case VAROWNERY:
                varname = "OWNERY";
                iTmp = ( NULL == powner ) ? 0 : powner->pos.y;
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
                    iTmp = ABS( powner->pos.x - pchr->pos.x ) + ABS( powner->pos.y - pchr->pos.y );
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
                    iTmp = vec_to_facing( powner->pos.x - pchr->pos.x , powner->pos.y - pchr->pos.y );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARXYTURNTO:
                varname = "XYTURNTO";
                iTmp = vec_to_facing( pstate->x - pchr->pos.x , pstate->y - pchr->pos.y );
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
                    iTmp = vec_to_facing( ptarget->pos.x - pchr->pos.x , ptarget->pos.y - pchr->pos.y );
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
                iTmp = ABS( pchr->pos_stt.x - pchr->pos.x ) + ABS( pchr->pos_stt.y - pchr->pos.y );
                break;

            case VARTARGETMAXLIFE:
                varname = "TARGETMAXLIFE";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->lifemax;
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
                iTmp = cfg.difficulty;
                break;

            case VARTIMEHOURS:
                varname = "TIMEHOURS";
                iTmp = getCurrentTime()->tm_hour;
                break;

            case VARTIMEMINUTES:
                varname = "TIMEMINUTES";
                iTmp = getCurrentTime()->tm_min;
                break;

            case VARTIMESECONDS:
                varname = "TIMESECONDS";
                iTmp = getCurrentTime()->tm_sec;
                break;

            case VARDATEMONTH:
                varname = "DATEMONTH";
                iTmp = getCurrentTime()->tm_mon + 1;
                break;

            case VARDATEDAY:
                varname = "DATEDAY";
                iTmp = getCurrentTime()->tm_mday;
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

    if ( debug_scripts )
    {
        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;
        fprintf( scr_file, "%s %s(%d) ", op, varname, iTmp );
    }
}

bool_t scr_increment_exe( ai_state_t * pself )
{
    if ( NULL == pself ) return bfalse;
    if ( pself->exe_pos < pself->exe_stt || pself->exe_pos >= pself->exe_end ) return bfalse;

    pself->exe_pos++;
    pself->opcode = AisCompiled_buffer[pself->exe_pos];

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t scr_set_exe( ai_state_t * pself, size_t offset )
{
    if ( NULL == pself ) return bfalse;
    if ( offset < pself->exe_stt || offset >= pself->exe_end ) return bfalse;

    pself->exe_pos = offset;
    pself->opcode  = AisCompiled_buffer[pself->exe_pos];

    return btrue;
}

//--------------------------------------------------------------------------------------------
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
bool_t ai_state_get_wp( ai_state_t * pself )
{
    // try to load up the top waypoint

    if ( NULL == pself || !INGAME_CHR( pself->index ) ) return bfalse;

    pself->wp_valid = waypoint_list_peek( &( pself->wp_lst ), pself->wp );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ai_state_ensure_wp( ai_state_t * pself )
{
    // is the current waypoint is not valid, try to load up the top waypoint

    if ( NULL == pself || !INGAME_CHR( pself->index ) ) return bfalse;

    if ( pself->wp_valid ) return btrue;

    return ai_state_get_wp( pself );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void set_alerts( const CHR_REF character )
{
    /// @details ZZ@> This function polls some alert conditions

    chr_t      * pchr;
    ai_state_t * pai;
    bool_t at_waypoint;

    // invalid characters do not think
    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;
    pai  = chr_get_pai( character );

    if ( waypoint_list_empty( &( pai->wp_lst ) ) ) return;

    // let's let mounts get alert updates...
    // imagine a mount, like a racecar, that needs to make sure that it follows X
    // waypoints around a track or something

    // mounts do not get alerts
    // if ( INGAME_CHR(pchr->attachedto) ) return;

    // is the current waypoint is not valid, try to load up the top waypoint
    ai_state_ensure_wp( pai );

    at_waypoint = bfalse;
    if ( pai->wp_valid )
    {
        at_waypoint = ( ABS( pchr->pos.x - pai->wp[kX] ) < WAYTHRESH ) &&
                      ( ABS( pchr->pos.y - pai->wp[kY] ) < WAYTHRESH );
    }

    if ( at_waypoint )
    {
        SET_BIT( pai->alert, ALERTIF_ATWAYPOINT );

        if ( waypoint_list_finished( &( pai->wp_lst ) ) )
        {
            // we are now at the last waypoint
            // if the object can be alerted to last waypoint, do it
            // this test needs to be done because the ALERTIF_ATLASTWAYPOINT
            // doubles for "at last waypoint" and "not put away"
            if ( !chr_get_pcap( character )->isequipment )
            {
                SET_BIT( pai->alert, ALERTIF_ATLASTWAYPOINT );
            }

            // !!!!restart the waypoint list, do not clear them!!!!
            waypoint_list_reset( &( pai->wp_lst ) );

            // load the top waypoint
            ai_state_get_wp( pai );
        }
        else if ( waypoint_list_advance( &( pai->wp_lst ) ) )
        {
            // load the top waypoint
            ai_state_get_wp( pai );
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_order( const CHR_REF character, Uint32 value )
{
    /// @details ZZ@> This function issues an value for help to all teammates

    CHR_REF cnt;
    int     counter;

    for ( cnt = 0, counter = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !INGAME_CHR( cnt ) ) continue;

        if ( chr_get_iteam( cnt ) == chr_get_iteam( character ) )
        {
            ai_add_order( chr_get_pai( cnt ), value, counter );
            counter++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_special_order( Uint32 value, IDSZ idsz )
{
    /// @details ZZ@> This function issues an order to all characters with the a matching special IDSZ

    CHR_REF cnt;
    int     counter;

    for ( cnt = 0, counter = 0; cnt < MAX_CHR; cnt++ )
    {
        cap_t * pcap;

        if ( !INGAME_CHR( cnt ) ) continue;

        pcap = chr_get_pcap( cnt );
        if ( NULL == pcap ) continue;

        if ( idsz == pcap->idsz[IDSZ_SPECIAL] )
        {
            ai_add_order( chr_get_pai( cnt ), value, counter );
            counter++;
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ai_state_free( ai_state_t * pself )
{
    if ( NULL == pself ) return bfalse;

    // free any allocated data
    PROFILE_FREE_STRUCT( pself );

    return btrue;
};

//--------------------------------------------------------------------------------------------
ai_state_t * ai_state_reconstruct( ai_state_t * pself )
{
    if ( NULL == pself ) return pself;

    // deallocate any existing data
    ai_state_free( pself );

    // set everything to safe values
    memset( pself, 0, sizeof( *pself ) );

    pself->index      = ( CHR_REF )MAX_CHR;
    pself->target     = ( CHR_REF )MAX_CHR;
    pself->owner      = ( CHR_REF )MAX_CHR;
    pself->child      = ( CHR_REF )MAX_CHR;
    pself->target_old = ( CHR_REF )MAX_CHR;
    pself->poof_time  = -1;

    pself->bumplast   = ( CHR_REF )MAX_CHR;
    pself->attacklast = ( CHR_REF )MAX_CHR;
    pself->hitlast    = ( CHR_REF )MAX_CHR;

    return pself;
}

//--------------------------------------------------------------------------------------------
ai_state_t * ai_state_ctor( ai_state_t * pself )
{
    if ( NULL == ai_state_reconstruct( pself ) ) return NULL;

    PROFILE_INIT_STRUCT( ai, pself );

    return pself;
}

//--------------------------------------------------------------------------------------------
ai_state_t * ai_state_dtor( ai_state_t * pself )
{
    if ( NULL == pself ) return pself;

    // initialize the object
    ai_state_reconstruct( pself );

    return pself;
}

