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
    auto mainLabel = std::make_shared<Ego::GUI::Label>("Input Settings");
    mainLabel->setPosition(Point2f(20, 20));
    addComponent(mainLabel);

    addInputOption("test");

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

void InputOptionsScreen::addInputOption(const std::string& label)
{
    //Label
    auto name = std::make_shared<Ego::GUI::Label>(label);
    name->setPosition(Point2f(20, 40));
    addComponent(name);

    //Button
    auto inputOption = std::make_shared<Ego::GUI::Button>("N/A");
    inputOption->setPosition(Point2f(20 + 100, 40));
    inputOption->setSize(Vector2f(200, 30));
    inputOption->setOnClickFunction(
    []{
    	//TODO
    });
    addComponent(inputOption);

}

} //GameStates
} //Ego
