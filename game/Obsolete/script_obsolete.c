#include "script.h"
#include "script_compile.h"


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

//--------------------------------------------------------------------------------------------
Uint8 run_function_obsolete( script_state_t * pstate, ai_state_t * pself )
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
        log_message( "SCRIPT ERROR: run_function_obsolete() - model == %d, class name == \"%s\" - Unknown opcode found!\n", script_error_model, script_error_classname );
        return bfalse;
    }

    if ( NULL == pself || pself->index >= MAXCHR || !ChrList[pself->index].on ) return bfalse;
    pchr = ChrList + pself->index;

    // debug stuff
    if ( debug_scripts )
    {
        Uint32 i;

        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;

        for ( i = 0; i < pself->indent; i++ ) { fprintf( scr_file, "  " ); }

        for ( i = 0; i < MAXCODE; i++ )
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
            returncode = ( 0 != ( pself->alert & ALERTIF_TARGETKILLED ) || !ChrList.lst[pself->target].alive );
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
                    pstate->x = pstate->x - turntocos[( pstate->turn & 0xFFFF )>>2] * pstate->distance;
                    pstate->y = pstate->y - turntosin[( pstate->turn & 0xFFFF )>>2] * pstate->distance;
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
            pstate->x -= turntocos[( pstate->turn & 0xFFFF )>>2] * pstate->distance;
            pstate->y -= turntosin[( pstate->turn & 0xFFFF )>>2] * pstate->distance;
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
            if ( get_target( pself->index, NEARBY, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR ) returncode = btrue;

            break;

        case FSETTARGETTOTARGETLEFTHAND:
            // This function sets the target to the target's left item
            sTmp = ChrList[pself->target].holdingwhich[SLOT_LEFT];
            returncode = bfalse;
            if ( sTmp != MAXCHR )
            {
                pself->target = sTmp;
                returncode = btrue;
            }
            break;

        case FSETTARGETTOTARGETRIGHTHAND:
            // This function sets the target to the target's right item
            sTmp = ChrList[pself->target].holdingwhich[SLOT_RIGHT];
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
            sTmp = ChrList[pself->target].holdingwhich[SLOT_LEFT];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( Uint32 ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( Uint32 ) pstate->argument )
                    returncode = btrue;
            }

            // Check right hand
            sTmp = ChrList[pself->target].holdingwhich[SLOT_RIGHT];
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
            sTmp = ChrList[pself->target].holdingwhich[SLOT_LEFT];
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
            sTmp = ChrList[pself->target].holdingwhich[SLOT_RIGHT];
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

            returncode = ( 0 != check_skills( pself->target, ( IDSZ )pstate->argument ) );

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
            sTmp = pchr->holdingwhich[SLOT_LEFT];
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

            sTmp = pchr->holdingwhich[SLOT_RIGHT];
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
                if ( CapList[ChrList[sTmp].model].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[ChrList[sTmp].model].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
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
            sTmp = ChrList[pself->target].holdingwhich[SLOT_LEFT];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
                {
                    returncode = btrue;
                    iTmp = ChrList[pself->target].holdingwhich[SLOT_LEFT];
                }
            }

            // Check right hand
            sTmp = ChrList[pself->target].holdingwhich[SLOT_RIGHT];
            if ( sTmp != MAXCHR )
            {
                sTmp = ChrList[sTmp].model;
                if ( CapList[sTmp].idsz[IDSZ_PARENT] == ( IDSZ ) pstate->argument || CapList[sTmp].idsz[IDSZ_TYPE] == ( IDSZ ) pstate->argument )
                {
                    returncode = btrue;
                    iTmp = ChrList[pself->target].holdingwhich[SLOT_RIGHT];
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
            module_add_idsz( pickedmodule_name, pstate->argument );
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
            damage_character( pself->target, 0, pstate->argument, 1, pchr->damagetargettype, pchr->team, pself->index, DAMFX_BLOC, btrue );
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
            returncode = ( sTmp != MAXCHR );
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
            if ( pchr->holdingwhich[SLOT_LEFT] == MAXCHR )
            {
                returncode = bfalse;
            }
            else
            {
                pself->target = pchr->holdingwhich[SLOT_LEFT];
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

            pstate->x = (( pself->order >> 14 ) & 1023 ) << 6;
            pstate->y = (( pself->order >> 4 ) & 1023 ) << 6;
            pstate->argument = pself->order & 15;
            break;

        case FSETTARGETTOWHOEVERWASHIT:
            // This function sets the target to whoever the character hit last,
            pself->target = pself->hitlast;
            break;

        case FSETTARGETTOWIDEENEMY:
            // This function finds an enemy, and proceeds only if there is one
            returncode = bfalse;
            if ( get_target( pself->index, WIDE, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR ) returncode = btrue;

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
            returncode = ( pchr->holdingwhich[SLOT_LEFT] == MAXCHR && pchr->holdingwhich[SLOT_RIGHT] == MAXCHR );
            break;

        case FRESTOCKTARGETAMMOIDALL:
            // This function restocks the ammo of every item the character is holding,
            // if the item matches the ID given ( parent or child type )
            iTmp = 0;  // Amount of ammo given
            sTmp = ChrList[pself->target].holdingwhich[SLOT_LEFT];
            iTmp += restock_ammo( sTmp, pstate->argument );
            sTmp = ChrList[pself->target].holdingwhich[SLOT_RIGHT];
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
            sTmp = ChrList[pself->target].holdingwhich[SLOT_LEFT];
            iTmp += restock_ammo( sTmp, pstate->argument );
            if ( iTmp == 0 )
            {
                sTmp = ChrList[pself->target].holdingwhich[SLOT_RIGHT];
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
            sTmp = pchr->holdingwhich[SLOT_LEFT];
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
            sTmp = pchr->holdingwhich[SLOT_RIGHT];
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
            tTmp = pchr->holdingwhich[SLOT_LEFT];
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
            tTmp = pchr->holdingwhich[SLOT_RIGHT];
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
            sTmp = pchr->holdingwhich[SLOT_LEFT];
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
            sTmp = pchr->holdingwhich[SLOT_RIGHT];
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
            sTmp = pchr->holdingwhich[SLOT_LEFT];
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
            sTmp = pchr->holdingwhich[SLOT_RIGHT];
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
            returncode = (( CapList[pchr->model].skindressy & iTmp ) != 0 );
            break;

        case FIFOVERWATER:
            // This function passes if the character is on a water tile
            returncode = bfalse;
            if ( INVALID_TILE != pchr->onwhichfan )
            {
                returncode = (( mesh.mem.tile_list[pchr->onwhichfan].fx & MESHFX_WATER ) != 0 && wateriswater );
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
            returncode = ( tTmp != TOTALMAXPRT );
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
            chr_set_frame( pself->index, ACTIONDA, iTmp, sTmp );
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
                if (( pstate->distance >> 2 ) & 1 )  blahteam = FRIEND;
                if ((( pstate->distance >> 1 ) & 1 ) && blahteam == FRIEND ) blahteam = ALL;
                else if ((( pstate->distance >> 1 ) & 1 ) ) blahteam = ENEMY;
                else returncode = bfalse;
                if ( returncode )
                {
                    returncode = bfalse;
                    if ( get_target( pself->index, WIDE, blahteam, (( pstate->distance >> 3 ) & 1 ),
                                     (( pstate->distance ) & 1 ), pstate->argument, (( pstate->distance >> 4 ) & 1 ) ) != MAXCHR ) returncode = btrue;
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
            returncode = ( tTmp != TOTALMAXPRT );
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
            returncode = ( tTmp != TOTALMAXPRT );
            break;

        case FIFSTATEISODD:
            returncode = ( pself->state & 1 );
            break;

        case FSETTARGETTODISTANTENEMY:
            // This function finds an enemy, within a certain distance to the character, and
            // proceeds only if there is one
            returncode = bfalse;
            if ( get_target( pself->index, pstate->distance, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR ) returncode = btrue;

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
            returncode = ( tTmp != TOTALMAXPRT );
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

            returncode = ( sTmp != MAXCHR );
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
            returncode = ( sTmp != MAXCHR );
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
            returncode = ( tTmp != TOTALMAXPRT );
            break;

        case FCREATEORDER:
            // This function packs up an order, using tmpx, tmpy, tmpargument and the
            // target ( if valid ) to create a new tmpargument
            sTmp = pself->target << 24;
            sTmp |= (( pstate->x >> 6 ) & 1023 ) << 14;
            sTmp |= (( pstate->y >> 6 ) & 1023 ) << 4;
            sTmp |= ( pstate->argument & 15 );
            pstate->argument = sTmp;
            break;

        case FORDERSPECIALID:
            // This function issues an order to all with the given special IDSZ
            issue_special_order( pstate->argument, pstate->distance );
            break;

        case FUNKURSETARGETINVENTORY:
            // This function unkurses every item a character is holding
            sTmp = ChrList[pself->target].holdingwhich[SLOT_LEFT];
            ChrList[sTmp].iskursed = bfalse;
            sTmp = ChrList[pself->target].holdingwhich[SLOT_RIGHT];
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
                if (( pstate->distance >> 2 ) & 1 )  blahteam = FRIEND;
                if ((( pstate->distance >> 1 ) & 1 ) && blahteam == FRIEND ) blahteam = ALL;
                else if ((( pstate->distance >> 1 ) & 1 ) ) blahteam = ENEMY;
                if ( blahteam != NONE )
                {
                    if ( get_target( pself->index, NEAREST, blahteam, (( pstate->distance >> 3 ) & 1 ),
                                     (( pstate->distance ) & 1 ), pstate->argument, (( pstate->distance >> 4 ) & 1 ) ) != MAXCHR ) returncode = btrue;
                }
            }
            break;

        case FSETTARGETTONEARESTENEMY:
            // This function finds the nearest target that meets the
            // requirements
            returncode = bfalse;
            if ( get_target( pself->index, 0, ENEMY, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR ) returncode = btrue;

            break;

        case FSETTARGETTONEARESTFRIEND:
            // This function finds the nearest target that meets the
            // requirements
            returncode = bfalse;
            if ( get_target( pself->index, 0, FRIEND, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR ) returncode = btrue;

            break;

        case FSETTARGETTONEARESTLIFEFORM:
            // This function finds the nearest target that meets the
            // requirements
            returncode = bfalse;
            if ( get_target( pself->index, 0, ALL, bfalse, bfalse, IDSZ_NONE, bfalse ) != MAXCHR ) returncode = btrue;

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
                returncode = ( ChrList[sTmp].holdingwhich[SLOT_LEFT] == pself->index );
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
            returncode = ( tTmp != TOTALMAXPRT );
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
                returncode = link_follow_modname( ptext, btrue );

                if ( !returncode )
                {
                    STRING tmpbuf;
                    snprintf( tmpbuf, sizeof( tmpbuf ), "That's too scary for %s...", pchr->name );
                    debug_message( tmpbuf );
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
            returncode = CapList[sTmp].isranged || ( CapList[sTmp].weaponaction != ACTIONPA );
            break;

        case FIFSOMEONEISSTEALING:
            // This function passes if someone stealed from it's shop
            returncode = ( pself->order == STOLEN && pself->rank == 3 );
            break;

        case FIFTARGETISASPELL:
            // Proceeds if the AI target has any particle with the [IDAM] or [WDAM] expansion
            iTmp = 0;
            returncode = bfalse;

            while ( iTmp < MAXPRTPIPPEROBJECT )
            {
                if ( PipList[MadList[ChrList[pself->target].inst.imad].prtpip[iTmp]].intdamagebonus || PipList[MadList[ChrList[pself->target].inst.imad].prtpip[iTmp]].wisdamagebonus )
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

            while ( iTmp < MAXCHR )
            {
                if ( ChrList[iTmp].isplayer )
                {
                    if ( modify_quest_idsz( ChrList[iTmp].name, ( IDSZ )pstate->argument, 0 ) == QUEST_BEATEN ) returncode = btrue;
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
                if ( modify_quest_idsz( ChrList[pself->target].name, pstate->argument, pstate->distance ) > QUEST_NONE ) returncode = btrue;
            }
            break;

        case FADDQUESTALLPLAYERS:
            //This function adds a quest idsz set in tmpargument into all local player's quest logs
            //The quest level is set to tmpdistance if the level is not already higher or QUEST_BEATEN
            iTmp = 0;
            returncode = bfalse;

            while ( iTmp < MAXPLAYER )
            {
                if ( ChrList[PlaList[iTmp].index].isplayer )
                {
                    returncode = btrue;
                    if ( !add_quest_idsz( ChrList[PlaList[iTmp].index].name , pstate->argument ) )    //Try to add it if not already there or beaten
                    {
                        Sint16 i;
                        i = check_player_quest( ChrList[PlaList[iTmp].index].name, pstate->argument );  //Get the current quest level
                        if ( i < 0 || i >= pstate->distance ) returncode = bfalse;    //It was already beaten
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
            log_message( "SCRIPT ERROR: run_function_obsolete() - ai script %d - unhandled script function %d\n", pself->type, valuecode );
            returncode = bfalse;
            break;
    }

    return returncode;
}

