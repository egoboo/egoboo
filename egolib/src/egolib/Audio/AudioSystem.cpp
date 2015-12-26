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
/// @author Johan Jansen

#include "egolib/Audio/AudioSystem.hpp"

#include "game/Graphics/CameraSystem.hpp"
#include "game/game.h"
#include "game/char.h"

//Compatability fix for older versions of SDL
#if SDL_VERSIONNUM(SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL) < SDL_VERSIONNUM(1, 2, 12)
Mix_Music *Mix_LoadMUSType_RW(SDL_RWops *rw, Mix_MusicType, int freesrc) {
    Mix_Music *ret = Mix_LoadMUS_RW(rw);
    if (rw && freesrc) SDL_RWclose(rw);
    return ret;
}
#endif

// TODO: move back to the header file when VS 2013 support is dropped
const float AudioSystem::DEFAULT_MAX_DISTANCE = 128.0f * 10.0f; //GRID_FSIZE*10.0f

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
    "shieldblock",
    "button",
    "game_ready",
    "perk_select",
    "gui_hover",
    "dodge",
    "critical_hit",
    "disintegrate",
    "drums",
    "angel_choir",
    "stealth",
    "stealth_end"
};

AudioSystem::AudioSystem() :
    _musicLoaded(),
    _musicIDToNameMap(),
    _soundsLoaded(),
    _globalSounds(),
    _loopingSounds(),
    _currentSongPlaying(),
    _maxSoundDistance(DEFAULT_MAX_DISTANCE)
{
    _globalSounds.fill(INVALID_SOUND_ID);

    // Initialize SDL mixer.
    if (egoboo_config_t::get().sound_effects_enable.getValue() || egoboo_config_t::get().sound_music_enable.getValue())
    {
        const SDL_version* link_version = Mix_Linked_Version();
		Log::get().info("initializing SDL mixer audio services version %d.%d.%d ... ", link_version->major, link_version->minor, link_version->patch);
        if (Mix_OpenAudio(egoboo_config_t::get().sound_highQuality_enable.getValue() ? MIX_HIGH_QUALITY : MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, egoboo_config_t::get().sound_outputBuffer_size.getValue()) < 0)
        {
			Log::get().message(" failure!\n");
			Log::get().warn("unable to initialize audio: \"%s\"\n", Mix_GetError());
            throw std::runtime_error("unable to initialize audio system");
        }
        else
        {
            setMusicVolume(egoboo_config_t::get().sound_music_volume.getValue());
            Mix_AllocateChannels(egoboo_config_t::get().sound_channel_count.getValue());

            //Check if we can load OGG Vorbis music (this is non-fatal, game runs fine without music)
            if (!Mix_Init(MIX_INIT_OGG)) {
				Log::get().warn(" failed! (%s)\n", Mix_GetError());
            }
            else {
				Log::get().message(" ... success!\n");
            }
        }
    }
}

void AudioSystem::setMusicVolume(int value)
{
    Mix_VolumeMusic(Ego::Math::constrain(value, 0, MIX_MAX_VOLUME));
}

void AudioSystem::setSoundEffectVolume(int value)
{
    Mix_Volume(-1, Ego::Math::constrain(value, 0, MIX_MAX_VOLUME));
}

void AudioSystem::loadGlobalSounds()
{
    // Load global sounds.
    for (size_t i = 0; i < GSND_COUNT; ++i)
    {
        _globalSounds[i] = loadSound(std::string("mp_data/") + wavenames[i]);
        if (_globalSounds[i] == INVALID_SOUND_ID)
        {
			Log::get().warn("global sound not loaded: %s\n", wavenames[i]);
        }
    }

    // Load any override sounds in local .mod folder.
    for (size_t cnt = 0; cnt < GSND_COUNT; cnt++)
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
    for (auto& music : _musicLoaded) {
        Mix_FreeMusic(music.second);
    }
    _musicLoaded.clear();
    _musicIDToNameMap.clear();

    for (Mix_Chunk *chunk : _soundsLoaded)
    {
        Mix_FreeChunk(chunk);
    }
    _soundsLoaded.clear();
    _loopingSounds.clear();

	Mix_CloseAudio();
}

