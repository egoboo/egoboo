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
   along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "char.h"


//--------------------------------------------------------------------------------------------
void remove_enchant( Uint16 enchantindex )
{
  // ZZ> This function removes a specific enchantment and adds it to the unused list
  Uint16 character, overlay;
  Uint16 lastenchant, currentenchant;
  int add;


  if ( enchantindex < MAXENCHANT )
  {
    if ( encon[enchantindex] )
    {
      // Unsparkle the spellbook
      character = encspawner[enchantindex];
      if ( character < MAXCHR )
      {
        chrsparkle[character] = NOSPARKLE;
        // Make the spawner unable to undo the enchantment
        if ( chrundoenchant[character] == enchantindex )
        {
          chrundoenchant[character] = MAXENCHANT;
        }
      }


      // Play the end sound
      character = enctarget[enchantindex];
      // if(eveawveindex[enchantindex] > 0) play_sound(chroldx[character], chroldy[character], capwaveindex[chrmodel[encspawner[enchantindex]]][evewaveindex[enchantindex]);


      // Unset enchant values, doing morph last
      unset_enchant_value( enchantindex, SETDAMAGETYPE );
      unset_enchant_value( enchantindex, SETNUMBEROFJUMPS );
      unset_enchant_value( enchantindex, SETLIFEBARCOLOR );
      unset_enchant_value( enchantindex, SETMANABARCOLOR );
      unset_enchant_value( enchantindex, SETSLASHMODIFIER );
      unset_enchant_value( enchantindex, SETCRUSHMODIFIER );
      unset_enchant_value( enchantindex, SETPOKEMODIFIER );
      unset_enchant_value( enchantindex, SETHOLYMODIFIER );
      unset_enchant_value( enchantindex, SETEVILMODIFIER );
      unset_enchant_value( enchantindex, SETFIREMODIFIER );
      unset_enchant_value( enchantindex, SETICEMODIFIER );
      unset_enchant_value( enchantindex, SETZAPMODIFIER );
      unset_enchant_value( enchantindex, SETFLASHINGAND );
      unset_enchant_value( enchantindex, SETLIGHTBLEND );
      unset_enchant_value( enchantindex, SETALPHABLEND );
      unset_enchant_value( enchantindex, SETSHEEN );
      unset_enchant_value( enchantindex, SETFLYTOHEIGHT );
      unset_enchant_value( enchantindex, SETWALKONWATER );
      unset_enchant_value( enchantindex, SETCANSEEINVISIBLE );
      unset_enchant_value( enchantindex, SETMISSILETREATMENT );
      unset_enchant_value( enchantindex, SETCOSTFOREACHMISSILE );
      unset_enchant_value( enchantindex, SETCHANNEL );
      unset_enchant_value( enchantindex, SETMORPH );


      // Remove all of the cumulative values
      add = 0;
      while ( add < MAXEVEADDVALUE )
      {
        remove_enchant_value( enchantindex, add );
        add++;
      }


      // Unlink it
      if ( chrfirstenchant[character] == enchantindex )
      {
        // It was the first in the list
        chrfirstenchant[character] = encnextenchant[enchantindex];
      }
      else
      {
        // Search until we find it
        lastenchant = currentenchant = chrfirstenchant[character];
        while ( currentenchant != enchantindex )
        {
          lastenchant = currentenchant;
          currentenchant = encnextenchant[currentenchant];
        }
        // Relink the last enchantment
        encnextenchant[lastenchant] = encnextenchant[enchantindex];
      }



      // See if we spit out an end message
      if ( eveendmessage[enceve[enchantindex]] >= 0 )
      {
        display_message( madmsgstart[enceve[enchantindex]] + eveendmessage[enceve[enchantindex]], enctarget[enchantindex] );
      }
      // Check to see if we spawn a poof
      if ( evepoofonend[enceve[enchantindex]] )
      {
        spawn_poof( enctarget[enchantindex], enceve[enchantindex] );
      }
      // Check to see if the character dies
      if ( evekillonend[enceve[enchantindex]] )
      {
        if ( chrinvictus[character] )  teammorale[chrbaseteam[character]]++;
        chrinvictus[character] = bfalse;
        kill_character( character, MAXCHR );
      }
      // Kill overlay too...
      overlay = encoverlay[enchantindex];
      if ( overlay < MAXCHR )
      {
        if ( chrinvictus[overlay] )  teammorale[chrbaseteam[overlay]]++;
        chrinvictus[overlay] = bfalse;
        kill_character( overlay, MAXCHR );
      }

      // Remove see kurse enchant (BAD: no check if it is a natural abillity?)
      if ( eveseekurse[enceve[enchantindex]] && !capcanseekurse[chrmodel[character]] )
      {
        chrcanseekurse[character] = bfalse;
      }





      // Now get rid of it
      encon[enchantindex] = bfalse;
      freeenchant[numfreeenchant] = enchantindex;
      numfreeenchant++;


      // Now fix dem weapons
      reset_character_alpha( chrholdingwhich[character][0] );
      reset_character_alpha( chrholdingwhich[character][1] );
    }
  }
}

