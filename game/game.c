/* Egoboo - game.c
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

//#define MENU_DEMO		// Uncomment this to build just the menu demo
#define DECLARE_GLOBALS
#include "egoboo.h"
#include "Clock.h"
#include "Link.h"
#include "Ui.h"
#include "Font.h"
#include "Log.h"
#include <SDL_endian.h>

#define INITGUID
#define NAME "Boo"
#define TITLE "Boo"

#define RELEASE(x) if (x) {x->Release(); x=NULL;}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
int what_action(char cTmp)
{
    // ZZ> This function changes a letter into an action code
    int action;
    action = ACTIONDA;
    if(cTmp=='U' || cTmp=='u')  action = ACTIONUA;
    if(cTmp=='T' || cTmp=='t')  action = ACTIONTA;
    if(cTmp=='S' || cTmp=='s')  action = ACTIONSA;
    if(cTmp=='C' || cTmp=='c')  action = ACTIONCA;
    if(cTmp=='B' || cTmp=='b')  action = ACTIONBA;
    if(cTmp=='L' || cTmp=='l')  action = ACTIONLA;
    if(cTmp=='X' || cTmp=='x')  action = ACTIONXA;
    if(cTmp=='F' || cTmp=='f')  action = ACTIONFA;
    if(cTmp=='P' || cTmp=='p')  action = ACTIONPA;
    if(cTmp=='Z' || cTmp=='z')  action = ACTIONZA;
    return action;
}

//--------------------------------------------------------------------------------------------
//OBSOLETE
/*void general_error(int a, int b, char *szerrortext)
{
    // ZZ> This function displays an error message
    // Steinbach's Guideline for Systems Programming:
    //   Never test for an error condition you don't know how to handle.
    char                buf[256];
    FILE*               filewrite;
    sprintf(buf, "%d, %d... %s\n", 0, 0, szerrortext);

    fprintf(stderr,"ERROR: %s\n",szerrortext);

    filewrite = fopen("errorlog.txt", "w");
    if(filewrite)
    {
        fprintf(filewrite, "I'M MELTING\n");
        fprintf(filewrite, "%d, %d... %s\n", a, b, szerrortext);
        fclose(filewrite);
    }
    release_module();
    close_session();

    release_grfx();
    fclose(globalnetworkerr);
    //DestroyWindow(hWnd);

    SDL_Quit ();
    exit(0);
}*/

//------------------------------------------------------------------------------
//Random Things-----------------------------------------------------------------
//------------------------------------------------------------------------------
void make_newloadname(char *modname, char *appendname, char *newloadname)
{
    // ZZ> This function takes some names and puts 'em together
    int cnt, tnc;
    char ctmp;

    cnt = 0;
    ctmp = modname[cnt];
    while(ctmp != 0)
    {
        newloadname[cnt] = ctmp;
        cnt++;
        ctmp = modname[cnt];
    }
    tnc = 0;
    ctmp = appendname[tnc];
    while(ctmp != 0)
    {
        newloadname[cnt] = ctmp;
        cnt++;
        tnc++;
        ctmp = appendname[tnc];
    }
    newloadname[cnt] = 0;
}

//--------------------------------------------------------------------------------------------
void load_global_waves(char *modname)
{
    // ZZ> This function loads the global waves
    char tmploadname[256];
    char newloadname[256];
    char wavename[256];
    int cnt;


    make_newloadname(modname, ("gamedat/"), tmploadname);
    cnt = 0;
    while(cnt < MAXWAVE)
    {
        sprintf(wavename, "sound%d.wav", cnt);
        make_newloadname(tmploadname, wavename, newloadname);
        globalwave[cnt] = Mix_LoadWAV(newloadname);
        cnt++;
    }

	//These sounds are always standard
	if(soundvalid)
	{
		globalwave[0] = Mix_LoadWAV("basicdat/globalparticles/coinget.wav");
		globalwave[1] = Mix_LoadWAV("basicdat/globalparticles/defend.wav");
		globalwave[4] = Mix_LoadWAV("basicdat/globalparticles/coinfall.wav");
	}


	/*	The Global Sounds
	*	0	- Pick up coin
	*	1	- Defend clank
	*	2	- Weather Effect
	*	3	- Hit Water (Splash)
	*	4	- Coin falls on ground

	//These new values todo should determine sound and particle effects
	Weather Type: DROPS, RAIN, SNOW
	Water Type: LAVA, WATER, DARK
	*/
}


////--------------------------------------------------------------------------------------------
void export_one_character(int character, int owner, int number, bool_t is_local)
{
  // ZZ> This function exports a character
  int tnc, profile;
  char letter;
  char fromdir[128];
  char todir[128];
  char fromfile[128];
  char tofile[128];
  char todirname[16];
  char todirfullname[64];

  // Don't export enchants
  disenchant_character(character);

  profile = chrmodel[character];
  if((capcancarrytonextmodule[profile] || capisitem[profile]==bfalse) && exportvalid)
  {
    // TWINK_BO.OBJ
    sprintf(todirname, "badname.obj");//"BADNAME.OBJ");
    tnc = 0;
    letter = chrname[owner][tnc];
    while(tnc < 8 && letter != 0)
    {
      letter = chrname[owner][tnc];
      if(letter >= 'A' && letter <= 'Z')  letter -= 'A' - 'a';
      if(letter != 0)
      {
        if(letter < 'a' || letter > 'z')  letter = '_';
        todirname[tnc] = letter;
        tnc++;
      }
    }
    todirname[tnc] = '.'; tnc++;
    todirname[tnc] = 'o'; tnc++;
    todirname[tnc] = 'b'; tnc++;
    todirname[tnc] = 'j'; tnc++;
    todirname[tnc] = 0;

    // Is it a character or an item?
    if(owner != character)
    {
      // Item is a subdirectory of the owner directory...
      sprintf(todirfullname, "%s/%d.obj", todirname, number);
    }
    else
    {
      // Character directory
      sprintf(todirfullname, "%s", todirname);
    }


    // players/twink.obj or players/twink.obj/sword.obj
    if(is_local)
    {
      sprintf(todir, "players/%s", todirfullname);
    }
    else
    {
      sprintf(todir, "remote/%s", todirfullname);
    }

    // modules/advent.mod/objects/advent.obj
    sprintf(fromdir, "%s", madname[profile]);

    // Delete all the old items
    if(owner == character)
    {
      tnc = 0;
      while(tnc < 8)
      {
        sprintf(tofile, "%s/%d.obj", todir, tnc);	/*.OBJ*/
        fs_removeDirectoryAndContents(tofile);
        tnc++;
      }
    }

    // Make the directory
    fs_createDirectory(todir);

    // Build the DATA.TXT file
    sprintf(tofile, "%s/data.txt", todir);	/*DATA.TXT*/
    export_one_character_profile(tofile, character);

    // Build the SKIN.TXT file
    sprintf(tofile, "%s/skin.txt", todir);	/*SKIN.TXT*/
    export_one_character_skin(tofile, character);

    // Build the NAMING.TXT file
    sprintf(tofile, "%s/naming.txt", todir);	/*NAMING.TXT*/
    export_one_character_name(tofile, character);

    // Copy all of the misc. data files
    sprintf(fromfile, "%s/message.txt", fromdir);	/*MESSAGE.TXT*/
    sprintf(tofile, "%s/message.txt", todir);	/*MESSAGE.TXT*/
    fs_copyFile(fromfile, tofile);
    sprintf(fromfile, "%s/tris.md2", fromdir);	/*TRIS.MD2*/
    sprintf(tofile,   "%s/tris.md2", todir);	/*TRIS.MD2*/
    fs_copyFile(fromfile, tofile);
    sprintf(fromfile, "%s/copy.txt", fromdir);	/*COPY.TXT*/
    sprintf(tofile,   "%s/copy.txt", todir);	/*COPY.TXT*/
    fs_copyFile(fromfile, tofile);
    sprintf(fromfile, "%s/script.txt", fromdir);
    sprintf(tofile,   "%s/script.txt", todir);
    fs_copyFile(fromfile, tofile);
    sprintf(fromfile, "%s/enchant.txt", fromdir);
    sprintf(tofile,   "%s/enchant.txt", todir);
    fs_copyFile(fromfile, tofile);
    sprintf(fromfile, "%s/credits.txt", fromdir);
    sprintf(tofile,   "%s/credits.txt", todir);
    fs_copyFile(fromfile, tofile);

    // Copy all of the particle files
    tnc = 0;
    while(tnc < MAXPRTPIPPEROBJECT)
    {
      sprintf(fromfile, "%s/part%d.txt", fromdir, tnc);
      sprintf(tofile,   "%s/part%d.txt", todir,   tnc);
      fs_copyFile(fromfile, tofile);
      tnc++;
    }

    // Copy all of the sound files
    tnc = 0;
    while(tnc < MAXWAVE)
    {
      sprintf(fromfile, "%s/sound%d.wav", fromdir, tnc);
      sprintf(tofile,   "%s/sound%d.wav", todir,   tnc);
      fs_copyFile(fromfile, tofile);
      tnc++;
    }

    // Copy all of the image files
    tnc = 0;
    while(tnc < 4)
    {
      sprintf(fromfile, "%s/tris%d.bmp", fromdir, tnc);
      sprintf(tofile,   "%s/tris%d.bmp", todir,   tnc);
      fs_copyFile(fromfile, tofile);
      sprintf(fromfile, "%s/icon%d.bmp", fromdir, tnc);
      sprintf(tofile,   "%s/icon%d.bmp", todir,   tnc);
      fs_copyFile(fromfile, tofile);
      tnc++;
    }
  }
}

//---------------------------------------------------------------------------------------------
void export_all_players(bool_t require_local)
{
  // ZZ> This function saves all the local players in the
  //     PLAYERS directory
  bool_t is_local;
  int cnt, character, item, number;

  // Check each player
  for(cnt = 0; cnt < MAXPLAYER; cnt++)
  {
    is_local = (0 != pladevice[cnt]);
    if( require_local && !is_local ) continue;
    if( !plavalid[cnt] ) continue;

    // Is it alive?
    character = plaindex[cnt];
    if(!chron[character] || !chralive[character]) continue;

    // Export the character
    export_one_character(character, character, 0, is_local);

    // Export the left hand item
    number = 0;
    item = chrholdingwhich[character][number];
    if(item != MAXCHR && chrisitem[item])  export_one_character(item, character, number, is_local);

    // Export the right hand item
    number = 1;
    item = chrholdingwhich[character][number];
    if(item != MAXCHR && chrisitem[item])  export_one_character(item, character, number, is_local);

    // Export the inventory
    number = 2;
    item = chrnextinpack[character];
    while(item != MAXCHR)
    {
      if(chrisitem[item])
      {
        export_one_character(item, character, number++, is_local);
      }

      item = chrnextinpack[item];
    }
  }

}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void export_all_local_players(void)
{
    // ZZ> This function saves all the local players in the
    //     PLAYERS directory

    // Check each player
    if(exportvalid)
    {
      export_all_players(btrue);
    }
}

//---------------------------------------------------------------------------------------------
void quit_module(void)
{
    // ZZ> This function forces a return to the menu
    moduleactive = bfalse;
    hostactive = bfalse;
    export_all_local_players();
	gamepaused = bfalse;
	if(soundvalid) Mix_FadeOutChannel(-1, 500);					//Stop all sounds that are playing
}

//--------------------------------------------------------------------------------------------
void quit_game(void)
{
	log_info("Exiting Egoboo %s the good way...\n", VERSION);
    // ZZ> This function exits the game entirely
    if(gameactive)
    {
      gameactive = bfalse;
    }
    if(moduleactive)
    {
      quit_module();
    }
    if(floatmemory != NULL)
    {
      free(floatmemory);
      floatmemory = NULL;
    }
}

/* ORIGINAL, UNOPTIMIZED VERSION
//--------------------------------------------------------------------------------------------
void goto_colon(FILE* fileread)
{
    // ZZ> This function moves a file read pointer to the next colon
    char cTmp;


    fscanf(fileread, "%c", &cTmp);
    while(cTmp != ':')
    {
        if(fscanf(fileread, "%c", &cTmp)==EOF)
        {
            if(globalname==NULL)
            {
                general_error(0, 0, "NOT ENOUGH COLONS IN FILE!!!");
            }
            else
            {
                general_error(0, 0, globalname);
            }
        }
    }
}
*/

//--------------------------------------------------------------------------------------------
void goto_colon(FILE* fileread)
{
    // ZZ> This function moves a file read pointer to the next colon
//    char cTmp;
    unsigned int ch = fgetc(fileread);

//    fscanf(fileread, "%c", &cTmp);
    while(ch != ':')
    {
		if (ch == EOF)
		{
			// not enough colons in file!
			log_error("There are not enough colons in file! (%s)\n", globalname);
		}

		ch = fgetc(fileread);
    }
}

//--------------------------------------------------------------------------------------------
unsigned char goto_colon_yesno(FILE* fileread)
{
    // ZZ> This function moves a file read pointer to the next colon, or it returns
    //     bfalse if there are no more
    char cTmp;

    do
    {
        if(fscanf(fileread, "%c", &cTmp)==EOF)
        {
            return bfalse;
        }
    }
    while(cTmp != ':');

    return btrue;
}

//--------------------------------------------------------------------------------------------
char get_first_letter(FILE* fileread)
{
    // ZZ> This function returns the next non-whitespace character
    char cTmp;
    fscanf(fileread, "%c", &cTmp);
    while(isspace(cTmp))
    {
        fscanf(fileread, "%c", &cTmp);
    }
    return cTmp;
}

//--------------------------------------------------------------------------------------------
//Tag Reading---------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void reset_tags()
{
    // ZZ> This function resets the tags
    numscantag = 0;
}

//--------------------------------------------------------------------------------------------
int read_tag(FILE *fileread)
{
    // ZZ> This function finds the next tag, returning btrue if it found one
    if(goto_colon_yesno(fileread))
    {
        if(numscantag < MAXTAG)
        {
            fscanf(fileread, "%s%d", tagname[numscantag], &tagvalue[numscantag]);
            numscantag++;
            return btrue;
        }
    }
    return bfalse;
}

//--------------------------------------------------------------------------------------------
void read_all_tags(char *szFilename)
{
    // ZZ> This function reads the scancode.txt file
    FILE* fileread;


    reset_tags();
    fileread = fopen(szFilename, "r");
    if(fileread)
    {
        while(read_tag(fileread));
        fclose(fileread);
    }
}

//--------------------------------------------------------------------------------------------
int tag_value(char *string)
{
    // ZZ> This function matches the string with its tag, and returns the value...
    //     It will return 255 if there are no matches.
    int cnt;

    cnt = 0;
    while(cnt < numscantag)
    {
        if(strcmp(string, tagname[cnt])==0)
        {
            // They match
            return tagvalue[cnt];
        }
        cnt++;
    }
    // No matches
    return 255;
}

//--------------------------------------------------------------------------------------------
void read_controls(char *szFilename)
{
    // ZZ> This function reads the controls.txt file
    FILE* fileread;
    char currenttag[TAGSIZE];
    int cnt;


    fileread = fopen(szFilename, "r");
    if(fileread)
    {
        cnt = 0;
        while(goto_colon_yesno(fileread) && cnt < MAXCONTROL)
        {
            fscanf(fileread, "%s", currenttag);
            controlvalue[cnt] = tag_value(currenttag);
            //printf("CTRL: %i, %s\n", controlvalue[cnt], currenttag);
            controliskey[cnt] = (currenttag[0] == 'K');
            cnt++;
        }
        fclose(fileread);
    }
}

//--------------------------------------------------------------------------------------------
unsigned char control_key_is_pressed(unsigned char control)
{
    // ZZ> This function returns btrue if the given control is pressed...
    if(netmessagemode)  return bfalse;

	if (sdlkeybuffer)
	    return (sdlkeybuffer[controlvalue[control]]!=0);
	else
		return bfalse;
}

