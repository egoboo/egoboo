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
/// @file egolib/Profiles/ProfileSystem.hpp
/// @author Johan Jansen

#pragma once
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#include "egolib/typedef.h"
#include "egolib/Profiles/LocalParticleProfileRef.hpp"

//Forward declarations
class ObjectProfile;
class ModuleProfile;
class ParticleProfile;
class EnchantProfile;
class LoadPlayerElement;
namespace Ego { class DeferredTexture; }

/// Placeholders used while importing profiles
struct pro_import_t
{
    pro_import_t() :
        slot(-1),
        player(0),
        slot_lst(),
        max_slot(0)
    {
        //ctor
    }

    int   slot;
    int   player;
    std::array<int, 10 * 4> slot_lst; //MAX_IMPORT_PER_PLAYER * MAX_PLAYER;
    int   max_slot;
};

class ProfileSystem : public Ego::Core::Singleton<ProfileSystem>
{

protected:

    // Befriend with singleton to grant access to ProfileSystem::ProfileSystem and ProfileSystem::~ProfileSystem.
    using TheSingleton = Ego::Core::Singleton < ProfileSystem > ;
    friend TheSingleton;

    ProfileSystem();

    ~ProfileSystem();

public:

    /**
     * @brief
     *  Reset all profile systems to state (conceptually) equivalent to their states directly after their initialization.
     */
    void reset();

    //ZF> TODO: This C-like function needs to be removed
    std::shared_ptr<ParticleProfile> pro_get_ppip(const PRO_REF iobj, const LocalParticleProfileRef& lppref);

    /**
     * @brief
     *  Get if if the specified profile ID has been loaded.
     * @param id
     *  the profile ID
     * @return
     *  @a true if the specified profile ID has been loaded,
     *  @a false otherwise
     */
    inline bool isValidProfileID(PRO_REF id) const
    {
        return _profilesLoaded.find(id) != _profilesLoaded.end();
    }

    /**
     *  @details This function loads one object and returns the object slot
     */
    PRO_REF loadOneProfile(const std::string &folderPath, int slot_override = -1);

    /**
     * @brief Loads only the slot number from data.txt
     *        If slot_override is valid, then that is used indead
     */
    int getProfileSlotNumber(const std::string &folderPath, int slot_override = -1);

    /**
     * @return Returns the ObjectProfile loaded into the specified slot number
     */
    const std::shared_ptr<ObjectProfile> &getProfile(PRO_REF slotNumber) const;

    const Ego::DeferredTexture& getSpellBookIcon(size_t index) const;

    /**
     * Get map of all profiles loaded
     */
    inline const std::unordered_map<PRO_REF, std::shared_ptr<ObjectProfile>>& getLoadedProfiles() const {
        return _profilesLoaded;
    }

    /**
     * @brief Scans the module folder and loads all ModuleProfiles (needs only to be done once)
     */
    void loadModuleProfiles();

    /**
     * @return list of all ModuleProfiles currently loaded
     */
    const std::vector<std::shared_ptr<ModuleProfile>>& getModuleProfiles() const {
        return _moduleProfilesLoaded;
    }

    void printDebugModuleList();

    /**
     * @brief
     *  Reload list of all possible characters we might load.
     */
    void loadAllSavedCharacters(const std::string &saveGameDirectory);

    const std::vector<std::shared_ptr<LoadPlayerElement>>& getSavedPlayers() const {
        return _loadPlayerList;
    }

    /**
     * @brief
     *  This reads in global particles (e.g. money).
     */
    void loadGlobalParticleProfiles();

private:
    std::unordered_map<PRO_REF, std::shared_ptr<ObjectProfile>> _profilesLoaded; //Maps slot numbers to ObjectProfiles

    std::unordered_map<PIP_REF, std::shared_ptr<ParticleProfile>> _particleProfilesLoaded; //Maps id's to ParticleProfiles

    std::vector<std::shared_ptr<ModuleProfile>> _moduleProfilesLoaded;  // List of all valid game modules loaded

    std::vector<std::shared_ptr<LoadPlayerElement>> _loadPlayerList; // List of characters that can be loaded (lightweight)
};

extern pro_import_t import_data;

