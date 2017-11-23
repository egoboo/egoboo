#include "game/GUI/InputListener.hpp"

namespace Ego {
namespace GUI {

InputListener::~InputListener() {
    //dtor
}

bool InputListener::notifyMousePointerMoved(const Events::MousePointerMovedEvent& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseWheelTurned(const Events::MouseWheelTurnedEvent& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseButtonPressed(const Events::MouseButtonPressedEvent& e) {
    // Default: Event is not handled.
    return false;
}


bool InputListener::notifyMouseButtonReleased(const Events::MouseButtonReleasedEvent& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyMouseButtonClicked(const Events::MouseButtonClickedEvent& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEvent& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyKeyboardKeyReleased(const Events::KeyboardKeyReleasedEvent& e) {
    // Default: Event is not handled.
    return false;
}

bool InputListener::notifyKeyboardKeyTyped(const Events::KeyboardKeyTypedEvent& e) {
    // Default: Event is not handled.
    return false;
}

} // namespace GUI
} // namespace Ego
