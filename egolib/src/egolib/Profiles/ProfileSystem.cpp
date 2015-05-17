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

/// @file  egolib/Profiles/ProfileSystem.cpp
/// @brief Implementation of functions for controlling and accessing object profiles
/// @details
/// @author Johan Jansen

#define EGOLIB_PROFILES_PRIVATE 1
#include "egolib/Profiles/ProfileSystem.hpp"
#include "egolib/Profiles/ObjectProfile.hpp"
#include "egolib/Profiles/ModuleProfile.hpp"
#include "game/GameStates/LoadPlayerElement.hpp"
#include "game/Entities/_Include.hpp"
#include "game/char.h"
#include "game/game.h"
#include "game/script_compile.h"

//Globals
pro_import_t import_data;

static const std::shared_ptr<ObjectProfile> NULL_PROFILE = nullptr;


ProfileSystem::ProfileSystem() :
    _bookIcons(),
    _profilesLoaded(),
    _moduleProfilesLoaded(),
    _loadPlayerList()
{
    // Initialize the particle and enchant profile system.
    PipStack.initialize();
    EveStack.initialize();

    // Initialize the MAD stack system.
    MadStack_ctor();

    // Initialize the script compiler.
    parser_state_t::initialize();

    // necessary for loading up the copy.txt file
    load_action_names_vfs("mp_data/actions.txt");
}


ProfileSystem::~ProfileSystem()
{
    // Reset all profiles.
    reset();

    // Uninitialize the script compiler.
    parser_state_t::uninitialize();

    // Uninitialize the enchant and particle profile system.
    EveStack.unintialize();
    PipStack.unintialize();

    // Uninitialize the MAD stack system.
    MadStack_dtor();

    // Clear the book icons.
    _bookIcons.clear();
}

void ProfileSystem::reset()
{
    /// @author ZZ
    /// @details This function clears out all of the model data

    // Release the allocated data in all profiles (sounds, textures, etc.).
    _profilesLoaded.clear();

    // Release list of loadable characters.
    _loadPlayerList.clear();

    // Reset particle, enchant and models.
    PipStack.reset();
    EveStack.reset();
    MadStack_reset();
}

const std::shared_ptr<ObjectProfile>& ProfileSystem::getProfile(PRO_REF slotNumber) const
{
    auto foundElement = _profilesLoaded.find(slotNumber);
    if (foundElement == _profilesLoaded.end()) return NULL_PROFILE;
    return foundElement->second;
}

int ProfileSystem::getProfileSlotNumber(const std::string &folderPath, int slot_override)
{
    if (slot_override >= 0 && slot_override != INVALID_PRO_REF)
    {
        // just use the slot that was provided
        return slot_override;
    }

    // grab the slot from the file
    std::string dataFilePath = folderPath + "/data.txt";

    if (0 == vfs_exists(dataFilePath.c_str())) {

        return -1;
    }

    // Open the file
    ReadContext ctxt(dataFilePath);
    if (!ctxt.ensureOpen()) return -1;

    // load the slot's slot no matter what
    int slot = vfs_get_next_int(ctxt);

    ctxt.close();

    // set the slot slot
    if (slot >= 0)
    {
        return slot;
    }
    else if (import_data.slot >= 0)
    {
        return import_data.slot;
    }

    return -1;
}

EVE_REF ProfileSystem::pro_get_ieve(const PRO_REF iobj)
{
    if (!isValidProfileID(iobj)) return INVALID_EVE_REF;

    return LOADED_EVE(_profilesLoaded[iobj]->getEnchantRef()) ? _profilesLoaded[iobj]->getEnchantRef() : INVALID_EVE_REF;
}

mad_t * ProfileSystem::pro_get_pmad(const PRO_REF iobj)
{
    if (!isValidProfileID(iobj)) return nullptr;

    if (!LOADED_MAD(_profilesLoaded[iobj]->getModelRef())) return nullptr;

    return MadStack.get_ptr(_profilesLoaded[iobj]->getModelRef());
}

eve_t * ProfileSystem::pro_get_peve(const PRO_REF iobj)
{
    if (!isValidProfileID(iobj)) return nullptr;

    if (!LOADED_EVE(_profilesLoaded[iobj]->getEnchantRef())) return nullptr;

    return EveStack.get_ptr(_profilesLoaded[iobj]->getEnchantRef());
}

pip_t * ProfileSystem::pro_get_ppip(const PRO_REF iobj, int pip_index)
{
    PIP_REF global_pip, local_pip;

    if (!isValidProfileID(iobj))
    {
        // check for a global pip
        global_pip = ((pip_index < 0) || (pip_index > MAX_PIP)) ? MAX_PIP : (PIP_REF)pip_index;
        if (LOADED_PIP(global_pip))
        {
            return PipStack.get_ptr(global_pip);
        }
        else
        {
            return nullptr;
        }
    }

    // find the local pip if it exists
    local_pip = INVALID_PIP_REF;
    if (pip_index < MAX_PIP_PER_PROFILE)
    {
        local_pip = _profilesLoaded[iobj]->getParticleProfile(pip_index);
    }

    return LOADED_PIP(local_pip) ? PipStack.get_ptr(local_pip) : nullptr;
}

