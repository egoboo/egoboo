#pragma

#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace Events {
/**
 * @ingroup ego-events
 * @brief Enumeration of the different kinds of mouse button event.
 */
enum class MouseButtonEventKind {
    /**
     * @brief Kind of a mouse button pressed event.
     */
    Pressed,
    /**
     * @brief Kind of a mouse button released event.
     */
    Released,
    /**
     * @brief Kind of a mouse button clicked event.
     */
    Clicked,
};

/**
 * @ingroup ego-events
 * @brief The event arguments of a mouse button event.
 */
template <MouseButtonEventKind _MouseButtonEventKind>
struct MouseButtonEventArgs {
private:
    Point2f position;
    int button;

public:
    /**
     * @brief Construct these mouse button event arguments with the specified values.
     * @param position the position of the mouse
     * @param button the button
     */
    MouseButtonEventArgs(const Point2f& position, int button)
        : position(position), button(button) {}
    
    /**
     * @brief Construct these mouse button event arguments with the values of other mouse button event arguments.
     * @param other the other mouse button event arguments
     */
    MouseButtonEventArgs(const MouseButtonEventArgs& other)
        : position(other.position), button(other.button) {}
    
    /**
     * @brief Construct these mouse button event arguments with the values of other mouse button event arguments.
     * @param other the other mouse button event arguments
     */
    MouseButtonEventArgs(MouseButtonEventArgs&& other)
        : position(other.position), button(other.button) {}
    
    /**
     * @brief Assign these mouse button event arguments with the values of other mouse button event arguments.
     * @param other the other mouse button event arguments
     * @return these mouse button event arguments
     */
    MouseButtonEventArgs operator=(const MouseButtonEventArgs& other) {
        MouseButtonEventArgs temporary(other);
        std::swap(*this, temporary);
        return *this;
    }
    
    /**
     * @brief Get the kind of the mouse button event.
     * @return the kind of the mouse button event
     */
    MouseButtonEventKind getKind() const {
        return _MouseButtonEventKind;
    }
    
    /**
     * @brief Get the mouse position.
     * @return the mouse position
     */
    const Point2f& getPosition() const {
        return position;
    }
    
    /**
     * @brief Get the mouse button.
     * @return the mouse button
     */
    int getButton() const {
        return button;
    }

};

} // namespace Events
} // namespace Ego
