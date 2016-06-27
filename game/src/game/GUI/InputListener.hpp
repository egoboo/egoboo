#pragma once

#include "egolib/egolib.h"

namespace Ego {
namespace GUI {
/**
 * This interface class allows the object to listen to any SDL events occuring. If any of the functions
 * return true, then it means the event will be consumed (and no other listeners will receive it)
 */
class InputListener {
public:
    virtual ~InputListener();
    /**
     * @brief Invoked if a mouse moved event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseMoved(const Events::MouseMovedEventArgs& e);
    /**
     * @brief Invoked if a keyboard key pressed event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEventArgs& e);
    /**
     * @brief Invoked if a mouse button clicked event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseButtonClicked(const Events::MouseButtonClickedEventArgs& e);
    /**
     * @brief Invoked if a mouse button released event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseButtonReleased(const Events::MouseButtonReleasedEventArgs& e);

    virtual bool notifyMouseScrolled(const int amount);
    
};

} // namespace GUI
} // namespace Ego