//--------------------------------------------------------------------------------------------
unsigned char control_mouse_is_pressed(unsigned char control)
{
    // ZZ> This function returns btrue if the given control is pressed...
    if(controliskey[control])
    {
        if(netmessagemode)  return bfalse;

		if (sdlkeybuffer)
	        return (sdlkeybuffer[controlvalue[control]]!=0);
		else
			return bfalse;
    }
    else
    {
        return (msb==controlvalue[control]);
    }
    return bfalse;
}

//--------------------------------------------------------------------------------------------
unsigned char control_joya_is_pressed(unsigned char control)
{
    // ZZ> This function returns btrue if the given control is pressed...
    if(controliskey[control])
    {
        if(netmessagemode)  return bfalse;

		if (sdlkeybuffer)
	        return (sdlkeybuffer[controlvalue[control]]!=0);
		else
			return bfalse;
    }
    else
    {
        return (jab==controlvalue[control]);
    }
    return bfalse;
}

//--------------------------------------------------------------------------------------------
unsigned char control_joyb_is_pressed(unsigned char control)
{
    // ZZ> This function returns btrue if the given control is pressed...
    if(controliskey[control])
    {
        if(netmessagemode)  return bfalse;

        if (sdlkeybuffer)
			return (sdlkeybuffer[controlvalue[control]]!=0);
		else
			return bfalse;
    }
    else
    {
        return (jbb==controlvalue[control]);
    }
    return bfalse;
}

//--------------------------------------------------------------------------------------------
void undo_idsz(int idsz)
{
    // ZZ> This function takes an integer and makes an text IDSZ out of it.
    //     It will set valueidsz to "NONE" if the idsz is 0
    if(idsz == IDSZNONE)
    {
        sprintf(valueidsz, "NONE");
    }
    else
    {
        valueidsz[0] = ((idsz>>15)&31) + 'A';
        valueidsz[1] = ((idsz>>10)&31) + 'A';
        valueidsz[2] = ((idsz>>5)&31) + 'A';
        valueidsz[3] = ((idsz)&31) + 'A';
        valueidsz[4] = 0;
    }
    return;
}

//--------------------------------------------------------------------------------------------
int get_idsz(FILE* fileread)
{
    // ZZ> This function reads and returns an IDSZ tag, or IDSZNONE if there wasn't one
    int test;

    int idsz = IDSZNONE;
    char cTmp = get_first_letter(fileread);
    if(cTmp == '[')
    {
        fscanf(fileread, "%c", &cTmp);  cTmp = cTmp-'A';  idsz=idsz|(cTmp<<15);
        fscanf(fileread, "%c", &cTmp);  cTmp = cTmp-'A';  idsz=idsz|(cTmp<<10);
        fscanf(fileread, "%c", &cTmp);  cTmp = cTmp-'A';  idsz=idsz|(cTmp<<5);
        fscanf(fileread, "%c", &cTmp);  cTmp = cTmp-'A';  idsz=idsz|(cTmp);
    }

    test = ('N'-'A'<<15)|('O'-'A'<<10)|('N'-'A'<<5)|('E'-'A');  // [NONE]

    if(idsz == test)
        idsz = IDSZNONE;

    return idsz;
}

//--------------------------------------------------------------------------------------------
int get_free_message(void)
{
    // This function finds the best message to use
    // Pick the first one
    int tnc = msgstart;
    msgstart++;
    msgstart = msgstart % maxmessage;
    return tnc;
}

