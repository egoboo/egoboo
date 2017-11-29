//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/integrations/idlib.hpp
/// @brief Integrations of Idlib and Idlib: Game Engine into Egolib.

#pragma once

#include "idlib/idlib.hpp"
#include "idlib/game-engine.hpp"


namespace Ego { namespace Events {

using MouseButtonClickedEvent = id::events::mouse_button_clicked_event;
using MouseButtonPressedEvent = id::events::mouse_button_pressed_event;
using MouseButtonReleasedEvent = id::events::mouse_button_released_event;

using KeyboardKeyTypedEvent = id::events::keyboard_key_typed_event;
using KeyboardKeyPressedEvent = id::events::keyboard_key_pressed_event;
using KeyboardKeyReleasedEvent = id::events::keyboard_key_released_event;

using MousePointerEnteredEvent = id::events::mouse_pointer_entered_event;
using MousePointerExitedEvent = id::events::mouse_pointer_exited_event;
using MousePointerMovedEvent = id::events::mouse_pointer_moved_event;

using MouseWheelTurnedEvent = id::events::mouse_wheel_turned_event;

using WindowResizedEvent = id::events::window_resized_event;

using WindowShownEvent = id::events::window_shown_event;
using WindowHiddenEvent = id::events::window_hidden_event;

using KeyboardInputFocusReceivedEvent = id::events::keyboard_input_focus_received_event;
using KeyboardInputFocusLostEvent = id::events::keyboard_input_focus_lost_event;

} } // namespace Ego::Events

namespace Ego {

using IndexDescriptor = id::index_descriptor;
using VertexElementDescriptor = id::vertex_component_descriptor;
using VertexDescriptor = id::vertex_descriptor;

using Extension = id::file_system::extension<char>;

} // namespace Ego

/// @brief Project a vector (x,y,z) to a vector (x,y).
/// @tparam S the scalar typedef
/// @param v the vector (x,y,z)
/// @return the vector (x,y)
template <typename S>
auto xy(const id::vector<S, 3>& v)
{ return id::vector<S, 2>(v.x(), v.y()); }

template <typename V, typename W>
auto dot(V&& v, W&& w)
{ return id::dot_product(std::forward<V>(v), std::forward<W>(w)); }

template <typename V, typename W>
auto cross(V&& v, W&& w)
{ return id::cross_product(std::forward<V>(v), std::forward<W>(w)); }

template <typename V>
auto normalize(V&& v)
{ return id::normalize(std::forward<V>(v), id::euclidean_norm_functor<std::decay_t<V>>()); }

/// single-precision floating-point 2d vector.
using Vector2f = id::vector<float, 2>;
/// single-precision floating-point 3d vector.
using Vector3f = id::vector<float, 3>;
/// single-precision floating-point 4d vector.
using Vector4f = id::vector<float, 4>;

/// single-precision floating-point 2d point.
using Point2f = id::point<Vector2f>;
/// single-precision floating-point 3d point.
using Point3f = id::point<Vector3f>;
/// single-precision floating-point 4d point.
using Point4f = id::point<Vector4f>;

/// single-precision floating-point 2d axis aligned box.
using AxisAlignedBox2f = id::axis_aligned_box<Point2f>;
/// single-precision floating-point rectangle (i.e. an axis aligned box in 2d),
using Rectangle2f = AxisAlignedBox2f;
/// A 3D axis aligned box.
using AxisAlignedBox3f = id::axis_aligned_box<Point3f>;

/// single-precision floating-point 2d sphere.
using Sphere2f = id::sphere<Point2f>;
/// single-precision floating-point circle (i.e. a sphere in 2d).
using Circle2f = Sphere2f;
/// single-precision floating-point 3d sphere.
using Sphere3f = id::sphere<Point3f>;

/// single-precision floating-point 2d line.
using Line2f = id::line<Point2f>;
/// single-precision floating-point 3d line.
using Line3f = id::line<Point3f>;

/// single-precision floating-point 2d ray.
using Ray2f = id::ray<Point2f>;
/// single-precision floating-point 3d ray.
using Ray3f = id::ray<Point3f>;

/// single-precision floating-point 3d plane.
using Plane3f = id::plane<Point3f>;

/// A 3D axis aligned cube.
using AxisAlignedCube3f = id::axis_aligned_cube<Point3f>;

namespace Ego { namespace Math {

/// A colour in RGB colour space with single-precision floating-point components each within the range from 0 (inclusive) to 1 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 1 indicates maximal intensity of the component.
using Colour3f = id::color<id::RGBf>;

/// A colour in RGBA colour space with single-precision floating-point components each within the range from 0 (inclusive) to 1 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 1 indicates maximal intensity of the component.
using Colour4f = id::color<id::RGBAf>;

/// A colour in RGB colour space with unsigned integer components each within the range from 0 (inclusive) to 255 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
using Colour3b = id::color<id::RGBb>;

/// A colour in RGBA colour space with unsigned integer components each within the range from 0 (inclusive) to 255 (inclusive).
/// A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
using Colour4b = id::color<id::RGBAb>;

/// The type of an angle in degrees.
/// See id::degrees for more information.
using Degrees = id::angle<float, id::degrees>;

/// The type of an angle in radians.
/// See id::radians for more information.
using Radians = id::angle<float, id::radians>;

/// The type of an angle in turns.
/// See id::turns for more information.
using Turns = id::angle<float, id::turns>;

} } // namespace Ego::Math
