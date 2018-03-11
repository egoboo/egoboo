#pragma once

#include "egolib/egolib.h"

/// The state of the weather.
struct WeatherState
{
    int timer_reset;                    ///< How long between each spawn?
    bool  over_water;                   ///< Only spawn over water?
    LocalParticleProfileRef part_gpip;  ///< Which particle to spawn?

    PLA_REF iplayer;
    int     time;                       ///< 0 is no weather

    void upload(const wawalite_weather_t& source);
    /// @brief Iterate the state of the weather.
    /// @remarks Drops snowflakes or rain or whatever.
    void update();
};
