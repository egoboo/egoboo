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

/// EgoScript knows five type categories.
/// Those are:
/// a) integer types,
/// b) natural types,
/// c) boolean types,
/// d) real types,
/// e) character types, and
/// f) string types.

/// From C++:
/// a) char, unsigned char, signed char are three distinct types.
/// b) Furthermore, neither uint8_t is necessarily the same type as unsigned char nor int8_t is necessarily the same type as signed char.
/// From EgoScript:
/// a) signed char is an EgoScript integer type and unsigned char is an EgoScript natural type.
/// b) float and double are EgoScript real types.
/// c) bool is an EgoScript boolean type.
/// d) char is an EgoScript character type.
/// e) std::string is an EgoScript string type.
template <typename Type, typename Enabled = void>
struct IsInteger;
template <typename Type, typename Enabled = void>
struct IsNatural;
template <typename Type, typename Enabled = void>
struct IsReal;
template <typename Type, typename Enabled = void>
struct IsBoolean;
template <typename Type, typename Enabled = void>
struct IsCharacter;
template <typename Type, typename Enabled = void>
struct IsString;


template <typename Type>
struct IsInteger<
    Type,
    std::enable_if_t<
        std::is_integral<Type>::value    &&
        std::is_signed<Type>::value      &&
        !std::is_same<Type, bool>::value &&
        !std::is_same<Type, char>::value
    >
> {
    static constexpr bool value = true;
};



template <typename Type>
struct IsNatural<
    Type,
    std::enable_if_t<
        std::is_integral<Type>::value    &&
        std::is_unsigned<Type>::value    &&
        !std::is_same<Type, bool>::value &&
        !std::is_same<Type, char>::value
    >
> {
    static constexpr bool value = true;
};

template <typename Type>
struct IsReal<
    Type,
    std::enable_if_t<
        std::is_floating_point<Type>::value
    >
> {
    static constexpr bool value = true;
};

template <typename Type>
struct IsBoolean <
    Type,
    std::enable_if_t<
        std::is_same<Type, bool>::value
    >
> {
    static constexpr bool value = true;
};

template <typename Type>
struct IsCharacter <
    Type,
    std::enable_if_t<
        std::is_same<Type, char>::value
    >
> {
    static constexpr bool value = true;
};

template <typename Type>
struct IsString <
    Type,
    std::enable_if_t<
        std::is_same<Type, std::string>::value
    >
> {
    static constexpr bool value = true;
};



/**
 * @brief
 *  Functor decoding the string representation of an EgoScript value into a C++ value.
 * @author
 *  Michael Heilmann
 */
template <typename TargetType, typename Enabled = void>
struct Decoder {
    /**
     * @brief
     *  Convert a string representing an EgoScript value into a C++ value.
     * @param source
     *  the source
     * @param [out] target
     *  reference which is assigned the result if the operator succeeds and is not modified if it fails
     * @return
     *  @a true on success, @a false on failure
     * @param TargetType
     *  the target type
     * @param Enabled
     *  for SFINAE
     * @post
     *  If this function succeeded then @a target was assigned the result of the conversion,
     *  otherwise it was not modified.
     * @remark
     *  This operator doesnot raise exceptions.
     */
    bool operator()(const std::string& source, TargetType& target) const;

};

namespace Internal {

// Helper for to float and to double conversion.
template <typename T>
T to(const std::string& x, std::size_t *n);

template <>
inline float to<float>(const std::string& x, std::size_t* n) {
    return std::stof(x, n);
}

template <>
inline double to<double>(const std::string& x, std::size_t* n) {
    return std::stod(x, n);
}
}

/**
 * @brief
 *  Specialization for EgoScript character types.
 */
template <typename TargetType>
struct Decoder<TargetType, std::enable_if_t<IsCharacter<TargetType>::value>> {
	bool operator()(const std::string& source, TargetType& target) const {
		static_assert(IsCharacter<TargetType>::value,
			          "TargetType must be an EgoScript character type");
		try {
			if (source.length() != 1) {
				throw id::invalid_argument_error(__FILE__, __LINE__, "not a valid EgoScript character integral");
			}
			target = source[0];
		} catch (const id::invalid_argument_error&) {
			return false;
		}
		return true;
	}
};

/** 
 * @brief
 *  Specialization for EgoScript integer types.
 */
