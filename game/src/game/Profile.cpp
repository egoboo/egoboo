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

/// @file  game/Profile.cpp
/// @brief ObjectProfile handling
/// @details
/// @author Johan Jansen

//ZF> TODO: check which headers can be removed
#include "egolib/bsp.h"
#include "game/Profile.hpp"
#include "game/graphic_texture.h"
#include "game/renderer_2d.h"
#include "game/script_compile.h"
#include "game/game.h"
#include "game/ChrList.h"
#include "game/PrtList.h"
#include "game/mesh.h"
#include "game/particle.h"
#include "game/mad.h"       //for loading md2
#include "game/audio/AudioSystem.hpp"

ObjectProfile::ObjectProfile() :
    _fileName("*NONE*"),
    _requestCount(0),
    _spawnCount(0),
    _icap(INVALID_CAP_REF),
    _imad(INVALID_MAD_REF),
    _ieve(INVALID_EVE_REF),
    _slotNumber(-1),

    _randomName(),
    _aiScript(),
    _particleProfiles{0},

    _texturesLoaded(),
    _iconsLoaded(),

    _messageList(),
    _soundList()
{
    _particleProfiles.fill(INVALID_PIP_REF);
    _soundList.fill(INVALID_SOUND_ID);

    chop_definition_init( &_randomName );
    memset(&_aiScript, 0, sizeof(script_info_t));
}

ObjectProfile::ObjectProfile(const std::string &folderPath, size_t slotNumber) : ObjectProfile()
{
    //Set some data
    _fileName = folderPath;
    _slotNumber = slotNumber;

    // load the character profile
    _icap = CapStack_load_one( folderPath.c_str(), slotNumber, false );

    // Load the model for this profile
    _imad = load_one_model_profile_vfs(folderPath.c_str(), slotNumber);

    // Load the enchantment for this profile
    STRING newloadname;
    make_newloadname( folderPath.c_str(), "/enchant.txt", newloadname );
    _ieve = EveStack_losd_one( newloadname, ( EVE_REF )_slotNumber );

    // Load the messages for this profile, do this before loading the AI script
    // to ensure any dynamic loaded messages get loaded last
    loadAllMessages(folderPath + "/message.txt");

    // Load the particles for this profile
    for (size_t cnt = 0; cnt < _particleProfiles.size(); cnt++ )
    {
        const std::string particleName = folderPath + "/part" + std::to_string(cnt) + ".txt";

        // Make sure it's referenced properly
        _particleProfiles[cnt] = PipStack_load_one(particleName.c_str(), INVALID_PIP_REF);
    }

    loadTextures(folderPath);

    // Load the waves for this iobj
    for ( size_t cnt = 0; cnt < _soundList.size(); cnt++ )
    {
        const std::string soundName = folderPath + "/sound" + std::to_string(cnt);
        _soundList[cnt] = _audioSystem.loadSound(soundName);
    }

    // Load the random naming table for this icap
    loadRandomNames(folderPath + "/naming.txt");

    // Fix lighting if need be
    if ( CapStack.lst[_icap].uniformlit )
    {
        mad_make_equally_lit_ref( _imad );
    }
}

ObjectProfile::~ObjectProfile()
{
    // release all of the sub-profiles
    CapStack_release_one( _icap );
    MadStack_release_one( _imad );
    //EveStack_release_one( pobj->ieve );

    //Release particle profiles
    for(PRO_REF particle : _particleProfiles) {
        PipStack_release_one(particle);
    }

    // release whatever textures are being used
    for(const auto &element : _texturesLoaded)
    {
        if ( element.second > TX_SPECIAL_LAST )
        {
            TxList_free_one( element.second );
        }
    }

    for(const auto &element : _iconsLoaded)
    {
        if ( element.second > TX_SPECIAL_LAST )
        {
            TxList_free_one( element.second );
        }
    }
}

