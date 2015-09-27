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

/// @file egolib/Profiles/ModuleProfile.hpp
/// @author Johan Jansen

#pragma once
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#include "egolib/typedef.h"
#include "egolib/Renderer/Texture.hpp"

enum ModuleFilter : uint8_t
{
    FILTER_OFF,
    FILTER_MAIN,
    FILTER_SIDE_QUEST,
    FILTER_TOWN,
    FILTER_FUN,
    NR_OF_MODULE_FILTERS,

    FILTER_STARTER        //Starter modules are special, place after last
};

class ModuleProfile
{
public:
    static const uint8_t RESPAWN_ANYTIME = 0xFF;

public:
    ModuleProfile();

    ~ModuleProfile();

    bool isModuleUnlocked() const;

    ModuleFilter getModuleType() const;

    inline oglx_texture_t *getIcon() {
        return _icon;
    }

    const std::string& getName() const {
        return _name;
    }

    bool isStarterModule() const {
        return _importAmount == 0;
    }

    uint8_t getRank() const {
        return _rank;
    }

    /**
     * @return
     *  the virtual pathname of the module
     */
    const std::string& getPath() const {
        return _vfsPath;
    }

    const std::string& getFolderName() const {
        return _folderName;
    }

    uint8_t getImportAmount() const {
        return _importAmount;
    }

    uint8_t getMinPlayers() const {
        return _minPlayers;
    }

    uint8_t getMaxPlayers() const {
        return _maxPlayers;
    }

    bool isExportAllowed() const {
        return _allowExport;
    }

    bool hasRespawnAnytime() const {
        return _respawnValid == RESPAWN_ANYTIME;
    }

    bool isRespawnValid() const {
        return 0 != _respawnValid;
    }

    const std::vector<std::string>& getSummary() const {
        return _summary;
    }

    static std::shared_ptr<ModuleProfile> loadFromFile(const std::string &filePath);
    static bool moduleHasIDSZ(const char *szModName, IDSZ idsz, size_t buffer_len, char * buffer);
    static bool moduleAddIDSZ(const char *szModName, IDSZ idsz, size_t buffer_len, const char * buffer);

private:
    bool _loaded;

    // data from menu.txt
    std::string _name;                      ///< Example: "Adventurer Starter"
    uint8_t _rank;                          ///< Number of stars

    std::string _reference;                 ///< the module reference string

    uint8_t   _importAmount;                ///< # of import characters
    bool    _allowExport;                   ///< Export characters?
    uint8_t   _minPlayers;                  ///< Number of players
    uint8_t   _maxPlayers;
    uint8_t   _respawnValid;                ///< Allow respawn
    std::vector<std::string> _summary;      ///< Quest description

    IDSZ     _unlockQuest;                  ///< the quest required to unlock this module
    int      _unlockQuestLevel;
    ModuleFilter    _moduleType;            ///< Main quest, town, sidequest or whatever
    bool            _beaten;                ///< The module has been marked with the [BEAT] eapansion

    oglx_texture_t *_icon;                  ///< the index of the module's tile image
    std::string _vfsPath;                   ///< the virtual pathname of the module ("mp_module/advent.mod")
    std::string _folderName;                ///< Folder name of module ("advent.mod")

    friend class ProfileSystem;
};
