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

/// @file   egolib/Math/Random.hpp
/// @brief  Utility helper to generate random numbers
/// @author Johan Jansen

#pragma once

#include "egolib/platform.h"
#include "egolib/typedef.h"

class Random
{
public:
	/**
	 * @brief
     *  Generate a random floating point number in the interval <tt>[0,1]</tt>.
     * @return
     *  a random floating point number in the interval <tt>[0,1]</tt>
	 */
    static float nextFloat();

    /**
    * @brief
    *   Generates a float from a FRange type
    * @param range
    *   Specifies the lower and upper range to generate the number from
    * @return
    *   A floating point value between range.from and range.to (inclusive)
    **/
    static float next(const FRange &range)
    {
        std::uniform_real_distribution<float> rand(range.from, range.to + std::numeric_limits<float>::epsilon());
        return rand(generator);
    }
    
    /**
	 * @brief
     *  Generate an integer number in the interval <tt>[0,high]</tt>.
     * @param high
     *  the upper bound (inclusive) for the random integer number generated
     * @return
     *  a random integer number in the interval <tt>[0,high]</tt>
     * @pre
     *  <tt>high >= 0</tt>
	 */
    template<typename T>
    static T next(const T high)
    {
        return next<T>(0, high);
    }

    /**
	 * @brief
     *  Generates an integer number in the interval <tt>[low,high]</tt>.
     * @param low
     *  the lower bound (inclusive) for the integer number returned by this function
     * @param high
     *  the upper bound (inclusive) for the integer number returned by this function
     * @return
     *  a random integer number in the interval <tt>[low,high]</tt>
     * @pre
     *  <tt>low <= high</tt>
	 */
    template<typename T>
    static T next(const T low, const T high)
    {
        static_assert(std::is_same<T, short>::value || std::is_same<T, int>::value ||
                      std::is_same<T, long>::value || std::is_same<T, long long>::value ||
                      std::is_same<T, unsigned short>::value || std::is_same<T, unsigned int>::value  ||
                      std::is_same<T, unsigned long>::value || std::is_same<T, unsigned long long>::value,
                      "T must be one of short, int, long, long long, unsigned short, "
                      "unsigned int, unsigned long, or unsigned long long");
        if (low > high)
        {
            throw std::invalid_argument("low > high");
        }
        if (low == high)
        {
            return low;
        }
        std::uniform_int_distribution<T> rand(low, high);
        return rand(generator);
    }

	/**
	 * @brief
     *  Randomly returns @a true or @a false.
     * @return
     *  @a true or @a false
	 */
    static bool nextBool();

	/**
	 * @brief
     *  Generates a random integer number in the interval <tt>[0,1]</tt>.
     * @return
     *  a random integer number in the interval <tt>[0,1]</tt>
	 */
    static int getPercent();

    /**
     * @brief
     *  Sets the random seed used for randomization.
     * @param seed
     *  the seed
     */
    static void setSeed(const long seed);

    /**
     * @brief
     *  Returns a reference to a random element in a vector.
     * @param container
     *  the container
     * @return
     *  a reference to a random element in the container
     */
    template<typename T>
    static const T& getRandomElement(const std::vector<T>& container)
    {
        assert(!container.empty());
        return container[ Random::next<size_t>(container.size()-1) ];
    }

    /**
     * @brief
     *  Returns a reference to a random element in this vector
     */
    template<typename T>
    static T& getRandomElement(std::vector<T> &container)
    {
        assert(!container.empty());
        return container[ Random::next<size_t>(container.size()-1) ];
    }

private:

    /**
     * @brief
     *  Mersenne Twister randomizer.
     */
    static std::mt19937 generator;

};
