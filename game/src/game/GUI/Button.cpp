#include "game/GUI/Button.hpp"

const Ego::Math::Colour4f Button::DEFAULT_BUTTON_COLOUR  = {0.66f, 0.00f, 0.00f, 0.60f};
const Ego::Math::Colour4f Button::HOVER_BUTTON_COLOUR    = {0.54f, 0.00f, 0.00f, 1.00f};
const Ego::Math::Colour4f Button::DISABLED_BUTTON_COLOUR = {0.25f, 0.25f, 0.25f, 0.60f};

Button::Button(int hotkey) :
    _mouseOver(false),
    _buttonTextRenderer(),
    _buttonTextWidth(),
    _buttonTextHeight(),
    _buttonText(),
    _onClickFunction(nullptr),
    _hotkey(hotkey),
    _slidyButtonTargetX(0.0f),
    _slidyButtonCurrentX(0.0f)
{
}

Button::Button(const std::string &buttonText, int hotkey) : Button(hotkey)
{
    setText(buttonText);
}

void Button::setText(const std::string &text)
{
    _buttonText = text;
    if (_buttonText.empty()) {
        _buttonTextRenderer = nullptr;
    } else {
        auto font = _gameEngine->getUIManager()->getFont(UIManager::UIFontType::FONT_DEFAULT);
        _buttonTextRenderer = font->layoutText(_buttonText, &_buttonTextWidth, &_buttonTextHeight);
    }
}

void Button::updateSlidyButtonEffect()
{
    if(getX() < _slidyButtonTargetX) {
        const float SLIDY_LERP = 2.0f*getWidth() / GameEngine::GAME_TARGET_FPS;
        _slidyButtonCurrentX += SLIDY_LERP;
        setX(_slidyButtonCurrentX);
    }
    else if(_slidyButtonTargetX > 0.0f) {
        setX(_slidyButtonTargetX);
        _slidyButtonTargetX = 0.0f;
    }
}

void Button::draw()
{
    //Update slidy button effect
    updateSlidyButtonEffect();
    
    auto &renderer = Ego::Renderer::get();

    // Draw the button
	renderer.getTextureUnit().setActivated(nullptr);

    // convert the virtual coordinates to screen coordinates
    //ui_virtual_to_screen( vx, vy, &x1, &y1 );
    //ui_virtual_to_screen( vx + vwidth, vy + vheight, &x2, &y2 );

    //Determine button color
    if(!isEnabled())
    {
        renderer.setColour(DISABLED_BUTTON_COLOUR);
    }
    else if(_mouseOver)
    {
        renderer.setColour(HOVER_BUTTON_COLOUR);
    }
    else
    {
        renderer.setColour(DEFAULT_BUTTON_COLOUR);
    }
    struct Vertex
    {
        float x, y;
    };
    auto vb = _gameEngine->getUIManager()->_vertexBuffer;
    Vertex *v = static_cast<Vertex *>(vb->lock());
    v->x = getX(); v->y = getY(); v++;
    v->x = getX(); v->y = getY() + getHeight(); v++;
    v->x = getX() + getWidth(); v->y = getY() + getHeight(); v++;
    v->x = getX() + getWidth(); v->y = getY();
    vb->unlock();
    renderer.render(*vb, Ego::PrimitiveType::Quadriliterals, 0, 4);

    //Draw centered text in button
    if(_buttonTextRenderer)
    {
        _buttonTextRenderer->render(getX() + (getWidth() - _buttonTextWidth) / 2, getY() + (getHeight() - _buttonTextHeight) / 2);
    }
}

bool Button::notifyMouseMoved(const Ego::Events::MouseMovedEventArgs& e)
{
    _mouseOver = contains(e.getPosition());

    return false;
}

bool Button::notifyMouseClicked(const Ego::Events::MouseClickedEventArgs& e)
{
    if(_mouseOver && e.getButton() == SDL_BUTTON_LEFT)
    {
        doClick();
        return true;
    }   

    return false;
}

void Button::doClick()
{
    if(!isEnabled()) return;

    AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_BUTTON_CLICK));

    //_mouseOver = true;
    _onClickFunction();
}

void Button::setOnClickFunction(const std::function<void()> onClick)
{
    _onClickFunction = onClick;
}

bool Button::notifyKeyDown(const int keyCode)
{
    //No hotkey assigned to this button
    if(_hotkey == SDLK_UNKNOWN) return false;

    //Hotkey pressed?
    if(keyCode == _hotkey)
    {
        doClick();
        return true;
    }

    return false;
}

void Button::beginSlidyButtonEffect(float offset)
{
    //Finish any old SlidyButton first effect that might be in place
    if(_slidyButtonTargetX > 0.0f) {
        setX(_slidyButtonTargetX);
    }

    _slidyButtonTargetX = getX();
    setX(getX() - offset);
    _slidyButtonCurrentX = getX();
}

bool Button::isEnabled() const
{
    if(!_onClickFunction) return false;
    return GUIComponent::isEnabled();
}

void Button::setEnabled(const bool enabled)
{
    if(!enabled) {
        _mouseOver = false;
    }

    GUIComponent::setEnabled(enabled);
}