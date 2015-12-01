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

/// @file  egolib/Time.cpp
/// @brief Time points, time durations, and time units.
/// @author Michael Heilmann
/// @todo Move into <tt>egolib/Time</tt>.

#include "egolib/Time.hpp"
#include "egolib/Core/System.hpp"



namespace Time {

template <>
typename UnitTraits<Unit::Ticks>::Type now<Unit::Ticks>() {
    return Ego::Core::System::get().getTimerService().getTicks();
}

template <>
typename UnitTraits<Unit::Milliseconds>::Type now<Unit::Milliseconds>() {
    return now<Unit::Ticks>();
}

template <>
typename UnitTraits<Unit::Seconds>::Type now<Unit::Seconds>() {
    return now<Unit::Milliseconds>() / 1000;
}

} // namespace Time



namespace Time {

// Convert from ticks to ticks (identity).
template <>
typename UnitTraits<Unit::Ticks>::Type to<Unit::Ticks, Unit::Ticks>(typename UnitTraits<Unit::Ticks>::Type source) {
    return source;
}

// Convert from milliseconds to milliseconds (identity).
template <>
typename UnitTraits<Unit::Milliseconds>::Type to<Unit::Milliseconds, Unit::Milliseconds>(typename UnitTraits<Unit::Milliseconds>::Type source) {
    return source;
}

// Convert from seconds to seconds (identity).
template <>
typename UnitTraits<Unit::Seconds>::Type to<Unit::Seconds, Unit::Seconds>(typename UnitTraits<Unit::Seconds>::Type source) {
    return source;
}

} // namespace Time



namespace Time {

// Convert from ticks to milliseconds.
template <>
typename UnitTraits<Unit::Milliseconds>::Type to<Unit::Milliseconds, Unit::Ticks>(typename UnitTraits<Unit::Ticks>::Type source) {
    return source;
}

// Convert from milliseconds to ticks.
template <>
typename UnitTraits<Unit::Ticks>::Type to<Unit::Ticks, Unit::Milliseconds>(typename UnitTraits<Unit::Milliseconds>::Type source) {
    return source;
}

} // namespace Time



namespace Time {

// Convert from milliseconds to seconds.
template <>
typename UnitTraits<Unit::Seconds>::Type to<Unit::Seconds, Unit::Milliseconds>(typename UnitTraits<Unit::Milliseconds>::Type source) {
    return source / 1000;
}

// Convert from seconds to milliseconds.
template <>
typename UnitTraits<Unit::Milliseconds>::Type to<Unit::Milliseconds, Unit::Seconds>(typename UnitTraits<Unit::Seconds>::Type source) {
    return source * 1000;
}

} // namespace Time
