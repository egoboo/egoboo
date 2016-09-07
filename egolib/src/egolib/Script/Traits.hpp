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

#include "egolib/platform.h"

namespace Ego
{
namespace Script
{

using namespace std;

/**
 * @brief
 *  Necessary descriptions of a type for generic textual input/output.
 *  Specializations of the traits template for a type (@a Type) provide a corresponding extended type
 *  (@a ExtendedType). The extended type is able to encode all values of the type plus a number of
 *  in-band values providing information about the start and the end of the input as well as errors.
 * @remark
 *  A specialization for type @a char is provided which is supposed to read UTF-8 NOBOM constrained
 *  to one Byte UTF-8 sequences modulo <tt>'\0'</tt>. Future extensions will include full UTF-8 NOBOM
 *  modulo <tt>'\0'</tt> support.
 * @todo
 *  Add full UTF-8 NOBOM modulo <tt>'\0'</tt> support.
 * @author
 *  Michael Heilmann
 */
template <typename Type>
struct Traits
{
};

template <>
struct Traits<char>
{

public:

    /**
     * @brief
     *  The type.
     */
    using Type = char;

    /**
     * @brief
     *  The extended type.
     */
    using ExtendedType = int;

public:

    /**
     * @brief
     *  Get the extended character value indicating the start of the input.
     * @return
     *  the extended character value
     */
    static ExtendedType startOfInput()
    {
        return static_cast<ExtendedType>(std::numeric_limits<Type>::max()) + 1;
    }

    /**
     * @brief
     *  Get the extended character value indicating the end of the input.
     * @return
     *  the extended character value
     */
    static ExtendedType endOfInput()
    {
        return static_cast<ExtendedType>(std::numeric_limits<Type>::max()) + 2;
    }
    
    /**
     * @brief
     *  Get the extended character value indicating an error.
     * @return
     *  the extended character value
     */
    static ExtendedType error()
    {
        return static_cast<ExtendedType>(numeric_limits<Type>::max()) + 3;
    }

    /**
     * @brief
     *  Get if a specified extended character is within
     *  the sub-set of extended character values representing valid characters.
     * @param echr
     *  the extended character
     * @return
     *  @a true if @a echr is within sub-set of extended character values representing valid characters,
     *  @a false otherwise
     */
    static bool isValid(const ExtendedType& echr)
    {
        return '\0' != echr
            && echr >= static_cast<ExtendedType>(numeric_limits<Type>::min())
            && echr <= static_cast<ExtendedType>(numeric_limits<Type>::max())
            ;
    }

    /**
     * @brief
     *  Get if an extended character is a whitespace character.
     * @param echr
     *  the extended character
     * @return
     *  @a true if the extended character is a whitespace character,
     *  @a false otherwise
     * @remark
     *  @code
     *  whitespace = Space | Tabulator
     *  @endcode
     */
    static bool isWhiteSpace(const ExtendedType& echr)
    {
        return ' ' == echr
            || '\t' == echr
                ;
    }

    /**
     * @brief
     *  Get if an extended character is a newline character.
     * @param echr
     *  the extended character
     * @return
     *  @a true if the extended character is a newline character,
     *  @a false otherwise
     * @remark
     *  @code
     *  newline = LineFeed | CarriageReturn
     *  @endcode
     */
    static bool isNewLine(const ExtendedType& echr)
    {
        return '\n' == echr
            || '\r' == echr
                ;
    }

    /**
     * @brief
     *  Get if an extended character is an alphabetic character.
     * @param echr
     *  the extended character
     * @return
     *  @a true if the extended character is an alphabetic character,
     *  @a false otherwise
     * @remark
     *  @code
     *  alphabetic := 'a' .. 'z'
     *              | 'A' .. 'Z'
     *  @endcode
     */
    static bool isAlphabetic(const ExtendedType& echr)
    {
        return ('a' <= echr && echr <= 'z')
            || ('A' <= echr && echr <= 'Z')
                ;
    }

    /**
     * @brief
     *  Get if an extended character is a digit character.
     * @param echr
     *  the extended character
     * @return
     *  @a true if the extended character is a digit character,
     *  @a false otherwise
     * @remark
     *  @code
     *  digit := '0' .. '9'
     *  @endcode
     */
    static bool isDigit(const ExtendedType& echr)
    {
        return '0' <= echr
            && echr <= '9'
                ;
    }

};

} // namespace Script
} // namespace Ego
