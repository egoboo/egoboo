#pragma once

#include "idlib/color.hpp"

namespace Ego {
    
/// A colour in RGB colour space with single-precision floating-point components each within the range from 0 (inclusive) to 1 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 1 indicates maximal intensity of the component.
using Colour3f = idlib::color<idlib::RGBf>;

/// A colour in RGBA colour space with single-precision floating-point components each within the range from 0 (inclusive) to 1 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 1 indicates maximal intensity of the component.
using Colour4f = idlib::color<idlib::RGBAf>;

/// A colour in RGB colour space with unsigned integer components each within the range from 0 (inclusive) to 255 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
using Colour3b = idlib::color<idlib::RGBb>;

/// A colour in RGBA colour space with unsigned integer components each within the range from 0 (inclusive) to 255 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
using Colour4b = idlib::color<idlib::RGBAb>;
    
} // namespace Ego
