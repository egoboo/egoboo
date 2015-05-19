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

namespace Ego
{

using namespace std;

/**
 * @brief
 *  Convert a character to upper case.
 * @param chr
 *  the character
 * @param lc
 *  the locale to use. Default is std::locale().
 * @return
 *  the upper case character
 * @todo
 *  Rename to @a toUpper.
 */
template <class CharType>
inline CharType toupper(CharType chr, const std::locale& lc = locale())
{
    return std::toupper(chr, lc);
}

/**
 * @brief
 *  In-place convert a string to upper case.
 * @param str
 *  the string
 * @param lc
 *  the locale to use. Default is std::locale().
 * @todo
 *  Rename to @a toUpper.
 */
template <typename CharType>
inline void toupper(basic_string<CharType>& str, const locale& lc = locale())
{
    // Capture lc by reference, capture nothing else.
    auto f = [&lc](const CharType chr) -> CharType { return Ego::toupper(chr, lc); };
    transform(str.begin(), str.end(), str.begin(), f);
}

/**
 * @brief
 *  Convert a character to lower case.
 * @param chr
 *  the character
 * @param lc
 *  the locale to use. Default is std::locale().
 * @return
 *  the lower case character
 * @todo
 *  Rename to @a toLower.
 */
template <class CharType>
inline CharType tolower(CharType chr, const locale& lc = locale())
{
    return std::tolower(chr, lc);
}

/**
 * @brief
 *  In-place convert a string to lower case.
 * @param str
 *  the string
 * @param lc
 *  the locale to use. Default is std::locale().
 * @todo
 *  Rename to @a toLower.
 */
template <class CharType>
inline void tolower(basic_string<CharType>& str, const locale& lc = locale())
{
    // Capture lc by reference, capture nothing else.
    auto f = [&lc] (const CharType chr) -> CharType { return Ego::tolower(chr, lc); };
    transform(str.begin(), str.end(), str.begin(), f);
}

/**
 * @brief
 *  Get if a character is an alphabetic character.
 * @param chr
 *  the character
 * @param lc
 *  the locale to use. Default is std::locale().
 * @return
 *  @a true if the character is an alphabetic character, @a false otherwise
 * @todo
 *  Rename to @a isAlpha.
 */
template <class CharType>
inline bool isalpha(CharType chr, const std::locale& lc = locale())
{
    return std::isalpha(chr, lc);
}

/**
 * @brief
 *  Get if a character is a printable character.
 * @param chr
 *  the character
 * @param lc
 *  the locale to use. Default is std::locale().
 * @return
 *  @a true if the character is a printable character, @a false otherwise
 * @todo
 *  Rename to @a isPrint.
 */
template <class CharType>
inline bool isprint(CharType chr, const std::locale& lc = std::locale())
{
    return std::isprint(chr, lc);
}

/**
 * @brief
 *  Get if a character is a digit character.
 * @param chr
 *  the character
 * @param lc
 *  the locale to use. Default is std::locale().
 * @return
 *  @a true if the character is a digit character, @a false otherwise
 * @todo
 *  Rename to @a isDigit.
 */
template <class CharType>
inline bool isdigit(CharType chr, const std::locale& lc = std::locale())
{
    return std::isdigit(chr, lc);
}

/**
 * @brief
 *  Get if a character is a control character.
 * @param chr
 *  the character
 * @param lc
 *  the locale to use. Default is std::locale().
 * @return
 *  @a true if the character is a control character, @a false otherwise
 * @todo
 *  Rename to @a isCntrl.
 */
template <class CharType>
inline bool iscntrl(CharType chr, const std::locale& lc = std::locale())
{
    return std::iscntrl(chr, lc);
}

/**
 * @brief
 *  Get if a character is a whitespace character.
 * @param chr
 *  the character
 * @param lc
 *  the locale to use. Default is std::locale().
 * @return
 *  @a true if the character is a whitespace character, @a false otherwise
 * @todo
 *  Rename to @a isSpace.
 */
template <class CharType>
inline bool isspace(CharType chr, const std::locale& lc = locale())
{
    return std::isspace(chr, lc);
}

/**
 * @brief
 *  Trim leading and trailing spaces.
 * @param str
 *  the string
 * @param lc
 *  the locale to use. Default is std::locale().
 * @return
 *  a string equal to @a str but with leading and trailing spaces removed
 */
template <typename CharType>
inline std::basic_string<CharType> trim(const std::basic_string<CharType>& str, const locale& lc = locale())
{
    // Capture lc by reference, capture nothing else.
    auto f = [&lc](const CharType chr) { return Ego::isspace(chr, lc); };
    auto front = std::find_if_not(str.begin(), str.end(), f);
    return std::basic_string<CharType>(front, std::find_if_not(str.rbegin(), typename std::basic_string<CharType>::const_reverse_iterator(front), f).base());
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
vector< basic_string<CharType> > split(const basic_string<CharType>& str, const basic_string<CharType>& delims)
{
    vector< basic_string<CharType> > v;
    typename basic_string<CharType>::size_type start = 0;
    auto pos = str.find_first_of(delims, start);
    while (pos != basic_string<CharType>::npos)
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

/**
 * @brief
 *  Get if a string is a suffix of another string.
 * @param string
 *  the string
 * @param suf
 *  the suffix
 * @return
 *  @a true if the string ends with the suffix,
 *  @a false otherwise
 */
template <typename CharType>
bool isSuffix(const basic_string<CharType>& string, const basic_string<CharType>& suffix) {
    if (suffix.size() > string.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), string.rbegin());
}

} // namespace Ego
