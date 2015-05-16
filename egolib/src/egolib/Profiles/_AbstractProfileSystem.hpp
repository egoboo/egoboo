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
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#include "egolib/typedef.h"
#include "egolib/vfs.h"

/// Temporary abstract profile system for unifying particle- and
/// enchant-profiles systems before merging both into ProfileSystem.
template <typename TYPE,typename REFTYPE,REFTYPE INVALIDREF,size_t CAPACITY,typename READER>
struct _AbstractProfileSystem : public Id::NonCopyable
{

protected:

    size_t _size;

    Ego::GUID _updateGUID;

    TYPE *_elements[CAPACITY];

    const std::string _profileTypeName;

    const std::string _debugPathName;

    REFTYPE get_free()
    {
        for (REFTYPE ref = 0; ref < CAPACITY; ++ref)
        {
            if (!isLoaded(ref))
            {
                return ref;
            }
        }
        return INVALIDREF;
    }


public:
    _AbstractProfileSystem(const std::string& profileTypeName,const std::string& debugPathName) :
        _size(0),
        _updateGUID(EGO_GUID_INVALID),
        _elements(),
        _profileTypeName(profileTypeName), 
        _debugPathName(debugPathName)
    {}

    Ego::GUID getUpdateGUID() const
    {
        return _updateGUID;
    }

    /**
     * @brief
     *	Get the size of this stack.
     * @return
     *	the size of this stack
     */
    inline size_t get_size() const
    {
        return _size;
    }

    /**
     * @brief
     *	Get the capacity of this stack.
     * @return
     *	the capacity of this stack
     */
    inline size_t get_capacity() const
    {
        return CAPACITY;
    }

    /**
     * @brief
     *	Get a pointer to the stack element at the specified index.
     * @param index
     *	the index
     * @return
     *	a pointer to the stack element if @a index is within bounds, @a false otherwise
     * @todo
     *	Raise an exception of @a index is greater than or equal to @a capacity.
     */
    TYPE *get_ptr(size_t index)
    {
        return (index >= CAPACITY) ? nullptr : _elements[index];
    }

    bool isValidRange(REFTYPE ref)
    {
        return ref < CAPACITY;
    }

    bool isLoaded(REFTYPE ref)
    {
        return isValidRange(ref) && _elements[ref]->_loaded;
    }

    void initialize()
    {
        REFTYPE ref = 0;
        try
        {
            for (ref = 0; ref < CAPACITY; ++ref)
            {
                _elements[ref] = new TYPE();
            }
        } 
        catch (std::exception& ex)
        {
            while (ref > 0)
            {
                delete _elements[--ref];
                _elements[ref] = nullptr;
            }
        }
        for (ref = 0; ref < CAPACITY; ++ref)
        {
            this->get_ptr(ref)->init();
        }
        // Reset the stack "pointer".
        _size = 0;
    }

    bool release_one(const REFTYPE ref)
    {
        if (!isValidRange(ref)) return false;
        TYPE *profile = this->get_ptr(ref);
        if (profile->_loaded)
        {
            profile->init();
#if 1
            if (_size == 0) throw std::underflow_error(_profileTypeName + " stack underflow");
            _size--;
#endif
        }
        return true;
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
#if 1
        if (_size == CAPACITY) throw std::overflow_error(_profileTypeName + " stack overflow");
        _size++;
#endif

        return ref;
    }

    void unintialize()
    {
        reset();
        for (REFTYPE ref = 0; ref < CAPACITY; ++ref)
        {
            delete _elements[ref];
            _elements[ref] = nullptr;
        }
    }

    void reset()
    {
        size_t numLoaded = 0;
        size_t maxSpawnRequestCount = 0;
        for (REFTYPE ref = 0; ref < CAPACITY; ref++)
        {
            if (isLoaded(ref))
            {
                TYPE *profile = get_ptr(ref);

                maxSpawnRequestCount = std::max(maxSpawnRequestCount, profile->_spawnRequestCount);
                numLoaded++;
            }
        }

        if (numLoaded > 0 && maxSpawnRequestCount > 0)
        {
            vfs_FILE *file = vfs_openWriteB(_debugPathName.c_str());
            if (nullptr != file)
            {
                vfs_printf(file, "List of used %s profiles\n\n", _profileTypeName.c_str());

                for (REFTYPE ref = 0; ref < CAPACITY; ref++)
                {
                    if (isLoaded(ref))
                    {
                        TYPE *profile = this->get_ptr(ref);
                        vfs_printf(file, "index == %d\tname == \"%s\"\tspawn count == %" PRIuZ "\tspawn request count == %" PRIuZ "\n",
                                   REF_TO_INT(ref), profile->_name, profile->_spawnCount, profile->_spawnRequestCount);
                    }
                }
                vfs_close(file);
            }
            for (REFTYPE ref = 0; ref < CAPACITY; ++ref)
            {
                release_one(ref);
            }
        }
    }

};
