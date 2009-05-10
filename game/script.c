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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - script.c
* Implements the game's scripting language.
*/

#include "script.h"
#include "script_compile.h"

#include "log.h"
#include "link.h"
#include "camera.h"
#include "passage.h"
#include "enchant.h"
#include "char.h"
#include "graphic.h"
#include "input.h"
#include "char.h"
#include "particle.h"
#include "network.h"

#include "egoboo.h"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static char * script_error_classname = "UNKNOWN";
static Uint16 script_error_model     = (Uint16)(~0);
static char * script_error_name      = "UNKNOWN";
static Uint16 script_error_index     = (Uint16)(~0);

static bool_t script_increment_exe( ai_state_t * pself );
static bool_t script_set_exe( ai_state_t * pself, size_t offset );

//static Uint8 run_function( script_state_t * pstate, ai_state_t * pself );
static Uint8 run_function_2( script_state_t * pstate, ai_state_t * pself );
static void  set_operand( script_state_t * pstate, Uint8 variable );
static void  run_operand( script_state_t * pstate, ai_state_t * pself );

static bool_t run_operation( script_state_t * pstate, ai_state_t * pself );
static bool_t run_function_call( script_state_t * pstate, ai_state_t * pself );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void let_character_think( Uint16 character )
{
    // ZZ> This function lets one character do AI stuff

    script_state_t   my_state;
    chr_t          * pchr;
    ai_state_t     * pself;

    if ( character >= MAXCHR || !ChrList[character].on )  return;
    pchr  = ChrList + character;
    pself = &(pchr->ai);

    // has the time for this character to die come and gone?
    if ( pself->poof_time >= 0 && pself->poof_time <= frame_wld ) return;

    // characters that are not "alive" should have greatly limited access to scripting...
    // in the past it was completely turned off
	//if ( !pchr->alive ) return;

    // debug a certain script
    //debug_scripts = ( pself->index == 385 && ChrList[pself->index].model == 76 );

    // target_old is set to the target every time the script is run
    pself->target_old = pself->target;

    // Make life easier
    script_error_classname = "UNKNOWN";
    script_error_model     = pchr->model;
    script_error_index     = (Uint16)(~0);
    script_error_name      = "UNKNOWN";
    if ( script_error_model < MAXMODEL )
    {
        script_error_classname = CapList[ script_error_model ].classname;

        script_error_index = MadList[script_error_model].ai;
        if ( script_error_index < MAXAI )
        {
            script_error_name = szAisName[script_error_index];
        }
    }
    
	if ( debug_scripts )
    {
        FILE * scr_file = (NULL == debug_script_file) ? stdout : debug_script_file;

        fprintf( scr_file,  "\n\n--------\n%d - %s\n", script_error_index, script_error_name );
        fprintf( scr_file,  "%d - %s\n", script_error_model, script_error_classname );

        // who are we related to?
        fprintf( scr_file,  "\tindex  == %d\n", pself->index );
        fprintf( scr_file,  "\ttarget == %d\n", pself->target );
        fprintf( scr_file,  "\towner  == %d\n", pself->owner );
        fprintf( scr_file,  "\tchild  == %d\n", pself->child );

        // some local storage
        fprintf( scr_file,  "\talert     == %x\n", pself->alert   );
        fprintf( scr_file,  "\tstate     == %d\n", pself->state   );
        fprintf( scr_file,  "\tcontent   == %d\n", pself->content );
        fprintf( scr_file,  "\ttimer     == %d\n", pself->timer   );
        fprintf( scr_file,  "\tframe_wld == %d\n", frame_wld      );

        // ai memory from the last event
        fprintf( scr_file,  "\tbumplast       == %d\n", pself->bumplast );
        fprintf( scr_file,  "\tattacklast     == %d\n", pself->attacklast );
        fprintf( scr_file,  "\thitlast        == %d\n", pself->hitlast );
        fprintf( scr_file,  "\tdirectionlast  == %d\n", pself->directionlast );
        fprintf( scr_file,  "\tdamagetypelast == %d\n", pself->damagetypelast );
        fprintf( scr_file,  "\tlastitemused   == %d\n", pself->lastitemused );
        fprintf( scr_file,  "\ttarget_old     == %d\n", pself->target_old );

        // message handling
        fprintf( scr_file,  "\torder == %d\n", pself->order );
        fprintf( scr_file,  "\tcounter == %d\n", pself->rank );

        // waypoints
        fprintf( scr_file,  "\twp_tail == %d\n", pself->wp_tail );
        fprintf( scr_file,  "\twp_head == %d\n\n", pself->wp_head );
    }

    // Clear the button latches
    if ( !ChrList[pself->index].isplayer )
    {
        ChrList[pself->index].latchbutton = 0;
    }

    // Reset the target if it can't be seen
    if ( !ChrList[pself->index].canseeinvisible && ChrList[pself->index].alive )
    {
        if ( ChrList[pself->target].inst.alpha <= INVISIBLE || ChrList[pself->target].inst.light <= INVISIBLE )
        {
            pself->target = pself->index;
        }
    }

    // reset the script state
    memset( &my_state, 0, sizeof(script_state_t) );

    // reset the ai
    pself->terminate = bfalse;
    pself->changed   = bfalse;
    pself->indent    = 0;
    pself->exe_stt   = iAisStartPosition[pself->type];
    pself->exe_end   = iAisEndPosition[pself->type];

    // Run the AI Script
    script_set_exe( pself, pself->exe_stt );
    while ( !pself->terminate && pself->exe_pos < pself->exe_end )
    {
        // This is used by the Else function
        // it only keeps track of functions
        pself->indent_last = pself->indent;
        pself->indent = GET_DATA_BITS( pself->opcode );

        // Was it a function
        if ( 0 != ( pself->opcode & FUNCTION_BIT ) )
        {
            if ( !run_function_call( &my_state, pself ) )
            {
                break;
            }
        }
        else
        {
            if ( !run_operation( &my_state, pself ) )
            {
                break;
            }
        }
    }

    // Set latches
    if ( !ChrList[pself->index].isplayer )
    {
        float latch2;
        if ( ChrList[pself->index].ismount && MAXCHR != ChrList[pself->index].holdingwhich[0] && ChrList[ChrList[pself->index].holdingwhich[0]].on )
        {
            // Mount
            ChrList[pself->index].latchx = ChrList[ChrList[pself->index].holdingwhich[0]].latchx;
            ChrList[pself->index].latchy = ChrList[ChrList[pself->index].holdingwhich[0]].latchy;
        }
        else if ( pself->wp_tail != pself->wp_head )
        {
            // Normal AI
            ChrList[pself->index].latchx = ( pself->wp_pos_x[pself->wp_tail] - ChrList[pself->index].xpos ) / (128 << 2);
            ChrList[pself->index].latchy = ( pself->wp_pos_y[pself->wp_tail] - ChrList[pself->index].ypos ) / (128 << 2);
        }
        else
        {
            // AI, but no valid waypoints
            ChrList[pself->index].latchx = 0;
            ChrList[pself->index].latchy = 0;
        }

        latch2 = ChrList[pself->index].latchx * ChrList[pself->index].latchx + ChrList[pself->index].latchy * ChrList[pself->index].latchy;
        if (latch2 > 1.0f)
        {
            latch2 = 1.0f / sqrt(latch2);
            ChrList[pself->index].latchx *= latch2;
            ChrList[pself->index].latchy *= latch2;
        }
    }

    // Clear alerts for next time around
    pself->alert = 0;
    if ( pself->changed )
    {
        pself->alert  |= ALERTIF_CHANGED;
        pself->changed = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
void set_alerts( Uint16 character )
{
    // ZZ> This function polls some alert conditions

    // invalid characters do not think
    if ( !ChrList[character].on ) return;

    // mounts do not get to think for themselves
    if ( MAXCHR != ChrList[character].attachedto ) return;
    if ( ChrList[character].ai.wp_tail != ChrList[character].ai.wp_head )
    {
        if ( ChrList[character].xpos < ChrList[character].ai.wp_pos_x[ChrList[character].ai.wp_tail] + WAYTHRESH &&
                ChrList[character].xpos > ChrList[character].ai.wp_pos_x[ChrList[character].ai.wp_tail] - WAYTHRESH &&
                ChrList[character].ypos < ChrList[character].ai.wp_pos_y[ChrList[character].ai.wp_tail] + WAYTHRESH &&
                ChrList[character].ypos > ChrList[character].ai.wp_pos_y[ChrList[character].ai.wp_tail] - WAYTHRESH )
        {
            ChrList[character].ai.alert |= ALERTIF_ATWAYPOINT;
            ChrList[character].ai.wp_tail++;
            if ( ChrList[character].ai.wp_tail > MAXWAY - 1 ) ChrList[character].ai.wp_tail = MAXWAY - 1;
        }
        if ( ChrList[character].ai.wp_tail >= ChrList[character].ai.wp_head )
        {
            // !!!!restart the waypoint list, do not clear them!!!!
            ChrList[character].ai.wp_tail    = 0;

            // if the object can be alerted to last waypoint, do it
            if ( !CapList[ChrList[character].model].isequipment )
            {
                ChrList[character].ai.alert |= ALERTIF_ATLASTWAYPOINT;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_order( Uint16 character, Uint32 order )
{
    // ZZ> This function issues an order for help to all teammates
    Uint8 team;
    Uint16 counter;
    Uint16 cnt;

    team = ChrList[character].team;
    counter = 0;
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( ChrList[cnt].team == team )
        {
            ChrList[cnt].ai.order = order;
            ChrList[cnt].ai.rank  = counter;
            ChrList[cnt].ai.alert |= ALERTIF_ORDERED;
            counter++;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void issue_special_order( Uint32 order, IDSZ idsz )
{
    // ZZ> This function issues an order to all characters with the a matching special IDSZ
    Uint16 counter;
    Uint16 cnt;

    counter = 0;
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( ChrList[cnt].on )
        {
            if ( CapList[ChrList[cnt].model].idsz[IDSZ_SPECIAL] == idsz )
            {
                ChrList[cnt].ai.order = order;
                ChrList[cnt].ai.rank  = counter;
                ChrList[cnt].ai.alert |= ALERTIF_ORDERED;
                counter++;
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t script_increment_exe( ai_state_t * pself )
{
    if ( NULL == pself ) return bfalse;
    if ( pself->exe_pos < pself->exe_stt || pself->exe_pos >= pself->exe_end ) return bfalse;

    pself->exe_pos++;
    pself->opcode = iCompiledAis[pself->exe_pos];

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t script_set_exe( ai_state_t * pself, size_t offset )
{
    if ( NULL == pself ) return bfalse;
    if ( offset < pself->exe_stt || offset >= pself->exe_end ) return bfalse;

    pself->exe_pos = offset;
    pself->opcode  = iCompiledAis[pself->exe_pos];

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t run_function_call( script_state_t * pstate, ai_state_t * pself )
{
    Uint8  functionreturn;

    // check for valid pointers
    if ( NULL == pstate || NULL == pself ) return bfalse;

    // check for valid execution pointer
    if ( pself->exe_pos < pself->exe_stt || pself->exe_pos >= pself->exe_end ) return bfalse;

    // Run the function
    functionreturn = run_function_2( pstate, pself );

    // move the execution pointer to the jump code
    script_increment_exe( pself );
    if ( functionreturn )
    {
        // move the execution pointer to the next opcode
        script_increment_exe( pself );
    }
    else
    {
        // use the jump code to jump to the right location
        size_t new_index = pself->opcode;

        // make sure the value is valid
        assert(new_index < AISMAXCOMPILESIZE && new_index >= pself->exe_stt && new_index <= pself->exe_end );

        // actually do the jump
        script_set_exe( pself, new_index );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t run_operation( script_state_t * pstate, ai_state_t * pself )
{
    char * variable;
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
        FILE * scr_file = (NULL == debug_script_file) ? stdout : debug_script_file;

        for (i = 0; i < pself->indent; i++) { fprintf( scr_file, "  " ); }

        for (i = 0; i < MAXCODE; i++)
        {
            if ( 'V' == cCodeType[i] && var_value == iCodeValue[i] )
            {
                variable = cCodeName[i];
                break;
            };
        }

        fprintf( scr_file, "%s = ", variable );
    }

    // Get the number of operands
    script_increment_exe( pself );
    operand_count = pself->opcode;

    // Now run the operation
    pstate->operationsum = 0;
    for ( i = 0; i < operand_count && pself->exe_pos < pself->exe_end; i++ )
    {
        script_increment_exe( pself );
        run_operand( pstate, pself );
    }
    if ( debug_scripts )
    {
        FILE * scr_file = (NULL == debug_script_file) ? stdout : debug_script_file;
        fprintf( scr_file, " == %d \n", pstate->operationsum );
    }

    // Save the results in the register that called the arithmetic
    set_operand( pstate, var_value );

    // go to the next opcode
    script_increment_exe( pself );

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint8 run_function_2( script_state_t * pstate, ai_state_t * pself )
{
    // BB> This is about half-way to what is needed for Lua integration

    // Mask out the indentation
    Uint32 valuecode = pself->opcode & VALUE_BITS;

    // Assume that the function will pass, as most do
    Uint8 returncode = btrue;
    if ( MAXCODE == valuecode )
    {
        log_message( "SCRIPT ERROR: run_function() - model == %d, class name == \"%s\" - Unknown opcode found!\n", script_error_model, script_error_classname );
        return bfalse;
    }

    // debug stuff
    if ( debug_scripts )
    {
        Uint32 i;
        FILE * scr_file = (NULL == debug_script_file) ? stdout : debug_script_file;

        for (i = 0; i < pself->indent; i++) { fprintf( scr_file,  "  " ); }

        for (i = 0; i < MAXCODE; i++)
        {
            if ( 'F' == cCodeType[i] && valuecode == iCodeValue[i] )
            {
                fprintf( scr_file,  "%s\n", cCodeName[i] );
                break;
            };
        }
    }

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
        case FIFHOLDERSCOREDAHIT: returncode = scr_HolderScoredAHit( pstate, pself ); break;
        case FIFHOLDERBLOCKED: returncode = scr_HolderBlocked( pstate, pself ); break;
            //case FGETSKILLLEVEL: returncode = scr_get_SkillLevel( pstate, pself ); break;
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
		case FIFMODULEHASIDSZ:		 returncode = scr_IfModuleHasIDSZ( pstate, pself );     break;

            // if none of the above, skip the line and log an error
        default:
            log_message( "SCRIPT ERROR: run_function() - ai script %d - unhandled script function %d\n", pself->type, valuecode );
            returncode = bfalse;
            break;
    }

    return returncode;
}
//--------------------------------------------------------------------------------------------
Uint8 run_function( script_state_t * pstate, ai_state_t * pself )
{
    // ZZ> This function runs a script function for the AI.
    //     It returns bfalse if the script should jump over the
    //     indented code that follows

    // Mask out the indentation
    Uint32 valuecode = pself->opcode & VALUE_BITS;

    // Assume that the function will pass, as most do
    Uint8 returncode = btrue;

    Uint16 sTmp;
    float fTmp;
    int iTmp, tTmp;
    int volume;
    IDSZ test;
    char cTmp[256];
    chr_t * pchr;

    if ( MAXCODE == valuecode )
    {
        log_message( "SCRIPT ERROR: run_function() - model == %d, class name == \"%s\" - Unknown opcode found!\n", script_error_model, script_error_classname );
        return bfalse;
    }

    if ( NULL == pself || pself->index >= MAXCHR || !ChrList[pself->index].on ) return bfalse;
    pchr = ChrList + pself->index;

    // debug stuff
    if ( debug_scripts )
    {
        Uint32 i;

        FILE * scr_file = (NULL == debug_script_file) ? stdout : debug_script_file;

        for (i = 0; i < pself->indent; i++) { fprintf( scr_file, "  " ); }

        for (i = 0; i < MAXCODE; i++)
        {
            if ( 'F' == cCodeType[i] && valuecode == iCodeValue[i] )
            {
                fprintf( scr_file, "%s\n", cCodeName[i] );
                break;
            };
        }
    }

    // Figure out which function to run
    switch ( valuecode )
    {
        case FIFSPAWNED:
            // Proceed only if it's a new character
            returncode = ( 0 != ( pself->alert & ALERTIF_SPAWNED ) );
            break;

        case FIFTIMEOUT:
            // Proceed only if time alert is set
            returncode = ( frame_wld > pself->timer );
            break;

        case FIFATWAYPOINT:
            // Proceed only if the character reached a waypoint
            returncode = ( 0 != ( pself->alert & ALERTIF_ATWAYPOINT ) );
            break;

        case FIFATLASTWAYPOINT:
            // Proceed only if the character reached its last waypoint
            returncode = ( 0 != ( pself->alert & ALERTIF_ATLASTWAYPOINT ) );
            break;

        case FIFATTACKED:
            // Proceed only if the character was damaged
            returncode = ( 0 != ( pself->alert & ALERTIF_ATTACKED ) );
            break;

        case FIFBUMPED:
            // Proceed only if the character was bumped
            returncode = ( 0 != ( pself->alert & ALERTIF_BUMPED ) );
            break;

        case FIFORDERED:
            // Proceed only if the character was ordered
            returncode = ( 0 != ( pself->alert & ALERTIF_ORDERED ) );
            break;

        case FIFCALLEDFORHELP:
            // Proceed only if the character was called for help
            returncode = ( 0 != ( pself->alert & ALERTIF_CALLEDFORHELP ) );
            break;

        case FSETCONTENT:
            // Set the content
            pself->content = pstate->argument;
            break;

        case FIFKILLED:
            // Proceed only if the character's been killed
            returncode = ( 0 != ( pself->alert & ALERTIF_KILLED ) );
            break;

        case FIFTARGETKILLED:
            // Proceed only if the character's target has just died
            returncode = ( 0 != ( pself->alert & ALERTIF_TARGETKILLED ) );
            break;

        case FCLEARWAYPOINTS:
            // Clear out all waypoints
            pself->wp_tail = 0;
            pself->wp_head = 0;
            break;

        case FADDWAYPOINT:
            // Add a waypoint to the waypoint list
            pself->wp_pos_x[pself->wp_head] = pstate->x;
            pself->wp_pos_y[pself->wp_head] = pstate->y;

            pself->wp_head++;
            if ( pself->wp_head > MAXWAY - 1 )  pself->wp_head = MAXWAY - 1;
            break;

        case FFINDPATH:
            // Yep this is it
            if ( ChrList[ pself->target ].model != pself->index )
            {
                if ( pstate->distance != MOVE_FOLLOW )
                {
                    pstate->x = ChrList[ ChrList[ ChrList[pself->target].model ].ai.target ].xpos;
                    pstate->y = ChrList[ ChrList[ ChrList[pself->target].model ].ai.target ].ypos;
                }
                else
                {
                    pstate->x = ( rand() & 1023 ) - 512 + ChrList[ ChrList[ ChrList[pself->target].model ].ai.target ].xpos;
                    pstate->y = ( rand() & 1023 ) - 512 + ChrList[ ChrList[ ChrList[pself->target].model ].ai.target ].ypos;
                }

                pstate->turn = ATAN2( ChrList[pself->target].ypos - pchr->ypos, ChrList[pself->target].xpos - pchr->xpos ) * 0xFFFF / ( TWO_PI );
                if ( pstate->distance == MOVE_RETREAT )
                {
                    pstate->turn += ( rand() & 16383 ) - 8192;
                }
                else
                {
                    pstate->turn += 32768;
                }
                pstate->turn &= 0xFFFF;
                if ( pstate->distance == MOVE_CHARGE || pstate->distance == MOVE_RETREAT )
                {
                    reset_character_accel( pself->index ); // Force 100% speed
                }

                // Secondly we run the Compass function (If we are not in follow mode)
                if ( pstate->distance != MOVE_FOLLOW )
                {
                    //sTmp = ( pstate->turn + 16384 );
                    pstate->x = pstate->x - turntocos[(pstate->turn & 0xFFFF)>>2] * pstate->distance;
                    pstate->y = pstate->y - turntosin[(pstate->turn & 0xFFFF)>>2] * pstate->distance;
                }

                // Then we add the waypoint(s), without clearing existing ones...
                pself->wp_pos_x[pself->wp_head] = pstate->x;
                pself->wp_pos_y[pself->wp_head] = pstate->y;

                pself->wp_head++;
                if ( pself->wp_head > MAXWAY - 1 ) pself->wp_head = MAXWAY - 1;

            }
            break;

        case FCOMPASS:
            // This function changes tmpx and tmpy in a circlular manner according
            // to tmpturn and tmpdistance
            //sTmp = ( pstate->turn + 16384 );
            pstate->x -= turntocos[(pstate->turn & 0xFFFF)>>2] * pstate->distance;
            pstate->y -= turntosin[(pstate->turn & 0xFFFF)>>2] * pstate->distance;
            break;

        case FGETTARGETARMORPRICE:
            // This function gets the armor cost for the given skin
            sTmp = pstate->argument % MAXSKIN;
            pstate->x = CapList[ChrList[pself->target].model].skincost[sTmp];
            break;

        case FSETTIME:
            // This function resets the time
            if ( pstate->argument > -1 )
            {
                pself->timer = frame_wld + pstate->argument;
            }
            break;

        case FGETCONTENT:
            // Get the content
            pstate->argument = pself->content;
            break;

        case FJOINTARGETTEAM:
            // This function allows the character to leave its own team and join another
            returncode = bfalse;
            if ( ChrList[pself->target].on )
            {
                switch_team( pself->index, ChrList[pself->target].team );
                returncode = btrue;
            }
            break;

        case FSETTARGETTONEARBYENEMY:
            // This function finds a nearby enemy, and proceeds only if there is one
            returncode = bfalse;
            if (get_target( pself->index, NEARBY, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse) != MAXCHR) returncode = btrue;

            break;

        case FSETTARGETTOTARGETLEFTHAND:
            // This function sets the target to the target's left item
            sTmp = ChrList[pself->target].holdingwhich[0];
            returncode = bfalse;
            if ( sTmp != MAXCHR )
            {
                pself->target = sTmp;
                returncode = btrue;
            }
            break;

        case FSETTARGETTOTARGETRIGHTHAND:
            // This function sets the target to the target's right item
            sTmp = ChrList[pself->target].holdingwhich[1];
            returncode = bfalse;
            if ( sTmp != MAXCHR )
            {
                pself->target = sTmp;
                returncode = btrue;
            }
            break;

        case FSETTARGETTOWHOEVERATTACKED:

            // This function sets the target to whoever attacked the character last,
            // failing for damage tiles
            if ( pself->attacklast != MAXCHR )
            {
                pself->target = pself->attacklast;
            }
            else
            {
                returncode = bfalse;
            }
            break;

        case FSETTARGETTOWHOEVERBUMPED:
            // This function sets the target to whoever bumped into the
            // character last.  It never fails
            pself->target = pself->bumplast;
            break;

        case FSETTARGETTOWHOEVERCALLEDFORHELP:
            // This function sets the target to whoever needs help
            pself->target = TeamList[pchr->team].sissy;
            break;

        case FSETTARGETTOOLDTARGET:
            // This function reverts to the target with whom the script started
            pself->target = pself->target_old;
            break;

        case FSETTURNMODETOVELOCITY:
            // This function sets the turn mode
            pchr->turnmode = TURNMODEVELOCITY;
            break;

        case FSETTURNMODETOWATCH:
            // This function sets the turn mode
            pchr->turnmode = TURNMODEWATCH;
            break;

        case FSETTURNMODETOSPIN:
            // This function sets the turn mode
            pchr->turnmode = TURNMODESPIN;
            break;

        case FSETBUMPHEIGHT:
            // This function changes a character's bump height
            pchr->bumpheight = pstate->argument * pchr->fat;
            pchr->bumpheightsave = pstate->argument;
            break;

        case FIFTARGETHASID:
            // This function proceeds if ID matches tmpargument
            sTmp = ChrList[pself->target].model;
            returncode = CapList[sTmp].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument;
            returncode = returncode | ( CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument );
            break;

        case FIFTARGETHASITEMID:
            // This function proceeds if the target has a matching item in his/her pack
            returncode = bfalse;
            // Check the pack
            sTmp = ChrList[pself->target].nextinpack;

            while ( sTmp != MAXCHR )
            {
                if ( CapList[ChrList[sTmp].model].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument || CapList[ChrList[sTmp].model].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
                {
                    returncode = btrue;
                    sTmp = MAXCHR;
                }
                else
                {
                    sTmp = ChrList[sTmp].nextinpack;
                }
            }

            // Check left hand
            sTmp = ChrList[pself->target].holdingwhich[0];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
                    returncode = btrue;
            }

            // Check right hand
            sTmp = ChrList[pself->target].holdingwhich[1];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
                    returncode = btrue;
            }
            break;

        case FIFTARGETHOLDINGITEMID:
            // This function proceeds if ID matches tmpargument and returns the latch for the
            // hand in tmpargument
            returncode = bfalse;
            // Check left hand
            sTmp = ChrList[pself->target].holdingwhich[0];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
                {
                    pstate->argument = LATCHBUTTON_LEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = ChrList[pself->target].holdingwhich[1];
            if ( sTmp != MAXCHR && !returncode )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
                {
                    pstate->argument = LATCHBUTTON_RIGHT;
                    returncode = btrue;
                }
            }
            break;

        case FIFTARGETHASSKILLID:
            // This function proceeds if ID matches tmpargument
            returncode = bfalse;

            returncode = (0 != check_skills( pself->target, ( IDSZ )pstate->argument ));

            break;

        case FELSE:
            // This function fails if the last function was more indented
            returncode = ( pself->indent >= pself->indent_last );
            break;

        case FRUN:
            reset_character_accel( pself->index );
            break;

        case FWALK:
            reset_character_accel( pself->index );
            pchr->maxaccel *= 0.66f;
            break;

        case FSNEAK:
            reset_character_accel( pself->index );
            pchr->maxaccel *= 0.33f;
            break;

        case FDOACTION:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid or if the character is doing
            // something else already
            returncode = bfalse;
            if ( pstate->argument < MAXACTION && pchr->actionready )
            {
                if ( MadList[pchr->inst.imad].actionvalid[pstate->argument] )
                {
                    pchr->action = pstate->argument;
                    pchr->inst.lip = 0;
                    pchr->inst.lastframe = pchr->inst.frame;
                    pchr->inst.frame = MadList[pchr->inst.imad].actionstart[pstate->argument];
                    pchr->actionready = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FKEEPACTION:
            // This function makes the current animation halt on the last frame
            pchr->keepaction = btrue;
            break;

        case FISSUEORDER:
            // This function issues an order to all teammates
            issue_order( pself->index, pstate->argument );
            break;

        case FDROPWEAPONS:
            // This funtion drops the character's in hand items/riders
            sTmp = pchr->holdingwhich[0];
            if ( sTmp != MAXCHR )
            {
                detach_character_from_mount( sTmp, btrue, btrue );
                if ( pchr->ismount )
                {
                    ChrList[sTmp].zvel = DISMOUNTZVEL;
                    ChrList[sTmp].zpos += DISMOUNTZVEL;
                    ChrList[sTmp].jumptime = JUMPDELAY;
                }
            }

            sTmp = pchr->holdingwhich[1];
            if ( sTmp != MAXCHR )
            {
                detach_character_from_mount( sTmp, btrue, btrue );
                if ( pchr->ismount )
                {
                    ChrList[sTmp].zvel = DISMOUNTZVEL;
                    ChrList[sTmp].zpos += DISMOUNTZVEL;
                    ChrList[sTmp].jumptime = JUMPDELAY;
                }
            }
            break;

        case FTARGETDOACTION:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid or if the target is doing
            // something else already
            returncode = bfalse;
            if ( ChrList[pself->target].alive )
            {
                if ( pstate->argument < MAXACTION && ChrList[pself->target].actionready )
                {
                    if ( MadList[ChrList[pself->target].inst.imad].actionvalid[pstate->argument] )
                    {
                        ChrList[pself->target].action = pstate->argument;
                        ChrList[pself->target].inst.lip = 0;
                        ChrList[pself->target].inst.lastframe = ChrList[pself->target].inst.frame;
                        ChrList[pself->target].inst.frame = MadList[ChrList[pself->target].inst.imad].actionstart[pstate->argument];
                        ChrList[pself->target].actionready = bfalse;
                        returncode = btrue;
                    }
                }
            }
            break;

        case FOPENPASSAGE:
            // This function opens the passage specified by tmpargument, failing if the
            // passage was already open
            returncode = open_passage( pstate->argument );
            break;

        case FCLOSEPASSAGE:
            // This function closes the passage specified by tmpargument, and proceeds
            // only if the passage is clear of obstructions
            returncode = close_passage( pstate->argument );
            break;

        case FIFPASSAGEOPEN:
            // This function proceeds only if the passage specified by tmpargument
            // is both valid and open
            returncode = bfalse;
            if ( pstate->argument < numpassage && pstate->argument >= 0 )
            {
                returncode = passopen[pstate->argument];
            }
            break;

        case FGOPOOF:
            // This function flags the character to be removed from the game
            returncode = bfalse;
            if ( !pchr->isplayer )
            {
                returncode = btrue;
                pself->poof_time = frame_wld;
            }
            break;

        case FCOSTTARGETITEMID:
            // This function checks if the target has a matching item, and poofs it
            returncode = bfalse;
            // Check the pack
            iTmp = MAXCHR;
            tTmp = pself->target;
            sTmp = ChrList[tTmp].nextinpack;

            while ( sTmp != MAXCHR )
            {
                if ( CapList[ChrList[sTmp].model].idsz[IDSZ_PARENT] == ( IDSZ) pstate->argument || CapList[ChrList[sTmp].model].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
                {
                    returncode = btrue;
                    iTmp = sTmp;
                    sTmp = MAXCHR;
                }
                else
                {
                    tTmp = sTmp;
                    sTmp = ChrList[sTmp].nextinpack;
                }
            }

            // Check left hand
            sTmp = ChrList[pself->target].holdingwhich[0];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
                {
                    returncode = btrue;
                    iTmp = ChrList[pself->target].holdingwhich[0];
                }
            }

            // Check right hand
            sTmp = ChrList[pself->target].holdingwhich[1];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
                {
                    returncode = btrue;
                    iTmp = ChrList[pself->target].holdingwhich[1];
                }
            }
            if ( returncode )
            {
                if ( ChrList[iTmp].ammo <= 1 )
                {
                    // Poof the item
                    if ( ChrList[iTmp].inpack )
                    {
                        // Remove from the pack
                        ChrList[tTmp].nextinpack = ChrList[iTmp].nextinpack;
                        ChrList[pself->target].numinpack--;
                        free_one_character( iTmp );
                    }
                    else
                    {
                        // Drop from hand
                        detach_character_from_mount( iTmp, btrue, bfalse );
                        free_one_character( iTmp );
                    }
                }
                else
                {
                    // Cost one ammo
                    ChrList[iTmp].ammo--;
                }
            }
            break;

        case FDOACTIONOVERRIDE:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid
            returncode = bfalse;
            if ( pstate->argument < MAXACTION )
            {
                if ( MadList[pchr->inst.imad].actionvalid[pstate->argument] )
                {
                    pchr->action = pstate->argument;
                    pchr->inst.lip = 0;
                    pchr->inst.lastframe = pchr->inst.frame;
                    pchr->inst.frame = MadList[pchr->inst.imad].actionstart[pstate->argument];
                    pchr->actionready = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FIFHEALED:
            // Proceed only if the character was healed
            returncode = ( 0 != ( pself->alert & ALERTIF_HEALED ) );
            break;

        case FSENDMESSAGE:
            // This function sends a message to the players
            display_message( pstate, MadList[pchr->inst.imad].msgstart + pstate->argument, pself->index );
            break;

        case FCALLFORHELP:
            // This function issues a call for help
            call_for_help( pself->index );
            break;

        case FADDIDSZ:
            // This function adds an idsz to the module's menu.txt file
            module_add_idsz( pickedmodule, pstate->argument );
            break;

        case FSETSTATE:
            // This function sets the character's state variable
            pself->state = pstate->argument;
            break;

        case FGETSTATE:
            // This function reads the character's state variable
            pstate->argument = pself->state;
            break;

        case FIFSTATEIS:
            // This function fails if the character's state is inequal to tmpargument
            returncode = ( pstate->argument == pself->state );
            break;

        case FIFTARGETCANOPENSTUFF:
            // This function fails if the target can't open stuff
            returncode = ChrList[pself->target].openstuff;
            break;

        case FIFGRABBED:
            // Proceed only if the character was picked up
            returncode = ( 0 != ( pself->alert & ALERTIF_GRABBED ) );
            break;

        case FIFDROPPED:
            // Proceed only if the character was dropped
            returncode = ( 0 != ( pself->alert & ALERTIF_DROPPED ) );
            break;

        case FSETTARGETTOWHOEVERISHOLDING:
            // This function sets the target to the character's mount or holder,
            // failing if the character has no mount or holder
            returncode = bfalse;
            if ( pchr->attachedto < MAXCHR )
            {
                pself->target = pchr->attachedto;
                returncode = btrue;
            }
            break;

        case FDAMAGETARGET:
            // This function applies little bit of love to the character's target.
            // The amount is set in tmpargument
            damage_character( pself->target, 0, pstate->argument, 1, pchr->damagetargettype, pchr->team, pself->index, DAMFXBLOC, btrue );
            break;

        case FIFXISLESSTHANY:
            // Proceed only if tmpx is less than tmpy
            returncode = ( pstate->x < pstate->y );
            break;

        case FSETWEATHERTIME:
            // Set the weather timer
            weathertimereset = pstate->argument;
            weathertime = pstate->argument;
            break;

        case FGETBUMPHEIGHT:
            // Get the characters bump height
            pstate->argument = pchr->bumpheight;
            break;

        case FIFREAFFIRMED:
            // Proceed only if the character was reaffirmed
            returncode = ( 0 != ( pself->alert & ALERTIF_REAFFIRMED ) );
            break;

        case FUNKEEPACTION:
            // This function makes the current animation start again
            pchr->keepaction = bfalse;
            break;

        case FIFTARGETISONOTHERTEAM:
            // This function proceeds only if the target is on another team
            returncode = ( ChrList[pself->target].alive && ChrList[pself->target].team != pchr->team );
            break;

        case FIFTARGETISONHATEDTEAM:
            // This function proceeds only if the target is on an enemy team
            returncode = ( ChrList[pself->target].alive && TeamList[pchr->team].hatesteam[ChrList[pself->target].team] && !ChrList[pself->target].invictus );
            break;

        case FPRESSLATCHBUTTON:
            // This function sets the latch buttons
            pchr->latchbutton = pchr->latchbutton | pstate->argument;
            break;

        case FSETTARGETTOTARGETOFLEADER:
            // This function sets the character's target to the target of its leader,
            // or it fails with no change if the leader is dead
            returncode = bfalse;
            if ( TeamList[pchr->team].leader != NOLEADER )
            {
                pself->target = ChrList[TeamList[pchr->team].leader].ai.target;
                returncode = btrue;
            }
            break;

        case FIFLEADERKILLED:
            // This function proceeds only if the character's leader has just died
            returncode = ( 0 != ( pself->alert & ALERTIF_LEADERKILLED ) );
            break;

        case FBECOMELEADER:
            // This function makes the character the team leader
            TeamList[pchr->team].leader = pself->index;
            break;

        case FCHANGETARGETARMOR:
            // This function sets the target's armor type and returns the old type
            // as tmpargument and the new type as tmpx
            iTmp = ChrList[pself->target].skin;
            pstate->x = change_armor( pself->target, pstate->argument );
            pstate->argument = iTmp;  // The character's old armor
            break;

        case FGIVEMONEYTOTARGET:
            // This function transfers money from the character to the target, and sets
            // tmpargument to the amount transferred
            iTmp = pchr->money;
            tTmp = ChrList[pself->target].money;
            iTmp -= pstate->argument;
            tTmp += pstate->argument;
            if ( iTmp < 0 ) { tTmp += iTmp;  pstate->argument += iTmp;  iTmp = 0; }
            if ( tTmp < 0 ) { iTmp += tTmp;  pstate->argument += tTmp;  tTmp = 0; }
            if ( iTmp > MAXMONEY ) { iTmp = MAXMONEY; }
            if ( tTmp > MAXMONEY ) { tTmp = MAXMONEY; }

            pchr->money = iTmp;
            ChrList[pself->target].money = tTmp;
            break;

        case FDROPKEYS:
            drop_keys( pself->index );
            break;

        case FIFLEADERISALIVE:
            // This function fails if there is no team leader
            returncode = ( TeamList[pchr->team].leader != NOLEADER );
            break;

        case FIFTARGETISOLDTARGET:
            // This function returns bfalse if the target has pstate->changed
            returncode = ( pself->target == pself->target_old );
            break;

        case FSETTARGETTOLEADER:

            // This function fails if there is no team leader
            if ( TeamList[pchr->team].leader == NOLEADER )
            {
                returncode = bfalse;
            }
            else
            {
                pself->target = TeamList[pchr->team].leader;
            }
            break;

        case FSPAWNCHARACTER:
            // This function spawns a character, failing if x,y is invalid
            sTmp = spawn_one_character( pstate->x, pstate->y, 0, pchr->model, pchr->team, 0, pstate->turn & 0xFFFF, NULL, MAXCHR );
            returncode = bfalse;
            if ( sTmp < MAXCHR )
            {
                if ( __chrhitawall( sTmp ) )
                {
                    free_one_character( sTmp );
                    sTmp = MAXCHR;
                }
                else
                {
                    ChrList[sTmp].iskursed = bfalse;

                    pself->child = sTmp;

                    tTmp = pchr->turnleftright >> 2;
                    ChrList[sTmp].xvel += turntocos[( tTmp+8192 )&TRIG_TABLE_MASK] * pstate->distance;
                    ChrList[sTmp].yvel += turntosin[( tTmp+8192 )&TRIG_TABLE_MASK] * pstate->distance;

                    ChrList[sTmp].ai.passage = pself->passage;
                    ChrList[sTmp].ai.owner   = pself->owner;
                }
            }
            returncode = (sTmp != MAXCHR);
            break;

        case FRESPAWNCHARACTER:
            // This function respawns the character at its starting location
            respawn_character( pself->index );
            break;

        case FCHANGETILE:
            // This function changes the floor image under the character
            returncode = bfalse;
            if ( INVALID_TILE != pchr->onwhichfan )
            {
                returncode = btrue;
                mesh.mem.tile_list[pchr->onwhichfan].img = pstate->argument & 0xFF;
            }
            break;

        case FIFUSED:
            // This function proceeds only if the character has been used
            returncode = ( 0 != ( pself->alert & ALERTIF_USED ) );
            break;

        case FDROPMONEY:
            // This function drops some of a character's money
            drop_money( pself->index, pstate->argument );
            break;

        case FSETOLDTARGET:
            // This function sets the old target to the current target
            pself->target_old = pself->target;
            break;

        case FDETACHFROMHOLDER:

            // This function drops the character, failing only if it was not held
            if ( pchr->attachedto != MAXCHR )
            {
                detach_character_from_mount( pself->index, btrue, btrue );
            }
            else
            {
                returncode = bfalse;
            }
            break;

        case FIFTARGETHASVULNERABILITYID:
            // This function proceeds if ID matches tmpargument
            returncode = ( CapList[ChrList[pself->target].model].idsz[IDSZ_VULNERABILITY] == ( IDSZ ) pstate->argument );
            break;

        case FCLEANUP:
            // This function issues the clean up order to all teammates
            issue_clean( pself->index );
            break;

        case FIFCLEANEDUP:
            // This function proceeds only if the character was told to clean up
            returncode = ( 0 != ( pself->alert & ALERTIF_CLEANEDUP ) );
            break;

        case FIFSITTING:
            // This function proceeds if the character is riding another
            returncode = ( pchr->attachedto != MAXCHR );
            break;

        case FIFTARGETISHURT:

            // This function passes only if the target is hurt and alive
            if ( !ChrList[pself->target].alive || ChrList[pself->target].life > ChrList[pself->target].lifemax - HURTDAMAGE )
                returncode = bfalse;

            break;

        case FIFTARGETISAPLAYER:
            // This function proceeds only if the target is a player ( may not be local )
            returncode = ChrList[pself->target].isplayer;
            break;

        case FPLAYSOUND:

            // This function plays a sound
            if ( pchr->oldz > PITNOSOUND && pstate->argument >= 0 && pstate->argument < MAXWAVE )
            {
                sound_play_chunk( pchr->oldx, pchr->oldy, CapList[pchr->model].wavelist[pstate->argument] );
            }
            break;

        case FSPAWNPARTICLE:
            // This function spawns a particle
            tTmp = pself->index;
            if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

            tTmp = spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, pchr->turnleftright, pchr->model, pstate->argument, pself->index, pstate->distance, pchr->team, tTmp, 0, MAXCHR );
            if ( tTmp != TOTALMAXPRT )
            {
                // Detach the particle
                attach_particle_to_character( tTmp, pself->index, pstate->distance );
                PrtList[tTmp].attachedtocharacter = MAXCHR;
                // Correct X, Y, Z spacing
                PrtList[tTmp].xpos += pstate->x;
                PrtList[tTmp].ypos += pstate->y;
                PrtList[tTmp].zpos += PipList[PrtList[tTmp].pip].zspacingbase;

                // Don't spawn in walls
                if ( __prthitawall( tTmp ) )
                {
                    PrtList[tTmp].xpos = pchr->xpos;
                    if ( __prthitawall( tTmp ) )
                    {
                        PrtList[tTmp].ypos = pchr->ypos;
                    }
                }
            }
            break;

        case FIFTARGETISALIVE:
            // This function proceeds only if the target is alive
            returncode = ChrList[pself->target].alive;
            break;

        case FSTOP:
            pchr->maxaccel = 0;
            break;

        case FDISAFFIRMCHARACTER:
            disaffirm_attached_particles( pself->index );
            break;

        case FREAFFIRMCHARACTER:
            reaffirm_attached_particles( pself->index );
            break;

        case FIFTARGETISSELF:
            // This function proceeds only if the target is the character too
            returncode = ( pself->target == pself->index );
            break;

        case FIFTARGETISMALE:
            // This function proceeds only if the target is male
            returncode = ( ChrList[pself->target].gender == GENMALE );
            break;

        case FIFTARGETISFEMALE:
            // This function proceeds only if the target is female
            returncode = ( ChrList[pself->target].gender == GENFEMALE );
            break;

        case FSETTARGETTOSELF:
            // This function sets the target to the character
            pself->target = pself->index;
            break;

        case FSETTARGETTORIDER:

            // This function sets the target to the character's left/only grip weapon,
            // failing if there is none
            if ( pchr->holdingwhich[0] == MAXCHR )
            {
                returncode = bfalse;
            }
            else
            {
                pself->target = pchr->holdingwhich[0];
            }
            break;

        case FGETATTACKTURN:
            // This function sets tmpturn to the direction of the last attack
            pstate->turn = pself->directionlast;
            break;

        case FGETDAMAGETYPE:
            // This function gets the last type of damage
            pstate->argument = pself->damagetypelast;
            break;

        case FBECOMESPELL:
            // This function turns the spellbook character into a spell based on its
            // content
            pchr->money = ( pchr->skin ) % MAXSKIN;
            change_character( pself->index, pself->content, 0, LEAVENONE );
            pself->content = 0;  // Reset so it doesn't mess up
            pself->state   = 0;  // Reset so it doesn't mess up
            pself->changed = btrue;
            break;

        case FBECOMESPELLBOOK:
            // This function turns the spell into a spellbook, and sets the content
            // accordingly
            pself->content = pchr->model;
            change_character( pself->index, SPELLBOOK, pchr->money % MAXSKIN, LEAVENONE );
            pself->state   = 0;  // Reset so it doesn't burn up
            pself->changed = btrue;
            break;

        case FIFSCOREDAHIT:
            // Proceed only if the character scored a hit
            returncode = ( 0 != ( pself->alert & ALERTIF_SCOREDAHIT ) );
            break;

        case FIFDISAFFIRMED:
            // Proceed only if the character was disaffirmed
            returncode = ( 0 != ( pself->alert & ALERTIF_DISAFFIRMED ) );
            break;

        case FTRANSLATEORDER:
            // This function gets the order and sets tmpx, tmpy, tmpargument and the
            // target ( if valid )
            sTmp = pself->order >> 24;
            if ( sTmp < MAXCHR )
            {
                pself->target = sTmp;
            }

            pstate->x = ( ( pself->order >> 14 ) & 1023 ) << 6;
            pstate->y = ( ( pself->order >> 4 ) & 1023 ) << 6;
            pstate->argument = pself->order & 15;
            break;

        case FSETTARGETTOWHOEVERWASHIT:
            // This function sets the target to whoever the character hit last,
            pself->target = pself->hitlast;
            break;

        case FSETTARGETTOWIDEENEMY:
            // This function finds an enemy, and proceeds only if there is one
            returncode = bfalse;
            if (get_target( pself->index, WIDE, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse) != MAXCHR) returncode = btrue;

            break;

        case FIFCHANGED:
            // Proceed only if the character was polymorphed
            returncode = ( 0 != ( pself->alert & ALERTIF_CHANGED ) );
            break;

        case FIFINWATER:
            // Proceed only if the character got wet
            returncode = ( 0 != ( pself->alert & ALERTIF_INWATER ) );
            break;

        case FIFBORED:
            // Proceed only if the character is bored
            returncode = ( 0 != ( pself->alert & ALERTIF_BORED ) );
            break;

        case FIFTOOMUCHBAGGAGE:
            // Proceed only if the character tried to grab too much
            returncode = ( 0 != ( pself->alert & ALERTIF_TOOMUCHBAGGAGE ) );
            break;

        case FIFGROGGED:
            // Proceed only if the character was grogged
            returncode = ( 0 != ( pself->alert & ALERTIF_GROGGED ) );
            break;

        case FIFDAZED:
            // Proceed only if the character was dazed
            returncode = ( 0 != ( pself->alert & ALERTIF_DAZED ) );
            break;

        case FIFTARGETHASSPECIALID:
            // This function proceeds if ID matches tmpargument
            returncode = ( CapList[ChrList[pself->target].model].idsz[IDSZ_SPECIAL] == ( IDSZ ) pstate->argument );
            break;

        case FPRESSTARGETLATCHBUTTON:
            // This function sets the target's latch buttons
            ChrList[pself->target].latchbutton = ChrList[pself->target].latchbutton | pstate->argument;
            break;

        case FIFINVISIBLE:
            // This function passes if the character is invisible
            returncode = ( pchr->inst.alpha <= INVISIBLE ) || ( pchr->inst.light <= INVISIBLE );
            break;

        case FIFARMORIS:
            // This function passes if the character's skin is tmpargument
            tTmp = pchr->skin;
            returncode = ( tTmp == pstate->argument );
            break;

        case FGETTARGETGROGTIME:
            // This function returns tmpargument as the grog time, and passes if it is not 0
            pstate->argument = pchr->grogtime;
            returncode = ( pstate->argument != 0 );
            break;

        case FGETTARGETDAZETIME:
            // This function returns tmpargument as the daze time, and passes if it is not 0
            pstate->argument = pchr->dazetime;
            returncode = ( pstate->argument != 0 );
            break;

        case FSETDAMAGETYPE:
            // This function sets the bump damage type
            pchr->damagetargettype = pstate->argument & ( DAMAGE_COUNT - 1 );
            break;

        case FSETWATERLEVEL:
            // This function raises and lowers the module's water
            fTmp = ( pstate->argument / 10.0f ) - waterdouselevel;
            watersurfacelevel += fTmp;
            waterdouselevel += fTmp;

            for ( iTmp = 0; iTmp < MAXWATERLAYER; iTmp++ )
            {
                waterlayerz[iTmp] += fTmp;
            }
            break;

        case FENCHANTTARGET:
            // This function enchants the target
            sTmp = spawn_enchant( pself->owner, pself->target, pself->index, MAXENCHANT, MAXMODEL );
            returncode = ( sTmp != MAXENCHANT );
            break;

        case FENCHANTCHILD:
            // This function can be used with SpawnCharacter to enchant the
            // newly spawned character
            sTmp = spawn_enchant( pself->owner, pself->child, pself->index, MAXENCHANT, MAXMODEL );
            returncode = ( sTmp != MAXENCHANT );
            break;

        case FTELEPORTTARGET:
            // This function teleports the target to the X, Y location, failing if the
            // location is off the map or blocked
            returncode = bfalse;
            if ( pstate->x > EDGE && pstate->y > EDGE && pstate->x < mesh.info.edge_x - EDGE && pstate->y < mesh.info.edge_y - EDGE )
            {
                // Yeah!  It worked!
                sTmp = pself->target;
                detach_character_from_mount( sTmp, btrue, bfalse );
                ChrList[sTmp].oldx = ChrList[sTmp].xpos;
                ChrList[sTmp].oldy = ChrList[sTmp].ypos;
                ChrList[sTmp].xpos = pstate->x;
                ChrList[sTmp].ypos = pstate->y;
                ChrList[sTmp].zpos = pstate->distance;
                ChrList[sTmp].turnleftright = pstate->turn & 0xFFFF;
                if ( __chrhitawall( sTmp ) )
                {
                    // No it didn't...
                    ChrList[sTmp].xpos = ChrList[sTmp].oldx;
                    ChrList[sTmp].ypos = ChrList[sTmp].oldy;
                    ChrList[sTmp].zpos = ChrList[sTmp].oldz;
                    ChrList[sTmp].turnleftright = ChrList[sTmp].oldturn;
                    returncode = bfalse;
                }
                else
                {
                    ChrList[sTmp].oldx = ChrList[sTmp].xpos;
                    ChrList[sTmp].oldy = ChrList[sTmp].ypos;
                    ChrList[sTmp].oldz = ChrList[sTmp].zpos;
                    ChrList[sTmp].oldturn = ChrList[sTmp].turnleftright;
                    returncode = btrue;
                }
            }
            break;

        case FGIVEEXPERIENCETOTARGET:
            // This function gives the target some experience, xptype from distance,
            // amount from argument...
            give_experience( pself->target, pstate->argument, pstate->distance, bfalse );
            break;

        case FINCREASEAMMO:

            // This function increases the ammo by one
            if ( pchr->ammo < pchr->ammomax )
            {
                pchr->ammo++;
            }
            break;

        case FUNKURSETARGET:
            // This function unkurses the target
            ChrList[pself->target].iskursed = bfalse;
            break;

        case FGIVEEXPERIENCETOTARGETTEAM:
            // This function gives experience to everyone on the target's team
            give_team_experience( ChrList[pself->target].team, pstate->argument, pstate->distance );
            break;

        case FIFUNARMED:
            // This function proceeds if the character has no item in hand
            returncode = ( pchr->holdingwhich[0] == MAXCHR && pchr->holdingwhich[1] == MAXCHR );
            break;

        case FRESTOCKTARGETAMMOIDALL:
            // This function restocks the ammo of every item the character is holding,
            // if the item matches the ID given ( parent or child type )
            iTmp = 0;  // Amount of ammo given
            sTmp = ChrList[pself->target].holdingwhich[0];
            iTmp += restock_ammo( sTmp, pstate->argument );
            sTmp = ChrList[pself->target].holdingwhich[1];
            iTmp += restock_ammo( sTmp, pstate->argument );
            sTmp = ChrList[pself->target].nextinpack;

            while ( sTmp != MAXCHR )
            {
                iTmp += restock_ammo( sTmp, pstate->argument );
                sTmp = ChrList[sTmp].nextinpack;
            }

            pstate->argument = iTmp;
            returncode = ( iTmp != 0 );
            break;

        case FRESTOCKTARGETAMMOIDFIRST:
            // This function restocks the ammo of the first item the character is holding,
            // if the item matches the ID given ( parent or child type )
            iTmp = 0;  // Amount of ammo given
            sTmp = ChrList[pself->target].holdingwhich[0];
            iTmp += restock_ammo( sTmp, pstate->argument );
            if ( iTmp == 0 )
            {
                sTmp = ChrList[pself->target].holdingwhich[1];
                iTmp += restock_ammo( sTmp, pstate->argument );
                if ( iTmp == 0 )
                {
                    sTmp = ChrList[pself->target].nextinpack;

                    while ( sTmp != MAXCHR && iTmp == 0 )
                    {
                        iTmp += restock_ammo( sTmp, pstate->argument );
                        sTmp = ChrList[sTmp].nextinpack;
                    }
                }
            }

            pstate->argument = iTmp;
            returncode = ( iTmp != 0 );
            break;

        case FFLASHTARGET:
            // This function flashes the character
            flash_character( pself->target, 255 );
            break;

        case FSETREDSHIFT:
            // This function alters a character's coloration
            pchr->inst.redshift = pstate->argument;
            break;

        case FSETGREENSHIFT:
            // This function alters a character's coloration
            pchr->inst.grnshift = pstate->argument;
            break;

        case FSETBLUESHIFT:
            // This function alters a character's coloration
            pchr->inst.blushift = pstate->argument;
            break;

        case FSETLIGHT:
            // This function alters a character's transparency
            pchr->inst.light = pstate->argument;
            break;

        case FSETALPHA:
            // This function alters a character's transparency
            pchr->inst.alpha = pstate->argument;
            break;

        case FIFHITFROMBEHIND:
            // This function proceeds if the character was attacked from behind
            returncode = bfalse;
            if ( pself->directionlast >= BEHIND - 8192 && pself->directionlast < BEHIND + 8192 )
                returncode = btrue;

            break;

        case FIFHITFROMFRONT:
            // This function proceeds if the character was attacked from the front
            returncode = bfalse;
            if ( pself->directionlast >= 49152 + 8192 || pself->directionlast < FRONT + 8192 )
                returncode = btrue;

            break;

        case FIFHITFROMLEFT:
            // This function proceeds if the character was attacked from the left
            returncode = bfalse;
            if ( pself->directionlast >= LEFT - 8192 && pself->directionlast < LEFT + 8192 )
                returncode = btrue;

            break;

        case FIFHITFROMRIGHT:
            // This function proceeds if the character was attacked from the right
            returncode = bfalse;
            if ( pself->directionlast >= RIGHT - 8192 && pself->directionlast < RIGHT + 8192 )
                returncode = btrue;

            break;

        case FIFTARGETISONSAMETEAM:
            // This function proceeds only if the target is on another team
            returncode = bfalse;
            if ( ChrList[pself->target].team == pchr->team )
                returncode = btrue;

            break;

        case FKILLTARGET:
            // This function kills the target
            kill_character( pself->target, pself->index );
            break;

        case FUNDOENCHANT:
            // This function undoes the last enchant
            returncode = ( pchr->undoenchant != MAXENCHANT );
            remove_enchant( pchr->undoenchant );
            break;

        case FGETWATERLEVEL:
            // This function gets the douse level for the water, returning it in tmpargument
            pstate->argument = waterdouselevel * 10;
            break;

        case FCOSTTARGETMANA:
            // This function costs the target some mana
            returncode = cost_mana( pself->target, pstate->argument, pself->index );
            break;

        case FIFTARGETHASANYID:
            // This function proceeds only if one of the target's IDSZ's matches tmpargument
            returncode = 0;
            tTmp = 0;

            while ( tTmp < IDSZ_COUNT )
            {
                returncode |= ( CapList[ChrList[pself->target].model].idsz[tTmp] == ( IDSZ ) pstate->argument );
                tTmp++;
            }
            break;

        case FSETBUMPSIZE:
            // This function sets the character's bump size
            fTmp = pchr->bumpsizebig;
            fTmp = fTmp / pchr->bumpsize;  // 1.5f or 2.0f
            pchr->bumpsize = pstate->argument * pchr->fat;
            pchr->bumpsizebig = fTmp * pchr->bumpsize;
            pchr->bumpsizesave = pstate->argument;
            pchr->bumpsizebigsave = fTmp * pchr->bumpsizesave;
            break;

        case FIFNOTDROPPED:
            // This function passes if a kursed item could not be dropped
            returncode = ( 0 != ( pself->alert & ALERTIF_NOTDROPPED ) );
            break;

        case FIFYISLESSTHANX:
            // This function passes only if tmpy is less than tmpx
            returncode = ( pstate->y < pstate->x );
            break;

        case FSETFLYHEIGHT:
            // This function sets a character's fly height
            pchr->flyheight = pstate->argument;
            break;

        case FIFBLOCKED:
            // This function passes if the character blocked an attack
            returncode = ( 0 != ( pself->alert & ALERTIF_BLOCKED ) );
            break;

        case FIFTARGETISDEFENDING:
            returncode = ( ChrList[pself->target].action >= ACTIONPA && ChrList[pself->target].action <= ACTIONPD );
            break;

        case FIFTARGETISATTACKING:
            returncode = ( ChrList[pself->target].action >= ACTIONUA && ChrList[pself->target].action <= ACTIONFD );
            break;

        case FIFSTATEIS0:
            returncode = ( 0 == pself->state );
            break;

        case FIFSTATEIS1:
            returncode = ( 1 == pself->state );
            break;

        case FIFSTATEIS2:
            returncode = ( 2 == pself->state );
            break;

        case FIFSTATEIS3:
            returncode = ( 3 == pself->state );
            break;

        case FIFSTATEIS4:
            returncode = ( 4 == pself->state );
            break;

        case FIFSTATEIS5:
            returncode = ( 5 == pself->state );
            break;

        case FIFSTATEIS6:
            returncode = ( 6 == pself->state );
            break;

        case FIFSTATEIS7:
            returncode = ( 7 == pself->state );
            break;

        case FIFCONTENTIS:
            returncode = ( pstate->argument == pself->content );
            break;

        case FSETTURNMODETOWATCHTARGET:
            // This function sets the turn mode
            pchr->turnmode = TURNMODEWATCHTARGET;
            break;

        case FIFSTATEISNOT:
            returncode = ( pstate->argument != pself->state );
            break;

        case FIFXISEQUALTOY:
            returncode = ( pstate->x == pstate->y );
            break;

        case FDEBUGMESSAGE:
            // This function spits out a debug message
            sprintf( cTmp, "aistate %d, aicontent %d, target %d", pself->state, pself->content, pself->target );
            debug_message( cTmp );
            sprintf( cTmp, "tmpx %d, tmpy %d", pstate->x, pstate->y );
            debug_message( cTmp );
            sprintf( cTmp, "tmpdistance %d, tmpturn %d", pstate->distance, pstate->turn );
            debug_message( cTmp );
            sprintf( cTmp, "tmpargument %d, selfturn %d", pstate->argument, pchr->turnleftright );
            debug_message( cTmp );
            break;

        case FBLACKTARGET:
            // This function makes the target flash black
            flash_character( pself->target, 0 );
            break;

        case FSENDMESSAGENEAR:
            // This function sends a message if the camera is in the nearby area.
            iTmp = ABS( pchr->oldx - gCamera.trackx ) + ABS( pchr->oldy - gCamera.tracky );
            if ( iTmp < MSGDISTANCE )
            {
                display_message( pstate, MadList[pchr->inst.imad].msgstart + pstate->argument, pself->index );
            }
            break;

        case FIFHITGROUND:
            // This function passes if the character just hit the ground
            returncode = ( 0 != ( pself->alert & ALERTIF_HITGROUND ) );
            break;

        case FIFNAMEISKNOWN:
            // This function passes if the character's name is known
            returncode = pchr->nameknown;
            break;

        case FIFUSAGEISKNOWN:
            // This function passes if the character's usage is known
            returncode = CapList[pchr->model].usageknown;
            break;

        case FIFHOLDINGITEMID:
            // This function passes if the character is holding an item with the IDSZ given
            // in tmpargument, returning the latch to press to use it
            returncode = bfalse;
            // Check left hand
            sTmp = pchr->holdingwhich[0];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
                {
                    pstate->argument = LATCHBUTTON_LEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = pchr->holdingwhich[1];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
                {
                    pstate->argument = LATCHBUTTON_RIGHT;
                    if ( returncode )  pstate->argument = LATCHBUTTON_LEFT + ( rand() & 1 );

                    returncode = btrue;
                }
            }
            break;

        case FIFHOLDINGRANGEDWEAPON:
            // This function passes if the character is holding a ranged weapon, returning
            // the latch to press to use it.  This also checks ammo/ammoknown.
            returncode = bfalse;
            pstate->argument = 0;
            // Check left hand
            tTmp = pchr->holdingwhich[0];
            if ( tTmp != MAXCHR )
            {
                sTmp = ChrList[tTmp].model;
                if ( CapList[sTmp].isranged && ( ChrList[tTmp].ammomax == 0 || ( ChrList[tTmp].ammo != 0 && ChrList[tTmp].ammoknown ) ) )
                {
                    pstate->argument = LATCHBUTTON_LEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            tTmp = pchr->holdingwhich[1];
            if ( tTmp != MAXCHR )
            {
                sTmp = ChrList[tTmp].model;
                if ( CapList[sTmp].isranged && ( ChrList[tTmp].ammomax == 0 || ( ChrList[tTmp].ammo != 0 && ChrList[tTmp].ammoknown ) ) )
                {
                    if ( pstate->argument == 0 || ( frame_all&1 ) )
                    {
                        pstate->argument = LATCHBUTTON_RIGHT;
                        returncode = btrue;
                    }
                }
            }
            break;

        case FIFHOLDINGMELEEWEAPON:
            // This function passes if the character is holding a melee weapon, returning
            // the latch to press to use it
            returncode = bfalse;
            pstate->argument = 0;
            // Check left hand
            sTmp = pchr->holdingwhich[0];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( !CapList[sTmp].isranged && CapList[sTmp].weaponaction != ACTIONPA )
                {
                    pstate->argument = LATCHBUTTON_LEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = pchr->holdingwhich[1];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( !CapList[sTmp].isranged && CapList[sTmp].weaponaction != ACTIONPA )
                {
                    if ( pstate->argument == 0 || ( frame_all&1 ) )
                    {
                        pstate->argument = LATCHBUTTON_RIGHT;
                        returncode = btrue;
                    }
                }
            }
            break;

        case FIFHOLDINGSHIELD:
            // This function passes if the character is holding a shield, returning the
            // latch to press to use it
            returncode = bfalse;
            pstate->argument = 0;
            // Check left hand
            sTmp = pchr->holdingwhich[0];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].weaponaction == ACTIONPA )
                {
                    pstate->argument = LATCHBUTTON_LEFT;
                    returncode = btrue;
                }
            }

            // Check right hand
            sTmp = pchr->holdingwhich[1];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].weaponaction == ACTIONPA )
                {
                    pstate->argument = LATCHBUTTON_RIGHT;
                    returncode = btrue;
                }
            }
            break;

        case FIFKURSED:
            // This function passes if the character is kursed
            returncode = pchr->iskursed;
            break;

        case FIFTARGETISKURSED:
            // This function passes if the target is kursed
            returncode = ChrList[pself->target].iskursed;
            break;

        case FIFTARGETISDRESSEDUP:
            // This function passes if the character's skin is dressy
            iTmp = pchr->skin;
            iTmp = 1 << iTmp;
            returncode = ( ( CapList[pchr->model].skindressy & iTmp ) != 0 );
            break;

        case FIFOVERWATER:
            // This function passes if the character is on a water tile
            returncode = bfalse;
            if ( INVALID_TILE != pchr->onwhichfan )
            {
                returncode = ( ( mesh.mem.tile_list[pchr->onwhichfan].fx & MESHFX_WATER ) != 0 && wateriswater );
            }
            break;

        case FIFTHROWN:
            // This function passes if the character was thrown
            returncode = ( 0 != ( pself->alert & ALERTIF_THROWN ) );
            break;

        case FMAKENAMEKNOWN:
            // This function makes the name of an item/character known.
            pchr->nameknown = btrue;
            //            ChrList[character].icon = btrue;
            break;

        case FMAKEUSAGEKNOWN:
            // This function makes the usage of an item known...  For XP gains from
            // using an unknown potion or such
            CapList[pchr->model].usageknown = btrue;
            break;

        case FSTOPTARGETMOVEMENT:
            // This function makes the target stop moving temporarily
            ChrList[pself->target].xvel = 0;
            ChrList[pself->target].yvel = 0;
            if ( ChrList[pself->target].zvel > 0 ) ChrList[pself->target].zvel = gravity;

            break;

        case FSETXY:
            // This function stores tmpx and tmpy in the storage array
            pself->x[pstate->argument&STORAND] = pstate->x;
            pself->y[pstate->argument&STORAND] = pstate->y;
            break;

        case FGETXY:
            // This function gets previously stored data, setting tmpx and tmpy
            pstate->x = pself->x[pstate->argument&STORAND];
            pstate->y = pself->y[pstate->argument&STORAND];
            break;

        case FADDXY:
            // This function adds tmpx and tmpy to the storage array
            pself->x[pstate->argument&STORAND] += pstate->x;
            pself->y[pstate->argument&STORAND] += pstate->y;
            break;

        case FMAKEAMMOKNOWN:
            // This function makes the ammo of an item/character known.
            pchr->ammoknown = btrue;
            break;

        case FSPAWNATTACHEDPARTICLE:
            // This function spawns an attached particle
            tTmp = pself->index;
            if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

            tTmp = spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, pchr->turnleftright, pchr->model, pstate->argument, pself->index, pstate->distance, pchr->team, tTmp, 0, MAXCHR );
            returncode = (tTmp != TOTALMAXPRT);
            break;

        case FSPAWNEXACTPARTICLE:
            // This function spawns an exactly placed particle
            tTmp = pself->index;
            if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

            spawn_one_particle( pstate->x, pstate->y, pstate->distance, pchr->turnleftright, pchr->model, pstate->argument, MAXCHR, 0, pchr->team, tTmp, 0, MAXCHR );
            break;

        case FACCELERATETARGET:
            // This function changes the target's speeds
            ChrList[pself->target].xvel += pstate->x;
            ChrList[pself->target].yvel += pstate->y;
            break;

        case FIFDISTANCEISMORETHANTURN:
            // This function proceeds tmpdistance is greater than tmpturn
            returncode = ( pstate->distance > ( int ) pstate->turn );
            break;

        case FIFCRUSHED:
            // This function proceeds only if the character was crushed
            returncode = ( 0 != ( pself->alert & ALERTIF_CRUSHED ) );
            break;

        case FMAKECRUSHVALID:
            // This function makes doors able to close on this object
            pchr->canbecrushed = btrue;
            break;

        case FSETTARGETTOLOWESTTARGET:

            // This sets the target to whatever the target is being held by,
            // The lowest in the set.  This function never fails
            while ( ChrList[pself->target].attachedto != MAXCHR )
            {
                pself->target = ChrList[pself->target].attachedto;
            }
            break;

        case FIFNOTPUTAWAY:
            // This function proceeds only if the character couln't be put in the pack
            returncode = ( 0 != ( pself->alert & ALERTIF_NOTPUTAWAY ) );
            break;

        case FIFTAKENOUT:
            // This function proceeds only if the character was taken out of the pack
            returncode = ( 0 != ( pself->alert & ALERTIF_TAKENOUT ) );
            break;

        case FIFAMMOOUT:
            // This function proceeds only if the character has no ammo
            returncode = ( pchr->ammo == 0 );
            break;

        case FPLAYSOUNDLOOPED:
            // This function plays a looped sound
            if ( moduleactive )
            {
                // You could use this, but right now there's no way to stop the sound later, so it's better not to start it
                // sound_play_chunk(CapList[ChrList[character].model].wavelist[pstate->argument], PANMID, volume, pstate->distance);
            }
            break;

        case FSTOPSOUND:
            // TODO: implement this (the scripter doesn't know which channel to stop)
            // This function stops playing a sound
            // sound_stop_channel([pstate->argument]);
            break;

        case FHEALSELF:

            // This function heals the character, without setting the alert or modifying
            // the amount
            if ( pchr->alive )
            {
                iTmp = pchr->life + pstate->argument;
                if ( iTmp > pchr->lifemax ) iTmp = pchr->lifemax;
                if ( iTmp < 1 ) iTmp = 1;

                pchr->life = iTmp;
            }
            break;

        case FEQUIP:
            // This function flags the character as being equipped
            pchr->isequipped = btrue;
            break;

        case FIFTARGETHASITEMIDEQUIPPED:
            // This function proceeds if the target has a matching item equipped
            returncode = bfalse;
            sTmp = ChrList[pself->target].nextinpack;

            while ( sTmp != MAXCHR )
            {
                if ( sTmp != pself->index && ChrList[sTmp].isequipped && ( CapList[ChrList[sTmp].model].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument || CapList[ChrList[sTmp].model].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument ) )
                {
                    returncode = btrue;
                    sTmp = MAXCHR;
                }
                else
                {
                    sTmp = ChrList[sTmp].nextinpack;
                }
            }
            break;

        case FSETOWNERTOTARGET:
            // This function sets the owner
            pself->owner = pself->target;
            break;

        case FSETTARGETTOOWNER:
            // This function sets the target to the owner
            pself->target = pself->owner;
            break;

        case FSETFRAME:
            // This function sets the character's current frame
            sTmp = pstate->argument & 3;
            iTmp = pstate->argument >> 2;
            chr_set_frame( pself->index, iTmp, sTmp );
            break;

        case FBREAKPASSAGE:
            // This function makes the tiles fall away ( turns into damage terrain )
            returncode = break_passage( pstate, pstate->argument, pstate->turn & 0xFFFF, pstate->distance, pstate->x, pstate->y );
            break;

        case FSETRELOADTIME:

            // This function makes weapons fire slower
            if ( pstate->argument > 0 ) pchr->reloadtime = pstate->argument;
            else pchr->reloadtime = 0;

            break;

        case FSETTARGETTOWIDEBLAHID:
            // This function sets the target based on the settings of
            // tmpargument and tmpdistance
            {
                TARGET_TYPE blahteam = ALL;
                if ( ( pstate->distance >> 2 ) & 1 )  blahteam = FRIEND;
                if ( (( pstate->distance >> 1 ) & 1) && blahteam == FRIEND ) blahteam = ALL;
                else if ((( pstate->distance >> 1 ) & 1)) blahteam = ENEMY;
                else returncode = bfalse;
                if (returncode)
                {
                    returncode = bfalse;
                    if (get_target(pself->index, WIDE, blahteam, ( ( pstate->distance >> 3 ) & 1 ),
                                   ( ( pstate->distance ) & 1 ), pstate->argument, (( pstate->distance >> 4 ) & 1) ) != MAXCHR) returncode = btrue;
                }
            }
            break;

        case FPOOFTARGET:
            // This function makes the target go away
            returncode = bfalse;
            if ( !ChrList[pself->target].isplayer )
            {
                returncode = btrue;
                if ( pself->target == pself->index )
                {
                    // Poof self later
                    pself->poof_time = frame_wld + 1;
                }
                else
                {
                    // Poof others now
                    ChrList[pself->target].ai.poof_time = frame_wld;
                    pself->target = pself->index;
                }
            }
            break;

        case FCHILDDOACTIONOVERRIDE:
            // This function starts a new action, if it is valid for the model
            // It will fail if the action is invalid
            returncode = bfalse;
            if ( pstate->argument < MAXACTION )
            {
                if ( MadList[ChrList[pself->child].inst.imad].actionvalid[pstate->argument] )
                {
                    ChrList[pself->child].action = pstate->argument;
                    ChrList[pself->child].inst.lip = 0;
                    ChrList[pself->child].inst.frame = MadList[ChrList[pself->child].inst.imad].actionstart[pstate->argument];
                    ChrList[pself->child].inst.lastframe = ChrList[pself->child].inst.frame;
                    ChrList[pself->child].actionready = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FSPAWNPOOF:
            // This function makes a lovely little poof at the character's location
            spawn_poof( pself->index, pchr->model );
            break;

        case FSETSPEEDPERCENT:
            reset_character_accel( pself->index );
            pchr->maxaccel = pchr->maxaccel * pstate->argument / 100.0f;
            break;

        case FSETCHILDSTATE:
            // This function sets the child's state
            ChrList[pself->child].ai.state = pstate->argument;
            break;

        case FSPAWNATTACHEDSIZEDPARTICLE:
            // This function spawns an attached particle, then sets its size
            tTmp = pself->index;
            if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

            tTmp = spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, pchr->turnleftright, pchr->model, pstate->argument, pself->index, pstate->distance, pchr->team, tTmp, 0, MAXCHR );
            if ( tTmp < TOTALMAXPRT )
            {
                PrtList[tTmp].size = pstate->turn;
            }
            returncode = (tTmp != TOTALMAXPRT);
            break;

        case FCHANGEARMOR:
            // This function sets the character's armor type and returns the old type
            // as tmpargument and the new type as tmpx
            pstate->x = pstate->argument;
            iTmp = pchr->skin;
            pstate->x = change_armor( pself->index, pstate->argument );
            pstate->argument = iTmp;  // The character's old armor
            break;

        case FSHOWTIMER:
            // This function turns the timer on, using the value for tmpargument
            timeron = btrue;
            timervalue = pstate->argument;
            break;

        case FIFFACINGTARGET:
            // This function proceeds only if the character is facing the target
            sTmp = ATAN2( ChrList[pself->target].ypos - pchr->ypos, ChrList[pself->target].xpos - pchr->xpos ) * 0xFFFF / ( TWO_PI );
            sTmp += 32768 - pchr->turnleftright;
            returncode = ( sTmp > 55535 || sTmp < 10000 );
            break;

        case FPLAYSOUNDVOLUME:

            // This function sets the volume of a sound and plays it
            if ( mixeron && moduleactive && pstate->distance >= 0 )
            {
                volume = pstate->distance;
                iTmp = -1;
                if ( pstate->argument >= 0 && pstate->argument < MAXWAVE )
                {
                    iTmp = sound_play_chunk( pchr->oldx, pchr->oldy, CapList[pchr->model].wavelist[pstate->argument] );
                }

                if ( -1 != iTmp )
                {
                    Mix_Volume( iTmp, pstate->distance );
                }
            }
            break;

        case FSPAWNATTACHEDFACEDPARTICLE:
            // This function spawns an attached particle with facing
            tTmp = pself->index;
            if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

            tTmp = spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, pstate->turn & 0xFFFF, pchr->model, pstate->argument, pself->index, pstate->distance, pchr->team, tTmp, 0, MAXCHR );
            returncode = (tTmp != TOTALMAXPRT);
            break;

        case FIFSTATEISODD:
            returncode = ( pself->state & 1 );
            break;

        case FSETTARGETTODISTANTENEMY:
            // This function finds an enemy, within a certain distance to the character, and
            // proceeds only if there is one
            returncode = bfalse;
            if (get_target(pself->index, pstate->distance, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse) != MAXCHR) returncode = btrue;

            break;

        case FTELEPORT:
            // This function teleports the character to the X, Y location, failing if the
            // location is off the map or blocked
            returncode = bfalse;
            if ( pstate->x > EDGE && pstate->y > EDGE && pstate->x < mesh.info.edge_x - EDGE && pstate->y < mesh.info.edge_y - EDGE )
            {
                float x_old, y_old;

                x_old = pchr->xpos;
                y_old = pchr->ypos;
                pchr->xpos = pstate->x;
                pchr->ypos = pstate->y;
                if ( 0 == __chrhitawall( pself->index ) )
                {
                    // Yeah!  It worked!
                    detach_character_from_mount( pself->index, btrue, bfalse );
                    pchr->oldx = pchr->xpos;
                    pchr->oldy = pchr->ypos;
                    returncode = btrue;
                }
                else
                {
                    // No it didn't...
                    pchr->xpos = x_old;
                    pchr->ypos = y_old;
                    returncode = bfalse;
                }
            }
            break;

        case FGIVESTRENGTHTOTARGET:

            // Permanently boost the target's strength
            if ( ChrList[pself->target].alive )
            {
                iTmp = pstate->argument;
                getadd( 0, ChrList[pself->target].strength, PERFECTSTAT, &iTmp );
                ChrList[pself->target].strength += iTmp;
            }
            break;

        case FGIVEWISDOMTOTARGET:

            // Permanently boost the target's wisdom
            if ( ChrList[pself->target].alive )
            {
                iTmp = pstate->argument;
                getadd( 0, ChrList[pself->target].wisdom, PERFECTSTAT, &iTmp );
                ChrList[pself->target].wisdom += iTmp;
            }
            break;

        case FGIVEINTELLIGENCETOTARGET:

            // Permanently boost the target's intelligence
            if ( ChrList[pself->target].alive )
            {
                iTmp = pstate->argument;
                getadd( 0, ChrList[pself->target].intelligence, PERFECTSTAT, &iTmp );
                ChrList[pself->target].intelligence += iTmp;
            }
            break;

        case FGIVEDEXTERITYTOTARGET:

            // Permanently boost the target's dexterity
            if ( ChrList[pself->target].alive )
            {
                iTmp = pstate->argument;
                getadd( 0, ChrList[pself->target].dexterity, PERFECTSTAT, &iTmp );
                ChrList[pself->target].dexterity += iTmp;
            }
            break;

        case FGIVELIFETOTARGET:

            // Permanently boost the target's life
            if ( ChrList[pself->target].alive )
            {
                iTmp = pstate->argument;
                getadd( LOWSTAT, ChrList[pself->target].lifemax, PERFECTBIG, &iTmp );
                ChrList[pself->target].lifemax += iTmp;
                if ( iTmp < 0 )
                {
                    getadd( 1, ChrList[pself->target].life, PERFECTBIG, &iTmp );
                }

                ChrList[pself->target].life += iTmp;
            }
            break;

        case FGIVEMANATOTARGET:

            // Permanently boost the target's mana
            if ( ChrList[pself->target].alive )
            {
                iTmp = pstate->argument;
                getadd( 0, ChrList[pself->target].manamax, PERFECTBIG, &iTmp );
                ChrList[pself->target].manamax += iTmp;
                if ( iTmp < 0 )
                {
                    getadd( 0, ChrList[pself->target].mana, PERFECTBIG, &iTmp );
                }

                ChrList[pself->target].mana += iTmp;
            }
            break;

        case FSHOWMAP:

            // Show the map...  Fails if map already visible
            if ( mapon )  returncode = bfalse;

            mapon = mapvalid;
            break;

        case FSHOWYOUAREHERE:
            // Show the camera target location
            youarehereon = mapvalid;
            break;

        case FSHOWBLIPXY:

            // Add a blip
            if ( numblip < MAXBLIP )
            {
                if ( pstate->x > 0 && pstate->x < mesh.info.edge_x && pstate->y > 0 && pstate->y < mesh.info.edge_y )
                {
                    if ( pstate->argument < NUMBAR && pstate->argument >= 0 )
                    {
                        blipx[numblip] = pstate->x * MAPSIZE / mesh.info.edge_x;
                        blipy[numblip] = pstate->y * MAPSIZE / mesh.info.edge_y;
                        blipc[numblip] = pstate->argument;
                        numblip++;
                    }
                }
            }
            break;

        case FHEALTARGET:

            // Give some life to the target
            if ( ChrList[pself->target].alive )
            {
                iTmp = pstate->argument;
                getadd( 1, ChrList[pself->target].life, ChrList[pself->target].lifemax, &iTmp );
                ChrList[pself->target].life += iTmp;
                // Check all enchants to see if they are removed
                iTmp = ChrList[pself->target].firstenchant;

                while ( iTmp != MAXENCHANT )
                {
                    test = Make_IDSZ( "HEAL" );  // [HEAL]
                    sTmp = EncList[iTmp].nextenchant;
                    if ( test == EveList[EncList[iTmp].eve].removedbyidsz )
                    {
                        remove_enchant( iTmp );
                    }

                    iTmp = sTmp;
                }
            }
            break;

        case FPUMPTARGET:

            // Give some mana to the target
            if ( ChrList[pself->target].alive )
            {
                iTmp = pstate->argument;
                getadd( 0, ChrList[pself->target].mana, ChrList[pself->target].manamax, &iTmp );
                ChrList[pself->target].mana += iTmp;
            }
            break;

        case FCOSTAMMO:

            // Take away one ammo
            if ( pchr->ammo > 0 )
            {
                pchr->ammo--;
            }
            break;

        case FMAKESIMILARNAMESKNOWN:
            // Make names of matching objects known
            iTmp = 0;

            while ( iTmp < MAXCHR )
            {
                sTmp = btrue;
                tTmp = 0;

                while ( tTmp < IDSZ_COUNT )
                {
                    if ( CapList[pchr->model].idsz[tTmp] != CapList[ChrList[iTmp].model].idsz[tTmp] )
                    {
                        sTmp = bfalse;
                    }

                    tTmp++;
                }
                if ( sTmp )
                {
                    ChrList[iTmp].nameknown = btrue;
                }

                iTmp++;
            }
            break;

        case FSPAWNATTACHEDHOLDERPARTICLE:
            // This function spawns an attached particle, attached to the holder
            tTmp = pself->index;
            if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

            tTmp = spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, pchr->turnleftright, pchr->model, pstate->argument, tTmp, pstate->distance, pchr->team, tTmp, 0, MAXCHR );
            returncode = (tTmp != TOTALMAXPRT);
            break;

        case FSETTARGETRELOADTIME:

            // This function sets the target's reload time
            if ( pstate->argument > 0 )
                ChrList[pself->target].reloadtime = pstate->argument;
            else ChrList[pself->target].reloadtime = 0;

            break;

        case FSETFOGLEVEL:
            // This function raises and lowers the module's fog
            fTmp = ( pstate->argument / 10.0f ) - fogtop;
            fogtop += fTmp;
            fogdistance += fTmp;
            fogon = fogallowed;
            if ( fogdistance < 1.0f )  fogon = bfalse;

            break;

        case FGETFOGLEVEL:
            // This function gets the fog level
            pstate->argument = fogtop * 10;
            break;

        case FSETFOGTAD:
            // This function changes the fog color
            fogred = pstate->turn;
            foggrn = pstate->argument;
            fogblu = pstate->distance;
            break;

        case FSETFOGBOTTOMLEVEL:
            // This function sets the module's bottom fog level...
            fTmp = ( pstate->argument / 10.0f ) - fogbottom;
            fogbottom += fTmp;
            fogdistance -= fTmp;
            fogon = fogallowed;
            if ( fogdistance < 1.0f )  fogon = bfalse;

            break;

        case FGETFOGBOTTOMLEVEL:
            // This function gets the fog level
            pstate->argument = fogbottom * 10;
            break;

        case FCORRECTACTIONFORHAND:

            // This function turns ZA into ZA, ZB, ZC, or ZD...
            // tmpargument must be set to one of the A actions beforehand...
            if ( pchr->attachedto != MAXCHR )
            {
                if ( pchr->inwhichhand == SLOT_LEFT )
                {
                    // A or B
                    pstate->argument = pstate->argument + ( rand() & 1 );
                }
                else
                {
                    // C or D
                    pstate->argument = pstate->argument + 2 + ( rand() & 1 );
                }
            }
            break;

        case FIFTARGETISMOUNTED:
            // This function proceeds if the target is riding a mount
            returncode = bfalse;
            if ( ChrList[pself->target].attachedto != MAXCHR )
            {
                returncode = ChrList[ChrList[pself->target].attachedto].ismount;
            }
            break;

        case FSPARKLEICON:

            // This function makes a blippie thing go around the icon
            if ( pstate->argument < NUMBAR && pstate->argument > -1 )
            {
                pchr->sparkle = pstate->argument;
            }
            break;

        case FUNSPARKLEICON:
            // This function stops the blippie thing
            pchr->sparkle = NOSPARKLE;
            break;

        case FGETTILEXY:

            // This function gets the tile at x,y
            returncode = bfalse;
            iTmp = mesh_get_tile( pstate->x, pstate->y );
            if ( iTmp != INVALID_TILE )
            {
                returncode = btrue;
                pstate->argument = mesh.mem.tile_list[iTmp].img & 0xFF;
            }
            break;

        case FSETTILEXY:

            // This function changes the tile at x,y
            returncode = bfalse;
            iTmp = mesh_get_tile( pstate->x, pstate->y );
            if ( iTmp != INVALID_TILE )
            {
                returncode = btrue;
                mesh.mem.tile_list[iTmp].img = ( pstate->argument & 0xFF );
            }

            break;

        case FSETSHADOWSIZE:
            // This function changes a character's shadow size
            pchr->shadowsize = pstate->argument * pchr->fat;
            pchr->shadowsizesave = pstate->argument;
            break;

        case FORDERTARGET:
            // This function orders one specific character...  The target
            // Be careful in using this, always checking IDSZ first
            ChrList[pself->target].ai.order   = pstate->argument;
            ChrList[pself->target].ai.rank    = 0;
            ChrList[pself->target].ai.alert  |= ALERTIF_ORDERED;
            break;

        case FSETTARGETTOWHOEVERISINPASSAGE:
            // This function lets passage rectangles be used as event triggers
            sTmp = who_is_blocking_passage( pstate->argument );
            returncode = bfalse;
            if ( sTmp != MAXCHR )
            {
                pself->target = sTmp;
                returncode = btrue;
            }
            break;

        case FIFCHARACTERWASABOOK:
            // This function proceeds if the base model is the same as the current
            // model or if the base model is SPELLBOOK
            returncode = ( pchr->basemodel == SPELLBOOK ||
                           pchr->basemodel == pchr->model );
            break;

        case FSETENCHANTBOOSTVALUES:
            // This function sets the boost values for the last enchantment
            iTmp = pchr->undoenchant;
            if ( iTmp != MAXENCHANT )
            {
                EncList[iTmp].ownermana = pstate->argument;
                EncList[iTmp].ownerlife = pstate->distance;
                EncList[iTmp].targetmana = pstate->x;
                EncList[iTmp].targetlife = pstate->y;
            }
            break;

        case FSPAWNCHARACTERXYZ:
            // This function spawns a character, failing if x,y,z is invalid
            sTmp = spawn_one_character( pstate->x, pstate->y, pstate->distance, pchr->model, pchr->team, 0, pstate->turn & 0xFFFF, NULL, MAXCHR );
            returncode = bfalse;
            if ( sTmp < MAXCHR )
            {
                if ( __chrhitawall( sTmp ) )
                {
                    free_one_character( sTmp );
                    sTmp = MAXCHR;
                }
                else
                {
                    ChrList[sTmp].iskursed = bfalse;

                    pself->child = sTmp;

                    ChrList[sTmp].ai.passage = pself->passage;
                    ChrList[sTmp].ai.owner   = pself->owner;
                }
            }

            returncode = (sTmp != MAXCHR);
            break;

        case FSPAWNEXACTCHARACTERXYZ:
            // This function spawns a character ( specific model slot ),
            // failing if x,y,z is invalid
            sTmp = spawn_one_character( pstate->x, pstate->y, pstate->distance, pstate->argument, pchr->team, 0, pstate->turn & 0xFFFF, NULL, MAXCHR );
            returncode = bfalse;
            if ( sTmp < MAXCHR )
            {
                if ( __chrhitawall( sTmp ) )
                {
                    free_one_character( sTmp );
                    sTmp = MAXCHR;
                }
                else
                {
                    ChrList[sTmp].iskursed = bfalse;

                    pself->child = sTmp;

                    ChrList[sTmp].ai.passage = pself->passage;
                    ChrList[sTmp].ai.owner   = pself->owner;
                }
            }
            returncode = (sTmp != MAXCHR);
            break;

        case FCHANGETARGETCLASS:
            // This function changes a character's model ( specific model slot )
            change_character( pself->target, pstate->argument, 0, LEAVEALL );
            break;

        case FPLAYFULLSOUND:

            // This function plays a sound loud for everyone...  Victory music
            if ( moduleactive && pstate->argument >= 0 && pstate->argument < MAXWAVE )
            {
                sound_play_chunk( gCamera.trackx, gCamera.tracky, CapList[pchr->model].wavelist[pstate->argument] );
            }
            break;

        case FSPAWNEXACTCHASEPARTICLE:
            // This function spawns an exactly placed particle that chases the target
            tTmp = pself->index;
            if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

            tTmp = spawn_one_particle( pstate->x, pstate->y, pstate->distance, pchr->turnleftright, pchr->model, pstate->argument, MAXCHR, 0, pchr->team, tTmp, 0, MAXCHR );
            if ( tTmp < maxparticles )
            {
                PrtList[tTmp].target = pself->target;
            }
            returncode = (tTmp != TOTALMAXPRT);
            break;

        case FCREATEORDER:
            // This function packs up an order, using tmpx, tmpy, tmpargument and the
            // target ( if valid ) to create a new tmpargument
            sTmp = pself->target << 24;
            sTmp |= ( ( pstate->x >> 6 ) & 1023 ) << 14;
            sTmp |= ( ( pstate->y >> 6 ) & 1023 ) << 4;
            sTmp |= ( pstate->argument & 15 );
            pstate->argument = sTmp;
            break;

        case FORDERSPECIALID:
            // This function issues an order to all with the given special IDSZ
            issue_special_order( pstate->argument, pstate->distance );
            break;

        case FUNKURSETARGETINVENTORY:
            // This function unkurses every item a character is holding
            sTmp = ChrList[pself->target].holdingwhich[0];
            ChrList[sTmp].iskursed = bfalse;
            sTmp = ChrList[pself->target].holdingwhich[1];
            ChrList[sTmp].iskursed = bfalse;
            sTmp = ChrList[pself->target].nextinpack;

            while ( sTmp != MAXCHR )
            {
                ChrList[sTmp].iskursed = bfalse;
                sTmp = ChrList[sTmp].nextinpack;
            }
            break;

        case FIFTARGETISSNEAKING:
            // This function proceeds if the target is doing ACTIONDA or ACTIONWA
            returncode = ( ChrList[pself->target].action == ACTIONDA || ChrList[pself->target].action == ACTIONWA );
            break;

        case FDROPITEMS:
            // This function drops all of the character's items
            drop_all_items( pself->index );
            break;

        case FRESPAWNTARGET:
            // This function respawns the target at its current location
            sTmp = pself->target;
            ChrList[sTmp].oldx = ChrList[sTmp].xpos;
            ChrList[sTmp].oldy = ChrList[sTmp].ypos;
            ChrList[sTmp].oldz = ChrList[sTmp].zpos;
            respawn_character( sTmp );
            ChrList[sTmp].xpos = ChrList[sTmp].oldx;
            ChrList[sTmp].ypos = ChrList[sTmp].oldy;
            ChrList[sTmp].zpos = ChrList[sTmp].oldz;
            break;

        case FTARGETDOACTIONSETFRAME:
            // This function starts a new action, if it is valid for the model and
            // sets the starting frame.  It will fail if the action is invalid
            returncode = bfalse;
            if ( pstate->argument < MAXACTION )
            {
                if ( MadList[ChrList[pself->target].inst.imad].actionvalid[pstate->argument] )
                {
                    ChrList[pself->target].action = pstate->argument;
                    ChrList[pself->target].inst.lip = 0;
                    ChrList[pself->target].inst.frame = MadList[ChrList[pself->target].inst.imad].actionstart[pstate->argument];
                    ChrList[pself->target].inst.lastframe = ChrList[pself->target].inst.frame;
                    ChrList[pself->target].actionready = bfalse;
                    returncode = btrue;
                }
            }
            break;

        case FIFTARGETCANSEEINVISIBLE:
            // This function proceeds if the target can see invisible
            returncode = ChrList[pself->target].canseeinvisible;
            break;

        case FSETTARGETTONEARESTBLAHID:
            // This function finds the nearest target that meets the
            // requirements
            {
                TARGET_TYPE blahteam = NONE;
                returncode = bfalse;
                if ( ( pstate->distance >> 2 ) & 1 )  blahteam = FRIEND;
                if ( (( pstate->distance >> 1 ) & 1) && blahteam == FRIEND ) blahteam = ALL;
                else if ((( pstate->distance >> 1 ) & 1)) blahteam = ENEMY;
                if (blahteam != NONE)
                {
                    if (get_target(pself->index, NEAREST, blahteam, ( ( pstate->distance >> 3 ) & 1 ),
                                   ( ( pstate->distance ) & 1 ), pstate->argument, (( pstate->distance >> 4 ) & 1) ) != MAXCHR) returncode = btrue;
                }
            }
            break;

        case FSETTARGETTONEARESTENEMY:
            // This function finds the nearest target that meets the
            // requirements
            returncode = bfalse;
            if (get_target(pself->index, 0, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR) returncode = btrue;

            break;

        case FSETTARGETTONEARESTFRIEND:
            // This function finds the nearest target that meets the
            // requirements
            returncode = bfalse;
            if (get_target(pself->index, 0, FRIEND, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR) returncode = btrue;

            break;

        case FSETTARGETTONEARESTLIFEFORM:
            // This function finds the nearest target that meets the
            // requirements
            returncode = bfalse;
            if (get_target(pself->index, 0, ALL, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR) returncode = btrue;

            break;

        case FFLASHPASSAGE:
            // This function makes the passage light or dark...  For debug...
            flash_passage( pstate->argument, pstate->distance );
            break;

        case FFINDTILEINPASSAGE:
            // This function finds the next tile in the passage, tmpx and tmpy are
            // required and set on return
            returncode = find_tile_in_passage( pstate, pstate->argument, pstate->distance );
            break;

        case FIFHELDINLEFTHAND:
            // This function proceeds if the character is in the left hand of another
            // character
            returncode = bfalse;
            sTmp = pchr->attachedto;
            if ( sTmp != MAXCHR )
            {
                returncode = ( ChrList[sTmp].holdingwhich[0] == pself->index );
            }
            break;

        case FNOTANITEM:
            // This function makes the pself->index a non-item character
            pchr->isitem = bfalse;
            break;

        case FSETCHILDAMMO:
            // This function sets the child's ammo
            ChrList[pself->child].ammo = pstate->argument;
            break;

        case FIFHITVULNERABLE:
            // This function proceeds if the character was hit by a weapon with the
            // correct vulnerability IDSZ...  [SILV] for Werewolves...
            returncode = ( 0 != ( pself->alert & ALERTIF_HITVULNERABLE ) );
            break;

        case FIFTARGETISFLYING:
            // This function proceeds if the character target is flying
            returncode = ( ChrList[pself->target].flyheight > 0 );
            break;

        case FIDENTIFYTARGET:
            // This function reveals the target's name, ammo, and usage
            // Proceeds if the target was unknown
            returncode = bfalse;
            sTmp = pself->target;
            if ( ChrList[sTmp].ammomax != 0 )  ChrList[sTmp].ammoknown = btrue;
            if ( ChrList[sTmp].name[0] != 'B' ||
                    ChrList[sTmp].name[1] != 'l' ||
                    ChrList[sTmp].name[2] != 'a' ||
                    ChrList[sTmp].name[3] != 'h' ||
                    ChrList[sTmp].name[4] != 0 )
            {
                returncode = !ChrList[sTmp].nameknown;
                ChrList[sTmp].nameknown = btrue;
            }

            CapList[ChrList[sTmp].model].usageknown = btrue;
            break;

        case FBEATMODULE:
            // This function displays the Module Ended message
            beatmodule = btrue;
            break;

        case FENDMODULE:
            // This function presses the Escape key
            if ( NULL != keyb.state_ptr )
            {
                keyb.state_ptr[SDLK_ESCAPE] = 1;
            }
            break;

        case FDISABLEEXPORT:
            // This function turns export off
            exportvalid = bfalse;
            break;

        case FENABLEEXPORT:
            // This function turns export on
            exportvalid = btrue;
            break;

        case FGETTARGETSTATE:
            // This function sets tmpargument to the state of the target
            pstate->argument = ChrList[pself->target].ai.state;
            break;

        case FIFEQUIPPED:
            // This proceeds if the character is equipped
            returncode = bfalse;
            if ( pchr->isequipped ) returncode = btrue;

            break;

        case FDROPTARGETMONEY:
            // This function drops some of the target's money
            drop_money( pself->target, pstate->argument );
            break;

        case FGETTARGETCONTENT:
            // This sets tmpargument to the current target's content value
            pstate->argument = ChrList[pself->target].ai.content;
            break;

        case FDROPTARGETKEYS:
            // This function makes the target drops keys in inventory (Not inhand)
            drop_keys( pself->target );
            break;

        case FJOINTEAM:
            // This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)
            switch_team( pself->index, pstate->argument );
            break;

        case FTARGETJOINTEAM:
            // This makes the target join a team specified in tmpargument (A = 0, 23 = Z, etc.)
            switch_team( pself->target, pstate->argument );
            break;

        case FCLEARMUSICPASSAGE:
            // This clears the music for a specified passage
            passagemusic[pstate->argument] = -1;
            break;

        case FCLEARENDMESSAGE:
            // This function empties the end-module text buffer
            endtext[0] = 0;
            endtextwrite = 0;
            break;

        case FADDENDMESSAGE:
            // This function appends a message to the end-module text buffer
            append_end_text( pstate,  MadList[pchr->inst.imad].msgstart + pstate->argument, pself->index );
            break;

        case FPLAYMUSIC:

            // This function begins playing a new track of music
            if ( musicvalid && ( songplaying != pstate->argument ) )
            {
                sound_play_song( pstate->argument, pstate->distance, -1 );
            }
            break;

        case FSETMUSICPASSAGE:
            // This function makes the given passage play music if a player enters it
            // tmpargument is the passage to set and tmpdistance is the music track to play...
            passagemusic[pstate->argument] = pstate->distance;
            break;

        case FMAKECRUSHINVALID:
            // This function makes doors unable to close on this object
            pchr->canbecrushed = bfalse;
            break;

        case FSTOPMUSIC:
            // This function stops the interactive music
            sound_stop_song();
            break;

        case FFLASHVARIABLE:
            // This function makes the character flash according to tmpargument
            flash_character( pself->index, pstate->argument );
            break;

        case FACCELERATEUP:
            // This function changes the character's up down velocity
            pchr->zvel += pstate->argument / 100.0f;
            break;

        case FFLASHVARIABLEHEIGHT:
            // This function makes the character flash, feet one color, head another...
            flash_character_height( pself->index, pstate->turn & 0xFFFF, pstate->x,
                                    pstate->distance, pstate->y );
            break;

        case FSETDAMAGETIME:
            // This function makes the character invincible for a little while
            pchr->damagetime = pstate->argument;
            break;

        case FIFSTATEIS8:
            returncode = ( 8 == pself->state );
            break;

        case FIFSTATEIS9:
            returncode = ( 9 == pself->state );
            break;

        case FIFSTATEIS10:
            returncode = ( 10 == pself->state );
            break;

        case FIFSTATEIS11:
            returncode = ( 11 == pself->state );
            break;

        case FIFSTATEIS12:
            returncode = ( 12 == pself->state );
            break;

        case FIFSTATEIS13:
            returncode = ( 13 == pself->state );
            break;

        case FIFSTATEIS14:
            returncode = ( 14 == pself->state );
            break;

        case FIFSTATEIS15:
            returncode = ( 15 == pself->state );
            break;

        case FIFTARGETISAMOUNT:
            returncode = ChrList[pself->target].ismount;
            break;

        case FIFTARGETISAPLATFORM:
            returncode = ChrList[pself->target].platform;
            break;

        case FADDSTAT:
            if ( !pchr->staton ) statlist_add( pself->index );

            break;

        case FDISENCHANTTARGET:
            returncode = ( ChrList[pself->target].firstenchant != MAXENCHANT );
            disenchant_character( pself->target );
            break;

        case FDISENCHANTALL:

            iTmp = 0;
            while ( iTmp < MAXENCHANT )
            {
                remove_enchant( iTmp );
                iTmp++;
            }
            break;

        case FSETVOLUMENEARESTTEAMMATE:
            /*PORT
            if(moduleactive && pstate->distance >= 0)
            {
            // Find the closest teammate
            iTmp = 10000;
            sTmp = 0;
            while(sTmp < MAXCHR)
            {
            if(ChrList[sTmp].on && ChrList[sTmp].alive && ChrList[sTmp].team == pchr->team)
            {
            distance = ABS(gCamera.trackx-ChrList[sTmp].oldx)+ABS(gCamera.tracky-ChrList[sTmp].oldy);
            if(distance < iTmp)  iTmp = distance;
            }
            sTmp++;
            }
            distance=iTmp+pstate->distance;
            volume = -distance;
            volume = volume<<VOLSHIFT;
            if(volume < VOLMIN) volume = VOLMIN;
            iTmp = CapList[pchr->model].wavelist[pstate->argument];
            if(iTmp < numsound && iTmp >= 0 && soundon)
            {
            lpDSBuffer[iTmp]->SetVolume(volume);
            }
            }
            */
            break;

        case FADDSHOPPASSAGE:
            // This function defines a shop area
            add_shop_passage( pself->index, pstate->argument );
            break;

        case FTARGETPAYFORARMOR:
            // This function costs the target some money, or fails if 'e doesn't have
            // enough...
            // tmpx is amount needed
            // tmpy is cost of new skin
            sTmp = pself->target;   // The target
            tTmp = ChrList[sTmp].model;           // The target's model
            iTmp =  CapList[tTmp].skincost[pstate->argument&3];
            pstate->y = iTmp;                // Cost of new skin
            iTmp -= CapList[tTmp].skincost[ChrList[sTmp].skin];  // Refund
            if ( iTmp > ChrList[sTmp].money )
            {
                // Not enough...
                pstate->x = iTmp - ChrList[sTmp].money;  // Amount needed
                returncode = bfalse;
            }
            else
            {
                // Pay for it...  Cost may be negative after refund...
                ChrList[sTmp].money -= iTmp;
                if ( ChrList[sTmp].money > MAXMONEY )  ChrList[sTmp].money = MAXMONEY;

                pstate->x = 0;
                returncode = btrue;
            }
            break;

        case FJOINEVILTEAM:
            // This function adds the character to the evil team...
            switch_team( pself->index, EVILTEAM );
            break;

        case FJOINNULLTEAM:
            // This function adds the character to the null team...
            switch_team( pself->index, NULLTEAM );
            break;

        case FJOINGOODTEAM:
            // This function adds the character to the good team...
            switch_team( pself->index, GOODTEAM );
            break;

        case FPITSKILL:
            // This function activates pit deaths...
            pitskill = btrue;
            break;

        case FSETTARGETTOPASSAGEID:
            // This function finds a character who is both in the passage and who has
            // an item with the given IDSZ
            sTmp = who_is_blocking_passage_ID( pstate->argument, pstate->distance );
            returncode = bfalse;
            if ( sTmp != MAXCHR )
            {
                pself->target = sTmp;
                returncode = btrue;
            }
            break;

        case FMAKENAMEUNKNOWN:
            // This function makes the name of an item/character unknown.
            pchr->nameknown = bfalse;
            break;

        case FSPAWNEXACTPARTICLEENDSPAWN:
            // This function spawns a particle that spawns a character...
            tTmp = pself->index;
            if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

            tTmp = spawn_one_particle( pstate->x, pstate->y, pstate->distance, pchr->turnleftright, pchr->model, pstate->argument, MAXCHR, 0, pchr->team, tTmp, 0, MAXCHR );
            if ( tTmp != maxparticles )
            {
                PrtList[tTmp].spawncharacterstate = pstate->turn;
            }
            returncode = (tTmp != TOTALMAXPRT);
            break;

        case FSPAWNPOOFSPEEDSPACINGDAMAGE:
            // This function makes a lovely little poof at the character's location,
            // adjusting the xy speed and spacing and the base damage first
            // Temporarily adjust the values for the particle type
            sTmp = pchr->model;
            sTmp = MadList[sTmp].prtpip[CapList[sTmp].gopoofprttype];
            iTmp = PipList[sTmp].xyvelbase;
            tTmp = PipList[sTmp].xyspacingbase;
            test = PipList[sTmp].damagebase;
            PipList[sTmp].xyvelbase = pstate->x;
            PipList[sTmp].xyspacingbase = pstate->y;
            PipList[sTmp].damagebase = pstate->argument;
            spawn_poof( pself->index, pchr->model );
            // Restore the saved values
            PipList[sTmp].xyvelbase = iTmp;
            PipList[sTmp].xyspacingbase = tTmp;
            PipList[sTmp].damagebase = test;
            break;

        case FGIVEEXPERIENCETOGOODTEAM:
            // This function gives experience to everyone on the G Team
            give_team_experience( GOODTEAM, pstate->argument, pstate->distance );
            break;

        case FDONOTHING:
            // This function does nothing (For use with combination with Else function or debugging)
            break;

        case FGROGTARGET:
            // This function grogs the target for a duration equal to tmpargument
            ChrList[pself->target].grogtime += pstate->argument;
            break;

        case FDAZETARGET:
            // This function dazes the target for a duration equal to tmpargument
            ChrList[pself->target].dazetime += pstate->argument;
            break;

        case FENABLERESPAWN:
            // This function turns respawn with JUMP button on
            respawnvalid = btrue;
            break;

        case FDISABLERESPAWN:
            // This function turns respawn with JUMP button off
            respawnvalid = bfalse;
            break;

        case FIFHOLDERSCOREDAHIT:
            // Proceed only if the character's holder scored a hit
            returncode = bfalse;
            if ( 0 != ( ChrList[pchr->attachedto].ai.alert & ALERTIF_SCOREDAHIT ) )
            {
                returncode = btrue;
                pself->target = ChrList[pchr->attachedto].ai.hitlast;
            }
            break;

        case FIFHOLDERBLOCKED:
            // This function passes if the holder blocked an attack
            returncode = bfalse;
            if ( 0 != ( ChrList[pchr->attachedto].ai.alert & ALERTIF_BLOCKED ) )
            {
                returncode = btrue;
                pself->target = ChrList[pchr->attachedto].ai.attacklast;
            }
            break;

            //case FGETSKILLLEVEL:
            //        // This function sets tmpargument to the shield profiency level of the target
            //        pstate->argument = CapList[ChrList[ChrList[character].attachedto].model].shieldprofiency;
            //    break;

        case FIFTARGETHASNOTFULLMANA:

            // This function passes only if the target is not at max mana and alive
            if ( !ChrList[pself->target].alive || ChrList[pself->target].mana > ChrList[pself->target].manamax - HURTDAMAGE )
                returncode = bfalse;

            break;

        case FENABLELISTENSKILL:
            // This function increases sound play range by 25%
            local_listening = btrue;
            break;

        case FSETTARGETTOLASTITEMUSED:

            // This sets the target to the last item the character used
            if ( pself->lastitemused == pself->index ) returncode = bfalse;
            else pself->target = pself->lastitemused;

            break;

        case FFOLLOWLINK:
            {
                // Skips to the next module!
                Uint16 model;
                int message_number, message_index;
                char * ptext;

                model = pchr->model;

                message_number = MadList[model].msgstart + pstate->argument;

                message_index = msgindex[message_number];

                ptext = msgtext + message_index;

                // !!!use the message text to control the links!!!!
                returncode = link_follow_modname( ptext );

                if (!returncode)
                {
                    STRING tmpbuf;
                    snprintf(tmpbuf, sizeof(tmpbuf), "That's too scary for %s...", pchr->name );
                    debug_message(tmpbuf);
                }
            }
            break;

        case FIFOPERATORISLINUX:
            // Proceeds if running on linux
#ifdef __unix__
            returncode = btrue;
#else
            returncode = bfalse;
#endif
            break;

        case FIFTARGETISAWEAPON:
            // Proceeds if the AI target is a melee or ranged weapon
            sTmp = ChrList[pself->target].model;
            returncode = CapList[sTmp].isranged || (CapList[sTmp].weaponaction != ACTIONPA);
            break;

        case FIFSOMEONEISSTEALING:
            // This function passes if someone stealed from it's shop
            returncode = ( pself->order == STOLEN && pself->rank == 3 );
            break;

        case FIFTARGETISASPELL:
            // Proceeds if the AI target has any particle with the [IDAM] or [WDAM] expansion
            iTmp = 0;
            returncode = bfalse;

            while (iTmp < MAXPRTPIPPEROBJECT)
            {
                if (PipList[MadList[ChrList[pself->target].inst.imad].prtpip[iTmp]].intdamagebonus || PipList[MadList[ChrList[pself->target].inst.imad].prtpip[iTmp]].wisdamagebonus)
                {
                    returncode = btrue;
                    break;
                }

                iTmp++;
            }
            break;

        case FIFBACKSTABBED:
            // Proceeds if HitFromBehind, target has [DISA] skill and damage is physical
            returncode = bfalse;
            if ( 0 != ( pself->alert & ALERTIF_ATTACKED ) )
            {
                sTmp = ChrList[pself->attacklast].model;
                if ( pself->directionlast >= BEHIND - 8192 && pself->directionlast < BEHIND + 8192 )
                {
                    if ( CapList[sTmp].idsz[IDSZ_SKILL] == Make_IDSZ( "STAB" ) )
                    {
                        iTmp = pself->damagetypelast;
                        if ( iTmp == DAMAGE_CRUSH || iTmp == DAMAGE_POKE || iTmp == DAMAGE_SLASH ) returncode = btrue;
                    }
                }
            }
            break;

        case FGETTARGETDAMAGETYPE:
            // This function gets the last type of damage for the target
            pstate->argument = ChrList[pself->target].ai.damagetypelast;
            break;

        case FADDQUEST:

            //This function adds a quest idsz set in tmpargument into the targets quest.txt
            if ( ChrList[pself->target].isplayer )
            {
                add_quest_idsz( ChrList[pself->target].name, pstate->argument );
                returncode = btrue;
            }
            else returncode = bfalse;

            break;

        case FBEATQUESTALLPLAYERS:
            //This function marks a IDSZ in the targets quest.txt as beaten
            returncode = bfalse;
            iTmp = 0;

            while (iTmp < MAXCHR)
            {
                if ( ChrList[iTmp].isplayer )
                {
                    if (modify_quest_idsz( ChrList[iTmp].name, (IDSZ)pstate->argument, 0 ) == QUEST_BEATEN) returncode = btrue;
                }

                iTmp++;
            }
            break;

        case FIFTARGETHASQUEST:

            //This function proceeds if the target has the unfinished quest specified in tmpargument
            //and sets tmpdistance to the Quest Level of the specified quest.
            if ( ChrList[pself->target].isplayer )
            {
                iTmp = check_player_quest( ChrList[pself->target].name, pstate->argument );
                if ( iTmp > QUEST_BEATEN )
                {
                    returncode = btrue;
                    pstate->distance = iTmp;
                }
                else returncode = bfalse;
            }
            break;

        case FSETQUESTLEVEL:
            //This function modifies the quest level for a specific quest IDSZ
            //tmpargument specifies quest idsz and tmpdistance the adjustment (which may be negative)
            returncode = bfalse;
            if ( ChrList[pself->target].isplayer && pstate->distance != 0 )
            {
                if (modify_quest_idsz( ChrList[pself->target].name, pstate->argument, pstate->distance ) > QUEST_NONE) returncode = btrue;
            }
            break;

        case FADDQUESTALLPLAYERS:
            //This function adds a quest idsz set in tmpargument into all local player's quest logs
            //The quest level is set to tmpdistance if the level is not already higher or QUEST_BEATEN
            iTmp = 0;
            returncode = bfalse;

            while (iTmp < MAXPLAYER)
            {
                if ( ChrList[PlaList[iTmp].index].isplayer )
                {
                    returncode = btrue;
                    if (!add_quest_idsz(ChrList[PlaList[iTmp].index].name , pstate->argument ))       //Try to add it if not already there or beaten
                    {
                        Sint16 i;
                        i = check_player_quest( ChrList[PlaList[iTmp].index].name, pstate->argument);   //Get the current quest level
                        if (i < 0 || i >= pstate->distance) returncode = bfalse;      //It was already beaten
                        else modify_quest_idsz( ChrList[PlaList[iTmp].index].name, pstate->argument, pstate->distance );//Not beaten yet, increase level by 1
                    }
                }

                iTmp++;
            }
            break;

        case FADDBLIPALLENEMIES:
            // show all enemies on the minimap who match the IDSZ given in tmpargument
            // it show only the enemies of the AI target
            local_senseenemies = pself->target;
            local_senseenemiesID = pstate->argument;
            break;

        case FPITSFALL:

            // This function activates pit teleportation...
            if ( pstate->x > EDGE && pstate->y > EDGE && pstate->x < mesh.info.edge_x - EDGE && pstate->y < mesh.info.edge_y - EDGE )
            {
                pitsfall = btrue;
                pitx = pstate->x;
                pity = pstate->y;
                pitz = pstate->distance;
            }
            else pitskill = btrue;

            break;

        case FIFTARGETISOWNER:
            // This function proceeds only if the target is on another team
            returncode = ( ChrList[pself->target].alive && pself->owner == pself->target );
            break;

        case FEND:
            pself->terminate = btrue;
            returncode       = bfalse;
            break;

        case FTAKEPICTURE:
            // This function proceeds only if the screenshot was successful
            returncode = dump_screenshot();
            break;

        case FIFOPERATORISMACINTOSH:
            // Proceeds if running on mac
#ifdef __APPLE__
            returncode = btrue;
#else
            returncode = bfalse;
#endif
            break;

            // If none of the above, skip the line and log an error
        default:
            log_message( "SCRIPT ERROR: run_function() - ai script %d - unhandled script function %d\n", pself->type, valuecode );
            returncode = bfalse;
            break;
    }

    return returncode;
}

//--------------------------------------------------------------------------------------------
void set_operand( script_state_t * pstate, Uint8 variable )
{
    // ZZ> This function sets one of the tmp* values for scripted AI
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
            log_warning("set_operand() - cannot assign a number to index %d", variable );
            break;
    }
}

//--------------------------------------------------------------------------------------------
void run_operand( script_state_t * pstate, ai_state_t * pself )
{
    // ZZ> This function does the scripted arithmetic in OPERATOR, OPERAND pairs

    char * varname, * op;

    STRING buffer = { '\0' };
    Uint8  variable;
    Uint8  operation;

    Uint32 iTmp;

    // get the operator
    iTmp      = 0;
    varname   = buffer;
    operation = GET_DATA_BITS( pself->opcode );
    if ( 0 != (pself->opcode & FUNCTION_BIT) )
    {
        // Get the working opcode from a constant, constants are all but high 5 bits
        iTmp = pself->opcode & VALUE_BITS;
        if ( debug_scripts ) snprintf( buffer, sizeof(STRING), "%d", iTmp );
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
                varname = "TMPdistance";
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
                iTmp = ChrList[pself->index].xpos;
                break;

            case VARSELFY:
                varname = "SELFY";
                iTmp = ChrList[pself->index].ypos;
                break;

            case VARSELFTURN:
                varname = "SELFTURN";
                iTmp = ChrList[pself->index].turnleftright;
                break;

            case VARSELFCOUNTER:
                varname = "SELFCOUNTER";
                iTmp = pself->rank;
                break;

            case VARSELFORDER:
                varname = "SELFORDER";
                iTmp = pself->order;
                break;

            case VARSELFMORALE:
                varname = "SELFMORALE";
                iTmp = TeamList[ChrList[pself->index].baseteam].morale;
                break;

            case VARSELFLIFE:
                varname = "SELFLIFE";
                iTmp = ChrList[pself->index].life;
                break;

            case VARTARGETX:
                varname = "TARGETX";
                iTmp = ChrList[pself->target].xpos;
                break;

            case VARTARGETY:
                varname = "TARGETY";
                iTmp = ChrList[pself->target].ypos;
                break;

            case VARTARGETDISTANCE:
                varname = "TARGETdistance";
                iTmp = ABS( ( int )( ChrList[pself->target].xpos - ChrList[pself->index].xpos ) ) +
                       ABS( ( int )( ChrList[pself->target].ypos - ChrList[pself->index].ypos ) );
                break;

            case VARTARGETTURN:
                varname = "TARGETTURN";
                iTmp = ChrList[pself->target].turnleftright;
                break;

            case VARLEADERX:
                varname = "LEADERX";
                iTmp = ChrList[pself->index].xpos;
                if ( TeamList[ChrList[pself->index].team].leader != NOLEADER )
                    iTmp = ChrList[TeamList[ChrList[pself->index].team].leader].xpos;

                break;

            case VARLEADERY:
                varname = "LEADERY";
                iTmp = ChrList[pself->index].ypos;
                if ( TeamList[ChrList[pself->index].team].leader != NOLEADER )
                    iTmp = ChrList[TeamList[ChrList[pself->index].team].leader].ypos;

                break;

            case VARLEADERDISTANCE:
                varname = "LEADERdistance";
                iTmp = 10000;
                if ( TeamList[ChrList[pself->index].team].leader != NOLEADER )
                    iTmp = ABS( ( int )( ChrList[TeamList[ChrList[pself->index].team].leader].xpos - ChrList[pself->index].xpos ) ) +
                           ABS( ( int )( ChrList[TeamList[ChrList[pself->index].team].leader].ypos - ChrList[pself->index].ypos ) );

                break;

            case VARLEADERTURN:
                varname = "LEADERTURN";
                iTmp = ChrList[pself->index].turnleftright;
                if ( TeamList[ChrList[pself->index].team].leader != NOLEADER )
                    iTmp = ChrList[TeamList[ChrList[pself->index].team].leader].turnleftright;

                break;

            case VARGOTOX:
                varname = "GOTOX";
                if (pself->wp_tail == pself->wp_head)
                {
                    iTmp = ChrList[pself->index].xpos;
                }
                else
                {
                    iTmp = pself->wp_pos_x[pself->wp_tail];
                }
                break;

            case VARGOTOY:
                varname = "GOTOY";
                if (pself->wp_tail == pself->wp_head)
                {
                    iTmp = ChrList[pself->index].ypos;
                }
                else
                {
                    iTmp = pself->wp_pos_y[pself->wp_tail];
                }
                break;

            case VARGOTODISTANCE:
                varname = "GOTOdistance";
                if (pself->wp_tail == pself->wp_head)
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = ABS( ( int )( pself->wp_pos_x[pself->wp_tail] - ChrList[pself->index].xpos ) ) +
                           ABS( ( int )( pself->wp_pos_y[pself->wp_tail] - ChrList[pself->index].ypos ) );
                }
                break;

            case VARTARGETTURNTO:
                varname = "TARGETTURNTO";
                iTmp = ATAN2( ChrList[pself->target].ypos - ChrList[pself->index].ypos, ChrList[pself->target].xpos - ChrList[pself->index].xpos ) * 0xFFFF / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 0xFFFF;
                break;

            case VARPASSAGE:
                varname = "PASSAGE";
                iTmp = pself->passage;
                break;

            case VARWEIGHT:
                varname = "WEIGHT";
                iTmp = ChrList[pself->index].holdingweight;
                break;

            case VARSELFALTITUDE:
                varname = "SELFALTITUDE";
                iTmp = ChrList[pself->index].zpos - ChrList[pself->index].level;
                break;

            case VARSELFID:
                varname = "SELFID";
                iTmp = CapList[ChrList[pself->index].model].idsz[IDSZ_TYPE];
                break;

            case VARSELFHATEID:
                varname = "SELFHATEID";
                iTmp = CapList[ChrList[pself->index].model].idsz[IDSZ_HATE];
                break;

            case VARSELFMANA:
                varname = "SELFMANA";
                iTmp = ChrList[pself->index].mana;
                if ( ChrList[pself->index].canchannel )  iTmp += ChrList[pself->index].life;

                break;

            case VARTARGETSTR:
                varname = "TARGETSTR";
                iTmp = ChrList[pself->target].strength;
                break;

            case VARTARGETWIS:
                varname = "TARGETWIS";
                iTmp = ChrList[pself->target].wisdom;
                break;

            case VARTARGETINT:
                varname = "TARGETINT";
                iTmp = ChrList[pself->target].intelligence;
                break;

            case VARTARGETDEX:
                varname = "TARGETDEX";
                iTmp = ChrList[pself->target].dexterity;
                break;

            case VARTARGETLIFE:
                varname = "TARGETLIFE";
                iTmp = ChrList[pself->target].life;
                break;

            case VARTARGETMANA:
                varname = "TARGETMANA";
                iTmp = ChrList[pself->target].mana;
                if ( ChrList[pself->target].canchannel )  iTmp += ChrList[pself->target].life;

                break;

            case VARTARGETLEVEL:
                varname = "TARGETLEVEL";
                iTmp = ChrList[pself->target].experiencelevel;
                break;

            case VARTARGETSPEEDX:
                varname = "TARGETSPEEDX";
                iTmp = ChrList[pself->target].xvel;
                break;

            case VARTARGETSPEEDY:
                varname = "TARGETSPEEDY";
                iTmp = ChrList[pself->target].yvel;
                break;

            case VARTARGETSPEEDZ:
                varname = "TARGETSPEEDZ";
                iTmp = ChrList[pself->target].zvel;
                break;

            case VARSELFSPAWNX:
                varname = "SELFSPAWNX";
                iTmp = ChrList[pself->index].xstt;
                break;

            case VARSELFSPAWNY:
                varname = "SELFSPAWNY";
                iTmp = ChrList[pself->index].ystt;
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
                iTmp = ChrList[pself->index].strength;
                break;

            case VARSELFWIS:
                varname = "SELFWIS";
                iTmp = ChrList[pself->index].wisdom;
                break;

            case VARSELFINT:
                varname = "SELFINT";
                iTmp = ChrList[pself->index].intelligence;
                break;

            case VARSELFDEX:
                varname = "SELFDEX";
                iTmp = ChrList[pself->index].dexterity;
                break;

            case VARSELFMANAFLOW:
                varname = "SELFMANAFLOW";
                iTmp = ChrList[pself->index].manaflow;
                break;

            case VARTARGETMANAFLOW:
                varname = "TARGETMANAFLOW";
                iTmp = ChrList[pself->target].manaflow;
                break;

            case VARSELFATTACHED:
                varname = "SELFATTACHED";
                iTmp = number_of_attached_particles( pself->index );
                break;

            case VARSWINGTURN:
                varname = "SWINGTURN";
                iTmp = gCamera.swing << 2;
                break;

            case VARXYDISTANCE:
                varname = "XYdistance";
                iTmp = SQRT( pstate->x * pstate->x + pstate->y * pstate->y );
                break;

            case VARSELFZ:
                varname = "SELFZ";
                iTmp = ChrList[pself->index].zpos;
                break;

            case VARTARGETALTITUDE:
                varname = "TARGETALTITUDE";
                iTmp = ChrList[pself->target].zpos - ChrList[pself->target].level;
                break;

            case VARTARGETZ:
                varname = "TARGETZ";
                iTmp = ChrList[pself->target].zpos;
                break;

            case VARSELFINDEX:
                varname = "SELFINDEX";
                iTmp = pself->index;
                break;

            case VAROWNERX:
                varname = "OWNERX";
                iTmp = ChrList[pself->owner].xpos;
                break;

            case VAROWNERY:
                varname = "OWNERY";
                iTmp = ChrList[pself->owner].ypos;
                break;

            case VAROWNERTURN:
                varname = "OWNERTURN";
                iTmp = ChrList[pself->owner].turnleftright;
                break;

            case VAROWNERDISTANCE:
                varname = "OWNERdistance";
                iTmp = ABS( ( int )( ChrList[pself->owner].xpos - ChrList[pself->index].xpos ) ) +
                       ABS( ( int )( ChrList[pself->owner].ypos - ChrList[pself->index].ypos ) );
                break;

            case VAROWNERTURNTO:
                varname = "OWNERTURNTO";
                iTmp = ATAN2( ChrList[pself->owner].ypos - ChrList[pself->index].ypos, ChrList[pself->owner].xpos - ChrList[pself->index].xpos ) * 0xFFFF / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 0xFFFF;
                break;

            case VARXYTURNTO:
                varname = "XYTURNTO";
                iTmp = ATAN2( pstate->y - ChrList[pself->index].ypos, pstate->x - ChrList[pself->index].xpos ) * 0xFFFF / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 0xFFFF;
                break;

            case VARSELFMONEY:
                varname = "SELFMONEY";
                iTmp = ChrList[pself->index].money;
                break;

            case VARSELFACCEL:
                varname = "SELFACCEL";
                iTmp = ( ChrList[pself->index].maxaccel * 100.0f );
                break;

            case VARTARGETEXP:
                varname = "TARGETEXP";
                iTmp = ChrList[pself->target].experience;
                break;

            case VARSELFAMMO:
                varname = "SELFAMMO";
                iTmp = ChrList[pself->index].ammo;
                break;

            case VARTARGETAMMO:
                varname = "TARGETAMMO";
                iTmp = ChrList[pself->target].ammo;
                break;

            case VARTARGETMONEY:
                varname = "TARGETMONEY";
                iTmp = ChrList[pself->target].money;
                break;

            case VARTARGETTURNAWAY:
                varname = "TARGETTURNAWAY";
                iTmp = ATAN2( ChrList[pself->target].ypos - ChrList[pself->index].ypos, ChrList[pself->target].xpos - ChrList[pself->index].xpos ) * 0xFFFF / ( TWO_PI );
                iTmp += 32768;
                iTmp &= 0xFFFF;
                break;

            case VARSELFLEVEL:
                varname = "SELFLEVEL";
                iTmp = ChrList[pself->index].experiencelevel;
                break;

            case VARTARGETRELOADTIME:
                varname = "TARGETRELOADTIME";
                iTmp = ChrList[pself->target].reloadtime;
                break;

            default:
                log_message( "SCRIPT ERROR: run_operand() - model == %d, class name == \"%s\" - Unknown variable found!\n", script_error_model, script_error_classname );
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
                pstate->operationsum /= iTmp;
            }
            else
            {
                log_message( "SCRIPT ERROR: run_operand() - model == %d, class name == \"%s\" - Cannot divide by zero!\n", script_error_model, script_error_classname );
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
                log_message( "SCRIPT ERROR: run_operand() - model == %d, class name == \"%s\" - Cannot modulo by zero!\n", script_error_model, script_error_classname );
            }
            break;

        default:
            log_message( "SCRIPT ERROR: run_operand() - model == %d, class name == \"%s\" - unknown op\n", script_error_model, script_error_classname );
            break;
    }

    if ( debug_scripts )
    {
        FILE * scr_file = (NULL == debug_script_file) ? stdout : debug_script_file;
        fprintf( scr_file, "%s %s(%d) ", op, varname, iTmp );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// About half of the way to some kind of Lua integration
// the functions below should be the stub functions for calling some "real" functions.
// The stub functions will be the wrappers for the current scripting system and the "real" functions
// will be wrapped for use by some scripting system using SWIG or some equivalent

// turn off this annoying warning
#if defined _MSC_VER
#    pragma warning(disable : 4189) // local variable is initialized but not referenced
#endif

#define SCRIPT_FUNCTION_BEGIN() \
    chr_t * pchr; \
    Uint16 sTmp = 0; \
    Uint8 returncode = 1; \
    if( NULL == pstate || NULL == pself || pself->index >= MAXCHR || !ChrList[pself->index].on ) return 0;\
    pchr = ChrList + pself->index;

#define SCRIPT_FUNCTION_END() \
    return returncode;

//--------------------------------------------------------------------------------------------
Uint8 scr_set_AlertBit( script_state_t * pstate, ai_state_t * pself )
{
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
Uint8 scr_ClearAlertBit( script_state_t * pstate, ai_state_t * pself )
{
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
Uint8 scr_TestAlertBit( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument >= 0 && pstate->argument < 32 )
    {
        returncode = ( 0 != ( pself->alert & ( 1 << pstate->argument ) ) );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Alert( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    pself->alert |= pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearAlert( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    pself->alert &= ~pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TestAlert( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 != ( pself->alert & pstate->argument ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Bit( script_state_t * pstate, ai_state_t * pself )
{
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
Uint8 scr_ClearBit( script_state_t * pstate, ai_state_t * pself )
{
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
Uint8 scr_TestBit( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->y >= 0 && pstate->y < 32 )
    {
        returncode = ( 0 != ( pstate->x & ( 1 << pstate->y ) ) );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Bits( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    pstate->x |= pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearBits( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    pstate->x &= ~pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TestBits( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 != ( pstate->x & pstate->y ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Spawned( script_state_t * pstate, ai_state_t * pself )
{
    // ZZ> This function proceeds if the character was spawned this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if it's a new character
    returncode = ( 0 != ( pself->alert & ALERTIF_SPAWNED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TimeOut( script_state_t * pstate, ai_state_t * pself )
{
    // This function proceeds if the character's aitime is 0...  Use
    // in conjunction with set_Time

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if time alert is set
    returncode = ( frame_wld > pself->timer );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AtWaypoint( script_state_t * pstate, ai_state_t * pself )
{
    // This function proceeds if the character reached its waypoint this
    // update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character reached a waypoint
    returncode = ( 0 != ( pself->alert & ALERTIF_ATWAYPOINT ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AtLastWaypoint( script_state_t * pstate, ai_state_t * pself )
{
    // This function proceeds if the character reached its last waypoint this
    // update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character reached its last waypoint
    returncode = ( 0 != ( pself->alert & ALERTIF_ATLASTWAYPOINT ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Attacked( script_state_t * pstate, ai_state_t * pself )
{
    // This function proceeds if the character ( an item ) was put in its
    // owner's pocket this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was damaged
    returncode = ( 0 != ( pself->alert & ALERTIF_ATTACKED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Bumped( script_state_t * pstate, ai_state_t * pself )
{
    // This function proceeds if the character was bumped by another character
    // this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was bumped
    returncode = ( 0 != ( pself->alert & ALERTIF_BUMPED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Ordered( script_state_t * pstate, ai_state_t * pself )
{
    // This function proceeds if the character got an order from another
    // character on its team this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was ordered
    returncode = ( 0 != ( pself->alert & ALERTIF_ORDERED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CalledForHelp( script_state_t * pstate, ai_state_t * pself )
{
    // This function proceeds if one of the character's teammates was nearly
    // killed this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was called for help
    returncode = ( 0 != ( pself->alert & ALERTIF_CALLEDFORHELP ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Content( script_state_t * pstate, ai_state_t * pself )
{
    // set_Content( tmpargument )
    // This function sets the content variable...  Used in conjunction with
    // get_Content...  Content is preserved from update to update

    SCRIPT_FUNCTION_BEGIN();

    // set_ the content
    pself->content = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Killed( script_state_t * pstate, ai_state_t * pself )
{
    // This function proceeds if the character was killed this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character's been killed
    returncode = ( 0 != ( pself->alert & ALERTIF_KILLED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetKilled( script_state_t * pstate, ai_state_t * pself )
{
    // This function proceeds if the character's target from last update was
    // killed during this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character's target has just died
    returncode = ( 0 != ( pself->alert & ALERTIF_TARGETKILLED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearWaypoints( script_state_t * pstate, ai_state_t * pself )
{
    // This function is used to move a character around...  Do this before
    // AddWaypoint

    SCRIPT_FUNCTION_BEGIN();

    // Clear out all waypoints
    pself->wp_tail = 0;
    pself->wp_head = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddWaypoint( script_state_t * pstate, ai_state_t * pself )
{
    // AddWaypoint( tmpx, tmpy )
    // This function tells the character where to move next

    SCRIPT_FUNCTION_BEGIN();

    // Add a waypoint to the waypoint list
    pself->wp_pos_x[pself->wp_head] = pstate->x;
    pself->wp_pos_y[pself->wp_head] = pstate->y;

    pself->wp_head++;
    if ( pself->wp_head > MAXWAY - 1 )  pself->wp_head = MAXWAY - 1;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FindPath( script_state_t * pstate, ai_state_t * pself )
{
    // This function doesn't work yet !!!BAD!!!

    SCRIPT_FUNCTION_BEGIN();

    // Yep this is it
    if ( ChrList[ pself->target ].model != pself->index )
    {
        if ( pstate->distance != MOVE_FOLLOW )
        {
            pstate->x = ChrList[ ChrList[ ChrList[pself->target].model ].ai.target ].xpos;
            pstate->y = ChrList[ ChrList[ ChrList[pself->target].model ].ai.target ].ypos;
        }
        else
        {
            pstate->x = ( rand() & 1023 ) - 512 + ChrList[ ChrList[ ChrList[pself->target].model ].ai.target ].xpos;
            pstate->y = ( rand() & 1023 ) - 512 + ChrList[ ChrList[ ChrList[pself->target].model ].ai.target ].ypos;
        }

        pstate->turn = ATAN2( ChrList[pself->target].ypos - pchr->ypos, ChrList[pself->target].xpos - pchr->xpos ) * 0xFFFF / ( TWO_PI );
        if ( pstate->distance == MOVE_RETREAT )
        {
            pstate->turn += ( rand() & 16383 ) - 8192;
        }
        else
        {
            pstate->turn += 32768;
        }
        pstate->turn &= 0xFFFF;
        if ( pstate->distance == MOVE_CHARGE || pstate->distance == MOVE_RETREAT )
        {
            reset_character_accel( pself->index ); // Force 100% speed
        }

        // Secondly we run the Compass function (If we are not in follow mode)
        if ( pstate->distance != MOVE_FOLLOW )
        {
            pstate->x = pstate->x - turntocos[(pstate->turn & 0xFFFF)>>2] * pstate->distance;
            pstate->y = pstate->y - turntosin[(pstate->turn & 0xFFFF)>>2] * pstate->distance;
        }

        // Then we add the waypoint(s), without clearing existing ones...
        pself->wp_pos_x[pself->wp_head] = pstate->x;
        pself->wp_pos_y[pself->wp_head] = pstate->y;

        pself->wp_head++;
        if ( pself->wp_head > MAXWAY - 1 ) pself->wp_head = MAXWAY - 1;

    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Compass( script_state_t * pstate, ai_state_t * pself )
{
    // Compass( tmpx, tmpy, tmpturn, tmpdistance )
    // This function modifies tmpx and tmpy, depending on the setting of
    // tmpdistance and tmpturn.  It acts like one of those Compass thing
    // with the two little needle legs

    SCRIPT_FUNCTION_BEGIN();

    pstate->x -= turntocos[(pstate->turn & 0xFFFF)>>2] * pstate->distance;
    pstate->y -= turntosin[(pstate->turn & 0xFFFF)>>2] * pstate->distance;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetArmorPrice( script_state_t * pstate, ai_state_t * pself )
{
    // get_TargetArmorPrice( tmpargument )
    // This function returns the cost of the desired skin upgrade, setting
    // tmpx to the price

    SCRIPT_FUNCTION_BEGIN();

    // This function gets the armor cost for the given skin
    sTmp = pstate->argument % MAXSKIN;
    pstate->x = CapList[ChrList[pself->target].model].skincost[sTmp];

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Time( script_state_t * pstate, ai_state_t * pself )
{
    // set_Time( tmpargument )
    // This function sets the character's time...  50 clicks per second...
    // Used in conjunction with _TimeOut

    SCRIPT_FUNCTION_BEGIN();

    // This function resets the time
    if ( pstate->argument > -1 )
    {
        pself->timer = frame_wld + pstate->argument;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_Content( script_state_t * pstate, ai_state_t * pself )
{
    // get_Content()
    // This function sets tmpargument to the character's content variable...
    // Used in conjunction with set_Content, or as a NOP to space out an Else

    SCRIPT_FUNCTION_BEGIN();

    // get_ the content
    pstate->argument = pself->content;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinTargetTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinTargetTeam()
    // This function lets a character join a different team...  Used
    // mostly for pets

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ChrList[pself->target].on )
    {
        switch_team( pself->index, ChrList[pself->target].team );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearbyEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToNearbyEnemy()
    // This function sets the target to a nearby enemy, failing if there are none

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if (get_target( pself->index, NEARBY, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse) != MAXCHR) returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetLeftHand( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToTargetLeftHand()
    // This function sets the target to the item in the target's left hand,
    // failing if the target has no left hand item

    SCRIPT_FUNCTION_BEGIN();

    sTmp = ChrList[pself->target].holdingwhich[0];
    returncode = bfalse;
    if ( sTmp != MAXCHR )
    {
        pself->target = sTmp;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetRightHand( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToTargetRightHand()
    // This function sets the target to the item in the target's right hand,
    // failing if the target has no right hand item

    SCRIPT_FUNCTION_BEGIN();

    sTmp = ChrList[pself->target].holdingwhich[1];
    returncode = bfalse;
    if ( sTmp != MAXCHR )
    {
        pself->target = sTmp;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverAttacked( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToWhoeverAttacked()
    // This function sets the target to whoever attacked the character last, failing for damage tiles

    SCRIPT_FUNCTION_BEGIN();
    if ( pself->attacklast != MAXCHR )
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
    // set_TargetToWhoeverBumped()
    // This function sets the target to whoever bumped the character last. It never fails

    SCRIPT_FUNCTION_BEGIN();

    pself->target = pself->bumplast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverCalledForHelp( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToWhoeverCalledForHelp()
    // This function sets the target to whoever called for help last...

    SCRIPT_FUNCTION_BEGIN();

    pself->target = TeamList[pchr->team].sissy;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToOldTarget( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToOldTarget( tmpargument )
    // This function sets the target to the target from last update, used to
    // undo other set_Target functions

    SCRIPT_FUNCTION_BEGIN();

    pself->target = pself->target_old;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToVelocity( script_state_t * pstate, ai_state_t * pself )
{
    // set_TurnModeToVelocity()
    // This function sets the character's movement mode to the default

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODEVELOCITY;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToWatch( script_state_t * pstate, ai_state_t * pself )
{
    // set_TurnModeToWatch()
    // This function makes the character look at its next waypoint, usually
    // used with close waypoints or the Stop function

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODEWATCH;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToSpin( script_state_t * pstate, ai_state_t * pself )
{
    // set_TurnModeToSpin()
    // This function makes the character spin around in a circle, usually
    // used for magical items and such

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the turn mode
    pchr->turnmode = TURNMODESPIN;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BumpHeight( script_state_t * pstate, ai_state_t * pself )
{
    // set_BumpHeight( tmpargument )
    // This function makes the character taller or shorter, usually used when
    // the character dies

    SCRIPT_FUNCTION_BEGIN();

    pchr->bumpheight = pstate->argument * pchr->fat;
    pchr->bumpheightsave = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasID( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetHasID( tmpargument )
    // This function proceeds if the target has either a parent or type IDSZ
    // matching tmpargument...

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if ID matches tmpargument
    sTmp = ChrList[pself->target].model;
    returncode = CapList[sTmp].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument;
    returncode = returncode | ( CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasItemID( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetHasItemID( tmpargument )
    // This function proceeds if the target has a matching item in his/her
    // pockets or hands.

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    // Check the pack
    sTmp = ChrList[pself->target].nextinpack;
    while ( sTmp != MAXCHR )
    {
        if ( CapList[ChrList[sTmp].model].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument || CapList[ChrList[sTmp].model].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
        {
            returncode = btrue;
            sTmp = MAXCHR;
        }
        else
        {
            sTmp = ChrList[sTmp].nextinpack;
        }
    }

    // Check left hand
    sTmp = ChrList[pself->target].holdingwhich[0];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
            returncode = btrue;
    }

    // Check right hand
    sTmp = ChrList[pself->target].holdingwhich[1];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
            returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHoldingItemID( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetHoldingItemID( tmpargument )
    // This function proceeds if the target has a matching item in his/her
    // hands.  It also sets tmpargument to the proper latch button to press
    // to use that item

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    // Check left hand
    sTmp = ChrList[pself->target].holdingwhich[0];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
        {
            pstate->argument = LATCHBUTTON_LEFT;
            returncode = btrue;
        }
    }

    // Check right hand
    sTmp = ChrList[pself->target].holdingwhich[1];
    if ( sTmp != MAXCHR && !returncode )
    {
        sTmp = ChrList[sTmp].model;
        if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
        {
            pstate->argument = LATCHBUTTON_RIGHT;
            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasSkillID( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if ID matches tmpargument
    returncode = bfalse;

    returncode = (0 != check_skills( pself->target, ( IDSZ )pstate->argument ));

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Else( script_state_t * pstate, ai_state_t * pself )
{
    // Else()
    // This function proceeds if the last function failed...  Need padding
    // before it...  See the above function for an example...

    SCRIPT_FUNCTION_BEGIN();

    // This function fails if the last function was more indented
    returncode = ( pself->indent >= pself->indent_last );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Run( script_state_t * pstate, ai_state_t * pself )
{
    // Run()
    // This function sets the character's maximum acceleration to its
    // actual maximum

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Walk( script_state_t * pstate, ai_state_t * pself )
{
    // Walk()
    // This function sets the character's maximum acceleration to 66%
    // of its actual maximum

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );
    pchr->maxaccel *= 0.66f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Sneak( script_state_t * pstate, ai_state_t * pself )
{
    // Sneak()
    // This function sets the character's maximum acceleration to 33%
    // of its actual maximum

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );
    pchr->maxaccel *= 0.33f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoAction( script_state_t * pstate, ai_state_t * pself )
{
    // DoAction( tmpargument )
    // This function makes the character do a given action if it isn't doing
    // anything better.  Fails if the action is invalid or if the character is doing
    // something else already

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument < MAXACTION && pchr->actionready )
    {
        if ( MadList[pchr->inst.imad].actionvalid[pstate->argument] )
        {
            pchr->action = pstate->argument;
            pchr->inst.lip = 0;
            pchr->inst.lastframe = pchr->inst.frame;
            pchr->inst.frame = MadList[pchr->inst.imad].actionstart[pstate->argument];
            pchr->actionready = bfalse;
            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KeepAction( script_state_t * pstate, ai_state_t * pself )
{
    // KeepAction()
    // This function makes the character's animation stop on its last frame
    // and stay there...  Usually used for dropped items

    SCRIPT_FUNCTION_BEGIN();

    pchr->keepaction = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IssueOrder( script_state_t * pstate, ai_state_t * pself )
{
    // IssueOrder( tmpargument  )
    // This function tells all of the character's teammates to do something,
    // though each teammate needs to interpret the order using _Ordered in
    // its own script...

    SCRIPT_FUNCTION_BEGIN();

    // This function issues an order to all teammates
    issue_order( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropWeapons( script_state_t * pstate, ai_state_t * pself )
{
    // DropWeapons()
    // This function drops the character's in-hand items...  It will also
    // buck the rider if the character is a mount

    SCRIPT_FUNCTION_BEGIN();

    // This funtion drops the character's in hand items/riders
    sTmp = pchr->holdingwhich[0];
    if ( sTmp != MAXCHR )
    {
        detach_character_from_mount( sTmp, btrue, btrue );
        if ( pchr->ismount )
        {
            ChrList[sTmp].zvel = DISMOUNTZVEL;
            ChrList[sTmp].zpos += DISMOUNTZVEL;
            ChrList[sTmp].jumptime = JUMPDELAY;
        }
    }

    sTmp = pchr->holdingwhich[1];
    if ( sTmp != MAXCHR )
    {
        detach_character_from_mount( sTmp, btrue, btrue );
        if ( pchr->ismount )
        {
            ChrList[sTmp].zvel = DISMOUNTZVEL;
            ChrList[sTmp].zpos += DISMOUNTZVEL;
            ChrList[sTmp].jumptime = JUMPDELAY;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDoAction( script_state_t * pstate, ai_state_t * pself )
{
    // TargetDoAction( tmpargument = "action" )
    // The function makes the target start a new action, if it is valid for the model
    // It will fail if the action is invalid or if the target is doing
    // something else already

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ChrList[pself->target].alive )
    {
        if ( pstate->argument < MAXACTION && ChrList[pself->target].actionready )
        {
            if ( MadList[ChrList[pself->target].inst.imad].actionvalid[pstate->argument] )
            {
                ChrList[pself->target].action = pstate->argument;
                ChrList[pself->target].inst.lip = 0;
                ChrList[pself->target].inst.lastframe = ChrList[pself->target].inst.frame;
                ChrList[pself->target].inst.frame = MadList[ChrList[pself->target].inst.imad].actionstart[pstate->argument];
                ChrList[pself->target].actionready = bfalse;
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

    // This function allows movement over the given passage area.
    // Fails if the passage is already open
    // Passage areas are defined in passage.txt and set in spawn.txt
    // for the given character

    SCRIPT_FUNCTION_BEGIN();

    // This function opens the passage specified by tmpargument, failing if the
    // passage was already open
    returncode = open_passage( pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClosePassage( script_state_t * pstate, ai_state_t * pself )
{
    // ClosePassage( tmpargument = "passage" )
    // This function prohibits movement over the given passage area, proceeding
    // if the passage isn't blocked.  Crushable characters within the passage
    // are crushed.

    SCRIPT_FUNCTION_BEGIN();

    // This function closes the passage specified by tmpargument, and proceeds
    // only if the passage is clear of obstructions
    returncode = close_passage( pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PassageOpen( script_state_t * pstate, ai_state_t * pself )
{
    // _PassageOpen( tmpargument = "passage" )
    // This function proceeds if the given passage is valid and open to movement
    // Used mostly by door characters to tell them when to run their open animation.

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument < numpassage && pstate->argument >= 0 )
    {
        returncode = passopen[pstate->argument];
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GoPoof( script_state_t * pstate, ai_state_t * pself )
{
    // GoPoof()
    // This function flags the character to be removed from the game entirely.
    // This doesn't work on players

    SCRIPT_FUNCTION_BEGIN();

    // This function flags the character to be removed from the game

    returncode = bfalse;
    if ( !pchr->isplayer )
    {
        returncode = btrue;
        pself->poof_time = frame_wld;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostTargetItemID( script_state_t * pstate, ai_state_t * pself )
{
    // CostTargetItemID( tmpargument = "idsz" )
    // This function proceeds if the target has a matching item, and poofs
    // that item...
    // For one use keys and such

    int tTmp;
    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function checks if the target has a matching item, and poofs it
    returncode = bfalse;

    // Check the pack
    iTmp = MAXCHR;
    tTmp = pself->target;
    sTmp = ChrList[tTmp].nextinpack;

    while ( sTmp != MAXCHR )
    {
        if ( CapList[ChrList[sTmp].model].idsz[IDSZ_PARENT] == ( IDSZ) pstate->argument || CapList[ChrList[sTmp].model].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
        {
            returncode = btrue;
            iTmp = sTmp;
            sTmp = MAXCHR;
        }
        else
        {
            tTmp = sTmp;
            sTmp = ChrList[sTmp].nextinpack;
        }
    }

    // Check left hand
    sTmp = ChrList[pself->target].holdingwhich[0];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
        {
            returncode = btrue;
            iTmp = ChrList[pself->target].holdingwhich[0];
        }
    }

    // Check right hand
    sTmp = ChrList[pself->target].holdingwhich[1];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
        {
            returncode = btrue;
            iTmp = ChrList[pself->target].holdingwhich[1];
        }
    }

    if ( returncode )
    {
        if ( ChrList[iTmp].ammo <= 1 )
        {
            // Poof the item
            if ( ChrList[iTmp].inpack )
            {
                // Remove from the pack
                ChrList[tTmp].nextinpack = ChrList[iTmp].nextinpack;
                ChrList[pself->target].numinpack--;
                free_one_character( iTmp );
                iTmp = MAXCHR;
            }
            else
            {
                // Drop from hand
                detach_character_from_mount( iTmp, btrue, bfalse );
                free_one_character( iTmp );
                iTmp = MAXCHR;
            }
        }
        else
        {
            // Cost one ammo
            ChrList[iTmp].ammo--;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoActionOverride( script_state_t * pstate, ai_state_t * pself )
{
    // DoActionOverride( tmpargument = "action" )
    // This function makes the character do a given action no matter what

    SCRIPT_FUNCTION_BEGIN();

    // This function starts a new action, if it is valid for the model
    // It will fail if the action is invalid
    returncode = bfalse;
    if ( pstate->argument < MAXACTION )
    {
        if ( MadList[pchr->inst.imad].actionvalid[pstate->argument] )
        {
            pchr->action = pstate->argument;
            pchr->inst.lip = 0;
            pchr->inst.lastframe = pchr->inst.frame;
            pchr->inst.frame = MadList[pchr->inst.imad].actionstart[pstate->argument];
            pchr->actionready = bfalse;
            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Healed( script_state_t * pstate, ai_state_t * pself )
{
    // _Healed()
    // This function proceeds if the character was healed by a healing particle

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was healed
    returncode = ( 0 != ( pself->alert & ALERTIF_HEALED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SendPlayerMessage( script_state_t * pstate, ai_state_t * pself )
{
    // SendPlayerMessage()

    SCRIPT_FUNCTION_BEGIN();

    // This function sends a message to the players
    display_message( pstate, MadList[pchr->inst.imad].msgstart + pstate->argument, pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CallForHelp( script_state_t * pstate, ai_state_t * pself )
{
    // CallForHelp()
    // This function calls all of the character's teammates for help...  The
    // teammates must use _CalledForHelp in their scripts

    SCRIPT_FUNCTION_BEGIN();

    // This function issues a call for help
    call_for_help( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddIDSZ( script_state_t * pstate, ai_state_t * pself )
{
    // AddIDSZ( tmpargument = "idsz" )
    // This function slaps an expansion IDSZ onto the module description...
    // Used to show completion of special quests for a given module

    SCRIPT_FUNCTION_BEGIN();

    // This function adds an idsz to the module's menu.txt file
    module_add_idsz( pickedmodule, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_State( script_state_t * pstate, ai_state_t * pself )
{
    // set_State( tmpargument = "state" )
    // This function sets the character's state.
    // VERY IMPORTANT... State is preserved from update to update

    SCRIPT_FUNCTION_BEGIN();

    pself->state = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_State( script_state_t * pstate, ai_state_t * pself )
{
    // get_State()

    SCRIPT_FUNCTION_BEGIN();

    // This function reads the character's state variable
    pstate->argument = pself->state;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs( script_state_t * pstate, ai_state_t * pself )
{
    // _StateIs( tmpargument = "state" )
    // This function proceeds if the character's state equals tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->argument == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanOpenStuff( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetCanOpenStuff()
    // This function proceeds if the target can open stuff ( set in data.txt )
    // Used by chests and buttons and such so only "smart" creatures can operate
    // them

    SCRIPT_FUNCTION_BEGIN();

    // This function fails if the target can't open stuff
	if(ChrList[pself->target].ismount && ChrList[ChrList[pself->target].holdingwhich[SLOT_LEFT]].openstuff) returncode = btrue;
    else returncode = ChrList[pself->target].openstuff;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Grabbed( script_state_t * pstate, ai_state_t * pself )
{
    // _Grabbed()
    // This function proceeds if the character was grabbed this update...
    // Used mostly by item characters

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was picked up
    returncode = ( 0 != ( pself->alert & ALERTIF_GRABBED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Dropped( script_state_t * pstate, ai_state_t * pself )
{
    // _Dropped()
    // This function proceeds if the character was dropped this update...
    // Used mostly by item characters

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was dropped
    returncode = ( 0 != ( pself->alert & ALERTIF_DROPPED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverIsHolding( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToWhoeverIsHolding()
    // This function sets the target to the character's holder or mount,
    // failing if the character is not held

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the target to the character's mount or holder,
    // failing if the character has no mount or holder
    returncode = bfalse;
    if ( pchr->attachedto < MAXCHR )
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
    // This function damages the target

    SCRIPT_FUNCTION_BEGIN();

    // This function applies little bit of love to the character's target.
    // The amount is set in tmpargument
    damage_character( pself->target, 0, pstate->argument, 1, pchr->damagetargettype, pchr->team, pself->index, DAMFXBLOC, btrue );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_XIsLessThanY( script_state_t * pstate, ai_state_t * pself )
{
    // _XIsLessThanY()
    // This function proceeds if tmpx is less than tmpy...

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->x < pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_WeatherTime( script_state_t * pstate, ai_state_t * pself )
{
    // set_WeatherTime( tmpargument = "time" )
    // This function can be used to slow down or speed up or stop rain and
    // other weather effects

    SCRIPT_FUNCTION_BEGIN();

    // set_ the weather timer
    weathertimereset = pstate->argument;
    weathertime = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_BumpHeight( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = get_BumpHeight()
    // This function sets tmpargument to the character's height

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = pchr->bumpheight;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Reaffirmed( script_state_t * pstate, ai_state_t * pself )
{
    // _Reaffirmed()
    // This function proceeds if the character was damaged by its reaffirm
    // damage type...  Used to relight the torch...

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was reaffirmed
    returncode = ( 0 != ( pself->alert & ALERTIF_REAFFIRMED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkeepAction( script_state_t * pstate, ai_state_t * pself )
{
    // UnkeepAction()
    // This function undoes KeepAction

    SCRIPT_FUNCTION_BEGIN();

    // This function makes the current animation start again
    pchr->keepaction = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnOtherTeam( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsOnOtherTeam()
    // This function proceeds if the target is on another team

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the target is on another team
    returncode = ( ChrList[pself->target].alive && ChrList[pself->target].team != pchr->team );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnHatedTeam( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsOnHatedTeam()
    // This function proceeds if the target is on an enemy team

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the target is on an enemy team
    returncode = ( ChrList[pself->target].alive && TeamList[pchr->team].hatesteam[ChrList[pself->target].team] && !ChrList[pself->target].invictus );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PressLatchButton( script_state_t * pstate, ai_state_t * pself )
{
    // PressLatchButton( tmpargument = "latch bits" )
    // This function emulates joystick button presses

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the latch buttons
    pchr->latchbutton = pchr->latchbutton | pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetOfLeader( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToTargetOfLeader()
    // This function sets the character's target to the target of its leader,
    // or it fails with no change if the leader is dead

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( TeamList[pchr->team].leader != NOLEADER )
    {
        pself->target = ChrList[TeamList[pchr->team].leader].ai.target;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_LeaderKilled( script_state_t * pstate, ai_state_t * pself )
{
    // _LeaderKilled()
    // This function proceeds if the team's leader died this update

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the character's leader has just died
    returncode = ( 0 != ( pself->alert & ALERTIF_LEADERKILLED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeLeader( script_state_t * pstate, ai_state_t * pself )
{
    // BecomeLeader()
    // This function makes the character the leader of the team

    SCRIPT_FUNCTION_BEGIN();

    // This function makes the character the team leader
    TeamList[pchr->team].leader = pself->index;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTargetArmor( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeTargetArmor( tmpargument = "armor" )
    // This function sets the armor type of the target...  Used for chests
    // set_s tmpargument as the old type and tmpx as the new type

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the target's armor type and returns the old type
    // as tmpargument and the new type as tmpx
    iTmp = ChrList[pself->target].skin;
    pstate->x = change_armor( pself->target, pstate->argument );
    pstate->argument = iTmp;  // The character's old armor

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveMoneyToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveMoneyToTarget( tmpargument = "money" )
    // This function increases the target's money, while decreasing the
    // character's own money.  tmpargument is set to the amount transferred

    int tTmp;
    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function transfers money from the character to the target, and sets
    // tmpargument to the amount transferred
    iTmp = pchr->money;
    tTmp = ChrList[pself->target].money;
    iTmp -= pstate->argument;
    tTmp += pstate->argument;
    if ( iTmp < 0 ) { tTmp += iTmp;  pstate->argument += iTmp;  iTmp = 0; }
    if ( tTmp < 0 ) { iTmp += tTmp;  pstate->argument += tTmp;  tTmp = 0; }
    if ( iTmp > MAXMONEY ) { iTmp = MAXMONEY; }
    if ( tTmp > MAXMONEY ) { tTmp = MAXMONEY; }

    pchr->money = iTmp;
    ChrList[pself->target].money = tTmp;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropKeys( script_state_t * pstate, ai_state_t * pself )
{
    // DropKeys()
    // This function drops all of the keys in the character's inventory.
    // This does NOT drop keys in the character's hands.

    SCRIPT_FUNCTION_BEGIN();

    drop_keys( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_LeaderIsAlive( script_state_t * pstate, ai_state_t * pself )
{
    // _LeaderIsAlive()
    // This function proceeds if the team has a leader

    SCRIPT_FUNCTION_BEGIN();

    // This function fails if there is no team leader
    returncode = ( TeamList[pchr->team].leader != NOLEADER );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOldTarget( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsOldTarget()
    // This function proceeds if the target is the same as it was last update

    SCRIPT_FUNCTION_BEGIN();

    // This function returns bfalse if the target has pstate->changed
    returncode = ( pself->target == pself->target_old );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLeader( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToLeader
    // This function sets the target to the leader, proceeding if their is
    // a valid leader for the character's team

    SCRIPT_FUNCTION_BEGIN();

    // This function fails if there is no team leader
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
    // SpawnCharacter(tmpx = "x", tmpy = "y", tmpturn = "turn", tmpdistance = "speed" )

    // This function spawns a character of the same type as the spawner...
    // This function spawns a character, failing if x,y is invalid

    // This is horribly complicated to use, so see ANIMATE.OBJ for an example
    // tmpx and tmpy give the coodinates, tmpturn gives the new character's
    // direction, and tmpdistance gives the new character's initial velocity

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = spawn_one_character( pstate->x, pstate->y, 0, pchr->model, pchr->team, 0, pstate->turn & 0xFFFF, NULL, MAXCHR );
    returncode = bfalse;
    if ( sTmp < MAXCHR )
    {
        if ( __chrhitawall( sTmp ) )
        {
            free_one_character( sTmp );
            sTmp = MAXCHR;
        }
        else
        {
            ChrList[sTmp].iskursed = bfalse;

            pself->child = sTmp;

            tTmp = pchr->turnleftright >> 2;
            ChrList[sTmp].xvel += turntocos[( tTmp+8192 )&TRIG_TABLE_MASK] * pstate->distance;
            ChrList[sTmp].yvel += turntosin[( tTmp+8192 )&TRIG_TABLE_MASK] * pstate->distance;

            ChrList[sTmp].ai.passage = pself->passage;
            ChrList[sTmp].ai.owner   = pself->owner;
        }
    }

    returncode = (sTmp != MAXCHR);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RespawnCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // RespawnCharacter()
    // This function respawns the character at its starting location...
    // Often used with the Clean functions

    SCRIPT_FUNCTION_BEGIN();

    respawn_character( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTile( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeTile( tmpargument = "tile type")
    // This function changes the tile under the character to the new tile type,
    // which is highly module dependent

    SCRIPT_FUNCTION_BEGIN();

    // This function changes the floor image under the character
    returncode = bfalse;
    if ( INVALID_TILE != pchr->onwhichfan )
    {
        returncode = btrue;
        mesh.mem.tile_list[pchr->onwhichfan].img = pstate->argument & 0xFF;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Used( script_state_t * pstate, ai_state_t * pself )
{
    // _Used()
    // This function proceeds if the character was used by its holder or rider...
    // Character's cannot be used if their reload time is greater than 0

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the character has been used
    returncode = ( 0 != ( pself->alert & ALERTIF_USED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropMoney( script_state_t * pstate, ai_state_t * pself )
{
    // DropMoney( tmpargument = "money" )
    // This function drops a certain amount of money, if the character has that
    // much

    SCRIPT_FUNCTION_BEGIN();

    // This function drops some of a character's money
    drop_money( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_OldTarget( script_state_t * pstate, ai_state_t * pself )
{
    // set_OldTarget()
    // This function sets the old target to the current target...  To allow
    // greater manipulations of the target

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the old target to the current target
    pself->target_old = pself->target;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DetachFromHolder( script_state_t * pstate, ai_state_t * pself )
{
    // DetachFromHolder()
    // This function drops the character or makes it get off its mount
    // Can be used to make slippery weapons, or to make certain characters
    // incapable of wielding certain weapons... "A troll can't grab a torch"

    SCRIPT_FUNCTION_BEGIN();

    // This function drops the character, failing only if it was not held
    if ( pchr->attachedto != MAXCHR )
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
    // _TargetHasVulnerabilityID( tmpargument = "vulnerability idsz" )
    // This function proceeds if the target is vulnerable to the given IDSZ...

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if ID matches tmpargument
    returncode = ( CapList[ChrList[pself->target].model].idsz[IDSZ_VULNERABILITY] == ( IDSZ ) pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CleanUp( script_state_t * pstate, ai_state_t * pself )
{
    // CleanUp()
    // This function tells all the dead characters on the team to clean
    // themselves up...  Usually done by the boss creature every second or so

    SCRIPT_FUNCTION_BEGIN();

    // This function issues the clean up order to all teammates
    issue_clean( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CleanedUp( script_state_t * pstate, ai_state_t * pself )
{
    // _CleanedUp()
    // This function proceeds if the character is dead and if the boss told it
    // to clean itself up

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the character was told to clean up
    returncode = ( 0 != ( pself->alert & ALERTIF_CLEANEDUP ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Sitting( script_state_t * pstate, ai_state_t * pself )
{
    // _Sitting()

    // This function proceeds if the character is riding a mount

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if the character is riding another
    returncode = ( pchr->attachedto != MAXCHR );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsHurt( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsHurt()
    // This function passes only if the target is hurt and alive

    SCRIPT_FUNCTION_BEGIN();
    if ( !ChrList[pself->target].alive || ChrList[pself->target].life > ChrList[pself->target].lifemax - HURTDAMAGE )
        returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAPlayer( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsAPlayer()
    // This function proceeds if the target is controlled by a human ( may not be local )

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList[pself->target].isplayer;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySound( script_state_t * pstate, ai_state_t * pself )
{
    // PlaySound( tmpargument = "sound", tmpdistance = "frequency" )
    // This function plays one of the character's sounds.
    // The sound fades out depending on its distance from the viewer

    SCRIPT_FUNCTION_BEGIN();

    // This function plays a sound
    if ( pchr->oldz > PITNOSOUND && pstate->argument >= 0 && pstate->argument < MAXWAVE )
    {
        sound_play_chunk( pchr->oldx, pchr->oldy, CapList[pchr->model].wavelist[pstate->argument] );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnParticle(tmpargument = "particle", tmpdistance = "character vertex", tmpx = "offset x", tmpy = "offset y" )

    // This function spawns a particle, offset from the character's location

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function spawns a particle
    tTmp = pself->index;
    if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

    tTmp = spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, pchr->turnleftright, pchr->model, pstate->argument, pself->index, pstate->distance, pchr->team, tTmp, 0, MAXCHR );
    if ( tTmp != TOTALMAXPRT )
    {
        // Detach the particle
        attach_particle_to_character( tTmp, pself->index, pstate->distance );
        PrtList[tTmp].attachedtocharacter = MAXCHR;
        // Correct X, Y, Z spacing
        PrtList[tTmp].xpos += pstate->x;
        PrtList[tTmp].ypos += pstate->y;
        PrtList[tTmp].zpos += PipList[PrtList[tTmp].pip].zspacingbase;

        // Don't spawn in walls
        if ( __prthitawall( tTmp ) )
        {
            PrtList[tTmp].xpos = pchr->xpos;
            if ( __prthitawall( tTmp ) )
            {
                PrtList[tTmp].ypos = pchr->ypos;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAlive( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsAlive()
    // This function proceeds if the target is alive

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the target is alive
    returncode = ChrList[pself->target].alive;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Stop( script_state_t * pstate, ai_state_t * pself )
{
    // Stop()
    // This function sets the character's maximum acceleration to 0...  Used
    // along with Walk and Run and Sneak

    SCRIPT_FUNCTION_BEGIN();

    pchr->maxaccel = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisaffirmCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // DisaffirmCharacter()
    // This function removes all the attached particles from a character
    // ( stuck arrows, flames, etc )

    SCRIPT_FUNCTION_BEGIN();

    disaffirm_attached_particles( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ReaffirmCharacter( script_state_t * pstate, ai_state_t * pself )
{
    // ReaffirmCharacter()
    // This function makes sure it has all of its reaffirmation particles
    // attached to it.
    // Used to make the torch light again

    SCRIPT_FUNCTION_BEGIN();

    reaffirm_attached_particles( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsSelf( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsSelf()

    // This function proceeds if the character is targeting itself

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->target == pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsMale( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsMale()

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the target is male
    returncode = ( ChrList[pself->target].gender == GENMALE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsFemale( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsFemale()

    // This function proceeds if the target is female

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the target is female
    returncode = ( ChrList[pself->target].gender == GENFEMALE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToSelf( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToSelf()
    // This function sets the target to the character itself

    SCRIPT_FUNCTION_BEGIN();

    pself->target = pself->index;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToRider( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToRider()
    // This function sets the target to whoever is riding the character (left/only grip),
    // failing if there is no rider

    SCRIPT_FUNCTION_BEGIN();
    if ( pchr->holdingwhich[0] == MAXCHR )
    {
        returncode = bfalse;
    }
    else
    {
        pself->target = pchr->holdingwhich[0];
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_AttackTurn( script_state_t * pstate, ai_state_t * pself )
{
    // This function sets tmpturn to the direction from which the last attack
    // gCamera.e.
    // Not particularly useful in most cases, but it could be.

    SCRIPT_FUNCTION_BEGIN();

    pstate->turn = pself->directionlast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_DamageType( script_state_t * pstate, ai_state_t * pself )
{
    // get_DamageType()
    // This function sets tmpargument to the damage type of the last attack that
    // hit the character

    SCRIPT_FUNCTION_BEGIN();

    // This function gets the last type of damage
    pstate->argument = pself->damagetypelast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeSpell( script_state_t * pstate, ai_state_t * pself )
{
    // BecomeSpell()
    // This function turns a spellbook character into a spell based on its
    // content.

    // TOO COMPLICATED TO EXPLAIN...  SHOULDN'T EVER BE NEEDED BY YOU...

    SCRIPT_FUNCTION_BEGIN();

    pchr->money = ( pchr->skin ) % MAXSKIN;
    change_character( pself->index, pself->content, 0, LEAVENONE );
    pself->content = 0;  // Reset so it doesn't mess up
    pself->state   = 0;  // Reset so it doesn't mess up
    pself->changed = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeSpellbook( script_state_t * pstate, ai_state_t * pself )
{
    // BecomeSpellbook()
    // This function turns a spell character into a spellbook and sets the content accordingly.
    // TOO COMPLICATED TO EXPLAIN. Just copy the spells that already exist, and don't change
    // them too much

    SCRIPT_FUNCTION_BEGIN();

    pself->content = pchr->model;
    change_character( pself->index, SPELLBOOK, pchr->money % MAXSKIN, LEAVENONE );
    pself->state   = 0;  // Reset so it doesn't burn up
    pself->changed = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ScoredAHit( script_state_t * pstate, ai_state_t * pself )
{
    // _ScoredAHit()
    // This function proceeds if the character damaged another character this
    // update...

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character scored a hit
    returncode = ( 0 != ( pself->alert & ALERTIF_SCOREDAHIT ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Disaffirmed( script_state_t * pstate, ai_state_t * pself )
{
    // _Disaffirmed()
    // This function proceeds if the character was disaffirmed...
    // This doesn't seem useful anymore...

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was disaffirmed
    returncode = ( 0 != ( pself->alert & ALERTIF_DISAFFIRMED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TranslateOrder( script_state_t * pstate, ai_state_t * pself )
{
    // TranslateOrder()
    // This function translates a packed order into understandable values...
    // See CreateOrder for more...  This function sets tmpx, tmpy, tmpargument,
    // and possibly sets the target ( which may not be good )

    SCRIPT_FUNCTION_BEGIN();

    // This function gets the order and sets tmpx, tmpy, tmpargument and the
    // target ( if valid )
    sTmp = pself->order >> 24;
    if ( sTmp < MAXCHR )
    {
        pself->target = sTmp;
    }

    pstate->x = ( ( pself->order >> 14 ) & 1023 ) << 6;
    pstate->y = ( ( pself->order >> 4 ) & 1023 ) << 6;
    pstate->argument = pself->order & 15;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverWasHit( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToWhoeverWasHit()
    // This function sets the target to whoever was hit by the character last

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the target to whoever the character hit last,
    pself->target = pself->hitlast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWideEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToWideEnemy()
    // This function sets the target to an enemy in the vicinity around the
    // character, failing if there are none

    SCRIPT_FUNCTION_BEGIN();

    // This function finds an enemy, and proceeds only if there is one
    returncode = bfalse;
    if (get_target( pself->index, WIDE, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse) != MAXCHR) returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Changed( script_state_t * pstate, ai_state_t * pself )
{
    // _Changed
    // This function proceeds if the character has changed shape.
    // Needed for morph spells and such

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was polymorphed
    returncode = ( 0 != ( pself->alert & ALERTIF_CHANGED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_InWater( script_state_t * pstate, ai_state_t * pself )
{
    // _InWater()
    // This function proceeds if the character has just entered into some water
    // this update ( and the water is really water, not fog or another effect )

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character got wet
    returncode = ( 0 != ( pself->alert & ALERTIF_INWATER ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Bored( script_state_t * pstate, ai_state_t * pself )
{
    // _Bored()
    // This function proceeds if the character has been standing idle too long

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character is bored
    returncode = ( 0 != ( pself->alert & ALERTIF_BORED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TooMuchBaggage( script_state_t * pstate, ai_state_t * pself )
{
    // _TooMuchBaggage()
    // This function proceeds if the character tries to put an item in his/her
    // pockets, but the character already has 6 items in the inventory.
    // Used to tell the players what's going on.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 != ( pself->alert & ALERTIF_TOOMUCHBAGGAGE ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Grogged( script_state_t * pstate, ai_state_t * pself )
{
    // _Grogged()
    // This function proceeds if the character has been grogged ( a type of
    // confusion ) this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 != ( pself->alert & ALERTIF_GROGGED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Dazed( script_state_t * pstate, ai_state_t * pself )
{
    // _Dazed
    // This function proceeds if the character has been dazed ( a type of
    // confusion ) this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was dazed
    returncode = ( 0 != ( pself->alert & ALERTIF_DAZED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasSpecialID( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetHasSpecialID( tmpargument = "special idsz" )
    // This function proceeds if the character has a special IDSZ ( in data.txt )

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( CapList[ChrList[pself->target].model].idsz[IDSZ_SPECIAL] == ( IDSZ ) pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PressTargetLatchButton( script_state_t * pstate, ai_state_t * pself )
{
    // PressTargetLatchButton( tmpargument = "latch bits" )
    // This function mimics joystick button presses for the target.
    // For making items force their own usage and such

    SCRIPT_FUNCTION_BEGIN();

    ChrList[pself->target].latchbutton = ChrList[pself->target].latchbutton | pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Invisible( script_state_t * pstate, ai_state_t * pself )
{
    // _Invisible()
    // This function proceeds if the character is invisible

    SCRIPT_FUNCTION_BEGIN();

    // This function passes if the character is invisible
    returncode = ( pchr->inst.alpha <= INVISIBLE ) || ( pchr->inst.light <= INVISIBLE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ArmorIs( script_state_t * pstate, ai_state_t * pself )
{
    // _ArmorIs( tmpargument = "skin" )
    // This function proceeds if the character's skin type equals tmpargument

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function passes if the character's skin is tmpargument
    tTmp = pchr->skin;
    returncode = ( tTmp == pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetGrogTime( script_state_t * pstate, ai_state_t * pself )
{
    // get_TargetGrogTime()
    // This function sets tmpargument to the number of updates before the
    // character is ungrogged, proceeding if the number is greater than 0

    SCRIPT_FUNCTION_BEGIN();

    // This function returns tmpargument as the grog time, and passes if it is not 0
    pstate->argument = pchr->grogtime;
    returncode = ( pstate->argument != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetDazeTime( script_state_t * pstate, ai_state_t * pself )
{
    // get_TargetDazeTime()
    // This function sets tmpargument to the number of updates before the
    // character is undazed, proceeding if the number is greater than 0

    SCRIPT_FUNCTION_BEGIN();

    // This function returns tmpargument as the daze time, and passes if it is not 0
    pstate->argument = pchr->dazetime;
    returncode = ( pstate->argument != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageType( script_state_t * pstate, ai_state_t * pself )
{
    // set_DamageType( tmpargument = "damage type" )
    // This function lets a weapon change the type of damage it inflicts

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the bump damage type
    pchr->damagetargettype = pstate->argument & ( DAMAGE_COUNT - 1 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_WaterLevel( script_state_t * pstate, ai_state_t * pself )
{
    // set_WaterLevel( tmpargument = "level" )
    // This function raises or lowers the water in the module

    int iTmp;
    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function raises and lowers the module's water
    fTmp = ( pstate->argument / 10.0f ) - waterdouselevel;
    watersurfacelevel += fTmp;
    waterdouselevel += fTmp;

    for ( iTmp = 0; iTmp < MAXWATERLAYER; iTmp++ )
    {
        waterlayerz[iTmp] += fTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnchantTarget( script_state_t * pstate, ai_state_t * pself )
{
    // EnchantTarget()
    // This function enchants the target with the enchantment given
    // in enchant.txt.
    // Make sure you use set_OwnerToTarget before doing this.

    SCRIPT_FUNCTION_BEGIN();

    // This function enchants the target
    sTmp = spawn_enchant( pself->owner, pself->target, pself->index, MAXENCHANT, MAXMODEL );
    returncode = ( sTmp != MAXENCHANT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnchantChild( script_state_t * pstate, ai_state_t * pself )
{
    // EnchantChild()
    // This function enchants the last character spawned with the enchantment
    // given in enchant.txt.
    // Make sure you use set_OwnerToTarget before doing this.

    SCRIPT_FUNCTION_BEGIN();

    // This function can be used with SpawnCharacter to enchant the
    // newly spawned character
    sTmp = spawn_enchant( pself->owner, pself->child, pself->index, MAXENCHANT, MAXMODEL );
    returncode = ( sTmp != MAXENCHANT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TeleportTarget( script_state_t * pstate, ai_state_t * pself )
{
    // TeleportTarget( tmpx = "x", tmpy = "y" )
    // This function teleports the target to the X, Y location, failing if the
    // location is off the map or blocked

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->x > EDGE && pstate->y > EDGE && pstate->x < mesh.info.edge_x - EDGE && pstate->y < mesh.info.edge_y - EDGE )
    {
        // Yeah!  It worked!
        sTmp = pself->target;
        detach_character_from_mount( sTmp, btrue, bfalse );
        ChrList[sTmp].oldx = ChrList[sTmp].xpos;
        ChrList[sTmp].oldy = ChrList[sTmp].ypos;
        ChrList[sTmp].xpos = pstate->x;
        ChrList[sTmp].ypos = pstate->y;
        ChrList[sTmp].zpos = pstate->distance;
        ChrList[sTmp].turnleftright = pstate->turn & 0xFFFF;
        if ( __chrhitawall( sTmp ) )
        {
            // No it didn't...
            ChrList[sTmp].xpos = ChrList[sTmp].oldx;
            ChrList[sTmp].ypos = ChrList[sTmp].oldy;
            ChrList[sTmp].zpos = ChrList[sTmp].oldz;
            ChrList[sTmp].turnleftright = ChrList[sTmp].oldturn;
            returncode = bfalse;
        }
        else
        {
            ChrList[sTmp].oldx = ChrList[sTmp].xpos;
            ChrList[sTmp].oldy = ChrList[sTmp].ypos;
            ChrList[sTmp].oldz = ChrList[sTmp].zpos;
            ChrList[sTmp].oldturn = ChrList[sTmp].turnleftright;
            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveExperienceToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveExperienceToTarget( tmpargument = "amount", tmpdistance = "type" )
    // This function gives the target some experience, xptype from distance,
    // amount from argument...

    SCRIPT_FUNCTION_BEGIN();

    give_experience( pself->target, pstate->argument, pstate->distance, bfalse );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IncreaseAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // IncreaseAmmo()
    // This function increases the character's ammo by 1

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
    // This function unkurses the target

    SCRIPT_FUNCTION_BEGIN();

    ChrList[pself->target].iskursed = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveExperienceToTargetTeam( script_state_t * pstate, ai_state_t * pself )
{
    // GiveExperienceToTargetTeam( tmpargument = "amount", tmpdistance = "type" )
    // This function gives experience to everyone on the target's team

    SCRIPT_FUNCTION_BEGIN();

    give_team_experience( ChrList[pself->target].team, pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Unarmed( script_state_t * pstate, ai_state_t * pself )
{
    // _Unarmed()
    // This function proceeds if the character is holding no items in hand.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pchr->holdingwhich[0] == MAXCHR && pchr->holdingwhich[1] == MAXCHR );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RestockTargetAmmoIDAll( script_state_t * pstate, ai_state_t * pself )
{
    // RestockTargetAmmoIDAll( tmpargument = "idsz" )
    // This function restocks the ammo of all of the target's items, if those
    // items have a matching parent or type IDSZ

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function restocks the ammo of every item the character is holding,
    // if the item matches the ID given ( parent or child type )
    iTmp = 0;  // Amount of ammo given
    sTmp = ChrList[pself->target].holdingwhich[0];
    iTmp += restock_ammo( sTmp, pstate->argument );
    sTmp = ChrList[pself->target].holdingwhich[1];
    iTmp += restock_ammo( sTmp, pstate->argument );
    sTmp = ChrList[pself->target].nextinpack;

    while ( sTmp != MAXCHR )
    {
        iTmp += restock_ammo( sTmp, pstate->argument );
        sTmp = ChrList[sTmp].nextinpack;
    }

    pstate->argument = iTmp;
    returncode = ( iTmp != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RestockTargetAmmoIDFirst( script_state_t * pstate, ai_state_t * pself )
{
    // RestockTargetAmmoIDFirst( tmpargument = "idsz" )
    // This function restocks the ammo of the first item the character is holding,
    // if the item matches the ID given ( parent or child type )

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = 0;  // Amount of ammo given
    sTmp = ChrList[pself->target].holdingwhich[0];
    iTmp += restock_ammo( sTmp, pstate->argument );
    if ( iTmp == 0 )
    {
        sTmp = ChrList[pself->target].holdingwhich[1];
        iTmp += restock_ammo( sTmp, pstate->argument );
        if ( iTmp == 0 )
        {
            sTmp = ChrList[pself->target].nextinpack;

            while ( sTmp != MAXCHR && iTmp == 0 )
            {
                iTmp += restock_ammo( sTmp, pstate->argument );
                sTmp = ChrList[sTmp].nextinpack;
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
    // This function makes the target flash

    SCRIPT_FUNCTION_BEGIN();

    // This function flashes the character
    flash_character( pself->target, 255 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_RedShift( script_state_t * pstate, ai_state_t * pself )
{
    // set_RedShift( tmpargument = "red darkening" )
    // This function sets the character's red shift ( 0 - 3 ), higher values
    // making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    // This function alters a character's coloration
    pchr->inst.redshift = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_GreenShift( script_state_t * pstate, ai_state_t * pself )
{
    // set_GreenShift( tmpargument = "green darkening" )
    // This function sets the character's green shift ( 0 - 3 ), higher values
    // making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    // This function alters a character's coloration
    pchr->inst.grnshift = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BlueShift( script_state_t * pstate, ai_state_t * pself )
{
    // set_BlueShift( tmpargument = "blue darkening" )
    // This function sets the character's blue shift ( 0 - 3 ), higher values
    // making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    // This function alters a character's coloration
    pchr->inst.blushift = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Light( script_state_t * pstate, ai_state_t * pself )
{
    // set_Light( tmpargument = "lighness" )
    // This function alters the character's transparency ( 0 - 254 )
    // 255 = no transparency

    SCRIPT_FUNCTION_BEGIN();

    pchr->inst.light = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Alpha( script_state_t * pstate, ai_state_t * pself )
{
    // set_Alpha( tmpargument = "alpha" )
    // This function alters the character's transparency ( 0 - 255 )
    // 255 = no transparency

    SCRIPT_FUNCTION_BEGIN();

    // This function alters a character's transparency
    pchr->inst.alpha = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromBehind( script_state_t * pstate, ai_state_t * pself )
{
    // _HitFromBehind()
    // This function proceeds if the last attack to the character gCamera.e from behind

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= BEHIND - 8192 && pself->directionlast < BEHIND + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromFront( script_state_t * pstate, ai_state_t * pself )
{
    // _HitFromFront()
    // This function proceeds if the last attack to the character gCamera.e
    // from the front

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= 49152 + 8192 || pself->directionlast < FRONT + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromLeft( script_state_t * pstate, ai_state_t * pself )
{
    // _HitFromLeft()
    // This function proceeds if the last attack to the character gCamera.e
    // from the left

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if the character was attacked from the left
    returncode = bfalse;
    if ( pself->directionlast >= LEFT - 8192 && pself->directionlast < LEFT + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromRight( script_state_t * pstate, ai_state_t * pself )
{
    // _HitFromRight()
    // This function proceeds if the last attack to the character gCamera.e
    // from the right

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= RIGHT - 8192 && pself->directionlast < RIGHT + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnSameTeam( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsOnSameTeam()
    // This function proceeds if the target is on the character's team

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ChrList[pself->target].team == pchr->team )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KillTarget( script_state_t * pstate, ai_state_t * pself )
{
    // KillTarget()
    // This function kills the target

    SCRIPT_FUNCTION_BEGIN();

    kill_character( pself->target, pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UndoEnchant( script_state_t * pstate, ai_state_t * pself )
{
    // UndoEnchant()
    // This function removes the last enchantment spawned by the character,
    // proceeding if an enchantment was removed

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pchr->undoenchant != MAXENCHANT );
    remove_enchant( pchr->undoenchant );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_WaterLevel( script_state_t * pstate, ai_state_t * pself )
{
    // get_WaterLevel()
    // This function sets tmpargument to the current douse level for the water * 10.
    // A waterlevel in wawalight of 85 would set tmpargument to 850

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = waterdouselevel * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostTargetMana( script_state_t * pstate, ai_state_t * pself )
{
    // CostTargetMana( tmpargument = "amount" )
    // This function costs the target a specific amount of mana, proceeding
    // if the target was able to pay the price...  The amounts are * 256

    SCRIPT_FUNCTION_BEGIN();

    returncode = cost_mana( pself->target, pstate->argument, pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasAnyID( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetHasAnyID( tmpargument = "idsz" )
    // This function proceeds if the target has any IDSZ that matches the given one

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = 0;
    tTmp = 0;
    while ( tTmp < IDSZ_COUNT )
    {
        returncode |= ( CapList[ChrList[pself->target].model].idsz[tTmp] == ( IDSZ ) pstate->argument );
        tTmp++;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BumpSize( script_state_t * pstate, ai_state_t * pself )
{
    // set_BumpSize( tmpargument = "size" )
    // This function sets the how wide the character is

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = pchr->bumpsizebig;
    fTmp = fTmp / pchr->bumpsize;  // 1.5f or 2.0f
    pchr->bumpsize = pstate->argument * pchr->fat;
    pchr->bumpsizebig = fTmp * pchr->bumpsize;
    pchr->bumpsizesave = pstate->argument;
    pchr->bumpsizebigsave = fTmp * pchr->bumpsizesave;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotDropped( script_state_t * pstate, ai_state_t * pself )
{
    // _NotDropped()
    // This function proceeds if the character is kursed and another character
    // was holding it and tried to drop it

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 != ( pself->alert & ALERTIF_NOTDROPPED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_YIsLessThanX( script_state_t * pstate, ai_state_t * pself )
{
    // _YIsLessThanX()
    // This function proceeds if tmpy is less than tmpx

    SCRIPT_FUNCTION_BEGIN();

    // This function passes only if tmpy is less than tmpx
    returncode = ( pstate->y < pstate->x );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FlyHeight( script_state_t * pstate, ai_state_t * pself )
{
    // set_FlyHeight( tmpargument = "height" )
    // This function makes the character fly ( or fall to ground if 0 )

    SCRIPT_FUNCTION_BEGIN();

    // This function sets a character's fly height
    pchr->flyheight = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Blocked( script_state_t * pstate, ai_state_t * pself )
{
    // _Blocked()
    // This function proceeds if the character blocked the attack of another
    // character this update

    SCRIPT_FUNCTION_BEGIN();

    // This function passes if the character blocked an attack
    returncode = ( 0 != ( pself->alert & ALERTIF_BLOCKED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsDefending( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsDefending()
    // This function proceeds if the target is holding up a shield or similar
    // defense

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList[pself->target].action >= ACTIONPA && ChrList[pself->target].action <= ACTIONPD );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAttacking( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsAttacking()
    // This function proceeds if the target is doing an attack action

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList[pself->target].action >= ACTIONUA && ChrList[pself->target].action <= ACTIONFD );

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
    // _ContentIs( tmpargument = "test" )
    // This function proceeds if the content matches tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->argument == pself->content );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToWatchTarget( script_state_t * pstate, ai_state_t * pself )
{
    // set_TurnModeToWatchTarget()
    // This function makes the character face its target, no matter what
    // direction it is moving in...  Undo this with set_TurnModeToVelocity

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the turn mode
    pchr->turnmode = TURNMODEWATCHTARGET;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIsNot( script_state_t * pstate, ai_state_t * pself )
{
    // _StateIsNot( tmpargument = "test" )
    // This function proceeds if the character's state does not equal tmpargument

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
    // This function spits out some useful numbers

    char cTmp[256];

    SCRIPT_FUNCTION_BEGIN();

    sprintf( cTmp, "aistate %d, aicontent %d, target %d", pself->state, pself->content, pself->target );
    debug_message( cTmp );
    sprintf( cTmp, "tmpx %d, tmpy %d", pstate->x, pstate->y );
    debug_message( cTmp );
    sprintf( cTmp, "tmpdistance %d, tmpturn %d", pstate->distance, pstate->turn );
    debug_message( cTmp );
    sprintf( cTmp, "tmpargument %d, selfturn %d", pstate->argument, pchr->turnleftright );
    debug_message( cTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BlackTarget( script_state_t * pstate, ai_state_t * pself )
{
    // BlackTarget()
    // The opposite of FlashTarget, causing the target to turn black

    SCRIPT_FUNCTION_BEGIN();

    flash_character( pself->target, 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SendMessageNear( script_state_t * pstate, ai_state_t * pself )
{
    // SendMessageNear( tmpargument = "message" )
    // This function sends a message if the camera is in the nearby area

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = ABS( pchr->oldx - gCamera.trackx ) + ABS( pchr->oldy - gCamera.tracky );
    if ( iTmp < MSGDISTANCE )
    {
        display_message( pstate, MadList[pchr->inst.imad].msgstart + pstate->argument, pself->index );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitGround( script_state_t * pstate, ai_state_t * pself )
{
    // _HitGround()
    // This function proceeds if a character hit the ground this update...
    // Used to determine when to play the sound for a dropped item

    SCRIPT_FUNCTION_BEGIN();

    // This function passes if the character just hit the ground
    returncode = ( 0 != ( pself->alert & ALERTIF_HITGROUND ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NameIsKnown( script_state_t * pstate, ai_state_t * pself )
{
    // _NameIsKnown()
    // This function proceeds if the character's name is known

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->nameknown;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UsageIsKnown( script_state_t * pstate, ai_state_t * pself )
{
    // _UsageIsKnown()
    // This function proceeds if the character's usage is known

    SCRIPT_FUNCTION_BEGIN();

    returncode = CapList[pchr->model].usageknown;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingItemID( script_state_t * pstate, ai_state_t * pself )
{
    // _HoldingItemID( tmpargument = "idsz" )
    // This function proceeds if the character is holding a specified item
    // in hand, setting tmpargument to the latch button to press to use it

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    // Check left hand
    sTmp = pchr->holdingwhich[0];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
        {
            pstate->argument = LATCHBUTTON_LEFT;
            returncode = btrue;
        }
    }

    // Check right hand
    sTmp = pchr->holdingwhich[1];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
        {
            pstate->argument = LATCHBUTTON_RIGHT;
            if ( returncode )  pstate->argument = LATCHBUTTON_LEFT + ( rand() & 1 );

            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingRangedWeapon( script_state_t * pstate, ai_state_t * pself )
{
    // _HoldingRangedWeapon()
    // This function passes if the character is holding a ranged weapon, returning
    // the latch to press to use it.  This also checks ammo/ammoknown.

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    pstate->argument = 0;

    // Check left hand
    tTmp = pchr->holdingwhich[0];
    if ( tTmp != MAXCHR )
    {
        sTmp = ChrList[tTmp].model;
        if ( CapList[sTmp].isranged && ( ChrList[tTmp].ammomax == 0 || ( ChrList[tTmp].ammo != 0 && ChrList[tTmp].ammoknown ) ) )
        {
            pstate->argument = LATCHBUTTON_LEFT;
            returncode = btrue;
        }
    }

    // Check right hand
    tTmp = pchr->holdingwhich[1];
    if ( tTmp != MAXCHR )
    {
        sTmp = ChrList[tTmp].model;
        if ( CapList[sTmp].isranged && ( ChrList[tTmp].ammomax == 0 || ( ChrList[tTmp].ammo != 0 && ChrList[tTmp].ammoknown ) ) )
        {
            if ( pstate->argument == 0 || ( frame_all&1 ) )
            {
                pstate->argument = LATCHBUTTON_RIGHT;
                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingMeleeWeapon( script_state_t * pstate, ai_state_t * pself )
{
    // _HoldingMeleeWeapon()
    // This function proceeds if the character is holding a specified item
    // in hand, setting tmpargument to the latch button to press to use it

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    pstate->argument = 0;

    // Check left hand
    sTmp = pchr->holdingwhich[0];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( !CapList[sTmp].isranged && CapList[sTmp].weaponaction != ACTIONPA )
        {
            pstate->argument = LATCHBUTTON_LEFT;
            returncode = btrue;
        }
    }

    // Check right hand
    sTmp = pchr->holdingwhich[1];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( !CapList[sTmp].isranged && CapList[sTmp].weaponaction != ACTIONPA )
        {
            if ( pstate->argument == 0 || ( frame_all&1 ) )
            {
                pstate->argument = LATCHBUTTON_RIGHT;
                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingShield( script_state_t * pstate, ai_state_t * pself )
{
    // _HoldingShield()
    // This function proceeds if the character is holding a specified item
    // in hand, setting tmpargument to the latch button to press to use it.
    // The button will need to be held down.

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    pstate->argument = 0;

    // Check left hand
    sTmp = pchr->holdingwhich[0];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( CapList[sTmp].weaponaction == ACTIONPA )
        {
            pstate->argument = LATCHBUTTON_LEFT;
            returncode = btrue;
        }
    }

    // Check right hand
    sTmp = pchr->holdingwhich[1];
    if ( sTmp != MAXCHR )
    {
        sTmp = ChrList[sTmp].model;
        if ( CapList[sTmp].weaponaction == ACTIONPA )
        {
            pstate->argument = LATCHBUTTON_RIGHT;
            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Kursed( script_state_t * pstate, ai_state_t * pself )
{
    // _Kursed()
    // This function proceeds if the character is kursed

    SCRIPT_FUNCTION_BEGIN();

    // This function passes if the character is kursed
    returncode = pchr->iskursed;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsKursed( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsKursed()
    // This function proceeds if the target is kursed

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList[pself->target].iskursed;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsDressedUp( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsDressedUp()
    // This function proceeds if the target is dressed in fancy clothes

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = pchr->skin;
    iTmp = 1 << iTmp;
    returncode = ( ( CapList[pchr->model].skindressy & iTmp ) != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OverWater( script_state_t * pstate, ai_state_t * pself )
{
    // _OverWater()
    // This function proceeds if the character is on a water tile

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( INVALID_TILE != pchr->onwhichfan )
    {
        returncode = ( ( 0 != ( mesh.mem.tile_list[pchr->onwhichfan].fx & MESHFX_WATER ) ) && wateriswater );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Thrown( script_state_t * pstate, ai_state_t * pself )
{
    // _Thrown()
    // This function proceeds if the character was thrown this update.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 != ( pself->alert & ALERTIF_THROWN ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeNameKnown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeNameKnown()
    // This function makes the name of the character known, for identifying
    // weapons and spells and such

    SCRIPT_FUNCTION_BEGIN();

    // This function makes the name of an item/character known.
    pchr->nameknown = btrue;
    //            pchr->icon = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeUsageKnown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeUsageKnown()
    // This function makes the usage known for this type of object
    // For XP gains from using an unknown potion or such

    SCRIPT_FUNCTION_BEGIN();

    CapList[pchr->model].usageknown = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopTargetMovement( script_state_t * pstate, ai_state_t * pself )
{
    // StopTargetMovement()
    // This function makes the target stop moving temporarily
    // set_s the target's x and y velocities to 0, and
    // sets the z velocity to 0 if the character is moving upwards.
    // This is a special function for the IronBall object

    SCRIPT_FUNCTION_BEGIN();

    ChrList[pself->target].xvel = 0;
    ChrList[pself->target].yvel = 0;
    if ( ChrList[pself->target].zvel > 0 ) ChrList[pself->target].zvel = gravity;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_XY( script_state_t * pstate, ai_state_t * pself )
{
    // set_XY( tmpargument = "index", tmpx = "x", tmpy = "y" )
    // This function sets one of the 8 permanent storage variable slots
    // ( each of which holds an x,y pair )

    SCRIPT_FUNCTION_BEGIN();

    pself->x[pstate->argument&STORAND] = pstate->x;
    pself->y[pstate->argument&STORAND] = pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_XY( script_state_t * pstate, ai_state_t * pself )
{
    // get_XY( tmpargument = "index" )
    // This function reads one of the 8 permanent storage variable slots,
    // setting tmpx and tmpy accordingly

    SCRIPT_FUNCTION_BEGIN();

    pstate->x = pself->x[pstate->argument&STORAND];
    pstate->y = pself->y[pstate->argument&STORAND];

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddXY( script_state_t * pstate, ai_state_t * pself )
{
    // AddXY( tmpargument = "index", tmpx = "x", tmpy = "y" )
    // This function alters the contents of one of the 8 permanent storage
    // slots

    SCRIPT_FUNCTION_BEGIN();

    pself->x[pstate->argument&STORAND] += pstate->x;
    pself->y[pstate->argument&STORAND] += pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeAmmoKnown( script_state_t * pstate, ai_state_t * pself )
{
    // MakeAmmoKnown()
    // This function makes the character's ammo known ( for items )

    SCRIPT_FUNCTION_BEGIN();

    pchr->ammoknown = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedParticle( tmpargument = "particle", tmpdistance = "vertex" )
    // This function spawns a particle attached to the character

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    tTmp = pself->index;
    if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

    tTmp = spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, pchr->turnleftright, pchr->model, pstate->argument, pself->index, pstate->distance, pchr->team, tTmp, 0, MAXCHR );
    returncode = (tTmp != TOTALMAXPRT);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnExactParticle( tmpargument = "particle", tmpx = "x", tmpy = "y", tmpdistance = "z" )
    // This function spawns a particle at a specific x, y, z position

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    tTmp = pself->index;
    if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

    spawn_one_particle( pstate->x, pstate->y, pstate->distance, pchr->turnleftright, pchr->model, pstate->argument, MAXCHR, 0, pchr->team, tTmp, 0, MAXCHR );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateTarget( script_state_t * pstate, ai_state_t * pself )
{
    // AccelerateTarget( tmpx = "acc x", tmpy = "acc y" )
    // This function changes the x and y speeds of the target

    SCRIPT_FUNCTION_BEGIN();

    ChrList[pself->target].xvel += pstate->x;
    ChrList[pself->target].yvel += pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_distanceIsMoreThanTurn( script_state_t * pstate, ai_state_t * pself )
{
    // _distanceIsMoreThanTurn()
    // This function proceeds tmpdistance is greater than tmpturn

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->distance > pstate->turn );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Crushed( script_state_t * pstate, ai_state_t * pself )
{
    // _Crushed()
    // This function proceeds if the character was crushed in a passage this
    // update...

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the character was crushed
    returncode = ( 0 != ( pself->alert & ALERTIF_CRUSHED ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeCrushValid( script_state_t * pstate, ai_state_t * pself )
{
    // MakeCrushValid()
    // This function makes a character able to be crushed by closing doors
    // and such

    SCRIPT_FUNCTION_BEGIN();

    pchr->canbecrushed = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLowestTarget( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToLowestTarget()
    // This function sets the target to the absolute bottom character.
    // The holder of the target, or the holder of the holder of the target, or
    // the holder of the holder of ther holder of the target, etc.

    SCRIPT_FUNCTION_BEGIN();

    // This sets the target to whatever the target is being held by,
    // The lowest in the set.  This function never fails
    while ( pself->target != MAXCHR && ChrList[pself->target].attachedto != MAXCHR )
    {
        pself->target = ChrList[pself->target].attachedto;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotPutAway( script_state_t * pstate, ai_state_t * pself )
{
    // _NotPutAway()
    // This function proceeds if the character couldn't be put into another
    // character's pockets for some reason.
    // It might be kursed or too big or something

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the character couln't be put in the pack
    returncode = ( 0 != ( pself->alert & ALERTIF_NOTPUTAWAY ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TakenOut( script_state_t * pstate, ai_state_t * pself )
{
    // _TakenOut()
    // This function proceeds if the character is equiped in another's inventory,
    // and the holder tried to unequip it ( take it out of pack ), but the
    // item was kursed and didn't cooperate

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the character was taken out of the pack
    returncode = ( 0 != ( pself->alert & ALERTIF_TAKENOUT ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AmmoOut( script_state_t * pstate, ai_state_t * pself )
{
    // _AmmoOut()
    // This function proceeds if the character itself has no ammo left.
    // This is for crossbows and such, not archers.

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the character has no ammo
    returncode = ( pchr->ammo == 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySoundLooped( script_state_t * pstate, ai_state_t * pself )
{
    // PlaySoundLooped( tmpargument = "sound", tmpdistance = "frequency" )

    // This function starts playing a continuous sound
    //!!BAD!! DOESN'T WORK !!BAD!!

    // You could use this, but right now there's no way to stop the sound later, so it's better not to start it

    SCRIPT_FUNCTION_BEGIN();

    //if ( moduleactive )
    //{
    //    sound_play_chunk(CapList[pchr->model].wavelist[pstate->argument], PANMID, volume, pstate->distance);
    //}

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopSound( script_state_t * pstate, ai_state_t * pself )
{
    // StopSound( tmpargument = "sound" )
    // This function stops the playing of a continuous sound
    //!!BAD!! DOESN'T WORK !!BAD!!

    // TODO: implement this (the scripter doesn't know which channel to stop)
    // This function stops playing a sound

    SCRIPT_FUNCTION_BEGIN();

    // sound_stop_channel([pstate->argument]);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HealSelf( script_state_t * pstate, ai_state_t * pself )
{
    // HealSelf()
    // This function gives life back to the character.
    // Values given as * 256
    // This does NOT remove [HEAL] enchants ( poisons )

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function heals the character, without setting the alert or modifying
    // the amount
    if ( pchr->alive )
    {
        iTmp = pchr->life + pstate->argument;
        if ( iTmp > pchr->lifemax ) iTmp = pchr->lifemax;
        if ( iTmp < 1 ) iTmp = 1;

        pchr->life = iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Equip( script_state_t * pstate, ai_state_t * pself )
{
    // Equip()
    // This function flags the character as being equipped.
    // This is used by equipment items when they are placed in the inventory

    SCRIPT_FUNCTION_BEGIN();

    // This function flags the character as being equipped
    pchr->isequipped = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasItemIDEquipped( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetHasItemIDEquipped( tmpargument = "item idsz" )
    // This function proceeds if the target already wearing a matching item

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if the target has a matching item equipped
    returncode = bfalse;
    sTmp = ChrList[pself->target].nextinpack;
    while ( sTmp != MAXCHR )
    {
        if ( sTmp != pself->index && ChrList[sTmp].isequipped && ( CapList[ChrList[sTmp].model].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument || CapList[ChrList[sTmp].model].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument ) )
        {
            returncode = btrue;
            sTmp = MAXCHR;
        }
        else
        {
            sTmp = ChrList[sTmp].nextinpack;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_OwnerToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // set_OwnerToTarget()
    // This function must be called before enchanting anything.
    // The owner is the character that pays the sustain costs and such for the enchantment

    SCRIPT_FUNCTION_BEGIN();

    pself->owner = pself->target;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToOwner( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToOwner()
    // This function sets the target to whoever was previously declared as the
    // owner.

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the target to the owner
    pself->target = pself->owner;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Frame( script_state_t * pstate, ai_state_t * pself )
{
    // set_Frame( tmpargument = "frame" )
    // This function sets the current .MD2 frame for the character...  Values are * 4

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pstate->argument & 3;
    iTmp = pstate->argument >> 2;
    chr_set_frame( pself->index, iTmp, sTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BreakPassage( script_state_t * pstate, ai_state_t * pself )
{
    // BreakPassage( tmpargument = "passage", tmpturn = "tile type", tmpdistance = "number of frames", tmpx = "animate ?", tmpy = "tile fx bits" )

    // This function makes the tiles fall away ( turns into damage terrain )
    // This function causes the tiles of a passage to increment if stepped on.
    // tmpx and tmpy are both set to the location of whoever broke the tile if
    // the function passed...

    SCRIPT_FUNCTION_BEGIN();

    returncode = break_passage( pstate, pstate->argument, pstate->turn & 0xFFFF, pstate->distance, pstate->x, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ReloadTime( script_state_t * pstate, ai_state_t * pself )
{
    // set_ReloadTime( tmpargument = "time" )
    // This function stops a character from being used for a while...  Used
    // by weapons to slow down their attack rate...  50 clicks per second...

    SCRIPT_FUNCTION_BEGIN();

    // This function makes weapons fire slower
    if ( pstate->argument > 0 ) pchr->reloadtime = pstate->argument;
    else pchr->reloadtime = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWideBlahID( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToWideBlahID( tmpargument = "idsz", tmpdistance = "blah bits" )
    // This function sets the target to a character that matches the description,
    // and who is located in the general vicinity of the character

    SCRIPT_FUNCTION_BEGIN();

    {
        TARGET_TYPE blahteam = ALL;
        if ( ( pstate->distance >> 2 ) & 1 )  blahteam = FRIEND;
        if ( (( pstate->distance >> 1 ) & 1) && blahteam == FRIEND ) blahteam = ALL;
        else if ((( pstate->distance >> 1 ) & 1)) blahteam = ENEMY;
        else returncode = bfalse;
        if (returncode)
        {
            returncode = bfalse;
            if (get_target(pself->index, WIDE, blahteam, ( ( pstate->distance >> 3 ) & 1 ),
                           ( ( pstate->distance ) & 1 ), pstate->argument, (( pstate->distance >> 4 ) & 1) ) != MAXCHR) returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PoofTarget( script_state_t * pstate, ai_state_t * pself )
{
    // PoofTarget()
    // This function removes the target from the game, failing if the
    // target is a player

    SCRIPT_FUNCTION_BEGIN();

    // This function makes the target go away
    returncode = bfalse;
    if ( !ChrList[pself->target].isplayer )
    {
        returncode = btrue;
        if ( pself->target == pself->index )
        {
            // Poof self later
            pself->poof_time = frame_wld + 1;
        }
        else
        {
            // Poof others now
            ChrList[pself->target].ai.poof_time = frame_wld;
            pself->target = pself->index;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChildDoActionOverride( script_state_t * pstate, ai_state_t * pself )
{
    // ChildDoActionOverride( tmpargument = action )

    // This function lets a character set the action of the last character
    // it spawned.  It also sets the current frame to the first frame of the
    // action ( no interpolation from last frame ).

    SCRIPT_FUNCTION_BEGIN();

    // This function starts a new action, if it is valid for the model
    // It will fail if the action is invalid
    returncode = bfalse;
    if ( pstate->argument < MAXACTION )
    {
        if ( MadList[ChrList[pself->child].inst.imad].actionvalid[pstate->argument] )
        {
            ChrList[pself->child].action = pstate->argument;
            ChrList[pself->child].inst.lip = 0;
            ChrList[pself->child].inst.frame = MadList[ChrList[pself->child].inst.imad].actionstart[pstate->argument];
            ChrList[pself->child].inst.lastframe = ChrList[pself->child].inst.frame;
            ChrList[pself->child].actionready = bfalse;
            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnPoof( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnPoof
    // This function makes a poof at the character's location.
    // The poof form and particle types are set in data.txt

    SCRIPT_FUNCTION_BEGIN();

    // This function makes a lovely little poof at the character's location
    spawn_poof( pself->index, pchr->model );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SpeedPercent( script_state_t * pstate, ai_state_t * pself )
{
    // set_SpeedPercent( tmpargument = "percent" )
    // This function acts like Run or Walk, except it allows the explicit
    // setting of the speed

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );
    pchr->maxaccel = pchr->maxaccel * pstate->argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildState( script_state_t * pstate, ai_state_t * pself )
{
    // set_ChildState( tmpargument = "state" )
    // This function lets a character set the state of the last character it
    // spawned

    SCRIPT_FUNCTION_BEGIN();

    ChrList[pself->child].ai.state = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedSizedParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedSizedParticle( tmpargument = "particle", tmpdistance = "vertex", tmpturn = "size" )
    // This function spawns a particle of the specific size attached to the
    // character.
    // For spell charging effects

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function spawns an attached particle, then sets its size
    tTmp = pself->index;
    if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

    tTmp = spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, pchr->turnleftright, pchr->model, pstate->argument, pself->index, pstate->distance, pchr->team, tTmp, 0, MAXCHR );
    if ( tTmp < maxparticles )
    {
        PrtList[tTmp].size = pstate->turn;
    }

    returncode = (tTmp != TOTALMAXPRT);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeArmor( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeArmor( tmpargument = "time" )
    // This function changes the character's armor.
    // set_s tmpargument as the old type and tmpx as the new type

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the character's armor type and returns the old type
    // as tmpargument and the new type as tmpx
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
    // This function sets the value displayed by the module timer...
    // For races and such...  50 clicks per second

    SCRIPT_FUNCTION_BEGIN();

    // This function turns the timer on, using the value for tmpargument
    timeron = btrue;
    timervalue = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FacingTarget( script_state_t * pstate, ai_state_t * pself )
{
    // _FacingTarget()
    // This function proceeds if the character is more or less facing its
    // target

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds only if the character is facing the target
    sTmp = ATAN2( ChrList[pself->target].ypos - pchr->ypos, ChrList[pself->target].xpos - pchr->xpos ) * 0xFFFF / ( TWO_PI );
    sTmp += 32768 - pchr->turnleftright;
    returncode = ( sTmp > 55535 || sTmp < 10000 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySoundVolume( script_state_t * pstate, ai_state_t * pself )
{
    int iTmp;
    int volume;

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the volume of a sound and plays it
    if ( mixeron && moduleactive && pstate->distance >= 0 )
    {
        volume = pstate->distance;
        iTmp = -1;
        if ( pstate->argument >= 0 && pstate->argument < MAXWAVE )
        {
            iTmp = sound_play_chunk( pchr->oldx, pchr->oldy, CapList[pchr->model].wavelist[pstate->argument] );
        }

        if ( -1 != iTmp )
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

    // This function spawns a particle attached to the character, facing the
    // same direction given by tmpturn

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function spawns an attached particle with facing
    tTmp = pself->index;
    if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

    tTmp = spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, pstate->turn & 0xFFFF, pchr->model, pstate->argument, pself->index, pstate->distance, pchr->team, tTmp, 0, MAXCHR );

    returncode = (tTmp != TOTALMAXPRT);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIsOdd( script_state_t * pstate, ai_state_t * pself )
{
    // _StateIsOdd()
    // This function proceeds if the character's state is 1, 3, 5, 7, etc.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->state & 1 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToDistantEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToDistantEnemy( tmpdistance = "distance" )
    // This function finds a character within a certain distance of the
    // character, failing if there are none

    SCRIPT_FUNCTION_BEGIN();

    // This function finds an enemy, within a certain distance to the character, and
    // proceeds only if there is one
    returncode = bfalse;
    if (get_target(pself->index, pstate->distance, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse) != MAXCHR) returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Teleport( script_state_t * pstate, ai_state_t * pself )
{
    // Teleport( tmpx = "x", tmpy = "y" )
    // This function teleports the character to a new location, failing if
    // the location is blocked or off the map

    SCRIPT_FUNCTION_BEGIN();

    // This function teleports the character to the X, Y location, failing if the
    // location is off the map or blocked
    returncode = bfalse;
    if ( pstate->x > EDGE && pstate->y > EDGE && pstate->x < mesh.info.edge_x - EDGE && pstate->y < mesh.info.edge_y - EDGE )
    {
        float x_old, y_old;

        x_old = pchr->xpos;
        y_old = pchr->ypos;
        pchr->xpos = pstate->x;
        pchr->ypos = pstate->y;
        if ( 0 == __chrhitawall( pself->index ) )
        {
            // Yeah!  It worked!
            detach_character_from_mount( pself->index, btrue, bfalse );
            pchr->oldx = pchr->xpos;
            pchr->oldy = pchr->ypos;
            returncode = btrue;
        }
        else
        {
            // No it didn't...
            pchr->xpos = x_old;
            pchr->ypos = y_old;
            returncode = bfalse;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveStrengthToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveStrengthToTarget()
    // Permanently boost the target's strength

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList[pself->target].strength, PERFECTSTAT, &iTmp );
        ChrList[pself->target].strength += iTmp;
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
    if ( ChrList[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList[pself->target].wisdom, PERFECTSTAT, &iTmp );
        ChrList[pself->target].wisdom += iTmp;
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
    if ( ChrList[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList[pself->target].intelligence, PERFECTSTAT, &iTmp );
        ChrList[pself->target].intelligence += iTmp;
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
    if ( ChrList[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList[pself->target].dexterity, PERFECTSTAT, &iTmp );
        ChrList[pself->target].dexterity += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveLifeToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveLifeToTarget()
    // Permanently boost the target's life

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( LOWSTAT, ChrList[pself->target].lifemax, PERFECTBIG, &iTmp );
        ChrList[pself->target].lifemax += iTmp;
        if ( iTmp < 0 )
        {
            getadd( 1, ChrList[pself->target].life, PERFECTBIG, &iTmp );
        }

        ChrList[pself->target].life += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveManaToTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GiveManaToTarget()
    // Permanently boost the target's mana

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList[pself->target].manamax, PERFECTBIG, &iTmp );
        ChrList[pself->target].manamax += iTmp;
        if ( iTmp < 0 )
        {
            getadd( 0, ChrList[pself->target].mana, PERFECTBIG, &iTmp );
        }

        ChrList[pself->target].mana += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowMap( script_state_t * pstate, ai_state_t * pself )
{
    // ShowMap()
    // This function shows the module's map.
    // Fails if map already visible

    SCRIPT_FUNCTION_BEGIN();
    if ( mapon )  returncode = bfalse;

    mapon = mapvalid;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowYouAreHere( script_state_t * pstate, ai_state_t * pself )
{
    // ShowYouAreHere()
    // This function shows the blinking white blip on the map that represents the
    // camera location

    SCRIPT_FUNCTION_BEGIN();

    youarehereon = mapvalid;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowBlipXY( script_state_t * pstate, ai_state_t * pself )
{
    // ShowBlipXY( tmpx = "x", tmpy = "y", tmpargument = "color" )

    // This function draws a blip on the map, and must be done each update

    SCRIPT_FUNCTION_BEGIN();

    // Add a blip
    if ( numblip < MAXBLIP )
    {
        if ( pstate->x > 0 && pstate->x < mesh.info.edge_x && pstate->y > 0 && pstate->y < mesh.info.edge_y )
        {
            if ( pstate->argument < NUMBAR && pstate->argument >= 0 )
            {
                blipx[numblip] = pstate->x * MAPSIZE / mesh.info.edge_x;
                blipy[numblip] = pstate->y * MAPSIZE / mesh.info.edge_y;
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
    // This function gives some life back to the target.
    // Values are * 256 Any enchantments that are removed by [HEAL], like poison, go away

    int iTmp;
    IDSZ test;

    SCRIPT_FUNCTION_BEGIN();
    if ( ChrList[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 1, ChrList[pself->target].life, ChrList[pself->target].lifemax, &iTmp );
        ChrList[pself->target].life += iTmp;
        // Check all enchants to see if they are removed
        iTmp = ChrList[pself->target].firstenchant;

        while ( iTmp != MAXENCHANT )
        {
            test = Make_IDSZ( "HEAL" );  // [HEAL]
            sTmp = EncList[iTmp].nextenchant;
            if ( test == EveList[EncList[iTmp].eve].removedbyidsz )
            {
                remove_enchant( iTmp );
            }

            iTmp = sTmp;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PumpTarget( script_state_t * pstate, ai_state_t * pself )
{
    // PumpTarget( tmpargument = "amount" )
    // This function gives some mana back to the target.
    // Values are * 256

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // Give some mana to the target
    if ( ChrList[pself->target].alive )
    {
        iTmp = pstate->argument;
        getadd( 0, ChrList[pself->target].mana, ChrList[pself->target].manamax, &iTmp );
        ChrList[pself->target].mana += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // CostAmmo()
    // This function costs the character 1 point of ammo

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
    // This function makes the names of similar objects known.
    // Checks all 6 IDSZ types to make sure they match.

    int tTmp;
    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = 0;
    while ( iTmp < MAXCHR )
    {
        sTmp = btrue;
        tTmp = 0;

        while ( tTmp < IDSZ_COUNT )
        {
            if ( CapList[pchr->model].idsz[tTmp] != CapList[ChrList[iTmp].model].idsz[tTmp] )
            {
                sTmp = bfalse;
            }

            tTmp++;
        }
        if ( sTmp )
        {
            ChrList[iTmp].nameknown = btrue;
        }

        iTmp++;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedHolderParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnAttachedHolderParticle( tmpargument = "particle", tmpdistance = "vertex" )

    // This function spawns a particle attached to the character's holder, or to the character if no holder

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    tTmp = pself->index;
    if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

    tTmp = spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, pchr->turnleftright, pchr->model, pstate->argument, tTmp, pstate->distance, pchr->team, tTmp, 0, MAXCHR );

    returncode = (tTmp != TOTALMAXPRT);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetReloadTime( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetReloadTime( tmpargument = "time" )

    // This function sets the target's reload time
    // This function stops the target from attacking for a while.

    SCRIPT_FUNCTION_BEGIN();
    if ( pstate->argument > 0 )
        ChrList[pself->target].reloadtime = pstate->argument;
    else ChrList[pself->target].reloadtime = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogLevel( script_state_t * pstate, ai_state_t * pself )
{
    // set_FogLevel( tmpargument = "level" )
    // This function sets the level of the module's fog.
    // Values are * 10
    //!!BAD!! DOESN'T WORK !!BAD!!

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( pstate->argument / 10.0f ) - fogtop;
    fogtop += fTmp;
    fogdistance += fTmp;
    fogon = fogallowed;
    if ( fogdistance < 1.0f )  fogon = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_FogLevel( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = get_FogLevel()
    // This function sets tmpargument to the level of the module's fog...
    //  Values are * 10

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = fogtop * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogTAD( script_state_t * pstate, ai_state_t * pself )
{
    // This function sets the color of the module's fog.
    // TAD stands for <turn, argument, distance> == <red, green, blue>.
    // Makes sense, huh?
    //!!BAD!! DOESN'T WORK !!BAD!!

    SCRIPT_FUNCTION_BEGIN();

    // This function changes the fog color
    fogred = pstate->turn;
    foggrn = pstate->argument;
    fogblu = pstate->distance;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogBottomLevel( script_state_t * pstate, ai_state_t * pself )
{
    // set_FogBottomLevel( tmpargument = "level" )

    // This function sets the level of the module's fog.
    // Values are * 10

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( pstate->argument / 10.0f ) - fogbottom;
    fogbottom += fTmp;
    fogdistance -= fTmp;
    fogon = fogallowed;
    if ( fogdistance < 1.0f )  fogon = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_FogBottomLevel( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = get_FogBottomLevel()

    // This function sets tmpargument to the level of the module's fog.
    //  Values are * 10

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = fogbottom * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CorrectActionForHand( script_state_t * pstate, ai_state_t * pself )
{
    // CorrectActionForHand( tmpargument = "action" )
    // This function changes tmpargument according to which hand the character
    // is held in It turns ZA into ZA, ZB, ZC, or ZD...
    // USAGE:  wizards casting spells

    SCRIPT_FUNCTION_BEGIN();
    if ( pchr->attachedto != MAXCHR )
    {
        if ( pchr->inwhichhand == SLOT_LEFT )
        {
            // A or B
            pstate->argument = pstate->argument + ( rand() & 1 );
        }
        else
        {
            // C or D
            pstate->argument = pstate->argument + 2 + ( rand() & 1 );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsMounted( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsMounted()
    // This function proceeds if the target is riding a mount

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ChrList[pself->target].attachedto != MAXCHR )
    {
        returncode = ChrList[ChrList[pself->target].attachedto].ismount;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SparkleIcon( script_state_t * pstate, ai_state_t * pself )
{
    // SparkleIcon( tmpargument = "color" )
    // This function starts little sparklies going around the character's icon

    SCRIPT_FUNCTION_BEGIN();
    if ( pstate->argument < NUMBAR && pstate->argument > -1 )
    {
        pchr->sparkle = pstate->argument;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnsparkleIcon( script_state_t * pstate, ai_state_t * pself )
{
    // UnsparkleIcon()
    // This function stops little sparklies going around the character's icon

    SCRIPT_FUNCTION_BEGIN();

    // This function stops the blippie thing
    pchr->sparkle = NOSPARKLE;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TileXY( script_state_t * pstate, ai_state_t * pself )
{
    // get_TileXY( tmpx = "x", tmpy = "y" )
    // This function sets tmpargument to the tile type at the specified
    // coordinates

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    iTmp = mesh_get_tile( pstate->x, pstate->y );
    if ( iTmp != INVALID_TILE )
    {
        returncode = btrue;
        pstate->argument = mesh.mem.tile_list[iTmp].img & 0xFF;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TileXY( script_state_t * pstate, ai_state_t * pself )
{
    // set_ShadowSize( tmpargument = "tile type", tmpx = "x", tmpy = "y" )
    // This function changes the tile type at the specified coordinates

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    iTmp = mesh_get_tile( pstate->x, pstate->y );
    if ( iTmp != INVALID_TILE )
    {
        returncode = btrue;
        mesh.mem.tile_list[iTmp].img = ( pstate->argument & 0xFF );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ShadowSize( script_state_t * pstate, ai_state_t * pself )
{
    // set_ShadowSize( tmpargument = "size" )
    // This function makes the character's shadow bigger or smaller

    SCRIPT_FUNCTION_BEGIN();

    pchr->shadowsize = pstate->argument * pchr->fat;
    pchr->shadowsizesave = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OrderTarget( script_state_t * pstate, ai_state_t * pself )
{
    // OrderTarget( tmpargument = "order" )
    // This function issues an order to the given target
    // Be careful in using this, always checking IDSZ first

    SCRIPT_FUNCTION_BEGIN();

    ChrList[pself->target].ai.order   = pstate->argument;
    ChrList[pself->target].ai.rank    = 0;
    ChrList[pself->target].ai.alert  |= ALERTIF_ORDERED;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverIsInPassage( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToWhoeverIsInPassage()
    // This function sets the target to whoever is blocking the given passage
    // This function lets passage rectangles be used as event triggers

    SCRIPT_FUNCTION_BEGIN();

    sTmp = who_is_blocking_passage( pstate->argument );
    returncode = bfalse;
    if ( sTmp != MAXCHR )
    {
        pself->target = sTmp;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CharacterWasABook( script_state_t * pstate, ai_state_t * pself )
{
    // _CharacterWasABook()
    // This function proceeds if the base model is the same as the current
    // model or if the base model is SPELLBOOK
    // USAGE: USED BY THE MORPH SPELL. Not much use elsewhere
    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if the base model is the same as the current
    // model or if the base model is SPELLBOOK
    returncode = ( pchr->basemodel == SPELLBOOK ||
                   pchr->basemodel == pchr->model );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_EnchantBoostValues( script_state_t * pstate, ai_state_t * pself )
{
    // set_EnchantBoostValues( tmpargument = "owner mana regen", tmpdistance = "owner life regen", tmpx = "target mana regen", tmpy = "target life regen" )
    // This function sets the mana and life drains for the last enchantment
    // spawned by this character.
    // Values are * 256

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // This function sets the boost values for the last enchantment
    iTmp = pchr->undoenchant;
    if ( iTmp != MAXENCHANT )
    {
        EncList[iTmp].ownermana = pstate->argument;
        EncList[iTmp].ownerlife = pstate->distance;
        EncList[iTmp].targetmana = pstate->x;
        EncList[iTmp].targetlife = pstate->y;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnCharacterXYZ( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnCharacterXYZ( tmpx = "x", tmpy = "y", tmpdistance = "z", tmpturn = "turn" )
    // This function spawns a character of the same type at a specific location, failing if x,y,z is invalid

    SCRIPT_FUNCTION_BEGIN();

    sTmp = spawn_one_character( pstate->x, pstate->y, pstate->distance, pchr->model, pchr->team, 0, pstate->turn & 0xFFFF, NULL, MAXCHR );
    returncode = bfalse;
    if ( sTmp < MAXCHR )
    {
        if ( __chrhitawall( sTmp ) )
        {
            free_one_character( sTmp );
            sTmp = MAXCHR;
        }
        else
        {
            ChrList[sTmp].iskursed = bfalse;

            pself->child = sTmp;

            ChrList[sTmp].ai.passage = pself->passage;
            ChrList[sTmp].ai.owner   = pself->owner;
        }
    }

    returncode = (sTmp != MAXCHR);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactCharacterXYZ( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnCharacterXYZ( tmpargument = "slot", tmpx = "x", tmpy = "y", tmpdistance = "z", tmpturn = "turn" )
    // This function spawns a character at a specific location, using a
    // specific model type, failing if x,y,z is invalid

    // DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS,
    // AS THE MODEL SLOTS MAY VARY FROM MODULE TO MODULE...

    SCRIPT_FUNCTION_BEGIN();

    sTmp = spawn_one_character( pstate->x, pstate->y, pstate->distance, pstate->argument, pchr->team, 0, pstate->turn & 0xFFFF, NULL, MAXCHR );
    returncode = bfalse;
    if ( sTmp < MAXCHR )
    {
        if ( __chrhitawall( sTmp ) )
        {
            free_one_character( sTmp );
            sTmp = MAXCHR;
        }
        else
        {
            ChrList[sTmp].iskursed = bfalse;

            pself->child = sTmp;

            ChrList[sTmp].ai.passage = pself->passage;
            ChrList[sTmp].ai.owner   = pself->owner;
        }
    }

    returncode = (sTmp != MAXCHR);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTargetClass( script_state_t * pstate, ai_state_t * pself )
{
    // ChangeTargetClass( tmpargument = "slot" )

    // This function changes the target character's model slot.
    // DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS, AS THE MODEL SLOTS MAY VARY FROM
    // MODULE TO MODULE.
    // USAGE: This is intended as a way to incorporate more player classes into the game...

    SCRIPT_FUNCTION_BEGIN();

    change_character( pself->target, pstate->argument, 0, LEAVEALL );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlayFullSound( script_state_t * pstate, ai_state_t * pself )
{
    // PlayFullSound( tmpargument = "sound", tmpdistance = "frequency" )
    // This function plays one of the character's sounds...  The sound will
    // be heard at full volume by all players

    SCRIPT_FUNCTION_BEGIN();

    // This function plays a sound loud for everyone...  Victory music
    if ( moduleactive && pstate->argument >= 0 && pstate->argument < MAXWAVE )
    {
        sound_play_chunk( gCamera.trackx, gCamera.tracky, CapList[pchr->model].wavelist[pstate->argument] );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactChaseParticle( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnExactChaseParticle( tmpargument = "particle", tmpx = "x", tmpy = "y", tmpdistance = "z" )
    // This function spawns a particle at a specific x, y, z position,
    // that will home in on the character's target

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    tTmp = pself->index;
    if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

    tTmp = spawn_one_particle( pstate->x, pstate->y, pstate->distance, pchr->turnleftright, pchr->model, pstate->argument, MAXCHR, 0, pchr->team, tTmp, 0, MAXCHR );
    if ( tTmp < maxparticles )
    {
        PrtList[tTmp].target = pself->target;
    }

    returncode = (tTmp != TOTALMAXPRT);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CreateOrder( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = CreateOrder( tmpx = "value1", tmpy = "value2", tmpargument = "order" )

    // This function compresses tmpx, tmpy, tmpargument ( 0 - 15 ), and the
    // character's target into tmpargument...  This new tmpargument can then
    // be issued as an order to teammates...  TranslateOrder will undo the
    // compression

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->target << 24;
    sTmp |= ( ( pstate->x >> 6 ) & 1023 ) << 14;
    sTmp |= ( ( pstate->y >> 6 ) & 1023 ) << 4;
    sTmp |= ( pstate->argument & 15 );
    pstate->argument = sTmp;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OrderSpecialID( script_state_t * pstate, ai_state_t * pself )
{
    // OrderSpecialID( tmpargument = "compressed order", tmpdistance = "idsz" )
    // This function orders all characters with the given special IDSZ...
    // Note that the IDSZ is set in tmpdistance...

    SCRIPT_FUNCTION_BEGIN();

    issue_special_order( pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkurseTargetInventory( script_state_t * pstate, ai_state_t * pself )
{
    // UnkurseTargetInventory()
    // This function unkurses all items held and in the pockets of the target

    SCRIPT_FUNCTION_BEGIN();

    // This function unkurses every item a character is holding
    sTmp = ChrList[pself->target].holdingwhich[0];
    ChrList[sTmp].iskursed = bfalse;
    sTmp = ChrList[pself->target].holdingwhich[1];
    ChrList[sTmp].iskursed = bfalse;
    sTmp = ChrList[pself->target].nextinpack;

    while ( sTmp != MAXCHR )
    {
        ChrList[sTmp].iskursed = bfalse;
        sTmp = ChrList[sTmp].nextinpack;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsSneaking( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsSneaking()
    // This function proceeds if the target is doing ACTIONWA or ACTIONDA

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if the target is doing ACTIONDA or ACTIONWA
    returncode = ( ChrList[pself->target].action == ACTIONDA || ChrList[pself->target].action == ACTIONWA );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropItems( script_state_t * pstate, ai_state_t * pself )
{
    // DropItems()
    // This function drops all of the items the character is holding

    SCRIPT_FUNCTION_BEGIN();

    // This function drops all of the character's items
    drop_all_items( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RespawnTarget( script_state_t * pstate, ai_state_t * pself )
{
    // RespawnTarget()
    // This function respawns the target at its current location

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->target;
    ChrList[sTmp].oldx = ChrList[sTmp].xpos;
    ChrList[sTmp].oldy = ChrList[sTmp].ypos;
    ChrList[sTmp].oldz = ChrList[sTmp].zpos;
    respawn_character( sTmp );
    ChrList[sTmp].xpos = ChrList[sTmp].oldx;
    ChrList[sTmp].ypos = ChrList[sTmp].oldy;
    ChrList[sTmp].zpos = ChrList[sTmp].oldz;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDoActionSetFrame( script_state_t * pstate, ai_state_t * pself )
{
    // TargetDoActionSetFrame( tmpargument = "action" )
    // This function starts the target doing the given action, and also sets
    // the starting frame to the first of the animation ( so there is no
    // interpolation 'cause it looks awful in some circumstances )
    // It will fail if the action is invalid

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument < MAXACTION )
    {
        if ( MadList[ChrList[pself->target].inst.imad].actionvalid[pstate->argument] )
        {
            ChrList[pself->target].action = pstate->argument;
            ChrList[pself->target].inst.lip = 0;
            ChrList[pself->target].inst.frame = MadList[ChrList[pself->target].inst.imad].actionstart[pstate->argument];
            ChrList[pself->target].inst.lastframe = ChrList[pself->target].inst.frame;
            ChrList[pself->target].actionready = bfalse;
            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanSeeInvisible( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetCanSeeInvisible()
    // This function proceeds if the target can see invisible

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if the target can see invisible
    returncode = ChrList[pself->target].canseeinvisible;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestBlahID( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToNearestBlahID( tmpargument = "idsz", tmpdistance = "blah bits" )

    // This function finds the NEAREST ( exact ) character that fits the given
    // parameters, failing if it finds none

    SCRIPT_FUNCTION_BEGIN();

    {
        TARGET_TYPE blahteam = NONE;
        returncode = bfalse;
        if ( ( pstate->distance >> 2 ) & 1 )  blahteam = FRIEND;
        if ( (( pstate->distance >> 1 ) & 1) && blahteam == FRIEND ) blahteam = ALL;
        else if ((( pstate->distance >> 1 ) & 1)) blahteam = ENEMY;
        if (blahteam != NONE)
        {
            if (get_target(pself->index, NEAREST, blahteam, ( ( pstate->distance >> 3 ) & 1 ),
                           ( ( pstate->distance ) & 1 ), pstate->argument, (( pstate->distance >> 4 ) & 1) ) != MAXCHR) returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestEnemy( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToNearestEnemy()
    // This function finds the NEAREST ( exact ) enemy, failing if it finds none

    SCRIPT_FUNCTION_BEGIN();

    returncode = (MAXCHR != get_target(pself->index, 0, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestFriend( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToNearestFriend()
    // This function finds the NEAREST ( exact ) friend, failing if it finds none

    SCRIPT_FUNCTION_BEGIN();

    returncode = (MAXCHR != get_target(pself->index, 0, FRIEND, bfalse, bfalse, IDSZ_NONE, bfalse ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestLifeform( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToNearestLifeform()

    // This function finds the NEAREST ( exact ) friend or enemy, failing if it
    // finds none

    SCRIPT_FUNCTION_BEGIN();

    returncode = (MAXCHR != get_target(pself->index, 0, ALL, bfalse, bfalse, IDSZ_NONE, bfalse ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashPassage( script_state_t * pstate, ai_state_t * pself )
{
    // FlashPassage( tmpargument = "passage", tmpdistance = "color" )

    // This function makes the given passage fully lit.
    // Usage: For debug purposes

    SCRIPT_FUNCTION_BEGIN();

    // This function makes the passage light or dark...  For debug...
    flash_passage( pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FindTileInPassage( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx, tmpy = FindTileInPassage( tmpargument = "passage", tmpdistance = "tile type", tmpx, tmpy )

    // This function finds all tiles of the specified type that lie within the
    // given passage.  Call multiple times to find multiple tiles.  tmpx and
    // tmpy will be set to the middle of the found tile if one is found, or
    // both will be set to 0 if no tile is found.
    // tmpx and tmpy are required and set on return

    SCRIPT_FUNCTION_BEGIN();

    // This function finds the next tile in the passage,
    returncode = find_tile_in_passage( pstate, pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HeldInLeftHand( script_state_t * pstate, ai_state_t * pself )
{
    // _HeldInLeftHand()
    // This function passes if another character is holding the character in its
    // left hand.
    // Usage: Used mostly by enchants that target the item of the other hand

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if the character is in the left hand of another
    // character
    returncode = bfalse;
    sTmp = pchr->attachedto;
    if ( sTmp != MAXCHR )
    {
        returncode = ( ChrList[sTmp].holdingwhich[0] == pself->index );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotAnItem( script_state_t * pstate, ai_state_t * pself )
{
    // NotAnItem()
    // This function makes the character a non-item character.
    // Usage: Used for spells that summon creatures

    SCRIPT_FUNCTION_BEGIN();

    pchr->isitem = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildAmmo( script_state_t * pstate, ai_state_t * pself )
{
    // set_ChildAmmo( tmpargument = "none" )
    // This function sets the ammo of the last character spawned by this character

    SCRIPT_FUNCTION_BEGIN();

    ChrList[pself->child].ammo = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitVulnerable( script_state_t * pstate, ai_state_t * pself )
{
    // _HitVulnerable()
    // This function proceeds if the character was hit by a weapon of its
    // vulnerability IDSZ.
    // For example, a werewolf gets hit by a [SILV] bullet.

    SCRIPT_FUNCTION_BEGIN();

    // This function proceeds if the character was hit by a weapon with the
    // correct vulnerability IDSZ...  [SILV] for Werewolves...
    returncode = ( 0 != ( pself->alert & ALERTIF_HITVULNERABLE ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsFlying( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsFlying()
    // This function proceeds if the character target is flying

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList[pself->target].flyheight > 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IdentifyTarget( script_state_t * pstate, ai_state_t * pself )
{
    // IdentifyTarget()
    // This function reveals the target's name, ammo, and usage
    // Proceeds if the target was unknown

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    sTmp = pself->target;
    if ( ChrList[sTmp].ammomax != 0 )  ChrList[sTmp].ammoknown = btrue;
    if ( ChrList[sTmp].name[0] != 'B' ||
            ChrList[sTmp].name[1] != 'l' ||
            ChrList[sTmp].name[2] != 'a' ||
            ChrList[sTmp].name[3] != 'h' ||
            ChrList[sTmp].name[4] != 0 )
    {
        returncode = !ChrList[sTmp].nameknown;
        ChrList[sTmp].nameknown = btrue;
    }

    CapList[ChrList[sTmp].model].usageknown = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BeatModule( script_state_t * pstate, ai_state_t * pself )
{
    // BeatModule()
    // This function displays the Module Ended message

    SCRIPT_FUNCTION_BEGIN();

    beatmodule = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EndModule( script_state_t * pstate, ai_state_t * pself )
{
    // EndModule()
    // This function presses the Escape key

    SCRIPT_FUNCTION_BEGIN();

    // This function presses the Escape key
    if ( NULL != keyb.state_ptr )
    {
        keyb.state_ptr[SDLK_ESCAPE] = 1;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableExport( script_state_t * pstate, ai_state_t * pself )
{
    // DisableExport()
    // This function turns export off

    SCRIPT_FUNCTION_BEGIN();

    exportvalid = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableExport( script_state_t * pstate, ai_state_t * pself )
{
    // EnableExport()
    // This function turns export on

    SCRIPT_FUNCTION_BEGIN();

    exportvalid = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetState( script_state_t * pstate, ai_state_t * pself )
{
    // tmpargument = get_TargetState()
    // This function sets tmpargument to the state of the target

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = ChrList[pself->target].ai.state;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Equipped( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This proceeds if the character is equipped
    returncode = bfalse;
    if ( pchr->isequipped ) returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropTargetMoney( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This function drops some of the target's money
    drop_money( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetContent( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This sets tmpargument to the current Target's content value
    pstate->argument = ChrList[pself->target].ai.content;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropTargetKeys( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This function makes the Target drops keys in inventory (Not inhand)
    drop_keys( pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinTeam( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)
    switch_team( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetJoinTeam( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This makes the Target join a Team specified in tmpargument (A = 0, 23 = Z, etc.)
    switch_team( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearMusicPassage( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This clears the music for a specified passage
    passagemusic[pstate->argument] = -1;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearEndMessage( script_state_t * pstate, ai_state_t * pself )
{
    // ClearEndMessage()
    // This function empties the end-module text buffer

    SCRIPT_FUNCTION_BEGIN();

    endtext[0] = '\0';
    endtextwrite = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddEndMessage( script_state_t * pstate, ai_state_t * pself )
{
    // AddEndMessage( tmpargument = "message" )
    // This function appends a message to the end-module text buffer

    SCRIPT_FUNCTION_BEGIN();

    append_end_text( pstate,  MadList[pchr->inst.imad].msgstart + pstate->argument, pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlayMusic( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This function begins playing a new track of music
    if ( musicvalid && ( songplaying != pstate->argument ) )
    {
        sound_play_song( pstate->argument, pstate->distance, -1 );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_MusicPassage( script_state_t * pstate, ai_state_t * pself )
{
    // set_MusicPassage( tmpargument = "passage", tmpturn = "type", tmpdistance = "repetitions" )

    // This function makes the given passage play music if a player enters it
    // tmpargument is the passage to set and tmpdistance is the music track to play...

    SCRIPT_FUNCTION_BEGIN();

    passagemusic[pstate->argument] = pstate->distance;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeCrushInvalid( script_state_t * pstate, ai_state_t * pself )
{
    // MakeCrushInvalid()
    // This function makes doors unable to close on this object

    SCRIPT_FUNCTION_BEGIN();

    pchr->canbecrushed = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopMusic( script_state_t * pstate, ai_state_t * pself )
{
    // StopMusic()
    // This function stops the interactive music

    SCRIPT_FUNCTION_BEGIN();

    sound_stop_song();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashVariable( script_state_t * pstate, ai_state_t * pself )
{
    // FlashVariable( tmpargument = "amount" )

    // This function makes the character flash according to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    flash_character( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateUp( script_state_t * pstate, ai_state_t * pself )
{
    // AccelerateUp( tmpargument = "acc z" )
    // This function makes the character accelerate up and down

    SCRIPT_FUNCTION_BEGIN();

    pchr->zvel += pstate->argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashVariableHeight( script_state_t * pstate, ai_state_t * pself )
{
    // FlashVariableHeight( tmpturn = "intensity bottom", tmpx = "bottom", tmpdistance = "intensity top", tmpy = "top" )
    // This function makes the character flash, feet one color, head another...

    SCRIPT_FUNCTION_BEGIN();

    flash_character_height( pself->index, pstate->turn & 0xFFFF, pstate->x, pstate->distance, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageTime( script_state_t * pstate, ai_state_t * pself )
{
    // set_DamageTime( tmpargument = "time" )
    // This function makes the character invincible for a little while

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
    // _TargetIsAMount()
    // This function passes if the Target is a mountable character

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList[pself->target].ismount;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAPlatform( script_state_t * pstate, ai_state_t * pself )
{
    // _TargetIsAPlatform()
    // This function passes if the Target is a platform character

    SCRIPT_FUNCTION_BEGIN();

    returncode = ChrList[pself->target].platform;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddStat( script_state_t * pstate, ai_state_t * pself )
{
    // AddStat()
    // This function turns on an NPC's status display

    SCRIPT_FUNCTION_BEGIN();

    if ( !pchr->staton )
    {
        statlist_add( pself->index );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisenchantTarget( script_state_t * pstate, ai_state_t * pself )
{
    // DisenchantTarget()
    // This function removes all enchantments on the Target character, proceeding
    // if there were any, failing if not

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( ChrList[pself->target].firstenchant != MAXENCHANT );
    disenchant_character( pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisenchantAll( script_state_t * pstate, ai_state_t * pself )
{
    // DisenchantAll()
    // This function removes all enchantments in the game

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = 0;
    while ( iTmp < MAXENCHANT )
    {
        remove_enchant( iTmp );
        iTmp++;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_VolumeNearestTeammate( script_state_t * pstate, ai_state_t * pself )
{
    // set_VolumeNearestTeammate( tmpargument = "sound", tmpdistance = "distance" )
    // This function lets insects buzz correctly...  The closest Team member
    // is used to determine the overall sound level.

    SCRIPT_FUNCTION_BEGIN();

    /*PORT
    if(moduleactive && pstate->distance >= 0)
    {
    // Find the closest Teammate
    iTmp = 10000;
    sTmp = 0;
    while(sTmp < MAXCHR)
    {
    if(ChrList[sTmp].on && ChrList[sTmp].alive && ChrList[sTmp].Team == pchr->Team)
    {
    distance = ABS(gCamera.trackx-ChrList[sTmp].oldx)+ABS(gCamera.tracky-ChrList[sTmp].oldy);
    if(distance < iTmp)  iTmp = distance;
    }
    sTmp++;
    }
    distance=iTmp+pstate->distance;
    volume = -distance;
    volume = volume<<VOLSHIFT;
    if(volume < VOLMIN) volume = VOLMIN;
    iTmp = CapList[pchr->model].wavelist[pstate->argument];
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
    // This function makes a passage behave as a shop area, as long as the
    // character is alive.

    SCRIPT_FUNCTION_BEGIN();

    add_shop_passage( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetPayForArmor( script_state_t * pstate, ai_state_t * pself )
{
    // tmpx, tmpy = TargetPayForArmor( tmpargument = "skin" )

    // This function costs the Target the appropriate amount of money for the
    // given armor type.  Passes if the character has enough, and fails if not.
    // Does trade-in bonus automatically.  tmpy is always set to cost of requested
    // skin tmpx is set to amount needed after trade-in ( 0 for pass ).

    int tTmp;
    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pself->target;   // The Target
    tTmp = ChrList[sTmp].model;           // The Target's model
    iTmp =  CapList[tTmp].skincost[pstate->argument&3];
    pstate->y = iTmp;                // Cost of new skin
    iTmp -= CapList[tTmp].skincost[ChrList[sTmp].skin];  // Refund
    if ( iTmp > ChrList[sTmp].money )
    {
        // Not enough...
        pstate->x = iTmp - ChrList[sTmp].money;  // Amount needed
        returncode = bfalse;
    }
    else
    {
        // Pay for it...  Cost may be negative after refund...
        ChrList[sTmp].money -= iTmp;
        if ( ChrList[sTmp].money > MAXMONEY )  ChrList[sTmp].money = MAXMONEY;

        pstate->x = 0;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinEvilTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinEvilTeam()
    // This function adds the character to the evil Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, EVILTEAM );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinNullTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinNullTeam()
    // This function adds the character to the null Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, NULLTEAM );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinGoodTeam( script_state_t * pstate, ai_state_t * pself )
{
    // JoinGoodTeam()
    // This function adds the character to the good Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, GOODTEAM );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PitsKill( script_state_t * pstate, ai_state_t * pself )
{
    // PitsKill()
    // This function activates pit deaths for when characters fall below a
    // certain altitude...

    SCRIPT_FUNCTION_BEGIN();

    pitskill = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToPassageID( script_state_t * pstate, ai_state_t * pself )
{
    // set_TargetToPassageID( tmpargument = "passage", tmpdistance = "idsz" )
    // This function finds a character who is both in the passage and who has
    // an item with the given IDSZ

    SCRIPT_FUNCTION_BEGIN();

    sTmp = who_is_blocking_passage_ID( pstate->argument, pstate->distance );
    returncode = bfalse;
    if ( sTmp != MAXCHR )
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
    // This function makes the name of an item/character unknown.
    // Usage: Use if you have subspawning of creatures from a book...

    SCRIPT_FUNCTION_BEGIN();

    pchr->nameknown = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactParticleEndSpawn( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnExactParticleEndSpawn( tmpargument = "particle", tmpturn = "state", tmpx = "x", tmpy = "y", tmpdistance = "z" )

    // This function spawns a particle at a specific x, y, z position.
    // When the particle ends, a character is spawned at its final location.
    // The character is the same type of whatever spawned the particle.

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    tTmp = pself->index;
    if ( pchr->attachedto != MAXCHR )  tTmp = pchr->attachedto;

    tTmp = spawn_one_particle( pstate->x, pstate->y, pstate->distance, pchr->turnleftright, pchr->model, pstate->argument, MAXCHR, 0, pchr->team, tTmp, 0, MAXCHR );
    if ( tTmp != maxparticles )
    {
        PrtList[tTmp].spawncharacterstate = pstate->turn;
    }

    returncode = (tTmp != TOTALMAXPRT);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnPoofSpeedSpacingDamage( script_state_t * pstate, ai_state_t * pself )
{
    // SpawnPoofSpeedSpacingDamage( tmpx = "xy speed", tmpy = "xy spacing", tmpargument = "damage" )

    // This function makes a lovely little poof at the character's location,
    // adjusting the xy speed and spacing and the base damage first
    // Temporarily adjust the values for the particle type

    int tTmp;
    int iTmp;
    IDSZ test;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pchr->model;
    sTmp = MadList[sTmp].prtpip[CapList[sTmp].gopoofprttype];
    iTmp = PipList[sTmp].xyvelbase;
    tTmp = PipList[sTmp].xyspacingbase;
    test = PipList[sTmp].damagebase;
    PipList[sTmp].xyvelbase = pstate->x;
    PipList[sTmp].xyspacingbase = pstate->y;
    PipList[sTmp].damagebase = pstate->argument;
    spawn_poof( pself->index, pchr->model );
    // Restore the saved values
    PipList[sTmp].xyvelbase = iTmp;
    PipList[sTmp].xyspacingbase = tTmp;
    PipList[sTmp].damagebase = test;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveExperienceToGoodTeam( script_state_t * pstate, ai_state_t * pself )
{
    // GiveExperienceToGoodTeam(  tmpargument = "amount", tmpdistance = "type" )
    // This function gives experience to everyone on the G Team

    SCRIPT_FUNCTION_BEGIN();

    give_team_experience( GOODTEAM, pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoNothing( script_state_t * pstate, ai_state_t * pself )
{
    // DoNothing()
    // This function does nothing
    // Use this for debugging or in combination with a Else function

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GrogTarget( script_state_t * pstate, ai_state_t * pself )
{
    // GrogTarget( tmpargument = "amount" )
    // This function grogs the Target for a duration equal to tmpargument

    SCRIPT_FUNCTION_BEGIN();

	if(CapList[ChrList[pself->target].model].canbegrogged) ChrList[pself->target].grogtime += pstate->argument;
	else returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DazeTarget( script_state_t * pstate, ai_state_t * pself )
{
    // DazeTarget( tmpargument = "amount" )
    // This function dazes the Target for a duration equal to tmpargument

    SCRIPT_FUNCTION_BEGIN();

	if(CapList[ChrList[pself->target].model].canbedazed) ChrList[pself->target].dazetime += pstate->argument;
	else returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableRespawn( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This function turns respawn with JUMP button on
    respawnvalid = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableRespawn( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This function turns respawn with JUMP button off
    respawnvalid = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HolderScoredAHit( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character's holder scored a hit
    returncode = bfalse;
    if ( 0 != ( ChrList[pchr->attachedto].ai.alert & ALERTIF_SCOREDAHIT ) )
    {
        returncode = btrue;
        pself->target = ChrList[pchr->attachedto].ai.hitlast;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HolderBlocked( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This function passes if the holder blocked an attack
    returncode = bfalse;
    if ( 0 != ( ChrList[pchr->attachedto].ai.alert & ALERTIF_BLOCKED ) )
    {
        returncode = btrue;
        pself->target = ChrList[pchr->attachedto].ai.attacklast;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
//Uint8 scr_get_SkillLevel( script_state_t * pstate, ai_state_t * pself )
//{
//    SCRIPT_FUNCTION_BEGIN();
//
//    // This function sets tmpargument to the shield profiency level of the Target
//    pstate->argument = CapList[pchr->attachedto].shieldprofiency;
//
//    SCRIPT_FUNCTION_END();
//}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasNotFullMana( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This function passes only if the Target is not at max mana and alive
    if ( !ChrList[pself->target].alive || ChrList[pself->target].mana > ChrList[pself->target].manamax - HURTDAMAGE )
        returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableListenSkill( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This function increases sound play range by 25%
    local_listening = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLastItemUsed( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // This sets the Target to the last item the character used
    if ( pself->lastitemused == pself->index ) returncode = bfalse;
    else pself->target = pself->lastitemused;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FollowLink( script_state_t * pstate, ai_state_t * pself )
{
    Uint16 model;
    int message_number, message_index;
    char * ptext;
    SCRIPT_FUNCTION_BEGIN();

    model = pchr->model;

    message_number = MadList[model].msgstart + pstate->argument;

    message_index = msgindex[message_number];

    ptext = msgtext + message_index;

    // Skips to the next module!
    returncode = link_follow_modname( ptext );

    if (!returncode)
    {
        STRING tmpbuf;
        snprintf(tmpbuf, sizeof(tmpbuf), "That's too scary for %s", pchr->name );
        debug_message(tmpbuf);
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OperatorIsLinux( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // Proceeds if running on linux
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
    SCRIPT_FUNCTION_BEGIN();

    // Proceeds if the AI Target Is a melee or ranged weapon
    sTmp = ChrList[pself->target].model;
    returncode = CapList[sTmp].isranged || (CapList[sTmp].weaponaction != ACTIONPA);

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SomeoneIsStealing( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // ThIs function passes if someone stealed from it's shop
    returncode = ( pself->order == STOLEN && pself->rank == 3 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsASpell( script_state_t * pstate, ai_state_t * pself )
{
    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // Proceeds if the AI Target has any particle with the [IDAM] or [WDAM] expansion
    iTmp = 0;
    returncode = bfalse;

    while (iTmp < MAXPRTPIPPEROBJECT)
    {
        if (PipList[MadList[ChrList[pself->target].inst.imad].prtpip[iTmp]].intdamagebonus || PipList[MadList[ChrList[pself->target].inst.imad].prtpip[iTmp]].wisdamagebonus)
        {
            returncode = btrue;
            break;
        }

        iTmp++;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Backstabbed( script_state_t * pstate, ai_state_t * pself )
{
    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    // Proceeds if HitFromBehind, Target has [DIsA] skill and damage Is physical
    returncode = bfalse;
    if ( 0 != ( pself->alert & ALERTIF_ATTACKED ) )
    {
        sTmp = ChrList[pself->attacklast].model;
        if ( pself->directionlast >= BEHIND - 8192 && pself->directionlast < BEHIND + 8192 )
        {
            if ( CapList[sTmp].idsz[IDSZ_SKILL] == Make_IDSZ( "STAB" ) )
            {
                iTmp = pself->damagetypelast;
                if ( iTmp == DAMAGE_CRUSH || iTmp == DAMAGE_POKE || iTmp == DAMAGE_SLASH ) returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetDamageType( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // ThIs function gets the last type of damage for the Target
    pstate->argument = ChrList[pself->target].ai.damagetypelast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddQuest( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    //ThIs function adds a quest idsz set in tmpargument into the Targets quest.txt
    if ( ChrList[pself->target].isplayer )
    {
        add_quest_idsz( ChrList[pself->target].name, pstate->argument );
        returncode = btrue;
    }
    else returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BeatQuestAllPlayers( script_state_t * pstate, ai_state_t * pself )
{
    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    //ThIs function marks a IDSZ in the Targets quest.txt as beaten
    returncode = bfalse;
    iTmp = 0;

    while (iTmp < MAXCHR)
    {
        if ( ChrList[iTmp].isplayer )
        {
            if (QUEST_BEATEN == modify_quest_idsz( ChrList[iTmp].name, (IDSZ)pstate->argument, 0 ) )
            {
                returncode = btrue;
            }
        }

        iTmp++;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasQuest( script_state_t * pstate, ai_state_t * pself )
{
    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    //ThIs function proceeds if the Target has the unfinIshed quest specIfied in tmpargument
    //and sets tmpdistance to the Quest Level of the specIfied quest.
    if ( ChrList[pself->target].isplayer )
    {
        iTmp = check_player_quest( ChrList[pself->target].name, pstate->argument );
        if ( iTmp > QUEST_BEATEN )
        {
            returncode = btrue;
            pstate->distance = iTmp;
        }
        else returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_QuestLevel( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    //ThIs function modIfies the quest level for a specIfic quest IDSZ
    //tmpargument specIfies quest idsz and tmpdistance the adjustment (which may be negative)
    returncode = bfalse;
    if ( ChrList[pself->target].isplayer && pstate->distance != 0 )
    {
        if (modify_quest_idsz( ChrList[pself->target].name, pstate->argument, pstate->distance ) > QUEST_NONE) returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddQuestAllPlayers( script_state_t * pstate, ai_state_t * pself )
{
    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    //ThIs function adds a quest idsz set in tmpargument into all local player's quest logs
    //The quest level Is set to tmpdistance if the level Is not already higher or QUEST_BEATEN
    iTmp = 0;
    returncode = bfalse;

    while (iTmp < MAXPLAYER)
    {
        if ( ChrList[PlaList[iTmp].index].isplayer )
        {
            returncode = btrue;
            if (!add_quest_idsz(ChrList[PlaList[iTmp].index].name , pstate->argument ))       //Try to add it if not already there or beaten
            {
                Sint16 i;
                i = check_player_quest( ChrList[PlaList[iTmp].index].name, pstate->argument);   //Get the current quest level
                if (i < 0 || i >= pstate->distance) returncode = bfalse;      //It was already beaten
                else modify_quest_idsz( ChrList[PlaList[iTmp].index].name, pstate->argument, pstate->distance );//Not beaten yet, increase level by 1
            }
        }

        iTmp++;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddBlipAllEnemies( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // show all enemies on the minimap who match the IDSZ given in tmpargument
    // it show only the enemies of the AI Target
    local_senseenemies = pself->target;
    local_senseenemiesID = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PitsFall( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // ThIs function activates pit teleportation...
    if ( pstate->x > EDGE && pstate->y > EDGE && pstate->x < mesh.info.edge_x - EDGE && pstate->y < mesh.info.edge_y - EDGE )
    {
        pitsfall = btrue;
        pitx = pstate->x;
        pity = pstate->y;
        pitz = pstate->distance;
    }
    else pitskill = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOwner( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // ThIs function proceeds only if the Target Is on another Team
    returncode = ( ChrList[pself->target].alive && pself->owner == pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_End( script_state_t * pstate, ai_state_t * pself )
{
    // End()
    // ThIs Is the last function in a script

    SCRIPT_FUNCTION_BEGIN();

    pself->terminate = btrue;
    returncode       = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TakePicture( script_state_t * pstate, ai_state_t * pself )
{
    // scr_TakePicture()
    // This function proceeds only if the screenshot was successful

    SCRIPT_FUNCTION_BEGIN();

    returncode = dump_screenshot();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Speech( script_state_t * pstate, ai_state_t * pself )
{
    // set_Speech( tmpargument = "sound" )
    // This function sets all of the RTS speech registers to tmpargument

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
    // set_MoveSpeech( tmpargument = "sound" )
    // This function sets the RTS move speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_MOVE] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SecondMoveSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // set_SecondMoveSpeech( tmpargument = "sound" )
    // This function sets the RTS movealt speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_MOVEALT] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_AttackSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // set_AttacksSpeech( tmpargument = "sound" )
    // This function sets the RTS attack speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_ATTACK] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_AssistSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // set_AssistSpeech( tmpargument = "sound" )
    // This function sets the RTS assist speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_ASSIST] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TerrainSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // set_TerrainSpeech( tmpargument = "sound" )
    // This function sets the RTS terrain speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_TERRAIN] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SelectSpeech( script_state_t * pstate, ai_state_t * pself )
{
    // set_SelectSpeech( tmpargument = "sound" )
    // This function sets the RTS select speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->soundindex[SPEECH_SELECT] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OperatorIsMacintosh( script_state_t * pstate, ai_state_t * pself )
{
    SCRIPT_FUNCTION_BEGIN();

    // Proceeds if the current running OS is mac
#ifdef __APPLE__
    returncode = btrue;
#else
    returncode = bfalse;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IfModuleHasIDSZ( script_state_t * pstate, ai_state_t * pself )
{
	//Proceeds if the specified module has the required IDSZ specified in tmpdistance
	//The module folder name to be checked is a string from message.txt
    SCRIPT_FUNCTION_BEGIN();

	//BAD: TODO: use message.txt to send the module name
	returncode = module_reference_matches("module.mod", pstate->distance);
    SCRIPT_FUNCTION_END();
}