template <typename TargetType>
struct Decoder<TargetType, std::enable_if_t<IsInteger<TargetType>::value>> {
    bool operator()(const std::string& source, TargetType& target) const {
        static_assert(IsInteger<TargetType>::value,
                      "TargetType must be an EgoScript integer type");
        try {
            try {
                if (source.empty() || isspace(source[0])) {
                    throw id::invalid_argument_error(__FILE__, __LINE__, "not a valid EgoScript integer literal");
                }
                size_t pos;
                long long x;
                try {
                    x = stoll(source, &pos);
                } catch (const std::invalid_argument&) {
                    throw id::invalid_argument_error(__FILE__, __LINE__, "conversion failed");
                } catch (const std::out_of_range&) {
                    throw id::out_of_bounds_error(__FILE__, __LINE__, "conversion failed");
                }
                if (pos != source.length()) {
                    throw id::out_of_bounds_error(__FILE__, __LINE__, "not a valid EgoScript integer literal");
                }
                if (x > std::numeric_limits<TargetType>::max()) {
                    throw id::out_of_bounds_error(__FILE__, __LINE__, std::string("the value of the EgoScript integer literal is greater than the greatest value representable by the EgoScript integer type `") + typeid(TargetType).name() + "`");
                }
                if (x < std::numeric_limits<TargetType>::min()) {
                    throw id::out_of_bounds_error(__FILE__, __LINE__, std::string("the value of the EgoScript integer literal is smaller than the smallest value representable by the EgoScript integer type `") + typeid(TargetType).name() + "`");
                }
                target = x;
            } catch (const id::out_of_bounds_error&) {
                return false;
            } catch (const id::invalid_argument_error&) {
                return false;
            }
        } catch (...) {
            return false;
        }
        return true;
    }
};

/**
 * @brief
 *  Specialization for EgoScript natural types.
 */
template <typename TargetType>
struct Decoder<TargetType, std::enable_if_t<IsNatural<TargetType>::value>> {
    bool operator()(const std::string& source, TargetType& target) const {
        static_assert(IsNatural<TargetType>::value,
                      "TargetType must be an EgoScript natural type");
        try {
            try {
                if (source.empty() || isspace(source[0])) {
                    throw id::invalid_argument_error(__FILE__, __LINE__, "not a valid EgoScript natural literal");
                }
                size_t pos;
                unsigned long long x;
                try {
                    x = stoull(source, &pos);
                } catch (const std::invalid_argument&) {
                    throw id::invalid_argument_error(__FILE__, __LINE__, "conversion failed");
                } catch (const std::out_of_range&) {
                    throw id::out_of_bounds_error(__FILE__, __LINE__, "conversion failed");
                }
                if (pos != source.length()) {
                    throw id::invalid_argument_error(__FILE__, __LINE__, "not a valid EgoScript natural literal");
                }
                if (x > std::numeric_limits<TargetType>::max()) {
                    throw id::out_of_bounds_error(__FILE__, __LINE__, std::string("the value of the EgoScript natural literal is greater than the greatest value representable by the EgoScript natural type `") + typeid(TargetType).name() + "`");

                }
                target = x;
            } catch (const id::out_of_bounds_error&) {
                return false;
            } catch (const id::invalid_argument_error&) {
                return false;
            }
        } catch (...) {
            return false;
        }
        return true;
    }
};

/**
 * @brief
 *  Specialization for EgoScript boolean types.
 */
template <typename TargetType>
struct Decoder<TargetType, std::enable_if_t<IsBoolean<TargetType>::value>> {
    bool operator()(const std::string& source, TargetType& target) const {
        static_assert(IsBoolean<TargetType>::value,
                      "TargetType must be an EgoScript boolean type");
        if (source == "true") {
            target = true;
            return true;
        } else if (source == "false") {
            target = false;
            return true;
        } else {
            throw id::invalid_argument_error(__FILE__, __LINE__, "not a valid EgoScript boolean literal");
        }
    }
};

/**
 * @brief
 *  Specialization for EgoScript string types.
 */
template <typename TargetType>
struct Decoder<TargetType, std::enable_if_t<IsString<TargetType>::value>> {
    bool operator()(const std::string& source, TargetType& target) const {
        static_assert(IsString<TargetType>::value,
                      "TargetType must be an EgoScript string type");
        target = source;
        return true;
    }
};




/**
 * @brief
 *  Specialization for EgoScript real types.
 */
