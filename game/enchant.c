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
#include "Md2.inl"
#include "script.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include <assert.h>

SEARCH_CONTEXT _g_search;

EVE EveList[MAXEVE];
ENC EncList[MAXENCHANT];

Uint16  numfreeenchant = 0;         // For allocating new ones
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

  AI_STATE * pstate;
  CHR  * pchr;
  CAP  * pcap;
  MAD  * pmad;

  // open file for debug info logging
  if( CData.DevMode ) filewrite = fs_fileOpen( PRI_NONE, NULL, CData.debug_file, "a" );

  // Make sure the team is valid
  if ( team >= TEAM_COUNT ) team %= TEAM_COUNT;

  pcap = CapList + profile;
  pmad = MadList + profile;

  // Get a new character
  if ( !pmad->used )
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
    fprintf( stdout, "spawn_one_character() - \n\tprofile == %d, pcap->classname == \"%s\", index == %d\n", profile, pcap->classname, ichr );
    fprintf( filewrite, "SUCCESS: spawn_one_character() - profile == %d, pcap->classname == \"%s\", index == %d\n", profile, pcap->classname, ichr );
    fs_fileClose( filewrite );
  }

  // "simplify" the notation
  pchr   = ChrList + ichr;
  pstate = &(pchr->aistate);

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
  VData_Blended_Allocate( &(pchr->vdata), md2_get_numVertices(pmad->md2_ptr) );

  pchr->basemodel = profile;
  pchr->stoppedby = pcap->stoppedby;
  pchr->lifeheal = pcap->lifeheal_fp8;
  pchr->manacost = pcap->manacost_fp8;
  pchr->inwater = bfalse;
  pchr->nameknown = pcap->nameknown;
  pchr->ammoknown = pcap->nameknown;
  pchr->hitready = btrue;
  pchr->boretime = DELAY_BORE;
  pchr->carefultime = DELAY_CAREFUL;
  pchr->canbecrushed = bfalse;
  pchr->damageboost = 0;
  pchr->icon = pcap->icon;

  //Ready for loop sound
  pchr->loopingchannel = -1;

  // Enchant stuff
  pchr->firstenchant = MAXENCHANT;
  pchr->undoenchant = MAXENCHANT;
  pchr->canseeinvisible = pcap->canseeinvisible;
  pchr->canchannel = bfalse;
  pchr->missiletreatment = MIS_NORMAL;
  pchr->missilecost = 0;

  //Skill Expansions
  pchr->canseekurse = pcap->canseekurse;
  pchr->canusedivine = pcap->canusedivine;
  pchr->canusearcane = pcap->canusearcane;
  pchr->candisarm = pcap->candisarm;
  pchr->canjoust = pcap->canjoust;
  pchr->canusetech = pcap->canusetech;
  pchr->canusepoison = pcap->canusepoison;
  pchr->canuseadvancedweapons = pcap->canuseadvancedweapons;
  pchr->canbackstab = pcap->canbackstab;
  pchr->canread = pcap->canread;


  // Kurse state
  pchr->iskursed = (( rand() % 100 ) < pcap->kursechance );
  if ( !pcap->isitem )  pchr->iskursed = bfalse;


  // Ammo
  pchr->ammomax = pcap->ammomax;
  pchr->ammo = pcap->ammo;


  // Gender
  pchr->gender = pcap->gender;
  if ( pchr->gender == GEN_RANDOM )  pchr->gender = GEN_FEMALE + ( rand() & 1 );

  // Team stuff
  pchr->team = team;
  pchr->baseteam = team;
  pchr->messagedata = TeamList[team].morale;
  if ( !pcap->invictus )  TeamList[team].morale++;
  pchr->message = 0;
  // Firstborn becomes the leader
  if ( !VALID_CHR( team_get_leader( team ) ) )
  {
    TeamList[team].leader = ichr;
  }

  // Skin
  if ( pcap->skinoverride != NOSKINOVERRIDE )
  {
    skin = pcap->skinoverride % MAXSKIN;
  }
  if ( skin >= pmad->skins )
  {
    skin = 0;
    if ( pmad->skins > 1 )
    {
      skin = rand() % pmad->skins;
    }
  }
  pchr->texture = pmad->skinstart + skin;

  // Life and Mana
  pchr->alive = btrue;
  pchr->lifecolor = pcap->lifecolor;
  pchr->manacolor = pcap->manacolor;
  pchr->lifemax_fp8 = generate_unsigned( &pcap->life_fp8 );
  pchr->life_fp8 = pchr->lifemax_fp8;
  pchr->lifereturn = pcap->lifereturn_fp8;
  pchr->manamax_fp8 = generate_unsigned( &pcap->mana_fp8 );
  pchr->manaflow_fp8 = generate_unsigned( &pcap->manaflow_fp8 );
  pchr->manareturn_fp8 = generate_unsigned( &pcap->manareturn_fp8 );  //>> MANARETURNSHIFT;
  pchr->mana_fp8 = pchr->manamax_fp8;

  // SWID
  pchr->strength_fp8 = generate_unsigned( &pcap->strength_fp8 );
  pchr->wisdom_fp8 = generate_unsigned( &pcap->wisdom_fp8 );
  pchr->intelligence_fp8 = generate_unsigned( &pcap->intelligence_fp8 );
  pchr->dexterity_fp8 = generate_unsigned( &pcap->dexterity_fp8 );

  // Damage
  pchr->defense_fp8 = pcap->defense_fp8[skin];
  pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;
  pchr->damagetargettype = pcap->damagetargettype;
  tnc = 0;
  while ( tnc < MAXDAMAGETYPE )
  {
    pchr->damagemodifier_fp8[tnc] = pcap->damagemodifier_fp8[tnc][skin];
    tnc++;
  }

  pchr->isplayer = bfalse;
  pchr->islocalplayer = bfalse;

  // AI stuff
  ai_state_new(pstate, ichr);

  // Flags
  pchr->stickybutt = pcap->stickybutt;
  pchr->openstuff = pcap->canopenstuff;
  pchr->transferblend = pcap->transferblend;
  pchr->enviro = pcap->enviro;
  pchr->waterwalk = pcap->waterwalk;
  pchr->isplatform = pcap->isplatform;
  pchr->isitem = pcap->isitem;
  pchr->invictus = pcap->invictus;
  pchr->ismount = pcap->ismount;
  pchr->cangrabmoney = pcap->cangrabmoney;

  // Jumping
  pchr->jump = pcap->jump;
  pchr->jumpready = btrue;
  pchr->jumpnumber = 1;
  pchr->jumpnumberreset = pcap->jumpnumber;
  pchr->jumptime = DELAY_JUMP;

  // Other junk
  pchr->flyheight = pcap->flyheight;
  pchr->maxaccel = pcap->maxaccel[skin];
  pchr->alpha_fp8 = pcap->alpha_fp8;
  pchr->light_fp8 = pcap->light_fp8;
  pchr->flashand = pcap->flashand;
  pchr->sheen_fp8 = pcap->sheen_fp8;
  pchr->dampen = pcap->dampen;

  // Character size and bumping
  pchr->fat = pcap->size;
  pchr->sizegoto = pchr->fat;
  pchr->sizegototime = 0;

  pchr->bmpdata_save.shadow  = pcap->shadowsize;
  pchr->bmpdata_save.size    = pcap->bumpsize;
  pchr->bmpdata_save.sizebig = pcap->bumpsizebig;
  pchr->bmpdata_save.height  = pcap->bumpheight;

  pchr->bmpdata.shadow   = pcap->shadowsize  * pchr->fat;
  pchr->bmpdata.size     = pcap->bumpsize    * pchr->fat;
  pchr->bmpdata.sizebig  = pcap->bumpsizebig * pchr->fat;
  pchr->bmpdata.height   = pcap->bumpheight  * pchr->fat;
  pchr->bumpstrength   = pcap->bumpstrength * FP8_TO_FLOAT( pcap->alpha_fp8 );



  pchr->bumpdampen = pcap->bumpdampen;
  pchr->weight = pcap->weight * pchr->fat * pchr->fat * pchr->fat;   // preserve density


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
  pchr->uoffvel = pcap->uoffvel;
  pchr->voffvel = pcap->voffvel;
  pchr->redshift = 0;
  pchr->grnshift = 0;
  pchr->blushift = 0;


  // Movement
  pchr->sneakspd = pcap->sneakspd;
  pchr->walkspd = pcap->walkspd;
  pchr->runspd = pcap->runspd;

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

  pchr->tlight.turn_lr.r = 0;
  pchr->tlight.turn_lr.g = 0;
  pchr->tlight.turn_lr.b = 0;

  pchr->vel.x = 0;
  pchr->vel.y = 0;
  pchr->vel.z = 0;
  pchr->mapturn_lr = 32768;  // These two mean on level surface
  pchr->mapturn_ud = 32768;
  pchr->scale = pchr->fat; // * MadList[pchr->model].scale * 4;

  // action stuff
  action_info_new( &(pchr->action) );
  anim_info_new( &(pchr->anim) );

  pchr->passage = 0;
  pchr->holdingweight = 0;
  pchr->onwhichplatform = MAXCHR;

  // Timers set to 0
  pchr->grogtime = 0.0f;
  pchr->dazetime = 0.0f;

  // Money is added later
  pchr->money = pcap->money;

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
    pchr->vrta_fp8[tnc].r = 0;
    pchr->vrta_fp8[tnc].g = 0;
    pchr->vrta_fp8[tnc].b = 0;
    tnc++;
  }

  // Particle attachments
  tnc = 0;
  while ( tnc < pcap->attachedprtamount )
  {
    spawn_one_particle( 1.0f, pchr->pos,
                        0, pchr->model, pcap->attachedprttype,
                        ichr, GRIP_LAST + tnc, pchr->team, ichr, tnc, MAXCHR );
    tnc++;
  }
  pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;


  // Experience
  if ( pcap->leveloverride != 0 )
  {
    while ( pchr->experiencelevel < pcap->leveloverride )
    {
      give_experience( ichr, 100, XP_DIRECT );
    }
  }
  else
  {
    pchr->experience = generate_unsigned( &pcap->experience );
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
void respawn_character( CHR_REF ichr )
{
  // ZZ> This function respawns a ichr
  Uint16 item, profile;
  CHR * pchr;
  CAP * pcap;
  AI_STATE * pstate;

  if ( !VALID_CHR( ichr )  ) return;

  pchr = ChrList + ichr;
  pstate = &(pchr->aistate);

  if( pchr->alive ) return;

  profile = pchr->model;
  pcap = CapList + profile;

  spawn_poof( ichr, profile );
  disaffirm_attached_particles( ichr );
  pchr->alive = btrue;
  pchr->boretime = DELAY_BORE;
  pchr->carefultime = DELAY_CAREFUL;
  pchr->life_fp8 = pchr->lifemax_fp8;
  pchr->mana_fp8 = pchr->manamax_fp8;
  pchr->pos.x = pchr->stt.x;
  pchr->pos.y = pchr->stt.y;
  pchr->pos.z = pchr->stt.z;
  pchr->vel.x = 0;
  pchr->vel.y = 0;
  pchr->vel.z = 0;
  pchr->team = pchr->baseteam;
  pchr->canbecrushed = bfalse;
  pchr->mapturn_lr = 32768;  // These two mean on level surface
  pchr->mapturn_ud = 32768;
  if ( !VALID_CHR( team_get_leader( pchr->team ) ) )  TeamList[pchr->team].leader = ichr;
  if ( !pchr->invictus )  TeamList[pchr->baseteam].morale++;

  action_info_new( &(pchr->action) );
  anim_info_new( &(pchr->anim) );

  pchr->isplatform = pcap->isplatform;
  pchr->flyheight  = pcap->flyheight;
  pchr->bumpdampen = pcap->bumpdampen;

  pchr->bmpdata_save.size    = pcap->bumpsize;
  pchr->bmpdata_save.sizebig = pcap->bumpsizebig;
  pchr->bmpdata_save.height  = pcap->bumpheight;

  pchr->bmpdata.size     = pcap->bumpsize * pchr->fat;
  pchr->bmpdata.sizebig  = pcap->bumpsizebig * pchr->fat;
  pchr->bmpdata.height   = pcap->bumpheight * pchr->fat;
  pchr->bumpstrength     = pcap->bumpstrength * FP8_TO_FLOAT( pcap->alpha_fp8 );

  // clear the alert and leave the state alone
  ai_state_renew(pstate, ichr);

  pchr->grogtime = 0.0f;
  pchr->dazetime = 0.0f;

  reaffirm_attached_particles( ichr );

  // Let worn items come back
  item  = chr_get_nextinpack( ichr );
  while ( VALID_CHR( item ) )
  {
    if ( ChrList[item].isequipped )
    {
      ChrList[item].isequipped = bfalse;
      ChrList[item].aistate.alert |= ALERT_ATLASTWAYPOINT;  // doubles as PutAway
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
  ChrList[ichr].aistate.type = MadList[new_profile].ai;
  ChrList[ichr].aistate.state = 0;
  ChrList[ichr].aistate.time = 0;
  ChrList[ichr].aistate.latch.x = 0;
  ChrList[ichr].aistate.latch.y = 0;
  ChrList[ichr].aistate.latch.b = 0;
  ChrList[ichr].aistate.turnmode = TURNMODE_VELOCITY;

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
    ChrList[ichr].vrta_fp8[tnc].r =
    ChrList[ichr].vrta_fp8[tnc].g =
    ChrList[ichr].vrta_fp8[tnc].b = 0;
    tnc++;
  }
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
      ChrList[cnt].aistate.time = 2;  // Don't let it think too much...
      ChrList[cnt].aistate.alert = ALERT_CLEANEDUP;
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
  ChrList[target].aistate.alert |= ALERT_SIGNALED;
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
    ChrList[cnt].aistate.alert |= ALERT_SIGNALED;
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

    ChrList[cnt].message        = order;
    ChrList[cnt].messagedata    = counter;
    ChrList[cnt].aistate.alert |= ALERT_SIGNALED;
    counter++;
  }
}

//--------------------------------------------------------------------------------------------
bool_t ai_state_advance_wp(AI_STATE * a, bool_t do_atlastwaypoint)
{
  if(NULL == a) return bfalse;

  a->alert |= ALERT_ATWAYPOINT;

  if( wp_list_advance( &(a->wp) ) )
  {
    // waypoint list is at or past its end
    if ( do_atlastwaypoint )
    {
      a->alert |= ALERT_ATLASTWAYPOINT;
    }
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
void set_alerts( CHR_REF ichr, float dUpdate )
{
  // ZZ> This function polls some alert conditions

  AI_STATE * pstate;
  CHR      * pchr;

  if( !VALID_CHR(ichr) ) return;

  pchr   = ChrList + ichr;
  pstate = &(pchr->aistate);

  pstate->time -= dUpdate;
  if ( pstate->time < 0 ) pstate->time = 0.0f;

  if ( ABS( pchr->pos.x - wp_list_x(&(pstate->wp)) ) < WAYTHRESH &&
       ABS( pchr->pos.y - wp_list_y(&(pstate->wp)) ) < WAYTHRESH )
  {
    ai_state_advance_wp(pstate, !CapList[pchr->model].isequipment);
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
        ChrList[character].aistate.morphed = btrue;
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


