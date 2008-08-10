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

//--------------------------------------------------------------------------------------------
void do_enchant_spawn()
{
    // ZZ> This function lets enchantments spawn particles
    int cnt, tnc;
    unsigned short facing, particle, eve, character;


    cnt = 0;
    while(cnt < MAXENCHANT)
    {
        if(encon[cnt])
        {
            eve = enceve[cnt];
            if(evecontspawnamount[eve]>0)
            {
                encspawntime[cnt]--;
                if(encspawntime[cnt] == 0)
                {
                    character = enctarget[cnt];
                    encspawntime[cnt] = evecontspawntime[eve];
                    facing = chrturnleftright[character];
                    tnc = 0;
                    while(tnc < evecontspawnamount[eve])
                    {
                        particle = spawn_one_particle(chrxpos[character], chrypos[character], chrzpos[character],
                                           facing, eve, evecontspawnpip[eve],
                                           MAXCHR, SPAWNLAST, chrteam[encowner[cnt]], encowner[cnt], tnc, MAXCHR);
                        facing+=evecontspawnfacingadd[eve];
                        tnc++;
                    }
                }
            }
        }
        cnt++;
    }
}


//--------------------------------------------------------------------------------------------
void disenchant_character(unsigned short cnt)
{
    // ZZ> This function removes all enchantments from a character
    while(chrfirstenchant[cnt] != MAXENCHANT)
    {
        remove_enchant(chrfirstenchant[cnt]);
    }
}

