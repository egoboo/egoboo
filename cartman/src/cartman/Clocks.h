#pragma once

#include "egolib/egolib.h"
#define CLOCKRATE 14
#define SECONDRATE 1000

enum class Unit {
    Ticks,
    Milliseconds,
    Seconds,
};

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
    template <Unit UnitType, typename ValueType> static ValueType timePassed();

    // Time passed in ticks.
    template <> static int timePassed<Unit::Ticks, int>() {
        return time_ticks;
    }
    template <> static double timePassed<Unit::Ticks,double>() {
        return time_ticks;
    }
    // Time passed in milliseconds.
    template <> static double timePassed<Unit::Milliseconds,double>() {
        return timePassed<Unit::Ticks,double>() * CLOCKRATE;
    }
    // Time passed in seconds.
    template <> static double timePassed<Unit::Seconds,double>() {
        return timePassed<Unit::Milliseconds,double>() / 1000.0;
    }
};