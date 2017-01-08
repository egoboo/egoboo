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

/// @file egolib/egolib_typedef.c
/// @brief Implementation of the support functions for Egoboo's special datatypes
/// @details

#include "egolib/typedef.h"

#include "egolib/Log/_Include.hpp"
#include "egolib/_math.h"

Ego::Math::Interval<float> pair_to_range(const IPair& source) {
    /// @author ZZ
    /// @details convert from a pair to a range

    if (source.base < 0) {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "randomization error - base ",
                                         source.base, " is less than 0", Log::EndOfEntry);
    }

    if (source.rand < 0) {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "randomization error - rand ",
                                         source.rand, " is less than 0", Log::EndOfEntry);
    }

    float a = FP8_TO_FLOAT(source.base);
    float b = FP8_TO_FLOAT(source.base + source.rand);
    float l = std::min(a, b);
    float u = std::max(a, b);
    return Ego::Math::Interval<float>(l, u);
}

IPair range_to_pair(const Ego::Math::Interval<float>& source) {
    float a = source.getLowerbound();
    float b = source.getUpperbound() - source.getLowerbound();
    return IPair(FLOAT_TO_FP8(a), FLOAT_TO_FP8(b));
}

Log::Entry& operator<<(Log::Entry& entry, const ObjectRef& ref)
{
    entry << ref.get();
    return entry;
}

Log::Entry& operator<<(Log::Entry& entry, const ObjectProfileRef& ref)
{
    entry << ref.get();
    return entry;
}


Log::Entry& operator<<(Log::Entry& entry, const ParticleProfileRef& ref)
{
    entry << ref.get();
    return entry;
}

Log::Entry& operator<<(Log::Entry& entry, const ParticleRef& ref)
{
    entry << ref.get();
    return entry;
}

