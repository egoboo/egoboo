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
struct PDLToken
{
private:
    /// @brief The kind of this token.
    PDLTokenKind m_kind;

    /// @brief The start location of this token.
    Id::Location m_startLocation;

    /// @brief The value of this token.
    int m_value;

    /// @brief The lexeme of this token.
    std::string m_lexeme;

public:
    /// @brief Get if this token is of the given kinds.
    /// @param kind the kind
    /// @return @a true if this token is of the given kind @a kind, @a false otherwise
    bool is(PDLTokenKind kind) const;

    /// @brief Get if this token is of one of the given kinds.
    /// @param kind1, kind2 the kinds
    /// @return @a true if this token is of one of the given kinds @a kind1 or @a kind2, @a false otherwise
    bool isOneOf(PDLTokenKind kind1, PDLTokenKind kind2) const;

    /// @brief Get if this token is of one of the given kinds.
    /// @param kind1, kind2, kinds ... the kinds
    /// @return @a true if this token is of one of the given kinds @a kind1, @a kind2, or @a kinds ..., @a false otherwise
    template <typename ... Kinds>
    bool isOneOf(PDLTokenKind kind1, PDLTokenKind kind2, Kinds ... kinds) const
    {
        return is(kind1) || isOneOf(kind2, kinds ...);
    }

    /// @brief Get if this token is an operator token i.e. is of one of the kinds which are called operators.
    /// @return @a true if this token is an operator token, @a false otherwise
    bool isOperator() const;

    /// @brief Get if this token is an assign operator token i.e. is of the kind "assign".
    /// @return @a true if this token is an assign operator token, @a false otherwise
    bool isAssignOperator() const;

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

    /// @brief Get the start location of this token.
    /// @return the start location of this token
    /// @see setStartLocation
    /// @remark The start location is the location at which the lexeme of this token starts at.
    Id::Location getStartLocation() const;

    /// @brief Set the start location of this token.
    /// @param startLocation the start location
    /// @see getStartLocation
    void setStartLocation(const Id::Location& startLocation);

    /// @brief Get the kind of this token.
    /// @return the kind of this token
    /// @see setKind
    PDLTokenKind getKind() const;

    /// @brief Set the kind of this token.
    /// @param kind the kind
    /// @see getKind
    void setKind(PDLTokenKind kind);

    /// @brief Set the lexeme of this token.
    /// @param lexeme the lexeme
    /// @see getLexeme
    void setLexeme(const std::string& lexeme);

    /// @brief Get the lexeme of this token.
    /// @return the lexeme of this token
    /// @see setLexeme
    const std::string& getLexeme() const;

    /// @brief Construct this token with default values.
    /// @remark The text of the token is the empty string, its type and its location is unknown.
    PDLToken();

    /// @brief Construct this token with the specified values.
    /// @param kind the kind of the token
    /// @param startLocation the start location of the token
    /// @param lexeme the lexeme of this token. Default is the empty string.
    PDLToken(PDLTokenKind kind, const Id::Location& startLocation, const std::string& lexeme = std::string());

    /// @brief Copy-construct this token from another token.
    /// @param other the other token
    PDLToken(const PDLToken& other);

    /// @brief Destruct this token.
    ~PDLToken();

    friend std::ostream& operator<<(std::ostream& os, const PDLToken& token);
};

/// @brief Overloaded &lt;&lt; operator for a token.
/// @param ostream the output stream to write to
/// @param token the token to write
/// @return the output stream
std::ostream& operator<<(std::ostream& os, const PDLToken& token);

} // namespace Script
} // namespace Ego