template <typename TargetType>
struct Decoder<TargetType, std::enable_if_t<IsReal<TargetType>::value>> {
    bool operator()(const std::string& source, TargetType& target) const {
        static_assert(std::is_same<TargetType, TargetType>::value,
                      "TargetType must be an EgoScript real type");
        try {
            try {
                if (source.empty() || isspace(source[0])) {
                    throw id::invalid_argument_error(__FILE__, __LINE__, "not a valid EgoScript real literal");
                }
                size_t pos;
                TargetType x;
                try {
                    x = Internal::to<TargetType>(source, &pos);
                } catch (const std::invalid_argument&) {
                    throw id::invalid_argument_error(__FILE__, __LINE__, "conversion failed");
                } catch (const std::out_of_range&) {
                    throw id::out_of_bounds_error(__FILE__, __LINE__, "conversion failed");
                }
                if (pos != source.length()) {
                    throw id::invalid_argument_error(__FILE__, __LINE__, "not a valid EgoScript real literal");
                }
                target = x;
            } catch (const id::out_of_bounds_error&) {
                return false;
            } catch (const id::invalid_argument_error&) {
                return false;
            }
        } catch (...) {
            return false;
        }
        return true;
    }
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief
 *  Functor encoding a C++ value into the string representing of an EgoScrip value.
 * @author
 *  Michael Heilmann
 */
template <typename SourceType, typename Enabled = void>
struct Encoder
{
    /**
     * @brief
     *  Convert a C++ value into string representing an EgoScript value.
     * @param source
     *  the source value
     * @param [out] target
     *  reference which is assigned the result if the operator succeeds and is not modified if it fails
     * @return
     *  @a true on success, @a false on failure
     * @param SourceType
     *  the source type
     * @param Enabled
     *  for SFINAE
     * @post
     *  If this function succeeded then @a target was assigned the result of the conversion,
     *  otherwise it was not modified.
     * @remark
     *  This operator doesnot raise exceptions.
     */
    bool operator()(SourceType& source, std::string& target);

};

/**
 * @brief
 *  Specialization for float and double.
 * @todo
 *  Should be mor terse and more efficient.
 */
template <typename SourceType>
struct Encoder<SourceType, std::enable_if_t<std::is_floating_point<SourceType>::value>>
{
    bool operator()(const SourceType& source, std::string& target)
    {
        static_assert(std::is_floating_point<SourceType>::value,
                      "SourceType must be an floating point type");
        try
        {
            target = std::to_string(source);
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
};

/**
 * @brief
 *  Specialization for signed integral types without the @a bool type.
 * @todo
 *  Should be mor terse and more efficient.
 */
template <typename SourceType>
struct Encoder<SourceType, std::enable_if_t<std::is_integral<SourceType>::value && !std::is_same<SourceType, bool>::value && std::is_signed<SourceType>::value>>
{
    bool operator()(const SourceType& source, std::string& target)
    {
        static_assert(std::is_integral<SourceType>::value && !std::is_same<SourceType, bool>::value && std::is_signed<SourceType>::value,
                      "SourceType must be an unsigned integral type and must not be bool");
        try
        {
            target = std::to_string(source);
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
};

/**
 * @brief
 *  Specialization for unsigned integral types without the @a bool, @a int8_t and @a uint8_t types.
 * @todo
 *  Should be mor terse and more efficient.
 */
template <typename SourceType>
struct Encoder<SourceType, std::enable_if_t<std::is_integral<SourceType>::value && !std::is_same<SourceType, int8_t>::value && !std::is_same<SourceType, uint8_t>::value && !std::is_same<SourceType, bool>::value && !std::is_signed<SourceType>::value>>
{

    bool operator()(const SourceType& source, std::string& target)
    {
        static_assert(std::is_integral<SourceType>::value && !std::is_same<SourceType, int8_t>::value && !std::is_same<SourceType, uint8_t>::value && !std::is_same<SourceType, bool>::value && !std::is_signed<SourceType>::value,
                      "SourceType must be an unsigned integral type and must not be bool, int8_t or uint8_t");
        try
        {
            target = std::to_string(source);
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
};

/**
 * @brief
 *  Specialization for std::string.
 */
template <typename SourceType>
struct Encoder<SourceType, std::enable_if_t<std::is_same<SourceType, std::string>::value>>
{
    bool operator()(const SourceType& source, std::string& target)
    {
        target = source;
        return true;
    }
};

/**
 * @brief
 *  Specialization for the @a int8_t type.
 */
template <typename SourceType>
struct Encoder<SourceType, std::enable_if_t<std::is_same<SourceType, int8_t>::value>>
{
    bool operator()(const SourceType& source, std::string& target)
    {
        static_assert(std::is_same<SourceType, int8_t>::value,
                      "SourceType must be int8_t");
        try
        {
            target = std::to_string((signed int)source);
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
};

/**
* @brief
*  Specialization for the @a uint8_t type.
*/
template <typename SourceType>
struct Encoder<SourceType, std::enable_if_t<std::is_same<SourceType, uint8_t>::value>>
{
    bool operator()(const SourceType& source, std::string& target)
    {
        static_assert(std::is_same<SourceType, uint8_t>::value,
                      "SourceType must be uint8_t");
        try
        {
            target = std::to_string((unsigned int)source);
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
};

/**
 * @brief
 *  Specialization for the @a bool type.
 */
template <typename SourceType>
struct Encoder<SourceType, std::enable_if_t<std::is_same<SourceType, bool>::value>>
{
    bool operator()(const SourceType& source, std::string& target)
    {
        target = source ? "true" : "false";
        return true;
    }
};

} // namespace Script
} // namespace Ego
