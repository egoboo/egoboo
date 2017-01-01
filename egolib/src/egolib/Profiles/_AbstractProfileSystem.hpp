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
#include "egolib/Log/_Include.hpp"

/// @brief The base class of all profile system (in particular but not restricted to
/// enchant, object and particle profile systems).
/// @remark A profile system stores tuples. The first element (called the key) of tuple
/// is a profile reference and the second element (called the value) is a pointer to a
/// profile. Not two different tuples with equivalent keys exist. Tuples can be added
/// and remove.
/// @remark The size of a profile system is its number of entries.
template <typename TypeArg, typename RefTypeArg>
struct AbstractProfileSystem : public Id::NonCopyable
{
public:
    using Type = TypeArg;
    using RefType = typename RefTypeArg::ValueType;
    static constexpr RefType MinimumRef = RefTypeArg::MinimumValue;
    static constexpr RefType MaximumRef = RefTypeArg::MaximumValue;
    static constexpr RefType InvalidRef = RefTypeArg::InvalidValue;

protected:
    static const std::shared_ptr<Type> nullProfile;

protected:
    std::unordered_map<RefType, std::shared_ptr<Type>> map;

    const std::string profileTypeName;

    const std::string debugPathName;


public:
    /// @brief Construct this profile system.
    /// @param profileTypeName the profile type name e.g. "enchant", "particle" or "object"
    /// @param debugPathName the path name of a file into which debug information about this abstract profile system is dumped in
    AbstractProfileSystem(const std::string& profileTypeName,const std::string& debugPathName) :
        map(), profileTypeName(profileTypeName), debugPathName(debugPathName)
    { /* Intentionally empty. */ }

    /// @brief Destruct this profile system.
    virtual ~AbstractProfileSystem()
    { /* Intentionally empty. */ }

public:
    /// @brief Get the size of this profile system.
    /// @return the size of this abstract profile system
    /// @remark The size of a profile system is its number of entries.
    inline size_t size() const
    {
        return map.size();
    }

    /// @brief Remove an entry.
    /// @param ref the reference
    /// @return the number of removed entries (zero or one)
    size_t unload(const RefType& ref)
    {
        size_t oldSize = size();
        map.erase(ref);
        return oldSize;
    }

    /// @brief Remove all entries.
    /// @return the number of removed entries
    size_t unloadAll()
    {
        size_t oldSize = size();
        map.clear();
        return oldSize;
    }

public:
    /// @brief Get the profile for a profile reference.
    /// @param ref the profile reference
    /// @return a pointer to the profile the profile loaded into the references to if any, a null pointer otherwise
    const std::shared_ptr<Type>& get_ptr(const RefType& ref) const
    {
        if (InvalidRef == ref)
        {
            return nullptr;
        }
        auto it = map.find(ref);
        if (it == map.cend())
        {
            return nullptr;
        }
        return it->second;
    }

    bool isValid(const RefType& ref) const
    {
        return InvalidRef != ref;
    }

    bool isLoaded(const RefType& ref) const
    {
        if (!isValid(ref))
        {
            return false;
        }
        return map.find(ref) != map.end();
    }

    /// Get the entries.
    /// @return the entries
    inline const std::unordered_map<RefType, std::shared_ptr<Type>>& getLoadedProfiles() const
    {
        return map;
    }


    /// @brief Add an entry for a profile reference and a profile.
    /// @return the target reference (see remarks) on sucess, InvalidRef on failure
    /// @remark If the override reference is loaded, then its entry is removed and
    /// the override reference becomes the target reference. Otherwise the target
    /// reference is a previously unused reference. An entry for the target reference
    /// and the profile is added.
    RefType load(const std::string& pathname, const RefType& override)
    {
        if(isLoaded(override))
        {
			Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "loaded over existing profile", Log::EndOfEntry);
        }

        RefType ref = InvalidRef;
        if (isValid(override))
        {
            ref = override;
        }
        else
        {
            for (RefType i = MinimumRef; i < MaximumRef; ++i)
            {
                if (!isLoaded(i))
                {
                    ref = i;
                    break;
                }
            }

            if (!isValid(ref))
            {
                return InvalidRef;
            }
        }
        
        auto profile = Type::readFromFile(pathname);
        if (!profile)
        {
            return InvalidRef;
        }

        map[ref] = profile;

        return ref;
    }

    void uninitialize()
    {
        unloadAll();
    }

    /// @brief Statistics about a profile system.
    struct Stats
    {
        /// @brief The size of the profile system.
        size_t size;

        /// @brief The maximum spawn count of all profiles.
        size_t maxSpawnCount;

        /// @brief The sum of the spawn counts of all profiles.
        /// If a profile is referenced by \f$n\f$ tuples, then its spawn count is added into the sum \f$n\f$ times.
        size_t sumSpawnCount;

        /// @brief The maximum spawn request count of all profiles.
        size_t maxSpawnRequestCount;
        
        /// @brief The sum of the spawn request counts of all profiles.
        /// If a profile is referenced by \f$n\f$ entries, then its spawn request count is added into the sum \f$n\f$ times.
        size_t sumSpawnRequestCount;

        Stats() :
            size(0), maxSpawnRequestCount(0), sumSpawnRequestCount(0), maxSpawnCount(0), sumSpawnCount(0)
        {}
    };

    /// @brief Get the statistics of this profile system.
    /// @return the statistics of this profile system
    Stats getStats() const
    {
        Stats stats;
        stats.size = size();
        for (const auto& pair : map)
        {
            const auto ptr = pair.second;
            // spawn request count
            stats.sumSpawnRequestCount += ptr->spawnRequestCount;
            stats.maxSpawnRequestCount = std::max(stats.maxSpawnRequestCount, ptr->spawnRequestCount);
            // spawn count
            stats.sumSpawnCount += ptr->spawnCount;
            stats.maxSpawnCount = std::max(stats.maxSpawnCount, ptr->spawnCount);
        }
        return stats;
    }

    /// @brief Dump information (including the statistics) of this profile system intos its debug file.
    void dump()
    {
        std::ofstream os;
        os.open(debugPathName, std::ofstream::out | std::ofstream::app);
        if (!os.is_open())
        {
            return;
        }
        dump(os);
        os.close();
    }

    /// @brief Dump information (including the stastics) of this profile system into an output stream.
    /// @param os the output stream
    void dump(std::ostream& os)
    {
        const auto stats = getStats();

        os << profileTypeName << " profile system" << std::endl;
        os << " size:                        " << stats.size << std::endl;
        os << " maximum spawn request count: " << stats.maxSpawnRequestCount << std::endl;
        os << " summed spawn request count:  " << stats.sumSpawnRequestCount << std::endl;
        os << " maximum spawn count:         " << stats.maxSpawnCount << std::endl;
        os << " summed spawn count:          " << stats.sumSpawnCount << std::endl;
        os << " list of loaded profiles:" << std::endl;
        for (const auto& pair : map)
        {
            const auto ptr = pair.second;
            os << " reference: " << pair.first << ","
               << " name: " << "`" << ptr->_name << "`" << ","
               << " spawn request count: " << ptr->_spawnRequestCount << ","
               << " spawn count: " << ptr->_spawnCount
               << std::endl;
        }
    }

};

template <typename TypeArg, typename RefTypeArg>
const std::shared_ptr<TypeArg> AbstractProfileSystem<TypeArg, RefTypeArg>::nullProfile = nullptr;
