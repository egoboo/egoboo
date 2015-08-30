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

LocalTime::LocalTimeType LocalTime::convert(const LocalTime::SystemTimePoint& source) {
	return convert(SystemClockType::to_time_t(source));
}

LocalTime::LocalTimeType LocalTime::convert(const std::time_t& source) {
	LocalTimeType target;
#if defined(_MSC_VER)
	if (0 != localtime_s(&target, &source)) {
		throw Id::EnvironmentErrorException(__FILE__, __LINE__, "Ego::Time", "localtime_s failed");
	}
#else
	if (NULL == localtime_r(&source, &target)) {
		throw Id::EnvironmentErrorException(__FILE__, __LINE__, "Ego::Time", "localtime_r failed");
	}
#endif
	return target;
}

LocalTime::LocalTime(const LocalTime& other)
	: _localTime(other._localTime) {
}

LocalTime& LocalTime::operator=(const LocalTime& other) {
	return *this;
}

bool LocalTime::operator == (const LocalTime& other) const {
	// There is no need to compare all members.
	// A point in time is identified by the second, minute, hour, day of year and year.

	return
		// Same second, minute and hour?
		   _localTime.tm_sec == other._localTime.tm_sec
		&& _localTime.tm_min == other._localTime.tm_min
		&& _localTime.tm_hour == other._localTime.tm_hour
		// Same day of year and same year?
		&& _localTime.tm_yday == other._localTime.tm_yday
		&& _localTime.tm_year == other._localTime.tm_year
		;

	return false;
}

bool LocalTime::operator != (const LocalTime& other) const {
	// There is no need to compare all members.
	// A point in time is identified by the second, minute, hour, day of year and year.

	return 
		// Same second, minute and hour?
		   _localTime.tm_sec != other._localTime.tm_sec
		|| _localTime.tm_min != other._localTime.tm_min
		|| _localTime.tm_hour != other._localTime.tm_hour
		// Same day of year and same year?
		|| _localTime.tm_yday != other._localTime.tm_yday
		|| _localTime.tm_year != other._localTime.tm_year
		;
}

static inline int compare(const std::tm& x, const std::tm& y) {
	if (x.tm_year < y.tm_year) return -1;
	else if (x.tm_year > y.tm_year) return +1;
	// x is equivalent to y w.r.t. to the years since 1900.
	if (x.tm_mon < y.tm_mon) return -1;
	else if (x.tm_mon > y.tm_mon) return +1;
	// x is equivalent to y w.r.t. to the months since January.
	if (x.tm_mday < y.tm_mday) return -1;
	else if (x.tm_mday > y.tm_mday) return +1;
	// x is equivalent to y w.r.t. to the day of the month.
	if (x.tm_hour < y.tm_hour) return -1;
	else if (x.tm_hour > y.tm_hour) return +1;
	// x is equivalent to y w.r.t. to the hours since midnight.
	if (x.tm_min < y.tm_min) return -1;
	else if (x.tm_min > y.tm_min) return +1;
	// x is equivalent o y w.r.t. to the minutes after the hour.
	if (x.tm_sec < y.tm_sec) return -1;
	else if (x.tm_sec > y.tm_sec) return +1;
	// x is equivalent to y w.r.t. to the seconds after the minute.
	// x is equivalent to y.
	return 0;
}

bool LocalTime::operator < (const LocalTime& other) const {
	return compare(_localTime, other._localTime) < 0;
}

bool LocalTime::operator > (const LocalTime& other) const {
	return compare(_localTime, other._localTime) > 0;
}

bool LocalTime::operator <= (const LocalTime& other) const {
	return compare(_localTime, other._localTime) <= 0;
}

bool LocalTime::operator >= (const LocalTime& other) const {
	return compare(_localTime, other._localTime) >= 0;
}

} // namespace Time
} // namespace Ego
