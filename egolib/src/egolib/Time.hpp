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

/// @file egolib/Time.hpp
/// @brief Time points, time durations, and time units.
/// @author Michael Heilmann
/// @todo Move into <tt>egolib/Time</tt>.

#pragma once

#include "egolib/typedef.h"
#include "egolib/Time/Unit.hpp"

/// SDL_GetTicks() always returns milli-seconds
/// @todo Remove this.
#define TICKS_PER_SEC 1000.0f

namespace Time {

/// (Time) UnitTraits provide information provide information about a (time) unit, in particular, the data type
/// used to represent time points <tt>UnitTraits<U>::Type</tt> for any time unit <tt>U</tt>. 
/// @remarks Specializations for all enumeration elements of the Time::Unit enumeration are provided.
template<Unit UnitType> struct UnitTraits;

/// Get the current time (aka "now") in the specified time unit.
/// @return the current time (aka "now") in the specified time unit.
/// @remarks Specializations for all enumeration elements of the Time::Unit enumeration are provided.
template <Unit UnitType>
typename UnitTraits<UnitType>::Type now();

/**
 * @brief Convert time points/time durations between units.
 * @tparam Target the target time unit
 * @tparam Source the source time unit
 * @param source the time point/time duration in the @a Source time unit
 * @return the time point/time duration in the @a Target time unit
 * @remark Specializations for all combinations of enumeration elements of the Time::Unit enumeration are provided.
 */
template <Unit Target, Unit Source>
typename UnitTraits<Target>::Type to(typename UnitTraits<Source>::Type source);

} // namespace Time


// Specializations

namespace Time {

// Template specialization of UnitTraits for Unit::Ticks.
template<> struct UnitTraits<Unit::Ticks> { typedef uint32_t Type; };

// Template specialization of UnitTraits for Unit::Milliseconds.
template<> struct UnitTraits<Unit::Milliseconds> { typedef uint32_t Type; };

// Template specialization of UnitTraits for Unit::Seconds.
template<> struct UnitTraits<Unit::Seconds> { typedef uint32_t Type; };

} // namespace Time



namespace Time {

template <>
typename UnitTraits<Unit::Ticks>::Type now<Unit::Ticks>();

template <>
typename UnitTraits<Unit::Milliseconds>::Type now<Unit::Milliseconds>();

template <>
typename UnitTraits<Unit::Seconds>::Type now<Unit::Seconds>();

} // namespace Time



namespace Time {

// Convert from ticks to ticks (identity).
template <>
typename UnitTraits<Unit::Ticks>::Type to<Unit::Ticks, Unit::Ticks>(typename UnitTraits<Unit::Ticks>::Type source);

// Convert from milliseconds to milliseconds (identity).
template <>
typename UnitTraits<Unit::Milliseconds>::Type to<Unit::Milliseconds, Unit::Milliseconds>(typename UnitTraits<Unit::Milliseconds>::Type source);

// Convert from seconds to seconds (identity).
template <>
typename UnitTraits<Unit::Seconds>::Type to<Unit::Seconds, Unit::Seconds>(typename UnitTraits<Unit::Seconds>::Type source);

} // namespace Time



namespace Time {

// Convert from ticks to milliseconds.
template <>
typename UnitTraits<Unit::Milliseconds>::Type to<Unit::Milliseconds, Unit::Ticks>(typename UnitTraits<Unit::Ticks>::Type source);

// Convert from milliseconds to ticks.
template <>
typename UnitTraits<Unit::Ticks>::Type to<Unit::Ticks, Unit::Milliseconds>(typename UnitTraits<Unit::Milliseconds>::Type source);

} // namespace Time



namespace Time {

// Convert from milliseconds to seconds.
template <>
typename UnitTraits<Unit::Seconds>::Type to<Unit::Seconds, Unit::Milliseconds>(typename UnitTraits<Unit::Milliseconds>::Type source);

// Convert from seconds to milliseconds.
template <>
typename UnitTraits<Unit::Milliseconds>::Type to<Unit::Milliseconds, Unit::Seconds>(typename UnitTraits<Unit::Seconds>::Type source);

} // namespace Time



namespace Time {

// For convenience, define a "Time::Ticks" and a "Time::Seconds" alias.
typedef UnitTraits<Unit::Ticks>::Type Ticks;
typedef UnitTraits<Unit::Seconds>::Type Seconds;

} // namespace Time
