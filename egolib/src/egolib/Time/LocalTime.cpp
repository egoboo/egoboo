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

#include "egolib/Time/LocalTime.hpp"

/// @file   egolib/Time/LocalTime.hpp
/// @brief  Local (aka Calendar) Time functionality
/// @author Michael Heilmann

namespace Ego {
namespace Time {

LocalTime::LocalTime() {
	static std::mutex mutex;
	std::lock_guard<std::mutex> lock(mutex);
	std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm *timeinfo = std::localtime(&time);
	if (!timeinfo) {
		throw std::runtime_error("unable to get local time");
	}
	_timeinfo = *timeinfo;
}
LocalTime::LocalTime(const LocalTime& other)
	: _timeinfo(other._timeinfo) {
}
LocalTime& LocalTime::operator=(const LocalTime& other) {
	return *this;
}

} // namespace Time
} // namespace Ego
