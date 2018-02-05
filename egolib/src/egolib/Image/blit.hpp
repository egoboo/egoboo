#pragma once

#include "egolib/platform.h"

namespace Ego {

template <typename T>
struct blit_functor;

template <typename T>
void blit(T *source, T *target)
{ blit_functor<T>()(source, target); }

template <typename T>
void blit(T *source, const Rectangle2f& source_rectangle, T *target)
{ blit_functor<T>()(source, source_rectangle, target); }

template <typename T>
void blit(T *source, T *target, const Point2f& target_position)
{ blit_functor<T>()(source, target, target_position); }

template <typename T>
void blit(T *source, const Rectangle2f& source_rectangle, T *target, const Point2f& target_position)
{ blit_functor<T>()(source, source_rectangle, target, target_position); }

} // namespace Ego
