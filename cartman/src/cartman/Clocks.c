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