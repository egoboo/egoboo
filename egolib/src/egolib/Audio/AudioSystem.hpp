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

    int maxsoundchannel;      ///< Max number of sounds playing at the same time
    int buffersize;           ///< Buffer size set in setup.txt
    bool highquality;          ///< Allow CD quality frequency sounds?

    SoundConfiguration() :
        enableSound(false),
        soundVolume(75),
        enableMusic(false),
        musicVolume(50),
        maxsoundchannel(16),
        buffersize(2048),
        highquality(false)
    {
        /* Nothing do to. */
    }
    void download(egoboo_config_t& cfg)
    {
        enableSound = cfg.sound_effects_enable.getValue();
        soundVolume = cfg.sound_effects_volume.getValue();
        enableMusic = cfg.sound_music_enable.getValue();
        musicVolume = cfg.sound_music_volume.getValue();
        maxsoundchannel = CLIP<uint16_t>(cfg.sound_channel_count.getValue(), 8, 128);
        buffersize = CLIP<uint16_t>(cfg.sound_outputBuffer_size.getValue(), 512, 8196);
        highquality = cfg.sound_highQuality_enable.getValue();
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
        /* Nothing to do. */
    }

    LoopingSound(const LoopingSound&) = delete;
    LoopingSound& operator=(const LoopingSound&) = delete;

    inline int getChannel() const
    {
        return _channel;
    }
    
    inline CHR_REF getOwner() const
    {
        return _object;
    }
    
    inline void setChannel(int channel)
    {
        _channel = channel;
    }

    inline SoundID getSoundID() const
    {
        return _soundID;
    }


private:
    int _channel;
    const CHR_REF _object;
    const SoundID _soundID;
};


/// Pre defined global particle sounds
enum GlobalSound : uint8_t
{
    GSND_COINGET,       //Coin grabbed
    GSND_DEFEND,        //Attack deflected clink
    GSND_SPLISH,    	//Raindrop
    GSND_SPLOSH,		//Hit water
    GSND_COINFALL,		//Coin hits ground
    GSND_LEVELUP,		//Character gains level
    GSND_PITFALL,		//Character falls down a pit
    GSND_SHIELDBLOCK,	//Shield block sound
    GSND_BUTTON_CLICK,	//GUI button clicked
    GSND_GAME_READY,	//Finished loading module
    GSND_PERK_SELECT,   //Selected new perk
    GSND_GUI_HOVER,     //Mouse over sound effect
    GSND_DODGE,         //Dodged attack
    GSND_CRITICAL_HIT,  //Critical Hit
    GSND_DISINTEGRATE,  //Disintegrated
    GSND_DRUMS,         //Used for "Too Silly to Die" perk
    GSND_ANGEL_CHOIR,   //Angel Choir
    GSND_COUNT
};

class AudioSystem : public Ego::Core::Singleton<AudioSystem>
{
public:
    static CONSTEXPR int MIX_HIGH_QUALITY = 44100;
    static CONSTEXPR size_t MENU_SONG = 0;

	// TODO: re-add constexpr when we drop VS 2013 support
	// Workaround for compilers without constexpr support (VS 2013);
	// only integral types can be initialized in-class for a const static member.
	static const float DEFAULT_MAX_DISTANCE;    ///< Default max hearing distance (10 tiles)

protected:
    // Befriend with the singleton to grant access to AudioSystem::~AudioSystem.
    using TheSingleton = Ego::Core::Singleton<AudioSystem>;
    friend TheSingleton;

    AudioSystem();
    virtual ~AudioSystem();

public:
    void loadGlobalSounds();

    /**
     * @brief
     *  Stop music.
     */
    void stopMusic();

    /**
     * @brief
     *  Resume music.
     */
    void resumeMusic();

    /**
     * Stop all sounds that are playing over time.
     */
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
     * @brief
     *  Updates all looping sounds (called preiodically by the game engine).
     */
    void updateLoopingSounds();

    /**
     *  @author ZF
     *  @details stops looping of a specific sound for all characters.
     *           If soundID is INVALID_SOUND_ID, then all sounds belonging to this character is stopped
     **/
    bool stopObjectLoopingSounds(const CHR_REF ichr, const SoundID soundID = INVALID_SOUND_ID);

    /**
     * @todo
     *  MH: "reconfigure" should be offered by all sub-systems, it is a useful concept.
     * @param cfg
     *  the configuration
     * @remark
     *  The audio system will adjust its settings to the specified configuration.
     *  and update the given configuration with information about its new settings.
     */
    void reconfigure(egoboo_config_t &cfg);

    void freeAllMusic();
    void freeAllSounds();

    /// @author ZF
    /// @details This function plays a specified sound and returns which channel it's using
    int playSound(const fvec3_t& snd_pos, const SoundID soundID);

    /**
     * @brief
     *  Loops the specified sound until it is explicitly stopped.
     */
    void playSoundLooped(const SoundID soundID, const CHR_REF owner);

    /// @author ZF
    /// @details This function plays a specified sound at full possible volume and returns which channel it's using
    int playSoundFull(SoundID soundID);

    inline SoundID getGlobalSound(GlobalSound id) const
    {
        return _globalSounds[id];
    }

    inline MusicID getCurrentMusicPlaying() const
    {
        return _currentSongPlaying;
    }

    /**
    * @brief
    *   sets the maximum distance from where sounds will be played.
    *   Sounds played far away will have lower volume than those nearby
    * @param distance
    *    distance to hear the sound. Normally a multiple of GRID_FSIZE
    *    e.g GRID_FSIZE*5 means 5 tiles.
    * @remark
    *   Default value is a the DEFAULT_MAX_DISTANCE constant found in this class
    **/
    void setMaxHearingDistance(const float distance);

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

    SoundConfiguration _audioConfig;
    std::vector<Mix_Music*> _musicLoaded;
    std::vector<Mix_Chunk*> _soundsLoaded;
    std::array<SoundID, GSND_COUNT> _globalSounds;

    std::forward_list<std::shared_ptr<LoopingSound>> _loopingSounds;
    MusicID _currentSongPlaying;
    float _maxSoundDistance;                                            ///< How far away can we hear sound effects?
};