void AudioSystem::reconfigure(egoboo_config_t& cfg)
{
    // Clear all data.
    _loopingSounds.clear();

    // Restore audio if needed
    if (egoboo_config_t::get().sound_effects_enable.getValue() || egoboo_config_t::get().sound_music_enable.getValue())
    {
        if (-1 != Mix_OpenAudio(egoboo_config_t::get().sound_highQuality_enable.getValue() ? MIX_HIGH_QUALITY : MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, egoboo_config_t::get().sound_outputBuffer_size.getValue()))
        {
            Mix_AllocateChannels(egoboo_config_t::get().sound_channel_count.getValue());
        }
        else
        {
			Log::get().warn("AudioSystem::reset() - Cannot get AudioSystem to start. (%s)\n", Mix_GetError());
        }
    }

    //Reset max hearing distance to default
    _maxSoundDistance = DEFAULT_MAX_DISTANCE;

    // Do we restart the music?
    if (egoboo_config_t::get().sound_music_enable.getValue())
    {
        // Load music if required.
        loadAllMusic();

        // Start playing queued/paused song.
        if(!_currentSongPlaying.empty()) {
            const auto& result = _musicLoaded.find(_currentSongPlaying);
            if (result != _musicLoaded.end()) {
                Mix_HaltMusic();
                Mix_FadeInMusic(result->second, -1, 500);
            }            
        }
    }
}

void AudioSystem::stopMusic()
{
    if (egoboo_config_t::get().sound_music_enable.getValue())
    {
        Mix_FadeOutMusic(2000);
        //Mix_HaltMusic();
        _currentSongPlaying.clear();
    }
}

SoundID AudioSystem::loadSound(const std::string &fileName)
{
    // Valid filename?
    if (fileName.empty())
    {
		Log::get().warn("trying to load empty string sound");
        return INVALID_SOUND_ID;
    }

    // blank out the data
    std::string fullFileName;
    Mix_Chunk* loadedSound = nullptr;
    bool fileExists = false;

    // try an ogg file
    fullFileName = fileName + ".ogg";
    if (vfs_exists(fullFileName.c_str()))
    {
        fileExists = true;
        loadedSound = Mix_LoadWAV_RW(vfs_openRWopsRead(fullFileName.c_str()), 1);
    }

    //OGG failed, try WAV instead
    if (nullptr == loadedSound)
    {
        fullFileName = fileName + ".wav";
        if (vfs_exists(fullFileName.c_str()))
        {
            fileExists = true;
            loadedSound = Mix_LoadWAV_RW(vfs_openRWopsRead(fullFileName.c_str()), 1);
        }
    }

    if (nullptr == loadedSound)
    {
        // there is an error only if the file exists and can't be loaded
        if (fileExists) {
			Log::get().warn("Sound file not found/loaded %s.\n", fileName.c_str());
        }

        return INVALID_SOUND_ID;
    }

    //Sound loaded!
    _soundsLoaded.push_back(loadedSound);
    return _soundsLoaded.size() - 1;
}

MusicID AudioSystem::loadMusic(const std::string &fileName)
{
    // Valid filename?
    if (fileName.empty()) {
        return INVALID_SOUND_ID;
    }

    Mix_Music* loadedMusic = Mix_LoadMUSType_RW(vfs_openRWopsRead(fileName.c_str()), MUS_NONE, 1);

    if (!loadedMusic)
    {
		Log::get().warn("Failed to load music (%s): %s.\n", fileName.c_str(), Mix_GetError());
        return INVALID_SOUND_ID;
    }

    // Got it!
    const std::string songName = fileName.substr(fileName.find_last_of('/') + 1);
    const MusicID id = _musicLoaded.size();
    _musicLoaded[songName] = loadedMusic;
    _musicIDToNameMap[id] = songName;

    return id;
}

void AudioSystem::playMusic(const std::string& songName, const uint16_t fadetime)
{ 
    // Dont restart a song we are already playing.
    if (songName == _currentSongPlaying) {
        return;
    }

    // Remember desired music.
   _currentSongPlaying = songName;

    if (!egoboo_config_t::get().sound_music_enable.getValue()) {
        return;
    }

    //Set music volume
    Mix_VolumeMusic(egoboo_config_t::get().sound_music_volume.getValue());

    //Get the actual music data from the name of the song
    const auto& result = _musicLoaded.find(songName);
    if(result == _musicLoaded.end()) {
        Log::get().warn("Failed to play music! (Song name does not exist: %s)\n", songName.c_str());        
        return;
    }

    // Mix_FadeOutMusic(fadetime);      // Stops the game too
    if (Mix_FadeInMusic(result->second, -1, fadetime) == -1) {
        Log::get().warn("Failed to play music! (%s)\n", Mix_GetError());
    }
}

