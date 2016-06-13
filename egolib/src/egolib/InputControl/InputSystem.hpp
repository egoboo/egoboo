#pragma once

#include "egolib/Math/_Include.hpp"
#include "egolib/Core/Singleton.hpp"

namespace Ego
{
namespace Input
{


class InputSystem : public Ego::Core::Singleton<InputSystem> {
public:
    InputSystem();
    ~InputSystem();
    
    void update();

    void initialize();

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

private:
    //Mouse input
    Vector2f _mouseMovement;
    std::array<bool, NR_OF_MOUSE_BUTTONS> _mouseButtonDown;
};


} //Input
} //Ego
