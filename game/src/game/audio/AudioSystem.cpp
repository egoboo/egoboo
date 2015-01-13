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
#include "game/audio/AudioSystem.hpp"

#include <cfloat>
#include "game/graphics/CameraSystem.hpp"
#include "game/game.h"

static const float MAX_DISTANCE = GRID_FSIZE * 10.0f;   ///< Max hearing distance (10 tiles)

// text filenames for the global sounds
static const std::array<const char*, GSND_COUNT> wavenames =
{
    "coinget",
    "defend",
    "splish",
    "splosh",
    "coinfall",
    "lvlup",
    "pitfall",
    "shieldblock"
};

AudioSystem::AudioSystem() :
	_initialized(false),
	_audioConfig(),
	_musicLoaded(),
	_soundsLoaded(),
	_globalSounds(),
	_loopingSounds(),
	_currentSongPlaying(INVALID_SOUND_CHANNEL)
{
	//ctor
    _globalSounds.fill(INVALID_SOUND_ID);
}


bool AudioSystem::initialize(const egoboo_config_t &pcfg)
{
	//Set configuration first
	setConfiguration(pcfg);

    //Initialize SDL Audio first
    // make sure that SDL audio is turned on
    if ( 0 == SDL_WasInit( SDL_INIT_AUDIO ) )
    {
        log_info( "Intializing SDL Audio... " );
        if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 )
        {
            log_message( "Failed!\n" );
            log_warning( "SDL error == \"%s\"\n", SDL_GetError() );

            _audioConfig.musicvalid = false;
            _audioConfig.soundvalid = false;
            return false;
        }
        else
        {
            log_message( "Success!\n" );
        }
    }

    //Next do SDL_Mixer
    if ( !_initialized && ( _audioConfig.musicvalid || _audioConfig.soundvalid ) )
    {
        const SDL_version* link_version = Mix_Linked_Version();
        log_info( "Initializing SDL_mixer audio services version %d.%d.%d... ", link_version->major, link_version->minor, link_version->patch );
        if ( Mix_OpenAudio( _audioConfig.highquality ? MIX_HIGH_QUALITY : MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, _audioConfig.buffersize ) < 0 )
        {
            _initialized = false;
            log_message( "Failure!\n" );
            log_warning( "Unable to initialize audio: %s\n", Mix_GetError() );
            return false;
        }
        else
        {
            Mix_VolumeMusic( _audioConfig.musicvolume );
            Mix_AllocateChannels( _audioConfig.maxsoundchannel );

            // initialize the music stack
            //music_stack_init();

            _initialized = true;

            log_message( "Success!\n" );
        }
    }

    return true;
}

void AudioSystem::loadGlobalSounds()
{
    //Load global sounds
    for(size_t i = 0; i < GSND_COUNT; ++i) {
        _globalSounds[i] = loadSound(std::string("mp_data/") + wavenames[i]);
    }

    //Load any override sounds in local .mod folder
    for(size_t cnt = 0; cnt < GSND_COUNT; cnt++ )
    {
        const std::string fileName = "mp_data/sound" + std::to_string(cnt);

        SoundID sound = loadSound(fileName);

        // only overwrite with a valid sound file
        if (sound != INVALID_SOUND_ID)
        {
            _globalSounds[cnt] = sound;
        }
    }
}

AudioSystem::~AudioSystem()
{
	if ( _initialized && ( 0 != SDL_WasInit( SDL_INIT_AUDIO ) ) )
    {
    	freeAllSounds();

        for(Mix_Music * music : _musicLoaded) {
            Mix_FreeMusic(music);
        }
        _musicLoaded.clear();

        Mix_CloseAudio();
        _initialized = false;
    }
}

void AudioSystem::reset()
{
    //Clear all data
    _loopingSounds.clear();

    //Restore audio if needed
    if(!_initialized)
    {
        if ( _audioConfig.musicvalid || _audioConfig.soundvalid )
        {
            if ( -1 != Mix_OpenAudio( _audioConfig.highquality ? MIX_HIGH_QUALITY : MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, _audioConfig.buffersize ) )
            {
                _initialized = true;
                Mix_AllocateChannels( _audioConfig.maxsoundchannel );
                Mix_VolumeMusic( _audioConfig.musicvolume );
            }
            else
            {
                log_warning( "AudioSystem::reset() - Cannot get AudioSystem to start. (%s)\n", Mix_GetError() );
            }
        }        
    }

    // Do we restart the music?
    if ( cfg.music_allowed )
    {
        //load music if required
        _audioSystem.loadAllMusic();
        
        //start playing queued/paused song
        if(_initialized && _currentSongPlaying >= 0 && _currentSongPlaying < _musicLoaded.size()) {
            Mix_FadeInMusic(_musicLoaded[_currentSongPlaying], -1, 500);
        }
    }
}

