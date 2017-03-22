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

/// @file   egolib/Core/StringUtilities.hpp
/// @brief  String utility functions (splitting, trimming, case conversion, ...)
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"
#include "idlib/to_lower.hpp"
#include "idlib/to_upper.hpp"
#include "idlib/prefix.hpp"
#include "idlib/suffix.hpp"

namespace Ego {

/// @brief Get if a character is an alphabetic character.
/// @param chr the character
/// @param lc the locale to use. Default is std::locale().
/// @return @a true if the character is an alphabetic character, @a false otherwise
template <class CharType>
inline bool isalpha(CharType chr, const std::locale& lc = std::locale()) {
    return std::isalpha(chr, lc);
}

/// @brief Get if a character is a printable character.
/// @param chr the character
/// @param lc the locale to use. Default is std::locale().
/// @return @a true if the character is a printable character, @a false otherwise
template <class CharType>
inline bool isprint(CharType chr, const std::locale& lc = std::locale()) {
    return std::isprint(chr, lc);
}

/// @brief Get if a character is a digit character.
/// @param chr the character
/// @param lc the locale to use. Default is std::locale().
/// @return @a true if the character is a digit character, @a false otherwise
template <class CharType>
inline bool isdigit(CharType chr, const std::locale& lc = std::locale()) {
    return std::isdigit(chr, lc);
}

/// @brief Get if a character is a control character.
/// @param chr the character
/// @param lc the locale to use. Default is std::locale().
/// @return @a true if the character is a control character, @a false otherwise
template <class CharType>
inline bool iscntrl(CharType chr, const std::locale& lc = std::locale()) {
    return std::iscntrl(chr, lc);
}

/// @brief Get if a character is a whitespace character.
/// @param chr the character
/// @param lc the locale to use. Default is std::locale().
/// @return @a true if the character is a whitespace character, @a false otherwise
template <class CharType>
inline bool isspace(CharType chr, const std::locale& lc = std::locale()) {
    return std::isspace(chr, lc);
}

} // namespace Ego

namespace Ego {

/// @brief Trim leading and trailing characters matching a predicate.
/// @param str the string
/// @param pred the predicate
/// @param lc the locale to use. Default is std::locale().
/// @return a string equal to @a str but with leading and trailing characters removed
template <typename CharType>
inline std::basic_string<CharType> trim(const std::basic_string<CharType>& str, std::function<bool(const CharType&)> pred, const std::locale& lc = std::locale()) {
    auto front = std::find_if_not(str.begin(), str.end(), pred);
    auto back = std::find_if_not(str.rbegin(), typename std::basic_string<CharType>::const_reverse_iterator(front), pred).base();
    return std::basic_string<CharType>(front, back);
}

/// @brief Trim leading characters matching a predicate.
/// @param str the string
/// @param pred the predicate
/// @param lc the locale to use. Default is std::locale().
/// @return a string equal to @a str but with leading characters removed
template <typename CharType>
inline std::basic_string<CharType> left_trim(const std::basic_string<CharType>& str, std::function<bool(const CharType&)> pred, const std::locale& lc = std::locale()) {
    auto front = std::find_if_not(str.begin(), str.end(), pred);
    auto back = str.end();
    return std::basic_string<CharType>(front, back);
}

/// @brief Trim trailing characters matching a predicate.
/// @param str the string
/// @param pred the predicate
/// @param lc the locale to use. Default is std::locale().
/// @return a string equal to @a str but with trailing characters removed
template <typename CharType>
inline std::basic_string<CharType> right_trim(const std::basic_string<CharType>& str, std::function<bool(const CharType&)> pred, const std::locale& lc = std::locale()) {
    auto front = str.begin();
    auto back = std::find_if_not(str.rbegin(), typename std::basic_string<CharType>::const_reverse_iterator(front), pred).base();
    return std::basic_string<CharType>(front, back);
}

/// @brief Trim leading spaces.
/// @param str the string
/// @param lc the locale to use. Default is std::locale().
/// @return a string equal to @a str but with leading spaces removed
template <typename CharType>
inline std::basic_string<CharType> left_trim_ws(const std::basic_string<CharType>& str, const std::locale& lc = std::locale()) {
    // Capture lc by reference, capture nothing else.
    return left_trim<CharType>(str, [&lc](const CharType& chr) { return Ego::isspace(chr, lc); }, lc);
}

/// @brief Trim trailing spaces.
/// @param str the string
/// @param lc the locale to use. Default is std::locale().
/// @return a string equal to @a str but with trailing spaces removed
template <typename CharType>
inline std::basic_string<CharType> right_trim_ws(const std::basic_string<CharType>& str, const std::locale& lc = std::locale()) {
    // Capture lc by reference, capture nothing else.
    return right_trim<CharType>(str, [&lc](const CharType& chr) { return Ego::isspace(chr, lc); }, lc);
}

/// @brief Trim leading and trailing spaces.
/// @param str the string
/// @param lc the locale to use. Default is std::locale().
/// @return a string equal to @a str but with leading and trailing spaces removed
template <typename CharType>
inline std::basic_string<CharType> trim_ws(const std::basic_string<CharType>& str, const std::locale& lc = std::locale()) {
    // Capture lc by reference, capture nothing else.
    return trim<CharType>(str, [&lc](const CharType& chr) { return Ego::isspace(chr, lc); }, lc);
}

/**
 * @brief
 *  Split a string.
 * @param str
 *  the string
 * @param delims
 *  a string of delimiters
 * @return
 *  a list of tokens
 */
template <typename CharType>
std::vector<std::basic_string<CharType>> split(const std::basic_string<CharType>& str, const std::basic_string<CharType>& delims)
{
    std::vector<std::basic_string<CharType>> v;
    typename std::basic_string<CharType>::size_type start = 0;
    auto pos = str.find_first_of(delims, start);
    while (pos != std::basic_string<CharType>::npos)
    {
        if (pos > start) // If the contents before the delimiter are non-empty, add them.
        {
            v.emplace_back(str, start, pos - start);
        }
        start = pos + 1;
        v.emplace_back(str, start - 1, 1); // At the delimiter itself.
        pos = str.find_first_of(delims, start);
    }
    if (start < str.length()) // If the contents before the end are non-empty, add them.
    {
        v.emplace_back(str, start, str.length() - start);
    }
    return v; // Return value optimization/move semantics hopefully kick-in here.
}

} // namespace Ego
