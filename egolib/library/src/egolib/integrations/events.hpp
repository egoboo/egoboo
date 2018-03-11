#pragma once

#include "idlib/game_engine.hpp"

namespace Ego::Events {

using MouseButtonClickedEvent = idlib::events::mouse_button_clicked_event; ///< @todo Remove this.
using MouseButtonPressedEvent = idlib::events::mouse_button_pressed_event; ///< @todo Remove this.
using MouseButtonReleasedEvent = idlib::events::mouse_button_released_event; ///< @todo Remove this.

using KeyboardKeyTypedEvent = idlib::events::keyboard_key_typed_event; ///< @todo Remove this.
using KeyboardKeyPressedEvent = idlib::events::keyboard_key_pressed_event; ///< @todo Remove this.
using KeyboardKeyReleasedEvent = idlib::events::keyboard_key_released_event; ///< @todo Remove this.

using MousePointerEnteredEvent = idlib::events::mouse_pointer_entered_event; ///< @todo Remove this.
using MousePointerExitedEvent = idlib::events::mouse_pointer_exited_event; ///< @todo Remove this.
using MousePointerMovedEvent = idlib::events::mouse_pointer_moved_event; ///< @todo Remove this.

using MouseWheelTurnedEvent = idlib::events::mouse_wheel_turned_event; ///< @todo Remove this.

} // namespace Ego::Events
