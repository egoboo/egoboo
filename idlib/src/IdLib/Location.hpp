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

/// @file   IdLib/Location.hpp
/// @brief  Declaration of a location in an Egoboo DSL file
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/Platform.hpp"

namespace Id {

/**
 * @brief
 *  A location is identified by the load name of the Egoboo DSL file and a line number within that file.
 */
class Location {

private:

    /**
     * @brief
     *  The load name of the file.
     */
    std::string _loadName;

    /**
     * @brief
     *  The line number of a line within the file.
     */
    size_t _lineNumber;

public:

    /**
     * @brief
     *  Construct this position from the specified arguments.
     * @param loadName
     *  the load name of the file
     * @param lineNumber
     *  the line number of a line within the file
     */
    Location(const std::string& loadName, const size_t lineNumber);

    /**
     * @brief
     *  Construct this lociation from another location (copy constructor).
     * @param other
     *  the other location
     */
    Location(const Location& other);

    /**
     * @brief
     *  Is this location equal to another location.
     * @param other
     *  the other location
     * @return
     *  @a true if this position is equal to the other location,
     *  @a false otherwise
     */
    bool operator==(const Location& other) const;

    /**
     * @brief
     *  Is this location not equal to another location.
     * @param other
     *  the other location
     * @return
     *  @a true if this position is not equal to the other location,
     *  @a false otherwise
     */
    bool operator!=(const Location& other) const;

    /**
     * @brief
     *  Get the load name of the file.
     * @return
     *  the load name of the file
     */
    const std::string& getLoadName() const;

    /**
     * @brief
     *  Get the line number in the file.
     * @return
     *  the line number in the file
     */
    size_t getLineNumber() const;

};

} // namespace Id
