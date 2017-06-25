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

/// @file egolib/Script/PDLToken.hpp
/// @brief Token of the PDL (Program Definition Language) of EgoScript.
/// @author Michael Heilmann

#pragma once

#include "egolib/Script/PDLTokenKind.hpp"

namespace Ego {
namespace Script {

/// @brief A token of the PDL (Program Definition Language) of EgoScript.
struct PDLToken : public id::token<PDLTokenKind, PDLTokenKind::Unknown>
{
private:
    /// @brief The end location of this token.
    id::location m_endLocation;

    /// @brief The value of this token.
    int m_value;

public:
    /// @brief Construct this token with default values.
    /// @remark The text of the token is the empty string, its type and its location is unknown.
    PDLToken();

    /// @brief Construct this token with the specified values.
    /// @param kind the kind of the token
    /// @param startLocation the start location of the token
    /// @param endLocation the end location of the token
    /// @param lexeme the lexeme of this token. Default is the empty string.
    PDLToken(PDLTokenKind kind, const id::location& startLocation,
             const id::location& endLocation,
             const std::string& lexeme = std::string());

    /// @brief Copy-construct this token from another token.
    /// @param other the other token
    PDLToken(const PDLToken& other);

    /// @brief Move-construct this token from another token.
    /// @param other the other token
    PDLToken(PDLToken&& other);

    /// @brief Destruct this token.
    ~PDLToken();

    /// @brief Assign this token from another token.
    /// @param other the other token
    /// @return this token
    PDLToken& operator=(PDLToken other);

    friend void swap(PDLToken& x, PDLToken& y)
    {
        using std::swap;
        swap(static_cast<id::token<PDLTokenKind, PDLTokenKind::Unknown>&>(x), static_cast<id::token<PDLTokenKind, PDLTokenKind::Unknown>&>(y));
        swap(x.m_endLocation, y.m_endLocation);
        swap(x.m_value, y.m_value);
    }

    /// @brief Overloaded &lt;&lt; operator for a token.
    /// @param ostream the output stream to write to
    /// @param token the token to write
    /// @return the output stream
    friend std::ostream& operator<<(std::ostream& os, const PDLToken& token);

public:
    /// @brief Get if this token is an operator token i.e. is of one of the kinds which are called operators.
    /// @return @a true if this token is an operator token, @a false otherwise
    bool isOperator() const;

    /// @brief Get if this token is an assign operator token i.e. is of the kind "assign".
    /// @return @a true if this token is an assign operator token, @a false otherwise
    bool isAssignOperator() const;

    /// @brief Get if this token is a literal token i.e. is of the kinds which are called literals.
    /// @return @a true if this token is a literal token, @a false otherwise
    bool isLiteral() const;

    /// @brief Get if this token is a constant token i.e. is of the kind "constant".
    /// @return @a true if this token is a constant token, @a false otherwise
    bool isConstant() const;

    /// @brief Get the value of this token.
    /// @return the value of this token
    /// @see setValue
    int getValue() const
    {
        return m_value;
    }

    /// @brief Set the value of this token.
    /// @param value the value
    /// @see getValue
    void setValue(int value)
    {
        m_value = value;
    }

    /// @brief @brief Get the end location of this token.
    /// @return the end location of this token
    /// @see setEndLocation
    /// @remark The end location is the location at which the lexeme of this token ends at.
    const id::location& getEndLocation() const
    {
        return m_endLocation;
    }

    /// @brief Set the end location of this token.
    /// @param endLocation the end location of this token
    /// @see getEndlocation
    void setEndLocation(const id::location& endLocation)
    {
        m_endLocation = endLocation;
    }
};

std::ostream& operator<<(std::ostream& os, const PDLToken& token);

} // namespace Script
} // namespace Ego
