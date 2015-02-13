#include "game/gui/Label.hpp"
#include "game/audio/AudioSystem.hpp"

Label::Label(const std::string &text) :
	_text()
{
	setText(text);
}

void Label::draw()
{
    //Draw text
	Ego::Renderer::getSingleton()->setColour(Ego::Colour4f::WHITE);
    fnt_drawTextBox_OGL(_gameEngine->getUIManager()->getDefaultFont(), {0xFF, 0xFF, 0xFF, 0xFF}, getX(), getY(), getWidth(), getHeight(), 25, nullptr, "%s", _text.c_str());
}

void Label::setText(const std::string &text)
{
	_text = text;

	//Recalculate our size
	int textWidth, textHeight;
	fnt_getTextBoxSize(_gameEngine->getUIManager()->getDefaultFont(), 25, _text.c_str(), &textWidth, &textHeight);
	setSize(textWidth, textHeight);
}
