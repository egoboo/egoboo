#include "game/gui/Label.hpp"
#include "game/audio/AudioSystem.hpp"
#include "game/ui.h"

Label::Label(const std::string &text) :
	_text(text)
{
	//ctor
}

void Label::draw()
{
    //Draw text
    GL_DEBUG(glColor4fv)(Ego::white_vec);
    fnt_drawText_OGL_immediate(ui_getFont(), {0xFF, 0xFF, 0xFF, 0x00}, getX(), getY(), "%s", _text.c_str());
}

void Label::setText(const std::string &text)
{
	_text = text;

	//Recalculate our size
	int textWidth, textHeight;
	fnt_getTextSize(ui_getFont(), _buttonText.c_str(), &textWidth, &textHeight);
	setSize(textWidth, textHeight;);
}
