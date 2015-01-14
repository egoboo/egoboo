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

/// @file  game/ProfileSystem.cpp
/// @brief Implementation of functions for controlling and accessing object profiles
/// @details
/// @author Johan Jansen

#include "game/ProfileSystem.hpp"
#include "game/Profile.hpp"
#include "game/particle.h"
#include "game/enchant.h"
#include "game/char.h"
#include "game/game.h"
#include "game/script_compile.h"
#include "game/graphic_texture.h"

//Globals
pro_import_t import_data;
ProfileSystem _profileSystem;

static const std::shared_ptr<ObjectProfile> NULL_PROFILE = nullptr;


ProfileSystem::ProfileSystem() :
	_initialized(false),
	_bookIcons()
{
	//ctor
}

void ProfileSystem::begin()
{

    if (_initialized)
    {
        // release all profile data and reinitialize the profile list
        releaseAllProfiles();

        // initialize the models
        model_system_end();

        _initialized = false;
    }

    // initialize all the sub-profile lists
    PipStack_init_all();
    EveStack_init_all();
    CapStack_init_all();
    MadStack_reconstruct_all();

    // fix the book icon list
    _bookIcons.clear();

    // initialize the models
    model_system_begin();

    // initialize the script compiler
    script_compiler_init();

    // necessary for loading up the copy.txt file
    load_action_names_vfs( "mp_data/actions.txt" );

    // necessary for reading "naming.txt" properly
    chop_data_init( &chop_mem );

    // something that is used in the game that is somewhat related to the profile stuff
    init_slot_idsz();

    // let the code know that everything is initialized
    _initialized = true;
}

void ProfileSystem::end()
{
    if (_initialized)
    {
        // release all profile data and reinitialize the profile list
        releaseAllProfiles();

        // initialize the models
        model_system_end();

        _initialized = false;
    }

    // reset the bookicon stuff
    _bookIcons.clear();
}

void ProfileSystem::releaseAllProfiles()
{
    /// @author ZZ
    /// @details This function clears out all of the model data

    // release the allocated data in all profiles (sounds, textures, etc.)
    _profilesLoaded.clear();

    // relese every type of sub-profile and re-initalize the lists
    PipStack_release_all();
    EveStack_release_all();
    CapStack_release_all();
    MadStack_release_all();
}

const std::shared_ptr<ObjectProfile>& ProfileSystem::getProfile(PRO_REF slotNumber) const {
    auto foundElement = _profilesLoaded.find(slotNumber);
    if(foundElement == _profilesLoaded.end()) return NULL_PROFILE;
    return foundElement->second;
}

//--------------------------------------------------------------------------------------------
int ProfileSystem::getProfileSlotNumber(const char * tmploadname, int slot_override)
{
    if ( slot_override > 0 && slot_override != INVALID_PRO_REF )
    {
        // just use the slot that was provided
        return slot_override;
    }

    // grab the slot from the file
    STRING szLoadName;
    make_newloadname( tmploadname, "/data.txt", szLoadName );

    if ( 0 == vfs_exists( szLoadName ) ) {

        return -1;
    }

    // Open the file
    vfs_FILE* fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread ) return -1;

    // load the slot's slot no matter what
    int slot = vfs_get_next_int( fileread );

    vfs_close( fileread );

    // set the slot slot
    if ( slot >= 0 )
    {
        return slot;
    }
    else if ( import_data.slot >= 0 )
    {
        return import_data.slot;
    }

    return -1;
}

//--------------------------------------------------------------------------------------------
//inline
//--------------------------------------------------------------------------------------------
CAP_REF ProfileSystem::pro_get_icap( const PRO_REF iobj )
{
    if ( !isValidProfileID( iobj ) ) return INVALID_CAP_REF;

    return LOADED_CAP( _profilesLoaded[iobj]->getCapRef() ) ? _profilesLoaded[iobj]->getCapRef() : INVALID_CAP_REF;
}

//--------------------------------------------------------------------------------------------
MAD_REF ProfileSystem::pro_get_imad( const PRO_REF iobj )
{
    if ( !isValidProfileID( iobj ) ) return INVALID_MAD_REF;

    return LOADED_MAD( _profilesLoaded[iobj]->getModelRef() ) ? _profilesLoaded[iobj]->getModelRef() : INVALID_MAD_REF;
}

//--------------------------------------------------------------------------------------------
EVE_REF ProfileSystem::pro_get_ieve( const PRO_REF iobj )
{
    if ( !isValidProfileID( iobj ) ) return INVALID_EVE_REF;

    return LOADED_EVE( _profilesLoaded[iobj]->getEnchantRef() ) ? _profilesLoaded[iobj]->getEnchantRef() : INVALID_EVE_REF;
}