void AudioSystem::playMusic(const int musicID, const uint16_t fadetime)
{
    //Is it a valid ID?
    const auto& result = _musicIDToNameMap.find(musicID);
    if (result != _musicIDToNameMap.end()) {
        playMusic(result->second, fadetime);
    }
}

void AudioSystem::loadAllMusic()
{
    if (!_musicLoaded.empty() || !egoboo_config_t::get().sound_music_enable.getValue()) return;

    // Open the playlist listing all music files
    ReadContext ctxt("mp_data/music/playlist.txt");
    if (!ctxt.ensureOpen())
    {
		Log::get().warn("Unable to read playlist file `%s`\n", ctxt.getLoadName().c_str());
        return;
    }

    // Load all music data into memory
    while (ctxt.skipToColon(true))
    {
        char songName[256];
        vfs_read_name(ctxt, songName, SDL_arraysize(songName));
        loadMusic(std::string("mp_data/music/") + songName + ".ogg");
    }

    //Special xmas theme, override the default menu theme song
    if (Zeitgeist::CheckTime(Zeitgeist::Time::Christmas))
    {
        loadMusic("mp_data/music/special/xmas.ogg");
    }
    else if (Zeitgeist::CheckTime(Zeitgeist::Time::Halloween))
    {
        loadMusic("mp_data/music/special/halloween.ogg");
    }
}

void AudioSystem::updateLoopingSound(const std::shared_ptr<LoopingSound> &sound)
{
    int channel = sound->getChannel();

    //skip dead stuff
    if (!_currentModule->getObjectHandler().exists(sound->getOwnerRef())) {

        //Stop loop if we just died
        if (channel != INVALID_SOUND_CHANNEL) {
            Mix_HaltChannel(channel);
            sound->setChannel(INVALID_SOUND_CHANNEL);
        }

        return;
    }

    const Vector3f soundPosition = _currentModule->getObjectHandler().get(sound->getOwnerRef())->getPosition();
    const float distance = getSoundDistance(soundPosition);

    //Sound is close enough to be heard?
    if (distance < _maxSoundDistance)
    {
        //No channel allocated to this sound yet? try to allocate a free one
        if (channel == INVALID_SOUND_CHANNEL) {
            channel = Mix_PlayChannel(-1, _soundsLoaded[sound->getSoundID()], -1);
        }

        //Update sound effects
        if (channel != INVALID_SOUND_CHANNEL) {
            mixAudioPosition3D(channel, distance, soundPosition);
            sound->setChannel(channel);
        }
    }
    else if (channel != INVALID_SOUND_CHANNEL)
    {
        //We are too far away to hear sound, stop it and free 
        //channel until we come closer again
        Mix_HaltChannel(channel);
        sound->setChannel(INVALID_SOUND_CHANNEL);
    }
}

void AudioSystem::updateLoopingSounds()
{
    for (const std::shared_ptr<LoopingSound> &sound : _loopingSounds)
    {
        updateLoopingSound(sound);
    }
}

size_t AudioSystem::stopObjectLoopingSounds(ObjectRef ownerRef, const SoundID soundID) {
	if (!_currentModule->getObjectHandler().exists(ownerRef)) {
		return 0;
	}
	size_t removedLoopCount = 0;
    std::forward_list<std::shared_ptr<LoopingSound>> removeLoops;
    for (const std::shared_ptr<LoopingSound> &sound : _loopingSounds) {
        // Either the sound ID must match or if INVALID_SOUND_ID is given,
        // stop all sounds that this character owns.
        if (soundID != INVALID_SOUND_ID && sound->getSoundID() != soundID) {
            continue;
        }
        if (sound->getOwnerRef() == ownerRef) {
            removeLoops.push_front(sound);
			removedLoopCount++;
        }
    }

    // Remove all looping sounds from list.
    for (const std::shared_ptr<LoopingSound> &sound : removeLoops) {
        _loopingSounds.remove(sound);
        Mix_HaltChannel(sound->getChannel());
    }

    return removedLoopCount;
}

void AudioSystem::fadeAllSounds()
{
    // Stop all sounds that are playing.
    Mix_FadeOutChannel(-1, 500);
}

int AudioSystem::playSoundFull(SoundID soundID)
{
    if (soundID < 0 || soundID >= _soundsLoaded.size())
    {
        return INVALID_SOUND_CHANNEL;
    }

    if (!egoboo_config_t::get().sound_effects_enable.getValue())
    {
        return INVALID_SOUND_CHANNEL;
    }

    // play the sound
    int channel = Mix_PlayChannel(-1, _soundsLoaded[soundID], 0);

    if (channel != INVALID_SOUND_CHANNEL) {
        //remove any 3D positional mixing effects
        Mix_SetPosition(channel, 0, 0);

        // we are still limited by the global sound volume
        Mix_Volume(channel, (128 * egoboo_config_t::get().sound_effects_volume.getValue()) / 100);
    }

    return channel;
}

