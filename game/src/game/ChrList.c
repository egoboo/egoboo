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

/// @file  game/ChrList.c
/// @brief Implementation of the ChrList_* functions

#include <utility>

#include "game/ChrList.h"
#include "game/char.h"
#include "game/profiles/ProfileSystem.hpp"

//--------------------------------------------------------------------------------------------
static std::vector<CHR_REF> chr_termination_list;
static std::vector<CHR_REF> chr_activation_list;

int chr_loop_depth = 0;
std::unordered_map<CHR_REF, std::shared_ptr<chr_t>> _characterList;

//--------------------------------------------------------------------------------------------
bool ChrList_free_one( const CHR_REF ichr )
{
    /// @author ZZ
    /// @details This function sticks a character back on the free enchant stack
    ///
    /// @note Tying ALLOCATED_CHR() and POBJ_TERMINATE() to ChrList_free_one()
    /// should be enough to ensure that no character is freed more than once

    bool retval;

    if ( !ALLOCATED_CHR( ichr ) ) return false;

#if (DEBUG_SCRIPT_LEVEL > 0) && defined(DEBUG_PROFILE) && defined(_DEBUG)
    chr_log_script_time( ichr );
#endif

    // if we are inside a ChrList loop, do not actually change the length of the
    // list. This will cause some problems later.
    if ( chr_loop_depth > 0 )
    {
        retval = ChrList_add_termination( ichr );
    }
    else
    {
        // deallocate any dynamically allocated memory
        //chr_config_deinitialize( ChrList_get_ptr(ichr), 100 );
        _characterList.erase(ichr);
        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
CHR_REF ChrList_allocate( const PRO_REF profile, const CHR_REF override )
{
    //Make sure the profile is valid
    if(!_profileSystem.isValidProfileID(profile))
    {
        log_warning("ChrList_allocate() - Tried to spawn character with invalid ProfileID: %d\n", profile);
        return INVALID_CHR_REF;
    }

    CHR_REF ichr = INVALID_CHR_REF;

    if(override != INVALID_CHR_REF)
    {
        if(_characterList.find(override) == _characterList.end())
        {
            ichr = override;
        }
        else
        {
            log_warning( "ChrList_allocate() - failed to override a character? character %d already spawned? \n", REF_TO_INT( override ) );
        }
    }
    else
    {
        //Find first unused CHR_REF slot
        for(CHR_REF i = 0; i < MAX_CHR; ++i)
        {
            if(_characterList.find(i) == _characterList.end())
            {
                ichr = i;
                break;
            }
        }

        //No free slots remaining?
        if(ichr == INVALID_CHR_REF)
        {
            log_warning( "ChrList_allocate() - No free character slots available\n" );
        }
    }

    if (ichr != INVALID_CHR_REF)
    {
        std::shared_ptr<chr_t> object = std::make_shared<chr_t>(profile, ichr);

        if(!object) {
            log_warning( "ChrList_allocate() - Unable to allocate object memory\n" );
            return INVALID_CHR_REF;
        }

        // allocate the new one
        _characterList[ichr] = object;

        // construct the new structure
        //chr_config_construct(object.get(), 100 );
    }

    return ichr;
}

//--------------------------------------------------------------------------------------------
void ChrList_cleanup()
{
    // go through the list and activate all the characters that
    // were created while the list was iterating
    //for(CHR_REF ichr : chr_activation_list)
//    //{
//    //    if ( !ALLOCATED_CHR( ichr ) ) continue;
//    //    chr_t * pchr = ChrList_get_ptr( ichr );
//
//    //    if ( !pchr->obj_base.turn_me_on ) continue;
//
    //    pchr->obj_base.on         = true;
    //    pchr->obj_base.turn_me_on = false;
    //}
    //chr_activation_list.clear();

    // go through and delete any characters that were
    // supposed to be deleted while the list was iterating
    for(CHR_REF ichr : chr_termination_list)
    {
        _characterList.erase(ichr);
        log_debug("erased char");
    }
    chr_termination_list.clear();
}

//--------------------------------------------------------------------------------------------
bool ChrList_add_activation( const CHR_REF ichr )
{
    // put this character into the activation list so that it can be activated right after
    // the ChrList loop is completed

    if ( !VALID_CHR_RANGE( ichr ) ) return false;

    chr_activation_list.push_back(ichr);
    //ChrList_get_ptr(ichr)->obj_base.turn_me_on = true;

    return true;
}

//--------------------------------------------------------------------------------------------
bool ChrList_add_termination( const CHR_REF ichr )
{
    if(!ALLOCATED_CHR(ichr)) {
        return false;  
    } 

    chr_termination_list.push_back(ichr);

    // at least mark the object as "waiting to be terminated"
    ChrList_get_ptr(ichr)->terminateRequested = true;

    return true;
}

//--------------------------------------------------------------------------------------------
bool ChrList_request_terminate( const CHR_REF ichr )
{
    /// @author BB
    /// @details Mark this character for deletion

    chr_t * pchr = ChrList_get_ptr( ichr );

    return chr_request_terminate( pchr );
}

//--------------------------------------------------------------------------------------------
chr_t* ChrList_get_ptr(CHR_REF ichr)
{
    const auto &result = _characterList.find(ichr);

    if(result == _characterList.end())
    {
        return nullptr;
    }

    return (*result).second.get();
}