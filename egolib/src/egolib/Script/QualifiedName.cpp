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

/// @file egolib/Script/QualifiedName.cpp
/// @brief A qualified names i.e. a name of the form @code{<name>('.'<name>)*}.
/// @author Michael Heilmann

#include "egolib/Script/QualifiedName.hpp"

namespace Ego {
namespace Script {

void QualifiedName::noQualifiedName(const char *file, int line, const Location& location, const string& string)
{
    ostringstream message;
    message << file << ":" << line << ": ";
    message << "argument string `" << string << "` does not not represent a valid qualified name";
    throw CompilationErrorException(file, line, CompilationErrorKind::Lexical,
                                    location, message.str());
}

QualifiedName::QualifiedName(const string& string) :
    m_string(string)
{
    if (string.empty())
    {
        noQualifiedName(__FILE__, __LINE__, Location("string `" + string + "`", 1), string);
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

QualifiedName::QualifiedName(const QualifiedName& other) :
    m_string(other.m_string)
{}

QualifiedName::~QualifiedName()
{}

QualifiedName& QualifiedName::operator=(const QualifiedName& other)
{
    m_string = other.m_string;
    return *this;
}

const string& QualifiedName::getString() const
{
    return m_string;
}

} // namespace Script
} // namespace Ego

namespace std {

using namespace Ego::Script;

size_t hash<QualifiedName>::operator()(const QualifiedName& x) const
{
    return hash<string>()(x.getString());
}

bool equal_to<QualifiedName>::operator()(const QualifiedName& x, const QualifiedName& y) const
{
    return equal_to<string>()(x.getString(), y.getString());
}

bool less<QualifiedName>::operator()(const QualifiedName& x, const QualifiedName& y) const
{
    // Sort the qualifid names lexicographically.
    return less<string>()(x.getString(), y.getString());
}

} // namespace std