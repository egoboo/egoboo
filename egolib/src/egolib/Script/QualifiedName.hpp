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

/// @file egolib/Script/QualifiedName.hpp
/// @brief A qualified names i.e. a name of the form @code{<name>('.'<name>)*}.
/// @author Michael Heilmann

#pragma once

#include "egolib/Script/Errors.hpp"

namespace Ego {
namespace Script {

/// @brief A qualified name is an element of the set of strings
/// @code
/// qualifiedName := name ('.' name)*
/// name := '_'* (alphabetic) (alphabetic|digit|'_')*
/// @endcode
/// @remark std::hash, std::equal_to and std::less have specializations for qualified names
struct QualifiedName final
{
private:
    /// @brief Raise an exception indicating that a string can not be parsed into qualified name.
    /// @param file, line the C/C++ source location associated with the exception
    /// @param location the source location w.r.t. the string
    /// @param string the offending string
    /// @throw LexicalError always raised with the specified C/C++ source location and a descriptive error message
    static void noQualifiedName(const char *file, int line, const Location& location, const string& string);

private:
    /// @brief The string.
    string m_string;

public:
    /// @brief Construct a qualified name from a string.
    /// @param string the string
    /// @post This qualified name was assigned the string.
    /// @throw Id::LexicalErrorException the argument string @a string does not represent a qualified name
    QualifiedName(const string& string);

    /// @brief Construct this qualified name from another qualified name.
    /// @param other the other qualified name
    /// @post This qualified name was assigned the string of the other qualified name.
    QualifiedName(const QualifiedName& other);

    /// @brief Destruct this qualified name.
    ~QualifiedName();

    /// @brief Assign this qualified name another qualified name.
    /// @param other the other qualified name
    /// @return this qualified name
    /// @post This qualified name was assigned the string of the other qualified name.
    QualifiedName& operator=(const QualifiedName& other);

    /// @brief Get the string of this qualified name.
    /// @return the string
    const string& getString() const;
};

} // namespace Script
} // namespace Ego

namespace std {
using namespace Ego::Script;

template <>
struct hash<QualifiedName>
{
    size_t operator()(const QualifiedName& x) const;
};

template <>
struct equal_to<QualifiedName>
{
    bool operator()(const QualifiedName& x, const QualifiedName& y) const;
};

template <>
struct less<QualifiedName>
{
    bool operator()(const QualifiedName& x, const QualifiedName& y) const;
};

} // namespace std
