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
 *  Functor decoding the string representation of an EgoScript value into a C++ value.
 * @author
 *  Michael Heilmann
 */
template <typename TargetType, typename Enabled = void>
struct Decoder
{
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
    bool operator()(const string& source, TargetType& target);

};

/** 
 * @brief
 *  Specialization for signed integral types without the @a bool type.
 * @todo
 *  Should be mor terse and more efficient.
 */
template <typename TargetType>
struct Decoder<TargetType, typename enable_if<is_integral<TargetType>::value && !is_same<TargetType, bool>::value && is_signed<TargetType>::value >::type>
{
    bool operator()(const string& source, TargetType& target)
    {
        static_assert(is_integral<TargetType>::value && !is_same<TargetType, bool>::value && is_signed<TargetType>::value,
                      "TargetType must be an signed integral type and must not be bool");
        try
        {
            try
            {
                if (source.empty() || isspace(source[0]))
                {
                    throw invalid_argument("not a valid EgoScript signed integral");
                }
                size_t pos;
                long long x = stoll(source, &pos);
                if (pos != source.length())
                {
                    throw std::invalid_argument("not a valid EgoScript signed integral");
                }
                if (x > std::numeric_limits<TargetType>::max())
                {
                    throw out_of_range("EgoScript signed integral too big");
                }
                if (x < std::numeric_limits<TargetType>::min())
                {
                    throw out_of_range("EgoScript signed integral too small");
                }
                target = x;
            }
            catch (invalid_argument&)
            {
                return false;
            }
            catch (out_of_range&)
            {
                return false;
            }
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
 *  Specialization for unsigned integral types without the @a bool type.
 * @todo
 *  Should be mor terse and more efficient.
 */
template <typename TargetType>
struct Decoder<TargetType, typename enable_if<is_integral<TargetType>::value && !is_same<TargetType, bool>::value && !is_signed<TargetType>::value >::type>
{
public:
    bool operator()(const string& source, TargetType& target)
    {
        static_assert(is_integral<TargetType>::value && !is_same<TargetType, bool>::value && !is_signed<TargetType>::value,
                      "TargetType must be an unsigned integral type and must not be bool");
        try
        {
            try
            {
                if (source.empty() || isspace(source[0]))
                {
                    throw invalid_argument("not a valid EgoScript unsigned integral value");
                }
                size_t pos;
                unsigned long long x = stoull(source, &pos);
                if (pos != source.length())
                {
                    throw std::invalid_argument("not a valid EgoScript unsigned integral value");
                }
                if (x > std::numeric_limits<TargetType>::max())
                {
                    throw out_of_range("EgoScript unsigned integral too big value");
                }
                target = x;
            }
            catch (const invalid_argument&)
            {
                return false;
            }
            catch (const out_of_range&)
            {
                return false;
            }
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
 *  Specialization for @a bool.
 * @remark
 *  Accepts exactly
 *  @code
 *  true | false
 *  @endcode
 */
template <typename TargetType>
struct Decoder<TargetType, typename enable_if<is_same<TargetType, bool>::value>::type>
{
    bool operator()(const string& source, TargetType& target)
    {
        if (source == "true")
        {
            target = true;
            return true;
        }
        else if (source == "false")
        {
            target = false;
            return true;
        }
        else
        {
            return false;
        }
    }
};

// Specialization for @a std::string.
template <typename TargetType>
struct Decoder<TargetType, typename enable_if<is_same<TargetType, string>::value>::type>
{
    bool operator()(const string& source, TargetType& target)
    {
        target = source;
        return true;
    }
};


// Specialization for @a float.
template <typename TargetType>
struct Decoder<TargetType, typename enable_if<is_same<TargetType, float>::value>::type>
{
    bool operator()(const string& source, float& target)
    {
        static_assert(is_same<TargetType,float>::value,
                      "TargetType must be `float`");
        try
        {
            try
            {
                if (source.empty() || isspace(source[0]))
                {
                    throw invalid_argument("not a valid EgoScript `float` value");
                }
                size_t pos;
                double x = std::stof(source, &pos);
                if (pos != source.length())
                {
                    throw std::invalid_argument("not a valid EgoScript `float` value");
                }
                target = x;
            }
            catch (const invalid_argument&)
            {
                return false;
            }
            catch (const out_of_range&)
            {
                return false;
            }
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
};

// Specialization for @a double.
template <typename TargetType>
struct Decoder<TargetType, typename enable_if<is_same<TargetType, double>::value>::type>
{
    bool operator()(const string& source, double& target)
    {
        static_assert(is_same<TargetType, double>::value,
                      "TargetType must be `double`");
        try
        {
            try
            {
                if (source.empty() || isspace(source[0]))
                {
                    throw invalid_argument("not a valid EgoScript `double` value");
                }
                size_t pos;
                double x = std::stod(source, &pos);
                if (pos != source.length())
                {
                    throw std::invalid_argument("not a valid EgoScript `double` value");
                }
                target = x;
            }
            catch (const invalid_argument&)
            {
                return false;
            }
            catch (const out_of_range&)
            {
                return false;
            }
        }
        catch (...)
        {
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
    bool operator()(SourceType& source, string& target);

};

/**
 * @brief
 *  Specialization for signed integral types without the @a bool type.
 * @todo
 *  Should be mor terse and more efficient.
 */
template <typename SourceType>
struct Encoder<SourceType, typename enable_if<is_integral<SourceType>::value && !is_same<SourceType, bool>::value && is_signed<SourceType>::value>::type>
{
    bool operator()(const SourceType& source, string& target)
    {
        static_assert(is_integral<SourceType>::value && !is_same<SourceType, bool>::value && is_signed<SourceType>::value,
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
struct Encoder<SourceType, typename enable_if<is_integral<SourceType>::value && !is_same<SourceType, int8_t>::value && !is_same<SourceType, uint8_t>::value && !is_same<SourceType, bool>::value && !is_signed<SourceType>::value >::type>
{

    bool operator()(const SourceType& source, string& target)
    {
        static_assert(is_integral<SourceType>::value && !is_same<SourceType, int8_t>::value &&!is_same<SourceType, uint8_t>::value && !is_same<SourceType, bool>::value && !is_signed<SourceType>::value,
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
struct Encoder<SourceType, typename enable_if<is_same<SourceType, string>::value>::type>
{
    bool operator()(const SourceType& source, string& target)
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
struct Encoder<SourceType, typename enable_if<is_same<SourceType, int8_t>::value>::type>
{
    bool operator()(const SourceType& source, string& target)
    {
        static_assert(is_same<SourceType, int8_t>::value,
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
struct Encoder<SourceType, typename enable_if<is_same<SourceType, uint8_t>::value>::type>
{
    bool operator()(const SourceType& source, string& target)
    {
        static_assert(is_same<SourceType, uint8_t>::value,
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
struct Encoder<SourceType, typename enable_if<is_same<SourceType, bool>::value>::type>
{
    bool operator()(const SourceType& source, string& target)
    {
        target = source ? "true" : "false";
        return true;
    }
};

} // namespace Script
} // namespace Ego