//--------------------------------------------------------------------------------------------
PIP_REF ProfileSystem::pro_get_ipip( const PRO_REF iobj, int pip_index )
{
    PIP_REF found_pip = INVALID_PIP_REF;

    if ( !isValidProfileID( iobj ) )
    {
        // check for a global pip
        PIP_REF global_pip = (( pip_index < 0 ) || ( pip_index > MAX_PIP ) ) ? MAX_PIP : ( PIP_REF )pip_index;
        if ( LOADED_PIP( global_pip ) )
        {
            found_pip = global_pip;
        }
    }
    else if ( pip_index < MAX_PIP_PER_PROFILE )
    {
        // this is a local pip
        PIP_REF itmp;

        // grab the local pip
        itmp = _profilesLoaded[iobj]->getParticleProfile(pip_index);
        if ( VALID_PIP_RANGE( itmp ) )
        {
            found_pip = itmp;
        }
    }

    return found_pip;
}

//--------------------------------------------------------------------------------------------
cap_t * ProfileSystem::pro_get_pcap( const PRO_REF iobj )
{
    if ( !isValidProfileID( iobj ) ) return nullptr;

    if ( !LOADED_CAP( _profilesLoaded[iobj]->getCapRef() ) ) return nullptr;

    return CapStack_get_ptr( _profilesLoaded[iobj]->getCapRef() );
}

//--------------------------------------------------------------------------------------------
mad_t * ProfileSystem::pro_get_pmad( const PRO_REF iobj )
{
    if ( !isValidProfileID( iobj ) ) return nullptr;

    if ( !LOADED_MAD( _profilesLoaded[iobj]->getModelRef() ) ) return nullptr;

    return MadStack_get_ptr( _profilesLoaded[iobj]->getModelRef() );
}

//--------------------------------------------------------------------------------------------
eve_t * ProfileSystem::pro_get_peve( const PRO_REF iobj )
{
    if ( !isValidProfileID( iobj ) ) return nullptr;

    if ( !LOADED_EVE( _profilesLoaded[iobj]->getEnchantRef() ) ) return nullptr;

    return EveStack_get_ptr( _profilesLoaded[iobj]->getEnchantRef() );
}

//--------------------------------------------------------------------------------------------
pip_t * ProfileSystem::pro_get_ppip( const PRO_REF iobj, int pip_index )
{
    ObjectProfile * pobj;
    PIP_REF global_pip, local_pip;

    if ( !isValidProfileID( iobj ) )
    {
        // check for a global pip
        global_pip = (( pip_index < 0 ) || ( pip_index > MAX_PIP ) ) ? MAX_PIP : ( PIP_REF )pip_index;
        if ( LOADED_PIP( global_pip ) )
        {
            return PipStack_get_ptr( global_pip );
        }
        else
        {
            return nullptr;
        }
    }

    // find the local pip if it exists
    local_pip = INVALID_PIP_REF;
    if ( pip_index < MAX_PIP_PER_PROFILE )
    {
        local_pip = _profilesLoaded[iobj]->getParticleProfile(pip_index);
    }

    return LOADED_PIP( local_pip ) ? PipStack.lst + local_pip : nullptr;
}

int ProfileSystem::loadOneProfile(const char* pathName, int slot_override )
{
    bool required = !VALID_CAP_RANGE( slot_override );

    // get a slot value
    int islot = getProfileSlotNumber( pathName, slot_override );

    // throw an error code if the slot is invalid of if the file doesn't exist
    if ( islot < 0 || islot > MAX_PROFILE )
    {
        // The data file wasn't found
        if ( required )
        {
            log_debug( "load_one_profile_vfs() - \"%s\" was not found. Overriding a global object?\n", pathName );
        }
        else if ( VALID_CAP_RANGE( slot_override ) && slot_override > PMod->importamount * MAX_IMPORT_PER_PLAYER )
        {
            log_warning( "load_one_profile_vfs() - Not able to open file \"%s\"\n", pathName );
        }

        return MAX_PROFILE;
    }

    // convert the slot to a profile reference
    PRO_REF iobj = ( PRO_REF )islot;

    // throw an error code if we are trying to load over an existing profile
    // without permission
    if (_profilesLoaded.find(iobj) != _profilesLoaded.end())
    {
        // Make sure global objects don't load over existing models
        if ( required && SPELLBOOK == iobj )
        {
            log_error( "load_one_profile_vfs() - object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, pathName );
        }
        else if ( required && overrideslots )
        {
            log_error( "load_one_profile_vfs() - object slot %i used twice (%s, %s)\n", REF_TO_INT( iobj ), _profilesLoaded[iobj]->getName().c_str(), pathName );
        }
        else
        {
            // Stop, we don't want to override it
            return MAX_PROFILE;
        }
    }

    //Actually allocate the object and create it
    _profilesLoaded[iobj] = std::make_shared<ObjectProfile>(pathName, islot);

    //ZF> TODO: This is kind of a dirty hack and could be done cleaner. If this item is the book object, 
    //    then the icons are also loaded into the global book icon array
    if ( SPELLBOOK == islot )
    {
        for(const auto &element : _profilesLoaded[iobj]->getAllIcons())
        {
            _bookIcons.push_back(element.second);
        }
    }

    return iobj;
}

TX_REF ProfileSystem::getSpellBookIcon(size_t index) const
{
    if(_bookIcons.empty()) return INVALID_TX_REF;
    return _bookIcons[index % _bookIcons.size()];
}