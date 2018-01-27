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

/// @file egolib/Script/DDLTokenDecoder.hpp
/// @brief Decoder from DDL tokens to C/C++ values.
/// @author Michael Heilmann

#pragma once

#include "egolib/Script/DDLToken.hpp"

namespace Ego {
namespace Script {

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
struct DDLTokenDecoder<TargetType, std::enable_if_t<idlib::c::internal::is_boolean<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!idlib::c::decoder<TargetType>()(token.get_lexeme(), temporary))
        {
            throw idlib::c::compilation_error(__FILE__, __LINE__, idlib::c::compilation_error_kind::lexical, token.get_start_location(),
                                           "unable to convert literal `" + token.get_lexeme() +
                                           "` into a value of EgoScript boolean type `" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

/// @brief Specialization for EgoScript character types.
template <typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<idlib::c::internal::is_character<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!idlib::c::decoder<TargetType>()(token.get_lexeme(), temporary))
        {
            throw idlib::c::compilation_error(__FILE__, __LINE__, idlib::c::compilation_error_kind::lexical, token.get_start_location(),
                                           "unable to convert literal `" + token.get_lexeme() +
                                           "` into a value of EgoScript character type `" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

/// @brief Specialization for EgoScript real types.
template <typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<idlib::c::internal::is_real<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!idlib::c::decoder<TargetType>()(token.get_lexeme(), temporary))
        {
            throw idlib::c::compilation_error(__FILE__, __LINE__, idlib::c::compilation_error_kind::lexical, token.get_start_location(),
                                           "unable to convert literal `" + token.get_lexeme() +
                                           "` into a value of EgoScript real type " + "`" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

/// @brief Specialization for EgoScript integer types.
template <typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<idlib::c::internal::is_integer<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!idlib::c::decoder<TargetType>()(token.get_lexeme(), temporary))
        {
            throw idlib::c::compilation_error(__FILE__, __LINE__, idlib::c::compilation_error_kind::lexical, token.get_start_location(),
                                           "unable to convert literal `" + token.get_lexeme() +
                                           "` into a value of EgoScript integer type " + "`" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

/// @brief Specialization for EgoScript natural types.
template<typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<idlib::c::internal::is_natural<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!idlib::c::decoder<TargetType>()(token.get_lexeme(), temporary))
        {
            throw idlib::c::compilation_error(__FILE__, __LINE__, idlib::c::compilation_error_kind::lexical, token.get_start_location(),
                                           "unable to convert literal `" + token.get_lexeme() +
                                           "` into a value of EgoScript natural type " + "`" + typeid(int).name() + "`");
        }
        return temporary;
    }
};

/// @brief Specialization for EgoScript string types.
template <typename TargetType>
struct DDLTokenDecoder<TargetType, std::enable_if_t<idlib::c::internal::is_string<TargetType>::value>>
{
    TargetType operator()(const DDLToken& token) const
    {
        TargetType temporary;
        if (!idlib::c::decoder<TargetType>()(token.get_lexeme(), temporary))
        {
            throw idlib::c::compilation_error(__FILE__, __LINE__, idlib::c::compilation_error_kind::lexical, token.get_start_location(),
                                           "unable to convert literal `" + token.get_lexeme() +
                                        "` into a value of EgoScript string type " + "`" + typeid(TargetType).name() + "`");
        }
        return temporary;
    }
};

} } // namespace Ego::Script
