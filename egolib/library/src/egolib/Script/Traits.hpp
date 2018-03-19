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

namespace Ego::Script
{

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
struct Traits;

template <>
struct Traits<char>
{

public:

    /**
     * @brief
     *  The type.
     */
    using Type = char;

    /// @brief A unicode code point in UTF-8 encoding..
    using ExtendedType = int;

public:
    /// @brief The first code point of the private use area in the basic multilingual plane in UTF-8 encoding.
    /// http://www.fileformat.info/info/unicode/char/e000/index.htm
    static constexpr int first_pua_bmp = 0xee8080;

    /// @brief The last code point of the private use area in the basic multilingual plane in UTF-8 encoding.
    /// http://www.fileformat.info/info/unicode/char/f8ff/index.htm
    static constexpr int last_pua_bmp = 0xefa3bf;

    /// @brief The code point of the zero terminator in UTF-8 encoding.
    static constexpr int zt = 0x000000;

    /// @brief Get the unicode code point in UTF-8 encoding indicating the start of the input.
    /// @return the unicode code point
    static constexpr ExtendedType startOfInput() noexcept
    {
        return first_pua_bmp + 1;
    }

    /// @brief Get the extended character value indicating the end of the input.
    /// @return the extended character value
    static constexpr ExtendedType endOfInput() noexcept
    {
        return first_pua_bmp + 2;
    }
    
    /// @brief Get the extended character value indicating an error.
    /// @return the extended character value
    static constexpr ExtendedType error() noexcept
    {
        return first_pua_bmp + 3;
    }

    /// @brief Get if a unicode code point in UTF-8 encoding is
    /// in the private use area in the basic multilingual plane.
    /// @param x the unicode code point in UTF-8 encoding
    /// @return  @a true if @a x is in the private use area in the basic multilingual plane, @a false otherwise
    static constexpr bool is_pua_bmp(ExtendedType x) noexcept
    {
        return (first_pua_bmp <= x)
            && (x <= last_pua_bmp);
    }

    /// @brief Get if a unicode code point in UTF-8 encoding is the zero-terminator.
    /// @param x the unicode code point in UTF-8 encoding
    /// @return  @a true if @a x is the zero-terminator, @a false otherwise
    static constexpr bool is_zt(ExtendedType x) noexcept
    {
        return zt == x;
    }
};

} // namespace Ego::Script