void AudioSystem::setConfiguration(const egoboo_config_t &pcfg)
{
    //Load config
    _audioConfig.soundvalid      = pcfg.sound_allowed;
    _audioConfig.soundvolume     = pcfg.sound_volume;
    _audioConfig.musicvalid      = pcfg.music_allowed;
    _audioConfig.musicvolume     = pcfg.music_volume;
    _audioConfig.maxsoundchannel = CLIP<uint16_t>(pcfg.sound_channel_count, 8, 128);
    _audioConfig.buffersize      = CLIP<uint16_t>(pcfg.sound_buffer_size, 512, 8196);
    _audioConfig.highquality     = pcfg.sound_highquality;
}

void AudioSystem::stopMusic()
{
    if ( _initialized && _audioConfig.musicvalid )
    {
    	Mix_FadeOutMusic(2000);
        //Mix_HaltMusic();
        _currentSongPlaying = INVALID_SOUND_CHANNEL;
    }
}

SoundID AudioSystem::loadSound(const std::string &fileName)
{
    //Can't load sounds if SDL_Mixer is not initialized
    if(!_initialized) {
    	return INVALID_SOUND_ID;
    }

    //Valid filename?
    if(fileName.empty()) {
    	return INVALID_SOUND_ID;
    }

    // blank out the data
    std::string fullFileName;
    Mix_Chunk* loadedSound = nullptr;
    bool fileExists = false;

    // try an ogg file
    fullFileName = fileName + ".ogg";
    if ( vfs_exists( fullFileName.c_str() ) )
    {
        fileExists = true;
        loadedSound = Mix_LoadWAV( vfs_resolveReadFilename( fullFileName.c_str() ) );
    }

    //OGG failed, try WAV instead
    if ( nullptr == loadedSound )
    {
    	fullFileName = fileName + ".wav";
        if ( vfs_exists( fullFileName.c_str() ) )
        {
            fileExists = true;
            loadedSound = Mix_LoadWAV( vfs_resolveReadFilename( fullFileName.c_str() ) );
        }
    }

    if ( fileExists && nullptr == loadedSound )
    {
        // there is an error only if the file exists and can't be loaded
        log_warning( "Sound file not found/loaded %s.\n", fileName.c_str() );
        return INVALID_SOUND_ID;
    }

    _soundsLoaded.push_back(loadedSound);

    return _soundsLoaded.size()-1;
}

MusicID AudioSystem::loadMusic(const std::string &fileName)
{
    //Can't load sounds if SDL_Mixer is not initialized
    if(!_initialized) {
    	return INVALID_SOUND_ID;
    }

    //Valid filename?
    if(fileName.empty()) {
    	return INVALID_SOUND_ID;
    }

    Mix_Music* loadedMusic = Mix_LoadMUS( fileName.c_str() );

    if(!loadedMusic) {
        log_warning( "Failed to load music (%s): %s.\n", fileName.c_str(), Mix_GetError() );
        return INVALID_SOUND_ID;
    }

    //Got it!
    _musicLoaded.push_back(loadedMusic);
    return _musicLoaded.size()-1;
}

void AudioSystem::playMusic(const int musicID, const uint16_t fadetime)
{
    //Dont restart a song we are already playing
    if(musicID == _currentSongPlaying) {
        return;
    }

    //Remember desired music
    _currentSongPlaying = musicID;

    if ( !_audioConfig.musicvalid || !_initialized ) return;

    if(musicID < 0 || musicID >= _musicLoaded.size()) {
    	return;
    }

    // Mix_FadeOutMusic(fadetime);      // Stops the game too
    if(Mix_FadeInMusic(_musicLoaded[musicID], -1, fadetime) == -1) {
        log_warning("failed to play music! %s\n", Mix_GetError());
    }
}

