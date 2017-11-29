#pragma once

#include "egolib/platform.h"

namespace Ego {

template <typename T>
struct get_pixel_functor;

template <typename T>
Math::Colour4b get_pixel(const T *image, const Point2f& point)
{ return get_pixel_functor<T>()(image, point); }

} // namespace Ego
