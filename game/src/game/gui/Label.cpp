#include "game/gui/Label.hpp"
#include "game/audio/AudioSystem.hpp"
#include "game/ui.h"

Label::Label(const std::string &text) :
	_text()
{
	setText(text);
}

void Label::draw()
{
    //Draw text
    GL_DEBUG(glColor4fv)(Ego::white_vec);
    ui_drawTextBox(ui_getFont(), _text.c_str(), getX(), getY(), 0, 0, 20);
}

void Label::setText(const std::string &text)
{
	_text = text;

	//Recalculate our size
	int textWidth, textHeight;
	fnt_getTextBoxSize(ui_getFont(), 20, _text.c_str(), &textWidth, &textHeight);
	setSize(textWidth, textHeight);
}
