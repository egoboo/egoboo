#include "game/GameStates/InputOptionsScreen.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Label.hpp"
#include "egolib/InputControl/InputDevice.hpp"

namespace Ego
{
namespace GameStates
{

InputOptionsScreen::InputOptionsScreen()
{
    //const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    //Main label
    auto mainLabel = std::make_shared<Ego::GUI::Label>("Input Settings");
    mainLabel->setPosition(Point2f(20, 20));
    addComponent(mainLabel);

    int xPos = 20;
    int yPos = mainLabel->getY() + mainLabel->getHeight() + 20;

    Ego::Input::InputDevice &inputDevice = Ego::Input::InputDevice::DeviceList[0];
    inputDevice.setInputMapping(Ego::Input::InputDevice::InputButton::MOVE_LEFT, SDL_SCANCODE_LEFT);

    addInputOption(xPos, yPos, "Move Left: ");


    //Shift all buttons to the right of the widest label
    for(std::shared_ptr<GUIComponent> &component : ComponentContainer::iterator()) {
        std::shared_ptr<Button> button = std::dynamic_pointer_cast<Button>(component);
        if(button) {
            button->setPosition(button->getX() + _maxLabelWidth, button->getY());
        }
    }

    //Back button
    auto backButton = std::make_shared<Ego::GUI::Button>("Save Settings", SDLK_ESCAPE);
    backButton->setPosition(Point2f(20, SCREEN_HEIGHT-80));
    backButton->setSize(Vector2f(200, 30));
    backButton->setOnClickFunction(
    [this]{
        endState();

        // save the setup file
        setup_upload(&egoboo_config_t::get());
    });
    addComponent(backButton);
}

void InputOptionsScreen::update()
{

}

void InputOptionsScreen::beginState()
{

}

void InputOptionsScreen::drawContainer()
{

}

void InputOptionsScreen::addInputOption(const int xPos, const int yPos, const std::string& label)
{
    //Label
    auto name = std::make_shared<Ego::GUI::Label>(label);
    name->setPosition(xPos, yPos);
    addComponent(name);
    _maxLabelWidth = std::max(_maxLabelWidth, name->getWidth());

    //Button
    auto inputOption = std::make_shared<Ego::GUI::Button>("N/A");
    inputOption->setPosition(xPos + 50, yPos);
    inputOption->setSize(200, 25);
    inputOption->setOnClickFunction(
    []{
    	//TODO
    });
    addComponent(inputOption);
}

} //GameStates
} //Ego
