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

#include "egolib/Math/Random.hpp"

// Static data initializer
std::random_device rd;
std::mt19937 Random::generator = std::mt19937(time(nullptr));

void Random::setSeed(const long seed)
{
    generator.seed(seed);
}

float Random::nextFloat()
{
    static std::uniform_real_distribution<float> rand(0.0f, std::nextafter(1.0f, std::numeric_limits<float>::max()));
    return rand(generator);
}

int Random::getPercent()
{
	static std::uniform_int_distribution<int> rand(1, 100);
    return rand(generator);
}

bool Random::nextBool()
{
    std::bernoulli_distribution rand(0.5);
    return rand(generator);
}
