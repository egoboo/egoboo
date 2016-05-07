//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//*
//********************************************************************************************

#pragma once

#include "egolib/egolib.h"
#define CLOCKRATE 14
#define SECONDRATE 1000

struct Clocks {
private:
    static int time_ticks;
public:
    static void incrementTime() {
        time_ticks++;
    }
    static void initialize() {
        time_ticks = 0;
    }
    static void update() {
        time_ticks = Time::now<Time::Unit::Ticks>() >> 3;
    }

public:
    template <Time::Unit UnitType, typename ValueType> static ValueType timePassed();

};

// Time passed in ticks.
template <> int Clocks::timePassed<Time::Unit::Ticks, int>();
template <> double Clocks::timePassed<Time::Unit::Ticks, double>();
// Time passed in milliseconds.
template <> double Clocks::timePassed<Time::Unit::Milliseconds, double>();
// Time passed in seconds.
template <> double Clocks::timePassed<Time::Unit::Seconds, double>();
