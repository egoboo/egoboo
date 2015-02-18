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
template <typename TYPE,typename REFTYPE,REFTYPE INVALIDREF,size_t COUNT,typename READER>
struct _AbstractProfileSystem : public Stack<TYPE, COUNT>
{
    const std::string _profileTypeName;
    const std::string _debugPathName;

    _AbstractProfileSystem(const std::string& profileTypeName,const std::string& debugPathName) :
        _profileTypeName(profileTypeName), _debugPathName(debugPathName)
    {}

    bool isValidRange(REFTYPE ref)
    {
        return ref < COUNT;
    }

    bool isLoaded(REFTYPE ref)
    {
        return isValidRange(ref) && lst[ref].loaded;
    }

    void init_all()
    {
        for (REFTYPE ref = 0; ref < COUNT; ++ref)
        {
            get_ptr(ref)->init();
        }
        // Reset the stack "pointer".
        count = 0;
    }

    bool release_one(const REFTYPE ref)
    {
        if (!isValidRange(ref)) return false;
        TYPE *profile = get_ptr(ref);
        if (profile->loaded) profile->init();
        return true;
    }

    REFTYPE get_free()
    {
        REFTYPE ref = INVALIDREF;

        if (count < COUNT)
        {
            ref = count;
            count++;
        }

        return ref;
    }


    /// @brief Load an profile into the profile stack.
    /// @return a reference to the profile on sucess, INVALIDREF on failure
    REFTYPE load_one(const char *loadName, const REFTYPE _override)
    {
        REFTYPE ref = INVALIDREF;
        if (isValidRange(_override))
        {
            release_one(_override);
            ref = _override;
        }
        else
        {
            ref = get_free();
        }

        if (!isValidRange(ref))
        {
            return INVALIDREF;
        }
        TYPE *profile = get_ptr(ref);

        if (!READER::read(profile, loadName))
        {
            return INVALIDREF;
        }

        return ref;
    }

    void release_all()
    {
        size_t numLoaded = 0;
        int max_request = 0;
        for (REFTYPE ref = 0; ref < COUNT; ref++)
        {
            if (isLoaded(ref))
            {
                TYPE *profile = get_ptr(ref);

                max_request = std::max(max_request, profile->request_count);
                numLoaded++;
            }
        }

        if (numLoaded > 0 && max_request > 0)
        {
            vfs_FILE * ftmp = vfs_openWriteB(_debugPathName.c_str());
            if (NULL != ftmp)
            {
                vfs_printf(ftmp, "List of used %s profiles\n\n", _profileTypeName.c_str());

                for (REFTYPE ref = 0; ref < COUNT; ref++)
                {
                    if (isLoaded(ref))
                    {
                        eve_t *profile = EveStack.get_ptr(ref);
                        vfs_printf(ftmp, "index == %d\tname == \"%s\"\tcreate_count == %d\trequest_count == %d\n",
                            REF_TO_INT(ref), profile->name, profile->create_count, profile->request_count);
                    }
                }
                vfs_close(ftmp);
            }
            for (REFTYPE ref = 0; ref < COUNT; ++ref)
            {
                release_one(ref);
            }
        }
    }

};