//--------------------------------------------------------------------------------------------
Uint16 enchant_value_filled( Uint16 enchantindex, Uint8 valueindex )
{
  // ZZ> This function returns MAXENCHANT if the enchantment's target has no conflicting
  //     set values in its other enchantments.  Otherwise it returns the enchantindex
  //     of the conflicting enchantment
  Uint16 character, currenchant;

  character = enctarget[enchantindex];
  currenchant = chrfirstenchant[character];
  while ( currenchant != MAXENCHANT )
  {
    if ( encsetyesno[currenchant][valueindex] )
    {
      return currenchant;
    }
    currenchant = encnextenchant[currenchant];
  }
  return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void set_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype )
{
  // ZZ> This function sets and saves one of the character's stats
  Uint16 conflict, character;


  encsetyesno[enchantindex][valueindex] = bfalse;
  if ( evesetyesno[enchanttype][valueindex] )
  {
    conflict = enchant_value_filled( enchantindex, valueindex );
    if ( conflict == MAXENCHANT || eveoverride[enchanttype] )
    {
      // Check for multiple enchantments
      if ( conflict < MAXENCHANT )
      {
        // Multiple enchantments aren't allowed for sets
        if ( everemoveoverridden[enchanttype] )
        {
          // Kill the old enchantment
          remove_enchant( conflict );
        }
        else
        {
          // Just unset the old enchantment's value
          unset_enchant_value( conflict, valueindex );
        }
      }
      // Set the value, and save the character's real stat
      character = enctarget[enchantindex];
      encsetyesno[enchantindex][valueindex] = btrue;
      switch ( valueindex )
      {
        case SETDAMAGETYPE:
          encsetsave[enchantindex][valueindex] = chrdamagetargettype[character];
          chrdamagetargettype[character] = evesetvalue[enchanttype][valueindex];
          break;
        case SETNUMBEROFJUMPS:
          encsetsave[enchantindex][valueindex] = chrjumpnumberreset[character];
          chrjumpnumberreset[character] = evesetvalue[enchanttype][valueindex];
          break;
        case SETLIFEBARCOLOR:
          encsetsave[enchantindex][valueindex] = chrlifecolor[character];
          chrlifecolor[character] = evesetvalue[enchanttype][valueindex];
          break;
        case SETMANABARCOLOR:
          encsetsave[enchantindex][valueindex] = chrmanacolor[character];
          chrmanacolor[character] = evesetvalue[enchanttype][valueindex];
          break;
        case SETSLASHMODIFIER:
          encsetsave[enchantindex][valueindex] = chrdamagemodifier[character][DAMAGESLASH];
          chrdamagemodifier[character][DAMAGESLASH] = evesetvalue[enchanttype][valueindex];
          break;
        case SETCRUSHMODIFIER:
          encsetsave[enchantindex][valueindex] = chrdamagemodifier[character][DAMAGECRUSH];
          chrdamagemodifier[character][DAMAGECRUSH] = evesetvalue[enchanttype][valueindex];
          break;
        case SETPOKEMODIFIER:
          encsetsave[enchantindex][valueindex] = chrdamagemodifier[character][DAMAGEPOKE];
          chrdamagemodifier[character][DAMAGEPOKE] = evesetvalue[enchanttype][valueindex];
          break;
        case SETHOLYMODIFIER:
          encsetsave[enchantindex][valueindex] = chrdamagemodifier[character][DAMAGEHOLY];
          chrdamagemodifier[character][DAMAGEHOLY] = evesetvalue[enchanttype][valueindex];
          break;
        case SETEVILMODIFIER:
          encsetsave[enchantindex][valueindex] = chrdamagemodifier[character][DAMAGEEVIL];
          chrdamagemodifier[character][DAMAGEEVIL] = evesetvalue[enchanttype][valueindex];
          break;
        case SETFIREMODIFIER:
          encsetsave[enchantindex][valueindex] = chrdamagemodifier[character][DAMAGEFIRE];
          chrdamagemodifier[character][DAMAGEFIRE] = evesetvalue[enchanttype][valueindex];
          break;
        case SETICEMODIFIER:
          encsetsave[enchantindex][valueindex] = chrdamagemodifier[character][DAMAGEICE];
          chrdamagemodifier[character][DAMAGEICE] = evesetvalue[enchanttype][valueindex];
          break;
        case SETZAPMODIFIER:
          encsetsave[enchantindex][valueindex] = chrdamagemodifier[character][DAMAGEZAP];
          chrdamagemodifier[character][DAMAGEZAP] = evesetvalue[enchanttype][valueindex];
          break;
        case SETFLASHINGAND:
          encsetsave[enchantindex][valueindex] = chrflashand[character];
          chrflashand[character] = evesetvalue[enchanttype][valueindex];
          break;
        case SETLIGHTBLEND:
          encsetsave[enchantindex][valueindex] = chrlight[character];
          chrlight[character] = evesetvalue[enchanttype][valueindex];
          break;
        case SETALPHABLEND:
          encsetsave[enchantindex][valueindex] = chralpha[character];
          chralpha[character] = evesetvalue[enchanttype][valueindex];
          break;
        case SETSHEEN:
          encsetsave[enchantindex][valueindex] = chrsheen[character];
          chrsheen[character] = evesetvalue[enchanttype][valueindex];
          break;
        case SETFLYTOHEIGHT:
          encsetsave[enchantindex][valueindex] = chrflyheight[character];
          if ( chrflyheight[character] == 0 && chrzpos[character] > -2 )
          {
            chrflyheight[character] = evesetvalue[enchanttype][valueindex];
          }
          break;
        case SETWALKONWATER:
          encsetsave[enchantindex][valueindex] = chrwaterwalk[character];
          if ( !chrwaterwalk[character] )
          {
            chrwaterwalk[character] = evesetvalue[enchanttype][valueindex];
          }
          break;
        case SETCANSEEINVISIBLE:
          encsetsave[enchantindex][valueindex] = chrcanseeinvisible[character];
          chrcanseeinvisible[character] = evesetvalue[enchanttype][valueindex];
          break;
        case SETMISSILETREATMENT:
          encsetsave[enchantindex][valueindex] = chrmissiletreatment[character];
          chrmissiletreatment[character] = evesetvalue[enchanttype][valueindex];
          break;
        case SETCOSTFOREACHMISSILE:
          encsetsave[enchantindex][valueindex] = chrmissilecost[character];
          chrmissilecost[character] = evesetvalue[enchanttype][valueindex];
          chrmissilehandler[character] = encowner[enchantindex];
          break;
        case SETMORPH:
          encsetsave[enchantindex][valueindex] = (0 != (chrtexture[character] - madskinstart[chrmodel[character]]));
          // Special handler for morph
          change_character( character, enchanttype, 0, LEAVEALL ); // LEAVEFIRST);
          chralert[character] |= ALERTIFCHANGED;
          break;
        case SETCHANNEL:
          encsetsave[enchantindex][valueindex] = chrcanchannel[character];
          chrcanchannel[character] = evesetvalue[enchanttype][valueindex];
          break;
      }
    }
  }
}


