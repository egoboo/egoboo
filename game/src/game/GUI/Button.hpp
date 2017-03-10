#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/Component.hpp"

namespace Ego {
namespace GUI {

class Button : public Component {
public:
    /// Signal invoked if the button was clicked.
    Id::Signal<void()> Clicked;
public:
    Button(int hotkey = SDLK_UNKNOWN);
    Button(const std::string &buttonText, int hotkey = SDLK_UNKNOWN);

    virtual void draw(DrawingContext& drawingContext) override;
    void setOnClickFunction(const std::function<void()> onClick);

public:
    /// @brief Get the button text.
    /// @return the button text
    const std::string& getText() const;
    /// @brief Set the button text.
    /// @param text the button text
    void setText(const std::string& text);

public:
    bool notifyMouseMoved(const Events::MouseMovedEventArgs& e) override;
    bool notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e) override;
    bool notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEventArgs& e) override;

    void doClick();

    void beginSlidyButtonEffect(float offset);

    void setEnabled(const bool enabled) override;

protected:
    void updateSlidyButtonEffect();

protected:
    bool _mouseOver;
    std::shared_ptr<Font::LaidTextRenderer> _buttonTextRenderer;
    int _buttonTextWidth;
    int _buttonTextHeight;

    static const Colour4f DEFAULT_BUTTON_COLOUR;
    static const Colour4f HOVER_BUTTON_COLOUR;
    static const Colour4f DISABLED_BUTTON_COLOUR;

private:
    std::string _buttonText;
    int _hotkey;
    float _slidyButtonTargetX;
    float _slidyButtonCurrentX;
};

} // namespace GUI
} // namespace Ego