float AudioSystem::getSoundDistance(const Vector3f& soundPosition)
{
    //Pick the camera that is nearest to the sound
    float distance = std::numeric_limits<float>::max();
    for(const std::shared_ptr<Camera> &camera : CameraSystem::get()->getCameraList()) {
        Vector3f cameraPosition = Vector3f(camera->getCenter().x(), camera->getCenter().y(), camera->getPosition().z());
        distance = std::min(distance, (cameraPosition - soundPosition).length());
    }

    //Calculate distance between camera and sound origin
    return distance;
}

void AudioSystem::mixAudioPosition3D(const int channel, float distance, const Vector3f& soundPosition)
{
    //Calculate the average position and rotation of all cameras
    Vector2f averageCameraPosition = Vector2f::zero();
    float averageRotation = 0.0f;
    for(const std::shared_ptr<Camera> &camera : CameraSystem::get()->getCameraList()) {
        averageCameraPosition.x() += camera->getCenter().x();
        averageCameraPosition.y() += camera->getCenter().y();
        averageRotation += camera->getTurnZ_turns();
    }
    averageCameraPosition *= 1.0f / CameraSystem::get()->getCameraList().size();
    averageRotation /= CameraSystem::get()->getCameraList().size();

    //Scale distance (0 is very close 255 is very far away)
    distance *= 255.0f / _maxSoundDistance;

    //Calculate angle from camera to sound origin
    float angle = std::atan2(averageCameraPosition.y() - soundPosition.y(), averageCameraPosition.x() - soundPosition.x());

    //Adjust for camera rotation
    angle += Ego::Math::TurnsToRadians(averageRotation);

    //limited global sound volume
    Mix_Volume(channel, (128 * egoboo_config_t::get().sound_effects_volume.getValue()) / 100);

    //Do 3D sound mixing
    Mix_SetPosition(channel, angle, distance);
}

void AudioSystem::playSoundLooped(const SoundID soundID, ObjectRef ownerRef)
{
    // Avoid invalid characters
    if (!_currentModule->getObjectHandler().exists(ownerRef)) {
        return;
    }

    // Check for invalid sounds
    if (soundID < 0 || soundID >= _soundsLoaded.size()) {
        return;
    }

    //Only allow one looping sound instance per character
    for (const std::shared_ptr<LoopingSound> &sound : _loopingSounds)
    {
        if (sound->getOwnerRef() == ownerRef && sound->getSoundID() == soundID) {
            return;
        }
    }

    //Create new looping sound
    std::shared_ptr<LoopingSound> sound = std::make_shared<LoopingSound>(ownerRef, soundID);

    // add the sound to the LoopedList
    _loopingSounds.push_front(sound);

    //First time update
    updateLoopingSound(sound);
}

int AudioSystem::playSound(const Vector3f& snd_pos, const SoundID soundID)
{
    // If the sound ID is not valid ...
    if (soundID < 0 || soundID >= _soundsLoaded.size())
    {
        // ... return invalid channel.
        return INVALID_SOUND_CHANNEL;
    }

    // If sound is not enabled ...
    if (!egoboo_config_t::get().sound_effects_enable.getValue())
    {
        // ... return invalid channel.
        return INVALID_SOUND_CHANNEL;
    }

    // Don't play sounds until the camera has been properly initialized.
    if (!CameraSystem::get())
    {
        return INVALID_SOUND_CHANNEL;
    }

    // Get distance from sound to camera.
    float distance = getSoundDistance(snd_pos);

    // Outside hearing distance?
    if (distance > _maxSoundDistance)
    {
        return INVALID_SOUND_CHANNEL;
    }

    // Play the sound once
    int channel = Mix_PlayChannel(-1, _soundsLoaded[soundID], 0);

    // could fail if no free channels are available.
    if (INVALID_SOUND_CHANNEL != channel)
    {
        // Apply 3D positional sound effect.
        mixAudioPosition3D(channel, distance, snd_pos);
    }
    else
    {
        /// @note ZF@> disabled this warning because this happens really often
        //log_debug( "Unable to play sound. (%s)\n", Mix_GetError() );
    }

    return channel;
}

void AudioSystem::setMaxHearingDistance(const float distance)
{
    if(distance > 0.0f) {
        _maxSoundDistance = distance;
    }
}