//char.c
//--------------------------------------------------------------------------------------------
void damage_character(unsigned short character, unsigned short direction,
    int damagebase, int damagerand, unsigned char damagetype, unsigned char team,
    unsigned short attacker, unsigned short effects)
{
    // ZZ> This function calculates and applies damage to a character.  It also
    //     sets alerts and begins actions.  Blocking and frame invincibility
    //     are done here too.  Direction is 0 if the attack is coming head on,
    //     16384 if from the right, 32768 if from the back, 49152 if from the
    //     left.
    int tnc;
    unsigned short action;
    int damage, basedamage;
    unsigned short experience, model, left, right;


    if(chralive[character] && damagebase>=0 && damagerand>=1)
    {
        // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
        // This can also be used to lessen effectiveness of healing
        damage = damagebase+(rand()%damagerand);
        basedamage = damage;
        damage = damage>>(chrdamagemodifier[character][damagetype]&DAMAGESHIFT);


        // Allow charging (Invert damage to mana)
        if(chrdamagemodifier[character][damagetype]&DAMAGECHARGE)
        {
            chrmana[character] += damage;
            if(chrmana[character] > chrmanamax[character])
            {
                chrmana[character] = chrmanamax[character];
            }
            return;
        }


        // Invert damage to heal
        if(chrdamagemodifier[character][damagetype]&DAMAGEINVERT)
            damage=-damage;


        // Remember the damage type
        chrdamagetypelast[character] = damagetype;
        chrdirectionlast[character] = direction;


        // Do it already
        if(damage > 0)
        {
            // Only damage if not invincible
            if(chrdamagetime[character]==0 && chrinvictus[character]==bfalse)
            {
                model = chrmodel[character];
                if((effects&DAMFXBLOC)==bfalse)
                {
                    // Only damage if hitting from proper direction
                    if(madframefx[chrframe[character]]&MADFXINVICTUS)
                    {
                        // I Frame...
                        direction -= capiframefacing[model];
                        left = (~capiframeangle[model]);
                        right = capiframeangle[model];
                        // Check for shield
                        if(chraction[character] >= ACTIONPA && chraction[character] <= ACTIONPD)
                        {
                            // Using a shield?
                            if(chraction[character] < ACTIONPC)
                            {
                                // Check left hand
                                if(chrholdingwhich[character][0] != MAXCHR)
                                {
                                    left = (~capiframeangle[chrmodel[chrholdingwhich[character][0]]]);
                                    right = capiframeangle[chrmodel[chrholdingwhich[character][0]]];
                                }
                            }
                            else
                            {
                                // Check right hand
                                if(chrholdingwhich[character][1] != MAXCHR)
                                {
                                    left = (~capiframeangle[chrmodel[chrholdingwhich[character][1]]]);
                                    right = capiframeangle[chrmodel[chrholdingwhich[character][1]]];
                                }
                            }
                        }
                    }
                    else
                    {
                        // N Frame
                        direction -= capnframefacing[model];
                        left = (~capnframeangle[model]);
                        right = capnframeangle[model];
                    }
                    // Check that direction
                    if(direction > left || direction < right)
                    {
                        damage = 0;
                    }
                }



                if(damage!=0)
                {
                    if(effects&DAMFXARMO)
                    {
                        chrlife[character]-=damage;
                    }
                    else
                    {
                        chrlife[character]-=((damage*chrdefense[character])>>8);
                    }


                    if(basedamage > MINDAMAGE)
                    {
                        // Call for help if below 1/2 life
                        if(chrlife[character] < (chrlifemax[character]>>1))
                            call_for_help(character);
                        // Spawn blood particles
                        if(capbloodvalid[model] && (damagetype < DAMAGEHOLY || capbloodvalid[model]==ULTRABLOODY))
                        {
                            spawn_one_particle(chrxpos[character], chrypos[character], chrzpos[character],
                                   chrturnleftright[character]+direction, chrmodel[character], capbloodprttype[model],
                                   MAXCHR, SPAWNLAST, chrteam[character], character, 0, MAXCHR);
                        }
                        // Set attack alert if it wasn't an accident
                        if(team == DAMAGETEAM)
                        {
                            chrattacklast[character] = MAXCHR;
                        }
                        else
                        {
                            // Don't alert the character too much if under constant fire
                            if(chrcarefultime[character] == 0)
                            {
                                // Don't let characters chase themselves...  That would be silly
                                if(attacker != character)
                                {
                                    chralert[character] = chralert[character]|ALERTIFATTACKED;
                                    chrattacklast[character] = attacker;
                                    chrcarefultime[character] = CAREFULTIME;
                                }
                            }
                        }
                    }


                    // Taking damage action
                    action = ACTIONHA;
                    if(chrlife[character] < 0)
                    {
                        // Character has died
                        chralive[character] = bfalse;
                        disenchant_character(character);
                        chrwaskilled[character] = btrue;
                        chrkeepaction[character] = btrue;
                        chrlife[character] = -1;
                        chrplatform[character] = btrue;
                        chrbumpdampen[character] = chrbumpdampen[character]/2.0;
                        action = ACTIONKA;
                        // Give kill experience
                        experience = capexperienceworth[model]+(chrexperience[character]*capexperienceexchange[model]);
                        if(attacker < MAXCHR)
                        {
                            // Set target
                            chraitarget[character] = attacker;
                            if(team == DAMAGETEAM)  chraitarget[character] = character;
                            if(team == NULLTEAM)  chraitarget[character] = character;
                            // Award direct kill experience
                            if(teamhatesteam[chrteam[attacker]][chrteam[character]])
                            {
                                give_experience(attacker, experience, XPKILLENEMY);
                            }
                            // Check for hated
                            if(capidsz[chrmodel[attacker]][IDSZHATE]==capidsz[model][IDSZPARENT] ||
                               capidsz[chrmodel[attacker]][IDSZHATE]==capidsz[model][IDSZTYPE])
                            {
                                give_experience(attacker, experience, XPKILLHATED);
                            }
                        }
                        // Clear all shop passages that it owned...
                        tnc = 0;
                        while(tnc < numshoppassage)
                        {
                            if(shopowner[tnc] == character)
                            {
                                shopowner[tnc] = NOOWNER;
                            }
                            tnc++;
                        }
                        // Let the other characters know it died
                        tnc = 0;
                        while(tnc < MAXCHR)
                        {
                            if(chron[tnc] && chralive[tnc])
                            {
                                if(chraitarget[tnc] == character)
                                {
                                    chralert[tnc] = chralert[tnc]|ALERTIFTARGETKILLED;
                                }
                                if((teamhatesteam[chrteam[tnc]][team]==bfalse)&&(teamhatesteam[chrteam[tnc]][chrteam[character]]==btrue))
                                {
                                    // All allies get team experience, but only if they also hate the dead guy's team
                                    give_experience(tnc, experience, XPTEAMKILL);
                                }
                            }
                            tnc++;
                        }
                        // Check if it was a leader
                        if(teamleader[chrteam[character]]==character)
                        {
                            // It was a leader, so set more alerts
                            tnc = 0;
                            while(tnc < MAXCHR)
                            {
                                if(chron[tnc]&&chrteam[tnc]==chrteam[character])
                                {
                                    // All folks on the leaders team get the alert
                                    chralert[tnc] = chralert[tnc]|ALERTIFLEADERKILLED;
                                }
                                tnc++;
                            }
                            // The team now has no leader
                            teamleader[chrteam[character]] = NOLEADER;
                        }
                        detach_character_from_mount(character, btrue, bfalse);
                        action+=(rand()&3);
                        play_action(character, action, bfalse);
                        // Turn off all sounds if it's a player
                        if(chrisplayer[character])
                        {
                            tnc = 0;
                            while(tnc < MAXWAVE)
                            {
                                //stop_sound(capwaveindex[chrmodel[character]][tnc]);
                                //TODO Zefz: Do we need this? This makes all sounds a character makes stop when it dies...
                                tnc++;
                            }
                        }
                        // Afford it one last thought if it's an AI
                        teammorale[chrbaseteam[character]]--;
                        chrteam[character] = chrbaseteam[character];
                        chralert[character] = ALERTIFKILLED;
                        chrsparkle[character] = NOSPARKLE;
                        chraitime[character] = 1;  // No timeout...
                        let_character_think(character);
                    }
                    else
                    {
                        if(basedamage > MINDAMAGE)
                        {
                            action+=(rand()&3);
                            play_action(character, action, bfalse);
                            // Make the character invincible for a limited time only
                            if(!(effects & DAMFXTIME))
                                chrdamagetime[character] = DAMAGETIME;
                        }
                    }
                }
                else
                {
                    // Spawn a defend particle
                    spawn_one_particle(chrxpos[character], chrypos[character], chrzpos[character], chrturnleftright[character], MAXMODEL, DEFEND, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR);
                    chrdamagetime[character] = DEFENDTIME;
                    chralert[character] = chralert[character]|ALERTIFBLOCKED;
                }
            }
        }
        else if(damage < 0)
        {
            chrlife[character]-=damage;
            if(chrlife[character] > chrlifemax[character])  chrlife[character] = chrlifemax[character];

            // Isssue an alert
            chralert[character] = chralert[character]|ALERTIFHEALED;
            chrattacklast[character] = attacker;
            if(team != DAMAGETEAM)
            {
                chrattacklast[character] = MAXCHR;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void kill_character(unsigned short character, unsigned short killer)
{
    // ZZ> This function kills a character...  MAXCHR killer for accidental death
    unsigned char modifier;

    if(chralive[character])
    {
        chrdamagetime[character] = 0;
        chrlife[character] = 1;
        modifier = chrdamagemodifier[character][DAMAGECRUSH];
        chrdamagemodifier[character][DAMAGECRUSH] = 1;
        if(killer != MAXCHR)
        {
            damage_character(character, 0, 512, 1, DAMAGECRUSH, chrteam[killer], killer, DAMFXARMO|DAMFXBLOC);
        }
        else
        {
            damage_character(character, 0, 512, 1, DAMAGECRUSH, DAMAGETEAM, chrbumplast[character], DAMFXARMO|DAMFXBLOC);
        }
        chrdamagemodifier[character][DAMAGECRUSH] = modifier;
    }
}

//--------------------------------------------------------------------------------------------
void spawn_poof(unsigned short character, unsigned short profile)
{
    // ZZ> This function spawns a character poof
    unsigned short sTmp;
    unsigned short origin;
    int iTmp;


    sTmp = chrturnleftright[character];
    iTmp = 0;
    origin = chraiowner[character];
    while(iTmp < capgopoofprtamount[profile])
    {
        spawn_one_particle(chroldx[character], chroldy[character], chroldz[character],
            sTmp, profile, capgopoofprttype[profile],
            MAXCHR, SPAWNLAST, chrteam[character], origin, iTmp, MAXCHR);
        sTmp+=capgopoofprtfacingadd[profile];
        iTmp++;
    }
}

//--------------------------------------------------------------------------------------------
void naming_names(int profile)
{
    // ZZ> This function generates a random name
    int read, write, section, mychop;
    char cTmp;

    if(capsectionsize[profile][0] == 0)
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
        while(section < MAXSECTION)
        {
            if(capsectionsize[profile][section] != 0)
            {
                mychop = capsectionstart[profile][section] + (rand()%capsectionsize[profile][section]);
                read = chopstart[mychop];
                cTmp = chopdata[read];
                while(cTmp != 0 && write < MAXCAPNAMESIZE-1)
                {
                    namingnames[write]=cTmp;
                    write++;
                    read++;
                    cTmp = chopdata[read];
                }
            }
            section++;
        }        
        if(write >= MAXCAPNAMESIZE) write = MAXCAPNAMESIZE-1;
        namingnames[write] = 0;
    }
}

//--------------------------------------------------------------------------------------------
void read_naming(int profile, char *szLoadname)
{
    // ZZ> This function reads a naming file
    FILE *fileread;
    int section, chopinsection, cnt;
    char mychop[32], cTmp;

    fileread = fopen(szLoadname, "r");
    if(fileread)
    {
        section = 0;
        chopinsection = 0;
        while(goto_colon_yesno(fileread) && section < MAXSECTION)
        {
            fscanf(fileread, "%s", mychop);
            if(mychop[0] != 'S' || mychop[1] != 'T' || mychop[2] != 'O' || mychop[3] != 'P')
            {
                if(chopwrite >= CHOPDATACHUNK)  chopwrite = CHOPDATACHUNK-1;
                chopstart[numchop] = chopwrite;
                cnt = 0;
                cTmp = mychop[0];
                while(cTmp != 0 && cnt < 31 && chopwrite < CHOPDATACHUNK)
                {
                    if(cTmp == '_') cTmp = ' ';
                    chopdata[chopwrite]=cTmp;
                    cnt++;
                    chopwrite++;
                    cTmp = mychop[cnt];
                }
                if(chopwrite >= CHOPDATACHUNK)  chopwrite = CHOPDATACHUNK-1;
                chopdata[chopwrite]=0;  chopwrite++;
                chopinsection++;
                numchop++;
            }
            else
            {
                capsectionsize[profile][section] = chopinsection;
                capsectionstart[profile][section] = numchop-chopinsection;
                section++;
                chopinsection = 0;
            }
        }
        fclose(fileread);
    }
}

//--------------------------------------------------------------------------------------------
void prime_names(void)
{
    // ZZ> This function prepares the name chopper for use
    int cnt, tnc;

    numchop = 0;
    chopwrite = 0;
    cnt = 0;
    while(cnt < MAXMODEL)
    {
        tnc = 0;
        while(tnc < MAXSECTION)
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
    unsigned char twist;

    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chrstickybutt[cnt] && chron[cnt] && chronwhichfan[cnt] != OFFEDGE)
        {
            twist = meshtwist[chronwhichfan[cnt]];
            chrturnmaplr[cnt] = maplrtwist[twist];
            chrturnmapud[cnt] = mapudtwist[twist];
        }
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
int spawn_one_character(float x, float y, float z, int profile, unsigned char team,
  unsigned char skin, unsigned short facing, char *name, int override)
{
    // ZZ> This function spawns a character and returns the character's index number
    //     if it worked, MAXCHR otherwise
    int cnt, tnc, ix, iy;


    // Make sure the team is valid
    if(team > MAXTEAM-1)
        team = MAXTEAM-1;


    // Get a new character
    cnt = MAXCHR;
    if(madused[profile])
    {
        if(override < MAXCHR)
        {
            cnt = get_free_character();
            if(cnt != override)
            {
                // Picked the wrong one, so put this one back and find the right one
                tnc = 0;
                while(tnc < MAXCHR)
                {
                    if(freechrlist[tnc] == override)
                    {
                        freechrlist[tnc] = cnt;
                        tnc = MAXCHR;
                    }
                    tnc++;
                }
                cnt = override;
            }
        }
        else
        {
            cnt = get_free_character();
        }
        if(cnt != MAXCHR)
        {
            // IMPORTANT!!!
            chrindolist[cnt] = bfalse;
            chrisequipped[cnt] = bfalse;
            chrsparkle[cnt] = NOSPARKLE;
            chroverlay[cnt] = bfalse;
            chrmissilehandler[cnt] = cnt;

            // SetXY stuff...  Just in case
            tnc = 0;
            while(tnc < MAXSTOR)
            {
                chraix[cnt][tnc] = 0;
                chraiy[cnt][tnc] = 0;
                tnc++;
            }

            // Set up model stuff
            chron[cnt] = btrue;
            chrreloadtime[cnt] = 0;
            chrinwhichhand[cnt] = GRIPLEFT;
            chrwaskilled[cnt] = bfalse;
            chrinpack[cnt] = bfalse;
            chrnextinpack[cnt] = MAXCHR;
            chrnuminpack[cnt] = 0;
            chrmodel[cnt] = profile;
            chrbasemodel[cnt] = profile;
            chrstoppedby[cnt] = capstoppedby[profile];
            chrlifeheal[cnt] = caplifeheal[profile];
            chrmanacost[cnt] = capmanacost[profile];
            chrinwater[cnt] = bfalse;
            chrnameknown[cnt] = capnameknown[profile];
            chrammoknown[cnt] = capnameknown[profile];
            chrhitready[cnt] = btrue;
            chrboretime[cnt] = BORETIME;
            chrcarefultime[cnt] = CAREFULTIME;
            chrcanbecrushed[cnt] = bfalse;
            chrdamageboost[cnt] = 0;
            chricon[cnt] = capicon[profile];


            // Enchant stuff
            chrfirstenchant[cnt] = MAXENCHANT;
            chrundoenchant[cnt] = MAXENCHANT;
            chrcanseeinvisible[cnt] = capcanseeinvisible[profile];
            chrcanseekurse[cnt] = capcanseekurse[profile];
            chrcanchannel[cnt] = bfalse;
            chrmissiletreatment[cnt] = MISNORMAL;
            chrmissilecost[cnt] = 0;


            // Kurse state
            chriskursed[cnt] = ((rand()%100) < capkursechance[profile]);
            if(capisitem[profile]==bfalse)  chriskursed[cnt] = bfalse;


            // Ammo
            chrammomax[cnt] = capammomax[profile];
            chrammo[cnt] = capammo[profile];


            // Gender
            chrgender[cnt] = capgender[profile];
            if(chrgender[cnt] == GENRANDOM)  chrgender[cnt] = GENFEMALE+(rand()&1);



            // Team stuff
            chrteam[cnt] = team;
            chrbaseteam[cnt] = team;
            chrcounter[cnt] = teammorale[team];
            if(capinvictus[profile]==bfalse)  teammorale[team]++;
            chrorder[cnt] = 0;
            // Firstborn becomes the leader
            if(teamleader[team]==NOLEADER)
            {
                teamleader[team] = cnt;
            }
            // Skin
            if(capskinoverride[profile]!=NOSKINOVERRIDE)
            {
                skin = capskinoverride[profile]&3;
            }
            if(skin >= madskins[profile])
            {
                skin = 0;
                if(madskins[profile] > 1)
                {
                    skin = rand()%madskins[profile];
                }
            }
            chrtexture[cnt] = madskinstart[profile]+skin;
            // Life and Mana
            chralive[cnt] = btrue;
            chrlifecolor[cnt] = caplifecolor[profile];
            chrmanacolor[cnt] = capmanacolor[profile];
            chrlifemax[cnt] = generate_number(caplifebase[profile], capliferand[profile]);
            chrlife[cnt] = chrlifemax[cnt];
            chrlifereturn[cnt] = caplifereturn[profile];
            chrmanamax[cnt] = generate_number(capmanabase[profile], capmanarand[profile]);
            chrmanaflow[cnt] = generate_number(capmanaflowbase[profile], capmanaflowrand[profile]);
            chrmanareturn[cnt] = generate_number(capmanareturnbase[profile], capmanareturnrand[profile]);
            chrmana[cnt] = chrmanamax[cnt];
            // SWID
            chrstrength[cnt] = generate_number(capstrengthbase[profile], capstrengthrand[profile]);
            chrwisdom[cnt] = generate_number(capwisdombase[profile], capwisdomrand[profile]);
            chrintelligence[cnt] = generate_number(capintelligencebase[profile], capintelligencerand[profile]);
            chrdexterity[cnt] = generate_number(capdexteritybase[profile], capdexterityrand[profile]);
            // Damage
            chrdefense[cnt] = capdefense[profile][skin];
            chrreaffirmdamagetype[cnt]=capattachedprtreaffirmdamagetype[profile];
            chrdamagetargettype[cnt] = capdamagetargettype[profile];
            tnc = 0;
            while(tnc < MAXDAMAGETYPE)
            {
                chrdamagemodifier[cnt][tnc] = capdamagemodifier[profile][tnc][skin];
                tnc++;
            }
            // AI stuff
            chraitype[cnt]= madai[chrmodel[cnt]];
            chrisplayer[cnt] = bfalse;
            chrislocalplayer[cnt] = bfalse;
            chralert[cnt] = ALERTIFSPAWNED;
            chraistate[cnt] = capstateoverride[profile];
            chraicontent[cnt] = capcontentoverride[profile];
            chraitarget[cnt] = cnt;
            chraiowner[cnt] = cnt;
            chraichild[cnt] = cnt;
            chraitime[cnt] = 0;
            chrlatchx[cnt] = 0;
            chrlatchy[cnt] = 0;
            chrlatchbutton[cnt] = 0;
            chrturnmode[cnt] = TURNMODEVELOCITY;
            // Flags
            chrstickybutt[cnt] = capstickybutt[profile];
            chropenstuff[cnt] = capcanopenstuff[profile];
            chrtransferblend[cnt] = captransferblend[profile];
            chrenviro[cnt] = capenviro[profile];
            chrwaterwalk[cnt] = capwaterwalk[profile];
            chrplatform[cnt] = capplatform[profile];
            chrisitem[cnt] = capisitem[profile];
            chrinvictus[cnt] = capinvictus[profile];
            chrismount[cnt] = capismount[profile];
            chrcangrabmoney[cnt] = capcangrabmoney[profile];
            // Jumping
            chrjump[cnt] = capjump[profile];
            chrjumpnumber[cnt] = 0;
            chrjumpnumberreset[cnt] = capjumpnumber[profile];
            chrjumptime[cnt] = JUMPDELAY;
            // Other junk
            chrflyheight[cnt] = capflyheight[profile];
            chrmaxaccel[cnt] = capmaxaccel[profile][skin];
            chralpha[cnt] = capalpha[profile];
            chrlight[cnt] = caplight[profile];
            chrflashand[cnt] = capflashand[profile];
            chrsheen[cnt] = capsheen[profile];
            chrdampen[cnt] = capdampen[profile];
            // Character size and bumping
            chrfat[cnt] = capsize[profile];
            chrsizegoto[cnt] = chrfat[cnt];
            chrsizegototime[cnt] = 0;
            chrshadowsize[cnt] = capshadowsize[profile]*chrfat[cnt];
            chrbumpsize[cnt] = capbumpsize[profile]*chrfat[cnt];
            chrbumpsizebig[cnt] = capbumpsizebig[profile]*chrfat[cnt];
            chrbumpheight[cnt] = capbumpheight[profile]*chrfat[cnt];

            chrshadowsizesave[cnt] = capshadowsize[profile];
            chrbumpsizesave[cnt] = capbumpsize[profile];
            chrbumpsizebigsave[cnt] = capbumpsizebig[profile];
            chrbumpheightsave[cnt] = capbumpheight[profile];

            chrbumpdampen[cnt] = capbumpdampen[profile];
            chrweight[cnt] = capweight[profile]*chrfat[cnt];
            if(capweight[profile]==255) chrweight[cnt] = 65535;
            chrbumplast[cnt] = cnt;
            chrattacklast[cnt] = MAXCHR;
            chrhitlast[cnt] = cnt;
            // Grip info
            chrattachedto[cnt] = MAXCHR;
            chrholdingwhich[cnt][0] = MAXCHR;
            chrholdingwhich[cnt][1] = MAXCHR;
            // Image rendering
            chruoffset[cnt] = 0;
            chrvoffset[cnt] = 0;
            chruoffvel[cnt] = capuoffvel[profile];
            chrvoffvel[cnt] = capvoffvel[profile];
            chrredshift[cnt] = 0;
            chrgrnshift[cnt] = 0;
            chrblushift[cnt] = 0;
            // Movement
            chrsneakspd[cnt] = capsneakspd[profile];
            chrwalkspd[cnt] = capwalkspd[profile];
            chrrunspd[cnt] = caprunspd[profile];


            // Set up position
            chrxpos[cnt] = x;
            chrypos[cnt] = y;
            chroldx[cnt] = x;
            chroldy[cnt] = y;
            chrturnleftright[cnt] = facing;
            chrlightturnleftright[cnt] = 0;
            ix = x;
            iy = y;
            chronwhichfan[cnt] = (ix>>7)+meshfanstart[iy>>7];
            chrlevel[cnt] = get_level(chrxpos[cnt], chrypos[cnt], chronwhichfan[cnt], chrwaterwalk[cnt])+RAISE;
            if(z < chrlevel[cnt])
                z = chrlevel[cnt];
            chrzpos[cnt] = z;
            chroldz[cnt] = z;
            chrxstt[cnt] = chrxpos[cnt];
            chrystt[cnt] = chrypos[cnt];
            chrzstt[cnt] = chrzpos[cnt];
            chrxvel[cnt] = 0;
            chryvel[cnt] = 0;
            chrzvel[cnt] = 0;
            chrturnmaplr[cnt] = 32768;  // These two mean on level surface
            chrturnmapud[cnt] = 32768;
            chrscale[cnt] = chrfat[cnt]*madscale[chrmodel[cnt]]*4;


            // AI and action stuff
            chraigoto[cnt] = 0;
            chraigotoadd[cnt] = 1;
            chraigotox[cnt][0] = chrxpos[cnt];
            chraigotoy[cnt][0] = chrypos[cnt];
            chractionready[cnt] = btrue;
            chrkeepaction[cnt] = bfalse;
            chrloopaction[cnt] = bfalse;
            chraction[cnt] = ACTIONDA;
            chrnextaction[cnt] = ACTIONDA;
            chrlip[cnt] = 0;
            chrframe[cnt] = madframestart[chrmodel[cnt]];
            chrlastframe[cnt] = chrframe[cnt];
            chrpassage[cnt] = 0;
            chrholdingweight[cnt] = 0;


            // Timers set to 0
            chrgrogtime[cnt] = 0;
            chrdazetime[cnt] = 0;


            // Money is added later
            chrmoney[cnt] = capmoney[profile];


            // Name the character
            if(name == NULL)
            {
                // Generate a random name
                naming_names(profile);
                sprintf(chrname[cnt], "%s", namingnames);
            }
            else
            {
                // A name has been given
                tnc = 0;
                while(tnc < MAXCAPNAMESIZE-1)
                {
                    chrname[cnt][tnc] = name[tnc];
                    tnc++;
                }
                chrname[cnt][tnc] = 0;
            }

            // Set up initial fade in lighting
            tnc = 0;
            while(tnc < madtransvertices[chrmodel[cnt]])
            {
                chrvrta[cnt][tnc] = 0;
                tnc++;
            }


            // Particle attachments
            tnc = 0;
            while(tnc < capattachedprtamount[profile])
            {
                spawn_one_particle(chrxpos[cnt], chrypos[cnt], chrzpos[cnt],
                                   0, chrmodel[cnt], capattachedprttype[profile],
                                   cnt, SPAWNLAST+tnc, chrteam[cnt], cnt, tnc, MAXCHR);
                tnc++;
            }
            chrreaffirmdamagetype[cnt] = capattachedprtreaffirmdamagetype[profile];


            // Experience
            tnc = generate_number(capexperiencebase[profile], capexperiencerand[profile]);
            if(tnc > MAXXP) tnc = MAXXP;
            chrexperience[cnt] = tnc;
            chrexperiencelevel[cnt] = capleveloverride[profile];
        }
    }
    return cnt;
}

//--------------------------------------------------------------------------------------------
void respawn_character(unsigned short character)
{
    // ZZ> This function respawns a character
    unsigned short item;

    if(chralive[character]==bfalse)
    {
        spawn_poof(character, chrmodel[character]);
        disaffirm_attached_particles(character);
        chralive[character] = btrue;
        chrboretime[character] = BORETIME;
        chrcarefultime[character] = CAREFULTIME;
        chrlife[character] = chrlifemax[character];
        chrmana[character] = chrmanamax[character];
        chrxpos[character] = chrxstt[character];
        chrypos[character] = chrystt[character];
        chrzpos[character] = chrzstt[character];
        chrxvel[character] = 0;
        chryvel[character] = 0;
        chrzvel[character] = 0;
        chrteam[character] = chrbaseteam[character];
        chrcanbecrushed[character] = bfalse;
        chrturnmaplr[character] = 32768;  // These two mean on level surface
        chrturnmapud[character] = 32768;
        if(teamleader[chrteam[character]]==NOLEADER)  teamleader[chrteam[character]]=character;
        if(chrinvictus[character]==bfalse)  teammorale[chrbaseteam[character]]++;
        chractionready[character] = btrue;
        chrkeepaction[character] = bfalse;
        chrloopaction[character] = bfalse;
        chraction[character] = ACTIONDA;
        chrnextaction[character] = ACTIONDA;
        chrlip[character] = 0;
        chrframe[character] = madframestart[chrmodel[character]];
        chrlastframe[character] = chrframe[character];
        chrplatform[character] = capplatform[chrmodel[character]];
        chrflyheight[character] = capflyheight[chrmodel[character]];
        chrbumpdampen[character] = capbumpdampen[chrmodel[character]];
        chrbumpsize[character] = capbumpsize[chrmodel[character]]*chrfat[character];
        chrbumpsizebig[character] = capbumpsizebig[chrmodel[character]]*chrfat[character];
        chrbumpheight[character] = capbumpheight[chrmodel[character]]*chrfat[character];

        chrbumpsizesave[character] = capbumpsize[chrmodel[character]];
        chrbumpsizebigsave[character] = capbumpsizebig[chrmodel[character]];
        chrbumpheightsave[character] = capbumpheight[chrmodel[character]];

//        chralert[character] = ALERTIFSPAWNED;
        chralert[character] = 0;
//        chraistate[character] = 0;
        chraitarget[character] = character;
        chraitime[character] = 0;
        chrgrogtime[character] = 0;
        chrdazetime[character] = 0;
        reaffirm_attached_particles(character);


        // Let worn items come back
        item = chrnextinpack[character];
        while(item != MAXCHR)
        {
            if(chrisequipped[item])
            {
                chrisequipped[item] = bfalse;
                chralert[item] |= ALERTIFATLASTWAYPOINT;  // doubles as PutAway
            }
            item = chrnextinpack[item];
        }
    }
}

//--------------------------------------------------------------------------------------------
unsigned short change_armor(unsigned short character, unsigned short skin)
{
    // ZZ> This function changes the armor of the character
    unsigned short enchant, sTmp;
    int iTmp;


    // Remove armor enchantments
    enchant = chrfirstenchant[character];
    while(enchant < MAXENCHANT)
    {
        unset_enchant_value(enchant, SETSLASHMODIFIER);
        unset_enchant_value(enchant, SETCRUSHMODIFIER);
        unset_enchant_value(enchant, SETPOKEMODIFIER);
        unset_enchant_value(enchant, SETHOLYMODIFIER);
        unset_enchant_value(enchant, SETEVILMODIFIER);
        unset_enchant_value(enchant, SETFIREMODIFIER);
        unset_enchant_value(enchant, SETICEMODIFIER);
        unset_enchant_value(enchant, SETZAPMODIFIER);
        enchant = encnextenchant[enchant];
    }


    // Change the skin
    sTmp = chrmodel[character];
    if(skin > madskins[sTmp])  skin = 0;
    chrtexture[character] = madskinstart[sTmp]+skin;


    // Change stats associated with skin
    chrdefense[character] = capdefense[sTmp][skin];
    iTmp = 0;
    while(iTmp < MAXDAMAGETYPE)
    {
        chrdamagemodifier[character][iTmp] = capdamagemodifier[sTmp][iTmp][skin];
        iTmp++;
    }
    chrmaxaccel[character] = capmaxaccel[sTmp][skin];


    // Reset armor enchantments
    // These should really be done in reverse order ( Start with last enchant ), but
    // I don't care at this point !!!BAD!!!
    enchant = chrfirstenchant[character];
    while(enchant < MAXENCHANT)
    {
        set_enchant_value(enchant, SETSLASHMODIFIER, enceve[enchant]);
        set_enchant_value(enchant, SETCRUSHMODIFIER, enceve[enchant]);
        set_enchant_value(enchant, SETPOKEMODIFIER, enceve[enchant]);
        set_enchant_value(enchant, SETHOLYMODIFIER, enceve[enchant]);
        set_enchant_value(enchant, SETEVILMODIFIER, enceve[enchant]);
        set_enchant_value(enchant, SETFIREMODIFIER, enceve[enchant]);
        set_enchant_value(enchant, SETICEMODIFIER, enceve[enchant]);
        set_enchant_value(enchant, SETZAPMODIFIER, enceve[enchant]);
        add_enchant_value(enchant, ADDACCEL, enceve[enchant]);
        add_enchant_value(enchant, ADDDEFENSE, enceve[enchant]);
        enchant = encnextenchant[enchant];
    }
    return skin;
}

//--------------------------------------------------------------------------------------------
void change_character(unsigned short cnt, unsigned short profile, unsigned char skin,
    unsigned char leavewhich)
{
    // ZZ> This function polymorphs a character, changing stats, dropping weapons
    int tnc, enchant;
    unsigned short sTmp, item;


    profile = profile & (MAXMODEL-1);
    if(madused[profile])
    {
        // Drop left weapon
        sTmp = chrholdingwhich[cnt][0];
        if(sTmp != MAXCHR && (capgripvalid[profile][0]==bfalse || capismount[profile]))
        {
            detach_character_from_mount(sTmp, btrue, btrue);
            if(chrismount[cnt])
            {
                chrzvel[sTmp] = DISMOUNTZVEL;
                chrzpos[sTmp]+=DISMOUNTZVEL;
                chrjumptime[sTmp]=JUMPDELAY;
            }
        }


        // Drop right weapon
        sTmp = chrholdingwhich[cnt][1];
        if(sTmp != MAXCHR && capgripvalid[profile][1]==bfalse)
        {
            detach_character_from_mount(sTmp, btrue, btrue);
            if(chrismount[cnt])
            {
                chrzvel[sTmp] = DISMOUNTZVEL;
                chrzpos[sTmp]+=DISMOUNTZVEL;
                chrjumptime[sTmp]=JUMPDELAY;
            }
        }


        // Remove particles
        disaffirm_attached_particles(cnt);


        // Remove enchantments
        if(leavewhich == LEAVEFIRST)
        {
            // Remove all enchantments except top one
            enchant = chrfirstenchant[cnt];
            if(enchant != MAXENCHANT)
            {
                while(encnextenchant[enchant] != MAXENCHANT)
                {
                    remove_enchant(encnextenchant[enchant]);
                }
            }
        }
        if(leavewhich == LEAVENONE)
        {
            // Remove all enchantments
            disenchant_character(cnt);
        }


        // Stuff that must be set
        chrmodel[cnt] = profile;
        chrstoppedby[cnt] = capstoppedby[profile];
        chrlifeheal[cnt] = caplifeheal[profile];
        chrmanacost[cnt] = capmanacost[profile];
        // Ammo
        chrammomax[cnt] = capammomax[profile];
        chrammo[cnt] = capammo[profile];
        // Gender
        if(capgender[profile] != GENRANDOM)  // GENRANDOM means keep old gender
        {
            chrgender[cnt] = capgender[profile];
        }


        // AI stuff
        chraitype[cnt]= madai[profile];
        chraistate[cnt] = 0;
        chraitime[cnt] = 0;
        chrlatchx[cnt] = 0;
        chrlatchy[cnt] = 0;
        chrlatchbutton[cnt] = 0;
        chrturnmode[cnt] = TURNMODEVELOCITY;
        // Flags
        chrstickybutt[cnt] = capstickybutt[profile];
        chropenstuff[cnt] = capcanopenstuff[profile];
        chrtransferblend[cnt] = captransferblend[profile];
        chrenviro[cnt] = capenviro[profile];
        chrplatform[cnt] = capplatform[profile];
        chrisitem[cnt] = capisitem[profile];
        chrinvictus[cnt] = capinvictus[profile];
        chrismount[cnt] = capismount[profile];
        chrcangrabmoney[cnt] = capcangrabmoney[profile];
        chrjumptime[cnt] = JUMPDELAY;
        // Character size and bumping
        chrshadowsize[cnt] = capshadowsize[profile]*chrfat[cnt];
        chrbumpsize[cnt] = capbumpsize[profile]*chrfat[cnt];
        chrbumpsizebig[cnt] = capbumpsizebig[profile]*chrfat[cnt];
        chrbumpheight[cnt] = capbumpheight[profile]*chrfat[cnt];

        chrshadowsizesave[cnt] = capshadowsize[profile];
        chrbumpsizesave[cnt] = capbumpsize[profile];
        chrbumpsizebigsave[cnt] = capbumpsizebig[profile];
        chrbumpheightsave[cnt] = capbumpheight[profile];

        chrbumpdampen[cnt] = capbumpdampen[profile];
        chrweight[cnt] = capweight[profile]*chrfat[cnt];
        if(capweight[profile]==255) chrweight[cnt] = 65535;
        // Character scales...  Magic numbers
        if(chrattachedto[cnt] == MAXCHR)
        {
            chrscale[cnt] = chrfat[cnt]*madscale[profile]*4;
        }
        else
        {
            chrscale[cnt] = chrfat[cnt]/(chrfat[chrattachedto[cnt]]*1280);
            tnc = madvertices[chrmodel[chrattachedto[cnt]]] - chrinwhichhand[cnt];
            chrweapongrip[cnt][0] = tnc;
            chrweapongrip[cnt][1] = tnc+1;
            chrweapongrip[cnt][2] = tnc+2;
            chrweapongrip[cnt][3] = tnc+3;
        }
        item = chrholdingwhich[cnt][0];
        if(item != MAXCHR)
        {
            chrscale[item] = chrfat[item]/(chrfat[cnt]*1280);
            tnc = madvertices[chrmodel[cnt]] - GRIPLEFT;
            chrweapongrip[item][0] = tnc;
            chrweapongrip[item][1] = tnc+1;
            chrweapongrip[item][2] = tnc+2;
            chrweapongrip[item][3] = tnc+3;
        }
        item = chrholdingwhich[cnt][1];
        if(item != MAXCHR)
        {
            chrscale[item] = chrfat[item]/(chrfat[cnt]*1280);
            tnc = madvertices[chrmodel[cnt]] - GRIPRIGHT;
            chrweapongrip[item][0] = tnc;
            chrweapongrip[item][1] = tnc+1;
            chrweapongrip[item][2] = tnc+2;
            chrweapongrip[item][3] = tnc+3;
        }
        // Image rendering
        chruoffset[cnt] = 0;
        chrvoffset[cnt] = 0;
        chruoffvel[cnt] = capuoffvel[profile];
        chrvoffvel[cnt] = capvoffvel[profile];
        // Movement
        chrsneakspd[cnt] = capsneakspd[profile];
        chrwalkspd[cnt] = capwalkspd[profile];
        chrrunspd[cnt] = caprunspd[profile];


        // AI and action stuff
        chractionready[cnt] = btrue;
        chrkeepaction[cnt] = bfalse;
        chrloopaction[cnt] = bfalse;
        chraction[cnt] = ACTIONDA;
        chrnextaction[cnt] = ACTIONDA;
        chrlip[cnt] = 0;
        chrframe[cnt] = madframestart[profile];
        chrlastframe[cnt] = chrframe[cnt];
        chrholdingweight[cnt] = 0;


        // Set the skin
        change_armor(cnt, skin);


        // Reaffirm them particles...
        chrreaffirmdamagetype[cnt] = capattachedprtreaffirmdamagetype[profile];
        reaffirm_attached_particles(cnt);


        // Set up initial fade in lighting
        tnc = 0;
        while(tnc < madtransvertices[chrmodel[cnt]])
        {
            chrvrta[cnt][tnc] = 0;
            tnc++;
        }
    }
}

//--------------------------------------------------------------------------------------------
unsigned short get_target_in_block(int x, int y, unsigned short character, char items,
    char friends, char enemies, char dead, char seeinvisible, unsigned int idsz,
    char excludeid)
{
    // ZZ> This is a good little helper, that returns != MAXCHR if a suitable target
    //     was found
    int cnt;
    unsigned short charb;
    unsigned int fanblock;
    unsigned char team;


    if(x >= 0 && x < (meshsizex>>2) && y >= 0 && y < (meshsizey>>2))
    {
        team = chrteam[character];
        fanblock = x+meshblockstart[y];
        charb = meshbumplistchr[fanblock];
        cnt = 0;
        while(cnt < meshbumplistchrnum[fanblock])
        {
            if(dead != chralive[charb] && (seeinvisible || (chralpha[charb]>INVISIBLE && chrlight[charb]>INVISIBLE)))
            {
                if((enemies && teamhatesteam[team][chrteam[charb]] && chrinvictus[charb]==bfalse) ||
                   (items && chrisitem[charb]) ||
                   (friends && chrbaseteam[charb]==team))
                {
                    if(charb != character && chrattachedto[character] != charb)
                    {
                        if(chrisitem[charb] == bfalse || items)
                        {
                            if(idsz != IDSZNONE)
                            {
                                if(capidsz[chrmodel[charb]][IDSZPARENT] == idsz ||
                                   capidsz[chrmodel[charb]][IDSZTYPE] == idsz)
                                {
                                    if(!excludeid) return charb;
                                }
                                else
                                {
                                    if(excludeid)  return charb;
                                }
                            }
                            else
                            {
                                return charb;
                            }
                        }
                    }
                }
            }
            charb = chrbumpnext[charb];
            cnt++;
        }
    }
    return MAXCHR;
}

//--------------------------------------------------------------------------------------------
unsigned short get_nearby_target(unsigned short character, char items,
    char friends, char enemies, char dead, unsigned int idsz)
{
    // ZZ> This function finds a nearby target, or it returns MAXCHR if it can't find one
    int x, y;
    char seeinvisible;
    seeinvisible = chrcanseeinvisible[character];


    // Current fanblock
    x = ((int)chrxpos[character])>>9;
    y = ((int)chrypos[character])>>9;
    return get_target_in_block(x, y, character, items, friends, enemies, dead, seeinvisible, idsz, 0);
}

//--------------------------------------------------------------------------------------------
unsigned char cost_mana(unsigned short character, int amount, unsigned short killer)
{
    // ZZ> This function takes mana from a character ( or gives mana ),
    //     and returns btrue if the character had enough to pay, or bfalse
    //     otherwise
    int iTmp;


    iTmp = chrmana[character] - amount;
    if(iTmp < 0)
    {
        chrmana[character] = 0;
        if(chrcanchannel[character])
        {
            chrlife[character] += iTmp;
            if(chrlife[character] <= 0)
            {
                kill_character(character, character);
            }
            return btrue;
        }
        return bfalse;
    }
    else
    {
        chrmana[character] = iTmp;
        if(iTmp > chrmanamax[character])
        {
            chrmana[character] = chrmanamax[character];
        }
    }
    return btrue;
}

//--------------------------------------------------------------------------------------------
unsigned short find_distant_target(unsigned short character, int maxdistance)
{
    // ZZ> This function finds a target, or it returns MAXCHR if it can't find one...
    //     maxdistance should be the square of the actual distance you want to use
    //     as the cutoff...
    int cnt, distance, xdistance, ydistance;
    unsigned char team;

    team = chrteam[character];
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chron[cnt])
        {
            if(chrattachedto[cnt]==MAXCHR && chrinpack[cnt]==bfalse)
            {
                if(teamhatesteam[team][chrteam[cnt]] && chralive[cnt] && chrinvictus[cnt]==bfalse)
                {
                    if(chrcanseeinvisible[character] || (chralpha[cnt]>INVISIBLE && chrlight[cnt]>INVISIBLE))
                    {
                        xdistance = chrxpos[cnt] - chrxpos[character];
                        ydistance = chrypos[cnt] - chrypos[character];
                        distance = xdistance*xdistance + ydistance*ydistance;
                        if(distance < maxdistance)
                        {
                            return cnt;
                        }
                    }
                }
            }
        }
        cnt++;
    }
    return MAXCHR;
}

//--------------------------------------------------------------------------------------------
void switch_team(int character, unsigned char team)
{
    // ZZ> This function makes a character join another team...
    if(team < MAXTEAM)
    {
        if(chrinvictus[character]==bfalse)
        {
            teammorale[chrbaseteam[character]]--;
            teammorale[team]++;
        }
        if((chrismount[character]==bfalse || chrholdingwhich[character][0]==MAXCHR) &&
           (chrisitem[character]==bfalse || chrattachedto[character]==MAXCHR))
        {
            chrteam[character] = team;
        }
        chrbaseteam[character] = team;
        if(teamleader[team]==NOLEADER)
        {
            teamleader[team] = character;
        }
    }
}

//--------------------------------------------------------------------------------------------
void get_nearest_in_block(int x, int y, unsigned short character, char items,
    char friends, char enemies, char dead, char seeinvisible, unsigned int idsz)
{
    // ZZ> This is a good little helper
    float distance, xdis, ydis;
    int cnt;
    unsigned char team;
    unsigned short charb;
    unsigned int fanblock;


    if(x >= 0 && x < (meshsizex>>2) && y >= 0 && y < (meshsizey>>2))
    {
        team = chrteam[character];
        fanblock = x+meshblockstart[y];
        charb = meshbumplistchr[fanblock];
        cnt = 0;
        while(cnt < meshbumplistchrnum[fanblock])
        {
            if(dead != chralive[charb] && (seeinvisible || (chralpha[charb]>INVISIBLE && chrlight[charb]>INVISIBLE)))
            {
                if((enemies && teamhatesteam[team][chrteam[charb]]) ||
                   (items && chrisitem[charb]) ||
                   (friends && chrteam[charb]==team) ||
                   (friends && enemies))
                {
                    if(charb != character && chrattachedto[character] != charb && chrattachedto[charb] == MAXCHR && chrinpack[charb] == bfalse)
                    {
                        if(chrinvictus[charb] == bfalse || items)
                        {
                            if(idsz != IDSZNONE)
                            {
                                if(capidsz[chrmodel[charb]][IDSZPARENT] == idsz ||
                                   capidsz[chrmodel[charb]][IDSZTYPE] == idsz)
                                {
                                    xdis = chrxpos[character] - chrxpos[charb];
                                    ydis = chrypos[character] - chrypos[charb];
                                    xdis = xdis*xdis;
                                    ydis = ydis*ydis;
                                    distance = xdis+ydis;
                                    if(distance < globaldistance)
                                    {
                                        globalnearest = charb;
                                        globaldistance = distance;
                                    }
                                }
                            }
                            else
                            {
                                xdis = chrxpos[character] - chrxpos[charb];
                                ydis = chrypos[character] - chrypos[charb];
                                xdis = xdis*xdis;
                                ydis = ydis*ydis;
                                distance = xdis+ydis;
                                if(distance < globaldistance)
                                {
                                    globalnearest = charb;
                                    globaldistance = distance;
                                }
                            }
                        }
                    }
                }
            }
            charb = chrbumpnext[charb];
            cnt++;
        }
    }
    return;
}

//--------------------------------------------------------------------------------------------
unsigned short get_nearest_target(unsigned short character, char items,
    char friends, char enemies, char dead, unsigned int idsz)
{
    // ZZ> This function finds an target, or it returns MAXCHR if it can't find one
    int x, y;
    char seeinvisible;
    seeinvisible = chrcanseeinvisible[character];


    // Current fanblock
    x = ((int)chrxpos[character])>>9;
    y = ((int)chrypos[character])>>9;


    globalnearest = MAXCHR;
    globaldistance = 999999;
    get_nearest_in_block(x, y, character, items, friends, enemies, dead, seeinvisible, idsz);

    get_nearest_in_block(x-1, y, character, items, friends, enemies, dead, seeinvisible, idsz);
    get_nearest_in_block(x+1, y, character, items, friends, enemies, dead, seeinvisible, idsz);
    get_nearest_in_block(x, y-1, character, items, friends, enemies, dead, seeinvisible, idsz);
    get_nearest_in_block(x, y+1, character, items, friends, enemies, dead, seeinvisible, idsz);

    get_nearest_in_block(x-1, y+1, character, items, friends, enemies, dead, seeinvisible, idsz);
    get_nearest_in_block(x+1, y-1, character, items, friends, enemies, dead, seeinvisible, idsz);
    get_nearest_in_block(x-1, y-1, character, items, friends, enemies, dead, seeinvisible, idsz);
    get_nearest_in_block(x+1, y+1, character, items, friends, enemies, dead, seeinvisible, idsz);
    return globalnearest;
}

//--------------------------------------------------------------------------------------------
unsigned short get_wide_target(unsigned short character, char items,
    char friends, char enemies, char dead, unsigned int idsz, char excludeid)
{
    // ZZ> This function finds an target, or it returns MAXCHR if it can't find one
    int x, y;
    unsigned short enemy;
    char seeinvisible;
    seeinvisible = chrcanseeinvisible[character];

    // Current fanblock
    x = ((int)chrxpos[character])>>9;
    y = ((int)chrypos[character])>>9;
    enemy = get_target_in_block(x, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
    if(enemy != MAXCHR)  return enemy;

    enemy = get_target_in_block(x-1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
    if(enemy != MAXCHR)  return enemy;
    enemy = get_target_in_block(x+1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
    if(enemy != MAXCHR)  return enemy;
    enemy = get_target_in_block(x, y-1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
    if(enemy != MAXCHR)  return enemy;
    enemy = get_target_in_block(x, y+1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
    if(enemy != MAXCHR)  return enemy;

    enemy = get_target_in_block(x-1, y+1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
    if(enemy != MAXCHR)  return enemy;
    enemy = get_target_in_block(x+1, y-1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
    if(enemy != MAXCHR)  return enemy;
    enemy = get_target_in_block(x-1, y-1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
    if(enemy != MAXCHR)  return enemy;
    enemy = get_target_in_block(x+1, y+1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
    return enemy;
}

//--------------------------------------------------------------------------------------------
void issue_clean(unsigned short character)
{
    // ZZ> This function issues a clean up order to all teammates
    unsigned char team;
    unsigned short cnt;


    team = chrteam[character];
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chrteam[cnt]==team && chralive[cnt] == bfalse)
        {
            chraitime[cnt] = 2;  // Don't let it think too much...
            chralert[cnt]=ALERTIFCLEANEDUP;
        }
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
int restock_ammo(unsigned short character, unsigned int idsz)
{
    // ZZ> This function restocks the characters ammo, if it needs ammo and if
    //     either its parent or type idsz match the given idsz.  This
    //     function returns the amount of ammo given.
    int amount, model;

    amount = 0;
    if(character < MAXCHR)
    {
        if(chron[character])
        {
            model = chrmodel[character];
            if(capidsz[model][IDSZPARENT] == idsz || capidsz[model][IDSZTYPE] == idsz)
            {
                if(chrammo[character] < chrammomax[character])
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
void issue_order(unsigned short character, unsigned int order)
{
    // ZZ> This function issues an order for help to all teammates
    unsigned char team;
    unsigned char counter;
    unsigned short cnt;


    team = chrteam[character];
    counter = 0;
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chrteam[cnt]==team)
        {
            chrorder[cnt] = order;
            chrcounter[cnt] = counter;
            chralert[cnt]=chralert[cnt]|ALERTIFORDERED;
            counter++;
        }
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void issue_special_order(unsigned int order, unsigned int idsz)
{
    // ZZ> This function issues an order to all characters with the a matching special IDSZ
    unsigned char counter;
    unsigned short cnt;


    counter = 0;
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chron[cnt])
        {
            if(capidsz[chrmodel[cnt]][IDSZSPECIAL] == idsz)
            {
                chrorder[cnt] = order;
                chrcounter[cnt] = counter;
                chralert[cnt]=chralert[cnt]|ALERTIFORDERED;
                counter++;
            }
        }
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void set_alerts(int character)
{
    // ZZ> This function polls some alert conditions
    if(chraitime[character]!=0)
    {
        chraitime[character]--;
    }
    if(chrxpos[character]<chraigotox[character][chraigoto[character]]+WAYTHRESH&&
       chrxpos[character]>chraigotox[character][chraigoto[character]]-WAYTHRESH&&
       chrypos[character]<chraigotoy[character][chraigoto[character]]+WAYTHRESH&&
       chrypos[character]>chraigotoy[character][chraigoto[character]]-WAYTHRESH)
    {
        chralert[character]=chralert[character]|ALERTIFATWAYPOINT;
        chraigoto[character]++;
        if(chraigoto[character]==chraigotoadd[character])
        {
            chraigoto[character] = 0;
            if(capisequipment[chrmodel[character]]==bfalse)
            {
                chralert[character]=chralert[character]|ALERTIFATLASTWAYPOINT;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void free_all_enchants()
{
    // ZZ> This functions frees all of the enchantments
    numfreeenchant = 0;
    while(numfreeenchant < MAXENCHANT)
    {
        freeenchant[numfreeenchant]=numfreeenchant;
        encon[numfreeenchant] = bfalse;
        numfreeenchant++;
    }
}

//--------------------------------------------------------------------------------------------
void load_one_enchant_type(char* szLoadName, unsigned short profile)
{
    // ZZ> This function loads the enchantment associated with an object
    FILE* fileread;
    char cTmp;
    int iTmp, tTmp, idsz, test;
    float fTmp;
    int num;

    globalname = szLoadName;
    evevalid[profile] = bfalse;
    fileread = fopen(szLoadName, "r");
    if(fileread)
    {
        evevalid[profile] = btrue;


        // btrue/bfalse values
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            everetarget[profile] = bfalse;
            if(cTmp == 'T' || cTmp == 't')  everetarget[profile] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            eveoverride[profile] = bfalse;
            if(cTmp == 'T' || cTmp == 't')  eveoverride[profile] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            everemoveoverridden[profile] = bfalse;
            if(cTmp == 'T' || cTmp == 't')  everemoveoverridden[profile] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evekillonend[profile] = bfalse;
            if(cTmp == 'T' || cTmp == 't')  evekillonend[profile] = btrue;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evepoofonend[profile] = bfalse;
            if(cTmp == 'T' || cTmp == 't')  evepoofonend[profile] = btrue;


        // More stuff
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  evetime[profile] = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  eveendmessage[profile] = iTmp;


        // Drain stuff
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  eveownermana[profile] = fTmp*256;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  evetargetmana[profile] = fTmp*256;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            eveendifcantpay[profile] = bfalse;
            if(cTmp == 'T' || cTmp == 't')  eveendifcantpay[profile] = btrue;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  eveownerlife[profile] = fTmp*256;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  evetargetlife[profile] = fTmp*256;


        // Specifics
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evedontdamagetype[profile] = DAMAGENULL;
            if(cTmp=='S' || cTmp=='s')  evedontdamagetype[profile] = DAMAGESLASH;
            if(cTmp=='C' || cTmp=='c')  evedontdamagetype[profile] = DAMAGECRUSH;
            if(cTmp=='P' || cTmp=='p')  evedontdamagetype[profile] = DAMAGEPOKE;
            if(cTmp=='H' || cTmp=='h')  evedontdamagetype[profile] = DAMAGEHOLY;
            if(cTmp=='E' || cTmp=='e')  evedontdamagetype[profile] = DAMAGEEVIL;
            if(cTmp=='F' || cTmp=='f')  evedontdamagetype[profile] = DAMAGEFIRE;
            if(cTmp=='I' || cTmp=='i')  evedontdamagetype[profile] = DAMAGEICE;
            if(cTmp=='Z' || cTmp=='z')  evedontdamagetype[profile] = DAMAGEZAP;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            eveonlydamagetype[profile] = DAMAGENULL;
            if(cTmp=='S' || cTmp=='s')  eveonlydamagetype[profile] = DAMAGESLASH;
            if(cTmp=='C' || cTmp=='c')  eveonlydamagetype[profile] = DAMAGECRUSH;
            if(cTmp=='P' || cTmp=='p')  eveonlydamagetype[profile] = DAMAGEPOKE;
            if(cTmp=='H' || cTmp=='h')  eveonlydamagetype[profile] = DAMAGEHOLY;
            if(cTmp=='E' || cTmp=='e')  eveonlydamagetype[profile] = DAMAGEEVIL;
            if(cTmp=='F' || cTmp=='f')  eveonlydamagetype[profile] = DAMAGEFIRE;
            if(cTmp=='I' || cTmp=='i')  eveonlydamagetype[profile] = DAMAGEICE;
            if(cTmp=='Z' || cTmp=='z')  eveonlydamagetype[profile] = DAMAGEZAP;
        goto_colon(fileread);  everemovedbyidsz[profile] = get_idsz(fileread);


        // Now the set values
        num = 0;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);
            evesetvalue[profile][num] = DAMAGESLASH;
            if(cTmp=='C' || cTmp=='c')  evesetvalue[profile][num] = DAMAGECRUSH;
            if(cTmp=='P' || cTmp=='p')  evesetvalue[profile][num] = DAMAGEPOKE;
            if(cTmp=='H' || cTmp=='h')  evesetvalue[profile][num] = DAMAGEHOLY;
            if(cTmp=='E' || cTmp=='e')  evesetvalue[profile][num] = DAMAGEEVIL;
            if(cTmp=='F' || cTmp=='f')  evesetvalue[profile][num] = DAMAGEFIRE;
            if(cTmp=='I' || cTmp=='i')  evesetvalue[profile][num] = DAMAGEICE;
            if(cTmp=='Z' || cTmp=='z')  evesetvalue[profile][num] = DAMAGEZAP;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            fscanf(fileread, "%d", &iTmp);  evesetvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            fscanf(fileread, "%d", &iTmp);  evesetvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            fscanf(fileread, "%d", &iTmp);  evesetvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);  iTmp = 0;
            if(cTmp == 'T') iTmp = DAMAGEINVERT;
            if(cTmp == 'C') iTmp = DAMAGECHARGE;
            fscanf(fileread, "%d", &tTmp);  evesetvalue[profile][num] = iTmp|tTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);  iTmp = 0;
            if(cTmp == 'T') iTmp = DAMAGEINVERT;
            if(cTmp == 'C') iTmp = DAMAGECHARGE;
            fscanf(fileread, "%d", &tTmp);  evesetvalue[profile][num] = iTmp|tTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);  iTmp = 0;
            if(cTmp == 'T') iTmp = DAMAGEINVERT;
            if(cTmp == 'C') iTmp = DAMAGECHARGE;
            fscanf(fileread, "%d", &tTmp);  evesetvalue[profile][num] = iTmp|tTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);  iTmp = 0;
            if(cTmp == 'T') iTmp = DAMAGEINVERT;
            if(cTmp == 'C') iTmp = DAMAGECHARGE;
            fscanf(fileread, "%d", &tTmp);  evesetvalue[profile][num] = iTmp|tTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);  iTmp = 0;
            if(cTmp == 'T') iTmp = DAMAGEINVERT;
            if(cTmp == 'C') iTmp = DAMAGECHARGE;
            fscanf(fileread, "%d", &tTmp);  evesetvalue[profile][num] = iTmp|tTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);  iTmp = 0;
            if(cTmp == 'T') iTmp = DAMAGEINVERT;
            if(cTmp == 'C') iTmp = DAMAGECHARGE;
            fscanf(fileread, "%d", &tTmp);  evesetvalue[profile][num] = iTmp|tTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);  iTmp = 0;
            if(cTmp == 'T') iTmp = DAMAGEINVERT;
            if(cTmp == 'C') iTmp = DAMAGECHARGE;
            fscanf(fileread, "%d", &tTmp);  evesetvalue[profile][num] = iTmp|tTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);  iTmp = 0;
            if(cTmp == 'T') iTmp = DAMAGEINVERT;
            if(cTmp == 'C') iTmp = DAMAGECHARGE;
            fscanf(fileread, "%d", &tTmp);  evesetvalue[profile][num] = iTmp|tTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            fscanf(fileread, "%d", &iTmp);  evesetvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            fscanf(fileread, "%d", &iTmp);  evesetvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            fscanf(fileread, "%d", &iTmp);  evesetvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            fscanf(fileread, "%d", &iTmp);  evesetvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            fscanf(fileread, "%d", &iTmp);  evesetvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);
            evesetvalue[profile][num] = (cTmp=='T' || cTmp=='t');
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);
            evesetvalue[profile][num] = (cTmp=='T' || cTmp=='t');
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            cTmp = get_first_letter(fileread);
            evesetvalue[profile][num] = MISNORMAL;
            if(cTmp=='R' || cTmp=='r')  evesetvalue[profile][num] = MISREFLECT;
            if(cTmp=='D' || cTmp=='d')  evesetvalue[profile][num] = MISDEFLECT;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            fscanf(fileread, "%f", &fTmp);  fTmp = fTmp * 16;
            evesetvalue[profile][num] = fTmp;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            evesetvalue[profile][num] = btrue;
            num++;
        goto_colon(fileread);  cTmp = get_first_letter(fileread);
            evesetyesno[profile][num] = (cTmp=='T' || cTmp=='t');
            evesetvalue[profile][num] = btrue;
            num++;


        // Now read in the add values
        num = 0;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
            eveaddvalue[profile][num] = fTmp * 16;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
            eveaddvalue[profile][num] = fTmp * 127;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
            eveaddvalue[profile][num] = fTmp * 127;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
            eveaddvalue[profile][num] = fTmp * 4;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
            eveaddvalue[profile][num] = fTmp * 127;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
            eveaddvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
            eveaddvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
            eveaddvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
            eveaddvalue[profile][num] = iTmp;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  // Defense is backwards
            eveaddvalue[profile][num] = -iTmp;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
            eveaddvalue[profile][num] = fTmp * 4;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
            eveaddvalue[profile][num] = fTmp * 4;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
            eveaddvalue[profile][num] = fTmp * 4;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
            eveaddvalue[profile][num] = fTmp * 4;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
            eveaddvalue[profile][num] = fTmp * 4;
            num++;
        goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
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
        while(goto_colon_yesno(fileread))
        {
            idsz = get_idsz(fileread);
            fscanf(fileread, "%c%d", &cTmp, &iTmp);
            test = ('A'-'A'<<15)|('M'-'A'<<10)|('O'-'A'<<5)|('U'-'A');  // [AMOU]
            if(idsz == test)  evecontspawnamount[profile] = iTmp;
            test = ('T'-'A'<<15)|('Y'-'A'<<10)|('P'-'A'<<5)|('E'-'A');  // [TYPE]
            if(idsz == test)  evecontspawnpip[profile] = iTmp;
            test = ('T'-'A'<<15)|('I'-'A'<<10)|('M'-'A'<<5)|('E'-'A');  // [TIME]
            if(idsz == test)  evecontspawntime[profile] = iTmp;
            test = ('F'-'A'<<15)|('A'-'A'<<10)|('C'-'A'<<5)|('E'-'A');  // [FACE]
            if(idsz == test)  evecontspawnfacingadd[profile] = iTmp;
            test = ('S'-'A'<<15)|('E'-'A'<<10)|('N'-'A'<<5)|('D'-'A');  // [SEND]
            if(idsz == test)
            {
				//TODO
				//This is wrong, it gets stored or loaded incorrectly (Loaded in game.c)
				if(iTmp >= 0 && iTmp < MAXWAVE) evewaveindex[profile] = iTmp;
            }
            test = ('S'-'A'<<15)|('F'-'A'<<10)|('R'-'A'<<5)|('Q'-'A');  // [SFRQ]
            if(idsz == test)  evefrequency[profile] = iTmp;	//OUTDATED??
            test = ('S'-'A'<<15)|('T'-'A'<<10)|('A'-'A'<<5)|('Y'-'A');  // [STAY]
            if(idsz == test)  evestayifnoowner[profile] = iTmp;
            test = ('O'-'A'<<15)|('V'-'A'<<10)|('E'-'A'<<5)|('R'-'A');  // [OVER]
            if(idsz == test)  eveoverlay[profile] = iTmp;
        }


        // All done ( finally )
        fclose(fileread);
    }
}

//--------------------------------------------------------------------------------------------
unsigned short get_free_enchant()
{
    // ZZ> This function returns the next free enchantment or MAXENCHANT if there are none
    if(numfreeenchant > 0)
    {
        numfreeenchant--;
        return freeenchant[numfreeenchant];
    }
    return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void unset_enchant_value(unsigned short enchantindex, unsigned char valueindex)
{
    // ZZ> This function unsets a set value
    unsigned short character;

    if(encsetyesno[enchantindex][valueindex]==btrue)
    {
        character = enctarget[enchantindex];
        switch(valueindex)
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
                change_character(character, chrbasemodel[character], encsetsave[enchantindex][valueindex], LEAVEALL);
                break;
            case SETCHANNEL:
                chrcanchannel[character] = encsetsave[enchantindex][valueindex];
                break;
        }
        encsetyesno[enchantindex][valueindex] = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
void remove_enchant_value(unsigned short enchantindex, unsigned char valueindex)
{
    // ZZ> This function undoes cumulative modification to character stats
    float fvaluetoadd;
    int valuetoadd;

    unsigned short character = enctarget[enchantindex];
    switch(valueindex)
    {
        case ADDJUMPPOWER:
            fvaluetoadd = encaddsave[enchantindex][valueindex]/16.0;
            chrjump[character]-=fvaluetoadd;
            break;
        case ADDBUMPDAMPEN:
            fvaluetoadd = encaddsave[enchantindex][valueindex]/128.0;
            chrbumpdampen[character]-=fvaluetoadd;
            break;
        case ADDBOUNCINESS:
            fvaluetoadd = encaddsave[enchantindex][valueindex]/128.0;
            chrdampen[character]-=fvaluetoadd;
            break;
        case ADDDAMAGE:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrdamageboost[character]-=valuetoadd;
            break;
        case ADDSIZE:
            fvaluetoadd = encaddsave[enchantindex][valueindex]/128.0;
            chrsizegoto[character]-=fvaluetoadd;
            chrsizegototime[character] = SIZETIME;
            break;
        case ADDACCEL:
            fvaluetoadd = encaddsave[enchantindex][valueindex]/1000.0;
            chrmaxaccel[character]-=fvaluetoadd;
            break;
        case ADDRED:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrredshift[character]-=valuetoadd;
            break;
        case ADDGRN:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrgrnshift[character]-=valuetoadd;
            break;
        case ADDBLU:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrblushift[character]-=valuetoadd;
            break;
        case ADDDEFENSE:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrdefense[character]-=valuetoadd;
            break;
        case ADDMANA:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrmanamax[character]-=valuetoadd;
            chrmana[character]-=valuetoadd;
            if(chrmana[character] < 0) chrmana[character] = 0;
            break;
        case ADDLIFE:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrlifemax[character]-=valuetoadd;
            chrlife[character]-=valuetoadd;
            if(chrlife[character] < 1) chrlife[character] = 1;
            break;
        case ADDSTRENGTH:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrstrength[character]-=valuetoadd;
            break;
        case ADDWISDOM:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrwisdom[character]-=valuetoadd;
            break;
        case ADDINTELLIGENCE:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrintelligence[character]-=valuetoadd;
            break;
        case ADDDEXTERITY:
            valuetoadd = encaddsave[enchantindex][valueindex];
            chrdexterity[character]-=valuetoadd;
            break;
    }
}
