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

/// @file IdLib/CompilationErrorException.hpp
/// @brief Definition of an exception related to compilation errors.
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/Exception.hpp"
#include "IdLib/Location.hpp"

namespace Id {

using namespace std;

/// @brief An enumeration of the different kinds of compilation errors.
enum class CompilationErrorKind
{
    /// @brief A compilation error that occurred during semantic analysis.
    Semantical,
    /// @brief A compilation error that occurred during syntactic analysis.
    Syntactical,
    /// @brief A compilation error that occurred during lexical analysis.
    Lexical,
};

/// @brief A compilation error exception.
class CompilationErrorException : public Exception
{
private:
    /// @brief The location associated with the compilation error.
    Location location;
    /// @brief A description of the compilation error.
    string description;
    /// @brief The kind of the compilation error.
    CompilationErrorKind kind;

public:
    /// @brief Construct this exception.
    /// @param file the C++ source file (as obtained by the __FILE__ macro) associated with this exception
    /// @param line the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
    /// @param kind the kind of the compilation error
    /// @param location the location associated with the error
    /// @param description a description of the error
    CompilationErrorException(const char *file, int line, CompilationErrorKind kind, const Location& location, const std::string& description) :
        Exception(file, line), kind(kind), location(location), description(description)
    {}

    /// @brief Copy construct this exception with the values of another exception.
    /// @param other the other exception
    CompilationErrorException(const CompilationErrorException& other) :
        Exception(other), kind(other.kind), location(other.location), description(other.description)
    {}

    /// @brief Assign this exception with the values of another exception.
    /// @param other the other exception
    /// @return this exception
    CompilationErrorException& operator=(const CompilationErrorException& other)
    {
        Exception::operator=(other);
        kind = other.kind;
        location = other.location;
        description = other.description;
        return *this;
    }

public:
    /// @brief Get the location associated with the compilation error.
    /// @return the location associated with the compilation error
    const Location& getLocation() const
    {
        return location;
    }

    /// @brief Get the kind of the compilation error.
    /// @return the kind of the compilation error
    CompilationErrorKind getKind() const
    {
        return kind;
    }

    /// @brief Overloaded cast to std::string operator.
    /// @return the result of the cast
    operator string() const override
    {
        ostringstream o;
        o << location.getFileName() << ": " << location.getLineNumber()
          << " (raised in file " << getFile() << ", line " << getLine() << ")";
        o << ": ";
        switch (kind)
        {
            case CompilationErrorKind::Lexical:
                o << "lexical error";
                break;
            case CompilationErrorKind::Syntactical:
                o << "syntactical error";
                break;
            case CompilationErrorKind::Semantical:
                o << "semantical error";
                break;
        };
        o << ": ";
        o << description;
        return o.str();
    }

};

} // namespace Id
