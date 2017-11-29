#pragma once

#include "egolib/platform.h"

namespace Ego {

template <typename T>
struct fill_functor;

/// @{
/// @brief Fill the image with the specified color.
/// @param color the fill color
/// @param rectangle the rectangle of the image to fill. Clipped against the rectangle of the image.

template <typename T>
void fill(T *image, const Math::Colour3b& color)
{ fill_functor<T>()(image, color); }

template <typename T>
void fill(T *image, const Math::Colour3b& color, const Rectangle2f& rectangle)
{ fill_functor<T>()(image, color, rectangle); }

template <typename T>
void fill(T *image, const Math::Colour4b& color)
{ fill_functor<T>()(image, color); }

template <typename T>
void fill(T *image, const Math::Colour4b& color, const Rectangle2f& rectangle)
{ fill_functor<T>()(image, color, rectangle); }

/// @}

} // namespace Ego