PRO_REF ProfileSystem::loadOneProfile(const std::string &pathName, int slot_override)
{
    bool required = !(slot_override < 0 || slot_override >= INVALID_PRO_REF);

    // get a slot value
    int islot = getProfileSlotNumber(pathName, slot_override);

    // throw an error code if the slot is invalid of if the file doesn't exist
    if (islot < 0 || islot >= INVALID_PRO_REF)
    {
        // The data file wasn't found
        if (required)
        {
            log_debug("ProfileSystem::loadOneProfile() - \"%s\" was not found. Overriding a global object?\n", pathName.c_str());
        }
        else if (required && slot_override > 4 * MAX_IMPORT_PER_PLAYER)
        {
            log_warning("ProfileSystem::loadOneProfile() - Not able to open file \"%s\"\n", pathName.c_str());
        }

        return INVALID_PRO_REF;
    }

    // convert the slot to a profile reference
    PRO_REF iobj = static_cast<PRO_REF>(islot);

    // throw an error code if we are trying to load over an existing profile
    // without permission
    if (_profilesLoaded.find(iobj) != _profilesLoaded.end())
    {
        // Make sure global objects don't load over existing models
        if (required && SPELLBOOK == iobj)
        {
            log_error("ProfileSystem::loadOneProfile() - object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, pathName.c_str());
        }
        else if (required && overrideslots)
        {
            log_error("ProfileSystem::loadOneProfile() - object slot %i used twice (%s, %s)\n", REF_TO_INT(iobj), _profilesLoaded[iobj]->getFilePath().c_str(), pathName.c_str());
        }
        else
        {
            // Stop, we don't want to override it
            return INVALID_PRO_REF;
        }
    }

    std::shared_ptr<ObjectProfile> profile = ObjectProfile::loadFromFile(pathName, iobj);
    if (!profile)
    {
        log_warning("ProfileSystem::loadOneProfile() - Failed to load (%s) into slot number %d\n", pathName.c_str(), iobj);
        return INVALID_PRO_REF;
    }

    //Success! Store object into the loaded profile map
    _profilesLoaded[iobj] = profile;

    //ZF> TODO: This is kind of a dirty hack and could be done cleaner. If this item is the book object, 
    //    then the icons are also loaded into the global book icon array
    if (SPELLBOOK == iobj)
    {
        _bookIcons = profile->getAllIcons();
    }

    return iobj;
}

TX_REF ProfileSystem::getSpellBookIcon(size_t index) const
{
    const auto& result = _bookIcons.find(index);

    if (result == _bookIcons.end()) {
        return INVALID_TX_REF;
    }

    return result->second;
}

void ProfileSystem::loadModuleProfiles()
{
    //Clear any previously loaded first
    _moduleProfilesLoaded.clear();

    // Search for all .mod directories and load the module info
    vfs_search_context_t *ctxt = vfs_findFirst("mp_modules", "mod", VFS_SEARCH_DIR);
    const char * vfs_ModPath = vfs_search_context_get_current(ctxt);

    while (nullptr != ctxt && VALID_CSTR(vfs_ModPath))
    {
        //Try to load menu.txt
        std::shared_ptr<ModuleProfile> module = ModuleProfile::loadFromFile(vfs_ModPath);
        if (module)
        {
            _moduleProfilesLoaded.push_back(module);
        }
        else
        {
            log_warning("Unable to load module: %s\n", vfs_ModPath);
        }

        ctxt = vfs_findNext(&ctxt);
        vfs_ModPath = vfs_search_context_get_current(ctxt);
    }
    vfs_findClose(&ctxt);
}


void ProfileSystem::printDebugModuleList()
{
    // Log a directory list
    vfs_FILE* filesave = vfs_openWrite("/debug/modules.txt");

    if (filesave == nullptr) {
        return;
    }

    vfs_printf(filesave, "This file logs all of the modules found\n");
    vfs_printf(filesave, "** Denotes an invalid module\n");
    vfs_printf(filesave, "## Denotes an unlockable module\n\n");

    for (size_t imod = 0; imod < _moduleProfilesLoaded.size(); ++imod)
    {
        const std::shared_ptr<ModuleProfile> &module = _moduleProfilesLoaded[imod];

        if (!module->_loaded)
        {
            vfs_printf(filesave, "**.  %s\n", module->_vfsPath.c_str());
        }
        else if (module->isModuleUnlocked())
        {
            vfs_printf(filesave, "%02d.  %s\n", REF_TO_INT(imod), module->_vfsPath.c_str());
        }
        else
        {
            vfs_printf(filesave, "##.  %s\n", module->_vfsPath.c_str());
        }
    }


    vfs_close(filesave);
}

void ProfileSystem::loadAllSavedCharacters(const std::string &saveGameDirectory)
{
    /// @author ZZ
    /// @details This function figures out which players may be imported, and loads basic
    ///     data for each

    //Clear any old imports
    _loadPlayerList.clear();

    // Search for all objects
    vfs_search_context_t *ctxt = vfs_findFirst(saveGameDirectory.c_str(), "obj", VFS_SEARCH_DIR);
    const char *foundfile = vfs_search_context_get_current(ctxt);

    while (NULL != ctxt && VALID_CSTR(foundfile))
    {
        std::string folderPath = foundfile;

        // is it a valid filename?
        if (folderPath.empty()) {
            continue;
        }

        // does the directory exist?
        if (!vfs_exists(folderPath.c_str())) {
            continue;
        }

        // offset the slots so that ChoosePlayer will have space to load the inventory objects
        int slot = (MAX_IMPORT_OBJECTS + 2) * _loadPlayerList.size();

        // try to load the character profile (do a lightweight load, we don't need all data)
        std::shared_ptr<ObjectProfile> profile = ObjectProfile::loadFromFile(folderPath, slot, true);
        if (!profile) {
            continue;
        }

        //Loaded!
        _loadPlayerList.push_back(std::make_shared<LoadPlayerElement>(profile));

        //Get next player object
        ctxt = vfs_findNext(&ctxt);
        foundfile = vfs_search_context_get_current(ctxt);
    }
    vfs_findClose(&ctxt);
}