void AudioSystem::loadAllMusic()
{
    if ( !_musicLoaded.empty() || !_audioConfig.musicvalid ) return;

    // Open the playlist listing all music files
    vfs_FILE *playlist = vfs_openRead( "mp_data/music/playlist.txt" );
    if ( nullptr == playlist )
    {
        log_warning( "Error reading music list. (mp_data/music/playlist.txt)\n");
        return;
    }

    // Load all music data into memory
    while ( !vfs_eof( playlist ) )
    {
        if ( goto_colon_vfs( NULL, playlist, true ) )
        {
        	char songName[256];

            vfs_get_string( playlist, songName, SDL_arraysize( songName ) );

            std::string path = std::string("mp_data/music/") + songName;

            loadMusic( vfs_resolveReadFilename(path.c_str()) );
        }
    }

    //Special xmas theme, override the default menu theme song
    if ( check_time( SEASON_CHRISTMAS ) )
    {
        MusicID specialSong = loadMusic( vfs_resolveReadFilename("mp_data/music/special/xmas.ogg") );
        if(specialSong != INVALID_SOUND_ID)
        	_musicLoaded[MENU_SONG] = _musicLoaded[specialSong];
    }
    else if ( check_time( SEASON_HALLOWEEN ) )
    {
        MusicID specialSong = loadMusic( vfs_resolveReadFilename("mp_data/music/special/halloween.ogg") );
        if(specialSong != INVALID_SOUND_ID)
            _musicLoaded[MENU_SONG] = _musicLoaded[specialSong];
    }

    vfs_close( playlist );
}

void AudioSystem::updateLoopingSound(const std::shared_ptr<LoopingSound> &sound)
{
    int channel = sound->getChannel();

    //skip dead stuff
    if(!INGAME_CHR(sound->getOwner())) {

        //Stop loop if we just died
        if(channel != INVALID_SOUND_CHANNEL) {
            Mix_HaltChannel(channel);
            sound->setChannel(INVALID_SOUND_CHANNEL);
        }        

        return;
    }

    const fvec3_t soundPosition = ChrList.lst[sound->getOwner()].pos;
    const float distance = getSoundDistance(soundPosition);

    //Sound is close enough to be heard?
    if(distance < MAX_DISTANCE)
    {
        //No channel allocated to this sound yet? try to allocate a free one
        if(channel == INVALID_SOUND_CHANNEL) {
            channel = Mix_PlayChannel(-1, _soundsLoaded[sound->getSoundID()], -1);
        }

        //Update sound effects
        if(channel != INVALID_SOUND_CHANNEL) {
            mixAudioPosition3D(channel, distance, soundPosition);
            sound->setChannel(channel);
        }
    }
    else if(channel != INVALID_SOUND_CHANNEL)
    {
        //We are too far away to hear sound, stop it and free 
        //channel until we come closer again
        Mix_HaltChannel(channel);
        sound->setChannel(INVALID_SOUND_CHANNEL);
    }
}

void AudioSystem::updateLoopingSounds()
{
	for(const std::shared_ptr<LoopingSound> &sound : _loopingSounds)
    {
        updateLoopingSound(sound);
    }
}

void AudioSystem::stopLoopingSoundByChannel(int channel)
{
	if(channel < 0) {
		return;
	}

    std::forward_list<std::shared_ptr<LoopingSound>> removeLoops;
	for(const std::shared_ptr<LoopingSound> &sound : _loopingSounds)
    {
    	if(sound->getChannel() == channel) {
    		removeLoops.push_front(sound);
    	}
    }

    //Remove all looping sounds from list
    for(const std::shared_ptr<LoopingSound> &sound : removeLoops) {
    	_loopingSounds.remove(sound);
    	Mix_HaltChannel(sound->getChannel());
    }
}

bool AudioSystem::stopObjectLoopingSounds(const CHR_REF ichr, const SoundID soundID)
{
    if ( !ALLOCATED_CHR( ichr ) ) return false;

    std::forward_list<std::shared_ptr<LoopingSound>> removeLoops;
	for(const std::shared_ptr<LoopingSound> &sound : _loopingSounds)
    {
        if(soundID != INVALID_SOUND_ID && sound->getSoundID() != soundID) {
            continue;
        }

    	if(sound->getOwner() == ichr) {
    		removeLoops.push_front(sound);
    	}
    }

    //Remove all looping sounds from list
    for(const std::shared_ptr<LoopingSound> &sound : removeLoops) {
    	_loopingSounds.remove(sound);
    	Mix_HaltChannel(sound->getChannel());
    }

    return !removeLoops.empty();
}