//--------------------------------------------------------------------------------------------
void display_message(int message, unsigned short character)
{
    // ZZ> This function sticks a message in the display queue and sets its timer
    int slot, read, write, cnt;
    char *eread;
    char szTmp[256];
    char cTmp, lTmp;

    unsigned short target = chraitarget[character];
    unsigned short owner = chraiowner[character];
    if (message < msgtotal)
    {
        slot = get_free_message();
        msgtime[slot] = MESSAGETIME;
        // Copy the message
        read = msgindex[message];
        cnt=0;
        write = 0;
        cTmp = msgtext[read];  read++;
        while(cTmp != 0)
        {
            if(cTmp == '%')
            {
                // Escape sequence
                eread = szTmp;
                szTmp[0] = 0;
                cTmp = msgtext[read];  read++;
                if(cTmp == 'n')  // Name
                {
                    if(chrnameknown[character])
                        sprintf(szTmp, "%s", chrname[character]);
                    else
                    {
                        lTmp = capclassname[chrmodel[character]][0];
                        if(lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
                            sprintf(szTmp, "an %s", capclassname[chrmodel[character]]);
                        else
                            sprintf(szTmp, "a %s", capclassname[chrmodel[character]]);
                    }
                    if(cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
                }
                if(cTmp == 'c')  // Class name
                {
                    eread = capclassname[chrmodel[character]];
                }
                if(cTmp == 't')  // Target name
                {
                    if(chrnameknown[target])
                        sprintf(szTmp, "%s", chrname[target]);
                    else
                    {
                        lTmp = capclassname[chrmodel[target]][0];
                        if(lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
                            sprintf(szTmp, "an %s", capclassname[chrmodel[target]]);
                        else
                            sprintf(szTmp, "a %s", capclassname[chrmodel[target]]);
                    }
                    if(cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
                }
                if(cTmp == 'o')  // Owner name
                {
                    if(chrnameknown[owner])
                        sprintf(szTmp, "%s", chrname[owner]);
                    else
                    {
                        lTmp = capclassname[chrmodel[owner]][0];
                        if(lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
                            sprintf(szTmp, "an %s", capclassname[chrmodel[owner]]);
                        else
                            sprintf(szTmp, "a %s", capclassname[chrmodel[owner]]);
                    }
                    if(cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
                }
                if(cTmp == 's')  // Target class name
                {
                    eread = capclassname[chrmodel[target]];
                }
                if(cTmp >= '0' && cTmp <= '3')  // Target's skin name
                {
                    eread = capskinname[chrmodel[target]][cTmp-'0'];
                }
                if(cTmp == 'd')  // tmpdistance value
                {
                    sprintf(szTmp, "%d", valuetmpdistance);
                }
                if(cTmp == 'x')  // tmpx value
                {
                    sprintf(szTmp, "%d", valuetmpx);
                }
                if(cTmp == 'y')  // tmpy value
                {
                    sprintf(szTmp, "%d", valuetmpy);
                }
                if(cTmp == 'D')  // tmpdistance value
                {
                    sprintf(szTmp, "%2d", valuetmpdistance);
                }
                if(cTmp == 'X')  // tmpx value
                {
                    sprintf(szTmp, "%2d", valuetmpx);
                }
                if(cTmp == 'Y')  // tmpy value
                {
                    sprintf(szTmp, "%2d", valuetmpy);
                }
                if(cTmp == 'a')  // Character's ammo
                {
                    if(chrammoknown[character])
                        sprintf(szTmp, "%d", chrammo[character]);
                    else
                        sprintf(szTmp, "?");
                }
                if(cTmp == 'k')  // Kurse state
                {
                    if(chriskursed[character])
                        sprintf(szTmp, "kursed");
                    else
                        sprintf(szTmp, "unkursed");
                }
                if(cTmp == 'p')  // Character's possessive
                {
                    if(chrgender[character] == GENFEMALE)
                    {
                        sprintf(szTmp, "her");
                    }
                    else
                    {
                        if(chrgender[character] == GENMALE)
                        {
                            sprintf(szTmp, "his");
                        }
                        else
                        {
                            sprintf(szTmp, "its");
                        }
                    }
                }
                if(cTmp == 'm')  // Character's gender
                {
                    if(chrgender[character] == GENFEMALE)
                    {
                        sprintf(szTmp, "female ");
                    }
                    else
                    {
                        if(chrgender[character] == GENMALE)
                        {
                            sprintf(szTmp, "male ");
                        }
                        else
                        {
                            sprintf(szTmp, " ");
                        }
                    }
                }
                if(cTmp == 'g')  // Target's possessive
                {
                    if(chrgender[target] == GENFEMALE)
                    {
                        sprintf(szTmp, "her");
                    }
                    else
                    {
                        if(chrgender[target] == GENMALE)
                        {
                            sprintf(szTmp, "his");
                        }
                        else
                        {
                            sprintf(szTmp, "its");
                        }
                    }
                }
                cTmp = *eread;  eread++;
                while(cTmp != 0 && write < MESSAGESIZE-1)
                {
                    msgtextdisplay[slot][write] = cTmp;
                    cTmp = *eread;  eread++;
                    write++;
                }
            }
            else
            {
                // Copy the letter
                if(write < MESSAGESIZE-1)
                {
                    msgtextdisplay[slot][write] = cTmp;
                    write++;
                }
            }
            cTmp = msgtext[read];  read++;
            cnt++;
        }
        msgtextdisplay[slot][write] = 0;
    }
}

//--------------------------------------------------------------------------------------------
void remove_enchant(unsigned short enchantindex)
{
    // ZZ> This function removes a specific enchantment and adds it to the unused list
    unsigned short character, overlay;
    unsigned short lastenchant, currentenchant;
    int add;


    if(enchantindex < MAXENCHANT)
    {
        if(encon[enchantindex])
        {
            // Unsparkle the spellbook
            character = encspawner[enchantindex];
            if(character < MAXCHR)
            {
                chrsparkle[character] = NOSPARKLE;
                // Make the spawner unable to undo the enchantment
                if(chrundoenchant[character] == enchantindex)
                {
                    chrundoenchant[character] = MAXENCHANT;
                }
            }


            // Play the end sound
            character = enctarget[enchantindex];
			//if(eveawveindex[enchantindex] > 0) play_sound(chroldx[character], chroldy[character], capwaveindex[chrmodel[encspawner[enchantindex]]][evewaveindex[enchantindex]);


            // Unset enchant values, doing morph last
            unset_enchant_value(enchantindex, SETDAMAGETYPE);
            unset_enchant_value(enchantindex, SETNUMBEROFJUMPS);
            unset_enchant_value(enchantindex, SETLIFEBARCOLOR);
            unset_enchant_value(enchantindex, SETMANABARCOLOR);
            unset_enchant_value(enchantindex, SETSLASHMODIFIER);
            unset_enchant_value(enchantindex, SETCRUSHMODIFIER);
            unset_enchant_value(enchantindex, SETPOKEMODIFIER);
            unset_enchant_value(enchantindex, SETHOLYMODIFIER);
            unset_enchant_value(enchantindex, SETEVILMODIFIER);
            unset_enchant_value(enchantindex, SETFIREMODIFIER);
            unset_enchant_value(enchantindex, SETICEMODIFIER);
            unset_enchant_value(enchantindex, SETZAPMODIFIER);
            unset_enchant_value(enchantindex, SETFLASHINGAND);
            unset_enchant_value(enchantindex, SETLIGHTBLEND);
            unset_enchant_value(enchantindex, SETALPHABLEND);
            unset_enchant_value(enchantindex, SETSHEEN);
            unset_enchant_value(enchantindex, SETFLYTOHEIGHT);
            unset_enchant_value(enchantindex, SETWALKONWATER);
            unset_enchant_value(enchantindex, SETCANSEEINVISIBLE);
            unset_enchant_value(enchantindex, SETMISSILETREATMENT);
            unset_enchant_value(enchantindex, SETCOSTFOREACHMISSILE);
            unset_enchant_value(enchantindex, SETCHANNEL);
            unset_enchant_value(enchantindex, SETMORPH);


            // Remove all of the cumulative values
            add = 0;
            while(add < MAXEVEADDVALUE)
            {
                remove_enchant_value(enchantindex, add);
                add++;
            }


            // Unlink it
            if(chrfirstenchant[character] == enchantindex)
            {
                // It was the first in the list
                chrfirstenchant[character] = encnextenchant[enchantindex];
            }
            else
            {
                // Search until we find it
                currentenchant = chrfirstenchant[character];
                while(currentenchant != enchantindex)
                {
                    lastenchant = currentenchant;
                    currentenchant = encnextenchant[currentenchant];
                }
                // Relink the last enchantment
                encnextenchant[lastenchant] = encnextenchant[enchantindex];
            }



            // See if we spit out an end message
            if(eveendmessage[enceve[enchantindex]] >= 0)
            {
                display_message(madmsgstart[enceve[enchantindex]]+eveendmessage[enceve[enchantindex]], enctarget[enchantindex]);
            }
            // Check to see if we spawn a poof
            if(evepoofonend[enceve[enchantindex]])
            {
                spawn_poof(enctarget[enchantindex], enceve[enchantindex]);
            }
            // Check to see if the character dies
            if(evekillonend[enceve[enchantindex]])
            {
                if(chrinvictus[character])  teammorale[chrbaseteam[character]]++;
                chrinvictus[character] = bfalse;
                kill_character(character, MAXCHR);
            }
            // Kill overlay too...
            overlay = encoverlay[enchantindex];
            if(overlay<MAXCHR)
            {
                if(chrinvictus[overlay])  teammorale[chrbaseteam[overlay]]++;
                chrinvictus[overlay] = bfalse;
                kill_character(overlay, MAXCHR);
            }

            // Remove see kurse enchant (BAD: no check if it is a natural abillity?)
            if(eveseekurse[enceve[enchantindex]] && !capcanseekurse[chrmodel[character]])
            {
                chrcanseekurse[character] = bfalse;
            }





            // Now get rid of it
            encon[enchantindex] = bfalse;
            freeenchant[numfreeenchant] = enchantindex;
            numfreeenchant++;


            // Now fix dem weapons
            reset_character_alpha(chrholdingwhich[character][0]);
            reset_character_alpha(chrholdingwhich[character][1]);
        }
    }
}

//--------------------------------------------------------------------------------------------
unsigned short enchant_value_filled(unsigned short enchantindex, unsigned char valueindex)
{
    // ZZ> This function returns MAXENCHANT if the enchantment's target has no conflicting
    //     set values in its other enchantments.  Otherwise it returns the enchantindex
    //     of the conflicting enchantment
    unsigned short character, currenchant;

    character = enctarget[enchantindex];
    currenchant = chrfirstenchant[character];
    while(currenchant != MAXENCHANT)
    {
        if(encsetyesno[currenchant][valueindex]==btrue)
        {
            return currenchant;
        }
        currenchant = encnextenchant[currenchant];
    }
    return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void set_enchant_value(unsigned short enchantindex, unsigned char valueindex,
    unsigned short enchanttype)
{
    // ZZ> This function sets and saves one of the character's stats
    unsigned short conflict, character;


    encsetyesno[enchantindex][valueindex] = bfalse;
    if(evesetyesno[enchanttype][valueindex])
    {
        conflict = enchant_value_filled(enchantindex, valueindex);
        if(conflict == MAXENCHANT || eveoverride[enchanttype])
        {
            // Check for multiple enchantments
            if(conflict < MAXENCHANT)
            {
                // Multiple enchantments aren't allowed for sets
                if(everemoveoverridden[enchanttype])
                {
                    // Kill the old enchantment
                    remove_enchant(conflict);
                }
                else
                {
                    // Just unset the old enchantment's value
                    unset_enchant_value(conflict, valueindex);
                }
            }
            // Set the value, and save the character's real stat
            character = enctarget[enchantindex];
            encsetyesno[enchantindex][valueindex] = btrue;
            switch(valueindex)
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
                    if(chrflyheight[character]==0 && chrzpos[character] > -2)
                    {
                        chrflyheight[character] = evesetvalue[enchanttype][valueindex];
                    }
                    break;
                case SETWALKONWATER:
                    encsetsave[enchantindex][valueindex] = chrwaterwalk[character];
                    if(chrwaterwalk[character]==bfalse)
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
                    encsetsave[enchantindex][valueindex] = chrtexture[character] - madskinstart[chrmodel[character]];
                    // Special handler for morph
                    change_character(character, enchanttype, 0, LEAVEALL); // LEAVEFIRST);
                    chralert[character]|=ALERTIFCHANGED;
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
void getadd(int min, int value, int max, int* valuetoadd)
{
    // ZZ> This function figures out what value to add should be in order
    //     to not overflow the min and max bounds
    int newvalue;

    newvalue = value+(*valuetoadd);
    if(newvalue < min)
    {
        // Increase valuetoadd to fit
        *valuetoadd = min-value;
        if(*valuetoadd > 0)  *valuetoadd=0;
        return;
    }


    if(newvalue > max)
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max-value;
        if(*valuetoadd < 0)  *valuetoadd=0;
    }
}

//--------------------------------------------------------------------------------------------
void fgetadd(float min, float value, float max, float* valuetoadd)
{
    // ZZ> This function figures out what value to add should be in order
    //     to not overflow the min and max bounds
    float newvalue;


    newvalue = value+(*valuetoadd);
    if(newvalue < min)
    {
        // Increase valuetoadd to fit
        *valuetoadd = min-value;
        if(*valuetoadd > 0)  *valuetoadd=0;
        return;
    }


    if(newvalue > max)
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max-value;
        if(*valuetoadd < 0)  *valuetoadd=0;
    }
}

//--------------------------------------------------------------------------------------------
void add_enchant_value(unsigned short enchantindex, unsigned char valueindex,
    unsigned short enchanttype)
{
    // ZZ> This function does cumulative modification to character stats
    int valuetoadd, newvalue;
    float fvaluetoadd, fnewvalue;
    unsigned short character;


    character = enctarget[enchantindex];
    switch(valueindex)
    {
        case ADDJUMPPOWER:
            fnewvalue = chrjump[character];
            fvaluetoadd = eveaddvalue[enchanttype][valueindex]/16.0;
            fgetadd(0, fnewvalue, 30.0, &fvaluetoadd);
            valuetoadd = fvaluetoadd*16.0; // Get save value
            fvaluetoadd = valuetoadd/16.0;
            chrjump[character]+=fvaluetoadd;
            break;
        case ADDBUMPDAMPEN:
            fnewvalue = chrbumpdampen[character];
            fvaluetoadd = eveaddvalue[enchanttype][valueindex]/128.0;
            fgetadd(0, fnewvalue, 1.0, &fvaluetoadd);
            valuetoadd = fvaluetoadd*128.0; // Get save value
            fvaluetoadd = valuetoadd/128.0;
            chrbumpdampen[character]+=fvaluetoadd;
            break;
        case ADDBOUNCINESS:
            fnewvalue = chrdampen[character];
            fvaluetoadd = eveaddvalue[enchanttype][valueindex]/128.0;
            fgetadd(0, fnewvalue, 0.95, &fvaluetoadd);
            valuetoadd = fvaluetoadd*128.0; // Get save value
            fvaluetoadd = valuetoadd/128.0;
            chrdampen[character]+=fvaluetoadd;
            break;
        case ADDDAMAGE:
            newvalue = chrdamageboost[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd(0, newvalue, 4096, &valuetoadd);
            chrdamageboost[character]+=valuetoadd;
            break;
        case ADDSIZE:
            fnewvalue = chrsizegoto[character];
            fvaluetoadd = eveaddvalue[enchanttype][valueindex]/128.0;
            fgetadd(0.5, fnewvalue, 2.0, &fvaluetoadd);
            valuetoadd = fvaluetoadd*128.0; // Get save value
            fvaluetoadd = valuetoadd/128.0;
            chrsizegoto[character]+=fvaluetoadd;
            chrsizegototime[character] = SIZETIME;
            break;
        case ADDACCEL:
            fnewvalue = chrmaxaccel[character];
            fvaluetoadd = eveaddvalue[enchanttype][valueindex]/80.0;
            fgetadd(0, fnewvalue, 1.5, &fvaluetoadd);
            valuetoadd = fvaluetoadd*1000.0; // Get save value
            fvaluetoadd = valuetoadd/1000.0;
            chrmaxaccel[character]+=fvaluetoadd;
            break;
        case ADDRED:
            newvalue = chrredshift[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex];
            getadd(0, newvalue, 6, &valuetoadd);
            chrredshift[character]+=valuetoadd;
            break;
        case ADDGRN:
            newvalue = chrgrnshift[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex];
            getadd(0, newvalue, 6, &valuetoadd);
            chrgrnshift[character]+=valuetoadd;
            break;
        case ADDBLU:
            newvalue = chrblushift[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex];
            getadd(0, newvalue, 6, &valuetoadd);
            chrblushift[character]+=valuetoadd;
            break;
        case ADDDEFENSE:
            newvalue = chrdefense[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex];
            getadd(55, newvalue, 255, &valuetoadd);  // Don't fix again!
            chrdefense[character]+=valuetoadd;
            break;
        case ADDMANA:
            newvalue = chrmanamax[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd(0, newvalue, PERFECTBIG, &valuetoadd);
            chrmanamax[character]+=valuetoadd;
            chrmana[character]+=valuetoadd;
            if(chrmana[character] < 0)  chrmana[character] = 0;
            break;
        case ADDLIFE:
            newvalue = chrlifemax[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd(LOWSTAT, newvalue, PERFECTBIG, &valuetoadd);
            chrlifemax[character]+=valuetoadd;
            chrlife[character]+=valuetoadd;
            if(chrlife[character] < 1)  chrlife[character] = 1;
            break;
        case ADDSTRENGTH:
            newvalue = chrstrength[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd(0, newvalue, HIGHSTAT, &valuetoadd);
            chrstrength[character]+=valuetoadd;
            break;
        case ADDWISDOM:
            newvalue = chrwisdom[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd(0, newvalue, HIGHSTAT, &valuetoadd);
            chrwisdom[character]+=valuetoadd;
            break;
        case ADDINTELLIGENCE:
            newvalue = chrintelligence[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd(0, newvalue, HIGHSTAT, &valuetoadd);
            chrintelligence[character]+=valuetoadd;
            break;
        case ADDDEXTERITY:
            newvalue = chrdexterity[character];
            valuetoadd = eveaddvalue[enchanttype][valueindex] << 6;
            getadd(0, newvalue, HIGHSTAT, &valuetoadd);
            chrdexterity[character]+=valuetoadd;
            break;
    }
    encaddsave[enchantindex][valueindex] = valuetoadd;  // Save the value for undo
}


//--------------------------------------------------------------------------------------------
unsigned short spawn_enchant(unsigned short owner, unsigned short target,
    unsigned short spawner, unsigned short enchantindex, unsigned short modeloptional)
{
    // ZZ> This function enchants a target, returning the enchantment index or MAXENCHANT
    //     if failed
    unsigned short enchanttype, overlay;
    int add;


    if(modeloptional < MAXMODEL)
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
    if(target < MAXCHR)
    {
        if(!chron[target] || !chralive[target])
           return MAXENCHANT;
    }
    else
    {
        // Invalid target
        return MAXENCHANT;
    }
    if(owner < MAXCHR)
    {
        if(!chron[owner] || !chralive[owner])
           return MAXENCHANT;
    }
    else
    {
        // Invalid target
        return MAXENCHANT;
    }


    if(evevalid[enchanttype])
    {
        if(enchantindex == MAXENCHANT)
        {
            // Should it choose an inhand item?
            if(everetarget[enchanttype])
            {
                // Is at least one valid?
                if(chrholdingwhich[target][0] == MAXCHR && chrholdingwhich[target][1] == MAXCHR)
                {
                    // No weapons to pick
                    return MAXENCHANT;
                }
                // Left, right, or both are valid
                if(chrholdingwhich[target][0] == MAXCHR)
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
            if(evedontdamagetype[enchanttype] != DAMAGENULL)
            {
                if((chrdamagemodifier[target][evedontdamagetype[enchanttype]]&7)>=3)  // Invert | Shift = 7
                {
                    return MAXENCHANT;
                }
            }
            if(eveonlydamagetype[enchanttype] != DAMAGENULL)
            {
                if(chrdamagetargettype[target] != eveonlydamagetype[enchanttype])
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
        if(enchantindex < MAXENCHANT)
        {
            // Make a new one
            encon[enchantindex] = btrue;
            enctarget[enchantindex] = target;
            encowner[enchantindex] = owner;
            encspawner[enchantindex] = spawner;
            if(spawner < MAXCHR)
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
            set_enchant_value(enchantindex, SETMORPH, enchanttype);
            set_enchant_value(enchantindex, SETDAMAGETYPE, enchanttype);
            set_enchant_value(enchantindex, SETNUMBEROFJUMPS, enchanttype);
            set_enchant_value(enchantindex, SETLIFEBARCOLOR, enchanttype);
            set_enchant_value(enchantindex, SETMANABARCOLOR, enchanttype);
            set_enchant_value(enchantindex, SETSLASHMODIFIER, enchanttype);
            set_enchant_value(enchantindex, SETCRUSHMODIFIER, enchanttype);
            set_enchant_value(enchantindex, SETPOKEMODIFIER, enchanttype);
            set_enchant_value(enchantindex, SETHOLYMODIFIER, enchanttype);
            set_enchant_value(enchantindex, SETEVILMODIFIER, enchanttype);
            set_enchant_value(enchantindex, SETFIREMODIFIER, enchanttype);
            set_enchant_value(enchantindex, SETICEMODIFIER, enchanttype);
            set_enchant_value(enchantindex, SETZAPMODIFIER, enchanttype);
            set_enchant_value(enchantindex, SETFLASHINGAND, enchanttype);
            set_enchant_value(enchantindex, SETLIGHTBLEND, enchanttype);
            set_enchant_value(enchantindex, SETALPHABLEND, enchanttype);
            set_enchant_value(enchantindex, SETSHEEN, enchanttype);
            set_enchant_value(enchantindex, SETFLYTOHEIGHT, enchanttype);
            set_enchant_value(enchantindex, SETWALKONWATER, enchanttype);
            set_enchant_value(enchantindex, SETCANSEEINVISIBLE, enchanttype);
            set_enchant_value(enchantindex, SETMISSILETREATMENT, enchanttype);
            set_enchant_value(enchantindex, SETCOSTFOREACHMISSILE, enchanttype);
            set_enchant_value(enchantindex, SETCHANNEL, enchanttype);


            // Now do all of the stat adds
            add = 0;
            while(add < MAXEVEADDVALUE)
            {
                add_enchant_value(enchantindex, add, enchanttype);
                add++;
            }


            // Create an overlay character?
            encoverlay[enchantindex] = MAXCHR;
            if(eveoverlay[enchanttype])
            {
                overlay = spawn_one_character(chrxpos[target], chrypos[target], chrzpos[target],
                                              enchanttype, chrteam[target], 0, chrturnleftright[target],
                                              NULL, MAXCHR);
                if(overlay < MAXCHR)
                {
                    encoverlay[enchantindex] = overlay;  // Kill this character on end...
                    chraitarget[overlay] = target;
                    chraistate[overlay] = eveoverlay[enchanttype];
                    chroverlay[overlay] = btrue;


                    // Start out with ActionMJ...  Object activated
                    if(madactionvalid[chrmodel[overlay]][ACTIONMJ])
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
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void load_action_names(char* loadname)
{
    // ZZ> This function loads all of the 2 letter action names
    FILE* fileread;
    int cnt;
    char first, second;

    fileread = fopen(loadname, "r");
    if(fileread)
    {
        cnt = 0;
        while(cnt < MAXACTION)
        {
            goto_colon(fileread);
            fscanf(fileread, "%c%c", &first, &second);
            cActionName[cnt][0] = first;
            cActionName[cnt][1] = second;
            cnt++;
        }
        fclose(fileread);
    }
}

//--------------------------------------------------------------------------------------------
void get_name(FILE* fileread, char *szName)
{
    // ZZ> This function loads a string of up to MAXCAPNAMESIZE characters, parsing
    //     it for underscores.  The szName argument is rewritten with the null terminated
    //     string
    int cnt;
    char cTmp;
    char szTmp[256];


    fscanf(fileread, "%s", szTmp);
    cnt = 0;
    while(cnt < MAXCAPNAMESIZE-1)
    {
        cTmp = szTmp[cnt];
        if(cTmp=='_')  cTmp=' ';
        szName[cnt] = cTmp;
        cnt++;
    }
    szName[cnt] = 0;
}

//--------------------------------------------------------------------------------------------
void read_setup(char* filename)
	{
    // ZZ> This function loads the setup file

	ConfigFilePtr lConfigSetup;
	char lCurSectionName[64];
	bool_t lTempBool;
	Sint32 lTmpInt;
	char lTmpStr[24];


	lConfigSetup = OpenConfigFile( filename );
	if ( lConfigSetup == NULL )
		{
		//Major Error
		log_error("Could not find Setup.txt\n");
		}
	else
		{
		globalname = filename; // heu!?

		/*********************************************

		GRAPHIC Section

		*********************************************/

		strcpy( lCurSectionName, "GRAPHIC" );

		//Draw z reflection?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "Z_REFLECTION", &zreflect ) == 0 )
			{
			zreflect = bfalse; // default
			}

		//Max number of vertrices (Should always be 100!)
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "MAX_NUMBER_VERTICES", &lTmpInt ) == 0 )
			{
			lTmpInt = 100; // default
			}
		maxtotalmeshvertices = lTmpInt * 1024;

		//Do fullscreen?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "FULLSCREEN", &fullscreen ) == 0 )
			{
			fullscreen = bfalse; // default
			}

		//Screen Size
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "SCREENSIZE_X", &lTmpInt ) == 0 )
			{
			lTmpInt = 640; // default
			}
		scrx = lTmpInt;
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "SCREENSIZE_Y", &lTmpInt ) == 0 )
			{
			lTmpInt = 480; // default
			}
		scry = lTmpInt;

		//Color depth
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "COLOR_DEPTH", &lTmpInt ) == 0 )
			{
			lTmpInt = 16; // default
			}
		scrd = lTmpInt;

		//The z depth
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "Z_DEPTH", &lTmpInt ) == 0 )
			{
			lTmpInt = 16; // default
			}
		scrz = lTmpInt;

		//Max number of messages displayed
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "MAX_TEXT_MESSAGE", &lTmpInt ) == 0 )
			{
			lTmpInt = 1; // default
			}
		messageon = btrue;
        maxmessage = lTmpInt;
        if(maxmessage < 1)  { maxmessage = 1;  messageon = bfalse; }
        if(maxmessage > MAXMESSAGE)  { maxmessage = MAXMESSAGE; }

		//Show status bars? (Life, mana, character icons, etc.)
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "STATUS_BAR", &staton ) == 0 )
			{
				staton = btrue; // default
			}
		wraptolerance = 32;
		if ( staton == btrue )
			{
				wraptolerance = 90;
			}

		//Perspective correction
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "PERSPECTIVE_CORRECT", &perspective ) == 0 )
			{
			perspective = bfalse; // default
			}

		//Enable dithering? (Reduces quality but increases preformance)
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "DITHERING", &dither ) == 0 )
			{
			dither = bfalse; // default
			}

		//Reflection fadeout
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "FLOOR_REFLECTION_FADEOUT", &lTempBool ) == 0 )
			{
			lTempBool = bfalse; // default
			}
		if ( lTempBool )
			{
			reffadeor = 0;
			}
		else
			{
			reffadeor = 255;
			}

		//Draw Reflection?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "REFLECTION", &refon ) == 0 )
			{
			refon = bfalse; // default
			}

		//Draw shadows?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "SHADOWS", &shaon ) == 0 )
			{
			shaon = bfalse; // default
			}

		//Draw good shadows (BAD! Not working yet)
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "SHADOW_AS_SPRITE", &shasprite ) == 0 )
			{
			shasprite = btrue; // default
			}

		//Draw phong mapping?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "PHONG", &phongon ) == 0 )
			{
			phongon = btrue; // default
			}

		//Draw water with more layers?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "MULTI_LAYER_WATER", &twolayerwateron ) == 0 )
			{
			twolayerwateron = bfalse; // default
			}

		//TODO: This is not implemented
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "OVERLAY", &overlayvalid ) == 0 )
			{
			overlayvalid = bfalse; // default
			}

		//Allow backgrounds?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "BACKGROUND", &backgroundvalid ) == 0 )
			{
			backgroundvalid = bfalse; // default
			}

		//Enable fog?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "FOG", &fogallowed ) == 0 )
			{
			fogallowed = bfalse; // default
			}

		//Do gourad shading?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "GOURAUD_SHADING", &shading ) == 0 )
		{
		  shading = bfalse; // default
		}

		//Enable antialiasing?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "ANTIALIASING", &antialiasing ) == 0 )
		{
			antialiasing = bfalse; // default
		}

		//Do we do texture filtering?
		if ( GetConfigValue( lConfigSetup, lCurSectionName, "TEXTURE_FILTERING", lTmpStr, 24 ) == 0 )
		{
			strcpy( lTmpStr, "LINEAR" ); // default
		}

		if(lTmpStr[0] == 'L' || lTmpStr[0] == 'l')  texturefilter = 1;
		if(lTmpStr[0] == 'B' || lTmpStr[0] == 'b')  texturefilter = 2;
		if(lTmpStr[0] == 'T' || lTmpStr[0] == 't')  texturefilter = 3;
		if(lTmpStr[0] == 'A' || lTmpStr[0] == 'a')  texturefilter = 4;


		/*********************************************

		SOUND Section

		*********************************************/

		strcpy( lCurSectionName, "SOUND" );

		//Enable sound
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "SOUND", &soundvalid ) == 0 )
			{
			soundvalid = bfalse; // default
			}

		//Enable music
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "MUSIC", &musicvalid ) == 0 )
			{
			musicvalid = bfalse; // default
			}

		//Music volume
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "MUSIC_VOLUME", &musicvolume ) == 0 )
			{
			musicvolume = 50; // default
			}

		//Sound volume
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "SOUND_VOLUME", &soundvolume ) == 0 )
			{
			soundvolume = 75; // default
			}

		//Max number of sound channels playing at the same time
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "MAX_SOUND_CHANNEL", &maxsoundchannel ) == 0 )
			{
			maxsoundchannel = 16; // default
			}
		if ( maxsoundchannel < 8 ) maxsoundchannel = 8;
		if ( maxsoundchannel > 32 ) maxsoundchannel = 32;

		//The output buffer size
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "OUPUT_BUFFER_SIZE", &buffersize ) == 0 )
			{
			buffersize = 2048; // default
			}
		if ( buffersize < 512 ) buffersize = 512;
		if ( buffersize > 8196 ) buffersize = 8196;


		/*********************************************

		CONTROL Section

		*********************************************/

		strcpy( lCurSectionName, "CONTROL" );

		//Camera control mode
		if ( GetConfigValue( lConfigSetup, lCurSectionName, "AUTOTURN_CAMERA", lTmpStr, 24) == 0 )
			{
			strcpy( lTmpStr, "GOOD" ); // default
			}

		if(lTmpStr[0] == 'G' || lTmpStr[0] == 'g')  autoturncamera = 255;
		if(lTmpStr[0] == 'T' || lTmpStr[0] == 't')  autoturncamera = btrue;
		if(lTmpStr[0] == 'F' || lTmpStr[0] == 'f')  autoturncamera = bfalse;

		//[claforte] Force autoturncamera to bfalse, or else it doesn't move right.
		//autoturncamera = bfalse;



		/*********************************************

		NETWORK Section

		*********************************************/

		strcpy( lCurSectionName, "NETWORK" );

		//Enable networking systems?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "NETWORK_ON", &networkon ) == 0 )
		{
			networkon = bfalse; // default
		}

		//Max lag
		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "LAG_TOLERANCE", &lTmpInt ) == 0 )
			{
			lTmpInt = 2; // default
			}
		lag = lTmpInt;

		/*
		goto_colon(fileread); fscanf(fileread, "%d", &orderlag);

		if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "RTS_LAG_TOLERANCE", &lTmpInt ) == 0 )
			{
			lTmpInt = 25; // default
			}
		orderlag = lTmpInt;
		*/

		//Name or IP of the host or the target to join
		if ( GetConfigValue( lConfigSetup, lCurSectionName, "HOST_NAME", nethostname, 64) == 0 )
			{
			strcpy( nethostname, "no host" ); // default
			}

		//Multiplayer name
		if ( GetConfigValue( lConfigSetup, lCurSectionName, "MULTIPLAYER_NAME", netmessagename, 64) == 0 )
			{
			strcpy( netmessagename, "little Raoul" ); // default
			}


		/*********************************************

		DEBUG Section

		*********************************************/

		strcpy( lCurSectionName, "DEBUG" );

		//Show the FPS counter?
		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "DISPLAY_FPS", &lTempBool ) == 0 )
		{
			lTempBool = btrue; // default
		}
		fpson = lTempBool;

		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "HIDE_MOUSE", &gHideMouse ) == 0 )
		{
			gHideMouse = btrue; // default
		}

		if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "GRAB_MOUSE", &gGrabMouse ) == 0 )
		{
			gGrabMouse = btrue; // default
		}

		CloseConfigFile( lConfigSetup );

		}
	}
