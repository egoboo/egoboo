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
