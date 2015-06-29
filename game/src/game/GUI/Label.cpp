#include "game/GUI/Label.hpp"

Label::Label(const std::string &text) :
	_text(),
    _font(_gameEngine->getUIManager()->getDefaultFont())
{
	setText(text);
}

void Label::draw()
{
    //Draw text
    _font->drawTextBox(_text, getX(), getY(), getWidth(), getHeight(), 25);
}

void Label::setText(const std::string &text)
{
	_text = text;

	//Recalculate our size
	int textWidth, textHeight;
    _font->getTextBoxSize(_text, 25, &textWidth, &textHeight);
	setSize(textWidth, textHeight);
}

void Label::setFont(const std::shared_ptr<Ego::Font> &font)
{
    _font = font;

    //Recalculate our size
    int textWidth, textHeight;
    _font->getTextBoxSize(_text, 25, &textWidth, &textHeight);
    setSize(textWidth, textHeight);
}
