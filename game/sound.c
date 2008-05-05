/* Egoboo - sound.c
 * Sound code in Egoboo is implemented using SDL_mixer.
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
#include "Log.h"

//This function enables the use of SDL_Mixer functions, returns btrue if success
bool_t sdlmixer_initialize()
{
	if((musicvalid || soundvalid) && !mixeron)
	{
		log_info("Initializing SDL_mixer audio services...\n");
	    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, buffersize);
		Mix_VolumeMusic(musicvolume);	//*1.28
		Mix_AllocateChannels(maxsoundchannel);
		if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, buffersize) != 0)
		{
			log_error("Unable to initialize audio: %s\n", Mix_GetError());
		}
		mixeron = btrue;
		return btrue;
	}
	else return bfalse;
}

//------------------------------------
//SOUND-------------------------------
//------------------------------------
int play_sound(float xpos, float ypos, Mix_Chunk *loadedwave)
{
	//This function plays a specified sound
    if (soundvalid)
    {
        int distance, volume, pan;
        distance = sqrt(pow(ABS(camtrackx-xpos), 2)+pow(ABS(camtracky-ypos), 2)); //Ugly, but just the distance formula
        volume = ((distance / VOLUMERATIO) * (1 + (soundvolume / 100))); //adjust volume with ratio and sound volume
        pan = 57.3 * ((1.5*PI)-atan2(camy-ypos, camx-xpos) - camturnleftright); //Convert the camera angle to the nearest integer degree
        if (pan < 0) pan += 360;
        if (pan > 360) pan -= 360;
        if(volume < 255)
        {
			if(loadedwave == NULL)
			{
				log_warning("Sound file not correctly loaded (Not found?).\n");
			}

			channel = Mix_PlayChannel(-1, loadedwave, 0);
            if (channel !=-1)
            {
                Mix_SetDistance(channel, volume);
                if (pan < 180)
                {
                    if (pan < 90) Mix_SetPanning(channel, 255 - (pan * 2.83),255);
                    else Mix_SetPanning(channel, 255 - ((180 - pan) * 2.83),255);
                }
                else
                {
                    if (pan < 270) Mix_SetPanning(channel, 255, 255 - ((pan - 180) * 2.83));
                    else Mix_SetPanning(channel, 255, 255 - ((360 - pan) * 2.83));
                }
            }
            else log_warning("All sound channels are currently in use. Sound is NOT playing.\n");
        }
    }
	return channel;
}

//TODO:
void stop_sound(int whichchannel)
{
	if(soundvalid) Mix_HaltChannel(whichchannel);
}

//------------------------------------------------------------------------------
//Music Stuff-------------------------------------------------------------------
//------------------------------------------------------------------------------
void load_all_music_sounds()
{
    //This function loads all of the music sounds
    char loadpath[128];
	char songname[128];
	FILE *playlist;
    int cnt;

	//Open the playlist listing all music files
	playlist = fopen("basicdat/music/playlist.txt", "r");
	if (playlist == NULL) log_warning("Error opening playlist.txt");

    // Load the music data into memory
    if(musicvalid && !musicinmemory)
    {
       cnt = 0;
	   while(cnt < MAXPLAYLISTLENGHT && !feof(playlist))
       {
		   goto_colon_yesno(playlist);
		   fscanf(playlist, "%s", songname);
	       sprintf(loadpath,("basicdat/music/%s"), songname);
           instrumenttosound[cnt] = Mix_LoadMUS(loadpath);
           cnt++;
        }
	    musicinmemory = btrue;
    }
	fclose(playlist);
}

void play_music(int songnumber, int fadetime, int loops)
{
	//This functions plays a specified track loaded into memory
	if(songplaying != songnumber && musicvalid)
	{
		Mix_FadeOutMusic(fadetime);
		Mix_PlayMusic(instrumenttosound[songnumber], loops);
		songplaying = songnumber;
	}
}

void stop_music()
{
	//This function sets music track to pause
	if(musicvalid)
	{
		Mix_HaltMusic();
	}
}