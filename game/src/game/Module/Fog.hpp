#pragma once

#include "egolib/egolib.h"

/// The in-game fog state
/// @warn Fog is currently not used
struct fog_instance_t
{
    bool _on;            ///< Do ground fog?
    float _top,
        _bottom;
    uint8_t _red, _grn, _blu;
    float _distance;

    void upload(const wawalite_fog_t& source);
};
