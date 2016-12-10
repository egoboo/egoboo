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

/// @file egolib/Script/Token.hpp
/// @brief Token of the DDL (Data Definition Language) of EgoScript.
/// @author Michael Heilmann

#pragma once

#include "egolib/typedef.h"
#include "egolib/Script/Conversion.hpp"

namespace Ego {
namespace Script {

/// @brief A token of the DDL (Data Definition Language) of EgoScript.
struct DDLToken
{
    /// @brief An enumeration of the token kinds of the DDL of EgoScript.
    enum class Kind
    {
        /// @brief An integer number literal.
        Integer,

        /// @brief A real number literal.
        Real,

        /// @brief A character literal.
        Character,

        /// @brief A string literal.
        String,

        /// @brief A boolean literal.
        Boolean,

        /// @brief A name (aka identifier).
        Name,

        /// @brief A comment.
        Comment,
    };

private:
    /// @brief The kind of this token.
    Kind m_kind;

    /// @brief The start location of this token.
    Id::Location m_startLocation;

    /// @brief The lexeme of this token.
    std::string m_lexeme;

public:
    /// @brief Construct this token.
    /// @param kind the kind of this token
    /// @param startLocation the start location of this token
    /// @param lexeme the lexeme of this token. Default is the empty string.
    DDLToken(Kind kind, const Id::Location& startLocation, const std::string& lexeme = std::string());

    /// @brief Copy-construct this token from another token.
    /// @param other the other token
    DDLToken(const DDLToken& other);

    /// @brief Get the kind of this token.
    /// @return the kind of this token
    Kind getKind() const;

    /// @brief Get the start location of this token.
    /// @return the start location of this token
    const Id::Location& getStartLocation() const;

    /// @brief Assign this token from another token.
    /// @param other the construction source
    /// @return this token
    DDLToken& operator=(const DDLToken& other);

    /// @brief Get the lexeme of this token.
    /// @return the lexeme of this token
    const std::string& getLexeme() const;

};

/// @brief Convert this token into a value of type @a type.
/// @tparam Type the type of the value to convert the value of this token into
/// @return the converted value
/// @throw Id::LexicalErrorException the conversion failed
template <typename TargetType, typename EnabledType = void>
struct DDLTokenDecoder
{
    TargetType operator()(const DDLToken& token) const;
};

/// @brief Specialization for EgoScript boolean types.
template <typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<IsBoolean<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!Decoder<TargetType>()(token.getLexeme(), temporary))
        {
            throw Id::LexicalErrorException(__FILE__, __LINE__, token.getStartLocation(),
                                            "unable to convert literal `" + token.getLexeme() +
                                            "` into a value of EgoScript boolean type `" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

/// @brief Specialization for EgoScript character types.
template <typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<IsCharacter<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!Decoder<TargetType>()(token.getLexeme(), temporary))
        {
            throw Id::LexicalErrorException(__FILE__, __LINE__, token.getStartLocation(),
                                            "unable to convert literal `" + token.getLexeme() +
                                            "` into a value of EgoScript character type `" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

/// @brief Specialization for EgoScript real types.
template <typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<IsReal<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!Decoder<TargetType>()(token.getLexeme(), temporary))
        {
            throw Id::LexicalErrorException(__FILE__, __LINE__, token.getStartLocation(),
                                            "unable to convert literal `" + token.getLexeme() +
                                            "` into a value of EgoScript real type " + "`" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

/// @brief Specialization for EgoScript integer types.
template <typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<IsInteger<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!Decoder<TargetType>()(token.getLexeme(), temporary))
        {
            throw Id::LexicalErrorException(__FILE__, __LINE__, token.getStartLocation(),
                                            "unable to convert literal `" + token.getLexeme() +
                                            "` into a value of EgoScript integer type " + "`" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

/// @brief Specialization for EgoScript natural types.
template<typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<IsNatural<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!Decoder<TargetType>()(token.getLexeme(), temporary))
        {
            throw Id::LexicalErrorException(__FILE__, __LINE__, token.getStartLocation(),
                                            "unable to convert literal `" + token.getLexeme() +
                                            "` into a value of EgoScript natural type " + "`" + typeid(int).name() + "`");
        }
        return temporary;
    }
};

/// @brief Specialization for EgoScript string types.
template <typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<IsString<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!Decoder<TargetType>()(token.getLexeme(), temporary))
        {
            throw Id::LexicalErrorException(__FILE__, __LINE__, token.getStartLocation(),
                                            "unable to convert literal `" + token.getLexeme() +
                                            "` into a value of EgoScript string type " + "`" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

} // namespace Script
} // namespace Ego