void AudioSystem::fadeAllSounds()
{
    if(!_initialized) {
    	return;
    }

    // Stop all sounds that are playing
    Mix_FadeOutChannel(-1, 500);
}

void AudioSystem::freeAllSounds()
{
	for(Mix_Chunk *chunk : _soundsLoaded) {
		Mix_FreeChunk(chunk);
	}
	_soundsLoaded.clear();
	_loopingSounds.clear();
}

int AudioSystem::playSoundFull(SoundID soundID)
{
	if(soundID < 0 || soundID >= _soundsLoaded.size()) {
		return INVALID_SOUND_CHANNEL;
	}

    if ( !_audioConfig.soundvalid || !_initialized ) {
    	return INVALID_SOUND_CHANNEL;
    }

    // play the sound
    int channel = Mix_PlayChannel(-1, _soundsLoaded[soundID], 0);

    if(channel != INVALID_SOUND_CHANNEL) {
	    // we are still limited by the global sound volume
	    Mix_Volume(channel, ( 128*_audioConfig.soundvolume ) / 100);
    }

    return channel;
}

float AudioSystem::getSoundDistance(const fvec3_t soundPosition)
{
    const float cameraX = _cameraSystem.getMainCamera()->getCenter().x;
    const float cameraY = _cameraSystem.getMainCamera()->getCenter().y;
    const float cameraZ = _cameraSystem.getMainCamera()->getPosition().z;
    
    //Calculate distance between camera and sound origin
    return std::sqrt((cameraX-soundPosition.x)*(cameraX-soundPosition.x) + (cameraY-soundPosition.y)*(cameraY-soundPosition.y) + (cameraZ-soundPosition.z)*(cameraZ-soundPosition.z));
}

void AudioSystem::mixAudioPosition3D(const int channel, float distance, const fvec3_t soundPosition)
{    
    const float cameraX = _cameraSystem.getMainCamera()->getCenter().x;
    const float cameraY = _cameraSystem.getMainCamera()->getCenter().y;

    //Scale distance (0 is very close 255 is very far away)
    distance *= 255.0f / MAX_DISTANCE;

    //Calculate angle from camera to sound origin
    float angle = std::atan2(cameraY-soundPosition.y, cameraX-soundPosition.x);
    
    //Adjust for camera rotation
    angle += _cameraSystem.getMainCamera()->getTurnZOne() * 2.0f * PI;

    //limited global sound volume
    Mix_Volume(channel, ( 128*_audioConfig.soundvolume ) / 100);

    //Do 3D sound mixing
    Mix_SetPosition(channel, angle, distance);  
}

void AudioSystem::playSoundLooped(const SoundID soundID, const CHR_REF owner)
{
    //Avoid invalid characters
    if(!INGAME_CHR(owner)) {
        return;
    }

    //Create new looping sound
    std::shared_ptr<LoopingSound> sound = std::make_shared<LoopingSound>(soundID, owner);

    // add the sound to the LoopedList
    _loopingSounds.push_front(sound);

    //First time update
    updateLoopingSound(sound);
}

int AudioSystem::playSound(const fvec3_t snd_pos, const SoundID soundID)
{
    //Check SoundID first
	if(soundID < 0 || soundID >= _soundsLoaded.size()) {
		return INVALID_SOUND_CHANNEL;
	}

    //Make sure sound is valid
    if ( !_audioConfig.soundvalid || !_initialized ) {
    	return INVALID_SOUND_CHANNEL;
    }

    //Don't play sounds until the camera has been properly initialized
    if(!_cameraSystem.isInitialized()) {
        return false;
    }

    //Get distance from sound to camera
    float distance = getSoundDistance(snd_pos);

    //Outside hearing distance?
    if(distance > MAX_DISTANCE) {
        return INVALID_SOUND_CHANNEL;
    }
    
    // play the sound once
    int channel = Mix_PlayChannel(-1, _soundsLoaded[soundID], 0);

    //could fail if no free channels are available
    if ( INVALID_SOUND_CHANNEL != channel )
    {
        //Apply 3D positional sound effect
        mixAudioPosition3D(channel, distance, snd_pos);
    }
    else
    {
        /// @note ZF@> disabled this warning because this happens really often
        //log_debug( "Unable to play sound. (%s)\n", Mix_GetError() );
    }

    return channel;
}
