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

/// @file IdLib/Location.hpp
/// @brief Declaration of a location in an Egoboo DSL file.
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/CRTP.hpp"
#include <type_traits>

namespace Id {

/// @brief A location is identified by a file name and line number within that file.
class Location : public EqualToExpr<Location>
{
private:
    /// @brief The file name of the file.
    std::string m_fileName;

    /// @brief The line number of a line within the file.
    size_t m_lineNumber;

public:
    /// @brief Construct this location.
    /// @param fileName the file name of the file
    /// @param lineNumber the line number of a line within the file
    Location(const std::string& fileName, const size_t lineNumber);

    /// @brief Copy-Construct this lociation from another location.
    /// @param other the other location
    Location(const Location& other);

    /// @brief Assign this location from another location.
    /// @param other the other location
    /// @return this location
    Location& operator=(Location other);

public:
    // CRTP
    bool equalTo(const Location& other) const;

public:
    /// @brief Get the file name of the file of this location.
    /// @return the file name of the file
    const std::string& getFileName() const;

    /// @brief Get the line number of a line in the file.
    /// @return the line number of a line in the file
    size_t getLineNumber() const;

public:
    /// @brief Move-construct this location from another location.
    /// @param other the other location
    Location(Location&& other);

    /// @brief Swap two locations.
    /// @param x, y the locations
    friend void swap(Location& x, Location& y)
    {
        using std::swap;
        swap(x.m_fileName, y.m_fileName);
        swap(x.m_lineNumber, y.m_lineNumber);
    }

}; // struct Location

static_assert(std::is_copy_constructible<Location>::value, "Id::Location must be copy constructible");
static_assert(std::is_move_constructible<Location>::value, "Id::Location must be move constructible");
static_assert(std::is_copy_assignable<Location>::value, "Id::Location must be copy assignable");
static_assert(std::is_move_assignable<Location>::value, "Id::Location must be move assignable");

} // namespace Id
