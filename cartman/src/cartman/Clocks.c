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

#include "cartman/Clocks.h"

int Clocks::time_ticks = 0;

template <> int Clocks::timePassed<Time::Unit::Ticks, int>() {
    return time_ticks;
}
template <> double Clocks::timePassed<Time::Unit::Ticks, double>() {
    return time_ticks;
}
// Time passed in milliseconds.
template <> double Clocks::timePassed<Time::Unit::Milliseconds, double>() {
    return timePassed<Time::Unit::Ticks, double>() * CLOCKRATE;
}
// Time passed in seconds.
template <> double Clocks::timePassed<Time::Unit::Seconds, double>() {
    return timePassed<Time::Unit::Milliseconds, double>() / 1000.0;
}