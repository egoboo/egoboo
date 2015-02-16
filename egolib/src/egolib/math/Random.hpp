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
/// @file  egolib/math/Random.hpp
/// @brief Utility helper to generate random numbers
/// @author Johan Jansen

#pragma once

#include "egolib/platform.h"

class Random
{
public:
	/**
	* @brief Returns random floating point number between 0.0f and 1.0f
	**/
    static float nextFloat();
    
    /**
	* @brief Generates an integer between 0 and high (inclusive)
	**/
    template<typename T>
    static T next(const T high)
    {
        return next<T>(0, high);
    }

    /**
	* @brief Generates an integer number between 0 and high (inclusive)
	**/
    template<typename T>
    static T next(const T low, const T high)
    {
        assert(low <= high);
        if(low >= high) return low;
        std::uniform_int_distribution<T> rand(low, high);
        return rand(generator);
    }

	/**
	* @brief Randomly returns true or false
	**/
    static bool nextBool();

	/**
	* @brief Generates a number from 1 to 100
	**/
    static int getPercent();

    /**
    * @brief Sets the random seed used for randomization
    **/
    static void setSeed(const long seed);

    /**
    * @brief Returns a reference to a random element in this vector
    **/
    template<typename T>
    static const T& getRandomElement(const std::vector<T> &container)
    {
        assert(!container.empty());
        return container[ Random::next<size_t>(container.size()-1) ];
    }

    /**
    * @brief Returns a reference to a random element in this vector
    **/
    template<typename T>
    static T& getRandomElement(std::vector<T> &container)
    {
        assert(!container.empty());
        return container[ Random::next<size_t>(container.size()-1) ];
    }

private:
    //Mersenne Twister randomizer
    static std::mt19937 generator;
};
