#include "game/gui/Button.hpp"
#include "egolib/Audio/AudioSystem.hpp"

const GLXvector4f Button::DEFAULT_BUTTON_COLOUR  = {0.66f, 0.00f, 0.00f, 0.60f};
const GLXvector4f Button::HOVER_BUTTON_COLOUR    = {0.54f, 0.00f, 0.00f, 1.00f};
const GLXvector4f Button::DISABLED_BUTTON_COLOUR = {0.25f, 0.25f, 0.25f, 0.60f};

Button::Button(int hotkey) :
    _mouseOver(false),
    _buttonText(),
    _onClickFunction(nullptr),
    _hotkey(hotkey),
    _slidyButtonTargetX(0.0f)
{
}

Button::Button(const std::string &buttonText, int hotkey) : Button(hotkey)
{
    _buttonText = buttonText;
}

void Button::setText(const std::string &text)
{
    _buttonText = text;
}

void Button::updateSlidyButtonEffect()
{
    if(getX() < _slidyButtonTargetX) {
        const float SLIDY_LERP = 2*getWidth() / GameEngine::GAME_TARGET_FPS;
        setX(getX() + SLIDY_LERP);
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

    // Draw the button
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );
    
    // convert the virtual coordinates to screen coordinates
    //ui_virtual_to_screen( vx, vy, &x1, &y1 );
    //ui_virtual_to_screen( vx + vwidth, vy + vheight, &x2, &y2 );

    //Determine button color
    if(!isEnabled())
    {
        GL_DEBUG( glColor4fv )( DISABLED_BUTTON_COLOUR );
    }
    else if(_mouseOver)
    {
        GL_DEBUG( glColor4fv )( HOVER_BUTTON_COLOUR );
    }
    else
    {
        GL_DEBUG( glColor4fv )( DEFAULT_BUTTON_COLOUR );
    }

    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glVertex2f )( getX(), getY() );
        GL_DEBUG( glVertex2f )( getX(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY() );
    }
    GL_DEBUG_END();

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );

    //Draw centered text in button
    if(!_buttonText.empty())
    {
        int textWidth, textHeight;
        fnt_getTextSize(_gameEngine->getUIManager()->getDefaultFont(), _buttonText.c_str(), &textWidth, &textHeight);

		Ego::Renderer::get().setColour(Ego::Colour4f::WHITE);
        fnt_drawText_OGL_immediate(_gameEngine->getUIManager()->getDefaultFont(), {0xFF, 0xFF, 0xFF, 0x00}, getX() + (getWidth()-textWidth)/2, getY() + (getHeight()-textHeight)/2, "%s", _buttonText.c_str());        
    }
}

bool Button::notifyMouseMoved(const int x, const int y)
{
    _mouseOver = contains(x, y);

    return false;
}

bool Button::notifyMouseClicked(const int button, const int x, const int y)
{
    if(_mouseOver && button == SDL_BUTTON_LEFT)
    {
        doClick();
        return true;
    }   

    return false;
}

void Button::doClick()
{
    if(!isEnabled()) return;

    _audioSystem.playSoundFull(_audioSystem.getGlobalSound(GSND_BUTTON_CLICK));

    _mouseOver = true;
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