//--------------------------------------------------------------------------------------------
void log_madused(char *savename)
{
    // ZZ> This is a debug function for checking model loads
    FILE* hFileWrite;
    int cnt;

    hFileWrite = fopen(savename, "w");
    if(hFileWrite)
    {
        fprintf(hFileWrite, "Slot usage for objects in last module loaded...\n");
        fprintf(hFileWrite, "%d of %d frames used...\n", madloadframe, MAXFRAME);
        cnt = 0;
        while(cnt < MAXMODEL)
        {
            fprintf(hFileWrite, "%3d %32s %s\n", cnt, capclassname[cnt], madname[cnt]);
            cnt++;
        }
        fclose(hFileWrite);
    }
}

//---------------------------------------------------------------------------------------------
void make_lightdirectionlookup()
{
    // ZZ> This function builds the lighting direction table
    //     The table is used to find which direction the light is coming
    //     from, based on the four corner vertices of a mesh tile.
    unsigned int cnt;
    unsigned short tl, tr, br, bl;
    int x, y;

    for (cnt = 0; cnt < 65536; cnt++)
    {
        tl = (cnt&0xf000)>>12;
        tr = (cnt&0x0f00)>>8;
        br = (cnt&0x00f0)>>4;
        bl = (cnt&0x000f);
        x = br+tr-bl-tl;
        y = br+bl-tl-tr;
        lightdirectionlookup[cnt] = (atan2(-y, x)+PI)*256/(2*PI);
    }
}

float sinlut[MAXLIGHTROTATION];
float coslut[MAXLIGHTROTATION];

//---------------------------------------------------------------------------------------------
float light_for_normal(int rotation, int normal, float lx, float ly, float lz, float ambi)
{
    // ZZ> This function helps make_lighttable
    float fTmp;
    float nx, ny, nz;
    float sinrot, cosrot;

    nx = md2normals[normal][0];
    ny = md2normals[normal][1];
    nz = md2normals[normal][2];
    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];
    fTmp = cosrot*nx + sinrot*ny;
    ny = cosrot*ny - sinrot*nx;
    nx = fTmp;
    fTmp = nx*lx + ny*ly + nz*lz + ambi;
    if(fTmp < ambi) fTmp = ambi;
    return fTmp;
}


//---------------------------------------------------------------------------------------------
void make_lighttable(float lx, float ly, float lz, float ambi)
{
    // ZZ> This function makes a light table to fake directional lighting
    int lev, cnt, tnc;
    int itmp, itmptwo;

	// Build a lookup table for sin/cos
	for (cnt = 0; cnt < MAXLIGHTROTATION; cnt++)
	{
		sinlut[cnt] = sin(2*PI*cnt/MAXLIGHTROTATION);
		coslut[cnt] = cos(2*PI*cnt/MAXLIGHTROTATION);
	}

    for (cnt = 0; cnt < MD2LIGHTINDICES-1; cnt++)  // Spikey mace
    {
        for (tnc = 0; tnc < MAXLIGHTROTATION; tnc++)
        {
            lev = MAXLIGHTLEVEL-1;
            itmp = (255*light_for_normal(tnc,
                                         cnt,
                                         lx*lev/MAXLIGHTLEVEL,
                                         ly*lev/MAXLIGHTLEVEL,
                                         lz*lev/MAXLIGHTLEVEL,
                                         ambi));
            // This creates the light value for each level entry
            while(lev >= 0)
            {
                itmptwo = (((lev*itmp/(MAXLIGHTLEVEL-1))));
                if(itmptwo > 255)  itmptwo = 255;
                lighttable[lev][tnc][cnt] = (unsigned char) itmptwo;
				lev--;
            }
        }
    }
    // Fill in index number 162 for the spike mace
    for (tnc = 0; tnc < MAXLIGHTROTATION; tnc++)
    {
        lev = MAXLIGHTLEVEL-1;
        itmp = 255;
        // This creates the light value for each level entry
        while(lev >= 0)
        {
            itmptwo = (((lev*itmp/(MAXLIGHTLEVEL-1))));
            if(itmptwo > 255)  itmptwo = 255;
            lighttable[lev][tnc][cnt] = (unsigned char) itmptwo;
            lev--;
        }
    }
}

//---------------------------------------------------------------------------------------------
void make_lighttospek(void)
{
    // ZZ> This function makes a light table to fake directional lighting
    int cnt, tnc;
    unsigned char spek;
    float fTmp, fPow;


    // New routine
    for (cnt = 0; cnt < MAXSPEKLEVEL; cnt++)
    {
        for (tnc = 0; tnc < 256; tnc++)
        {
            fTmp = tnc/256.0;
            fPow = (fTmp*4.0)+1;
            fTmp = pow(fTmp, fPow);
            fTmp = fTmp*cnt*255.0/MAXSPEKLEVEL;
            if(fTmp<0) fTmp=0;
            if(fTmp>255) fTmp=255;
            spek = fTmp;
            spek = spek>>1;
            lighttospek[cnt][tnc] = (0xff000000)|(spek<<16)|(spek<<8)|(spek);
        }
    }
}

//---------------------------------------------------------------------------------------------
int vertexconnected(int modelindex, int vertex)
{
    // ZZ> This function returns 1 if the model vertex is connected, 0 otherwise
    int cnt, tnc, entry;

    entry = 0;
    for (cnt = 0; cnt < madcommands[modelindex]; cnt++)
    {
        for (tnc = 0; tnc < madcommandsize[modelindex][cnt]; tnc++)
        {
            if(madcommandvrt[modelindex][entry] == vertex)
            {
                // The vertex is used
                return 1;
            }
            entry++;
        }
    }

    // The vertex is not used
    return 0;
}

//---------------------------------------------------------------------------------------------
void get_madtransvertices(int modelindex)
{
    // ZZ> This function gets the number of vertices to transform for a model...
    //     That means every one except the grip ( unconnected ) vertices
    int cnt, trans = 0;

    for (cnt = 0; cnt < madvertices[modelindex]; cnt++)
        trans += vertexconnected(modelindex, cnt);

    madtransvertices[modelindex] = trans;
}

//---------------------------------------------------------------------------------------------
int rip_md2_header(void)
{
    // ZZ> This function makes sure an md2 is really an md2
    int iTmp;
    int* ipIntPointer;

    // Check the file type
    ipIntPointer = (int*) cLoadBuffer;
    #ifdef SDL_LIL_ENDIAN
    iTmp = ipIntPointer[0];
    #else
    iTmp = SDL_Swap32( ipIntPointer[0] );
    #endif
    if(iTmp != MD2START ) return bfalse;

    return btrue;
}

//---------------------------------------------------------------------------------------------
void fix_md2_normals(unsigned short modelindex)
{
    // ZZ> This function helps light not flicker so much
    int cnt, tnc;
    unsigned char indexofcurrent, indexofnext, indexofnextnext, indexofnextnextnext;
    unsigned char indexofnextnextnextnext;
    unsigned int frame;

    frame = madframestart[modelindex];
    cnt = 0;
    while(cnt < madvertices[modelindex])
    {
        tnc = 0;
        while(tnc < madframes[modelindex])
        {
            indexofcurrent = madvrta[frame][cnt];
            indexofnext = madvrta[frame+1][cnt];
            indexofnextnext = madvrta[frame+2][cnt];
            indexofnextnextnext = madvrta[frame+3][cnt];
            indexofnextnextnextnext = madvrta[frame+4][cnt];
            if(indexofcurrent == indexofnextnext && indexofnext != indexofcurrent)
            {
                madvrta[frame+1][cnt] = indexofcurrent;
            }
            if(indexofcurrent == indexofnextnextnext)
            {
                if(indexofnext != indexofcurrent)
                {
                    madvrta[frame+1][cnt] = indexofcurrent;
                }
                if(indexofnextnext != indexofcurrent)
                {
                    madvrta[frame+2][cnt] = indexofcurrent;
                }
            }
            if(indexofcurrent == indexofnextnextnextnext)
            {
                if(indexofnext != indexofcurrent)
                {
                    madvrta[frame+1][cnt] = indexofcurrent;
                }
                if(indexofnextnext != indexofcurrent)
                {
                    madvrta[frame+2][cnt] = indexofcurrent;
                }
                if(indexofnextnextnext != indexofcurrent)
                {
                    madvrta[frame+3][cnt] = indexofcurrent;
                }
            }
            tnc++;
        }
        cnt++;
    }
}

//---------------------------------------------------------------------------------------------
void rip_md2_commands(unsigned short modelindex)
{
    // ZZ> This function converts an md2's GL commands into our little command list thing
    int iTmp;
    float fTmpu, fTmpv;
    int iNumVertices;
	int tnc;

    char* cpCharPointer = (char*) cLoadBuffer;
    int* ipIntPointer = (int*) cLoadBuffer;
    float* fpFloatPointer = (float*) cLoadBuffer;

	// Number of GL commands in the MD2
    #ifdef SDL_LIL_ENDIAN
    int iNumCommands = ipIntPointer[9];
    #else
    int iNumCommands = SDL_Swap32( ipIntPointer[9] );
    #endif

	// Offset (in DWORDS) from the start of the file to the gl command list.
	#ifdef SDL_LIL_ENDIAN
	int iCommandOffset = ipIntPointer[15]>>2;
	#else
	int iCommandOffset = SDL_Swap32( ipIntPointer[15] )>>2;
	#endif

    // Read in each command
    // iNumCommands isn't the number of commands, rather the number of dwords in
    // the command list...  Use iCommandCount to figure out how many we use
    int iCommandCount = 0;
	int entry = 0;

    int cnt = 0;
	while(cnt < iNumCommands)
    {
        #ifdef SDL_LIL_ENDIAN
        iNumVertices = ipIntPointer[iCommandOffset]; iCommandOffset++; cnt++;
        #else
        iNumVertices = SDL_Swap32( ipIntPointer[iCommandOffset] );  iCommandOffset++;  cnt++;
        #endif
        if(iNumVertices != 0)
        {
            if(iNumVertices < 0)
            {
                // Fans start with a negative
                iNumVertices = -iNumVertices;
                // PORT: madcommandtype[modelindex][iCommandCount] = (unsigned char) D3DPT_TRIANGLEFAN;
                madcommandtype[modelindex][iCommandCount] = GL_TRIANGLE_FAN;
                madcommandsize[modelindex][iCommandCount] = (unsigned char) iNumVertices;
            }
            else
            {
                // Strips start with a positive
                // PORT: madcommandtype[modelindex][iCommandCount] = (unsigned char) D3DPT_TRIANGLESTRIP;
                madcommandtype[modelindex][iCommandCount] = GL_TRIANGLE_STRIP;
                madcommandsize[modelindex][iCommandCount] = (unsigned char) iNumVertices;
            }

            // Read in vertices for each command
            tnc = 0;
            while(tnc < iNumVertices)
            {
                #ifdef SDL_LIL_ENDIAN
                fTmpu = fpFloatPointer[iCommandOffset];  iCommandOffset++;  cnt++;
                fTmpv = fpFloatPointer[iCommandOffset];  iCommandOffset++;  cnt++;
                iTmp = ipIntPointer[iCommandOffset];  iCommandOffset++;  cnt++;
                #else
                fTmpu = LoadFloatByteswapped( &fpFloatPointer[iCommandOffset] );  iCommandOffset++;  cnt++;
                fTmpv = LoadFloatByteswapped( &fpFloatPointer[iCommandOffset] );  iCommandOffset++;  cnt++;
                iTmp = SDL_Swap32( ipIntPointer[iCommandOffset] );  iCommandOffset++;  cnt++;
                #endif
                madcommandu[modelindex][entry] = fTmpu-(.5/64); // GL doesn't align correctly
                madcommandv[modelindex][entry] = fTmpv-(.5/64); // with D3D
                madcommandvrt[modelindex][entry] = (unsigned short) iTmp;
                entry++;
                tnc++;
            }
            iCommandCount++;
        }
    }
    madcommands[modelindex] = iCommandCount;
}

