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

/// @file egolib/clock.c
/// @brief Clock & timer implementation
/// @details This implementation was adapted from Noel Lopis' article in Game Programming Gems 4.

#include "egolib/clock.h"
#include "egolib/log.h"
#include "game/Core/GameEngine.hpp"

// this include must be the absolute last include
#include "egolib/mem.h"

ClockState_t *ClockState_t::create(const std::string& name, size_t slidingWindowCapacity)
{
    ClockState_t *self;
	try {
		self = new ClockState_t(name, slidingWindowCapacity);
	} catch (...) {
		return nullptr;
	}
	return self;
}

void ClockState_t::destroy(ClockState_t *self)
{
	if (self) {
		delete self;
	}
}

ClockState_t::ClockState_t(const std::string& name, size_t slidingWindowCapacity)
	: _name(name), _maxElapsedTime(0.2), _stopwatch(), _slidingWindow(slidingWindowCapacity) {
}

void ClockState_t::enter() {
	_stopwatch.reset();
	_stopwatch.start();
}

void ClockState_t::leave() {
	// Stop the stopwatch.
	_stopwatch.stop();
	// Get the elapsed time.
	double elapsedTime = std::min(_maxElapsedTime, _stopwatch.elapsed());
	// Add the elapsed time to the sliding window.
	_slidingWindow.add(elapsedTime);
	// Reset the stopwatch.
	_stopwatch.reset();
}

void ClockState_t::reinit() {
	_stopwatch.stop(); _stopwatch.reset(); /// @todo The stopwatch should also have a reinit method.
	_slidingWindow.clear();
}

double ClockState_t::avg() const
{
	if (_slidingWindow.empty()) {
		return 0;
	} else {
        double totalTime = 0;
        for (size_t i = 0; i < _slidingWindow.size(); ++i) {
            totalTime += _slidingWindow.get(i);
        }
        return totalTime / _slidingWindow.size();
    }
}

double ClockState_t::lst() const
{
	if (_slidingWindow.empty()) {
		return 0;
	} else {
		return _slidingWindow.get(_slidingWindow.size()-1);
	}
}
