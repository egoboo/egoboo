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
// @Author Johan Jansen
//

#pragma once

#include <SDL_mixer.h>
#include "egolib/egoboo_setup.h"
#include "egolib/Math/_Include.hpp"
#if 0
#include "game/Entities/_Include.hpp"
#endif

typedef int MusicID;
typedef int SoundID;

CONSTEXPR int INVALID_SOUND_CHANNEL = -1;
CONSTEXPR SoundID INVALID_SOUND_ID = -1;

//Sound settings
struct SoundConfiguration
{
public:
    /**
     * @brief
     *  Enable/disable playing of sounds.
     */
	bool enableSound;
	uint8_t soundVolume;          ///< Volume of sounds.

    /**
     * @brief
     *  Enable/disable playing of music and loops.
     * @todo
     *  Music and loops should be seperated.
     */
	bool enableMusic;
	uint8_t musicVolume;         ///< The volume of music.

	int        maxsoundchannel;      ///< Max number of sounds playing at the same time
	int        buffersize;           ///< Buffer size set in setup.txt
	bool       highquality;          ///< Allow CD quality frequency sounds?

	SoundConfiguration() :
    	enableSound(false),
    	soundVolume(75),  
    	enableMusic(false),
    	musicVolume(50),  
    	maxsoundchannel(16),
    	buffersize(2048),
    	highquality(false)
	{
		// ctor to safe default values
	}
};

/// Data needed to store and manipulate a looped sound
class LoopingSound
{
public:
	LoopingSound(CHR_REF ref, SoundID soundID) :
		_channel(INVALID_SOUND_CHANNEL),
		_object(ref),
		_soundID(soundID)
	{
		//ctor
	}

    LoopingSound(const LoopingSound&) = delete;
    LoopingSound& operator=(const LoopingSound&) = delete;

	inline int getChannel() const {return _channel;}
	inline CHR_REF getOwner() const {return _object;}
	inline void setChannel(int channel) {_channel = channel;}
	inline SoundID getSoundID() const {return _soundID;}


private:
    int     	  _channel;
    const CHR_REF _object;
    const SoundID _soundID;
};


/// Pre defined global particle sounds
enum GlobalSound : uint8_t
{
    GSND_GETCOIN = 0,	//Grab money
    GSND_DEFEND,		//Immune clink
    GSND_SPLISH,		//Raindrop
    GSND_SPLOSH,		//Hit water
    GSND_COINFALL,		//Coin hits ground
    GSND_LEVELUP,		//Character gains level
    GSND_PITFALL,		//Character falls down a pit
    GSND_SHIELDBLOCK,	//Shield block sound
    GSND_BUTTON_CLICK,	//GUI button clicked
    GSND_GAME_READY,	//Finished loading module
    GSND_COUNT
};

class AudioSystem
{
public:
	static CONSTEXPR int MIX_HIGH_QUALITY = 44100;
	static CONSTEXPR size_t MENU_SONG = 0;

public:
	AudioSystem();
	~AudioSystem();

	bool initialize(const egoboo_config_t &pcfg);
    void uninitialize();

	void loadGlobalSounds();

	void reset();

	/// @author ZF
    /// @details This function sets music track to pause
	void stopMusic();
	
	void resumeMusic();

	/**
	* Stop all sounds that are playing
	**/
	void fadeAllSounds();

	/**
	* @author ZF
    * @details This functions plays a specified track loaded into memory
    **/
	void playMusic(const MusicID musicID, const uint16_t fadetime = 0);

	SoundID loadSound(const std::string &fileName);

	/// @author ZF
    /// @details This function loads all of the music sounds
	void loadAllMusic();

	/**
	* @brief Updates all looping sounds (called preiodically by the game engine)
	**/
	void updateLoopingSounds();

    /** 
    *  @author ZF
    *  @details stops looping of a specific sound for all characters.
    *			If soundID is INVALID_SOUND_ID, then all sounds belonging to this character is stopped
    **/
	bool stopObjectLoopingSounds(const CHR_REF ichr, const SoundID soundID = INVALID_SOUND_ID);

	void setConfiguration(const egoboo_config_t &pcfg);

	void freeAllSounds();

    /// @author ZF
    /// @details This function plays a specified sound and returns which channel it's using
    int playSound(const fvec3_t snd_pos, const SoundID soundID);

    /**
    * @brief Loops the specified sound until it is explicitly stopped
    **/
    void playSoundLooped(const SoundID soundID, const CHR_REF owner);

    /// @author ZF
    /// @details This function plays a specified sound at full possible volume and returns which channel it's using
	int playSoundFull(SoundID soundID);

	inline SoundID getGlobalSound(GlobalSound id) const {return _globalSounds[id];}

	inline MusicID getCurrentMusicPlaying() const {return _currentSongPlaying;}

private:
	/**
	* @brief Loads one music track. Returns nullptr if it fails
	**/
	MusicID loadMusic(const std::string &fileName);

	/**
	 * @brief applies 3D spatial effect to the specified sound (using volume and panning)
     * @param channel
     *  the channel
     * @param distance
     *  the distance between the sound origin and the listener
     * @param soundPosition
     *  the sound origin
	 */
	void mixAudioPosition3D(const int channel, float distance, const fvec3_t soundPosition);

	/**
     * @brief
     *  Calculates distance between the sound origin and the players.
     * @param soundPosition
     *  the sound origin
     * @return
     *  the distance between the sound origin and the players
	 */
	float getSoundDistance(const fvec3_t soundPosition);

	/**
	 * @brief
     *  Updates one looping sound effect.
     * @param sound
     *  the looping sound effect
	 */
	void updateLoopingSound(const std::shared_ptr<LoopingSound>& sound);

private:
	bool _initialized;
	SoundConfiguration _audioConfig;
	std::vector<Mix_Music*> _musicLoaded;
	std::vector<Mix_Chunk*> _soundsLoaded;
	std::array<SoundID, GSND_COUNT> _globalSounds;

	std::forward_list<std::shared_ptr<LoopingSound>> _loopingSounds;
	MusicID _currentSongPlaying;
};

/// @todo Remove this global.
extern AudioSystem  _audioSystem;