//---------------------------------------------------------------------------------------------
int rip_md2_frame_name(int frame)
{
    // ZZ> This function gets frame names from the load buffer, it returns
    //     btrue if the name in cFrameName[] is valid
    int iFrameOffset;
    int iNumVertices;
    int iNumFrames;
    int cnt;
    int* ipNamePointer;
    int* ipIntPointer;
    int foundname;

    // Jump to the Frames section of the md2 data
    ipNamePointer = (int*) cFrameName;
    ipIntPointer = (int*) cLoadBuffer;

    #ifdef SDL_LIL_ENDIAN
    iNumVertices = ipIntPointer[6];
    iNumFrames = ipIntPointer[10];
    iFrameOffset = ipIntPointer[14]>>2;
    #else
    iNumVertices = SDL_Swap32( ipIntPointer[6] );
    iNumFrames = SDL_Swap32( ipIntPointer[10] );
    iFrameOffset = SDL_Swap32( ipIntPointer[14] )>>2;
    #endif


    // Chug through each frame
    foundname = bfalse;
    cnt = 0;
    while(cnt < iNumFrames && !foundname)
    {
        iFrameOffset+=6;
        if(cnt == frame)
        {
            ipNamePointer[0] = ipIntPointer[iFrameOffset]; iFrameOffset++;
            ipNamePointer[1] = ipIntPointer[iFrameOffset]; iFrameOffset++;
            ipNamePointer[2] = ipIntPointer[iFrameOffset]; iFrameOffset++;
            ipNamePointer[3] = ipIntPointer[iFrameOffset]; iFrameOffset++;
            foundname = btrue;
        }
        else
        {
            iFrameOffset+=4;
        }
        iFrameOffset+=iNumVertices;
        cnt++;
    }
    cFrameName[15] = 0;  // Make sure it's null terminated
    return foundname;
}

