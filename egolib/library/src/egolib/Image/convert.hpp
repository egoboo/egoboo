#pragma once

#include "egolib/Graphics/PixelFormat.hpp"

namespace Ego {

template <typename T>
struct convert_functor;

template <typename T>
auto convert(const std::shared_ptr<T>& pixels, const pixel_descriptor& format) -> decltype(convert_functor<T>()(pixels, format))
{ return convert_functor<T>()(pixels, format); }

} // namespace Ego
