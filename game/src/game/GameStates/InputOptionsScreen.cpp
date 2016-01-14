#include "game/GameStates/InputOptionsScreen.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Label.hpp"

namespace Ego
{
namespace GameStates
{

InputOptionsScreen::InputOptionsScreen()
{
    //const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    //Main label
    std::shared_ptr<Label> mainLabel = std::make_shared<Label>("Input Settings");
    mainLabel->setPosition(20, 20);
    addComponent(mainLabel);

    addInputOption("test");

    //Back button
    std::shared_ptr<Button> backButton = std::make_shared<Button>("Save Settings", SDLK_ESCAPE);
    backButton->setPosition(20, SCREEN_HEIGHT-80);
    backButton->setSize(200, 30);
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

void InputOptionsScreen::addInputOption(const std::string& label)
{
    //Label
    std::shared_ptr<Label> name = std::make_shared<Label>(label);
    name->setPosition(20, 40);
    addComponent(name);

    //Button
    std::shared_ptr<Button> inputOption = std::make_shared<Button>("N/A");
    inputOption->setPosition(20 + 100, 40);
    inputOption->setSize(200, 30);
    inputOption->setOnClickFunction(
    []{
    	//TODO
    });
    addComponent(inputOption);

}

} //GameStates
} //Ego
