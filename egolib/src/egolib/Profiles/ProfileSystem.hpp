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
#include "egolib/Core/Singleton.hpp"
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

#include "egolib/Profiles/_AbstractProfileSystem.hpp"
#include "egolib/Profiles/EnchantProfile.hpp"
#include "egolib/Profiles/ParticleProfile.hpp"

class ProfileSystem : public Ego::Core::Singleton<ProfileSystem> {
protected:
    friend Ego::Core::Singleton<ProfileSystem>::CreateFunctorType;
    friend Ego::Core::Singleton<ProfileSystem>::DestroyFunctorType;

    ProfileSystem();
    ~ProfileSystem();

public:

    AbstractProfileSystem<EnchantProfile, EnchantProfileRef> EnchantProfileSystem;
    AbstractProfileSystem<ParticleProfile, ParticleProfileRef> ParticleProfileSystem;

    /**
     * @brief
     *  Reset all profile systems to state (conceptually) equivalent to their states directly after their initialization.
     */
    void reset();

    /// @{
    /// @brief Get if an object profile reference was loaded.
    /// @param ref the object profile reference
    /// @return @a true if the specified object profile reference was loaded, @a false otherwise
    bool isLoaded(PRO_REF ref) const;
    bool isLoaded(ObjectProfileRef ref) const;
    /// @}

    /**
     *  @details This function loads one object and returns the object slot
     */
    ObjectProfileRef loadOneProfile(const std::string &folderPath, int slot_override = -1);

    /**
     * @brief Loads only the slot number from data.txt
     *        If slot_override is valid, then that is used indead
     */
    int getProfileSlotNumber(const std::string &folderPath, int slot_override = -1);

    ///@{
    /// @brief Get the object profile of a profile reference.
    /// @param ref the object profile reference
    /// @return a pointer to the profile or a null pointer
    const std::shared_ptr<ObjectProfile>& getProfile(PRO_REF ref) const;
    const std::shared_ptr<ObjectProfile>& getProfile(ObjectProfileRef ref) const;
    ///@}

    const std::shared_ptr<ObjectProfile>& getProfile(const std::string& name) const;

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
    std::unordered_map<std::string, std::shared_ptr<ObjectProfile>> _profilesLoadedByName; //Maps names to ObjectProfiles

    std::vector<std::shared_ptr<ModuleProfile>> _moduleProfilesLoaded;  // List of all valid game modules loaded

    std::vector<std::shared_ptr<LoadPlayerElement>> _loadPlayerList; // List of characters that can be loaded (lightweight)
};

// TODO: Remove this.
inline bool LOADED_PIP(PIP_REF ref) {
    return ProfileSystem::get().ParticleProfileSystem.isLoaded(ref);
}

// TODO: Remove this.
extern pro_import_t import_data;

