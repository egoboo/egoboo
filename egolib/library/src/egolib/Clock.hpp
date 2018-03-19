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

/// @file    egolib/Clock.hpp
/// @brief   clock & timer functionality
/// @author  Michael Heilmann

#pragma once

#include "egolib/Time/LocalTime.hpp"
#include "egolib/Time/SlidingWindow.hpp"
#include "idlib/chrono.hpp"
#include <string>

namespace Ego::Time {

/** 
 * @brief
 *	The policy of a clock decides over its behavior when being faced with recursive starts and stops.
 */
struct ClockPolicy {
	/** 
	 * @brief
	 *	The clock is stopped by any call to Clock::leave().
	 */
	struct NonRecursive{ };
	/**
	 * @brief
	 *	The clock is stopped if the balance of calls to Clock::enter() and Clock::leave() reaches zero.
	 * @todo
	 *	Add implementation.
	 */
	struct Recursive { };
};

/**
 * @brief
 *	A clock. Specializations for ClockPoliciy::NonRecursive and ClockPolicy::Recursive are provided.
 * @remark
 *	A clock - usually in conjunction with a clock scope - is used to measure the amount of time spent
 *	in a particular section of the source code. For instance, to measure the amount of time spent on 
 *	on some fragment
 *	@code
 *	foo();
 *	@endcode
 *	one might declare a global variable
 *	@code
 *	Clock<ClockPolicy::NonRecursive> fooClock("fooClock");
 *	@endcode
 *	and modify the above lines as
 *	@code
 *	fooClock.enter();
 *	foo();
 *	fooClock.leave();
 *	@endcode
 * @remark
 *	A problem occurs if <tt>foo</tt> might raise an exception such that <tt>fooClock.leave</tt> is never
 *	called. To make up for C++ exceptions, ClockScope is provided, which starts the clock upon its creation
 *	and stops the clock upon its destruction.
 *	@code
 *	{
 *	ClockScope<ClockPolicy::NonRecursive> fooScope(fooBlock);
 *	foo();
 *	}
 *	@endcode
 * @remark
 *	Consider the following fragment
 *	@code
 *	Clock<ClockPolicy::NonRecursive> fibClock("fibClock");
 *	int fib(int i) {
 *		ClockScope<ClockPolicy::NonRecursive> scope(fibClock);
 *		if (i <= 1) {
 *			return i;
 *		}
 *		return fib(i-1) + fib(i-2);
 *	}
 *	@endcode
 *	with a global clock declared as
 *	@code
 *	Clock<ClockPolicy::NonRecursive> fibClock;
 *	@endcode
 *	which attempts to measure the actual time spent in the <tt>fib</tt> function.
 *	This function is recursive and the time spent on returning from the recursion
 *	is not measured as the first time <tt>scope</tt> goes out of scope in some
 *	recursive call, the clock is stopped.
 *	To make up for that, the clock policy ClockPolicy::Recursive is provided which
 *	stops the clock only if the number of times the clock is started and stopped
 *	decrements to zero. To properly measure the time spent in the <tt>fib</tt>
 *	function, modify the above fragment to
 *	@code
 *	Clock<ClockPolicy::Recursive> fibClock("fibClock");
 *	fib(int i) {
 *		ClockScope<ClockPolicy::Recursive> scope(fibClock);
 *		if (i <= 1) {
 *			return i;
 *		}
 *		return fib(i-1) + fib(i-2);
 *	}
 *	@endcode
 */
template <typename _ClockPolicy>
struct Clock;

namespace Internal {

/**
 * @internal
 */
template <typename _ClockPolicy>
struct AbstractClock {
protected:

	/**
	 * @brief
	 *	The name of the clock.
	 */
	std::string _name;

	/**
	 * @brief
	 *	A sliding window holding the a finite, consecutive subset of the measured durations.
	 */
	SlidingWindow<double> _slidingWindow;
	/**
	 * @brief
	 *	The stopwatch backing this clock.
	 */
	idlib::stopwatch _stopwatch;

protected:

