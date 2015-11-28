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
    _profilesLoaded(),
    _moduleProfilesLoaded(),
    _loadPlayerList()
{
    // Initialize the particle and enchant profile system.
    ParticleProfileSystem::get().initialize();
    EnchantProfileSystem.initialize();

    // Initialize the script compiler.
    parser_state_t::initialize();
}


ProfileSystem::~ProfileSystem()
{
    // Uninitialize the script compiler.
    parser_state_t::uninitialize();

    // Uninitialize the enchant and particle profile system.
    EnchantProfileSystem.unintialize();
    ParticleProfileSystem::get().unintialize();
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
    ParticleProfileSystem::get().reset();
    EnchantProfileSystem.reset();
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

    if (!vfs_exists(dataFilePath.c_str())) {

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

std::shared_ptr<pip_t> ProfileSystem::pro_get_ppip(const PRO_REF iobj, const LocalParticleProfileRef& lppref)
{
    if (!isValidProfileID(iobj))
    {
        // check for a global pip
        PIP_REF global_pip = ((lppref.get() < 0) || (lppref.get() > MAX_PIP)) ? MAX_PIP : (PIP_REF)lppref.get();
        if (LOADED_PIP(global_pip))
        {
            return ParticleProfileSystem::get().get_ptr(global_pip);
        }
        else
        {
            return nullptr;
        }
    }

    // find the local pip if it exists
    PIP_REF local_pip = INVALID_PIP_REF;
    if (lppref.get() < MAX_PIP_PER_PROFILE)
    {
        local_pip = _profilesLoaded[iobj]->getParticleProfile(lppref);
    }

    return LOADED_PIP(local_pip) ? ParticleProfileSystem::get().get_ptr(local_pip) : nullptr;
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
			Log::get().debug("ProfileSystem::loadOneProfile() - \"%s\" was not found. Overriding a global object?\n", pathName.c_str());
        }
        else if (required && slot_override > 4 * MAX_IMPORT_PER_PLAYER)
        {
			Log::get().warn("ProfileSystem::loadOneProfile() - Not able to open file \"%s\"\n", pathName.c_str());
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
			std::ostringstream os;
			os << __FILE__ << ":" << __LINE__ << ": "
				<< " object slot " << SPELLBOOK << " is a special reserved slot number and cannot be used by " << pathName.c_str() << std::endl;
			Log::get().error("%s", os.str().c_str());
			throw std::runtime_error(os.str());
        }
        else if (required && overrideslots)
        {
			std::ostringstream os;
			os << __FILE__ << ":" << __LINE__ << ": "
			   << "object slot " << REF_TO_INT(iobj) << " is already used by " << _profilesLoaded[iobj]->getPathname().c_str() << " and cannot be used by " << pathName.c_str() << std::endl;
			Log::get().error("%s", os.str().c_str());
			throw std::runtime_error(os.str());
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
		Log::get().warn("ProfileSystem::loadOneProfile() - Failed to load (%s) into slot number %d\n", pathName.c_str(), iobj);
        return INVALID_PRO_REF;
    }

    //Success! Store object into the loaded profile map
    _profilesLoaded[iobj] = profile;

    return iobj;
}

const Ego::DeferredOpenGLTexture& ProfileSystem::getSpellBookIcon(size_t index) const
{
    return _profilesLoaded.find(SPELLBOOK)->second->getIcon(index);
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
			Log::get().warn("Unable to load module: %s\n", vfs_ModPath);
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

void ProfileSystem::loadGlobalParticleProfiles()
{
    const char *loadpath;

    // Load in the standard global particles ( the coins for example )
    loadpath = "mp_data/1money.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_COIN1 ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s",os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/5money.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_COIN5 ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/25money.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_COIN25 ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/100money.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_COIN100 ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/200money.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_GEM200 ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/500money.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_GEM500 ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/1000money.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_GEM1000 ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/2000money.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_GEM2000 ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/disintegrate_start.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_DISINTEGRATE_START ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/disintegrate_particle.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_DISINTEGRATE_PARTICLE ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }
#if 0
    // Load module specific information
    loadpath = "mp_data/weather4.txt";
    if (INVALID_PIP_REF == ParticleProfileSystem::get().load_one(loadpath, (PIP_REF)PIP_WEATHER))
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/weather5.txt";
    if (INVALID_PIP_REF == ParticleProfileSystem::get().load_one(loadpath, (PIP_REF)PIP_WEATHER_FINISH))
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }
#endif

    loadpath = "mp_data/splash.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_SPLASH ) )
    {
		std::ostringstream os;
		os << __FILE__ << ":" << __LINE__ << ": data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    loadpath = "mp_data/ripple.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_RIPPLE ) )
    {
		std::ostringstream os;
		os << __FILE__ << ":" << __LINE__ << ": data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    // This is also global...
    loadpath = "mp_data/defend.txt";
    if ( INVALID_PIP_REF == ParticleProfileSystem::get().load_one( loadpath, ( PIP_REF )PIP_DEFEND ) )
    {
		std::ostringstream os;
		os << "data file `" << loadpath << "` was not found" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }
}
