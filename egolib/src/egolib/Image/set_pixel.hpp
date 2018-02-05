#pragma once

#include "egolib/platform.h"

namespace Ego {

template <typename T>
struct set_pixel_functor;

template <typename T>
void set_pixel(T *image, const Math::Colour3b& color, const Point2f& position)
{ set_pixel_functor<T>()(image, color, position); }

template <typename T>
void set_pixel(T *image, const Math::Colour4b& color, const Point2f& point)
{ set_pixel_functor<T>()(image, color, point); }

} // namespace Ego
