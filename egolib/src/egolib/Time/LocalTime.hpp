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

/// @file   egolib/Time/LocalTime.hpp
/// @brief  Local (aka Calendar) Time functionality
/// @author Michael Heilmann

#pragma once

#include "egolib/typedef.h"

namespace Ego {
namespace Time {
/**
 * @brief
 *	"local" (aka "calendar") time class . Essentially, this class
 *	provides the same functionality as std::localtime/std::tm, but
 *	is multi-threading safe.
 * @author
 *  Michael Heilmann
 */
struct LocalTime {
    using SystemClockType = std::chrono::system_clock;
    using SystemTimePoint = std::chrono::system_clock::time_point;
	using LocalTimeType = std::tm;
private:
	LocalTimeType _localTime;
	/// convert std::chrono::system_clock::time_point object (system clock time) into an std::tm object (local time).
	static LocalTimeType convert(const SystemTimePoint& source);
	/// Convert an std::time object (system clock time) into an std::tm time object (local time).
	static LocalTimeType convert(const std::time_t& source);
public:
    /**
     * @brief
     *  Construct this local time object.
	 * @param src
	 *	a system clock time point
     */
	LocalTime(const SystemTimePoint& other = SystemClockType::now())
		: _localTime(convert(other)) {
	}
    /**
     * @brief
     *  Construct this local time object with the local time of another local time object.
     * @param other
     *  the other local time object
     */
	LocalTime(const LocalTime& other);
    
	/**
     * @brief
     *  Assign this local time object with the local time of another local time object.
     * @param other
     *  the other local time object
     * @return
     *  this local time object
     */
    // As always, return non-const reference in order to allow chaining for the sake of orthogonality.
    LocalTime& operator=(const LocalTime& other);

	/**
	 * @brief
	 *	Get if this local time is equal to another local time.
	 * @param other
	 *	the other local time
	 * @return
	 *	@a true if this local time is equal to the other local time
	 */
	bool operator==(const LocalTime& other) const;

	/**
	 * @brief
	 *	Get if this local time is not equal to another local time.
	 * @param other
	 *	the other local time
	 * @return
	 *	@a true if this local time is equal to the other local time
	 */
	bool operator!=(const LocalTime& other) const;

	/**
	 * @brief
	 *	Get if this local time is smaller than another local time.
	 * @param other
	 *	the other local time
	 * @return
	 *	@a true if this local time is smaller than the other local time
	 */
	bool operator < (const LocalTime& other) const;

	/**
	 * @brief
	 *	Get if this local time is greater than another local time.
	 * @param other
	 *	the other local time
	 * @return
	 *	@a true if this local time is greater than the other local time
	 */
	bool operator > (const LocalTime& other) const;

	/**
	 * @brief
	 *	Get if this local time is smaller than or equal to another local time.
	 * @param other
	 *	the other local time
	 * @return
	 *	@a true if this local time is smaller than or equal to the other local time
	*/
	bool operator <= (const LocalTime& other) const;

	/**
	 * @brief
	 *	Get if this local time is greater than than or equal to another local time.
	 * @param other
	 *	the other local time
	 * @return
	 *	@a true if this local time is greater than or equal to the other local time
	 */
	bool operator >= (const LocalTime& other) const;

    /**
     * @brief
     *  The day of the month,
     *  an integer within the interval of [1,31].
     * @return
     *  the day of the month
     */
    int getDayOfMonth() const {
		return _localTime.tm_mday;
    }
    /**
     * @brief
     *  Days since Sunday,
     *  an integer within the interval of [0,6].
     * @return
     *  days since Sunday
     */
    int getDayOfWeek() const {
		return _localTime.tm_wday;
    }
    /**
     * @brief
     *  Hours since midnight,
     *  an integer within the interval of [0,23].
     * @return
     *  hours since midnight
     */
    int getHours() const {
		return _localTime.tm_hour;
    }
    /**
     * @brief
     *  Minutes after the hour,
     *  an integer within the interval of [0,59].
     * @return
     *  minutes after the hour
     */
    int getMinutes() const {
		return _localTime.tm_min;
    }
    /**
     * @brief
     *  Seconds after the minute,
     *  an integer within the interval of [0,59]/[0,60].
     *  The extra range is used to indicate a leap second.
     * @return
     *  seconds after the minue
     */
    int getSeconds() const {
		return _localTime.tm_sec;
    }
    /**
     * @brief
     *  Months since January,
     *  an integer within the interval odf [0,11].
     * @return
     *  months since January
     */
    int getMonth() const {
		return _localTime.tm_mon;
    }
    /**
     * @brief
     *  Years since 1900.
     * @return
     *  the years since 1900.
     */
    int getYear() const {
		return _localTime.tm_year;
    }
    /**
     * @brief
     *  Days since January 1st,
     *  in integer within the interval of [0,365].
     * @return
     *  days since January 1st
     */
    int getDayOfYear() const {
		return _localTime.tm_yday;
    }

};

} // Time
} // Ego
