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

using MouseButtonClickedEvent = idlib::events::mouse_button_clicked_event;
using MouseButtonPressedEvent = idlib::events::mouse_button_pressed_event;
using MouseButtonReleasedEvent = idlib::events::mouse_button_released_event;

using KeyboardKeyTypedEvent = idlib::events::keyboard_key_typed_event;
using KeyboardKeyPressedEvent = idlib::events::keyboard_key_pressed_event;
using KeyboardKeyReleasedEvent = idlib::events::keyboard_key_released_event;

using MousePointerEnteredEvent = idlib::events::mouse_pointer_entered_event;
using MousePointerExitedEvent = idlib::events::mouse_pointer_exited_event;
using MousePointerMovedEvent = idlib::events::mouse_pointer_moved_event;

using MouseWheelTurnedEvent = idlib::events::mouse_wheel_turned_event;

using WindowResizedEvent = idlib::events::window_resized_event;

using WindowShownEvent = idlib::events::window_shown_event;
using WindowHiddenEvent = idlib::events::window_hidden_event;

using KeyboardInputFocusReceivedEvent = idlib::events::keyboard_input_focus_received_event;
using KeyboardInputFocusLostEvent = idlib::events::keyboard_input_focus_lost_event;

} } // namespace Ego::Events

namespace Ego {

using IndexDescriptor = idlib::index_descriptor;
using VertexElementDescriptor = idlib::vertex_component_descriptor;
using VertexDescriptor = idlib::vertex_descriptor;
using Extension = idlib::file_system::extension<char>;
using VertexElementDescriptor = idlib::vertex_component_descriptor;

} // namespace Ego

/// @brief Project a vector (x,y,z) to a vector (x,y).
/// @tparam S the scalar typedef
/// @param v the vector (x,y,z)
/// @return the vector (x,y)
template <typename S>
auto xy(const idlib::vector<S, 3>& v)
{ return idlib::vector<S, 2>(v.x(), v.y()); }

template <typename V, typename W>
auto dot(V&& v, W&& w)
{ return idlib::dot_product(std::forward<V>(v), std::forward<W>(w)); }

template <typename V, typename W>
auto cross(V&& v, W&& w)
{ return idlib::cross_product(std::forward<V>(v), std::forward<W>(w)); }

template <typename V>
auto normalize(V&& v)
{ return idlib::normalize(std::forward<V>(v), idlib::euclidean_norm_functor<std::decay_t<V>>()); }

/// single-precision floating-point 2d vector.
using Vector2f = idlib::vector<float, 2>;
/// single-precision floating-point 3d vector.
using Vector3f = idlib::vector<float, 3>;
/// single-precision floating-point 4d vector.
using Vector4f = idlib::vector<float, 4>;

/// single-precision floating-point 2d point.
using Point2f = idlib::point<Vector2f>;
/// single-precision floating-point 3d point.
using Point3f = idlib::point<Vector3f>;
/// single-precision floating-point 4d point.
using Point4f = idlib::point<Vector4f>;

/// single-precision floating-point 2d axis aligned box.
using AxisAlignedBox2f = idlib::axis_aligned_box<Point2f>;
/// single-precision floating-point rectangle (i.e. an axis aligned box in 2d),
using Rectangle2f = AxisAlignedBox2f;
/// A 3D axis aligned box.
using AxisAlignedBox3f = idlib::axis_aligned_box<Point3f>;

/// single-precision floating-point 2d sphere.
using Sphere2f = idlib::sphere<Point2f>;
/// single-precision floating-point circle (i.e. a sphere in 2d).
using Circle2f = Sphere2f;
/// single-precision floating-point 3d sphere.
using Sphere3f = idlib::sphere<Point3f>;

/// single-precision floating-point 2d line.
using Line2f = idlib::line<Point2f>;
/// single-precision floating-point 3d line.
using Line3f = idlib::line<Point3f>;

/// single-precision floating-point 2d ray.
using Ray2f = idlib::ray<Point2f>;
/// single-precision floating-point 3d ray.
using Ray3f = idlib::ray<Point3f>;

/// single-precision floating-point 3d plane.
using Plane3f = idlib::plane<Point3f>;

/// A 3D axis aligned cube.
using AxisAlignedCube3f = idlib::axis_aligned_cube<Point3f>;

namespace Ego { namespace Math {

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

/// The type of an angle in degrees.
/// See idlib::degrees for more information.
using Degrees = idlib::angle<float, idlib::degrees>;

/// The type of an angle in radians.
/// See idlib::radians for more information.
using Radians = idlib::angle<float, idlib::radians>;

/// The type of an angle in turns.
/// See idlib::turns for more information.
using Turns = idlib::angle<float, idlib::turns>;

} } // namespace Ego::Math
