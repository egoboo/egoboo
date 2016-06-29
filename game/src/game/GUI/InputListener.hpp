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

public:
    /**
     * @brief Invoked if a mouse moved event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseMoved(const Events::MouseMovedEventArgs& e);

public:
    /**
     * @brief Invoked if a mouse wheel turned event occurred.
     * @param e the event arguments
     * @retrn @a true if th event was processed, @a false otherwise
     */
    virtual bool notifyMouseWheelTurned(const Events::MouseWheelTurnedEventArgs& e);

public:
    /**
     * @brief Invoked if a mouse button pressed event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e);

    /**
     * @brief Invoked if a mouse button released event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseButtonReleased(const Events::MouseButtonReleasedEventArgs& e);

    /**
     * @brief Invoked if a mouse button clicked event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyMouseButtonClicked(const Events::MouseButtonClickedEventArgs& e);

public:
    /**
     * @brief Invoked if a keyboard key pressed event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEventArgs& e);

    /**
     * @brief Invoked if a keyboard key released event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyKeyboardKeyReleased(const Events::KeyboardKeyReleasedEventArgs& e);

    /**
     * @brief Invoked if a keyboard key typed event occurred.
     * @param e the event arguments
     * @return @a true if the event was processed, @a false otherwise
     */
    virtual bool notifyKeyboardKeyTyped(const Events::KeyboardKeyTypedEventArgs& e);
  
};

} // namespace GUI
} // namespace Ego
