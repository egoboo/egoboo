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

#include "enchant.h"
#include "mesh.h"
#include "char.h"
#include "log.h"
#include "passage.h"
#include "particle.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include <assert.h>

SEARCH_CONTEXT g_search;

EVE EveList[MAXEVE];
ENC EncList[MAXENCHANT];

Uint16  numfreeenchant = 0;             // For allocating new ones
Uint16  freeenchant[MAXENCHANT];    //

//--------------------------------------------------------------------------------------------
void do_enchant_spawn( float dUpdate )
{
  // ZZ> This function lets enchantments spawn particles
  int cnt, tnc;
  Uint16 facing, particle, eve, character;


  cnt = 0;
  while ( cnt < MAXENCHANT )
  {
    if ( EncList[cnt].on )
    {
      eve = EncList[cnt].eve;
      if ( EveList[eve].contspawnamount > 0 )
      {
        EncList[cnt].spawntime -= dUpdate;
        if ( EncList[cnt].spawntime <= 0 ) EncList[cnt].spawntime = 0;

        if ( EncList[cnt].spawntime == 0 )
        {
          character = EncList[cnt].target;
          EncList[cnt].spawntime = EveList[eve].contspawntime;
          facing = ChrList[character].turn_lr;
          tnc = 0;
          while ( tnc < EveList[eve].contspawnamount )
          {
            particle = spawn_one_particle( 1.0f, ChrList[character].pos,
                                           facing, eve, EveList[eve].contspawnpip,
                                           MAXCHR, GRIP_LAST, ChrList[EncList[cnt].owner].team, EncList[cnt].owner, tnc, MAXCHR );
            facing += EveList[eve].contspawnfacingadd;
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
  while ( ChrList[cnt].firstenchant != MAXENCHANT )
  {
    remove_enchant( ChrList[cnt].firstenchant );
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
  if ( ChrList[character].isplayer && CData.DevMode ) return;

  if ( ChrList[character].alive && pdam->ibase >= 0 && pdam->irand >= 1 )
  {
    // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
    // This can also be used to lessen effectiveness of healing
    damage = generate_unsigned( pdam );
    basedamage = damage;
    damage >>= ( ChrList[character].damagemodifier_fp8[damagetype] & DAMAGE_SHIFT );


    // Allow charging (Invert damage to mana)
    if ( ChrList[character].damagemodifier_fp8[damagetype]&DAMAGE_CHARGE )
    {
      ChrList[character].mana_fp8 += damage;
      if ( ChrList[character].mana_fp8 > ChrList[character].manamax_fp8 )
      {
        ChrList[character].mana_fp8 = ChrList[character].manamax_fp8;
      }
      return;
    }

    // Mana damage (Deal damage to mana)
    if ( ChrList[character].damagemodifier_fp8[damagetype]&DAMAGE_MANA )
    {
      ChrList[character].mana_fp8 -= damage;
      if ( ChrList[character].mana_fp8 < 0 )
      {
        ChrList[character].mana_fp8 = 0;
      }
      return;
    }


    // Invert damage to heal
    if ( ChrList[character].damagemodifier_fp8[damagetype]&DAMAGE_INVERT )
      damage = -damage;


    // Remember the damage type
    ChrList[character].damagetypelast = damagetype;
    ChrList[character].directionlast = direction;


    // Do it already
    if ( damage > 0 )
    {
      // Only damage if not invincible
      if ( ChrList[character].damagetime == 0 && !ChrList[character].invictus )
      {
        model = ChrList[character].model;
        if ( HAS_SOME_BITS( effects, DAMFX_BLOC ) )
        {
          // Only damage if hitting from proper direction
          if ( HAS_SOME_BITS( MadList[ChrList[character].model].framefx[ChrList[character].anim.next], MADFX_INVICTUS ) )
          {
            // I Frame...
            direction -= CapList[model].iframefacing;
            left = ( ~CapList[model].iframeangle );
            right = CapList[model].iframeangle;

            // Check for shield
            if ( ChrList[character].action.now >= ACTION_PA && ChrList[character].action.now <= ACTION_PD )
            {
              // Using a shield?
              if ( ChrList[character].action.now < ACTION_PC )
              {
                // Check left hand
                CHR_REF iholder = chr_get_holdingwhich( character, SLOT_LEFT );
                if ( VALID_CHR( iholder ) )
                {
                  left  = ~CapList[iholder].iframeangle;
                  right = CapList[iholder].iframeangle;
                }
              }
              else
              {
                // Check right hand
                CHR_REF iholder = chr_get_holdingwhich( character, SLOT_RIGHT );
                if ( VALID_CHR( iholder ) )
                {
                  left  = ~CapList[iholder].iframeangle;
                  right = CapList[iholder].iframeangle;
                }
              }
            }
          }
          else
          {
            // N Frame
            direction -= CapList[model].nframefacing;
            left = ( ~CapList[model].nframeangle );
            right = CapList[model].nframeangle;
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
            ChrList[character].life_fp8 -= damage;
          }
          else
          {
            ChrList[character].life_fp8 -= FP8_MUL( damage, ChrList[character].defense_fp8 );
          }


          if ( basedamage > DAMAGE_MIN )
          {
            // Call for help if below 1/2 life
            if ( ChrList[character].life_fp8 < ( ChrList[character].lifemax_fp8 >> 1 ) ) //Zefz: Removed, because it caused guards to attack
              call_for_help( character );                    //when dispelling overlay spells (Faerie Light)

            // Spawn blud particles
            if ( CapList[model].bludlevel > BLUD_NONE && ( damagetype < DAMAGE_HOLY || CapList[model].bludlevel == BLUD_ULTRA ) )
            {
              spawn_one_particle( 1.0f, ChrList[character].pos,
                                  ChrList[character].turn_lr + direction, ChrList[character].model, CapList[model].bludprttype,
                                  MAXCHR, GRIP_LAST, ChrList[character].team, character, 0, MAXCHR );
            }
            // Set attack alert if it wasn't an accident
            if ( team == TEAM_DAMAGE )
            {
              ChrList[character].aiattacklast = MAXCHR;
            }
            else
            {
              // Don't alert the character too much if under constant fire
              if ( ChrList[character].carefultime == 0 )
              {
                // Don't let characters chase themselves...  That would be silly
                if ( attacker != character )
                {
                  ChrList[character].alert |= ALERT_ATTACKED;
                  ChrList[character].aiattacklast = attacker;
                  ChrList[character].carefultime = DELAY_CAREFUL;
                }
              }
            }
          }


          // Taking damage action
          action = ACTION_HA;
          if ( ChrList[character].life_fp8 < 0 )
          {
            // Character has died
            ChrList[character].alive = bfalse;
            disenchant_character( character );
            ChrList[character].action.keep = btrue;
            ChrList[character].life_fp8 = -1;
            ChrList[character].isplatform = btrue;
            ChrList[character].bumpdampen /= 2.0;
            action = ACTION_KA;
            stop_sound(ChrList[character].loopingchannel);    //Stop sound loops
            ChrList[character].loopingchannel = -1;
            // Give kill experience
            experience = CapList[model].experienceworth + ( ChrList[character].experience * CapList[model].experienceexchange );
            if ( VALID_CHR( attacker ) )
            {
              // Set target
              ChrList[character].aitarget = attacker;
              if ( team == TEAM_DAMAGE )  ChrList[character].aitarget = character;
              if ( team == TEAM_NULL )  ChrList[character].aitarget = character;
              // Award direct kill experience
              if ( TeamList[ChrList[attacker].team].hatesteam[ChrList[character].team] )
              {
                give_experience( attacker, experience, XP_KILLENEMY );
              }

              // Check for hated
              if ( CAP_INHERIT_IDSZ( model, CapList[ChrList[attacker].model].idsz[IDSZ_HATE] ) )
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
              if ( ChrList[tnc].on && ChrList[tnc].alive )
              {
                if ( ChrList[tnc].aitarget == character )
                {
                  ChrList[tnc].alert |= ALERT_TARGETKILLED;
                }
                if ( !TeamList[ChrList[tnc].team].hatesteam[team] && TeamList[ChrList[tnc].team].hatesteam[ChrList[character].team] )
                {
                  // All allies get team experience, but only if they also hate the dead guy's team
                  give_experience( tnc, experience, XP_TEAMKILL );
                }
              }
              tnc++;
            }

            // Check if it was a leader
            if ( team_get_leader( ChrList[character].team ) == character )
            {
              // It was a leader, so set more alerts
              tnc = 0;
              while ( tnc < MAXCHR )
              {
                if ( ChrList[tnc].on && ChrList[tnc].team == ChrList[character].team )
                {
                  // All folks on the leaders team get the alert
                  ChrList[tnc].alert |= ALERT_LEADERKILLED;
                }
                tnc++;
              }

              // The team now has no leader
              TeamList[ChrList[character].team].leader = search_best_leader( ChrList[character].team, character );
            }

            detach_character_from_mount( character, btrue, bfalse );
            action += ( rand() & 3 );
            play_action( character, action, bfalse );

            // Turn off all sounds if it's a player
            for ( tnc = 0; tnc < MAXWAVE; tnc++ )
            {
              //TODO Zefz: Do we need this? This makes all sounds a character makes stop when it dies...
              //           This may stop death sounds
              //stop_sound(ChrList[character].model);
            }

            // Afford it one last thought if it's an AI
            TeamList[ChrList[character].baseteam].morale--;
            ChrList[character].team = ChrList[character].baseteam;
            ChrList[character].alert = ALERT_KILLED;
            ChrList[character].sparkle = NOSPARKLE;
            ChrList[character].aitime = 1;  // No timeout...
            let_character_think( character, 1.0f );
          }
          else
          {
            if ( basedamage > DAMAGE_MIN )
            {
              action += ( rand() & 3 );
              play_action( character, action, bfalse );

              // Make the character invincible for a limited time only
              if ( HAS_NO_BITS( effects, DAMFX_TIME ) )
                ChrList[character].damagetime = DELAY_DAMAGE;
            }
          }
        }
        else
        {
          // Spawn a defend particle
          spawn_one_particle( ChrList[character].bumpstrength, ChrList[character].pos, ChrList[character].turn_lr, MAXMODEL, PRTPIP_DEFEND, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
          ChrList[character].damagetime = DELAY_DEFEND;
          ChrList[character].alert |= ALERT_BLOCKED;
        }
      }
    }
    else if ( damage < 0 )
    {
      ChrList[character].life_fp8 -= damage;
      if ( ChrList[character].life_fp8 > ChrList[character].lifemax_fp8 )  ChrList[character].life_fp8 = ChrList[character].lifemax_fp8;

      // Isssue an alert
      ChrList[character].alert |= ALERT_HEALED;
      ChrList[character].aiattacklast = attacker;
      if ( team != TEAM_DAMAGE )
      {
        ChrList[character].aiattacklast = MAXCHR;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void kill_character( CHR_REF character, Uint16 killer )
{
  // ZZ> This function kills a character...  MAXCHR killer for accidental death
  Uint8 modifier;

  if ( !ChrList[character].alive ) return;

  ChrList[character].damagetime = 0;
  ChrList[character].life_fp8 = 1;
  modifier = ChrList[character].damagemodifier_fp8[DAMAGE_CRUSH];
  ChrList[character].damagemodifier_fp8[DAMAGE_CRUSH] = 1;
  if ( VALID_CHR( killer ) )
  {
    PAIR ptemp = {512, 1};
    damage_character( character, 0, &ptemp, DAMAGE_CRUSH, ChrList[killer].team, killer, DAMFX_ARMO | DAMFX_BLOC );
  }
  else
  {
    PAIR ptemp = {512, 1};
    damage_character( character, 0, &ptemp, DAMAGE_CRUSH, TEAM_DAMAGE, chr_get_aibumplast( character ), DAMFX_ARMO | DAMFX_BLOC );
  }
  ChrList[character].damagemodifier_fp8[DAMAGE_CRUSH] = modifier;

  // try something here.
  ChrList[character].isplatform = btrue;
  ChrList[character].ismount  = bfalse;
}

//--------------------------------------------------------------------------------------------
void spawn_poof( CHR_REF character, Uint16 profile )
{
  // ZZ> This function spawns a character poof
  Uint16 sTmp;
  Uint16 origin;
  int iTmp;


  sTmp = ChrList[character].turn_lr;
  iTmp = 0;
  origin = chr_get_aiowner( character );
  while ( iTmp < CapList[profile].gopoofprtamount )
  {
    spawn_one_particle( 1.0f, ChrList[character].pos_old,
                        sTmp, profile, CapList[profile].gopoofprttype,
                        MAXCHR, GRIP_LAST, ChrList[character].team, origin, iTmp, MAXCHR );
    sTmp += CapList[profile].gopoofprtfacingadd;
    iTmp++;
  }
}

//--------------------------------------------------------------------------------------------
void naming_names( int profile )
{
  // ZZ> This function generates a random name
  int read, write, section, mychop;
  char cTmp;

  if ( CapList[profile].sectionsize[0] == 0 )
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
      if ( CapList[profile].sectionsize[section] != 0 )
      {
        mychop = CapList[profile].sectionstart[section] + ( rand() % CapList[profile].sectionsize[section] );
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
      CapList[profile].sectionsize[section] = chopinsection;
      CapList[profile].sectionstart[section] = numchop - chopinsection;
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
      CapList[cnt].sectionstart[tnc] = MAXCHOP;
      CapList[cnt].sectionsize[tnc] = 0;
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
    if ( ChrList[cnt].stickybutt && ChrList[cnt].on && ChrList[cnt].onwhichfan != INVALID_FAN )
    {
      twist = mesh_get_twist( ChrList[cnt].onwhichfan );
      ChrList[cnt].mapturn_lr = maptwist_lr[twist];
      ChrList[cnt].mapturn_ud = maptwist_ud[twist];
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
CHR_REF spawn_one_character( vect3 pos, int profile, TEAM team,
                            Uint8 skin, Uint16 facing, char *name, Uint16 override )
{
  // ZZ> This function spawns a character and returns the character's index number
  //     if it worked, MAXCHR otherwise
  int ichr, tnc;
  FILE * filewrite;
  CHR  * pchr;

  // open file for debug info logging
  if( CData.DevMode ) filewrite = fs_fileOpen( PRI_NONE, NULL, CData.debug_file, "a" );

  // Make sure the team is valid
  if ( team >= TEAM_COUNT ) team %= TEAM_COUNT;


  // Get a new character
  if ( !MadList[profile].used )
  {
    if(CData.DevMode)
    {
      fprintf( stderr, "spawn_one_character() - \n\tfailed to spawn : model %d doesn't exist\n", profile );
      fprintf( filewrite, "WARNING: spawn_one_character() - failed to spawn : model %d doesn't exist\n", profile );
      fs_fileClose( filewrite );
    }
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
      if(CData.DevMode)
      {
        fprintf( stderr, "spawn_one_character() - \n\tfailed to spawn : cannot find override index %d\n", override );
        fprintf( filewrite, "WARNING: spawn_one_character() - failed to spawn : cannot find override index %d\n", override );
        fs_fileClose( filewrite );
      }
      return MAXCHR;
    }
  }
  else
  {
    ichr = get_free_character();

    if ( MAXCHR == ichr )
    {
      if(CData.DevMode)
      {
        fprintf( stderr, "spawn_one_character() - \n\tfailed to spawn : get_free_character() returned invalid value %d\n", ichr );
        fprintf( filewrite, "WARNING: spawn_one_character() - failed to spawn : get_free_character() returned invalid value %d\n", ichr );
        fs_fileClose( filewrite );
      }
      return MAXCHR;
    }
  }

  if(CData.DevMode)
  {
    fprintf( stdout, "spawn_one_character() - \n\tprofile == %d, CapList[profile].classname == \"%s\", index == %d\n", profile, CapList[profile].classname, ichr );
    fprintf( filewrite, "SUCCESS: spawn_one_character() - profile == %d, CapList[profile].classname == \"%s\", index == %d\n", profile, CapList[profile].classname, ichr );
    fs_fileClose( filewrite );
  }

  // "simplify" the notation
  pchr = ChrList + ichr;

  // clear any old data
  memset(pchr, 0, sizeof(CHR));

  // IMPORTANT!!!
  pchr->indolist = bfalse;
  pchr->isequipped = bfalse;
  pchr->sparkle = NOSPARKLE;
  pchr->overlay = bfalse;
  pchr->missilehandler = ichr;

  // Set up model stuff
  pchr->on = btrue;
  pchr->freeme = bfalse;
  pchr->gopoof = bfalse;
  pchr->reloadtime = 0;
  pchr->inwhichslot = SLOT_NONE;
  pchr->inwhichpack = MAXCHR;
  pchr->nextinpack = MAXCHR;
  pchr->numinpack = 0;
  pchr->model = profile;
  VData_Blended_construct( &(pchr->vdata) );
  VData_Blended_Allocate( &(pchr->vdata), md2_get_numVertices(MadList[profile].md2_ptr) );

  pchr->basemodel = profile;
  pchr->stoppedby = CapList[profile].stoppedby;
  pchr->lifeheal = CapList[profile].lifeheal_fp8;
  pchr->manacost = CapList[profile].manacost_fp8;
  pchr->inwater = bfalse;
  pchr->nameknown = CapList[profile].nameknown;
  pchr->ammoknown = CapList[profile].nameknown;
  pchr->hitready = btrue;
  pchr->boretime = DELAY_BORE;
  pchr->carefultime = DELAY_CAREFUL;
  pchr->canbecrushed = bfalse;
  pchr->damageboost = 0;
  pchr->icon = CapList[profile].icon;

  //Ready for loop sound
  pchr->loopingchannel = -1;

  // Enchant stuff
  pchr->firstenchant = MAXENCHANT;
  pchr->undoenchant = MAXENCHANT;
  pchr->canseeinvisible = CapList[profile].canseeinvisible;
  pchr->canchannel = bfalse;
  pchr->missiletreatment = MIS_NORMAL;
  pchr->missilecost = 0;

  //Skill Expansions
  pchr->canseekurse = CapList[profile].canseekurse;
  pchr->canusedivine = CapList[profile].canusedivine;
  pchr->canusearcane = CapList[profile].canusearcane;
  pchr->candisarm = CapList[profile].candisarm;
  pchr->canjoust = CapList[profile].canjoust;
  pchr->canusetech = CapList[profile].canusetech;
  pchr->canusepoison = CapList[profile].canusepoison;
  pchr->canuseadvancedweapons = CapList[profile].canuseadvancedweapons;
  pchr->canbackstab = CapList[profile].canbackstab;
  pchr->canread = CapList[profile].canread;


  // Kurse state
  pchr->iskursed = (( rand() % 100 ) < CapList[profile].kursechance );
  if ( !CapList[profile].isitem )  pchr->iskursed = bfalse;


  // Ammo
  pchr->ammomax = CapList[profile].ammomax;
  pchr->ammo = CapList[profile].ammo;


  // Gender
  pchr->gender = CapList[profile].gender;
  if ( pchr->gender == GEN_RANDOM )  pchr->gender = GEN_FEMALE + ( rand() & 1 );

  // Team stuff
  pchr->team = team;
  pchr->baseteam = team;
  pchr->messagedata = TeamList[team].morale;
  if ( !CapList[profile].invictus )  TeamList[team].morale++;
  pchr->message = 0;
  // Firstborn becomes the leader
  if ( !VALID_CHR( team_get_leader( team ) ) )
  {
    TeamList[team].leader = ichr;
  }

  // Skin
  if ( CapList[profile].skinoverride != NOSKINOVERRIDE )
  {
    skin = CapList[profile].skinoverride % MAXSKIN;
  }
  if ( skin >= MadList[profile].skins )
  {
    skin = 0;
    if ( MadList[profile].skins > 1 )
    {
      skin = rand() % MadList[profile].skins;
    }
  }
  pchr->texture = MadList[profile].skinstart + skin;

  // Life and Mana
  pchr->alive = btrue;
  pchr->lifecolor = CapList[profile].lifecolor;
  pchr->manacolor = CapList[profile].manacolor;
  pchr->lifemax_fp8 = generate_unsigned( &CapList[profile].life_fp8 );
  pchr->life_fp8 = pchr->lifemax_fp8;
  pchr->lifereturn = CapList[profile].lifereturn_fp8;
  pchr->manamax_fp8 = generate_unsigned( &CapList[profile].mana_fp8 );
  pchr->manaflow_fp8 = generate_unsigned( &CapList[profile].manaflow_fp8 );
  pchr->manareturn_fp8 = generate_unsigned( &CapList[profile].manareturn_fp8 );  //>> MANARETURNSHIFT;
  pchr->mana_fp8 = pchr->manamax_fp8;

  // SWID
  pchr->strength_fp8 = generate_unsigned( &CapList[profile].strength_fp8 );
  pchr->wisdom_fp8 = generate_unsigned( &CapList[profile].wisdom_fp8 );
  pchr->intelligence_fp8 = generate_unsigned( &CapList[profile].intelligence_fp8 );
  pchr->dexterity_fp8 = generate_unsigned( &CapList[profile].dexterity_fp8 );

  // Damage
  pchr->defense_fp8 = CapList[profile].defense_fp8[skin];
  pchr->reaffirmdamagetype = CapList[profile].attachedprtreaffirmdamagetype;
  pchr->damagetargettype = CapList[profile].damagetargettype;
  tnc = 0;
  while ( tnc < MAXDAMAGETYPE )
  {
    pchr->damagemodifier_fp8[tnc] = CapList[profile].damagemodifier_fp8[tnc][skin];
    tnc++;
  }

  // AI stuff
  pchr->aitype = MadList[pchr->model].ai;
  pchr->isplayer = bfalse;
  pchr->islocalplayer = bfalse;
  pchr->alert = ALERT_SPAWNED;
  pchr->aistate = CapList[profile].stateoverride;
  pchr->aicontent = CapList[profile].contentoverride;
  pchr->aitarget = ichr;
  pchr->aiowner = ichr;
  pchr->aichild = ichr;
  pchr->aitime = 0;
  tnc = 0;
  while ( tnc < MAXSTOR )
  {
    pchr->aix[tnc] = 0;
    pchr->aiy[tnc] = 0;
    tnc++;
  }
  pchr->aimorphed = bfalse;

  pchr->latch.x = 0;
  pchr->latch.y = 0;
  pchr->latch.b = 0;
  pchr->turnmode = TURNMODE_VELOCITY;

  // Flags
  pchr->stickybutt = CapList[profile].stickybutt;
  pchr->openstuff = CapList[profile].canopenstuff;
  pchr->transferblend = CapList[profile].transferblend;
  pchr->enviro = CapList[profile].enviro;
  pchr->waterwalk = CapList[profile].waterwalk;
  pchr->isplatform = CapList[profile].isplatform;
  pchr->isitem = CapList[profile].isitem;
  pchr->invictus = CapList[profile].invictus;
  pchr->ismount = CapList[profile].ismount;
  pchr->cangrabmoney = CapList[profile].cangrabmoney;

  // Jumping
  pchr->jump = CapList[profile].jump;
  pchr->jumpready = btrue;
  pchr->jumpnumber = 1;
  pchr->jumpnumberreset = CapList[profile].jumpnumber;
  pchr->jumptime = DELAY_JUMP;

  // Other junk
  pchr->flyheight = CapList[profile].flyheight;
  pchr->maxaccel = CapList[profile].maxaccel[skin];
  pchr->alpha_fp8 = CapList[profile].alpha_fp8;
  pchr->light_fp8 = CapList[profile].light_fp8;
  pchr->flashand = CapList[profile].flashand;
  pchr->sheen_fp8 = CapList[profile].sheen_fp8;
  pchr->dampen = CapList[profile].dampen;

  // Character size and bumping
  pchr->fat = CapList[profile].size;
  pchr->sizegoto = pchr->fat;
  pchr->sizegototime = 0;

  pchr->bmpdata_save.shadow  = CapList[profile].shadowsize;
  pchr->bmpdata_save.size    = CapList[profile].bumpsize;
  pchr->bmpdata_save.sizebig = CapList[profile].bumpsizebig;
  pchr->bmpdata_save.height  = CapList[profile].bumpheight;

  pchr->bmpdata.shadow   = CapList[profile].shadowsize  * pchr->fat;
  pchr->bmpdata.size     = CapList[profile].bumpsize    * pchr->fat;
  pchr->bmpdata.sizebig  = CapList[profile].bumpsizebig * pchr->fat;
  pchr->bmpdata.height   = CapList[profile].bumpheight  * pchr->fat;
  pchr->bumpstrength   = CapList[profile].bumpstrength * FP8_TO_FLOAT( CapList[profile].alpha_fp8 );



  pchr->bumpdampen = CapList[profile].bumpdampen;
  pchr->weight = CapList[profile].weight * pchr->fat * pchr->fat * pchr->fat;   // preserve density
  pchr->aibumplast = ichr;
  pchr->aiattacklast = MAXCHR;
  pchr->aihitlast = ichr;

  // Grip info
  pchr->inwhichslot = SLOT_NONE;
  pchr->attachedto = MAXCHR;
  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    pchr->holdingwhich[_slot] = MAXCHR;
  }

  // Image rendering
  pchr->uoffset_fp8 = 0;
  pchr->voffset_fp8 = 0;
  pchr->uoffvel = CapList[profile].uoffvel;
  pchr->voffvel = CapList[profile].voffvel;
  pchr->redshift = 0;
  pchr->grnshift = 0;
  pchr->blushift = 0;


  // Movement
  pchr->sneakspd = CapList[profile].sneakspd;
  pchr->walkspd = CapList[profile].walkspd;
  pchr->runspd = CapList[profile].runspd;

  // Set up position
  pchr->pos.x = pos.x;
  pchr->pos.y = pos.y;
  pchr->turn_lr = facing;
  pchr->onwhichfan = mesh_get_fan( pchr->pos );
  pchr->level = mesh_get_level( pchr->onwhichfan, pchr->pos.x, pchr->pos.y, pchr->waterwalk ) + RAISE;
  if ( pos.z < pchr->level ) pos.z = pchr->level;
  pchr->pos.z = pos.z;

  pchr->stt         = pchr->pos;
  pchr->pos_old     = pchr->pos;
  pchr->turn_lr_old = pchr->turn_lr;

  pchr->lightturn_lrr = 0;
  pchr->lightturn_lrg = 0;
  pchr->lightturn_lrb = 0;

  pchr->vel.x = 0;
  pchr->vel.y = 0;
  pchr->vel.z = 0;
  pchr->trgvel.x = 0;
  pchr->trgvel.y = 0;
  pchr->trgvel.z = 0;
  pchr->mapturn_lr = 32768;  // These two mean on level surface
  pchr->mapturn_ud = 32768;
  pchr->scale = pchr->fat; // * MadList[pchr->model].scale * 4;

  // AI and action stuff
  pchr->aigoto = 0;
  pchr->aigotoadd = 1;
  pchr->aigotox[0] = pchr->pos.x;
  pchr->aigotoy[0] = pchr->pos.y;
  pchr->action.ready = btrue;
  pchr->action.keep = bfalse;
  pchr->action.loop = bfalse;
  pchr->action.now = ACTION_DA;
  pchr->action.next = ACTION_DA;
  pchr->anim.lip_fp8 = 0;
  pchr->anim.flip = 0.0f;
  pchr->anim.last = pchr->anim.next = 0;
  pchr->passage = 0;
  pchr->holdingweight = 0;
  pchr->onwhichplatform = MAXCHR;

  // Timers set to 0
  pchr->grogtime = 0.0f;
  pchr->dazetime = 0.0f;

  // Money is added later
  pchr->money = CapList[profile].money;

  // Name the character
  if ( name == NULL )
  {
    // Generate a random name
    naming_names( profile );
    strncpy( pchr->name, namingnames, sizeof( pchr->name ) );
  }
  else
  {
    // A name has been given
    tnc = 0;
    while ( tnc < MAXCAPNAMESIZE - 1 )
    {
      pchr->name[tnc] = name[tnc];
      tnc++;
    }
    pchr->name[tnc] = 0;
  }

  // Set up initial fade in lighting
  tnc = 0;
  while ( tnc < MadList[pchr->model].transvertices )
  {
    pchr->vrtar_fp8[tnc] = 0;
    pchr->vrtag_fp8[tnc] = 0;
    pchr->vrtab_fp8[tnc] = 0;
    tnc++;
  }

  // Particle attachments
  tnc = 0;
  while ( tnc < CapList[profile].attachedprtamount )
  {
    spawn_one_particle( 1.0f, pchr->pos,
                        0, pchr->model, CapList[profile].attachedprttype,
                        ichr, GRIP_LAST + tnc, pchr->team, ichr, tnc, MAXCHR );
    tnc++;
  }
  pchr->reaffirmdamagetype = CapList[profile].attachedprtreaffirmdamagetype;


  // Experience
  if ( CapList[profile].leveloverride != 0 )
  {
    while ( pchr->experiencelevel < CapList[profile].leveloverride )
    {
      give_experience( ichr, 100, XP_DIRECT );
    }
  }
  else
  {
    pchr->experience = generate_unsigned( &CapList[profile].experience );
    pchr->experiencelevel = calc_chr_level( ichr );
  }


  pchr->pancakepos.x = pchr->pancakepos.y = pchr->pancakepos.z = 1.0;
  pchr->pancakevel.x = pchr->pancakevel.y = pchr->pancakevel.z = 0.0f;

  pchr->loopingchannel = INVALID_CHANNEL;

  // calculate the bumpers
  assert(NULL == pchr->bmpdata.cv_tree);
  make_one_character_matrix( ichr );

  return ichr;
}

//--------------------------------------------------------------------------------------------
void respawn_character( CHR_REF character )
{
  // ZZ> This function respawns a character
  Uint16 item, profile;

  if ( !VALID_CHR( character ) || ChrList[character].alive ) return;

  profile = ChrList[character].model;

  spawn_poof( character, profile );
  disaffirm_attached_particles( character );
  ChrList[character].alive = btrue;
  ChrList[character].boretime = DELAY_BORE;
  ChrList[character].carefultime = DELAY_CAREFUL;
  ChrList[character].life_fp8 = ChrList[character].lifemax_fp8;
  ChrList[character].mana_fp8 = ChrList[character].manamax_fp8;
  ChrList[character].pos.x = ChrList[character].stt.x;
  ChrList[character].pos.y = ChrList[character].stt.y;
  ChrList[character].pos.z = ChrList[character].stt.z;
  ChrList[character].vel.x = 0;
  ChrList[character].vel.y = 0;
  ChrList[character].vel.z = 0;
  ChrList[character].trgvel.x = 0;
  ChrList[character].trgvel.y = 0;
  ChrList[character].trgvel.z = 0;
  ChrList[character].team = ChrList[character].baseteam;
  ChrList[character].canbecrushed = bfalse;
  ChrList[character].mapturn_lr = 32768;  // These two mean on level surface
  ChrList[character].mapturn_ud = 32768;
  if ( !VALID_CHR( team_get_leader( ChrList[character].team ) ) )  TeamList[ChrList[character].team].leader = character;
  if ( !ChrList[character].invictus )  TeamList[ChrList[character].baseteam].morale++;

  ChrList[character].action.ready = btrue;
  ChrList[character].action.keep = bfalse;
  ChrList[character].action.loop = bfalse;
  ChrList[character].action.now = ACTION_DA;
  ChrList[character].action.next = ACTION_DA;

  ChrList[character].anim.lip_fp8 = 0;
  ChrList[character].anim.flip = 0.0f;
  ChrList[character].anim.next = 0;
  ChrList[character].anim.last  = ChrList[character].anim.next;

  ChrList[character].isplatform = CapList[profile].isplatform;
  ChrList[character].flyheight  = CapList[profile].flyheight;
  ChrList[character].bumpdampen = CapList[profile].bumpdampen;

  ChrList[character].bmpdata_save.size    = CapList[profile].bumpsize;
  ChrList[character].bmpdata_save.sizebig = CapList[profile].bumpsizebig;
  ChrList[character].bmpdata_save.height  = CapList[profile].bumpheight;

  ChrList[character].bmpdata.size     = CapList[profile].bumpsize * ChrList[character].fat;
  ChrList[character].bmpdata.sizebig  = CapList[profile].bumpsizebig * ChrList[character].fat;
  ChrList[character].bmpdata.height   = CapList[profile].bumpheight * ChrList[character].fat;
  ChrList[character].bumpstrength = CapList[profile].bumpstrength * FP8_TO_FLOAT( CapList[profile].alpha_fp8 );

  // clear the alert and leave the state alone
  ChrList[character].alert = ALERT_NONE;
  ChrList[character].aitarget = character;
  ChrList[character].aitime = 0;
  ChrList[character].grogtime = 0.0f;
  ChrList[character].dazetime = 0.0f;
  reaffirm_attached_particles( character );


  // Let worn items come back
  item  = chr_get_nextinpack( character );
  while ( VALID_CHR( item ) )
  {
    if ( ChrList[item].isequipped )
    {
      ChrList[item].isequipped = bfalse;
      ChrList[item].alert |= ALERT_ATLASTWAYPOINT;  // doubles as PutAway
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
  enchant = ChrList[character].firstenchant;
  while ( enchant < MAXENCHANT )
  {
    for ( cnt = SETSLASHMODIFIER; cnt <= SETZAPMODIFIER; cnt++ )
    {
      unset_enchant_value( enchant, cnt );
    }
    enchant = EncList[enchant].nextenchant;
  }


  // Change the skin
  sTmp = ChrList[character].model;
  if ( skin > MadList[sTmp].skins )  skin = 0;
  ChrList[character].texture = MadList[sTmp].skinstart + skin;


  // Change stats associated with skin
  ChrList[character].defense_fp8 = CapList[sTmp].defense_fp8[skin];
  iTmp = 0;
  while ( iTmp < MAXDAMAGETYPE )
  {
    ChrList[character].damagemodifier_fp8[iTmp] = CapList[sTmp].damagemodifier_fp8[iTmp][skin];
    iTmp++;
  }
  ChrList[character].maxaccel = CapList[sTmp].maxaccel[skin];


  // Reset armor enchantments
  // These should really be done in reverse order ( Start with last enchant ), but
  // I don't care at this point !!!BAD!!!
  enchant = ChrList[character].firstenchant;
  while ( enchant < MAXENCHANT )
  {
    for ( cnt = SETSLASHMODIFIER; cnt <= SETZAPMODIFIER; cnt++ )
    {
      set_enchant_value( enchant, cnt, EncList[enchant].eve );
    };
    add_enchant_value( enchant, ADDACCEL, EncList[enchant].eve );
    add_enchant_value( enchant, ADDDEFENSE, EncList[enchant].eve );
    enchant = EncList[enchant].nextenchant;
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

  if ( new_profile > MAXMODEL || !MadList[new_profile].used ) return;

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    sTmp = chr_get_holdingwhich( ichr, _slot );
    if ( !CapList[new_profile].slotvalid[_slot] )
    {
      if ( detach_character_from_mount( sTmp, btrue, btrue ) )
      {
        if ( _slot == SLOT_SADDLE )
        {
          ChrList[sTmp].accum_vel.z += DISMOUNTZVEL;
          ChrList[sTmp].accum_pos.z += DISMOUNTZVEL;
          ChrList[sTmp].jumptime  = DELAY_JUMP;
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
        enchant = ChrList[ichr].firstenchant;
        if ( enchant != MAXENCHANT )
        {
          while ( EncList[enchant].nextenchant != MAXENCHANT )
          {
            remove_enchant( EncList[enchant].nextenchant );
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
  ChrList[ichr].model     = new_profile;
  VData_Blended_destruct( &(ChrList[numfreechr].vdata) );
  VData_Blended_construct( &(ChrList[numfreechr].vdata) );
  VData_Blended_Allocate( &(ChrList[numfreechr].vdata), md2_get_numVertices(MadList[new_profile].md2_ptr) );

  ChrList[ichr].stoppedby = CapList[new_profile].stoppedby;
  ChrList[ichr].lifeheal  = CapList[new_profile].lifeheal_fp8;
  ChrList[ichr].manacost  = CapList[new_profile].manacost_fp8;

  // Ammo
  ChrList[ichr].ammomax = CapList[new_profile].ammomax;
  ChrList[ichr].ammo = CapList[new_profile].ammo;
  // Gender
  if ( CapList[new_profile].gender != GEN_RANDOM ) // GEN_RANDOM means keep old gender
  {
    ChrList[ichr].gender = CapList[new_profile].gender;
  }


  // AI stuff
  ChrList[ichr].aitype = MadList[new_profile].ai;
  ChrList[ichr].aistate = 0;
  ChrList[ichr].aitime = 0;
  ChrList[ichr].latch.x = 0;
  ChrList[ichr].latch.y = 0;
  ChrList[ichr].latch.b = 0;
  ChrList[ichr].turnmode = TURNMODE_VELOCITY;

  // Flags
  ChrList[ichr].stickybutt = CapList[new_profile].stickybutt;
  ChrList[ichr].openstuff = CapList[new_profile].canopenstuff;
  ChrList[ichr].transferblend = CapList[new_profile].transferblend;
  ChrList[ichr].enviro = CapList[new_profile].enviro;
  ChrList[ichr].isplatform = CapList[new_profile].isplatform;
  ChrList[ichr].isitem = CapList[new_profile].isitem;
  ChrList[ichr].invictus = CapList[new_profile].invictus;
  ChrList[ichr].ismount = CapList[new_profile].ismount;
  ChrList[ichr].cangrabmoney = CapList[new_profile].cangrabmoney;
  ChrList[ichr].jumptime = DELAY_JUMP;

  // Character size and bumping
  ChrList[ichr].bmpdata_save.shadow  = CapList[new_profile].shadowsize;
  ChrList[ichr].bmpdata_save.size    = CapList[new_profile].bumpsize;
  ChrList[ichr].bmpdata_save.sizebig = CapList[new_profile].bumpsizebig;
  ChrList[ichr].bmpdata_save.height  = CapList[new_profile].bumpheight;

  ChrList[ichr].bmpdata.shadow   = CapList[new_profile].shadowsize * ChrList[ichr].fat;
  ChrList[ichr].bmpdata.size     = CapList[new_profile].bumpsize * ChrList[ichr].fat;
  ChrList[ichr].bmpdata.sizebig  = CapList[new_profile].bumpsizebig * ChrList[ichr].fat;
  ChrList[ichr].bmpdata.height   = CapList[new_profile].bumpheight * ChrList[ichr].fat;
  ChrList[ichr].bumpstrength     = CapList[new_profile].bumpstrength * FP8_TO_FLOAT( CapList[new_profile].alpha_fp8 );

  ChrList[ichr].bumpdampen = CapList[new_profile].bumpdampen;
  ChrList[ichr].weight = CapList[new_profile].weight * ChrList[ichr].fat * ChrList[ichr].fat * ChrList[ichr].fat;     // preserve density
  ChrList[ichr].scale = ChrList[ichr].fat;


  // Character scales...  Magic numbers
  imount = chr_get_attachedto( ichr );
  if ( VALID_CHR( imount ) )
  {
    Uint16 imodel =  ChrList[imount].model;
    Uint16 vrtoffset = slot_to_offset( ChrList[ichr].inwhichslot );

    if( !VALID_MDL(imodel) )
    {
      ChrList[ichr].attachedgrip[0] = 0;
      ChrList[ichr].attachedgrip[1] = 0xFFFF;
      ChrList[ichr].attachedgrip[2] = 0xFFFF;
      ChrList[ichr].attachedgrip[3] = 0xFFFF;
    }
    else if ( MadList[imodel].vertices > vrtoffset && vrtoffset > 0 )
    {
      tnc = MadList[imodel].vertices - vrtoffset;
      ChrList[ichr].attachedgrip[0] = tnc;
      ChrList[ichr].attachedgrip[1] = tnc + 1;
      ChrList[ichr].attachedgrip[2] = tnc + 2;
      ChrList[ichr].attachedgrip[3] = tnc + 3;
    }
    else
    {
      ChrList[ichr].attachedgrip[0] = MadList[imodel].vertices - 1;
      ChrList[ichr].attachedgrip[1] = 0xFFFF;
      ChrList[ichr].attachedgrip[2] = 0xFFFF;
      ChrList[ichr].attachedgrip[3] = 0xFFFF;
    }
  }

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    item = chr_get_holdingwhich( ichr, _slot );
    if ( VALID_CHR( item ) )
    {
      tnc = MadList[ChrList[ichr].model].vertices - slot_to_grip( _slot );
      ChrList[item].attachedgrip[0] = tnc;
      ChrList[item].attachedgrip[1] = tnc + 1;
      ChrList[item].attachedgrip[2] = tnc + 2;
      ChrList[item].attachedgrip[3] = tnc + 3;
    }
  }

  // Image rendering
  ChrList[ichr].uoffset_fp8 = 0;
  ChrList[ichr].voffset_fp8 = 0;
  ChrList[ichr].uoffvel = CapList[new_profile].uoffvel;
  ChrList[ichr].voffvel = CapList[new_profile].voffvel;


  // Movement
  ChrList[ichr].sneakspd = CapList[new_profile].sneakspd;
  ChrList[ichr].walkspd = CapList[new_profile].walkspd;
  ChrList[ichr].runspd = CapList[new_profile].runspd;


  // AI and action stuff
  ChrList[ichr].action.ready = btrue;
  ChrList[ichr].action.keep = bfalse;
  ChrList[ichr].action.loop = bfalse;
  ChrList[ichr].action.now = ACTION_DA;
  ChrList[ichr].action.next = ACTION_DA;

  ChrList[ichr].anim.lip_fp8 = 0;
  ChrList[ichr].anim.flip = 0.0f;
  ChrList[ichr].anim.next = 0;
  ChrList[ichr].anim.last = ChrList[ichr].anim.next;

  ChrList[ichr].holdingweight = 0;
  ChrList[ichr].onwhichplatform = MAXCHR;

  // Set the new_skin
  change_armor( ichr, new_skin );


  // Reaffirm them particles...
  ChrList[ichr].reaffirmdamagetype = CapList[new_profile].attachedprtreaffirmdamagetype;
  reaffirm_attached_particles( ichr );

  make_one_character_matrix(ichr);

  // Set up initial fade in lighting
  tnc = 0;
  while ( tnc < MadList[ChrList[ichr].model].transvertices )
  {
    ChrList[ichr].vrtar_fp8[tnc] =
    ChrList[ichr].vrtag_fp8[tnc] =
    ChrList[ichr].vrtab_fp8[tnc] = 0;
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
  team = ChrList[character].team;
  for ( cnt = 0, charb = bumplist.chr[fanblock];
        cnt < bumplist.num_chr[fanblock] && VALID_CHR( charb );
        cnt++, charb = chr_get_bumpnext(charb) )
  {
    // don't find stupid stuff
    if ( !VALID_CHR( charb ) || 0.0f == ChrList[charb].bumpstrength ) continue;

    // don't find yourself or any of the items you're holding
    if ( character == charb || ChrList[charb].attachedto == character || ChrList[charb].inwhichpack == character ) continue;

    // don't find your mount or your master
    if ( ChrList[character].attachedto == charb || ChrList[character].inwhichpack == charb ) continue;

    // don't find anything you can't see
    if (( !seeinvisible && chr_is_invisible( charb ) ) || chr_in_pack( charb ) ) continue;

    // if we need to find friends, don't find enemies
    if ( require_friends && TeamList[team].hatesteam[ChrList[charb].team] ) continue;

    // if we need to find enemies, don't find friends or invictus
    if ( require_enemies && ( !TeamList[team].hatesteam[ChrList[charb].team] || ChrList[charb].invictus ) ) continue;

    // if we require being alive, don't accept dead things
    if ( require_alive && !ChrList[charb].alive ) continue;

    // if we require not an item, don't accept items
    if ( require_noitems && ChrList[charb].isitem ) continue;

    ballowed = bfalse;
    if ( IDSZ_NONE == idsz )
    {
      ballowed = btrue;
    }
    else if ( CAP_INHERIT_IDSZ( ChrList[charb].model, idsz ) )
    {
      ballowed = !excludeid;
    }

    if ( ballowed )
    {
      diff.x = ChrList[character].pos.x - ChrList[charb].pos.x;
      diff.y = ChrList[character].pos.y - ChrList[charb].pos.y;
      diff.z = ChrList[character].pos.z - ChrList[charb].pos.z;

      dist = DotProduct(diff, diff);
      if ( g_search.initialize || dist < g_search.distance )
      {
        g_search.distance   = dist;
        g_search.besttarget = charb;
        g_search.initialize = bfalse;
      }
    }

  }

  return g_search.besttarget;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_search_nearby_target( CHR_REF character, bool_t ask_items,
                                  bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ ask_idsz )
{
  // ZZ> This function finds a nearby target, or it returns MAXCHR if it can't find one
  int ix,ix_min,ix_max, iy,iy_min,iy_max;
  bool_t seeinvisible = ChrList[character].canseeinvisible;

  if ( !VALID_CHR( character ) || chr_in_pack( character ) ) return MAXCHR;

  g_search.initialize = btrue;
  g_search.besttarget = MAXCHR;

  // Current fanblock
  ix_min = MESH_FLOAT_TO_BLOCK( mesh_clip_x( ChrList[character].bmpdata.cv.x_min ) );
  ix_max = MESH_FLOAT_TO_BLOCK( mesh_clip_x( ChrList[character].bmpdata.cv.x_max ) );
  iy_min = MESH_FLOAT_TO_BLOCK( mesh_clip_y( ChrList[character].bmpdata.cv.y_min ) );
  iy_max = MESH_FLOAT_TO_BLOCK( mesh_clip_y( ChrList[character].bmpdata.cv.y_max ) );

  for( ix = ix_min; ix<=ix_max; ix++ )
  {
    for( iy=iy_min; iy<=iy_max; iy++ )
    {
      chr_search_target_in_block( ix, iy, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, ask_idsz, bfalse );
    };
  };

  return ( VALID_CHR(g_search.besttarget) && (g_search.besttarget!=character) ? g_search.besttarget : MAXCHR );
}

//--------------------------------------------------------------------------------------------
bool_t cost_mana( CHR_REF character, int amount, Uint16 killer )
{
  // ZZ> This function takes mana from a character ( or gives mana ),
  //     and returns btrue if the character had enough to pay, or bfalse
  //     otherwise
  int iTmp;


  iTmp = ChrList[character].mana_fp8 - amount;
  if ( iTmp < 0 )
  {
    ChrList[character].mana_fp8 = 0;
    if ( ChrList[character].canchannel )
    {
      ChrList[character].life_fp8 += iTmp;
      if ( ChrList[character].life_fp8 <= 0 )
      {
        kill_character( character, character );
      }
      return btrue;
    }
    return bfalse;
  }
  else
  {
    ChrList[character].mana_fp8 = iTmp;
    if ( iTmp > ChrList[character].manamax_fp8 )
    {
      ChrList[character].mana_fp8 = ChrList[character].manamax_fp8;
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

  team = ChrList[character].team;
  minchr = MAXCHR;
  mindist = maxdist;
  for ( charb = 0; charb < MAXCHR; charb++ )
  {
    // don't find stupid items
    if ( !VALID_CHR( charb ) || 0.0f == ChrList[charb].bumpstrength ) continue;

    // don't find yourself or items you are carrying
    if ( character == charb || ChrList[charb].attachedto == character || ChrList[charb].inwhichpack == character ) continue;

    // don't find thigs you can't see
    if (( !ChrList[character].canseeinvisible && chr_is_invisible( charb ) ) || chr_in_pack( charb ) ) continue;

    // don't find dead things if not asked for
    if ( require_alive && ( !ChrList[charb].alive || ChrList[charb].isitem ) ) continue;

    // don't find enemies unless asked for
    if ( ask_enemies && ( !TeamList[team].hatesteam[ChrList[charb].team] || ChrList[charb].invictus ) ) continue;

    xdist = ChrList[charb].pos.x - ChrList[character].pos.x;
    ydist = ChrList[charb].pos.y - ChrList[character].pos.y;
    zdist = ChrList[charb].pos.z - ChrList[character].pos.z;
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
    if ( !ChrList[character].invictus )
    {
      TeamList[ChrList[character].baseteam].morale--;
      TeamList[team].morale++;
    }
    if (( !ChrList[character].ismount || !chr_using_slot( character, SLOT_SADDLE ) ) &&
        ( !ChrList[character].isitem  || !chr_attached( character ) ) )
    {
      ChrList[character].team = team;
    }

    ChrList[character].baseteam = team;
    if ( !VALID_CHR( team_get_leader( team ) ) )
    {
      TeamList[team].leader = character;
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

  team     = ChrList[character].team;
  charb    = bumplist.chr[fanblock];
  for ( cnt = 0; cnt < bumplist.num_chr[fanblock] && VALID_CHR( charb ); cnt++, charb = chr_get_bumpnext(charb) )
  {
    // don't find stupid stuff
    if ( !VALID_CHR( charb ) || 0.0f == ChrList[charb].bumpstrength ) continue;

    // don't find yourself or any of the items you're holding
    if ( character == charb || ChrList[charb].attachedto == character || ChrList[charb].inwhichpack == character ) continue;

    // don't find your mount or your master
    if ( ChrList[character].attachedto == charb || ChrList[character].inwhichpack == charb ) continue;

    // don't find anything you can't see
    if (( !seeinvisible && chr_is_invisible( charb ) ) || chr_in_pack( charb ) ) continue;

    // if we need to find friends, don't find enemies
    if ( require_friends && TeamList[team].hatesteam[ChrList[charb].team] ) continue;

    // if we need to find enemies, don't find friends or invictus
    if ( require_enemies && ( !TeamList[team].hatesteam[ChrList[charb].team] || ChrList[charb].invictus ) ) continue;

    // if we require being alive, don't accept dead things
    if ( require_alive && !ChrList[charb].alive ) continue;

    // if we require not an item, don't accept items
    if ( require_noitems && ChrList[charb].isitem ) continue;

    if ( IDSZ_NONE == idsz || CAP_INHERIT_IDSZ( ChrList[charb].model, idsz ) )
    {
      xdis = ChrList[character].pos.x - ChrList[charb].pos.x;
      ydis = ChrList[character].pos.y - ChrList[charb].pos.y;
      zdis = ChrList[character].pos.z - ChrList[charb].pos.z;
      xdis *= xdis;
      ydis *= ydis;
      zdis *= zdis;
      dis = xdis + ydis + zdis;
      if ( g_search.initialize || dis < g_search.distance )
      {
        g_search.nearest  = charb;
        g_search.distance = dis;
        g_search.initialize = bfalse;
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
  bool_t seeinvisible = ChrList[character].canseeinvisible;

  if ( !VALID_CHR( character ) ) return MAXCHR;

  // Current fanblock
  x = MESH_FLOAT_TO_BLOCK( ChrList[character].pos.x );
  y = MESH_FLOAT_TO_BLOCK( ChrList[character].pos.y );

  g_search.initialize = btrue;
  chr_search_nearest_in_block( x + 0, y + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );

  if ( !VALID_CHR( g_search.nearest ) )
  {
    chr_search_nearest_in_block( x - 1, y + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x + 1, y + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x + 0, y - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x + 0, y + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
  };

  if ( !VALID_CHR( g_search.nearest ) )
  {
    chr_search_nearest_in_block( x - 1, y + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x + 1, y - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x - 1, y - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_nearest_in_block( x + 1, y + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
  };

  if ( g_search.nearest == character )
    return MAXCHR;
  else
    return g_search.nearest;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_search_wide_target( CHR_REF character, bool_t ask_items,
                                bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz, bool_t excludeid )
{
  // ZZ> This function finds an target, or it returns MAXCHR if it can't find one
  int ix, iy;
  CHR_REF target;
  char seeinvisible;
  seeinvisible = ChrList[character].canseeinvisible;

  if ( !VALID_CHR( character ) ) return MAXCHR;

  g_search.initialize = btrue;
  g_search.besttarget = MAXCHR;

  // Current fanblock
  ix = MESH_FLOAT_TO_BLOCK( ChrList[character].pos.x );
  iy = MESH_FLOAT_TO_BLOCK( ChrList[character].pos.y );

  target = chr_search_target_in_block( ix + 0, iy + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  if ( VALID_CHR( g_search.besttarget ) && g_search.besttarget != character )  return g_search.besttarget;

  target = chr_search_target_in_block( ix - 1, iy + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix + 1, iy + 0, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix + 0, iy - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix + 0, iy + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  if ( VALID_CHR( g_search.besttarget ) && g_search.besttarget != character )  return g_search.besttarget;

  target = chr_search_target_in_block( ix - 1, iy + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix + 1, iy - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix - 1, iy - 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  target = chr_search_target_in_block( ix + 1, iy + 1, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  if ( VALID_CHR( g_search.besttarget ) && g_search.besttarget != character )  return g_search.besttarget;

  return MAXCHR;
}

//--------------------------------------------------------------------------------------------
void issue_clean( CHR_REF character )
{
  // ZZ> This function issues a clean up order to all teammates
  TEAM team;
  Uint16 cnt;


  team = ChrList[character].team;
  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( ChrList[cnt].team == team && !ChrList[cnt].alive )
    {
      ChrList[cnt].aitime = 2;  // Don't let it think too much...
      ChrList[cnt].alert = ALERT_CLEANEDUP;
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
    if ( ChrList[character].on )
    {
      model = ChrList[character].model;
      if ( CAP_INHERIT_IDSZ( model, idsz ) )
      {
        if ( ChrList[character].ammo < ChrList[character].ammomax )
        {
          amount = ChrList[character].ammomax - ChrList[character].ammo;
          ChrList[character].ammo = ChrList[character].ammomax;
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

  ChrList[target].message = ( upper << 16 ) | lower;
  ChrList[target].messagedata = 0;
  ChrList[target].alert |= ALERT_SIGNALED;
};


//--------------------------------------------------------------------------------------------
void signal_team( CHR_REF character, Uint32 message )
{
  // ZZ> This function issues an message for help to all teammates
  TEAM team;
  Uint8 counter;
  Uint16 cnt;

  team = ChrList[character].team;
  counter = 0;
  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( cnt ) || ChrList[cnt].team != team ) continue;

    ChrList[cnt].message = message;
    ChrList[cnt].messagedata = counter;
    ChrList[cnt].alert |= ALERT_SIGNALED;
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

    model = ChrList[cnt].model;

    if ( CapList[model].idsz[index] != idsz ) continue;

    ChrList[cnt].message       = order;
    ChrList[cnt].messagedata = counter;
    ChrList[cnt].alert |= ALERT_SIGNALED;
    counter++;
  }
}

//--------------------------------------------------------------------------------------------
void set_alerts( CHR_REF character, float dUpdate )
{
  // ZZ> This function polls some alert conditions

  ChrList[character].aitime -= dUpdate;
  if ( ChrList[character].aitime < 0 ) ChrList[character].aitime = 0.0f;

  if ( ABS( ChrList[character].pos.x - ChrList[character].aigotox[ChrList[character].aigoto] ) < WAYTHRESH &&
       ABS( ChrList[character].pos.y - ChrList[character].aigotoy[ChrList[character].aigoto] ) < WAYTHRESH )
  {
    ChrList[character].alert |= ALERT_ATWAYPOINT;
    ChrList[character].aigoto++;
    if ( ChrList[character].aigoto == ChrList[character].aigotoadd )
    {
      ChrList[character].aigoto = 0;
      if ( !CapList[ChrList[character].model].isequipment )
      {
        ChrList[character].alert |= ALERT_ATLASTWAYPOINT;
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
    EncList[numfreeenchant].on = bfalse;
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
  EveList[profile].valid = bfalse;
  fileread = fs_fileOpen( PRI_NONE, NULL, szLoadName, "r" );
  if ( NULL == fileread ) return;

  // btrue/bfalse values
  EveList[profile].retarget = fget_next_bool( fileread );
  EveList[profile].override = fget_next_bool( fileread );
  EveList[profile].removeoverridden = fget_next_bool( fileread );
  EveList[profile].killonend = fget_next_bool( fileread );
  EveList[profile].poofonend = fget_next_bool( fileread );


  // More stuff
  EveList[profile].time = fget_next_int( fileread );
  EveList[profile].endmessage = fget_next_int( fileread );
  // make -1 the "never-ending enchant" marker
  if ( EveList[profile].time == 0 ) EveList[profile].time = -1;


  // Drain stuff
  EveList[profile].ownermana = fget_next_fixed( fileread );
  EveList[profile].targetmana = fget_next_fixed( fileread );
  EveList[profile].endifcantpay = fget_next_bool( fileread );
  EveList[profile].ownerlife = fget_next_fixed( fileread );
  EveList[profile].targetlife = fget_next_fixed( fileread );


  // Specifics
  EveList[profile].dontdamagetype = fget_next_damage( fileread );
  EveList[profile].onlydamagetype = fget_next_damage( fileread );
  EveList[profile].removedbyidsz = fget_next_idsz( fileread );


  // Now the set values
  num = 0;
  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_damage( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_int( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_int( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_int( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_damage_modifier( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_damage_modifier( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_damage_modifier( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_damage_modifier( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_damage_modifier( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_damage_modifier( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_damage_modifier( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_damage_modifier( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_int( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_int( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_int( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_int( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_int( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_bool( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_bool( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  cTmp = fget_first_letter( fileread );
  EveList[profile].setvalue[num] = MIS_NORMAL;
  switch ( toupper( cTmp ) )
  {
    case 'R': EveList[profile].setvalue[num] = MIS_REFLECT; break;
    case 'D': EveList[profile].setvalue[num] = MIS_DEFLECT; break;
  };
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_float( fileread ) * 16;
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_bool( fileread );
  num++;

  EveList[profile].setyesno[num] = fget_next_bool( fileread );
  EveList[profile].setvalue[num] = fget_bool( fileread );
  num++;


  // Now read in the add values
  num = 0;
  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 16;
  num++;

  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 127;
  num++;

  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 127;
  num++;

  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 127;
  num++;

  EveList[profile].addvalue[num] = fget_next_int( fileread );
  num++;

  EveList[profile].addvalue[num] = fget_next_int( fileread );
  num++;

  EveList[profile].addvalue[num] = fget_next_int( fileread );
  num++;

  EveList[profile].addvalue[num] = fget_next_int( fileread );
  num++;

  EveList[profile].addvalue[num] = fget_next_int( fileread );
  num++;

  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  EveList[profile].addvalue[num] = fget_next_float( fileread ) * 4;
  num++;


  // Clear expansions...
  EveList[profile].contspawntime = 0;
  EveList[profile].contspawnamount = 0;
  EveList[profile].contspawnfacingadd = 0;
  EveList[profile].contspawnpip = 0;
  EveList[profile].endsound = INVALID_SOUND;
  EveList[profile].stayifnoowner = 0;
  EveList[profile].overlay = 0;
  EveList[profile].canseekurse = bfalse;

  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );

    if ( MAKE_IDSZ( "AMOU" ) == idsz )  EveList[profile].contspawnamount = iTmp;
    else if ( MAKE_IDSZ( "TYPE" ) == idsz )  EveList[profile].contspawnpip = iTmp;
    else if ( MAKE_IDSZ( "TIME" ) == idsz )  EveList[profile].contspawntime = iTmp;
    else if ( MAKE_IDSZ( "FACE" ) == idsz )  EveList[profile].contspawnfacingadd = iTmp;
    else if ( MAKE_IDSZ( "SEND" ) == idsz )  EveList[profile].endsound = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "STAY" ) == idsz )  EveList[profile].stayifnoowner = iTmp;
    else if ( MAKE_IDSZ( "OVER" ) == idsz )  EveList[profile].overlay = iTmp;
    else if ( MAKE_IDSZ( "CKUR" ) == idsz )  EveList[profile].overlay = (bfalse != iTmp);
  }

  EveList[profile].valid = btrue;

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

  if ( EncList[enchantindex].setyesno[valueindex] )
  {
    character = EncList[enchantindex].target;
    switch ( valueindex )
    {
      case SETDAMAGETYPE:
        ChrList[character].damagetargettype = EncList[enchantindex].setsave[valueindex];
        break;

      case SETNUMBEROFJUMPS:
        ChrList[character].jumpnumberreset = EncList[enchantindex].setsave[valueindex];
        break;

      case SETLIFEBARCOLOR:
        ChrList[character].lifecolor = EncList[enchantindex].setsave[valueindex];
        break;

      case SETMANABARCOLOR:
        ChrList[character].manacolor = EncList[enchantindex].setsave[valueindex];
        break;

      case SETSLASHMODIFIER:
        ChrList[character].damagemodifier_fp8[DAMAGE_SLASH] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETCRUSHMODIFIER:
        ChrList[character].damagemodifier_fp8[DAMAGE_CRUSH] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETPOKEMODIFIER:
        ChrList[character].damagemodifier_fp8[DAMAGE_POKE] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETHOLYMODIFIER:
        ChrList[character].damagemodifier_fp8[DAMAGE_HOLY] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETEVILMODIFIER:
        ChrList[character].damagemodifier_fp8[DAMAGE_EVIL] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETFIREMODIFIER:
        ChrList[character].damagemodifier_fp8[DAMAGE_FIRE] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETICEMODIFIER:
        ChrList[character].damagemodifier_fp8[DAMAGE_ICE] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETZAPMODIFIER:
        ChrList[character].damagemodifier_fp8[DAMAGE_ZAP] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETFLASHINGAND:
        ChrList[character].flashand = EncList[enchantindex].setsave[valueindex];
        break;

      case SETLIGHTBLEND:
        ChrList[character].light_fp8 = EncList[enchantindex].setsave[valueindex];
        break;

      case SETALPHABLEND:
        ChrList[character].alpha_fp8 = EncList[enchantindex].setsave[valueindex];
        break;

      case SETSHEEN:
        ChrList[character].sheen_fp8 = EncList[enchantindex].setsave[valueindex];
        break;

      case SETFLYTOHEIGHT:
        ChrList[character].flyheight = EncList[enchantindex].setsave[valueindex];
        break;

      case SETWALKONWATER:
        ChrList[character].waterwalk = EncList[enchantindex].setsave[valueindex];
        break;

      case SETCANSEEINVISIBLE:
        ChrList[character].canseeinvisible = EncList[enchantindex].setsave[valueindex];
        break;

      case SETMISSILETREATMENT:
        ChrList[character].missiletreatment = EncList[enchantindex].setsave[valueindex];
        break;

      case SETCOSTFOREACHMISSILE:
        ChrList[character].missilecost = EncList[enchantindex].setsave[valueindex];
        ChrList[character].missilehandler = character;
        break;

      case SETMORPH:
        // Need special handler for when this is removed
        change_character( character, ChrList[character].basemodel, EncList[enchantindex].setsave[valueindex], LEAVE_ALL );
        ChrList[character].aimorphed = btrue;
        break;

      case SETCHANNEL:
        ChrList[character].canchannel = EncList[enchantindex].setsave[valueindex];
        break;

    }
    EncList[enchantindex].setyesno[valueindex] = bfalse;
  }
}

//--------------------------------------------------------------------------------------------
void remove_enchant_value( Uint16 enchantindex, Uint8 valueindex )
{
  // ZZ> This function undoes cumulative modification to character stats
  float fvaluetoadd;
  int valuetoadd;

  CHR_REF character = EncList[enchantindex].target;
  switch ( valueindex )
  {
    case ADDJUMPPOWER:
      fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 16.0;
      ChrList[character].jump -= fvaluetoadd;
      break;

    case ADDBUMPDAMPEN:
      fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 128.0;
      ChrList[character].bumpdampen -= fvaluetoadd;
      break;

    case ADDBOUNCINESS:
      fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 128.0;
      ChrList[character].dampen -= fvaluetoadd;
      break;

    case ADDDAMAGE:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].damageboost -= valuetoadd;
      break;

    case ADDSIZE:
      fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 128.0;
      ChrList[character].sizegoto -= fvaluetoadd;
      ChrList[character].sizegototime = DELAY_RESIZE;
      break;

    case ADDACCEL:
      fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 1000.0;
      ChrList[character].maxaccel -= fvaluetoadd;
      break;

    case ADDRED:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].redshift -= valuetoadd;
      break;

    case ADDGRN:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].grnshift -= valuetoadd;
      break;

    case ADDBLU:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].blushift -= valuetoadd;
      break;

    case ADDDEFENSE:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].defense_fp8 -= valuetoadd;
      break;

    case ADDMANA:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].manamax_fp8 -= valuetoadd;
      ChrList[character].mana_fp8 -= valuetoadd;
      if ( ChrList[character].mana_fp8 < 0 ) ChrList[character].mana_fp8 = 0;
      break;

    case ADDLIFE:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].lifemax_fp8 -= valuetoadd;
      ChrList[character].life_fp8 -= valuetoadd;
      if ( ChrList[character].life_fp8 < 1 ) ChrList[character].life_fp8 = 1;
      break;

    case ADDSTRENGTH:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].strength_fp8 -= valuetoadd;
      break;

    case ADDWISDOM:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].wisdom_fp8 -= valuetoadd;
      break;

    case ADDINTELLIGENCE:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].intelligence_fp8 -= valuetoadd;
      break;

    case ADDDEXTERITY:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].dexterity_fp8 -= valuetoadd;
      break;

  }
}


