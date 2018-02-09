#pragma once

namespace Ego {
	
/// @brief Specifies a padding.
struct padding
{
    int left;
    int top;
    int right;
    int bottom;
};

/// @remark The padding shall be black (if no alpha channel is present) and shall be transparent black (if alpha channel is present).
template <typename T>
struct pad_functor;

template <typename T>
auto pad(const std::shared_ptr<T>& pixels, padding& padding) -> decltype(pad_functor<T>()(pixels, padding))
{ return pad_functor<T>()(pixels, padding); }

} // namespace Ego