//---------------------------------------------------------------------------------------------
void rip_md2_frames(unsigned short modelindex)
{
    // ZZ> This function gets frames from the load buffer and adds them to
    //     the indexed model
    unsigned char cTmpx, cTmpy, cTmpz;
    unsigned char cTmpNormalIndex;
    float fRealx, fRealy, fRealz;
    float fScalex, fScaley, fScalez;
    float fTranslatex, fTranslatey, fTranslatez;
    int iFrameOffset;
    int iNumVertices;
    int iNumFrames;
    int cnt, tnc;
    char* cpCharPointer;
    int* ipIntPointer;
    float* fpFloatPointer;


    // Jump to the Frames section of the md2 data
    cpCharPointer = (char*) cLoadBuffer;
    ipIntPointer = (int*) cLoadBuffer;
    fpFloatPointer = (float*) cLoadBuffer;

    #ifdef SDL_LIL_ENDIAN
    iNumVertices = ipIntPointer[6];
    iNumFrames = ipIntPointer[10];
    iFrameOffset = ipIntPointer[14]>>2;
    #else
    iNumVertices = SDL_Swap32( ipIntPointer[6] );
    iNumFrames = SDL_Swap32( ipIntPointer[10] );
    iFrameOffset = SDL_Swap32( ipIntPointer[14] )>>2;
    #endif


    // Read in each frame
    madframestart[modelindex] = madloadframe;
    madframes[modelindex] = iNumFrames;
    madvertices[modelindex] = iNumVertices;
    madscale[modelindex] = (float) (1.0/320.0);  // Scale each vertex float to fit it in a short
    cnt = 0;
    while(cnt < iNumFrames && madloadframe < MAXFRAME)
    {
        #ifdef SDL_LIL_ENDIAN
        fScalex = fpFloatPointer[iFrameOffset]; iFrameOffset++;
        fScaley = fpFloatPointer[iFrameOffset]; iFrameOffset++;
        fScalez = fpFloatPointer[iFrameOffset]; iFrameOffset++;
        fTranslatex = fpFloatPointer[iFrameOffset]; iFrameOffset++;
        fTranslatey = fpFloatPointer[iFrameOffset]; iFrameOffset++;
        fTranslatez = fpFloatPointer[iFrameOffset]; iFrameOffset++;
        #else
        fScalex = LoadFloatByteswapped( &fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        fScaley = LoadFloatByteswapped( &fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        fScalez = LoadFloatByteswapped( &fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        fTranslatex = LoadFloatByteswapped( &fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        fTranslatey = LoadFloatByteswapped( &fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        fTranslatez = LoadFloatByteswapped( &fpFloatPointer[iFrameOffset] ); iFrameOffset++;
        #endif


        iFrameOffset+=4;
        tnc = 0;
        while(tnc < iNumVertices)
        {
            // This should work because it's reading a single character
            cTmpx = cpCharPointer[(iFrameOffset<<2)];
            cTmpy = cpCharPointer[(iFrameOffset<<2)+1];
            cTmpz = cpCharPointer[(iFrameOffset<<2)+2];
            cTmpNormalIndex = cpCharPointer[(iFrameOffset<<2)+3];
            fRealx = (cTmpx*fScalex)+fTranslatex;
            fRealy = (cTmpy*fScaley)+fTranslatey;
            fRealz = (cTmpz*fScalez)+fTranslatez;
//            fRealx = (cTmpx*fScalex);
//            fRealy = (cTmpy*fScaley);
//            fRealz = (cTmpz*fScalez);
//            madvrtx[madloadframe][tnc] = (signed short) (fRealx*256); // HUK
            madvrtx[madloadframe][tnc] = (signed short) (-fRealx*256);
            madvrty[madloadframe][tnc] = (signed short) (fRealy*256);
            madvrtz[madloadframe][tnc] = (signed short) (fRealz*256);
            madvrta[madloadframe][tnc] = cTmpNormalIndex;
            iFrameOffset++;
            tnc++;
        }
        madloadframe++;
        cnt++;
    }
}

//---------------------------------------------------------------------------------------------
int load_one_md2(char* szLoadname, unsigned short modelindex)
{
    // ZZ> This function loads an id md2 file, storing the converted data in the indexed model
//    int iFileHandleRead;
    size_t iBytesRead = 0;
    int iReturnValue;

    // Read the input file
    FILE *file = fopen(szLoadname,"rb");
	if (!file)
	{
		log_warning("Cannot load file! (%s)\n", szLoadname);
		return bfalse;
	}

	// Read up to MD2MAXLOADSIZE bytes from the file into the cLoadBuffer array.
	iBytesRead = fread(cLoadBuffer, 1, MD2MAXLOADSIZE, file);
	if (iBytesRead == 0)
		return bfalse;

    // Check the header
    // TODO: Verify that the header's filesize correspond to iBytesRead.
	iReturnValue = rip_md2_header();
    if (iReturnValue == bfalse)
		  return bfalse;

	// Get the frame vertices
    rip_md2_frames(modelindex);
    // Get the commands
    rip_md2_commands(modelindex);
    // Fix them normals
    fix_md2_normals(modelindex);
    // Figure out how many vertices to transform
    get_madtransvertices(modelindex);

	fclose(file);

    return btrue;
}

//--------------------------------------------------------------------------------------------
void make_enviro(void)
{
    // ZZ> This function sets up the environment mapping table
    int cnt;
    float z;
    float x, y;

    // Find the environment map positions
    for (cnt = 0; cnt < MD2LIGHTINDICES; cnt++)
    {
        x = md2normals[cnt][0];
        y = md2normals[cnt][1];
        x = (atan2(y, x)+PI)/(PI);
        x--;

		if(x < 0)
			x--;

        indextoenvirox[cnt] = x;
    }

	for (cnt = 0; cnt < 256; cnt++)
    {
        z = cnt / 256.0;  // Z is between 0 and 1
        lighttoenviroy[cnt] = z;
    }
}

//--------------------------------------------------------------------------------------------
void show_stat(unsigned short statindex)
{
    // ZZ> This function shows the more specific stats for a character
    int character, level;
    char text[64];
    char gender[8];

    if(statdelay == 0)
    {
        if(statindex < numstat)
        {
            character = statlist[statindex];


            // Name
            sprintf(text, "=%s=", chrname[character]);
            debug_message(text);


            // Level and gender and class
            gender[0] = 0;
            if(chralive[character])
            {
                if(chrgender[character] == GENMALE)
                {
                    sprintf(gender, "male ");
                }
                if(chrgender[character] == GENFEMALE)
                {
                    sprintf(gender, "female ");
                }
                level = chrexperiencelevel[character];
                if(level == 0)
                    sprintf(text, " 1st level %s%s", gender, capclassname[chrmodel[character]]);
                if(level == 1)
                    sprintf(text, " 2nd level %s%s", gender, capclassname[chrmodel[character]]);
                if(level == 2)
                    sprintf(text, " 3rd level %s%s", gender, capclassname[chrmodel[character]]);
                if(level >  2)
                    sprintf(text, " %dth level %s%s", level+1, gender, capclassname[chrmodel[character]]);
            }
            else
            {
                sprintf(text, " Dead %s", capclassname[chrmodel[character]]);
            }

            // Stats
            debug_message(text);
            sprintf(text, " STR:~%2d~WIS:~%2d~DEF:~%d", chrstrength[character]>>8, chrwisdom[character]>>8, 255-chrdefense[character]);
            debug_message(text);
            sprintf(text, " INT:~%2d~DEX:~%2d~EXP:~%d", chrintelligence[character]>>8, chrdexterity[character]>>8, chrexperience[character]);
            debug_message(text);
            statdelay = 10;
        }
    }
}


//--------------------------------------------------------------------------------------------
void show_armor(unsigned short statindex)
{
    // ZF> This function shows detailed armor information for the character
    char text[64], tmps[64];
    short character, skinlevel;

    if(statdelay == 0)
    {
        if(statindex < numstat)
        {
            character = statlist[statindex];
			skinlevel = chrtexture[character] - madskinstart[chrmodel[character]];

			//Armor Name
            sprintf(text, "=%s=", capskinname[chrmodel[character]]);
            debug_message(text);

            //Armor Stats
			sprintf(text, " DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d", 255-capdefense[chrmodel[character]][skinlevel],
				capdamagemodifier[chrmodel[character]][0][skinlevel]&DAMAGESHIFT,
				capdamagemodifier[chrmodel[character]][1][skinlevel]&DAMAGESHIFT,
				capdamagemodifier[chrmodel[character]][2][skinlevel]&DAMAGESHIFT );
            debug_message(text);

			sprintf(text, " HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
				capdamagemodifier[chrmodel[character]][3][skinlevel]&DAMAGESHIFT,
				capdamagemodifier[chrmodel[character]][4][skinlevel]&DAMAGESHIFT,
				capdamagemodifier[chrmodel[character]][5][skinlevel]&DAMAGESHIFT,
				capdamagemodifier[chrmodel[character]][6][skinlevel]&DAMAGESHIFT,
				capdamagemodifier[chrmodel[character]][7][skinlevel]&DAMAGESHIFT);
            debug_message(text);

			if(capskindressy[chrmodel[character]]) sprintf(tmps, "Light Armor");
			else								   sprintf(tmps, "Heavy Armor");
			sprintf(text, " Type: %s", tmps);

			//Speed and jumps
			if(chrjumpnumberreset[character] == 0)  sprintf(text, "None (0)");
			if(chrjumpnumberreset[character] == 1)  sprintf(text, "Novice (1)");
			if(chrjumpnumberreset[character] == 2)  sprintf(text, "Skilled (2)");
			if(chrjumpnumberreset[character] == 3)  sprintf(text, "Master (3)");
			if(chrjumpnumberreset[character] > 3)   sprintf(text, "Inhuman (4+)");
			sprintf(tmps, "Jump Skill: %s", text);
            sprintf(text, " Speed:~%3.0f~~%s", capmaxaccel[chrmodel[character]][skinlevel]*80, tmps);
			debug_message(text);

            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_full_status(unsigned short statindex)
{
    // ZF> This function shows detailed armor information for the character including magic
    char text[64], tmps[64];
    short character;
	int i = 0;
    if(statdelay == 0)
    {
        if(statindex < numstat)
        {
            character = statlist[statindex];

			//Enchanted?
			while(i != MAXENCHANT)
			{
				//Found a active enchantment that is not a skill of the character
				if(encon[i] && encspawner[i] != character && enctarget[i] == character) break;
				i++;
			}
			if(i != MAXENCHANT) sprintf(text, "=%s is enchanted!=", chrname[character]);
            else sprintf(text, "=%s is unenchanted=", chrname[character]);
            debug_message(text);

            //Armor Stats
			sprintf(text, " DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d",
				255-chrdefense[character],
				chrdamagemodifier[character][0]&DAMAGESHIFT,
				chrdamagemodifier[character][1]&DAMAGESHIFT,
				chrdamagemodifier[character][2]&DAMAGESHIFT );
            debug_message(text);
			sprintf(text, " HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
				chrdamagemodifier[character][3]&DAMAGESHIFT,
				chrdamagemodifier[character][4]&DAMAGESHIFT,
				chrdamagemodifier[character][5]&DAMAGESHIFT,
				chrdamagemodifier[character][6]&DAMAGESHIFT,
				chrdamagemodifier[character][7]&DAMAGESHIFT);
            debug_message(text);
			//Speed and jumps
			if(chrjumpnumberreset[character] == 0)  sprintf(text, "None (0)");
			if(chrjumpnumberreset[character] == 1)  sprintf(text, "Novice (1)");
			if(chrjumpnumberreset[character] == 2)  sprintf(text, "Skilled (2)");
			if(chrjumpnumberreset[character] == 3)  sprintf(text, "Master (3)");
			if(chrjumpnumberreset[character] > 3)   sprintf(text, "Inhuman (4+)");
			sprintf(tmps, "Jump Skill: %s", text);
            sprintf(text, " Speed:~%3.0f~~%s", chrmaxaccel[character]*80, tmps);
			debug_message(text);
            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_magic_status(unsigned short statindex)
{
    // ZF> Displays special enchantment effects for the character
    char text[64], tmpa[64], tmpb[64];
    short character;
	int i = 0;
    if(statdelay == 0)
    {
        if(statindex < numstat)
        {
            character = statlist[statindex];

			//Enchanted?
			while(i != MAXENCHANT)
			{
				//Found a active enchantment that is not a skill of the character
				if(encon[i] && encspawner[i] != character && enctarget[i] == character) break;
				i++;
			}
			if(i != MAXENCHANT) sprintf(text, "=%s is enchanted!=", chrname[character]);
            else sprintf(text, "=%s is unenchanted=", chrname[character]);
            debug_message(text);

            //Enchantment status
			if(chrcanseeinvisible[character])  sprintf(tmpa, "Yes");
			else							   sprintf(tmpa, "No");
			if(chrcanseekurse[character])      sprintf(tmpb, "Yes");
			else							   sprintf(tmpb, "No");
			sprintf(text, " See Invisible: %s~~See Kurses: %s", tmpa, tmpb);
			debug_message(text);

			if(chrcanchannel[character])	   sprintf(tmpa, "Yes");
			else							   sprintf(tmpa, "No");
			if(chrwaterwalk[character])        sprintf(tmpb, "Yes");
			else							   sprintf(tmpb, "No");
			sprintf(text, " Channel Life: %s~~Waterwalking: %s", tmpa, tmpb);
			debug_message(text);

			if(chrflyheight[character] > 0)    sprintf(tmpa, "Yes");
			else							   sprintf(tmpa, "No");
			if(chrmissiletreatment[character] == MISREFLECT)       sprintf(tmpb, "Reflect");
			else if(chrmissiletreatment[character] == MISREFLECT)  sprintf(tmpb, "Deflect");
		    else												   sprintf(tmpb, "None");
			sprintf(text, " Flying: %s~~Missile Protection: %s", tmpa, tmpb);
			debug_message(text);

            statdelay = 10;
        }
    }
}


//--------------------------------------------------------------------------------------------
void check_stats()
{
    // ZZ> This function lets the players check character stats
    if(netmessagemode == bfalse)
    {
		//Display armor stats?
		if(SDLKEYDOWN(SDLK_LSHIFT))
		{
			if(SDLKEYDOWN(SDLK_1))  show_armor(0);
	        if(SDLKEYDOWN(SDLK_2))  show_armor(1);
			if(SDLKEYDOWN(SDLK_3))  show_armor(2);
		    if(SDLKEYDOWN(SDLK_4))  show_armor(3);
	        if(SDLKEYDOWN(SDLK_5))  show_armor(4);
			if(SDLKEYDOWN(SDLK_6))  show_armor(5);
		    if(SDLKEYDOWN(SDLK_7))  show_armor(6);
	        if(SDLKEYDOWN(SDLK_8))  show_armor(7);
		}

		//Display enchantment stats?
		else if(SDLKEYDOWN(SDLK_LCTRL))
		{
		    if(SDLKEYDOWN(SDLK_1))  show_full_status(0);
	        if(SDLKEYDOWN(SDLK_2))  show_full_status(1);
			if(SDLKEYDOWN(SDLK_3))  show_full_status(2);
		    if(SDLKEYDOWN(SDLK_4))  show_full_status(3);
	        if(SDLKEYDOWN(SDLK_5))  show_full_status(4);
			if(SDLKEYDOWN(SDLK_6))  show_full_status(5);
		    if(SDLKEYDOWN(SDLK_7))  show_full_status(6);
	        if(SDLKEYDOWN(SDLK_8))  show_full_status(7);
		}

		//Display character stats?
		else if(SDLKEYDOWN(SDLK_LALT))
		{
		    if(SDLKEYDOWN(SDLK_1))  show_magic_status(0);
	        if(SDLKEYDOWN(SDLK_2))  show_magic_status(1);
			if(SDLKEYDOWN(SDLK_3))  show_magic_status(2);
		    if(SDLKEYDOWN(SDLK_4))  show_magic_status(3);
	        if(SDLKEYDOWN(SDLK_5))  show_magic_status(4);
			if(SDLKEYDOWN(SDLK_6))  show_magic_status(5);
		    if(SDLKEYDOWN(SDLK_7))  show_magic_status(6);
	        if(SDLKEYDOWN(SDLK_8))  show_magic_status(7);
		}


		//Display character stats?
		else
		{
		    if(SDLKEYDOWN(SDLK_1))  show_stat(0);
	        if(SDLKEYDOWN(SDLK_2))  show_stat(1);
			if(SDLKEYDOWN(SDLK_3))  show_stat(2);
		    if(SDLKEYDOWN(SDLK_4))  show_stat(3);
	        if(SDLKEYDOWN(SDLK_5))  show_stat(4);
			if(SDLKEYDOWN(SDLK_6))  show_stat(5);
		    if(SDLKEYDOWN(SDLK_7))  show_stat(6);
	        if(SDLKEYDOWN(SDLK_8))  show_stat(7);
		}


		// !!!BAD!!!  CHEAT
        if(SDLKEYDOWN(SDLK_x))
        {
            if(SDLKEYDOWN(SDLK_1) && plaindex[0]<MAXCHR)  give_experience(plaindex[0], 25, XPDIRECT);
            if(SDLKEYDOWN(SDLK_2) && plaindex[1]<MAXCHR)  give_experience(plaindex[1], 25, XPDIRECT);
            statdelay = 0;
        }
        if(SDLKEYDOWN(SDLK_m) && SDLKEYDOWN(SDLK_LSHIFT))
        {
			mapon = mapvalid;
			youarehereon = btrue;
		}
        if(SDLKEYDOWN(SDLK_z))
        {
			if(SDLKEYDOWN(SDLK_1) && plaindex[0]<MAXCHR)  chrlife[plaindex[0]]+=128;
		}
    }
}

void check_screenshot()
{
	//This function checks if we want to take a screenshot
    if (SDLKEYDOWN(SDLK_F11))
    {
        if (!dump_screenshot())									//Take the shot, returns bfalse if failed
        {
            debug_message("Error writing screenshot");
            log_warning("Error writing screenshot/n");			//Log the error in log.txt
        }
    }
}

bool_t dump_screenshot()
{
    // dumps the current screen (GL context) to a new bitmap file
    // right now it dumps it to whatever the current directory is

    // returns btrue if successful, bfalse otherwise

    SDL_Surface *screen, *temp;
    unsigned char *pixels;
    char buff[100], buff2[100];
    int i;
    FILE *test;

    screen = SDL_GetVideoSurface();
    temp = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24,
       #if SDL_BYTEORDER == SDL_LIL_ENDIAN
           0x000000FF, 0x0000FF00, 0x00FF0000, 0
       #else
           0x00FF0000, 0x0000FF00, 0x000000FF, 0
       #endif
           );

    if (temp == NULL)
        return bfalse;

    pixels = malloc(3*screen->w * screen->h);
    if (pixels == NULL)
    {
        SDL_FreeSurface(temp);
        return bfalse;
    }

    glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    for (i=0; i < screen->h; i++)
        memcpy(((char *) temp->pixels) + temp->pitch * i, pixels + 3 * screen->w * (screen->h-i-1), screen->w * 3);
    free(pixels);

    // find the next EGO??.BMP file for writing
    i=0;
    test=NULL;

    do
    {
        if (test != NULL)
            fclose(test);

        sprintf(buff, "ego%02d.bmp",i);

        // lame way of checking if the file already exists...
        test = fopen(buff, "rb");
        i++;

    } while ((test != NULL) && (i < 100));

    SDL_SaveBMP(temp, buff);
    SDL_FreeSurface(temp);

    sprintf(buff2, "Saved to %s", buff);
    debug_message(buff2);

    return btrue;
}

//--------------------------------------------------------------------------------------------
void add_stat(unsigned short character)
{
    // ZZ> This function adds a status display to the do list
    if(numstat < MAXSTAT)
    {
        statlist[numstat] = character;
        chrstaton[character] = btrue;
        numstat++;
    }
}

//--------------------------------------------------------------------------------------------
void move_to_top(unsigned short character)
{
    // ZZ> This function puts the character on top of the statlist
    int cnt, oldloc;


    // Find where it is
    oldloc = numstat;

	for (cnt = 0; cnt < numstat; cnt++)
        if(statlist[cnt] == character)
        {
            oldloc = cnt;
            cnt = numstat;
        }

    // Change position
    if(oldloc < numstat)
    {
        // Move all the lower ones up
        while(oldloc > 0)
        {
            oldloc--;
            statlist[oldloc+1] = statlist[oldloc];
        }
        // Put the character in the top slot
        statlist[0] = character;
    }
}

//--------------------------------------------------------------------------------------------
void sort_stat()
{
    // ZZ> This function puts all of the local players on top of the statlist
    int cnt;

	for (cnt = 0; cnt < numpla; cnt++)
        if(plavalid[cnt] && pladevice[cnt] != INPUTNONE)
        {
            move_to_top(plaindex[cnt]);
        }
}

//--------------------------------------------------------------------------------------------
void move_water(void)
{
    // ZZ> This function animates the water overlays
    int layer;

    for (layer = 0; layer < MAXWATERLAYER; layer++)
    {
        waterlayeru[layer] += waterlayeruadd[layer];
        waterlayerv[layer] += waterlayervadd[layer];
        if (waterlayeru[layer]>1.0)  waterlayeru[layer]-=1.0;
        if (waterlayerv[layer]>1.0)  waterlayerv[layer]-=1.0;
        if (waterlayeru[layer]<-1.0)  waterlayeru[layer]+=1.0;
        if (waterlayerv[layer]<-1.0)  waterlayerv[layer]+=1.0;
        waterlayerframe[layer]=(waterlayerframe[layer]+waterlayerframeadd[layer])&WATERFRAMEAND;
    }
}

//--------------------------------------------------------------------------------------------
void play_action(unsigned short character, unsigned short action, unsigned char actionready)
{
    // ZZ> This function starts a generic action for a character
    if(madactionvalid[chrmodel[character]][action])
    {
        chrnextaction[character] = ACTIONDA;
        chraction[character] = action;
        chrlip[character] = 0;
        chrlastframe[character] = chrframe[character];
        chrframe[character] = madactionstart[chrmodel[character]][chraction[character]];
        chractionready[character] = actionready;
    }
}

//--------------------------------------------------------------------------------------------
void set_frame(unsigned short character, unsigned short frame, unsigned char lip)
{
    // ZZ> This function sets the frame for a character explicitly...  This is used to
    //     rotate Tank turrets
    chrnextaction[character] = ACTIONDA;
    chraction[character] = ACTIONDA;
    chrlip[character] = (lip<<6);
    chrlastframe[character] = madactionstart[chrmodel[character]][ACTIONDA] + frame;
    chrframe[character] = madactionstart[chrmodel[character]][ACTIONDA] + frame + 1;
    chractionready[character] = btrue;
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha(unsigned short character)
{
    // ZZ> This function fixes an item's transparency
    unsigned short enchant, mount;

    if(character != MAXCHR)
    {
        mount = chrattachedto[character];
        if(chron[character] && mount != MAXCHR && chrisitem[character] && chrtransferblend[mount])
        {
            // Okay, reset transparency
            enchant = chrfirstenchant[character];
            while(enchant < MAXENCHANT)
            {
                unset_enchant_value(enchant, SETALPHABLEND);
                unset_enchant_value(enchant, SETLIGHTBLEND);
                enchant = encnextenchant[enchant];
            }
            chralpha[character] = chrbasealpha[character];
            chrlight[character] = caplight[chrmodel[character]];
            enchant = chrfirstenchant[character];
            while(enchant < MAXENCHANT)
            {
                set_enchant_value(enchant, SETALPHABLEND, enceve[enchant]);
                set_enchant_value(enchant, SETLIGHTBLEND, enceve[enchant]);
                enchant = encnextenchant[enchant];
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
int generate_number(int numbase, int numrand)
{
    // ZZ> This function generates a random number
    int tmp = 0;

	tmp = numbase;
    if(numrand > 0)
    {
		tmp+=(rand()%numrand);
	}
	else
	{
	    log_warning("One of the data pairs is wrong! (%i and %i) Cannot be 0 or less.\n", numbase, numrand);
		numrand = numbase;
	}

	return tmp;
}

//--------------------------------------------------------------------------------------------
void drop_money(unsigned short character, unsigned short money)
{
    // ZZ> This function drops some of a character's money
    unsigned short huns, tfives, fives, ones, cnt;

    if(money > chrmoney[character])  money = chrmoney[character];
    if(money>0 && chrzpos[character] > -2)
    {
        chrmoney[character] = chrmoney[character]-money;
        huns = money/100;  money -= (huns<<7)-(huns<<5)+(huns<<2);
        tfives = money/25;  money -= (tfives<<5)-(tfives<<3)+tfives;
        fives = money/5;  money -= (fives<<2)+fives;
        ones = money;

		for (cnt = 0; cnt < ones; cnt++)
            spawn_one_particle(chrxpos[character], chrypos[character],  chrzpos[character], 0, MAXMODEL, COIN1, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR);

		for (cnt = 0; cnt < fives; cnt++)
            spawn_one_particle(chrxpos[character], chrypos[character],  chrzpos[character], 0, MAXMODEL, COIN5, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR);

		for (cnt = 0; cnt < tfives; cnt++)
            spawn_one_particle(chrxpos[character], chrypos[character],  chrzpos[character], 0, MAXMODEL, COIN25, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR);

        for (cnt = 0; cnt < huns; cnt++)
            spawn_one_particle(chrxpos[character], chrypos[character],  chrzpos[character], 0, MAXMODEL, COIN100, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR);

		chrdamagetime[character] = DAMAGETIME;  // So it doesn't grab it again
    }
}

//--------------------------------------------------------------------------------------------
void call_for_help(unsigned short character)
{
    // ZZ> This function issues a call for help to all allies
    unsigned char team;
    unsigned short cnt;

    team = chrteam[character];
    teamsissy[team] = character;

	for (cnt = 0; cnt < MAXCHR; cnt++)
        if(chron[cnt] && cnt!=character && teamhatesteam[chrteam[cnt]][team] == bfalse)
	        chralert[cnt]=chralert[cnt]|ALERTIFCALLEDFORHELP;
}

//--------------------------------------------------------------------------------------------
void give_experience(int character, int amount, unsigned char xptype)
{
    // ZZ> This function gives a character experience, and pawns off level gains to
    //     another function
    int newamount;
    unsigned char curlevel;
    int number;
    int profile;
    char text[128];


    if(chrinvictus[character]==bfalse)
    {
        // Figure out how much experience to give
        profile = chrmodel[character];
        newamount = amount;
        if(xptype < MAXEXPERIENCETYPE)
        {
            newamount = amount*capexperiencerate[profile][xptype];
        }
        newamount+=chrexperience[character];
        if(newamount > MAXXP)  newamount = MAXXP;
        chrexperience[character]=newamount;


        // Do level ups and stat changes
        curlevel = chrexperiencelevel[character];
        if(curlevel+1 < 20)
        {
			//unsigned int xpneeded = capexperienceforlevel[profile][5] + (((capexperienceforlevel[profile][5]*curlevel*curlevel*curlevel))>>7);
			unsigned int xpneeded = capexperienceforlevel[profile][5] + ((curlevel+1)*(curlevel+1)*(curlevel+1)*15);
			if(curlevel < MAXLEVEL-1) xpneeded = capexperienceforlevel[profile][curlevel+1];
            if(chrexperience[character] >= xpneeded)
            {
                // The character is ready to advance...
                if(chrisplayer[character])
                {
					sprintf(text, "%s gained a level!!!", chrname[character]);
					sound = Mix_LoadWAV("basicdat/lvlup.wav");
					Mix_PlayChannel(-1, sound, 0);
                    debug_message(text);
                }
                chrexperiencelevel[character]++;

				//BAD!! Prevents multiple levels !!BAD
				chrexperience[character] = xpneeded;

                // Size
                chrsizegoto[character]+=(capsizeperlevel[profile]>1);  // Limit this?
                chrsizegototime[character] = SIZETIME;
                // Strength
                number = generate_number(capstrengthperlevelbase[profile], capstrengthperlevelrand[profile]);
                number = number+chrstrength[character];
                if(number > PERFECTSTAT) number = PERFECTSTAT;
                chrstrength[character] = number;
                // Wisdom
                number = generate_number(capwisdomperlevelbase[profile], capwisdomperlevelrand[profile]);
                number = number+chrwisdom[character];
                if(number > PERFECTSTAT) number = PERFECTSTAT;
                chrwisdom[character] = number;
                // Intelligence
                number = generate_number(capintelligenceperlevelbase[profile], capintelligenceperlevelrand[profile]);
                number = number+chrintelligence[character];
                if(number > PERFECTSTAT) number = PERFECTSTAT;
                chrintelligence[character] = number;
                // Dexterity
                number = generate_number(capdexterityperlevelbase[profile], capdexterityperlevelrand[profile]);
                number = number+chrdexterity[character];
                if(number > PERFECTSTAT) number = PERFECTSTAT;
                chrdexterity[character] = number;
                // Life
                number = generate_number(caplifeperlevelbase[profile], caplifeperlevelrand[profile]);
                number = number+chrlifemax[character];
                if(number > PERFECTBIG) number = PERFECTBIG;
                chrlife[character]+=(number-chrlifemax[character]);
                chrlifemax[character] = number;
                // Mana
                number = generate_number(capmanaperlevelbase[profile], capmanaperlevelrand[profile]);
                number = number+chrmanamax[character];
                if(number > PERFECTBIG) number = PERFECTBIG;
                chrmana[character]+=(number-chrmanamax[character]);
                chrmanamax[character] = number;
                // Mana Return
                number = generate_number(capmanareturnperlevelbase[profile], capmanareturnperlevelrand[profile]);
                number = number+chrmanareturn[character];
                if(number > PERFECTSTAT) number = PERFECTSTAT;
                chrmanareturn[character] = number;
                // Mana Flow
                number = generate_number(capmanaflowperlevelbase[profile], capmanaflowperlevelrand[profile]);
                number = number+chrmanaflow[character];
                if(number > PERFECTSTAT) number = PERFECTSTAT;
                chrmanaflow[character] = number;
            }
        }
    }
}


//--------------------------------------------------------------------------------------------
void give_team_experience(unsigned char team, int amount, unsigned char xptype)
{
    // ZZ> This function gives a character experience, and pawns off level gains to
    //     another function
    int cnt;

	for (cnt = 0; cnt < MAXCHR; cnt++)
        if(chrteam[cnt] == team && chron[cnt])
            give_experience(cnt, amount, xptype);
}


//--------------------------------------------------------------------------------------------
void setup_alliances(char *modname)
{
    // ZZ> This function reads the alliance file
    char newloadname[256];
    char szTemp[256];
    unsigned char teama, teamb;
    FILE *fileread;


    // Load the file
    make_newloadname(modname, "gamedat/alliance.txt", newloadname);
    fileread = fopen(newloadname, "r");
    if(fileread)
    {
        while(goto_colon_yesno(fileread))
        {
            fscanf(fileread, "%s", szTemp);
            teama = (szTemp[0]-'A')%MAXTEAM;
            fscanf(fileread, "%s", szTemp);
            teamb = (szTemp[0]-'A')%MAXTEAM;
            teamhatesteam[teama][teamb] = bfalse;
        }
        fclose(fileread);
    }
}

//grfx.c
//--------------------------------------------------------------------------------------------
void load_mesh_fans()
{
    // ZZ> This function loads fan types for the terrain
    int cnt, entry;
    int numfantype, fantype, bigfantype, vertices;
    int numcommand, command, commandsize;
    int itmp;
    float ftmp;
    FILE* fileread;
    float offx, offy;


    // Initialize all mesh types to 0
    entry = 0;
    while(entry < MAXMESHTYPE)
    {
        meshcommandnumvertices[entry] = 0;
        meshcommands[entry] = 0;
        entry++;
    }


    // Open the file and go to it
    fileread = fopen("basicdat/fans.txt", "r");
    if(fileread)
    {
        goto_colon(fileread);
        fscanf(fileread, "%d", &numfantype);
        fantype = 0;
        bigfantype = MAXMESHTYPE/2; // Duplicate for 64x64 tiles
        while(fantype < numfantype)
        {
            goto_colon(fileread);
            fscanf(fileread, "%d", &vertices);
            meshcommandnumvertices[fantype] = vertices;
            meshcommandnumvertices[bigfantype] = vertices;  // Dupe
            cnt = 0;
            while(cnt < vertices)
            {
                goto_colon(fileread);
                fscanf(fileread, "%d", &itmp);
                goto_colon(fileread);
                fscanf(fileread, "%f", &ftmp);
                meshcommandu[fantype][cnt] = ftmp;
                meshcommandu[bigfantype][cnt] = ftmp;  // Dupe
                goto_colon(fileread);
                fscanf(fileread, "%f", &ftmp);
                meshcommandv[fantype][cnt] = ftmp;
                meshcommandv[bigfantype][cnt] = ftmp;  // Dupe
                cnt++;
            }


            goto_colon(fileread);
            fscanf(fileread, "%d", &numcommand);
            meshcommands[fantype] = numcommand;
            meshcommands[bigfantype] = numcommand;  // Dupe
            entry = 0;
            command = 0;
            while(command < numcommand)
            {
                goto_colon(fileread);
                fscanf(fileread, "%d", &commandsize);
                meshcommandsize[fantype][command] = commandsize;
                meshcommandsize[bigfantype][command] = commandsize;  // Dupe
                cnt = 0;
                while(cnt < commandsize)
                {
                    goto_colon(fileread);
                    fscanf(fileread, "%d", &itmp);
                    meshcommandvrt[fantype][entry] = itmp;
                    meshcommandvrt[bigfantype][entry] = itmp;  // Dupe
                    entry++;
                    cnt++;
                }
                command++;
            }
            fantype++;
            bigfantype++;  // Dupe
        }
        fclose(fileread);
    }


    // Correct all of them silly texture positions for seamless tiling
    entry = 0;
    while(entry < MAXMESHTYPE/2)
    {
        cnt = 0;
        while(cnt < meshcommandnumvertices[entry])
        {
//            meshcommandu[entry][cnt] = ((.5/32)+(meshcommandu[entry][cnt]*31/32))/8;
//            meshcommandv[entry][cnt] = ((.5/32)+(meshcommandv[entry][cnt]*31/32))/8;
            meshcommandu[entry][cnt] = ((.6/32)+(meshcommandu[entry][cnt]*30.8/32))/8;
            meshcommandv[entry][cnt] = ((.6/32)+(meshcommandv[entry][cnt]*30.8/32))/8;
            cnt++;
        }
        entry++;
    }
    // Do for big tiles too
    while(entry < MAXMESHTYPE)
    {
        cnt = 0;
        while(cnt < meshcommandnumvertices[entry])
        {
//            meshcommandu[entry][cnt] = ((.5/64)+(meshcommandu[entry][cnt]*63/64))/4;
//            meshcommandv[entry][cnt] = ((.5/64)+(meshcommandv[entry][cnt]*63/64))/4;
            meshcommandu[entry][cnt] = ((.6/64)+(meshcommandu[entry][cnt]*62.8/64))/4;
            meshcommandv[entry][cnt] = ((.6/64)+(meshcommandv[entry][cnt]*62.8/64))/4;
            cnt++;
        }
        entry++;
    }


    // Make tile texture offsets
    entry = 0;
    while(entry < MAXTILETYPE)
    {
        offx = (entry&7)/8.0;
        offy = (entry>>3)/8.0;
        meshtileoffu[entry] = offx;
        meshtileoffv[entry] = offy;
        entry++;
    }
}

//--------------------------------------------------------------------------------------------
void make_fanstart()
{
    // ZZ> This function builds a look up table to ease calculating the
    //     fan number given an x,y pair
    int cnt;


    cnt = 0;
    while(cnt < meshsizey)
    {
        meshfanstart[cnt] = meshsizex*cnt;
        cnt++;
    }
    cnt = 0;
    while(cnt < (meshsizey>>2))
    {
        meshblockstart[cnt] = (meshsizex>>2)*cnt;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_twist()
{
    // ZZ> This function precomputes surface normals and steep hill acceleration for
    //     the mesh
    int cnt;
    int x, y;
    float xslide, yslide;

    cnt = 0;
    while(cnt < 256)
    {
        y = cnt>>4;
        x = cnt&15;
        y = y-7;  // -7 to 8
        x = x-7;  // -7 to 8
        mapudtwist[cnt] = 32768+y*SLOPE;
        maplrtwist[cnt] = 32768+x*SLOPE;
        if(ABS(y) >=7 ) y=y<<1;
        if(ABS(x) >=7 ) x=x<<1;
        xslide = x*SLIDE;
        yslide = y*SLIDE;
        if(xslide < 0)
        {
            xslide+=SLIDEFIX;
            if(xslide > 0)
              xslide=0;
        }
        else
        {
            xslide-=SLIDEFIX;
            if(xslide < 0)
              xslide=0;
        }
        if(yslide < 0)
        {
            yslide+=SLIDEFIX;
            if(yslide > 0)
              yslide=0;
        }
        else
        {
            yslide-=SLIDEFIX;
            if(yslide < 0)
              yslide=0;
        }
        veludtwist[cnt] = -yslide*hillslide;
        vellrtwist[cnt] = xslide*hillslide;
        flattwist[cnt] = bfalse;
        if(ABS(veludtwist[cnt]) + ABS(vellrtwist[cnt]) < SLIDEFIX*4)
        {
            flattwist[cnt] = btrue;
        }
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
int load_mesh(char *modname)
{
  // ZZ> This function loads the level.mpd file
  FILE* fileread;
  char newloadname[256];
  int itmp, cnt;
  float ftmp;
  int fan;
  int numvert, numfan;
  int x, y, vert;

  make_newloadname(modname, "gamedat/level.mpd", newloadname);
  fileread = fopen(newloadname, "rb");
  if(fileread)
  {
    #ifdef SDL_LIL_ENDIAN
    fread(&itmp, 4, 1, fileread);  if(itmp != MAPID) return bfalse;
    fread(&itmp, 4, 1, fileread);  numvert = itmp;
    fread(&itmp, 4, 1, fileread);  meshsizex = itmp;
    fread(&itmp, 4, 1, fileread);  meshsizey = itmp;
    #else
    fread(&itmp, 4, 1, fileread);  if( ( int )SDL_Swap32( itmp ) != MAPID) return bfalse;
    fread(&itmp, 4, 1, fileread);  numvert = ( int )SDL_Swap32( itmp );
    fread(&itmp, 4, 1, fileread);  meshsizex = ( int )SDL_Swap32( itmp );
    fread(&itmp, 4, 1, fileread);  meshsizey = ( int )SDL_Swap32( itmp );
    #endif

    numfan = meshsizex*meshsizey;
    meshedgex = meshsizex*128;
    meshedgey = meshsizey*128;
    numfanblock = ((meshsizex>>2))*((meshsizey>>2));  // MESHSIZEX MUST BE MULTIPLE OF 4
    watershift = 3;
    if(meshsizex > 16)  watershift++;
    if(meshsizex > 32)  watershift++;
    if(meshsizex > 64)  watershift++;
    if(meshsizex > 128)  watershift++;
    if(meshsizex > 256)  watershift++;


    // Load fan data
    fan = 0;
    while(fan < numfan)
    {
      fread(&itmp, 4, 1, fileread);

      #ifdef SDL_LIL_ENDIAN
      meshtype[fan] = itmp>>24;
      meshfx[fan] = itmp>>16;
      meshtile[fan] = itmp;
      #else
      meshtype[fan] = SDL_Swap32( itmp )>>24;
      meshfx[fan] = SDL_Swap32( itmp )>>16;
      meshtile[fan] = SDL_Swap32( itmp );
      #endif

      fan++;
    }
    // Load fan data
    fan = 0;
    while(fan < numfan)
    {
      fread(&itmp, 1, 1, fileread);

      #ifdef SDL_LIL_ENDIAN
      meshtwist[fan] = itmp;
      #else
      meshtwist[fan] = SDL_Swap32( itmp );
      #endif

      fan++;
    }


    // Load vertex x data
    cnt = 0;
    while(cnt < numvert)
    {
      fread(&ftmp, 4, 1, fileread);

      #ifdef SDL_LIL_ENDIAN
      meshvrtx[cnt] = ftmp;
      #else
      meshvrtx[cnt] = LoadFloatByteswapped( &ftmp );
      #endif

      cnt++;
    }
    // Load vertex y data
    cnt = 0;
    while(cnt < numvert)
    {
      fread(&ftmp, 4, 1, fileread);

      #ifdef SDL_LIL_ENDIAN
      meshvrty[cnt] = ftmp;
      #else
      meshvrty[cnt] = LoadFloatByteswapped( &ftmp );
      #endif

      cnt++;
    }
    // Load vertex z data
    cnt = 0;
    while(cnt < numvert)
    {
      fread(&ftmp, 4, 1, fileread);

      #ifdef SDL_LIL_ENDIAN
      meshvrtz[cnt] = ftmp/16.0;  // Cartman uses 4 bit fixed point for Z
      #else
      meshvrtz[cnt] = (LoadFloatByteswapped( &ftmp ))/16.0;  // Cartman uses 4 bit fixed point for Z
      #endif

      cnt++;
    }
	// GS - set to if(1) to disable lighting!!!!
    if(0)    //(shading == D3DSHADE_FLAT && rtscontrol==bfalse)
    {
      // Assume fullbright
      cnt = 0;
      while(cnt < numvert)
      {
        meshvrta[cnt] = 255;
        meshvrtl[cnt] = 255;
        cnt++;
      }
    }
    else
    {
      // Load vertex a data
      cnt = 0;
      while(cnt < numvert)
      {
        fread(&itmp, 1, 1, fileread);

        #ifdef SDL_LIL_ENDIAN
        meshvrta[cnt] = itmp;
        #else
        meshvrta[cnt] = SDL_Swap32( itmp );
        #endif
        meshvrtl[cnt] = 0;

        cnt++;
      }
    }
    fclose(fileread);


    make_fanstart();


    vert = 0;
    y = 0;
    while(y < meshsizey)
    {
      x = 0;
      while(x < meshsizex)
      {
        fan = meshfanstart[y]+x;
        meshvrtstart[fan] = vert;
        vert+=meshcommandnumvertices[meshtype[fan]];
        x++;
      }
      y++;
    }

    return btrue;
  }
  else log_warning("Cannot find level.mpd!!\n");
  return bfalse;
}

//--------------------------------------------------------------------------------------------
void update_game()
{
    // ZZ> This function does several iterations of character movements and such
    //     to keep the game in sync.
    int cnt, numdead;

    // Check for all local players being dead
    alllocalpladead = bfalse;
    localseeinvisible = bfalse;
    localseekurse = bfalse;
    cnt = 0;
    numdead = 0;
    while(cnt < MAXPLAYER)
    {
        if(plavalid[cnt] && pladevice[cnt] != INPUTNONE)
        {
            if(chralive[plaindex[cnt]] == bfalse)
            {
                numdead++;
                if (SDLKEYDOWN(SDLK_SPACE && respawnvalid == btrue))
				{
                	respawn_character(plaindex[cnt]);
					chrexperience[cnt] = (chrexperience[cnt])*EXPKEEP;	//Apply xp Penality
                }
            }
            else
            {
                if(chrcanseeinvisible[plaindex[cnt]])
                {
                    localseeinvisible = btrue;
                }
                if(chrcanseekurse[plaindex[cnt]])
                {
                    localseekurse = btrue;
                }
            }
        }
        cnt++;
    }
    if(numdead >= numlocalpla)
    {
        alllocalpladead = btrue;
    }

    // This is the main game loop
    msgtimechange = 0;

	// [claforte Jan 6th 2001]
	// TODO: Put that back in place once networking is functional.
    while(wldclock < allclock && (numplatimes > 0 || rtscontrol))
    {
        // Important stuff to keep in sync
        srand(randsave);
        sv_talkToRemotes();
        resize_characters();
        keep_weapons_with_holders();
        let_ai_think();
        do_weather_spawn();
        do_enchant_spawn();
        unbuffer_player_latches();
        move_characters();
        move_particles();
        make_character_matrices();
        attach_particles();
        make_onwhichfan();
        bump_characters();
        stat_return();
        pit_kill();
        // Generate the new seed
        randsave += *((unsigned int*) &md2normals[wldframe&127][0]);
        randsave += *((unsigned int*) &md2normals[randsave&127][1]);

        // Stuff for which sync doesn't matter
        flash_select();
        animate_tiles();
        move_water();

        // Timers
        wldclock+=FRAMESKIP;
        wldframe++;
        msgtimechange++;
        if(statdelay > 0)  statdelay--;

		//statclock++;
		statclock+=FRAMESKIP;
    }
    if(rtscontrol == bfalse)
    {
        if(numplatimes == 0)
        {
            // The remote ran out of messages, and is now twiddling its thumbs...
            // Make it go slower so it doesn't happen again
            wldclock+=25;
        }
        if(numplatimes > 3 && hostactive == bfalse)
        {
            // The host has too many messages, and is probably experiencing control
            // lag...  Speed it up so it gets closer to sync
            wldclock-=5;
        }
    }
}

//--------------------------------------------------------------------------------------------
void update_timers()
{
    // ZZ> This function updates the game timers
    lstclock = allclock;
    allclock = SDL_GetTicks()-sttclock;
    fpsclock+=allclock-lstclock;
    if(fpsclock >= TICKS_PER_SEC)
    {
        create_szfpstext(fpsframe);
        fpsclock = 0;
        fpsframe = 0;
    }
}

//--------------------------------------------------------------------------------------------
void read_pair(FILE* fileread)
{
    // ZZ> This function reads a damage/stat pair ( eg. 5-9 )
    char cTmp;
    float  fBase, fRand;

    fscanf(fileread, "%f", &fBase);  // The first number
    pairbase = fBase*256;
    cTmp = get_first_letter(fileread);  // The hyphen
    if(cTmp!='-')
    {
        // Not in correct format, so fail
        pairrand = 1;
        return;
    }
    fscanf(fileread, "%f", &fRand);  // The second number
    pairrand = fRand*256;
    pairrand = pairrand-pairbase;
    if(pairrand<1)
        pairrand = 1;
}

//--------------------------------------------------------------------------------------------
void undo_pair(int base, int rand)
{
    // ZZ> This function generates a damage/stat pair ( eg. 3-6.5 )
    //     from the base and random values.  It set pairfrom and
    //     pairto
    pairfrom = base/256.0;
    pairto = rand/256.0;
    if(pairfrom < 0.0)  pairfrom = 0.0;
    if(pairto < 0.0)  pairto = 0.0;
    pairto += pairfrom;
}

//--------------------------------------------------------------------------------------------
void ftruthf(FILE* filewrite, char* text, unsigned char truth)
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     btrue bfalse statements

    fprintf(filewrite, text);
    if(truth)
    {
        fprintf(filewrite, "TRUE\n");
    }
    else
    {
        fprintf(filewrite, "FALSE\n");
    }
}

//--------------------------------------------------------------------------------------------
void fdamagf(FILE* filewrite, char* text, unsigned char damagetype)
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
    fprintf(filewrite, text);
    if(damagetype == DAMAGESLASH)
        fprintf(filewrite, "SLASH\n");
    if(damagetype == DAMAGECRUSH)
        fprintf(filewrite, "CRUSH\n");
    if(damagetype == DAMAGEPOKE)
        fprintf(filewrite, "POKE\n");
    if(damagetype == DAMAGEHOLY)
        fprintf(filewrite, "HOLY\n");
    if(damagetype == DAMAGEEVIL)
        fprintf(filewrite, "EVIL\n");
    if(damagetype == DAMAGEFIRE)
        fprintf(filewrite, "FIRE\n");
    if(damagetype == DAMAGEICE)
        fprintf(filewrite, "ICE\n");
    if(damagetype == DAMAGEZAP)
        fprintf(filewrite, "ZAP\n");
    if(damagetype == DAMAGENULL)
        fprintf(filewrite, "NONE\n");
}

//--------------------------------------------------------------------------------------------
void factiof(FILE* filewrite, char* text, unsigned char action)
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
    fprintf(filewrite, text);
    if(action == ACTIONDA)
        fprintf(filewrite, "WALK\n");
    if(action == ACTIONUA)
        fprintf(filewrite, "UNARMED\n");
    if(action == ACTIONTA)
        fprintf(filewrite, "THRUST\n");
    if(action == ACTIONSA)
        fprintf(filewrite, "SLASH\n");
    if(action == ACTIONCA)
        fprintf(filewrite, "CHOP\n");
    if(action == ACTIONBA)
        fprintf(filewrite, "BASH\n");
    if(action == ACTIONLA)
        fprintf(filewrite, "LONGBOW\n");
    if(action == ACTIONXA)
        fprintf(filewrite, "XBOW\n");
    if(action == ACTIONFA)
        fprintf(filewrite, "FLING\n");
    if(action == ACTIONPA)
        fprintf(filewrite, "PARRY\n");
    if(action == ACTIONZA)
        fprintf(filewrite, "ZAP\n");
}

//--------------------------------------------------------------------------------------------
void fgendef(FILE* filewrite, char* text, unsigned char gender)
{
    // ZZ> This function kinda mimics fprintf for the output of
    //     MALE FEMALE OTHER statements

    fprintf(filewrite, text);
    if(gender == GENMALE)
        fprintf(filewrite, "MALE\n");
    if(gender == GENFEMALE)
        fprintf(filewrite, "FEMALE\n");
    if(gender == GENOTHER)
        fprintf(filewrite, "OTHER\n");
}

//--------------------------------------------------------------------------------------------
void fpairof(FILE* filewrite, char* text, int base, int rand)
{
    // ZZ> This function mimics fprintf in spitting out
    //     damage/stat pairs
    undo_pair(base, rand);
    fprintf(filewrite, text);
    fprintf(filewrite, "%4.2f-%4.2f\n", pairfrom, pairto);
}

//--------------------------------------------------------------------------------------------
void funderf(FILE* filewrite, char* text, char* usename)
{
    // ZZ> This function mimics fprintf in spitting out
    //     a name with underscore spaces
    char cTmp;
    int cnt;


    fprintf(filewrite, text);
    cnt = 0;
    cTmp = usename[0];
    cnt++;
    while(cTmp != 0)
    {
        if(cTmp == ' ')
        {
            fprintf(filewrite, "_");
        }
        else
        {
            fprintf(filewrite, "%c", cTmp);
        }
        cTmp = usename[cnt];
        cnt++;
    }
    fprintf(filewrite, "\n");
}

//--------------------------------------------------------------------------------------------
void get_message(FILE* fileread)
{
    // ZZ> This function loads a string into the message buffer, making sure it
    //     is null terminated.
    int cnt;
    char cTmp;
    char szTmp[256];


    if(msgtotal<MAXTOTALMESSAGE)
    {
        if(msgtotalindex>=MESSAGEBUFFERSIZE)
        {
            msgtotalindex = MESSAGEBUFFERSIZE-1;
        }
        msgindex[msgtotal]=msgtotalindex;
        fscanf(fileread, "%s", szTmp);
        szTmp[255] = 0;
        cTmp = szTmp[0];
        cnt = 1;
        while(cTmp!=0 && msgtotalindex<MESSAGEBUFFERSIZE-1)
        {
            if(cTmp=='_')  cTmp=' ';
            msgtext[msgtotalindex] = cTmp;
            msgtotalindex++;
            cTmp = szTmp[cnt];
            cnt++;
        }
        msgtext[msgtotalindex]=0;  msgtotalindex++;
        msgtotal++;
    }
}

//--------------------------------------------------------------------------------------------
void load_all_messages(char *loadname, int object)
{
    // ZZ> This function loads all of an objects messages
    FILE *fileread;


    madmsgstart[object] = 0;
    fileread = fopen(loadname, "r");
    if(fileread)
    {
        madmsgstart[object] = msgtotal;
        while(goto_colon_yesno(fileread))
        {
            get_message(fileread);
        }
        fclose(fileread);
    }
}


//--------------------------------------------------------------------------------------------
void reset_teams()
{
    // ZZ> This function makes everyone hate everyone else
    int teama, teamb;


    teama = 0;
    while(teama < MAXTEAM)
    {
        // Make the team hate everyone
        teamb = 0;
        while(teamb < MAXTEAM)
        {
            teamhatesteam[teama][teamb] = btrue;
            teamb++;
        }
        // Make the team like itself
        teamhatesteam[teama][teama] = bfalse;
        // Set defaults
        teamleader[teama] = NOLEADER;
        teamsissy[teama] = 0;
        teammorale[teama] = 0;
        teama++;
    }


    // Keep the null team neutral
    teama = 0;
    while(teama < MAXTEAM)
    {
        teamhatesteam[teama][NULLTEAM] = bfalse;
        teamhatesteam[NULLTEAM][teama] = bfalse;
        teama++;
    }
}

//--------------------------------------------------------------------------------------------
void reset_messages()
{
    // ZZ> This makes messages safe to use
    int cnt;

    msgtotal=0;
    msgtotalindex=0;
    msgtimechange=0;
    msgstart=0;
    cnt = 0;
    while(cnt < MAXMESSAGE)
    {
        msgtime[cnt] = 0;
        cnt++;
    }
    cnt = 0;
    while(cnt < MAXTOTALMESSAGE)
    {
        msgindex[cnt] = 0;
        cnt++;
    }
    msgtext[0] = 0;
}

//--------------------------------------------------------------------------------------------
void make_randie()
{
    // ZZ> This function makes the random number table
    int tnc, cnt;


    // Fill in the basic values
    cnt = 0;
    while(cnt < MAXRAND)
    {
        randie[cnt] = rand()<<1;
        cnt++;
    }


    // Keep adjusting those values
    tnc = 0;
    while(tnc < 20)
    {
        cnt = 0;
        while(cnt < MAXRAND)
        {
            randie[cnt] += rand();
            cnt++;
        }
        tnc++;
    }

    // All done
    randindex = 0;
}

//--------------------------------------------------------------------------------------------
void reset_timers()
{
    // ZZ> This function resets the timers...
    sttclock = SDL_GetTicks();
    allclock = 0;
    lstclock = 0;
    wldclock = 0;
    statclock = 0;
    pitclock = 0;  pitskill = bfalse;
    wldframe = 0;
    allframe = 0;
    fpsframe = 0;
    outofsync = bfalse;
}

void menu_loadInitialMenu();

extern int doMenu(float deltaTime);
extern int initMenus();

//--------------------------------------------------------------------------------------------
int SDL_main(int argc, char **argv)
{
    // ZZ> This is where the program starts and all the high level stuff happens
    struct glvector t1={0,0,0};
    struct glvector t2={0,0,-1};
    struct glvector t3={0,1,0};
	double frameDuration;
	int menuActive = 1;
	int menuResult;

	// Initialize logging first, so that we can use it everywhere.
	log_init();
	log_setLoggingLevel(2);

	// start initializing the various subsystems
	log_message("Starting Egoboo %s...\n", VERSION);

	sys_initialize();
	clock_init();
	fs_init();

    read_setup("setup.txt");
    read_all_tags("basicdat/scancode.txt");
    read_controls("controls.txt");
    reset_ai_script();
    load_ai_codes("basicdat/aicodes.txt");
    load_action_names("basicdat/actions.txt");

    sdlinit(argc,argv);
    glinit(argc,argv);
	net_initialize();
	ui_initialize("basicdat/Negatori.ttf", 24);
    sdlmixer_initialize();

	//Linking system
	log_info("Initializing module linking... ");
	if(link_build("basicdat/link.txt", LinkList)) log_message("Success!\n");
	else log_message("Failed!\n");

    if(!get_mesh_memory())
    {
        log_error("Reduce the maximum number of vertices!!!  See SETUP.TXT\n");
        return bfalse;
    }

    // Matrix init stuff (from remove.c)
    rotmeshtopside = ((float)scrx/scry)*ROTMESHTOPSIDE/(1.33333);
    rotmeshbottomside = ((float)scrx/scry)*ROTMESHBOTTOMSIDE/(1.33333);
    rotmeshup = ((float)scrx/scry)*ROTMESHUP/(1.33333);
    rotmeshdown = ((float)scrx/scry)*ROTMESHDOWN/(1.33333);
    mWorld = IdentityMatrix();
    mViewSave = ViewMatrix( t1,t2,t3,0 );
    mProjection = ProjectionMatrix(.001f, 2000.0f, (float)(FOV*PI/180)); // 60 degree FOV
    mProjection = MatrixMult(Translate(0, 0, -.999996), mProjection); // Fix Z value...
    mProjection = MatrixMult(ScaleXYZ(-1, -1, 100000), mProjection);  // HUK // ...'cause it needs it

	//[claforte] Fudge the values.
	mProjection.v[10] /= 2.0;
	mProjection.v[11] /= 2.0;

	//Load stuff into memory
    prime_icons();
    prime_titleimage();
    make_textureoffset();  // THIS SHOULD WORK
    make_lightdirectionlookup(); // THIS SHOULD WORK
    make_turntosin();  // THIS SHOULD WORK
    make_enviro(); // THIS SHOULD WORK
    load_mesh_fans(); // THIS SHOULD WORK
    load_blip_bitmap();
	load_all_menu_images();
	load_all_music_sounds();
	initMenus();				//Start the game menu

	// Let the normal OS mouse cursor work
	SDL_WM_GrabInput(SDL_GRAB_OFF);
	SDL_ShowCursor(1);

	// Network's temporarily disabled

	clock_frameStep();
	frameDuration = clock_getFrameDuration();
    gameactive = btrue;

    while(gameactive || menuActive)
    {
		// Clock updates each frame
		clock_frameStep();
		frameDuration = clock_getFrameDuration();

		// Either run through the menus, or jump into the game
		if(menuActive)
		{
			//Play the menu music
			play_music(0, 500, -1);

			// do menus
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			read_input();

			//Pressed panic button
			if(SDLKEYDOWN(SDLK_q) && SDLKEYDOWN(SDLK_LCTRL))
			{
				menuActive = 0;
				gameactive = bfalse;
			}

			ui_beginFrame(frameDuration);

			menuResult = doMenu((float)frameDuration);
			switch(menuResult)
			{
			case 1:
				// Go ahead and start the game
				menuActive = 0;
				gameactive = btrue;
				networkon = bfalse;
				hostactive = btrue;

				if ( gGrabMouse )
				{
					SDL_WM_GrabInput ( SDL_GRAB_ON );
				}

				if ( gHideMouse )
				{
					SDL_ShowCursor(0);	// Hide the mouse cursor
				}
				break;

			case -1:
				// The user selected "Quit"
				menuActive = 0;
				gameactive = bfalse;
				break;
			}

			ui_endFrame();

			SDL_GL_SwapBuffers();
		} else
		{
			// Do the game
			// Did we get through all the menus?
			if(gameactive)
			{
				//printf("MENU: game is now active\n");
				// Start a new module
				seed = time(NULL);
				srand(seed);


				load_module(pickedmodule);  // :TODO: Seems to be the next part to fix


				pressed = bfalse;
				make_onwhichfan();
				reset_camera();
				reset_timers();
				figure_out_what_to_draw();
				make_character_matrices();
				attach_particles();

				if(networkon)
				{
					log_info("SDL_main: Loading module %s...\n", pickedmodule);
					netmessagemode = bfalse;
					netmessagedelay = 20;
					net_sayHello();
				}

				// Let the game go
				moduleactive = btrue;
				randsave = 0;
				srand(0);
				//printf("moduleactive: %d\n", moduleactive);
				while(moduleactive)
				{
					// This is the control loop
					read_input();
					input_net_message();
					check_screenshot();


					if ( !SDLKEYDOWN( SDLK_F8 ) ) pausekeyready = btrue;

					//Todo zefz: where to put this?
					//Check for pause key		//TODO: What to do in network games?
					if(SDLKEYDOWN(SDLK_F8) && keyon && pausekeyready)
					{
						pausekeyready = bfalse;
						if(gamepaused) gamepaused = bfalse;
						else gamepaused = btrue;
					}

					// Do important things
					if(!gamepaused || networkon)
					{
						check_stats();
						set_local_latches();
						update_timers();
						check_passage_music();

						// NETWORK PORT
						listen_for_packets();
						if(waitingforplayers == bfalse)
						{
							cl_talkToHost();
							update_game();
						}
						else
						{

							wldclock = allclock;
						}
					}
					else
					{
						update_timers();
						wldclock = allclock;
					}

					// Do the display stuff
					move_camera();
					figure_out_what_to_draw();
					//printf("DIAG: doing draw_main\n");
					draw_main();

					// Check for quitters
					// :TODO: nolocalplayers is not set correctly
					if(SDLKEYDOWN(SDLK_ESCAPE) /*|| nolocalplayers*/ )
					{
						quit_module();
						gameactive = bfalse;
						menuActive = 1;

						// Let the normal OS mouse cursor work
						SDL_WM_GrabInput(SDL_GRAB_OFF);
						SDL_ShowCursor(1);
					}
				}
				release_module();
				close_session();
			}
		}
    }

    quit_game();
	Mix_CloseAudio();
    release_grfx();
	ui_shutdown();
	net_shutDown();
	clock_shutdown();
	sys_shutdown();

    return btrue;
}
