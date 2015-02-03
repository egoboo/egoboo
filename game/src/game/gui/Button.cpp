#include "game/gui/Button.hpp"
#include "game/ui.h"

#include "game/menu.h"

Button::Button(int hotkey) :
    _buttonText(),
    _clickSoundID("button.wav"),
    _mouseOver(false),
    _onClickFunction(nullptr),
    _hotkey(hotkey)
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

void Button::draw()
{
    ui_drawButton(0, getX(), getY(), getWidth(), getHeight(), nullptr);

    //Draw centered text in button
    if(!_buttonText.empty())
    {
        int textWidth, textHeight;
        fnt_getTextSize(ui_getFont(), _buttonText.c_str(), &textWidth, &textHeight);

        GL_DEBUG( glColor3f )(1, 1, 1);
        fnt_drawText_OGL_immediate(ui_getFont(), {0xFF, 0xFF, 0xFF, 0x00}, getX() + (getWidth()-textWidth)/2, getY() + (getHeight()-textHeight)/2, "%s", _buttonText.c_str());        
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
    if(!_onClickFunction || !isEnabled()) return;

    if(!_clickSoundID.empty()) 
    {
        //_audioSystem.playSound(_clickSoundID);
    }

    _mouseOver = true;
    _onClickFunction();
}

void Button::setOnClick(const std::function<void()> onClick)
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

void Button::setClickSound(const std::string &soundID)
{
    _clickSoundID = soundID;
}
