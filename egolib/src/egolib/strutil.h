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

/// @file  egolib/strutil.h
/// @brief String manipulation functions.

#pragma once

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

/// end-of-string character. assume standard null terminated string
#   define CSTR_END '\0'
#   define EMPTY_CSTR { CSTR_END }

#   define VALID_CSTR(PSTR)   ((NULL!=PSTR) && (CSTR_END != PSTR[0]))
#   define INVALID_CSTR(PSTR) ((NULL==PSTR) || (CSTR_END == PSTR[0]))

/**
 * @brief
 *  Convert a character to upper case using the current locale.
 * @param chr
 *  the character
 * @return
 *  the upper case character
 */
template <class CharType>
inline CharType char_toupper(CharType chr, const std::locale& locale = std::locale())
{
    return std::toupper(chr, locale);
}

/**
* @brief
*  Convert a character to lower case using the current locale.
* @param chr
*  the character
* @return
*  the lower case character
*/
template <class CharType>
inline CharType char_tolower(CharType chr, const std::locale& locale = std::locale())
{
    return std::tolower(chr, locale);
}

/**
 * @brief
 *  Get if a character is a whitespace character using the current locale.
 * @param chr
 *  the character
 * @return
 *  @a true if the character is a whitespace character, @a false otherwise
 */
template <class CharType>
inline bool char_isspace(CharType chr, const std::locale& locale = std::locale())
{
    return std::isspace(chr, locale);
}

/**
 * @brief
 *  Trim leading and trailing spaces.
 * @param str
 *  the string
 * @return
 *  a string equal to @a str but with leading and trailing spaces removed
 */
template <typename CharType>
inline std::basic_string<CharType> trim(const std::basic_string<CharType>& str, const std::locale& locale = std::locale())
{
    auto isspace = [=,&locale](const CharType chr) { return std::isspace(chr, locale); };
    auto front = std::find_if_not(str.begin(), str.end(), isspace);
    return std::basic_string<CharType>(front, std::find_if_not(str.rbegin(), typename std::basic_string<CharType>::const_reverse_iterator(front), isspace).base());
}

// libc++ and gcc doesn't define std::toupper<int>(int, std::locale),
// so narrow the int to a char.
#if defined(_LIBCPP_VERSION) || defined(__GNUC__)
template <>
inline int char_toupper<int>(int chr, const std::locale& locale)
{
    EGOBOO_ASSERT(chr >= 0 && chr <= 255);
    return char_toupper(static_cast<char>(chr), locale);
}
template <>
inline int char_tolower<int>(int chr, const std::locale& locale)
{
    EGOBOO_ASSERT(chr >= 0 && chr <= 255);
    return char_tolower(static_cast<char>(chr), locale);
}
template <>
inline bool char_isspace<int>(int chr, const std::locale& locale)
{
    EGOBOO_ASSERT(chr >= 0 && chr <= 255);
    return char_isspace(static_cast<char>(chr), locale);
}
#endif

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    char * str_decode( char *strout, size_t insize, const char * strin );
    char * str_encode( char *strout, size_t insize, const char * strin );
    char * str_encode_path( const char *character );

    char * str_clean_path( char * str, size_t size );
    char * str_convert_slash_net( char * str, size_t size );
    char * str_convert_slash_sys( char * str, size_t size );

    char * str_append_slash( char * str, size_t size );
    char * str_append_slash_net( char * str, size_t size );

    void   str_trim( char *pStr );
    void   str_add_linebreaks( char * text, size_t text_len, size_t line_len );

#if defined(__GNUC__) && !(defined (__MINGW) || defined(__MINGW32__))
    char* strupr( char * str );
    char* strlwr( char * str );
#endif
