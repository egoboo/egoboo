/// @brief Pad a pixel buffer to the right (bottom) such that its width (height)
/// is the smallest power of two greater than or equal to its old width (height).
#pragma once

namespace Ego {

template <typename T>
struct power_of_two_functor;

template <typename T>
auto power_of_two(const std::shared_ptr<T>& pixels) -> decltype(power_of_two_functor<T>()(pixels))
{ return power_of_two_functor<T>()(pixels); }

} // namespace Ego