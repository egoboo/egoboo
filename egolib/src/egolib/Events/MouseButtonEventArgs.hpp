#pragma once

#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace Events {

/// @brief The event arguments of a mouse button event.
template <id::event::mouse_button_event_kind Kind>
class MouseButtonEventArgs
{
private:
    /// @brief The position of the mouse.
    Point2f m_position;

    /// @brief The mouse button.
    int m_button;

public:
    /// @brief Construct these mouse button event arguments with the specified values.
    /// @param position the position of the mouse
    /// @param button the mouse button
    explicit MouseButtonEventArgs(const Point2f& position, int button)
        : m_position(position), m_button(button)
    {}

    MouseButtonEventArgs(const MouseButtonEventArgs&) = default;
    MouseButtonEventArgs(MouseButtonEventArgs&&) = default;
    MouseButtonEventArgs& operator=(const MouseButtonEventArgs&) = default;
    MouseButtonEventArgs& operator=(MouseButtonEventArgs&&) = default;

    /// @brief Get the kind of the mouse button event.
    /// @return the kind of the mouse button event
    id::event::mouse_button_event_kind kind() const
    {
        return Kind;
    }

    /// @brief Get the mouse position.
    /// @return the mouse position
    const Point2f& position() const
    {
        return m_position;
    }

    /// @brief Get the mouse button.
    /// @return the mouse button
    int getButton() const
    {
        return m_button;
    }

}; // class MouseButtonEventArgs

} // namespace Events
} // namespace Ego
