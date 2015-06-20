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
 *  Reprenation of "local" (aka "calendar") time.
 *  Esentially wrapper around std::localtime avoid calls to std::localtime
 *  whenever possible and thread-safe w.r.t. its own calls to std::localtime.
 *  However, if other code uses std::localtime, tht thread-safety is not
 *  guaranteed anymore.
 * @remark
 *   Unlike using localtime() and friends, using this class is thread-safe if no one else calls localtime() and friends.
 * @see
 *    http://en.cppreference.com/w/cpp/chrono/c/tm
 * @author
 *  Michael Heilmann
 * @todo
 *  Establish full thread-safety by not using std::localtime.
 */
struct LocalTime {
private:
    std::tm _timeinfo;
public:
    /**
     * @brief
     *  Construct this local time object with the current local time.
     */
    LocalTime();
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
    LocalTime& operator=(const LocalTime& other);
    /**
     * @brief
     *  The day of the month,
     *  an integer within the interval of [1,31].
     * @return
     *  the day of the month
     */
    int getDayOfMonth() const {
        return _timeinfo.tm_mday;
    }
    /**
     * @brief
     *  Days since Sunday,
     *  an integer within the interval of [0,6].
     * @return
     *  days since Sunday
     */
    int getDayOfWeek() const {
        return _timeinfo.tm_wday;
    }
    /**
     * @brief
     *  Hours since midnight,
     *  an integer within the interval of [0,23].
     * @return
     *  hours since midnight
     */
    int getHours() const {
        return _timeinfo.tm_hour;
    }
    /**
     * @brief
     *  Minutes after the hour,
     *  an integer within the interval of [0,59].
     * @return
     *  minutes after the hour
     */
    int getMinutes() const {
        return _timeinfo.tm_min;
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
        return _timeinfo.tm_sec;
    }
    /**
     * @brief
     *  Months since January,
     *  an integer within the interval odf [0,11].
     * @return
     *  months since January
     */
    int getMonth() const {
        return _timeinfo.tm_mon;
    }
    /**
     * @brief
     *  Years since 1900.
     * @return
     *  the years since 1900.
     */
    int getYear() const {
        return _timeinfo.tm_year;
    }
    /**
     * @brief
     *  Days since January 1st,
     *  in integer within the interval of [0,365].
     * @return
     *  days since January 1st
     */
    int getDayOfYear() const {
        return _timeinfo.tm_yday;
    }
};
} // Time
} // Ego
