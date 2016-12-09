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

#include "game/egoboo.h"

#define MAX_OPCODE 1024 ///< Number of lines in AICODES.TXT

/// @brief A token.
struct Token
{
    enum class Type
    {
        Unknown,
        Function,
        Variable,
        Constant,

        Assign, ///< Token type of an "assign" operator.
        And, ///< Token type of an "and" operator.
        Plus, ///< Token type of a "plus" ("add" or "unary plus") operator.
        Minus, ///< Token type of a "minus" ("subtract" or "unary minus") operator.
        Multiply, ///< Token type of a "multiply" operator.
        Divide, ///< Token type of a "divide" operator.
        Modulus, ///< Token type of a "modulus" operator.
        ShiftRight, ///< Token type of a "shift right" operator.
        ShiftLeft, ///< Token type of a "shift left" operator.

        /// Token type of zero or more whitespaces.
        /// getValue indicates the number of whitespaces.
        Whitespace,

        /// Token type of zero or more newlines.
        /// getValue indicates the number of newlines.
        Newline,

        /// Token type of zero to 15 indentions.
        /// getValue indicates the number of indentions.
        Indent,

        /// Token type of the end of a line.
        EndOfLine,

        /// Token type of a numeric literal.
        NumericLiteral,

        /// Token type of a name.
        Name,

        /// Token type of a string.
        String,

        /// Token type of a reference.
        Reference,

        /// Token type of an IDSZ.
        IDSZ,
    };

private:
    /// @brief The type of this token.
    Type _type;

    /// @brief The location.
    Location _location;

    /// @brief The value of this token.
    int _value;

    /// @brief The text of this token.
    std::string _text;

public:
    /// @brief Get if this token is of the given token type.
    /// @param type the token type
    /// @return @a true if this token is of the token type @a type, @a false otherwise
    bool is(Type type) const;

    /// @brief Get if this token is of one of the given token types.
    /// @param type0, type1 the token types
    /// @return @a true if this token is of one of the given token types, @a false otherwise
    bool isOneOf(Type type0, Type type1) const;

    /// @brief Get if this token is of one of the given token types.
    /// @param type0, type1, types the token types
    /// @return @a true if this token is of one of the given token types, @a false otherwise
    template <typename ... Types>
    bool isOneOf(Type type0, Type type1, Types ... types) const
    {
        return is(type0) || isOneOf(type1, types ...);
    }

    /// @brief Get if this token is an operator token.
    /// @return @a true if this token is an operator token, @a false otherwise
    bool isOperator() const;

    /// @brief Get if this token is an "assign" operator token.
    /// @return @a true if this token is an "assign" operator token, @a false otherwise
    bool isAssignOperator() const;

    /// @brief Get the value of this token.
    /// @return the value of this token
    /// @see setValue
    int getValue() const
    {
        return _value;
    }

    /// @brief Set the value of this token.
    /// @param value the value
    /// @see getValue
    void setValue(int value)
    {
        _value = value;
    }

    /// @brief Get the location of this token.
    /// @return the location of this token
    /// @see setLocation
    /// @remark The location is the location at which the lexeme of this token starts at.
    Location getLocation() const;

    /// @brief Set the location of this token.
    /// @param location the location
    /// @see getLocation
    void setLocation(const Location& location);

    /// @brief Get the type of this token.
    /// @return the type of this token
    /// @see setType
    Type getType() const;

    /// @brief Set the type of this token.
    /// @param type the type
    /// @see getType
    void setType(Type type);

    /// @brief Set the text of this token.
    /// @param text the text
    /// @see getText
    void setText(const std::string& text);

    /// @brief Get the text of this token.
    /// @return the text of this token
    /// @see setText
    const std::string& getText() const;

    /// @brief Construct this token with default values.
    /// @remark The text of the token is the empty string, its type and its location is unknown.
    Token();

    /// @brief Construct this token with the specified type, location, and text.
    /// @param type the type of the token
    /// @param location the location of the token
    /// @remark The text of the token is the empty string.
    Token(Type type, const Location& location);

    /// @brief Construct this token with values of another token.
    /// @param other the other token
    Token(const Token& other);

    /// @brief Destruct this token.
    ~Token();

    friend std::ostream& operator<<(std::ostream& os, const Token& token);
};

/// @brief Overloaded &lt;&lt; operator for a token type.
/// @param ostream the output stream to write to
/// @param tokenType the token type to write
/// @return the outputstream
std::ostream& operator<<(std::ostream& os, const Token::Type& tokenType);

/// @brief Overloaded &lt;&lt; operator for a token.
/// @param ostream the output stream to write to
/// @param token the token to write
/// @return the output stream
std::ostream& operator<<(std::ostream& os, const Token& token);
