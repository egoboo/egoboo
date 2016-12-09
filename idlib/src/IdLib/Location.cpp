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

/// @file   IdLib/Location.cpp
/// @brief  Definition of a location in an Egoboo DSL file
/// @author Michael Heilmann

#define IDLIB_PRIVATE 1
#include "IdLib/Location.hpp"
#undef IDLIB_PRIVATE

namespace Id {

Location::Location(const std::string& fileName, const size_t lineNumber)
    : m_fileName(fileName), m_lineNumber(lineNumber)
{}

Location::Location(const Location& other)
    : m_fileName(other.m_fileName), m_lineNumber(other.m_lineNumber)
{}

Location::Location(Location&& other)
    : m_fileName(std::move(other.m_fileName)), m_lineNumber(std::move(other.m_lineNumber))
{}

Location& Location::operator=(Location other)
{
    swap(*this, other);
    return *this;
}

bool Location::equalTo(const Location& other) const
{
    return m_fileName == other.m_fileName
        && m_lineNumber == other.m_lineNumber;
}

const std::string& Location::getFileName() const
{
    return m_fileName;
}

size_t Location::getLineNumber() const
{
    return m_lineNumber;
}

} // namespace Id
