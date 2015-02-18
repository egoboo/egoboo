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

#pragma once
#if !defined(GAME_PROFILES_PRIVATE) || GAME_PROFILES_PRIVATE != 1
#error(do not include directly, include `game/profiles/_Include.hpp` instead)
#endif

#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"

/// Temporary abstract profile system for unifying particle- and
/// enchant-profiles systems before merging both into ProfileSystem.
template <typename TYPE,typename REFTYPE,REFTYPE INVALIDREF,size_t COUNT>
struct _AbstractProfileSystem : public Stack<TYPE, COUNT>
{
    const std::string _debugPathName;

    _AbstractProfileSystem(const std::string& debugPathName) :
        _debugPathName(debugPathName)
    {}

    bool isValidRange(REFTYPE ref)
    {
        return ref < COUNT;
    }

    bool isLoaded(REFTYPE ref)
    {
        return isValidRange(ref) && lst[ref].loaded;
    }

    /// @brief Load an profile into the profile stack.
    /// @return a reference to the profile on sucess, INVALIDREF on failure

};