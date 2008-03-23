/* Egoboo - enchant.c
*/

/*
This file is part of Egoboo.

Egoboo is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Egoboo is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "mesh.h"

//--------------------------------------------------------------------------------------------
void do_enchant_spawn( float dUpdate )
{
  // ZZ> This function lets enchantments spawn particles
  int cnt, tnc;
  Uint16 facing, particle, eve, character;


  cnt = 0;
  while ( cnt < MAXENCHANT )
  {
    if ( encon[cnt] )
    {
      eve = enceve[cnt];
      if ( evecontspawnamount[eve] > 0 )
      {
        encspawntime[cnt] -= dUpdate;
        if ( encspawntime[cnt] <= 0 ) encspawntime[cnt] = 0;

        if ( encspawntime[cnt] == 0 )
        {
          character = enctarget[cnt];
          encspawntime[cnt] = evecontspawntime[eve];
          facing = chrturn_lr[character];
          tnc = 0;
          while ( tnc < evecontspawnamount[eve] )
          {
            particle = spawn_one_particle( 1.0f, chrpos[character],
                                           facing, eve, evecontspawnpip[eve],
                                           MAXCHR, GRIP_LAST, chrteam[encowner[cnt]], encowner[cnt], tnc, MAXCHR );
            facing += evecontspawnfacingadd[eve];
            tnc++;
          }
        }
      }
    }
    cnt++;
  }
}


//--------------------------------------------------------------------------------------------
void disenchant_character( Uint16 cnt )
{
  // ZZ> This function removes all enchantments from a character
  while ( chrfirstenchant[cnt] != MAXENCHANT )
  {
    remove_enchant( chrfirstenchant[cnt] );
  }
}

//char.c
//--------------------------------------------------------------------------------------------
void damage_character( CHR_REF character, Uint16 direction,
                       PAIR * pdam, DAMAGE damagetype, TEAM team,
                       Uint16 attacker, Uint16 effects )
{
  // ZZ> This function calculates and applies damage to a character.  It also
  //     sets alerts and begins actions.  Blocking and frame invincibility
  //     are done here too.  Direction is FRONT if the attack is coming head on,
  //     RIGHT if from the right, BEHIND if from the back, LEFT if from the
  //     left.
  int tnc;
  ACTION action;
  int damage, basedamage;
  Uint16 experience, model, left, right;

  if ( NULL == pdam ) return;
  if ( chrisplayer[character] && CData.DevMode ) return;

  if ( chralive[character] && pdam->ibase >= 0 && pdam->irand >= 1 )
  {
    // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
    // This can also be used to lessen effectiveness of healing
    damage = generate_unsigned( pdam );
    basedamage = damage;
    damage >>= ( chrdamagemodifier_fp8[character][damagetype] & DAMAGE_SHIFT );


    // Allow charging (Invert damage to mana)
    if ( chrdamagemodifier_fp8[character][damagetype]&DAMAGE_CHARGE )
    {
      chrmana_fp8[character] += damage;
      if ( chrmana_fp8[character] > chrmanamax_fp8[character] )
      {
        chrmana_fp8[character] = chrmanamax_fp8[character];
      }
      return;
    }

    // Mana damage (Deal damage to mana)
    if ( chrdamagemodifier_fp8[character][damagetype]&DAMAGE_MANA )
    {
      chrmana_fp8[character] -= damage;
      if ( chrmana_fp8[character] < 0 )
      {
        chrmana_fp8[character] = 0;
      }
      return;
    }

    // Invert damage to heal
    if ( chrdamagemodifier_fp8[character][damagetype]&DAMAGE_INVERT )
      damage = -damage;


    // Remember the damage type
    chrdamagetypelast[character] = damagetype;
    chrdirectionlast[character] = direction;


    // Do it already
    if ( damage > 0 )
    {
      // Only damage if not invincible
      if ( chrdamagetime[character] == 0 && !chrinvictus[character] )
      {
        model = chrmodel[character];
        if ( HAS_SOME_BITS( effects, DAMFX_BLOC ) )
        {
          // Only damage if hitting from proper direction
          if ( HAS_SOME_BITS( madframefx[chrframe[character]], MADFX_INVICTUS ) )
          {
            // I Frame...
            direction -= capiframefacing[model];
            left = ( ~capiframeangle[model] );
            right = capiframeangle[model];

            // Check for shield
            if ( chraction[character] >= ACTION_PA && chraction[character] <= ACTION_PD )
            {
              // Using a shield?
              if ( chraction[character] < ACTION_PC )
              {
                // Check left hand
                CHR_REF iholder = chr_get_holdingwhich( character, SLOT_LEFT );
                if ( VALID_CHR( iholder ) )
                {
                  left  = ~capiframeangle[iholder];
                  right = capiframeangle[iholder];
                }
              }
              else
              {
                // Check right hand
                CHR_REF iholder = chr_get_holdingwhich( character, SLOT_RIGHT );
                if ( VALID_CHR( iholder ) )
                {
                  left  = ~capiframeangle[iholder];
                  right = capiframeangle[iholder];
                }
              }
            }
          }
          else
          {
            // N Frame
            direction -= capnframefacing[model];
            left = ( ~capnframeangle[model] );
            right = capnframeangle[model];
          }
          // Check that direction
          if ( direction > left || direction < right )
          {
            damage = 0;
          }
        }



        if ( damage != 0 )
        {
          if ( HAS_SOME_BITS( effects, DAMFX_ARMO ) )
          {
            chrlife_fp8[character] -= damage;
          }
          else
          {
            chrlife_fp8[character] -= FP8_MUL( damage, chrdefense_fp8[character] );
          }


          if ( basedamage > MINDAMAGE )
          {
            // Call for help if below 1/2 life
            if ( chrlife_fp8[character] < ( chrlifemax_fp8[character] >> 1 ) ) //Zefz: Removed, because it caused guards to attack
              call_for_help( character );										//when dispelling overlay spells (Faerie Light)

            // Spawn blud particles
            if ( capbludlevel[model] > BLUD_NONE && ( damagetype < DAMAGE_HOLY || capbludlevel[model] == BLUD_ULTRA ) )
            {
              spawn_one_particle( 1.0f, chrpos[character],
                                  chrturn_lr[character] + direction, chrmodel[character], capbludprttype[model],
                                  MAXCHR, GRIP_LAST, chrteam[character], character, 0, MAXCHR );
            }
            // Set attack alert if it wasn't an accident
            if ( team == TEAM_DAMAGE )
            {
              chraiattacklast[character] = MAXCHR;
            }
            else
            {
              // Don't alert the character too much if under constant fire
              if ( chrcarefultime[character] == 0 )
              {
                // Don't let characters chase themselves...  That would be silly
                if ( attacker != character )
                {
                  chralert[character] |= ALERT_ATTACKED;
                  chraiattacklast[character] = attacker;
                  chrcarefultime[character] = DELAY_CAREFUL;
                }
              }
            }
          }


          // Taking damage action
          action = ACTION_HA;
          if ( chrlife_fp8[character] < 0 )
          {
            // Character has died
            chralive[character] = bfalse;
            disenchant_character( character );
            chrkeepaction[character] = btrue;
            chrlife_fp8[character] = -1;
            chrisplatform[character] = btrue;
            chrbumpdampen[character] /= 2.0;
            action = ACTION_KA;
            // Give kill experience
            experience = capexperienceworth[model] + ( chrexperience[character] * capexperienceexchange[model] );
            if ( VALID_CHR( attacker ) )
            {
              // Set target
              chraitarget[character] = attacker;
              if ( team == TEAM_DAMAGE )  chraitarget[character] = character;
              if ( team == TEAM_NULL )  chraitarget[character] = character;
              // Award direct kill experience
              if ( teamhatesteam[chrteam[attacker]][chrteam[character]] )
              {
                give_experience( attacker, experience, XP_KILLENEMY );
              }

              // Check for hated
              if ( CAP_INHERIT_IDSZ( model, capidsz[chrmodel[attacker]][IDSZ_HATE] ) )
              {
                give_experience( attacker, experience, XP_KILLHATED );
              }
            }

            // Clear all shop passages that it owned...
            tnc = 0;
            while ( tnc < numshoppassage )
            {
              if ( shopowner[tnc] == character )
              {
                shopowner[tnc] = NOOWNER;
              }
              tnc++;
            }

            // Let the other characters know it died
            tnc = 0;
            while ( tnc < MAXCHR )
            {
              if ( chron[tnc] && chralive[tnc] )
              {
                if ( chraitarget[tnc] == character )
                {
                  chralert[tnc] |= ALERT_TARGETKILLED;
                }
                if ( !teamhatesteam[chrteam[tnc]][team] && teamhatesteam[chrteam[tnc]][chrteam[character]] )
                {
                  // All allies get team experience, but only if they also hate the dead guy's team
                  give_experience( tnc, experience, XP_TEAMKILL );
                }
              }
              tnc++;
            }

            // Check if it was a leader
            if ( team_get_leader( chrteam[character] ) == character )
            {
              // It was a leader, so set more alerts
              tnc = 0;
              while ( tnc < MAXCHR )
              {
                if ( chron[tnc] && chrteam[tnc] == chrteam[character] )
                {
                  // All folks on the leaders team get the alert
                  chralert[tnc] |= ALERT_LEADERKILLED;
                }
                tnc++;
              }

              // The team now has no leader
              teamleader[chrteam[character]] = search_best_leader( chrteam[character], character );
            }

            detach_character_from_mount( character, btrue, bfalse );
            action += ( rand() & 3 );
            play_action( character, action, bfalse );

            // Turn off all sounds if it's a player
            for ( tnc = 0; tnc < MAXWAVE; tnc++ )
            {
              //TODO Zefz: Do we need this? This makes all sounds a character makes stop when it dies...
              //stop_sound(chrmodel[character]);
            }

            // Afford it one last thought if it's an AI
            teammorale[chrbaseteam[character]]--;
            chrteam[character] = chrbaseteam[character];
            chralert[character] = ALERT_KILLED;
            chrsparkle[character] = NOSPARKLE;
            chraitime[character] = 1;  // No timeout...
            let_character_think( character, 1.0f );
          }
          else
          {
            if ( basedamage > MINDAMAGE )
            {
              action += ( rand() & 3 );
              play_action( character, action, bfalse );

              // Make the character invincible for a limited time only
              if ( HAS_NO_BITS( effects, DAMFX_TIME ) )
                chrdamagetime[character] = DELAY_DAMAGE;
            }
          }
        }
        else
        {
          // Spawn a defend particle
          spawn_one_particle( chrbumpstrength[character], chrpos[character], chrturn_lr[character], MAXMODEL, PRTPIP_DEFEND, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
          chrdamagetime[character] = DELAY_DEFEND;
          chralert[character] |= ALERT_BLOCKED;
        }
      }
    }
    else if ( damage < 0 )
    {
      chrlife_fp8[character] -= damage;
      if ( chrlife_fp8[character] > chrlifemax_fp8[character] )  chrlife_fp8[character] = chrlifemax_fp8[character];

      // Isssue an alert
      chralert[character] |= ALERT_HEALED;
      chraiattacklast[character] = attacker;
      if ( team != TEAM_DAMAGE )
      {
        chraiattacklast[character] = MAXCHR;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void kill_character( CHR_REF character, Uint16 killer )
{
  // ZZ> This function kills a character...  MAXCHR killer for accidental death
  Uint8 modifier;

  if ( !chralive[character] ) return;

  chrdamagetime[character] = 0;
  chrlife_fp8[character] = 1;
  modifier = chrdamagemodifier_fp8[character][DAMAGE_CRUSH];
  chrdamagemodifier_fp8[character][DAMAGE_CRUSH] = 1;
  if ( VALID_CHR( killer ) )
  {
    PAIR ptemp = {512, 1};
    damage_character( character, 0, &ptemp, DAMAGE_CRUSH, chrteam[killer], killer, DAMFX_ARMO | DAMFX_BLOC );
  }
  else
  {
    PAIR ptemp = {512, 1};
    damage_character( character, 0, &ptemp, DAMAGE_CRUSH, TEAM_DAMAGE, chr_get_aibumplast( character ), DAMFX_ARMO | DAMFX_BLOC );
  }
  chrdamagemodifier_fp8[character][DAMAGE_CRUSH] = modifier;

  // try something here.
  chrisplatform[character] = btrue;
  chrismount[character]  = bfalse;
}

//--------------------------------------------------------------------------------------------
void spawn_poof( CHR_REF character, Uint16 profile )
{
  // ZZ> This function spawns a character poof
  Uint16 sTmp;
  Uint16 origin;
  int iTmp;


  sTmp = chrturn_lr[character];
  iTmp = 0;
  origin = chr_get_aiowner( character );
  while ( iTmp < capgopoofprtamount[profile] )
  {
    spawn_one_particle( 1.0f, chrpos_old[character],
                        sTmp, profile, capgopoofprttype[profile],
                        MAXCHR, GRIP_LAST, chrteam[character], origin, iTmp, MAXCHR );
    sTmp += capgopoofprtfacingadd[profile];
    iTmp++;
  }
}

//--------------------------------------------------------------------------------------------
void naming_names( int profile )
{
  // ZZ> This function generates a random name
  int read, write, section, mychop;
  char cTmp;

  if ( capsectionsize[profile][0] == 0 )
  {
    namingnames[0] = 'B';
    namingnames[1] = 'l';
    namingnames[2] = 'a';
    namingnames[3] = 'h';
    namingnames[4] = 0;
  }
  else
  {
    write = 0;
    section = 0;
    while ( section < MAXSECTION )
    {
      if ( capsectionsize[profile][section] != 0 )
      {
        mychop = capsectionstart[profile][section] + ( rand() % capsectionsize[profile][section] );
        read = chopstart[mychop];
        cTmp = chopdata[read];
        while ( cTmp != 0 && write < MAXCAPNAMESIZE - 1 )
        {
          namingnames[write] = cTmp;
          write++;
          read++;
          cTmp = chopdata[read];
        }
      }
      section++;
    }
    if ( write >= MAXCAPNAMESIZE ) write = MAXCAPNAMESIZE - 1;
    namingnames[write] = 0;
  }
}

//--------------------------------------------------------------------------------------------
void read_naming( int profile, char *szLoadname )
{
  // ZZ> This function reads a naming file
  FILE *fileread;
  int section, chopinsection, cnt;
  char mychop[32], cTmp;

  fileread = fs_fileOpen( PRI_NONE, NULL, szLoadname, "r" );
  if ( NULL == fileread ) return;

  section = 0;
  chopinsection = 0;
  while ( fget_next_string( fileread, mychop, sizeof( mychop ) ) && section < MAXSECTION )
  {
    if ( 0 != strcmp( mychop, "STOP" ) )
    {
      if ( chopwrite >= CHOPDATACHUNK )  chopwrite = CHOPDATACHUNK - 1;
      chopstart[numchop] = chopwrite;

      cnt = 0;
      cTmp = mychop[0];
      while ( cTmp != 0 && cnt < 31 && chopwrite < CHOPDATACHUNK )
      {
        if ( cTmp == '_' ) cTmp = ' ';
        chopdata[chopwrite] = cTmp;
        cnt++;
        chopwrite++;
        cTmp = mychop[cnt];
      }

      if ( chopwrite >= CHOPDATACHUNK )  chopwrite = CHOPDATACHUNK - 1;
      chopdata[chopwrite] = 0;  chopwrite++;
      chopinsection++;
      numchop++;
    }
    else
    {
      capsectionsize[profile][section] = chopinsection;
      capsectionstart[profile][section] = numchop - chopinsection;
      section++;
      chopinsection = 0;
    }
  }
  fs_fileClose( fileread );

}

//--------------------------------------------------------------------------------------------
void prime_names( void )
{
  // ZZ> This function prepares the name chopper for use
  int cnt, tnc;

  numchop = 0;
  chopwrite = 0;
  cnt = 0;
  while ( cnt < MAXMODEL )
  {
    tnc = 0;
    while ( tnc < MAXSECTION )
    {
      capsectionstart[cnt][tnc] = MAXCHOP;
      capsectionsize[cnt][tnc] = 0;
      tnc++;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain()
{
  // ZZ> This function sets all of the character's starting tilt values
  int cnt;
  Uint8 twist;

  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( chrstickybutt[cnt] && chron[cnt] && chronwhichfan[cnt] != INVALID_FAN )
    {
      twist = mesh_get_twist( chronwhichfan[cnt] );
      chrmapturn_lr[cnt] = maptwist_lr[twist];
      chrmapturn_ud[cnt] = maptwist_ud[twist];
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_character( vect3 pos, int profile, TEAM team,
                            Uint8 skin, Uint16 facing, char *name, Uint16 override )
{
  // ZZ> This function spawns a character and returns the character's index number
  //     if it worked, MAXCHR otherwise
  int ichr, tnc;

  // Make sure the team is valid
  if ( team >= TEAM_COUNT ) team %= TEAM_COUNT;


  // Get a new character
  if ( !madused[profile] )
  {
    fprintf( stderr, "spawn_one_character() - \n\tfailed to spawn : model %d doesn't exist\n", profile );
    return MAXCHR;
  }

  ichr = MAXCHR;
  if ( VALID_CHR( override ) )
  {
    ichr = get_free_character();
    if ( ichr != override )
    {
      // Picked the wrong one, so put this one back and find the right one
      tnc = 0;
      while ( tnc < MAXCHR )
      {
        if ( freechrlist[tnc] == override )
        {
          freechrlist[tnc] = ichr;
          tnc = MAXCHR;
        }
        tnc++;
      }
      ichr = override;
    }

    if ( MAXCHR == ichr )
    {
      fprintf( stderr, "spawn_one_character() - \n\tfailed to spawn : cannot find override index %d\n", override );
      return MAXCHR;
    }
  }
  else
  {
    ichr = get_free_character();

    if ( MAXCHR == ichr )
    {
      fprintf( stderr, "spawn_one_character() - \n\tfailed to spawn : get_free_character() returned invalid value %d\n", ichr );
      return MAXCHR;
    }
  }

  fprintf( stdout, "spawn_one_character() - \n\tprofile == %d, capclassname[profile] == \"%s\", index == %d\n", profile, capclassname[profile], ichr );

  // IMPORTANT!!!
  chrindolist[ichr] = bfalse;
  chrisequipped[ichr] = bfalse;
  chrsparkle[ichr] = NOSPARKLE;
  chroverlay[ichr] = bfalse;
  chrmissilehandler[ichr] = ichr;

  // Set up model stuff
  chron[ichr] = btrue;
  chrfreeme[ichr] = bfalse;
  chrgopoof[ichr] = bfalse;
  chrreloadtime[ichr] = 0;
  chrinwhichslot[ichr] = SLOT_NONE;
  chrinwhichpack[ichr] = MAXCHR;
  chrnextinpack[ichr] = MAXCHR;
  chrnuminpack[ichr] = 0;
  chrmodel[ichr] = profile;
  chrbasemodel[ichr] = profile;
  chrstoppedby[ichr] = capstoppedby[profile];
  chrlifeheal[ichr] = caplifeheal_fp8[profile];
  chrmanacost[ichr] = capmanacost_fp8[profile];
  chrinwater[ichr] = bfalse;
  chrnameknown[ichr] = capnameknown[profile];
  chrammoknown[ichr] = capnameknown[profile];
  chrhitready[ichr] = btrue;
  chrboretime[ichr] = DELAY_BORE;
  chrcarefultime[ichr] = DELAY_CAREFUL;
  chrcanbecrushed[ichr] = bfalse;
  chrdamageboost[ichr] = 0;
  chricon[ichr] = capicon[profile];

  // Enchant stuff
  chrfirstenchant[ichr] = MAXENCHANT;
  chrundoenchant[ichr] = MAXENCHANT;
  chrcanseeinvisible[ichr] = capcanseeinvisible[profile];
  chrcanchannel[ichr] = bfalse;
  chrmissiletreatment[ichr] = MIS_NORMAL;
  chrmissilecost[ichr] = 0;

  //Skill Expansions
  chrcanseekurse[ichr] = capcanseekurse[profile];
  chrcanusedivine[ichr] = capcanusedivine[profile];
  chrcanusearcane[ichr] = capcanusearcane[profile];
  chrcandisarm[ichr] = capcandisarm[profile];
  chrcanjoust[ichr] = capcanjoust[profile];
  chrcanusetech[ichr] = capcanusetech[profile];
  chrcanusepoison[ichr] = capcanusepoison[profile];
  chrcanuseadvancedweapons[ichr] = capcanuseadvancedweapons[profile];
  chrcanbackstab[ichr] = capcanbackstab[profile];
  chrcanread[ichr] = capcanread[profile];


  // Kurse state
  chriskursed[ichr] = (( rand() % 100 ) < capkursechance[profile] );
  if ( !capisitem[profile] )  chriskursed[ichr] = bfalse;


  // Ammo
  chrammomax[ichr] = capammomax[profile];
  chrammo[ichr] = capammo[profile];


  // Gender
  chrgender[ichr] = capgender[profile];
  if ( chrgender[ichr] == GEN_RANDOM )  chrgender[ichr] = GEN_FEMALE + ( rand() & 1 );

  // Team stuff
  chrteam[ichr] = team;
  chrbaseteam[ichr] = team;
  chrmessagedata[ichr] = teammorale[team];
  if ( !capinvictus[profile] )  teammorale[team]++;
  chrmessage[ichr] = 0;
  // Firstborn becomes the leader
  if ( !VALID_CHR( team_get_leader( team ) ) )
  {
    teamleader[team] = ichr;
  }

  // Skin
  if ( capskinoverride[profile] != NOSKINOVERRIDE )
  {
    skin = capskinoverride[profile] % MAXSKIN;
  }
  if ( skin >= madskins[profile] )
  {
    skin = 0;
    if ( madskins[profile] > 1 )
    {
      skin = rand() % madskins[profile];
    }
  }
  chrtexture[ichr] = madskinstart[profile] + skin;

  // Life and Mana
  chralive[ichr] = btrue;
  chrlifecolor[ichr] = caplifecolor[profile];
  chrmanacolor[ichr] = capmanacolor[profile];
  chrlifemax_fp8[ichr] = generate_unsigned( &caplife_fp8[profile] );
  chrlife_fp8[ichr] = chrlifemax_fp8[ichr];
  chrlifereturn[ichr] = caplifereturn_fp8[profile];
  chrmanamax_fp8[ichr] = generate_unsigned( &capmana_fp8[profile] );
  chrmanaflow_fp8[ichr] = generate_unsigned( &capmanaflow_fp8[profile] );
  chrmanareturn_fp8[ichr] = generate_unsigned( &capmanareturn_fp8[profile] );  //>> MANARETURNSHIFT;
  chrmana_fp8[ichr] = chrmanamax_fp8[ichr];

  // SWID
  chrstrength_fp8[ichr] = generate_unsigned( &capstrength_fp8[profile] );
  chrwisdom_fp8[ichr] = generate_unsigned( &capwisdom_fp8[profile] );
  chrintelligence_fp8[ichr] = generate_unsigned( &capintelligence_fp8[profile] );
  chrdexterity_fp8[ichr] = generate_unsigned( &capdexterity_fp8[profile] );

  // Damage
  chrdefense_fp8[ichr] = capdefense_fp8[profile][skin];
  chrreaffirmdamagetype[ichr] = capattachedprtreaffirmdamagetype[profile];
  chrdamagetargettype[ichr] = capdamagetargettype[profile];
  tnc = 0;
  while ( tnc < MAXDAMAGETYPE )
  {
    chrdamagemodifier_fp8[ichr][tnc] = capdamagemodifier_fp8[profile][tnc][skin];
    tnc++;
  }

  // AI stuff
  chraitype[ichr] = madai[chrmodel[ichr]];
  chrisplayer[ichr] = bfalse;
  chrislocalplayer[ichr] = bfalse;
  chralert[ichr] = ALERT_SPAWNED;
  chraistate[ichr] = capstateoverride[profile];
  chraicontent[ichr] = capcontentoverride[profile];
  chraitarget[ichr] = ichr;
  chraiowner[ichr] = ichr;
  chraichild[ichr] = ichr;
  chraitime[ichr] = 0;
  tnc = 0;
  while ( tnc < MAXSTOR )
  {
    chraix[ichr][tnc] = 0;
    chraiy[ichr][tnc] = 0;
    tnc++;
  }
  chraimorphed[ichr] = bfalse;

  chrlatchx[ichr] = 0;
  chrlatchy[ichr] = 0;
  chrlatchbutton[ichr] = 0;
  chrturnmode[ichr] = TURNMODE_VELOCITY;

  // Flags
  chrstickybutt[ichr] = capstickybutt[profile];
  chropenstuff[ichr] = capcanopenstuff[profile];
  chrtransferblend[ichr] = captransferblend[profile];
  chrenviro[ichr] = capenviro[profile];
  chrwaterwalk[ichr] = capwaterwalk[profile];
  chrisplatform[ichr] = capisplatform[profile];
  chrisitem[ichr] = capisitem[profile];
  chrinvictus[ichr] = capinvictus[profile];
  chrismount[ichr] = capismount[profile];
  chrcangrabmoney[ichr] = capcangrabmoney[profile];

  // Jumping
  chrjump[ichr] = capjump[profile];
  chrjumpready[ichr] = btrue;
  chrjumpnumber[ichr] = 1;
  chrjumpnumberreset[ichr] = capjumpnumber[profile];
  chrjumptime[ichr] = DELAY_JUMP;

  // Other junk
  chrflyheight[ichr] = capflyheight[profile];
  chrmaxaccel[ichr] = capmaxaccel[profile][skin];
  chralpha_fp8[ichr] = capalpha_fp8[profile];
  chrlight_fp8[ichr] = caplight_fp8[profile];
  chrflashand[ichr] = capflashand[profile];
  chrsheen_fp8[ichr] = capsheen_fp8[profile];
  chrdampen[ichr] = capdampen[profile];

  // Character size and bumping
  chrfat[ichr] = capsize[profile];
  chrsizegoto[ichr] = chrfat[ichr];
  chrsizegototime[ichr] = 0;

  chrshadowsizesave[ichr]  = capshadowsize[profile];
  chrbumpsizesave[ichr]    = capbumpsize[profile];
  chrbumpsizebigsave[ichr] = capbumpsizebig[profile];
  chrbumpheightsave[ichr]  = capbumpheight[profile];

  chrshadowsize[ichr]   = capshadowsize[profile]  * chrfat[ichr];
  chrbumpsize[ichr]     = capbumpsize[profile]    * chrfat[ichr];
  chrbumpsizebig[ichr]  = capbumpsizebig[profile] * chrfat[ichr];
  chrbumpheight[ichr]   = capbumpheight[profile]  * chrfat[ichr];
  chrbumpstrength[ichr] = capbumpstrength[profile] * FP8_TO_FLOAT( capalpha_fp8[profile] );



  chrbumpdampen[ichr] = capbumpdampen[profile];
  chrweight[ichr] = capweight[profile] * chrfat[ichr] * chrfat[ichr] * chrfat[ichr];   // preserve density
  chraibumplast[ichr] = ichr;
  chraiattacklast[ichr] = MAXCHR;
  chraihitlast[ichr] = ichr;

  // Grip info
  chrinwhichslot[ichr] = SLOT_NONE;
  chrattachedto[ichr] = MAXCHR;
  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    chrholdingwhich[ichr][_slot] = MAXCHR;
  }

  // Image rendering
  chruoffset_fp8[ichr] = 0;
  chrvoffset_fp8[ichr] = 0;
  chruoffvel[ichr] = capuoffvel[profile];
  chrvoffvel[ichr] = capvoffvel[profile];
  chrredshift[ichr] = 0;
  chrgrnshift[ichr] = 0;
  chrblushift[ichr] = 0;


  // Movement
  chrsneakspd[ichr] = capsneakspd[profile];
  chrwalkspd[ichr] = capwalkspd[profile];
  chrrunspd[ichr] = caprunspd[profile];

  // Set up position
  chrpos[ichr].x = pos.x;
  chrpos[ichr].y = pos.y;
  chrturn_lr[ichr] = facing;
  chronwhichfan[ichr] = mesh_get_fan( chrpos[ichr] );
  chrlevel[ichr] = mesh_get_level( chronwhichfan[ichr], chrpos[ichr].x, chrpos[ichr].y, chrwaterwalk[ichr] ) + RAISE;
  if ( pos.z < chrlevel[ichr] ) pos.z = chrlevel[ichr];
  chrpos[ichr].z = pos.z;

  chrstt[ichr]         = chrpos[ichr];
  chrpos_old[ichr]     = chrpos[ichr];
  chrturn_lr_old[ichr] = chrturn_lr[ichr];

  chrlightturn_lrr[ichr] = 0;
  chrlightturn_lrg[ichr] = 0;
  chrlightturn_lrb[ichr] = 0;

  chrvel[ichr].x = 0;
  chrvel[ichr].y = 0;
  chrvel[ichr].z = 0;
  chrtrgvel[ichr].x = 0;
  chrtrgvel[ichr].y = 0;
  chrtrgvel[ichr].z = 0;
  chrmapturn_lr[ichr] = 32768;  // These two mean on level surface
  chrmapturn_ud[ichr] = 32768;
  chrscale[ichr] = chrfat[ichr]; // * madscale[chrmodel[ichr]] * 4;

  // AI and action stuff
  chraigoto[ichr] = 0;
  chraigotoadd[ichr] = 1;
  chraigotox[ichr][0] = chrpos[ichr].x;
  chraigotoy[ichr][0] = chrpos[ichr].y;
  chractionready[ichr] = btrue;
  chrkeepaction[ichr] = bfalse;
  chrloopaction[ichr] = bfalse;
  chraction[ichr] = ACTION_DA;
  chrnextaction[ichr] = ACTION_DA;
  chrlip_fp8[ichr] = 0;
  chrflip[ichr] = 0.0f;
  chrframe[ichr] = madframestart[chrmodel[ichr]];
  chrlastframe[ichr] = chrframe[ichr];
  chrpassage[ichr] = 0;
  chrholdingweight[ichr] = 0;
  chronwhichplatform[ichr] = MAXCHR;

  // Timers set to 0
  chrgrogtime[ichr] = 0.0f;
  chrdazetime[ichr] = 0.0f;

  // Money is added later
  chrmoney[ichr] = capmoney[profile];

  // Name the character
  if ( name == NULL )
  {
    // Generate a random name
    naming_names( profile );
    strncpy( chrname[ichr], namingnames, sizeof( chrname[ichr] ) );
  }
  else
  {
    // A name has been given
    tnc = 0;
    while ( tnc < MAXCAPNAMESIZE - 1 )
    {
      chrname[ichr][tnc] = name[tnc];
      tnc++;
    }
    chrname[ichr][tnc] = 0;
  }

  // Set up initial fade in lighting
  tnc = 0;
  while ( tnc < madtransvertices[chrmodel[ichr]] )
  {
    chrvrtar_fp8[ichr][tnc] = 0;
    chrvrtag_fp8[ichr][tnc] = 0;
    chrvrtab_fp8[ichr][tnc] = 0;
    tnc++;
  }

  // Particle attachments
  tnc = 0;
  while ( tnc < capattachedprtamount[profile] )
  {
    spawn_one_particle( 1.0f, chrpos[ichr],
                        0, chrmodel[ichr], capattachedprttype[profile],
                        ichr, GRIP_LAST + tnc, chrteam[ichr], ichr, tnc, MAXCHR );
    tnc++;
  }
  chrreaffirmdamagetype[ichr] = capattachedprtreaffirmdamagetype[profile];


  // Experience
  if ( capleveloverride[profile] != 0 )
  {
    while ( chrexperiencelevel[ichr] < capleveloverride[profile] )
    {
      give_experience( ichr, 100, XP_DIRECT );
    }
  }
  else
  {
    chrexperience[ichr] = generate_unsigned( &capexperience[profile] );
    chrexperiencelevel[ichr] = calc_chr_level( ichr );
  }


  chrpancakepos[ichr].x = chrpancakepos[ichr].y = chrpancakepos[ichr].z = 1.0;
  chrpancakevel[ichr].x = chrpancakevel[ichr].y = chrpancakevel[ichr].z = 0.0f;

  chrloopingchannel[numfreechr] = INVALID_CHANNEL;

  return ichr;
}

//--------------------------------------------------------------------------------------------
void respawn_character( CHR_REF character )
{
  // ZZ> This function respawns a character
  Uint16 item, profile;

  if ( !VALID_CHR( character ) || chralive[character] ) return;

  profile = chrmodel[character];

  spawn_poof( character, profile );
  disaffirm_attached_particles( character );
  chralive[character] = btrue;
  chrboretime[character] = DELAY_BORE;
  chrcarefultime[character] = DELAY_CAREFUL;
  chrlife_fp8[character] = chrlifemax_fp8[character];
  chrmana_fp8[character] = chrmanamax_fp8[character];
  chrpos[character].x = chrstt[character].x;
  chrpos[character].y = chrstt[character].y;
  chrpos[character].z = chrstt[character].z;
  chrvel[character].x = 0;
  chrvel[character].y = 0;
  chrvel[character].z = 0;
  chrtrgvel[character].x = 0;
  chrtrgvel[character].y = 0;
  chrtrgvel[character].z = 0;
  chrteam[character] = chrbaseteam[character];
  chrcanbecrushed[character] = bfalse;
  chrmapturn_lr[character] = 32768;  // These two mean on level surface
  chrmapturn_ud[character] = 32768;
  if ( !VALID_CHR( team_get_leader( chrteam[character] ) ) )  teamleader[chrteam[character]] = character;
  if ( !chrinvictus[character] )  teammorale[chrbaseteam[character]]++;
  chractionready[character] = btrue;
  chrkeepaction[character] = bfalse;
  chrloopaction[character] = bfalse;
  chraction[character] = ACTION_DA;
  chrnextaction[character] = ACTION_DA;
  chrlip_fp8[character] = 0;
  chrflip[character] = 0.0f;
  chrframe[character] = madframestart[profile];
  chrlastframe[character] = chrframe[character];
  chrisplatform[character] = capisplatform[profile];
  chrflyheight[character] = capflyheight[profile];
  chrbumpdampen[character] = capbumpdampen[profile];

  chrbumpsizesave[character]    = capbumpsize[profile];
  chrbumpsizebigsave[character] = capbumpsizebig[profile];
  chrbumpheightsave[character]  = capbumpheight[profile];

  chrbumpsize[character]     = capbumpsize[profile] * chrfat[character];
  chrbumpsizebig[character]  = capbumpsizebig[profile] * chrfat[character];
  chrbumpheight[character]   = capbumpheight[profile] * chrfat[character];
  chrbumpstrength[character] = capbumpstrength[profile] * FP8_TO_FLOAT( capalpha_fp8[profile] );

  // clear the alert and leave the state alone
  chralert[character] = ALERT_NONE;
  chraitarget[character] = character;
  chraitime[character] = 0;
  chrgrogtime[character] = 0.0f;
  chrdazetime[character] = 0.0f;
  reaffirm_attached_particles( character );


  // Let worn items come back
  item  = chr_get_nextinpack( character );
  while ( VALID_CHR( item ) )
  {
    if ( chrisequipped[item] )
    {
      chrisequipped[item] = bfalse;
      chralert[item] |= ALERT_ATLASTWAYPOINT;  // doubles as PutAway
    }
    item  = chr_get_nextinpack( item );
  }
}

//--------------------------------------------------------------------------------------------
Uint16 change_armor( CHR_REF character, Uint16 skin )
{
  // ZZ> This function changes the armor of the character
  Uint16 enchant, sTmp;
  int iTmp, cnt;


  // Remove armor enchantments
  enchant = chrfirstenchant[character];
  while ( enchant < MAXENCHANT )
  {
    for ( cnt = SETSLASHMODIFIER; cnt <= SETZAPMODIFIER; cnt++ )
    {
      unset_enchant_value( enchant, cnt );
    }
    enchant = encnextenchant[enchant];
  }


  // Change the skin
  sTmp = chrmodel[character];
  if ( skin > madskins[sTmp] )  skin = 0;
  chrtexture[character] = madskinstart[sTmp] + skin;


  // Change stats associated with skin
  chrdefense_fp8[character] = capdefense_fp8[sTmp][skin];
  iTmp = 0;
  while ( iTmp < MAXDAMAGETYPE )
  {
    chrdamagemodifier_fp8[character][iTmp] = capdamagemodifier_fp8[sTmp][iTmp][skin];
    iTmp++;
  }
  chrmaxaccel[character] = capmaxaccel[sTmp][skin];


  // Reset armor enchantments
  // These should really be done in reverse order ( Start with last enchant ), but
  // I don't care at this point !!!BAD!!!
  enchant = chrfirstenchant[character];
  while ( enchant < MAXENCHANT )
  {
    for ( cnt = SETSLASHMODIFIER; cnt <= SETZAPMODIFIER; cnt++ )
    {
      set_enchant_value( enchant, cnt, enceve[enchant] );
    };
    add_enchant_value( enchant, ADDACCEL, enceve[enchant] );
    add_enchant_value( enchant, ADDDEFENSE, enceve[enchant] );
    enchant = encnextenchant[enchant];
  }
  return skin;
}

//--------------------------------------------------------------------------------------------
void change_character( CHR_REF ichr, Uint16 new_profile, Uint8 new_skin,
                       Uint8 leavewhich )
{
  // ZZ> This function polymorphs a character, changing stats, dropping weapons
  int tnc, enchant;
  Uint16 sTmp;
  CHR_REF item, imount;

  if ( new_profile > MAXMODEL || !madused[new_profile] ) return;

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    sTmp = chr_get_holdingwhich( ichr, _slot );
    if ( !capslotvalid[new_profile][_slot] )
    {
      if ( detach_character_from_mount( sTmp, btrue, btrue ) )
      {
        if ( _slot == SLOT_SADDLE )
        {
          chraccum_vel[sTmp].z += DISMOUNTZVEL;
          chraccum_pos[sTmp].z += DISMOUNTZVEL;
          chrjumptime[sTmp]  = DELAY_JUMP;
        };
      }
    }
  }

  // Remove particles
  disaffirm_attached_particles( ichr );


  switch ( leavewhich )
  {
    case LEAVE_FIRST:
      {
        // Remove all enchantments except top one
        enchant = chrfirstenchant[ichr];
        if ( enchant != MAXENCHANT )
        {
          while ( encnextenchant[enchant] != MAXENCHANT )
          {
            remove_enchant( encnextenchant[enchant] );
          }
        }
      }
      break;

    case LEAVE_NONE:
      {
        // Remove all enchantments
        disenchant_character( ichr );
      }
      break;
  }

  // Stuff that must be set
  chrmodel[ichr]     = new_profile;
  chrstoppedby[ichr] = capstoppedby[new_profile];
  chrlifeheal[ichr]  = caplifeheal_fp8[new_profile];
  chrmanacost[ichr]  = capmanacost_fp8[new_profile];

  // Ammo
  chrammomax[ichr] = capammomax[new_profile];
  chrammo[ichr] = capammo[new_profile];
  // Gender
  if ( capgender[new_profile] != GEN_RANDOM ) // GEN_RANDOM means keep old gender
  {
    chrgender[ichr] = capgender[new_profile];
  }


  // AI stuff
  chraitype[ichr] = madai[new_profile];
  chraistate[ichr] = 0;
  chraitime[ichr] = 0;
  chrlatchx[ichr] = 0;
  chrlatchy[ichr] = 0;
  chrlatchbutton[ichr] = 0;
  chrturnmode[ichr] = TURNMODE_VELOCITY;

  // Flags
  chrstickybutt[ichr] = capstickybutt[new_profile];
  chropenstuff[ichr] = capcanopenstuff[new_profile];
  chrtransferblend[ichr] = captransferblend[new_profile];
  chrenviro[ichr] = capenviro[new_profile];
  chrisplatform[ichr] = capisplatform[new_profile];
  chrisitem[ichr] = capisitem[new_profile];
  chrinvictus[ichr] = capinvictus[new_profile];
  chrismount[ichr] = capismount[new_profile];
  chrcangrabmoney[ichr] = capcangrabmoney[new_profile];
  chrjumptime[ichr] = DELAY_JUMP;

  // Character size and bumping
  chrshadowsizesave[ichr]  = capshadowsize[new_profile];
  chrbumpsizesave[ichr]    = capbumpsize[new_profile];
  chrbumpsizebigsave[ichr] = capbumpsizebig[new_profile];
  chrbumpheightsave[ichr]  = capbumpheight[new_profile];

  chrshadowsize[ichr]   = capshadowsize[new_profile] * chrfat[ichr];
  chrbumpsize[ichr]     = capbumpsize[new_profile] * chrfat[ichr];
  chrbumpsizebig[ichr]  = capbumpsizebig[new_profile] * chrfat[ichr];
  chrbumpheight[ichr]   = capbumpheight[new_profile] * chrfat[ichr];
  chrbumpstrength[ichr] = capbumpstrength[new_profile] * FP8_TO_FLOAT( capalpha_fp8[new_profile] );

  chrbumpdampen[ichr] = capbumpdampen[new_profile];
  chrweight[ichr] = capweight[new_profile] * chrfat[ichr] * chrfat[ichr] * chrfat[ichr];     // preserve density
  chrscale[ichr] = chrfat[ichr];


  // Character scales...  Magic numbers
  imount = chr_get_attachedto( ichr );
  if ( VALID_CHR( imount ) )
  {
    Uint16 imodel =  chrmodel[imount];
    Uint16 vrtoffset = slot_to_offset( chrinwhichslot[ichr] );

    if( !VALID_MDL(imodel) )
    {
      chrattachedgrip[ichr][0] =
      chrattachedgrip[ichr][1] =
      chrattachedgrip[ichr][2] =
      chrattachedgrip[ichr][3] = 0;
    }
    else if ( madvertices[imodel] > vrtoffset && vrtoffset > 0 )
    {
      tnc = madvertices[imodel] - vrtoffset;
      chrattachedgrip[ichr][0] = tnc;
      chrattachedgrip[ichr][1] = tnc + 1;
      chrattachedgrip[ichr][2] = tnc + 2;
      chrattachedgrip[ichr][3] = tnc + 3;
    }
    else
    {
      chrattachedgrip[ichr][0] =
      chrattachedgrip[ichr][1] =
      chrattachedgrip[ichr][2] =
      chrattachedgrip[ichr][3] = madvertices[imodel] - 1;
    }
  }

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    item = chr_get_holdingwhich( ichr, _slot );
    if ( VALID_CHR( item ) )
    {
      tnc = madvertices[chrmodel[ichr]] - slot_to_grip( _slot );
      chrattachedgrip[item][0] = tnc;
      chrattachedgrip[item][1] = tnc + 1;
      chrattachedgrip[item][2] = tnc + 2;
      chrattachedgrip[item][3] = tnc + 3;
    }
  }

  // Image rendering
  chruoffset_fp8[ichr] = 0;
  chrvoffset_fp8[ichr] = 0;
  chruoffvel[ichr] = capuoffvel[new_profile];
  chrvoffvel[ichr] = capvoffvel[new_profile];


  // Movement
  chrsneakspd[ichr] = capsneakspd[new_profile];
  chrwalkspd[ichr] = capwalkspd[new_profile];
  chrrunspd[ichr] = caprunspd[new_profile];


  // AI and action stuff
  chractionready[ichr] = btrue;
  chrkeepaction[ichr] = bfalse;
  chrloopaction[ichr] = bfalse;
  chraction[ichr] = ACTION_DA;
  chrnextaction[ichr] = ACTION_DA;
  chrlip_fp8[ichr] = 0;
  chrflip[ichr] = 0.0f;
  chrframe[ichr] = madframestart[new_profile];
  chrlastframe[ichr] = chrframe[ichr];
  chrholdingweight[ichr] = 0;
  chronwhichplatform[ichr] = MAXCHR;


  // Set the new_skin
  change_armor( ichr, new_skin );


  // Reaffirm them particles...
  chrreaffirmdamagetype[ichr] = capattachedprtreaffirmdamagetype[new_profile];
  reaffirm_attached_particles( ichr );


  // Set up initial fade in lighting
  tnc = 0;
  while ( tnc < madtransvertices[chrmodel[ichr]] )
  {
    chrvrtar_fp8[ichr][tnc] =
      chrvrtag_fp8[ichr][tnc] =
        chrvrtab_fp8[ichr][tnc] = 0;
    tnc++;
  }
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_search_target_in_block( int block_x, int block_y, CHR_REF character, bool_t ask_items,
                                    bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz,
                                    bool_t excludeid )
{
  // ZZ> This is a good little helper, that returns != MAXCHR if a suitable target was found

  int cnt;
  CHR_REF charb;
  Uint32 fanblock;
  TEAM team;
  vect3 diff;
  float dist;

  bool_t require_friends =  ask_friends && !ask_enemies;
  bool_t require_enemies = !ask_friends &&  ask_enemies;
  bool_t require_alive   = !ask_dead;
  bool_t require_noitems = !ask_items;
  bool_t ballowed;

  if ( !VALID_CHR( character ) ) return MAXCHR;

  fanblock = mesh_convert_block( block_x, block_y );
  team = chrteam[character];
  for ( cnt = 0, charb = bumplistchr[fanblock];
        cnt < bumplistchrnum[fanblock] && VALID_CHR( charb );
        cnt++, charb = chr_get_bumpnext(charb) )
  {
    // don't find stupid stuff
    if ( !VALID_CHR( charb ) || 0.0f == chrbumpstrength[charb] ) continue;

    // don't find yourself or any of the items you're holding
    if ( character == charb || chrattachedto[charb] == character || chrinwhichpack[charb] == character ) continue;

    // don't find your mount or your master
    if ( chrattachedto[character] == charb || chrinwhichpack[character] == charb ) continue;

    // don't find anything you can't see
    if (( !seeinvisible && chr_is_invisible( charb ) ) || chr_in_pack( charb ) ) continue;

    // if we need to find friends, don't find enemies
    if ( require_friends && teamhatesteam[team][chrteam[charb]] ) continue;

    // if we need to find enemies, don't find friends or invictus
    if ( require_enemies && ( !teamhatesteam[team][chrteam[charb]] || chrinvictus[charb] ) ) continue;

    // if we require being alive, don't accept dead things
    if ( require_alive && !chralive[charb] ) continue;

    // if we require not an item, don't accept items
    if ( require_noitems && chrisitem[charb] ) continue;

    ballowed = bfalse;
    if ( IDSZ_NONE == idsz )
    {
      ballowed = btrue;
    }
    else if ( CAP_INHERIT_IDSZ( chrmodel[charb], idsz ) )
    {
      ballowed = !excludeid;
    }

    if ( ballowed )
    {
      diff.x = chrpos[character].x - chrpos[charb].x;
      diff.y = chrpos[character].y - chrpos[charb].y;
      diff.z = chrpos[character].z - chrpos[charb].z;

      dist = DotProduct(diff, diff);
      if ( search_initialize || dist < search_distance )
      {
        search_distance   = dist;
        search_besttarget = charb;
        search_initialize = bfalse;
      }
    }

  }

  return search_besttarget;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_search_nearby_target( CHR_REF character, bool_t ask_items,
                                  bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ ask_idsz )
{
  // ZZ> This function finds a nearby target, or it returns MAXCHR if it can't find one
  int ix,ix_min,ix_max, iy,iy_min,iy_max;
  bool_t seeinvisible = chrcanseeinvisible[character];

  if ( !VALID_CHR( character ) || chr_in_pack( character ) ) return MAXCHR;

  search_initialize = btrue;
  search_besttarget = MAXCHR;

  // Current fanblock
  ix_min = MESH_FLOAT_TO_BLOCK( mesh_clip_x( chrpos[character].x - chrbumpsize[character] ) );
  ix_max = MESH_FLOAT_TO_BLOCK( mesh_clip_x( chrpos[character].x + chrbumpsize[character] ) );
  iy_min = MESH_FLOAT_TO_BLOCK( mesh_clip_y( chrpos[character].y - chrbumpsize[character] ) );
  iy_max = MESH_FLOAT_TO_BLOCK( mesh_clip_y( chrpos[character].y + chrbumpsize[character] ) );

  for( ix = ix_min; ix<=ix_max; ix++ )
  {
    for( iy=iy_min; iy<=iy_max; iy++ )
    {
      chr_search_target_in_block( ix, iy, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, ask_idsz, bfalse );
    };
  };

  return ( VALID_CHR(search_besttarget) && (search_besttarget!=character) ? search_besttarget : MAXCHR );
}

//--------------------------------------------------------------------------------------------
bool_t cost_mana( CHR_REF character, int amount, Uint16 killer )
{
  // ZZ> This function takes mana from a character ( or gives mana ),
  //     and returns btrue if the character had enough to pay, or bfalse
  //     otherwise
  int iTmp;


  iTmp = chrmana_fp8[character] - amount;
  if ( iTmp < 0 )
  {
    chrmana_fp8[character] = 0;
    if ( chrcanchannel[character] )
    {
      chrlife_fp8[character] += iTmp;
      if ( chrlife_fp8[character] <= 0 )
      {
        kill_character( character, character );
      }
      return btrue;
    }
    return bfalse;
  }
  else
  {
    chrmana_fp8[character] = iTmp;
    if ( iTmp > chrmanamax_fp8[character] )
    {
      chrmana_fp8[character] = chrmanamax_fp8[character];
    }
  }
  return btrue;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_search_distant_target( CHR_REF character, int maxdist, bool_t ask_enemies, bool_t ask_dead )
{
  // ZZ> This function finds a target, or it returns MAXCHR if it can't find one...
  //     maxdist should be the square of the actual dist you want to use
  //     as the cutoff...
  int charb, dist, mindist, xdist, ydist, zdist;
  Uint16 minchr = MAXCHR;
  bool_t require_friends = !ask_enemies;
  bool_t require_alive   = !ask_dead;
  TEAM team;

  if ( !VALID_CHR( character ) ) return MAXCHR;

  team = chrteam[character];
  minchr = MAXCHR;
  mindist = maxdist;
  for ( charb = 0; charb < MAXCHR; charb++ )
  {
    // don't find stupid items
    if ( !VALID_CHR( charb ) || 0.0f == chrbumpstrength[charb] ) continue;

    // don't find yourself or items you are carrying
    if ( character == charb || chrattachedto[charb] == character || chrinwhichpack[charb] == character ) continue;

    // don't find thigs you can't see
    if (( !chrcanseeinvisible[character] && chr_is_invisible( charb ) ) || chr_in_pack( charb ) ) continue;

    // don't find dead things if not asked for
    if ( require_alive && ( !chralive[charb] || chrisitem[charb] ) ) continue;

    // don't find enemies unless asked for
    if ( ask_enemies && ( !teamhatesteam[team][chrteam[charb]] || chrinvictus[charb] ) ) continue;

    xdist = chrpos[charb].x - chrpos[character].x;
    ydist = chrpos[charb].y - chrpos[character].y;
    zdist = chrpos[charb].z - chrpos[character].z;
    dist = xdist * xdist + ydist * ydist + zdist * zdist;

    if ( dist < mindist )
    {
      mindist = dist;
      minchr  = charb;
    };
  }

  return minchr;
}

//--------------------------------------------------------------------------------------------
void switch_team( CHR_REF character, TEAM team )
{
  // ZZ> This function makes a character join another team...
  if ( team < TEAM_COUNT )
  {
    if ( !chrinvictus[character] )
    {
      teammorale[chrbaseteam[character]]--;
      teammorale[team]++;
    }
    if (( !chrismount[character] || !chr_using_slot( character, SLOT_SADDLE ) ) &&
        ( !chrisitem[character]  || !chr_attached( character ) ) )
    {
      chrteam[character] = team;
    }

    chrbaseteam[character] = team;
    if ( !VALID_CHR( team_get_leader( team ) ) )
    {
      teamleader[team] = character;
    }
  }
}

//--------------------------------------------------------------------------------------------
void chr_search_nearest_in_block( int block_x, int block_y, CHR_REF character, bool_t ask_items,
                                  bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz )
{
  // ZZ> This is a good little helper
  float dis, xdis, ydis, zdis;
  int cnt;
  TEAM team;
  CHR_REF charb;
  Uint32 fanblock;
  bool_t require_friends =  ask_friends && !ask_enemies;
  bool_t require_enemies = !ask_friends &&  ask_enemies;
  bool_t require_alive   = !ask_dead;
  bool_t require_noitems = !ask_items;


  // blocks that are off the mesh are not stored
  fanblock = mesh_convert_block( block_x, block_y );

  // if character is not defined, return
  if ( !VALID_CHR( character ) ) return;

  team     = chrteam[character];
  charb    = bumplistchr[fanblock];
  for ( cnt = 0; cnt < bumplistchrnum[fanblock] && VALID_CHR( charb ); cnt++, charb = chr_get_bumpnext(charb) )
  {
    // don't find stupid stuff
    if ( !VALID_CHR( charb ) || 0.0f == chrbumpstrength[charb] ) continue;

    // don't find yourself or any of the items you're holding
    if ( character == charb || chrattachedto[charb] == character || chrinwhichpack[charb] == character ) continue;

    // don't find your mount or your master
    if ( chrattachedto[character] == charb || chrinwhichpack[character] == charb ) continue;

    // don't find anything you can't see
    if (( !seeinvisible && chr_is_invisible( charb ) ) || chr_in_pack( charb ) ) continue;

    // if we need to find friends, don't find enemies
    if ( require_friends && teamhatesteam[team][chrteam[charb]] ) continue;

    // if we need to find enemies, don't find friends or invictus
    if ( require_enemies && ( !teamhatesteam[team][chrteam[charb]] || chrinvictus[charb] ) ) continue;

    // if we require being alive, don't accept dead things
    if ( require_alive && !chralive[charb] ) continue;

    // if we require not an item, don't accept items
    if ( require_noitems && chrisitem[charb] ) continue;

    if ( IDSZ_NONE == idsz || CAP_INHERIT_IDSZ( chrmodel[charb], idsz ) )
    {
      xdis = chrpos[character].x - chrpos[charb].x;
      ydis = chrpos[character].y - chrpos[charb].y;
      zdis = chrpos[character].z - chrpos[charb].z;
      xdis *= xdis;
      ydis *= ydis;
      zdis *= zdis;
      dis = xdis + ydis + zdis;
      if ( search_initialize || dis < search_distance )
      {
        search_nearest  = charb;
        search_distance = dis;
        search_initialize = bfalse;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_search_nearest_target( CHR_REF character, bool_t ask_items,
                                   bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz )
{
  // ZZ> This function finds an target, or it returns MAXCHR if it can't find one
  int x, y;
  bool_t seeinvisible = chrcanseeinvisible[character];

  if ( !VALID_CHR( character ) ) return MAXCHR;

  // Current fanblock
  x = MESH_FLOAT_TO_BLOCK( chrpos[character].x );
  y = MESH_FLOAT_TO_BLOCK( chrpos[character].y );

  search_initialize = btrue;
  chr_search_nearest_in_block( x + 0, y + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );

  if ( !VALID_CHR( search_nearest ) )
  {
    chr_search_nearest_in_block( x - 1, y + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x + 1, y + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x + 0, y - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x + 0, y + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
  };

  if ( !VALID_CHR( search_nearest ) )
  {
    chr_search_nearest_in_block( x - 1, y + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x + 1, y - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x - 1, y - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x + 1, y + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
  };

  if ( search_nearest == character )
    return MAXCHR;
  else
    return search_nearest;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_search_wide_target( CHR_REF character, bool_t ask_items,
                                bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz, bool_t excludeid )
{
  // ZZ> This function finds an target, or it returns MAXCHR if it can't find one
  int ix, iy;
  CHR_REF target;
  char seeinvisible;
  seeinvisible = chrcanseeinvisible[character];

  if ( !VALID_CHR( character ) ) return MAXCHR;

  search_initialize = btrue;
  search_besttarget = MAXCHR;

  // Current fanblock
  ix = MESH_FLOAT_TO_BLOCK( chrpos[character].x );
  iy = MESH_FLOAT_TO_BLOCK( chrpos[character].y );

  target = chr_search_target_in_block( ix + 0, iy + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  if ( VALID_CHR( search_besttarget ) && search_besttarget != character )  return search_besttarget;

  target = chr_search_target_in_block( ix - 1, iy + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix + 1, iy + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix + 0, iy - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix + 0, iy + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  if ( VALID_CHR( search_besttarget ) && search_besttarget != character )  return search_besttarget;

  target = chr_search_target_in_block( ix - 1, iy + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix + 1, iy - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix - 1, iy - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix + 1, iy + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  if ( VALID_CHR( search_besttarget ) && search_besttarget != character )  return search_besttarget;

  return MAXCHR;
}

//--------------------------------------------------------------------------------------------
void issue_clean( CHR_REF character )
{
  // ZZ> This function issues a clean up order to all teammates
  TEAM team;
  Uint16 cnt;


  team = chrteam[character];
  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( chrteam[cnt] == team && !chralive[cnt] )
    {
      chraitime[cnt] = 2;  // Don't let it think too much...
      chralert[cnt] = ALERT_CLEANEDUP;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
int restock_ammo( CHR_REF character, IDSZ idsz )
{
  // ZZ> This function restocks the characters ammo, if it needs ammo and if
  //     either its parent or type idsz match the given idsz.  This
  //     function returns the amount of ammo given.
  int amount, model;

  amount = 0;
  if ( character < MAXCHR )
  {
    if ( chron[character] )
    {
      model = chrmodel[character];
      if ( CAP_INHERIT_IDSZ( model, idsz ) )
      {
        if ( chrammo[character] < chrammomax[character] )
        {
          amount = chrammomax[character] - chrammo[character];
          chrammo[character] = chrammomax[character];
        }
      }
    }
  }
  return amount;
}

//--------------------------------------------------------------------------------------------
void signal_target( Uint16 target, Uint16 upper, Uint16 lower )
{
  if ( !VALID_CHR( target ) ) return;

  chrmessage[target] = ( upper << 16 ) | lower;
  chrmessagedata[target] = 0;
  chralert[target] |= ALERT_SIGNALED;
};


//--------------------------------------------------------------------------------------------
void signal_team( CHR_REF character, Uint32 message )
{
  // ZZ> This function issues an message for help to all teammates
  TEAM team;
  Uint8 counter;
  Uint16 cnt;

  team = chrteam[character];
  counter = 0;
  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( cnt ) || chrteam[cnt] != team ) continue;

    chrmessage[cnt] = message;
    chrmessagedata[cnt] = counter;
    chralert[cnt] |= ALERT_SIGNALED;
    counter++;
  }
}

//--------------------------------------------------------------------------------------------
void signal_idsz_index( Uint32 order, IDSZ idsz, IDSZ_INDEX index )
{
  // ZZ> This function issues an order to all characters with the a matching special IDSZ
  Uint8 counter;
  Uint16 cnt, model;

  counter = 0;
  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( cnt ) ) continue;

    model = chrmodel[cnt];

    if ( capidsz[model][index] != idsz ) continue;

    chrmessage[cnt]       = order;
    chrmessagedata[cnt] = counter;
    chralert[cnt] |= ALERT_SIGNALED;
    counter++;
  }
}

//--------------------------------------------------------------------------------------------
void set_alerts( CHR_REF character, float dUpdate )
{
  // ZZ> This function polls some alert conditions

  float waythresh;

  chraitime[character] -= dUpdate;
  if ( chraitime[character] < 0 ) chraitime[character] = 0.0f;

  waythresh = ( WAYTHRESH + chrbumpsize[character] ) * 0.5f;
  if ( ABS( chrpos[character].x - chraigotox[character][chraigoto[character]] ) < waythresh &&
       ABS( chrpos[character].y - chraigotoy[character][chraigoto[character]] ) < waythresh )
  {
    chralert[character] |= ALERT_ATWAYPOINT;
    chraigoto[character]++;
    if ( chraigoto[character] == chraigotoadd[character] )
    {
      chraigoto[character] = 0;
      if ( !capisequipment[chrmodel[character]] )
      {
        chralert[character] |= ALERT_ATLASTWAYPOINT;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void free_all_enchants()
{
  // ZZ> This functions frees all of the enchantments
  numfreeenchant = 0;
  while ( numfreeenchant < MAXENCHANT )
  {
    freeenchant[numfreeenchant] = numfreeenchant;
    encon[numfreeenchant] = bfalse;
    numfreeenchant++;
  }
}

//--------------------------------------------------------------------------------------------
Uint32 fget_damage_modifier( FILE * fileread )
{
  Uint32 iTmp = 0, tTmp;
  char cTmp;
  if ( NULL == fileread || feof( fileread ) ) return iTmp;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'F': iTmp = 0;             break;
    case 'T': iTmp = DAMAGE_INVERT; break;
    case 'C': iTmp = DAMAGE_CHARGE; break;
    case 'M': iTmp = DAMAGE_MANA;   break;
  };
  tTmp = fget_int( fileread );

  return iTmp | tTmp;
};

//--------------------------------------------------------------------------------------------
void load_one_enchant_profile( char* szLoadName, Uint16 profile )
{
  // ZZ> This function loads the enchantment associated with an object
  FILE* fileread;
  char cTmp;
  int iTmp;
  int num;
  IDSZ idsz;

  globalname = szLoadName;
  evevalid[profile] = bfalse;
  fileread = fs_fileOpen( PRI_NONE, NULL, szLoadName, "r" );
  if ( NULL == fileread ) return;

  // btrue/bfalse values
  everetarget[profile] = fget_next_bool( fileread );
  eveoverride[profile] = fget_next_bool( fileread );
  everemoveoverridden[profile] = fget_next_bool( fileread );
  evekillonend[profile] = fget_next_bool( fileread );
  evepoofonend[profile] = fget_next_bool( fileread );


  // More stuff
  evetime[profile] = fget_next_int( fileread );
  eveendmessage[profile] = fget_next_int( fileread );
  // make -1 the "never-ending enchant" marker
  if ( evetime[profile] == 0 ) evetime[profile] = -1;


  // Drain stuff
  eveownermana[profile] = fget_next_fixed( fileread );
  evetargetmana[profile] = fget_next_fixed( fileread );
  eveendifcantpay[profile] = fget_next_bool( fileread );
  eveownerlife[profile] = fget_next_fixed( fileread );
  evetargetlife[profile] = fget_next_fixed( fileread );


  // Specifics
  evedontdamagetype[profile] = fget_next_damage( fileread );
  eveonlydamagetype[profile] = fget_next_damage( fileread );
  everemovedbyidsz[profile] = fget_next_idsz( fileread );


  // Now the set values
  num = 0;
  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_damage( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_int( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_int( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_int( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_damage_modifier( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_damage_modifier( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_damage_modifier( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_damage_modifier( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_damage_modifier( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_damage_modifier( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_damage_modifier( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_damage_modifier( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_int( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_int( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_int( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_int( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_int( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_bool( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_bool( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  cTmp = fget_first_letter( fileread );
  evesetvalue[profile][num] = MIS_NORMAL;
  switch ( toupper( cTmp ) )
  {
    case 'R': evesetvalue[profile][num] = MIS_REFLECT; break;
    case 'D': evesetvalue[profile][num] = MIS_DEFLECT; break;
  };
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_float( fileread ) * 16;
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_bool( fileread );
  num++;

  evesetyesno[profile][num] = fget_next_bool( fileread );
  evesetvalue[profile][num] = fget_bool( fileread );
  num++;


  // Now read in the add values
  num = 0;
  eveaddvalue[profile][num] = fget_next_float( fileread ) * 16;
  num++;

  eveaddvalue[profile][num] = fget_next_float( fileread ) * 127;
  num++;

  eveaddvalue[profile][num] = fget_next_float( fileread ) * 127;
  num++;

  eveaddvalue[profile][num] = fget_next_float( fileread ) * 4;
  num++;

  eveaddvalue[profile][num] = fget_next_float( fileread ) * 127;
  num++;

  eveaddvalue[profile][num] = fget_next_int( fileread );
  num++;

  eveaddvalue[profile][num] = fget_next_int( fileread );
  num++;

  eveaddvalue[profile][num] = fget_next_int( fileread );
  num++;

  eveaddvalue[profile][num] = fget_next_int( fileread );
  num++;

  eveaddvalue[profile][num] = fget_next_int( fileread );
  num++;

  eveaddvalue[profile][num] = fget_next_float( fileread ) * 4;
  num++;

  eveaddvalue[profile][num] = fget_next_float( fileread ) * 4;
  num++;

  eveaddvalue[profile][num] = fget_next_float( fileread ) * 4;
  num++;

  eveaddvalue[profile][num] = fget_next_float( fileread ) * 4;
  num++;

  eveaddvalue[profile][num] = fget_next_float( fileread ) * 4;
  num++;

  eveaddvalue[profile][num] = fget_next_float( fileread ) * 4;
  num++;


  // Clear expansions...
  evecontspawntime[profile] = 0;
  evecontspawnamount[profile] = 0;
  evecontspawnfacingadd[profile] = 0;
  evecontspawnpip[profile] = 0;
  eveendsound[profile] = INVALID_SOUND;
  evestayifnoowner[profile] = 0;
  eveoverlay[profile] = 0;
  evecanseekurse[profile] = 0;
  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );

    if ( MAKE_IDSZ( "AMOU" ) == idsz )  evecontspawnamount[profile] = iTmp;
    else if ( MAKE_IDSZ( "TYPE" ) == idsz )  evecontspawnpip[profile] = iTmp;
    else if ( MAKE_IDSZ( "TIME" ) == idsz )  evecontspawntime[profile] = iTmp;
    else if ( MAKE_IDSZ( "FACE" ) == idsz )  evecontspawnfacingadd[profile] = iTmp;
    else if ( MAKE_IDSZ( "SEND" ) == idsz )  eveendsound[profile] = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "STAY" ) == idsz )  evestayifnoowner[profile] = iTmp;
    else if ( MAKE_IDSZ( "OVER" ) == idsz )  eveoverlay[profile] = iTmp;
    else if ( MAKE_IDSZ( "CKUR" ) == idsz )  evecanseekurse[profile] = iTmp;
  }

  evevalid[profile] = btrue;

  // All done ( finally )
  fs_fileClose( fileread );
}

//--------------------------------------------------------------------------------------------
Uint16 get_free_enchant()
{
  // ZZ> This function returns the next free enchantment or MAXENCHANT if there are none
  if ( numfreeenchant > 0 )
  {
    numfreeenchant--;
    return freeenchant[numfreeenchant];
  }
  return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void unset_enchant_value( Uint16 enchantindex, Uint8 valueindex )
{
  // ZZ> This function unsets a set value
  CHR_REF character;

  if ( encsetyesno[enchantindex][valueindex] )
  {
    character = enctarget[enchantindex];
    switch ( valueindex )
    {
      case SETDAMAGETYPE:
        chrdamagetargettype[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETNUMBEROFJUMPS:
        chrjumpnumberreset[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETLIFEBARCOLOR:
        chrlifecolor[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETMANABARCOLOR:
        chrmanacolor[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETSLASHMODIFIER:
        chrdamagemodifier_fp8[character][DAMAGE_SLASH] = encsetsave[enchantindex][valueindex];
        break;

      case SETCRUSHMODIFIER:
        chrdamagemodifier_fp8[character][DAMAGE_CRUSH] = encsetsave[enchantindex][valueindex];
        break;

      case SETPOKEMODIFIER:
        chrdamagemodifier_fp8[character][DAMAGE_POKE] = encsetsave[enchantindex][valueindex];
        break;

      case SETHOLYMODIFIER:
        chrdamagemodifier_fp8[character][DAMAGE_HOLY] = encsetsave[enchantindex][valueindex];
        break;

      case SETEVILMODIFIER:
        chrdamagemodifier_fp8[character][DAMAGE_EVIL] = encsetsave[enchantindex][valueindex];
        break;

      case SETFIREMODIFIER:
        chrdamagemodifier_fp8[character][DAMAGE_FIRE] = encsetsave[enchantindex][valueindex];
        break;

      case SETICEMODIFIER:
        chrdamagemodifier_fp8[character][DAMAGE_ICE] = encsetsave[enchantindex][valueindex];
        break;

      case SETZAPMODIFIER:
        chrdamagemodifier_fp8[character][DAMAGE_ZAP] = encsetsave[enchantindex][valueindex];
        break;

      case SETFLASHINGAND:
        chrflashand[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETLIGHTBLEND:
        chrlight_fp8[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETALPHABLEND:
        chralpha_fp8[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETSHEEN:
        chrsheen_fp8[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETFLYTOHEIGHT:
        chrflyheight[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETWALKONWATER:
        chrwaterwalk[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETCANSEEINVISIBLE:
        chrcanseeinvisible[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETMISSILETREATMENT:
        chrmissiletreatment[character] = encsetsave[enchantindex][valueindex];
        break;

      case SETCOSTFOREACHMISSILE:
        chrmissilecost[character] = encsetsave[enchantindex][valueindex];
        chrmissilehandler[character] = character;
        break;

      case SETMORPH:
        // Need special handler for when this is removed
        change_character( character, chrbasemodel[character], encsetsave[enchantindex][valueindex], LEAVE_ALL );
        chraimorphed[character] = btrue;
        break;

      case SETCHANNEL:
        chrcanchannel[character] = encsetsave[enchantindex][valueindex];
        break;

    }
    encsetyesno[enchantindex][valueindex] = bfalse;
  }
}

//--------------------------------------------------------------------------------------------
void remove_enchant_value( Uint16 enchantindex, Uint8 valueindex )
{
  // ZZ> This function undoes cumulative modification to character stats
  float fvaluetoadd;
  int valuetoadd;

  CHR_REF character = enctarget[enchantindex];
  switch ( valueindex )
  {
    case ADDJUMPPOWER:
      fvaluetoadd = encaddsave[enchantindex][valueindex] / 16.0;
      chrjump[character] -= fvaluetoadd;
      break;

    case ADDBUMPDAMPEN:
      fvaluetoadd = encaddsave[enchantindex][valueindex] / 128.0;
      chrbumpdampen[character] -= fvaluetoadd;
      break;

    case ADDBOUNCINESS:
      fvaluetoadd = encaddsave[enchantindex][valueindex] / 128.0;
      chrdampen[character] -= fvaluetoadd;
      break;

    case ADDDAMAGE:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrdamageboost[character] -= valuetoadd;
      break;

    case ADDSIZE:
      fvaluetoadd = encaddsave[enchantindex][valueindex] / 128.0;
      chrsizegoto[character] -= fvaluetoadd;
      chrsizegototime[character] = DELAY_RESIZE;
      break;

    case ADDACCEL:
      fvaluetoadd = encaddsave[enchantindex][valueindex] / 1000.0;
      chrmaxaccel[character] -= fvaluetoadd;
      break;

    case ADDRED:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrredshift[character] -= valuetoadd;
      break;

    case ADDGRN:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrgrnshift[character] -= valuetoadd;
      break;

    case ADDBLU:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrblushift[character] -= valuetoadd;
      break;

    case ADDDEFENSE:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrdefense_fp8[character] -= valuetoadd;
      break;

    case ADDMANA:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrmanamax_fp8[character] -= valuetoadd;
      chrmana_fp8[character] -= valuetoadd;
      if ( chrmana_fp8[character] < 0 ) chrmana_fp8[character] = 0;
      break;

    case ADDLIFE:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrlifemax_fp8[character] -= valuetoadd;
      chrlife_fp8[character] -= valuetoadd;
      if ( chrlife_fp8[character] < 1 ) chrlife_fp8[character] = 1;
      break;

    case ADDSTRENGTH:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrstrength_fp8[character] -= valuetoadd;
      break;

    case ADDWISDOM:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrwisdom_fp8[character] -= valuetoadd;
      break;

    case ADDINTELLIGENCE:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrintelligence_fp8[character] -= valuetoadd;
      break;

    case ADDDEXTERITY:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrdexterity_fp8[character] -= valuetoadd;
      break;

  }
}


