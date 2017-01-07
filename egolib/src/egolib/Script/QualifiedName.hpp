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

/// @file   egolib/Script/QualifiedName.hpp
/// @brief  A qualified names i.e. a name of the form @code{<name>('.'<name>)*}.
/// @author Michael Heilmann

#pragma once

#include "egolib/Script/Errors.hpp"

namespace Ego
{
namespace Script
{

using namespace std;

/**
 * @brief
 *  A qualified name is an element of the set of strings
 *  @code
 *  qualifiedName := name ('.' name)*
 *  name := '_'* (alphabetic) (alphabetic|digit|'_')*
 *  @endcode
 * @remark
 *  std::hash, std::equal_to and std::less have specializations for qualified names
 */
struct QualifiedName final
{

private:
    
    /**
     * @brief
     *  Raise an exception indicating that a string can not be parsed into qualified name.
     * @param file, line
     *  the C/C++ source location associated with the exception
     * @param location
     *  the source location w.r.t. the string
     * @param string
     *  the offending string
     * @throw LexicalError
     *  always raised with the specified C/C++ source location and a descriptive error message
     */
    static void noQualifiedName(const char *file, int line, const Location& location, const string& string)
    {
        ostringstream message;
        message << file << ":" << line << ": ";
        message << "argument string `" << string << "` does not not represent a valid qualified name";
        throw Id::CompilationErrorException(file, line, Id::CompilationErrorKind::Lexical,
                                            location, message.str());
    }

private:

    /**
     * @brief
     *  The string.
     */
    string _string;

public:

    /**
     * @brief
     *  Destruct this qualified name.
     */
    virtual ~QualifiedName()
    {}

    /**
     * @brief
     *  Get the string of this qualified name.
     * @return
     *  the string
     */
    const string& getString() const
    {
        return _string;
    }

    /**
     * @brief
     *  Construct this qualified name from another qualified name.
     * @param other
     *  the other qualified name
     * @post
     *  This qualified name was assigned the string of the other qualified name.
     */
    QualifiedName(const QualifiedName& other) :
        _string(other._string)
    {
    }

    /**
     * @brief
     *  Assign this qualified name another qualified name.
     * @param other
     *  the other qualified name
     * @return
     *  this qualified name
     * @post
     *  This qualified name was assigned the string of the other qualified name.
     */
    QualifiedName& operator=(const QualifiedName& other)
    {
        _string = other._string;
        return *this;
    }

    /**
     * @brief
     *  Construct a qualified name from a string.
     * @param string
     *  the string
     * @post
     *  This qualified name was assigned the string.
     * @throw Id::LexicalErrorException
     *  if the argument string @a string does not represent a qualified name
     */
    QualifiedName(const string& string) :
        _string(string)
    {
        if (string.empty())
        {
            noQualifiedName(__FILE__, __LINE__, Location("string `" + string + "`",1), string);
        }
        auto begin = string.cbegin(), end = string.cend();
        auto current = begin;
        while (true)
        {
            while ('_' == *current)
            {
                current++;
            }
            if (!isalpha(*current))
            {
                noQualifiedName(__FILE__, __LINE__, Location("string `" + string + "`", 1), string);
            }
            do
            {
                current++;
            } while (current != end && (isalpha(*current) || isdigit(*current) || '_' == *current));
            if (current != end)
            {
                if ('.' != *current)
                {
                    noQualifiedName(__FILE__, __LINE__, Location("string `" + string + "`", 1), string);
                }
                begin = ++current;
            }
            else
            {
                break;
            }
        }
        // As the string is was not empty, the argument string must represent a valid qualified name if this point is reached.
    }
};

} // namespace Script
} // namespace Ego

namespace std
{
using namespace Ego::Script;
    
template <>
struct hash<QualifiedName>
{
    size_t operator()(const QualifiedName& x) const
    {
        return hash<string>()(x.getString());
    }
};

template <>
struct equal_to<QualifiedName>
{
    bool operator()(const QualifiedName& x, const QualifiedName& y) const
    {
        return equal_to<string>()(x.getString(),y.getString());
    }
};

template <>
struct less<QualifiedName>
{
    bool operator()(const QualifiedName& x, const QualifiedName& y) const
    {
        // Sort the qualifid names lexicographically.
        return less<string>()(x.getString(), y.getString());
    }
};

} // namespace std
