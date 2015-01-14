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

/// @file game/Profile.hpp
#pragma once

#include <unordered_map>

#include "game/script.h"     //for script_info_t
//#include "game/char.h"
#include "game/mad.h"
#include "game/chop.h"
#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Forward declarations
typedef int SoundID;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a wrapper for all the datafiles in the *.obj dir
class ObjectProfile
{
public:
    /**
    * @brief Loads and parses a profile folder and builds a profile object from it.
    **/
    ObjectProfile(const std::string &folderPath, size_t slotNumber);

    /**
    * @brief Deconstructor
    **/
    ~ObjectProfile();

    /// @author ZF
    /// @details This adds one string to the list of messages associated with a profile. The function will
    //              dynamically allocate more memory if there are more messages than array size
    //  @param filterDuplicates don't add it if it already exists
    void addMessage(const std::string &message, const bool filterDuplicates = false);

    /**
    * @return a string loaded into the specified index, or an empty string if the index is not valid
    **/
    const std::string& getMessage(size_t index) const;

    SoundID getSoundID(int index) const;

    IDSZ getIDSZ(size_t type) const;

    inline bool isValidMessageID(int id) const {return id > 0 && id < _messageList.size();}

    /// @author BB
    /// @details use the profile's chop to generate a name. Return "*NONE*" on a failure.
    const char* generateRandomName();

    /**
    * @return get the skin of the specified index number (or skin 0 if index is invalid)
    **/
    TX_REF getSkin(size_t index);

    /**
    * @return get the icon of the specified index number (or icon 0 if index is invalid)
    **/
    TX_REF getIcon(size_t index);

    inline const std::unordered_map<size_t, TX_REF> &getAllIcons() const {return _iconsLoaded;}

    inline const std::string& getName() const {return _fileName;}

    inline MAD_REF getModelRef() const {return _imad;}
    inline CAP_REF getCapRef() const {return _icap;}
    inline EVE_REF getEnchantRef() const {return _ieve;}

    /**
    * Gets the particle profile loaded into the specified index number
    **/
    PIP_REF getParticleProfile(size_t index) const;

    /**
    * Write access getter
    **/
    inline script_info_t& getAIScript() {return _aiScript;}
    inline chop_definition_t& getRandomNameData() {return _randomName;}

private:
    /**
    * Private default constructor
    **/
    ObjectProfile();

    /**
    * @brief Loads the md2 model for this profile
    **/
    //void loadModel(const std::string &filePath);

    /**
    * @brief Parses message.txt and loads all messages from it
    **/
    void loadAllMessages(const std::string &filePath);

    /**
    * @brief load all tris*.bmp or tris*.png skin and icons
    **/
    void loadTextures(const std::string &folderPath);

    /**
    * @brief Loads naming.txt and builds a random name list
    **/
    bool loadRandomNames(const std::string &filePath);

private:
    std::string _fileName;                      ///< Usually the source filename
    size_t _requestCount;                       ///< the number of attempted spawns
    size_t _spawnCount;                         ///< the number of successful spawns

    // the sub-profiles
    CAP_REF _icap;                             ///< the cap for this profile
    MAD_REF _imad;                             ///< the md2 model for this profile
    EVE_REF _ieve;                             ///< the enchant profile for this profile
    int _slotNumber;

    /// the random naming info
    chop_definition_t _randomName;
    
    script_info_t _aiScript;                    ///< the AI script for this profile

    //Particles
    std::array<PIP_REF, MAX_PIP_PER_PROFILE> _particleProfiles;

    // the profile skins
    std::unordered_map<size_t, TX_REF> _texturesLoaded;
    std::unordered_map<size_t, TX_REF> _iconsLoaded;

    // the profile message info
    std::vector<std::string> _messageList;   ///< Dynamic array of messages

    // sounds
    std::array<SoundID, 30> _soundList;             ///< sounds in a object
};