void ObjectProfile::loadTextures(const std::string &folderPath)
{
    //Clear texture references
    _texturesLoaded.clear();
    _iconsLoaded.clear();

    // Load the skins and icons
    for (size_t cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        STRING newloadname;

        // do the texture
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/tris%d", folderPath.c_str(), cnt );

        TX_REF skin = TxList_load_one_vfs( newloadname, INVALID_TX_REF, TRANSCOLOR );
        if ( VALID_TX_RANGE( skin ) )
        {
            _texturesLoaded[cnt] = skin;
        }

        // do the icon
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/icon%d", folderPath.c_str(), cnt );

        TX_REF icon = TxList_load_one_vfs( newloadname, INVALID_TX_REF, INVALID_KEY );
        if ( VALID_TX_RANGE( icon ) )
        {
            _iconsLoaded[cnt] = icon;
        }
    }

    // If we didn't get a skin, set it to the water texture
    if ( _texturesLoaded.empty() )
    {
        _texturesLoaded[0] = TX_WATER_TOP;
        log_warning( "Object is missing a skin (%s)!\n", _fileName.c_str() );
    }

    // If we didn't get a icon, set it to the NULL icon
    if ( _iconsLoaded.empty())
    {
        _iconsLoaded[0] = TX_ICON_NULL;
        log_debug( "Object is missing an icon (%s)!\n", _fileName.c_str() );
    }
}

void ObjectProfile::addMessage(const std::string &message, const bool filterDuplicates)
{
    std::string parsedMessage = message;

    //replace underscore with whitespace
    std::replace(parsedMessage.begin(), parsedMessage.end(), '_', ' ');

    //Don't add the same message twice
    if(filterDuplicates) {
        for(const std::string& msg : _messageList) {
            if(msg == parsedMessage) {
                return;
            }
        }
    }

    //Add it to the list!
    _messageList.push_back(parsedMessage);
}

const std::string& ObjectProfile::getMessage(size_t index) const
{
    static const std::string EMPTY_STRING;

    if(index > _messageList.size()) {
        return EMPTY_STRING;
    }

    return _messageList[index];
}

void ObjectProfile::loadAllMessages(const std::string &filePath)
{
    /// @author ZF
    /// @details This function loads all messages for an object

    vfs_FILE *fileread = vfs_openRead( filePath.c_str() );
    if ( fileread )
    {
        STRING line;

        while ( goto_colon_vfs( NULL, fileread, true ) )
        {
            //Load one line
            vfs_get_string( fileread, line, SDL_arraysize( line ) );
            addMessage(line);
        }

        vfs_close( fileread );
    }
}

const char * ObjectProfile::generateRandomName()
{
    if ( !LOADED_CAP( _icap ) ) return "*NONE*";
    cap_t * pcap = CapStack_get_ptr( _icap );

    //If no random names loaded, return class name instead
    if ( 0 == _randomName.section[0].size ) {
        return pcap->classname;
    }

    const char* randomName = chop_create( &chop_mem, &_randomName );

    if (!randomName) {
        return "*NONE*";
    }

    return randomName;
}

bool ObjectProfile::loadRandomNames( const std::string &fileName )
{
    // clear out any current definition
    chop_definition_init( &_randomName );

    return chop_load_vfs( &chop_mem, fileName.c_str(), &_randomName);
}


SoundID ObjectProfile::getSoundID( int index ) const
{
    if ( index < 0 || index >= _soundList.size() ) {
        return INVALID_SOUND_ID;
    }

    return _soundList[index];
}

IDSZ ObjectProfile::getIDSZ(size_t type) const
{
    if(type >= IDSZ_COUNT) {
        return IDSZ_NONE;
    }

    cap_t * pcap = CapStack_get_ptr(_icap);
    if ( !pcap ) {
        return IDSZ_NONE;
    }

    return pcap->idsz[type];
}

TX_REF ObjectProfile::getSkin(size_t index)
{
    if(_texturesLoaded.find(index) == _texturesLoaded.end()) {
        return _texturesLoaded[0];
    }

    return _texturesLoaded[index];
}

TX_REF ObjectProfile::getIcon(size_t index)
{
    if(_iconsLoaded.find(index) == _iconsLoaded.end()) {
        return _iconsLoaded[0];
    }

    return _iconsLoaded[index];
}

PIP_REF ObjectProfile::getParticleProfile(size_t index) const
{
    if(index > _particleProfiles.size()) {
        return INVALID_PIP_REF;
    }
    return _particleProfiles[index];
}