	/**
	 * @brief
	 *	Construct this abstract clock.
	 * @param name
	 *	the name of the clock
	 * @param slidingWindowCapacity
	 *	the capacity of the sliding window
	 * @throw std::invalid_argument
	 *	if the sliding window capacity is not positive
	 * @post
	 *	The clock is in its initial state w.r.t. the current point in time.
	 */
	AbstractClock(const std::string& name, size_t slidingWindowCapacity)
		: _name(name), _stopwatch(), _slidingWindow(slidingWindowCapacity) {
		// Intentionally empty.
	}
	virtual ~AbstractClock() {
		// Intentionally empty.
	}

public:

	/**
	 * @brief
	 *	Get the name of this clock.
	 * @return
	 *	the name of this clock
	 */
	const std::string& getName() const {
		return _name;
	}

	/**
	 * @brief
	 *	Get the average duration spend in the associated code section(s).
	 * @return
	 *	the average duration spent in the associated code section(s)
	 * @remark
	 *	If no duration was measured yet, @a 0 is returned.
	 */
	double avg() const {
		if (_slidingWindow.empty()) {
			return 0;
		}
		else {
			double totalTime = 0;
			for (size_t i = 0; i < _slidingWindow.size(); ++i) {
				totalTime += _slidingWindow.get(i);
			}
			return totalTime / _slidingWindow.size();
		}
	}

	/**
	 * @brief
	 *	Get the last duration spent in the associated code section(s).
	 * @return
	 *	the last duration spent in the associated code section(s)
	 * @remark
	 *	If no duration was measured yet, @a 0 is returned.
	 */
	double lst() const {
		if (_slidingWindow.empty()) {
			return 0;
		}
		else {
			return _slidingWindow.get(_slidingWindow.size() - 1);
		}
	}

	/**
	 * @brief
	 *	Enter the observed section.
	 */
	virtual void enter() {
		_stopwatch.reset();
		_stopwatch.start();
	}

	/**
	 * @brief
	 *	Leave the observed section.
	 */
	virtual void leave() {
		// Stop the stopwatch.
		_stopwatch.stop();
		// Add the elapsed time to the sliding window.
		_slidingWindow.add(_stopwatch.elapsed());
		// Reset the stopwatch.
		_stopwatch.reset();
	}

	/**
	 * @brief
	 *	Put the clock into its initial state w.r.t. the current point in time.
	 */
	virtual void reinit() {
		_slidingWindow.clear();
	}
};

} // namespace Internal

/**
 * @brief
 *	The specialization for ClockPolicy::NonRecursive.
 */
template <>
struct Clock<ClockPolicy::NonRecursive> : public Internal::AbstractClock<ClockPolicy::NonRecursive>
{

public:

	/**
	 * @brief
	 *	Construct this clock.
	 * @param name
	 *	the name of this clock
	 * @param slidingWindowCapacity
	 *	the capacity of the sliding window of this clock
	 * @throw std::invalid_argument
	 *	if the sliding window capacity is not positive
	 * @post
	 *	The clock is in its initial state w.r.t. the current point in time.
	 */
	Clock(const std::string& name, size_t slidingWindowCapacity);

};

/**
 * @brief
 *	The specialization for ClockPolicy::Recursive.
 */
template <>
struct Clock<ClockPolicy::Recursive> : public Internal::AbstractClock<ClockPolicy::Recursive> {

private:

	/**
	 * @internal
	 * @brief
	 *	The balance of enter and leave calls.
	 */
	int _balance;

public:

	/**
	 * @brief
	 *	Construct this clock.
	 * @param name
	 *	the name of this clock
	 * @param slidingWindowCapacity
	 *	the capacity of the sliding window of this clock
	 * @throw std::invalid_argument
	 *	if the sliding window capacity is not positive
	 * @post
	 *	The clock is in its initial state w.r.t. the current point in time.
	 */
	Clock(const std::string& name, size_t slidingWindowCapacity);

	/** @internal @copydoc AbstractClock::enter */
	virtual void enter() override;

	/** @internal @copydoc AbstractClock::leave */
	virtual void leave() override;
};

template <typename _ClockPolicy>
struct ClockScope : private idlib::non_copyable {
private:
	Clock<_ClockPolicy>& _clock;
public:
	ClockScope(Clock<_ClockPolicy>& clock) :
		_clock(clock) {
		_clock.enter();
	}
	~ClockScope() {
		_clock.leave();
	}
};

} // namespace Ego::Time