//--------------------------------------------------------------------------------------------
void add_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype )
{
  // ZZ> This function does cumulative modification to character stats
  int valuetoadd, newvalue;
  float fvaluetoadd, fnewvalue;
  Uint16 character;


  valuetoadd = 0;
  character = enctarget[enchantindex];
  switch ( valueindex )
  {
    case ADDJUMPPOWER:
      fnewvalue = chrjump[character];
      fvaluetoadd = eveaddvalue[enchanttype][valueindex] / 16.0f;
      fgetadd( 0, fnewvalue, 30.0f, &fvaluetoadd );
      valuetoadd = fvaluetoadd * 16.0f; // Get save value
      fvaluetoadd = valuetoadd / 16.0f;
      chrjump[character] += fvaluetoadd;
      break;
    case ADDBUMPDAMPEN:
      fnewvalue = chrbumpdampen[character];
      fvaluetoadd = eveaddvalue[enchanttype][valueindex] / 128.0f;
      fgetadd( 0, fnewvalue, 1.0f, &fvaluetoadd );
      valuetoadd = fvaluetoadd * 128.0f; // Get save value
      fvaluetoadd = valuetoadd / 128.0f;
      chrbumpdampen[character] += fvaluetoadd;
      break;
    case ADDBOUNCINESS:
      fnewvalue = chrdampen[character];
      fvaluetoadd = eveaddvalue[enchanttype][valueindex] / 128.0f;
      fgetadd( 0, fnewvalue, 0.95f, &fvaluetoadd );
      valuetoadd = fvaluetoadd * 128.0f; // Get save value
      fvaluetoadd = valuetoadd / 128.0f;
      chrdampen[character] += fvaluetoadd;
      break;
    case ADDDAMAGE:
      newvalue = chrdamageboost[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
      getadd( 0, newvalue, 4096, &valuetoadd );
      chrdamageboost[character] += valuetoadd;
      break;
    case ADDSIZE:
      fnewvalue = chrsizegoto[character];
      fvaluetoadd = eveaddvalue[enchanttype][valueindex] / 128.0f;
      fgetadd( 0.5f, fnewvalue, 2.0f, &fvaluetoadd );
      valuetoadd = fvaluetoadd * 128.0f; // Get save value
      fvaluetoadd = valuetoadd / 128.0f;
      chrsizegoto[character] += fvaluetoadd;
      chrsizegototime[character] = SIZETIME;
      break;
    case ADDACCEL:
      fnewvalue = chrmaxaccel[character];
      fvaluetoadd = eveaddvalue[enchanttype][valueindex] / 80.0f;
      fgetadd( 0, fnewvalue, 1.5f, &fvaluetoadd );
      valuetoadd = fvaluetoadd * 1000.0f; // Get save value
      fvaluetoadd = valuetoadd / 1000.0f;
      chrmaxaccel[character] += fvaluetoadd;
      break;
    case ADDRED:
      newvalue = chrredshift[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex];
      getadd( 0, newvalue, 6, &valuetoadd );
      chrredshift[character] += valuetoadd;
      break;
    case ADDGRN:
      newvalue = chrgrnshift[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex];
      getadd( 0, newvalue, 6, &valuetoadd );
      chrgrnshift[character] += valuetoadd;
      break;
    case ADDBLU:
      newvalue = chrblushift[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex];
      getadd( 0, newvalue, 6, &valuetoadd );
      chrblushift[character] += valuetoadd;
      break;
    case ADDDEFENSE:
      newvalue = chrdefense[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex];
      getadd( 55, newvalue, 255, &valuetoadd );  // Don't fix again!
      chrdefense[character] += valuetoadd;
      break;
    case ADDMANA:
      newvalue = chrmanamax[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
      getadd( 0, newvalue, PERFECTBIG, &valuetoadd );
      chrmanamax[character] += valuetoadd;
      chrmana[character] += valuetoadd;
      if ( chrmana[character] < 0 )  chrmana[character] = 0;
      break;
    case ADDLIFE:
      newvalue = chrlifemax[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
      getadd( LOWSTAT, newvalue, PERFECTBIG, &valuetoadd );
      chrlifemax[character] += valuetoadd;
      chrlife[character] += valuetoadd;
      if ( chrlife[character] < 1 )  chrlife[character] = 1;
      break;
    case ADDSTRENGTH:
      newvalue = chrstrength[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
      getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
      chrstrength[character] += valuetoadd;
      break;
    case ADDWISDOM:
      newvalue = chrwisdom[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
      getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
      chrwisdom[character] += valuetoadd;
      break;
    case ADDINTELLIGENCE:
      newvalue = chrintelligence[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
      getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
      chrintelligence[character] += valuetoadd;
      break;
    case ADDDEXTERITY:
      newvalue = chrdexterity[character];
      valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
      getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
      chrdexterity[character] += valuetoadd;
      break;
  }
  encaddsave[enchantindex][valueindex] = valuetoadd;  // Save the value for undo
}


//--------------------------------------------------------------------------------------------
Uint16 spawn_enchant( Uint16 owner, Uint16 target,
                      Uint16 spawner, Uint16 enchantindex, Uint16 modeloptional )
{
  // ZZ> This function enchants a target, returning the enchantment index or MAXENCHANT
  //     if failed
  Uint16 enchanttype, overlay;
  int add;


  if ( modeloptional < MAXMODEL )
  {
    // The enchantment type is given explicitly
    enchanttype = modeloptional;
  }
  else
  {
    // The enchantment type is given by the spawner
    enchanttype = chrmodel[spawner];
  }


  // Target and owner must both be alive and on and valid
  if ( target < MAXCHR )
  {
    if ( !chron[target] || !chralive[target] )
      return MAXENCHANT;
  }
  else
  {
    // Invalid target
    return MAXENCHANT;
  }
  if ( owner < MAXCHR )
  {
    if ( !chron[owner] || !chralive[owner] )
      return MAXENCHANT;
  }
  else
  {
    // Invalid target
    return MAXENCHANT;
  }


  if ( evevalid[enchanttype] )
  {
    if ( enchantindex == MAXENCHANT )
    {
      // Should it choose an inhand item?
      if ( everetarget[enchanttype] )
      {
        // Is at least one valid?
        if ( chrholdingwhich[target][0] == MAXCHR && chrholdingwhich[target][1] == MAXCHR )
        {
          // No weapons to pick
          return MAXENCHANT;
        }
        // Left, right, or both are valid
        if ( chrholdingwhich[target][0] == MAXCHR )
        {
          // Only right hand is valid
          target = chrholdingwhich[target][1];
        }
        else
        {
          // Pick left hand
          target = chrholdingwhich[target][0];
        }
      }


      // Make sure it's valid
      if ( evedontdamagetype[enchanttype] != DAMAGENULL )
      {
        if ( ( chrdamagemodifier[target][evedontdamagetype[enchanttype]]&7 ) >= 3 )  // Invert | Shift = 7
        {
          return MAXENCHANT;
        }
      }
      if ( eveonlydamagetype[enchanttype] != DAMAGENULL )
      {
        if ( chrdamagetargettype[target] != eveonlydamagetype[enchanttype] )
        {
          return MAXENCHANT;
        }
      }


      // Find one to use
      enchantindex = get_free_enchant();
    }
    else
    {
      numfreeenchant--;  // To keep it in order
    }
    if ( enchantindex < MAXENCHANT )
    {
      // Make a new one
      encon[enchantindex] = btrue;
      enctarget[enchantindex] = target;
      encowner[enchantindex] = owner;
      encspawner[enchantindex] = spawner;
      if ( spawner < MAXCHR )
      {
        chrundoenchant[spawner] = enchantindex;
      }
      enceve[enchantindex] = enchanttype;
      enctime[enchantindex] = evetime[enchanttype];
      encspawntime[enchantindex] = 1;
      encownermana[enchantindex] = eveownermana[enchanttype];
      encownerlife[enchantindex] = eveownerlife[enchanttype];
      enctargetmana[enchantindex] = evetargetmana[enchanttype];
      enctargetlife[enchantindex] = evetargetlife[enchanttype];



      // Add it as first in the list
      encnextenchant[enchantindex] = chrfirstenchant[target];
      chrfirstenchant[target] = enchantindex;


      // Now set all of the specific values, morph first
      set_enchant_value( enchantindex, SETMORPH, enchanttype );
      set_enchant_value( enchantindex, SETDAMAGETYPE, enchanttype );
      set_enchant_value( enchantindex, SETNUMBEROFJUMPS, enchanttype );
      set_enchant_value( enchantindex, SETLIFEBARCOLOR, enchanttype );
      set_enchant_value( enchantindex, SETMANABARCOLOR, enchanttype );
      set_enchant_value( enchantindex, SETSLASHMODIFIER, enchanttype );
      set_enchant_value( enchantindex, SETCRUSHMODIFIER, enchanttype );
      set_enchant_value( enchantindex, SETPOKEMODIFIER, enchanttype );
      set_enchant_value( enchantindex, SETHOLYMODIFIER, enchanttype );
      set_enchant_value( enchantindex, SETEVILMODIFIER, enchanttype );
      set_enchant_value( enchantindex, SETFIREMODIFIER, enchanttype );
      set_enchant_value( enchantindex, SETICEMODIFIER, enchanttype );
      set_enchant_value( enchantindex, SETZAPMODIFIER, enchanttype );
      set_enchant_value( enchantindex, SETFLASHINGAND, enchanttype );
      set_enchant_value( enchantindex, SETLIGHTBLEND, enchanttype );
      set_enchant_value( enchantindex, SETALPHABLEND, enchanttype );
      set_enchant_value( enchantindex, SETSHEEN, enchanttype );
      set_enchant_value( enchantindex, SETFLYTOHEIGHT, enchanttype );
      set_enchant_value( enchantindex, SETWALKONWATER, enchanttype );
      set_enchant_value( enchantindex, SETCANSEEINVISIBLE, enchanttype );
      set_enchant_value( enchantindex, SETMISSILETREATMENT, enchanttype );
      set_enchant_value( enchantindex, SETCOSTFOREACHMISSILE, enchanttype );
      set_enchant_value( enchantindex, SETCHANNEL, enchanttype );


      // Now do all of the stat adds
      add = 0;
      while ( add < MAXEVEADDVALUE )
      {
        add_enchant_value( enchantindex, add, enchanttype );
        add++;
      }


      // Create an overlay character?
      encoverlay[enchantindex] = MAXCHR;
      if ( eveoverlay[enchanttype] )
      {
        overlay = spawn_one_character( chrxpos[target], chrypos[target], chrzpos[target],
                                       enchanttype, chrteam[target], 0, chrturnleftright[target],
                                       NULL, MAXCHR );
        if ( overlay < MAXCHR )
        {
          encoverlay[enchantindex] = overlay;  // Kill this character on end...
          chraitarget[overlay] = target;
          chraistate[overlay] = eveoverlay[enchanttype];
          chroverlay[overlay] = btrue;


          // Start out with ActionMJ...  Object activated
          if ( madactionvalid[chrmodel[overlay]][ACTIONMJ] )
          {
            chraction[overlay] = ACTIONMJ;
            chrlip[overlay] = 0;
            chrframe[overlay] = madactionstart[chrmodel[overlay]][ACTIONMJ];
            chrlastframe[overlay] = chrframe[overlay];
            chractionready[overlay] = bfalse;
          }
          chrlight[overlay] = 254;  // Assume it's transparent...
        }
      }
    }
    return enchantindex;
  }
  return MAXENCHANT;
}


//--------------------------------------------------------------------------------------------
void do_enchant_spawn()
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
        encspawntime[cnt]--;
        if ( encspawntime[cnt] == 0 )
        {
          character = enctarget[cnt];
          encspawntime[cnt] = evecontspawntime[eve];
          facing = chrturnleftright[character];
          tnc = 0;
          while ( tnc < evecontspawnamount[eve] )
          {
            particle = spawn_one_particle( chrxpos[character], chrypos[character], chrzpos[character],
                                           facing, eve, evecontspawnpip[eve],
                                           MAXCHR, SPAWNLAST, chrteam[encowner[cnt]], encowner[cnt], tnc, MAXCHR );
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
void load_one_enchant_type( char* szLoadName, Uint16 profile )
{
  // ZZ> This function loads the enchantment associated with an object
  FILE* fileread;
  char cTmp;
  int iTmp, tTmp, idsz, test;
  float fTmp;
  int num;

  globalname = szLoadName;
  evevalid[profile] = bfalse;
  fileread = fopen( szLoadName, "r" );
  if ( fileread )
  {
    evevalid[profile] = btrue;


    // btrue/bfalse values
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    everetarget[profile] = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  everetarget[profile] = btrue;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    eveoverride[profile] = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  eveoverride[profile] = btrue;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    everemoveoverridden[profile] = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  everemoveoverridden[profile] = btrue;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evekillonend[profile] = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  evekillonend[profile] = btrue;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evepoofonend[profile] = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  evepoofonend[profile] = btrue;


    // More stuff
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  evetime[profile] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  eveendmessage[profile] = iTmp;


    // Drain stuff
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  eveownermana[profile] = fTmp * 256;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  evetargetmana[profile] = fTmp * 256;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    eveendifcantpay[profile] = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  eveendifcantpay[profile] = btrue;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  eveownerlife[profile] = fTmp * 256;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  evetargetlife[profile] = fTmp * 256;


    // Specifics
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evedontdamagetype[profile] = DAMAGENULL;
    if ( cTmp == 'S' || cTmp == 's' )  evedontdamagetype[profile] = DAMAGESLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  evedontdamagetype[profile] = DAMAGECRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  evedontdamagetype[profile] = DAMAGEPOKE;
    if ( cTmp == 'H' || cTmp == 'h' )  evedontdamagetype[profile] = DAMAGEHOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  evedontdamagetype[profile] = DAMAGEEVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  evedontdamagetype[profile] = DAMAGEFIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  evedontdamagetype[profile] = DAMAGEICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  evedontdamagetype[profile] = DAMAGEZAP;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    eveonlydamagetype[profile] = DAMAGENULL;
    if ( cTmp == 'S' || cTmp == 's' )  eveonlydamagetype[profile] = DAMAGESLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  eveonlydamagetype[profile] = DAMAGECRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  eveonlydamagetype[profile] = DAMAGEPOKE;
    if ( cTmp == 'H' || cTmp == 'h' )  eveonlydamagetype[profile] = DAMAGEHOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  eveonlydamagetype[profile] = DAMAGEEVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  eveonlydamagetype[profile] = DAMAGEFIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  eveonlydamagetype[profile] = DAMAGEICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  eveonlydamagetype[profile] = DAMAGEZAP;
    goto_colon( fileread );  everemovedbyidsz[profile] = get_idsz( fileread );


    // Now the set values
    num = 0;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );
    evesetvalue[profile][num] = DAMAGESLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  evesetvalue[profile][num] = DAMAGECRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  evesetvalue[profile][num] = DAMAGEPOKE;
    if ( cTmp == 'H' || cTmp == 'h' )  evesetvalue[profile][num] = DAMAGEHOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  evesetvalue[profile][num] = DAMAGEEVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  evesetvalue[profile][num] = DAMAGEFIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  evesetvalue[profile][num] = DAMAGEICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  evesetvalue[profile][num] = DAMAGEZAP;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );  iTmp = 0;
    if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;
    if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;
    fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );  iTmp = 0;
    if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;
    if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;
    fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );  iTmp = 0;
    if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;
    if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;
    fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );  iTmp = 0;
    if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;
    if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;
    fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );  iTmp = 0;
    if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;
    if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;
    fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );  iTmp = 0;
    if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;
    if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;
    fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );  iTmp = 0;
    if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;
    if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;
    fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );  iTmp = 0;
    if ( cTmp == 'T' ) iTmp = DAMAGEINVERT;
    if ( cTmp == 'C' ) iTmp = DAMAGECHARGE;
    fscanf( fileread, "%d", &tTmp );  evesetvalue[profile][num] = iTmp | tTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%d", &iTmp );  evesetvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );
    evesetvalue[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );
    evesetvalue[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    cTmp = get_first_letter( fileread );
    evesetvalue[profile][num] = MISNORMAL;
    if ( cTmp == 'R' || cTmp == 'r' )  evesetvalue[profile][num] = MISREFLECT;
    if ( cTmp == 'D' || cTmp == 'd' )  evesetvalue[profile][num] = MISDEFLECT;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    fscanf( fileread, "%f", &fTmp );  fTmp = fTmp * 16;
    evesetvalue[profile][num] = fTmp;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    evesetvalue[profile][num] = btrue;
    num++;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    evesetyesno[profile][num] = ( cTmp == 'T' || cTmp == 't' );
    evesetvalue[profile][num] = btrue;
    num++;


    // Now read in the add values
    num = 0;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 16;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 127;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 127;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 127;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    eveaddvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    eveaddvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    eveaddvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    eveaddvalue[profile][num] = iTmp;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Defense is backwards
    eveaddvalue[profile][num] = -iTmp;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 4;
    num++;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );
    eveaddvalue[profile][num] = fTmp * 4;
    num++;

    // Clear expansions...
    evecontspawntime[profile] = 0;
    evecontspawnamount[profile] = 0;
    evecontspawnfacingadd[profile] = 0;
    evecontspawnpip[profile] = 0;
    evewaveindex[profile] = -1;
    evefrequency[profile] = 11025;
    evestayifnoowner[profile] = 0;
    eveoverlay[profile] = 0;
    // Read expansions
    while ( goto_colon_yesno( fileread ) )
    {
      idsz = get_idsz( fileread );
      fscanf( fileread, "%c%d", &cTmp, &iTmp );
      test = Make_IDSZ( 'A', 'M', 'O', 'U' );  // [AMOU]
      if ( idsz == test )  evecontspawnamount[profile] = iTmp;
      test = Make_IDSZ( 'T', 'Y', 'P', 'E' );  // [TYPE]
      if ( idsz == test )  evecontspawnpip[profile] = iTmp;
      test = Make_IDSZ( 'T', 'I', 'M', 'E' );  // [TIME]
      if ( idsz == test )  evecontspawntime[profile] = iTmp;
      test = Make_IDSZ( 'F', 'A', 'C', 'E' );  // [FACE]
      if ( idsz == test )  evecontspawnfacingadd[profile] = iTmp;
      test = Make_IDSZ( 'S', 'E', 'N', 'D' );  // [SEND]
      if ( idsz == test )
      {
        // TODO
        // This is wrong, it gets stored or loaded incorrectly (Loaded in game.c)
        if ( iTmp >= 0 && iTmp < MAXWAVE ) evewaveindex[profile] = iTmp;
      }
      test = Make_IDSZ( 'S', 'F', 'R', 'Q' );  // [SFRQ]
      if ( idsz == test )  evefrequency[profile] = iTmp;  // OUTDATED??
      test = Make_IDSZ( 'S', 'T', 'A', 'Y' );  // [STAY]
      if ( idsz == test )  evestayifnoowner[profile] = (0!=iTmp);
      test = Make_IDSZ( 'O', 'V', 'E', 'R' );  // [OVER]
      if ( idsz == test )  eveoverlay[profile] = iTmp;
      test = Make_IDSZ( 'C', 'K', 'U', 'R' );  // [CKUR]
      if ( idsz == test )  eveseekurse[profile] = iTmp;
    }


    // All done ( finally )
    fclose( fileread );
  }
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
  Uint16 character;

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
        chrdamagemodifier[character][DAMAGESLASH] = encsetsave[enchantindex][valueindex];
        break;
      case SETCRUSHMODIFIER:
        chrdamagemodifier[character][DAMAGECRUSH] = encsetsave[enchantindex][valueindex];
        break;
      case SETPOKEMODIFIER:
        chrdamagemodifier[character][DAMAGEPOKE] = encsetsave[enchantindex][valueindex];
        break;
      case SETHOLYMODIFIER:
        chrdamagemodifier[character][DAMAGEHOLY] = encsetsave[enchantindex][valueindex];
        break;
      case SETEVILMODIFIER:
        chrdamagemodifier[character][DAMAGEEVIL] = encsetsave[enchantindex][valueindex];
        break;
      case SETFIREMODIFIER:
        chrdamagemodifier[character][DAMAGEFIRE] = encsetsave[enchantindex][valueindex];
        break;
      case SETICEMODIFIER:
        chrdamagemodifier[character][DAMAGEICE] = encsetsave[enchantindex][valueindex];
        break;
      case SETZAPMODIFIER:
        chrdamagemodifier[character][DAMAGEZAP] = encsetsave[enchantindex][valueindex];
        break;
      case SETFLASHINGAND:
        chrflashand[character] = encsetsave[enchantindex][valueindex];
        break;
      case SETLIGHTBLEND:
        chrlight[character] = encsetsave[enchantindex][valueindex];
        break;
      case SETALPHABLEND:
        chralpha[character] = encsetsave[enchantindex][valueindex];
        break;
      case SETSHEEN:
        chrsheen[character] = encsetsave[enchantindex][valueindex];
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
        change_character( character, chrbasemodel[character], encsetsave[enchantindex][valueindex], LEAVEALL );
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

  Uint16 character = enctarget[enchantindex];
  switch ( valueindex )
  {
    case ADDJUMPPOWER:
      fvaluetoadd = encaddsave[enchantindex][valueindex] / 16.0f;
      chrjump[character] -= fvaluetoadd;
      break;
    case ADDBUMPDAMPEN:
      fvaluetoadd = encaddsave[enchantindex][valueindex] / 128.0f;
      chrbumpdampen[character] -= fvaluetoadd;
      break;
    case ADDBOUNCINESS:
      fvaluetoadd = encaddsave[enchantindex][valueindex] / 128.0f;
      chrdampen[character] -= fvaluetoadd;
      break;
    case ADDDAMAGE:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrdamageboost[character] -= valuetoadd;
      break;
    case ADDSIZE:
      fvaluetoadd = encaddsave[enchantindex][valueindex] / 128.0f;
      chrsizegoto[character] -= fvaluetoadd;
      chrsizegototime[character] = SIZETIME;
      break;
    case ADDACCEL:
      fvaluetoadd = encaddsave[enchantindex][valueindex] / 1000.0f;
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
      chrdefense[character] -= valuetoadd;
      break;
    case ADDMANA:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrmanamax[character] -= valuetoadd;
      chrmana[character] -= valuetoadd;
      if ( chrmana[character] < 0 ) chrmana[character] = 0;
      break;
    case ADDLIFE:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrlifemax[character] -= valuetoadd;
      chrlife[character] -= valuetoadd;
      if ( chrlife[character] < 1 ) chrlife[character] = 1;
      break;
    case ADDSTRENGTH:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrstrength[character] -= valuetoadd;
      break;
    case ADDWISDOM:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrwisdom[character] -= valuetoadd;
      break;
    case ADDINTELLIGENCE:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrintelligence[character] -= valuetoadd;
      break;
    case ADDDEXTERITY:
      valuetoadd = encaddsave[enchantindex][valueindex];
      chrdexterity[character] -= valuetoadd;
      break;
  }
}
