#include "game/GUI/OptionsButton.hpp"

OptionsButton::OptionsButton(const std::string &label) :
	_label(label)
{

}

void OptionsButton::setPosition(const int x, const int y)
{
	_label.setPosition(x, y);
	GUIComponent::setPosition(x + 200, y-getHeight()/2);
}

void OptionsButton::draw()
{
	_label.draw();
	Button::draw();
}