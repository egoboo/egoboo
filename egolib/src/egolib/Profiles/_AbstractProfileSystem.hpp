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
#include <fstream>
#include "egolib/Log/_Include.hpp"

/**
 * @brief
 *  The base class of all profile system - in particular but not restricted to
 *  enchant, object and particle profile systems.
 * @author
 *  Michael Heilmann
 * @todo
 *  Move this class and all other profile-related things into some appropriate namespace.
 */
template <typename TYPE, typename REFTYPE, REFTYPE INVALIDREF, size_t CAPACITY>
struct AbstractProfileSystem : public Id::NonCopyable
{

protected:

    std::unordered_map<REFTYPE, std::shared_ptr<TYPE>> _map;

    const std::string _profileTypeName;

    const std::string _debugPathName;


public:

    /**
     * @brief
     *  Construct his abstract profile system.
     * @param profileTypeName
     *  the profile type name e.g. "enchant", "particle" or "object"
     * @param debugPathName
     *  the path name of a file into which debug information about this abstract profile system is dumped in
     */
    AbstractProfileSystem(const std::string& profileTypeName,const std::string& debugPathName) :
        _map(),
        _profileTypeName(profileTypeName), 
        _debugPathName(debugPathName) {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Destruct this abstract profile system.
     * @remark
     *  Intentionally protected.
     */
    virtual ~AbstractProfileSystem() {
        /* Intentionally empty. */
    }

    /**
     * @brief
     *  Get the size of this abstract profile system.
     * @return
     *  the size of this abstract profile system
     * @remark
     *  The size of an abstract profile system is the current number of profiles in that abstract profile system.
     */
    inline size_t get_size() const {
        return _map.size();
    }

    /**
     * @brief
     *  Get the capacity of this abstract profile system.
     * @return
     *  the capacity of this abstract profile system
     * @remark
     *  The capacity of an abstract profile system is the maximum number of profiles in that abstract profile system.
     */
    inline size_t get_capacity() const {
        return CAPACITY;
    }

    bool release(const REFTYPE ref)
    {
        return _map.erase(ref);
    }

    /**
     * @brief
     *  Get the profile for a profile reference.
     * @param ref
     *  the profile reference
     * @return
     *  a pointer to the profile for the profile reference of the profile reference is valid,
     *  a null pointer otherwise
     */
    std::shared_ptr<TYPE> get_ptr(REFTYPE ref) {
        if (!isValidRange(ref) || !isLoaded(ref)) {
            return nullptr;
        }
        if (!_map[ref]) {
            _map[ref] = std::make_shared<TYPE>();
        }
        return _map[ref];
    }

    bool isValidRange(REFTYPE ref) {
        return ref < CAPACITY;
    }

    bool isLoaded(REFTYPE ref) {
        return _map.find(ref) != _map.end();
    }

    void initialize() {
    }

    /// @brief Load an profile into the profile stack.
    /// @return a reference to the profile on sucess, INVALIDREF on failure
    REFTYPE load_one(const std::string& pathname, const REFTYPE _override)
    {
        if(isLoaded(_override)) {
			Log::get().warn("%s:%d:%s: loaded over existing profile\n", __FILE__, __LINE__, __FUNCTION__);
        }

        REFTYPE ref = INVALIDREF;
        if (isValidRange(_override)) {
            ref = _override;
        }
        else {
            for(REFTYPE i = 0; i < CAPACITY; ++i) {
                if(!isLoaded(i)) {
                    ref = i;
                    break;
                }
            }

            if (!isValidRange(ref)) {
                return INVALIDREF;
            }
        }

        //Allocate memory for new profile
        std::shared_ptr<TYPE> profile = TYPE::readFromFile(pathname);

        if (!profile) {
            return INVALIDREF;
        }

        _map[ref] = profile;
        return ref;
    }

    void unintialize()
    {
        _map.clear();
    }

    void reset()
    {
        dump();
        _map.clear();
    }

    /**
     * @brief
     *  Statistics about an abstract profile system.
     */
    struct Stats {
        /**
         * @brief
         *  The number of loaded profiles i.e.
         *  \f{align*}{
         *  \left|\left\{p | p \in Profiles \wedge p.loaded = true\right\}\right|
         *  \f}
         *  where \f$Profiles\f$ is the set of profiles.
         */
        size_t _loadedCount;
        /**
         * @brief
         *  The maximum spawn count (of all profiles) i.e.
         *  \f{align*}{
         *  \begin{cases}
         *  \max_{i=0}^n\left(p_i._spawnCount\right) & \text{if }n > 0\\
         *  0                                        & \text{otherwise}
         *  \end{cases}
         *  \f}
         *  where \f$n=|Profiles|\f$ is the magnitude of the set of profiles.
         */
        size_t _maxSpawnCount;
        /**
         * @brief
         *  The spawn count sum (of all profiles) i.e.
         *  \f{align*}{
         *  \begin{cases}
         *  \sum_{i=0}^n p_i._spawnCount & \text{if }n > 0\\
         *  0                            & \text{otherwise}
         *  \end{cases}
         *  \f}
         *  where \f$n=|Profiles|\f$ is the magnitude of the set of profiles.
         */
        size_t _sumSpawnCount;

        /**
         * @brief
         *  The maximum spawn request count (of all profiles) i.e.
         *  \f{align*}{
         *  \begin{cases}
         *  \max_{i=0}^n\left(p_i._spawnRequestCount\right) & \text{if }n > 0\\
         *  0                                               & \text{otherwise}
         *  \end{cases}
         *  \f}
         *  where \f$n=|Profiles|\f$ is the magnitude of the set of profiles.
         */
        size_t _maxSpawnRequestCount;
        
        /**
         * @brief
         *  The spawn request count sum (of all profiles) i.e.
         *  \f{align*}{
         *  \begin{cases}
         *  \sum_{i=0}^n p_i._spawnRequestCount & \text{if }n > 0\\
         *  0                                   & \text{otherwise}
         *  \end{cases}
         *  \f}
         *  where \f$n=|Profiles|\f$ is the magnitude of the set of profiles.
         */
        size_t _sumSpawnRequestCount;
    };

    /**
     * @brief
     *  Get the statistics of this abstract profile system.
     */
    Stats getStats() {
        Stats stats;
        stats._loadedCount = 0;
        stats._maxSpawnRequestCount = 0;
        stats._sumSpawnRequestCount = 0;
        stats._maxSpawnCount = 0;
        stats._sumSpawnCount = 0;
        for (const auto& pair : _map) {
            const auto ptr = pair.second;
            stats._loadedCount++;
            // spawn request count
            stats._sumSpawnRequestCount += ptr->_spawnRequestCount;
            stats._maxSpawnRequestCount = std::max(stats._maxSpawnRequestCount, ptr->_spawnRequestCount);
            // spawn count
            stats._sumSpawnCount += ptr->_spawnCount;
            stats._maxSpawnCount = std::max(stats._maxSpawnCount, ptr->_spawnCount);
        }
        return stats;
    }

    /**
     * @brief
     *  Dump information (including the statistics) of this profile into the debug file of this
     *  abstract profile system.
     */
    void dump() {
        std::ofstream os;
        os.open(_debugPathName, std::ofstream::out | std::ofstream::app);
        if (!os.is_open()) {
            return;
        }
        dump(os);
        os.close();
    }


    /**
     * @brief
     *  Dump information (including the stastics) of this profile system into an output stream.
     * @param os
     *  the output stream
     */
    void dump(std::ostream& os) {
        const auto stats = getStats();

        os << _profileTypeName << " profile system" << std::endl;
        os << " loaded count:                " << stats._loadedCount << std::endl;
        os << " maximum spawn request count: " << stats._maxSpawnRequestCount << std::endl;
        os << " summed spawn request count:  " << stats._sumSpawnRequestCount << std::endl;
        os << " maximum spawn count:         " << stats._maxSpawnCount << std::endl;
        os << " summed spawn count:          " << stats._sumSpawnCount << std::endl;
        os << " list of loaded profiles:" << std::endl;
        for (const auto& pair : _map) {
            const auto ptr = pair.second;
            os << " reference: " << pair.first << ","
               << " name: " << "`" << ptr->_name << "`" << ","
               << " spawn request count: " << ptr->_spawnRequestCount << ","
               << " spawn count: " << ptr->_spawnCount
               << std::endl;
        }
    }

};
