#include "egolib/game/GUI/Button.hpp"
#include "egolib/game/Core/GameEngine.hpp"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/Audio/AudioSystem.hpp"

namespace Ego::GUI {

const Colour4f Button::DEFAULT_BUTTON_COLOUR = {0.66f, 0.00f, 0.00f, 0.60f};
const Colour4f Button::HOVER_BUTTON_COLOUR = {0.54f, 0.00f, 0.00f, 1.00f};
const Colour4f Button::DISABLED_BUTTON_COLOUR = {0.25f, 0.25f, 0.25f, 0.60f};

Button::Button(int hotkey) :
    _mouseOver(false),
    _buttonTextRenderer(),
    _buttonTextWidth(),
    _buttonTextHeight(),
    _buttonText(),
    _hotkey(hotkey),
    _slidyButtonTargetX(0.0f),
    _slidyButtonCurrentX(0.0f) {}

Button::Button(const std::string &buttonText, int hotkey) : Button(hotkey) {
    setText(buttonText);
}

const std::string& Button::getText() const {
    return _buttonText;
}

void Button::setText(const std::string &text) {
    _buttonText = text;
    if (_buttonText.empty()) {
        _buttonTextRenderer = nullptr;
    } else {
        auto font = _gameEngine->getUIManager()->getFont(UIManager::UIFontType::FONT_DEFAULT);
        _buttonTextRenderer = font->layoutText(_buttonText, &_buttonTextWidth, &_buttonTextHeight);
    }
}

void Button::updateSlidyButtonEffect() {
    if (getX() < _slidyButtonTargetX) {
        const float SLIDY_LERP = 2.0f*getWidth() / GameEngine::GAME_TARGET_FPS;
        _slidyButtonCurrentX += SLIDY_LERP;
        setX(_slidyButtonCurrentX);
    } else if (_slidyButtonTargetX > 0.0f) {
        setX(_slidyButtonTargetX);
        _slidyButtonTargetX = 0.0f;
    }
}

void Button::draw(DrawingContext& drawingContext) {
    //Update slidy button effect
    updateSlidyButtonEffect();

    auto &renderer = Renderer::get();

    // Draw the button
    renderer.getTextureUnit().setActivated(nullptr);

    // Determine button color
    if (!isEnabled()) {
        renderer.setColour(DISABLED_BUTTON_COLOUR);
    } else if (_mouseOver) {
        renderer.setColour(HOVER_BUTTON_COLOUR);
    } else {
        renderer.setColour(DEFAULT_BUTTON_COLOUR);
    }
    _gameEngine->getUIManager()->drawQuad2d(getDerivedBounds());
    // Draw centered text in button
    if (_buttonTextRenderer) {
        _buttonTextRenderer->render(getDerivedPosition().x() + (getWidth() - _buttonTextWidth) / 2,
                                    getDerivedPosition().y() + (getHeight() - _buttonTextHeight) / 2);
    }
}

bool Button::notifyMousePointerMoved(const Events::MousePointerMovedEvent& e) {
    _mouseOver = contains(e.get_position());

    return false;
}

bool Button::notifyMouseButtonPressed(const Events::MouseButtonPressedEvent& e) {
    if (_mouseOver && e.get_button() == SDL_BUTTON_LEFT) {
        doClick();
        return true;
    }

    return false;
}

void Button::doClick() {
    if (!isEnabled()) return;
    AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_BUTTON_CLICK));
    Clicked();
}

void Button::setOnClickFunction(const std::function<void()> onClick) {
    Clicked.subscribe(onClick);
}

bool Button::notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEvent& e) {
    //No hotkey assigned to this button
    if (_hotkey == SDLK_UNKNOWN) return false;

    //Hotkey pressed?
    if (e.get_key() == _hotkey) {
        doClick();
        return true;
    }

    return false;
}

void Button::beginSlidyButtonEffect(float offset) {
    //Finish any old SlidyButton first effect that might be in place
    if (_slidyButtonTargetX > 0.0f) {
        setX(_slidyButtonTargetX);
    }

    _slidyButtonTargetX = getX();
    setX(getX() - offset);
    _slidyButtonCurrentX = getX();
}

void Button::setEnabled(const bool enabled) {
    if (!enabled) {
        _mouseOver = false;
    }
    Component::setEnabled(enabled);
}

} // namespace Ego::GUI
