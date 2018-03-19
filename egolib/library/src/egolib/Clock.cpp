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

/// @file egolib/Clock.cpp
/// @brief Clock & timer implementation
/// @details This implementation was adapted from Noel Lopis' article in Game Programming Gems 4.

#include "egolib/Clock.hpp"

namespace Ego::Time {

Clock<ClockPolicy::NonRecursive>::Clock(const std::string& name, size_t slidingWindowCapacity)
	: Internal::AbstractClock<ClockPolicy::NonRecursive>(name, slidingWindowCapacity) {
}

Clock<ClockPolicy::Recursive>::Clock(const std::string& name, size_t slidingWindowCapacity)
	: Internal::AbstractClock<ClockPolicy::Recursive>(name, slidingWindowCapacity), _balance(0) {
}

void Clock<ClockPolicy::Recursive>::enter() {
	if (0 == _balance++) {
		this->Internal::AbstractClock<ClockPolicy::Recursive>::enter();
	}
}

void Clock<ClockPolicy::Recursive>::leave() {
	if (--_balance == 0) {
		this->Internal::AbstractClock<ClockPolicy::Recursive>::leave();
	}
}

} // namespace Ego::Time
