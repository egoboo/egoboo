#pragma once

#include "egolib/Math/_Include.hpp"
#include "egolib/Core/Singleton.hpp"
#include "egolib/InputControl/ModifierKeys.hpp"

namespace Ego
{
namespace Input
{


class InputSystem : public Ego::Core::Singleton<InputSystem> {
public:
    static constexpr float MOUSE_SENSITIVITY = 12.0f; //TODO: make configurable in settings.txt

    InputSystem();
    virtual ~InputSystem();
    
    void update();

    enum MouseButton : uint8_t
    {
        LEFT,
        MIDDLE,
        RIGHT,
        X1,
        X2,
        NR_OF_MOUSE_BUTTONS         //always last
    };

    const Vector2f& getMouseMovement() const;

    bool isMouseButtonDown(const MouseButton) const;

    bool isKeyDown(const SDL_Keycode key) const;

    /// @brief Get the modifier keys state.
    /// @return the modifier key state
    Ego::ModifierKeys getModifierKeys() const;

private:

    //Mouse input
    Vector2f _mouseMovement;
    std::array<bool, NR_OF_MOUSE_BUTTONS> _mouseButtonDown;

    //Keyboard input
    Ego::ModifierKeys _modifierKeys;
};

} //Input
} //Ego

struct latch_t {
public:
    latch_t() : 
        input(Vector2f::zero()), 
        b() 
    {
        //ctor
    }

    void clear() {
        input = Vector2f();
        b.reset();
    }

public:
    /// The input.
    Vector2f input;
    /// The button bits.
    std::bitset<32> b;         ///< the button bits
};
