#include "game/GUI/Label.hpp"

Label::Label(const std::string &text) :
	_text()
{
	setText(text);
}

void Label::draw()
{
    //Draw text
    _gameEngine->getUIManager()->getDefaultFont()->drawTextBox(_text, getX(), getY(), getWidth(), getHeight(), 25);
}

void Label::setText(const std::string &text)
{
	_text = text;

	//Recalculate our size
	int textWidth, textHeight;
    _gameEngine->getUIManager()->getDefaultFont()->getTextBoxSize(_text, 25, &textWidth, &textHeight);
	setSize(textWidth, textHeight);
}
