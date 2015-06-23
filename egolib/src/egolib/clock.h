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

/// @file    egolib/clock.h
/// @brief   clock & timer functionality
/// @details This implementation was adapted from Noel Lopis' article in
///          Game Programming Gems 4.

#pragma once

#include "egolib/Time/LocalTime.hpp"
#include "egolib/Time/Stopwatch.hpp"
#include "egolib/Time/SlidingWindow.hpp"

/// The description of a single clock
struct ClockState_t
{
	// Clock data
	std::string _name;

	Ego::Time::Stopwatch _stopwatch;

	/**
	 * @brief
	 *	Regardless of how much time elapsed between the start and the end,
	 *	the clock assumes that at most this time elapsed between the start and the end.
	 *	The default is 0.2 seconds.
	 */
	double _maxElapsedTime;

	// Circular buffer to hold frame histories
	Ego::Time::SlidingWindow<double> _slidingWindow;

public:
	/**
	 * @brief
	 *	Construct a clock state.
	 * @param name
	 *	the name of the clock
	 * @param slidingWindowCapacity
	 *	the capacity of the sliding window
	 * @throw std::invalid_argument
	 *	if the sliding window capacity is not positive
	 * @post
	 *	The clock is in its initial state w.r.t. the current point in time.
	 */
	ClockState_t(const std::string& name, size_t slidingWindowCapacity);
public:

	/**
	 * @brief
	 *	Create a clock.
	 * @param name
	 *	the name of the clock
	 * @param slidingWindowCapacity
	 *	the capacity of the sliding window
	 * @return
	 *	a pointer to the clock on success, a null pointer on failure
	 */
	static ClockState_t *create(const std::string& name, size_t slidingWindowCapacity);

	/**
	 * @brief
	 *	Destroy this clock.
	 * @param self
	 *	this clock
	 */
	static void destroy(ClockState_t *self);

	/**
	 * @brief
	 *	Enter the observed section.
	 */
	void enter();
	/**
	 * @brief
	 *	Leave the observed section.
	 */
	void leave();
	/**
	 * @brief
	 *	Put the clock into its initial state w.r.t. the current point in time.
	 */
	void reinit();

public:

	/**
	 * @brief
	 *	Get the average frame duration.
	 * @return
	 *	the average frame duration
	 * @remark
	 *	If no frame duration was measured, @a 0 is returned.
	 */
	double avg() const;

	/**
	 * @brief
	 *	Get the last frame duration.
	 * @return
	 *	the last frame duration
	 * @remark
	 *	If no frame duration was measured, @a 0 is returned.
	 */
	double lst() const;
};

struct ClockScope : public Id::NonCopyable {
private:
	ClockState_t& _clock;
public:
	ClockScope(ClockState_t& clock) :
		_clock(clock) {
		_clock.enter();
	}
	~ClockScope() {
		_clock.leave();
	}
};
