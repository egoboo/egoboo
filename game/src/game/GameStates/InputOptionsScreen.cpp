#include "game/GameStates/InputOptionsScreen.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Label.hpp"
#include "egolib/InputControl/ControlSettingsFile.hpp"

namespace Ego
{
namespace GameStates
{

InputOptionsScreen::InputOptionsScreen() :
    _maxLabelWidth(0),
    _bindingButtonPosX(0),
    _bindingButtonPosY(0),
    _activeButton(nullptr),
    _activeBinding(Ego::Input::InputDevice::InputButton::COUNT)
{
    //const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    //Main label
    auto mainLabel = std::make_shared<Ego::GUI::Label>("Input Settings");
    mainLabel->setPosition(Point2f(20, 20));
    addComponent(mainLabel);

    _bindingButtonPosX = 20;
    _bindingButtonPosY = mainLabel->getY() + mainLabel->getHeight() + 20;

    addInputOption("Move Up: ", Ego::Input::InputDevice::InputButton::MOVE_UP);
    addInputOption("Move Right: ", Ego::Input::InputDevice::InputButton::MOVE_RIGHT);
    addInputOption("Move Down: ", Ego::Input::InputDevice::InputButton::MOVE_DOWN);
    addInputOption("Move Left: ", Ego::Input::InputDevice::InputButton::MOVE_LEFT);
    addInputOption("Jump: ", Ego::Input::InputDevice::InputButton::JUMP);
    addInputOption("Stealth: ", Ego::Input::InputDevice::InputButton::STEALTH);

    addInputOption("Use Left: ", Ego::Input::InputDevice::InputButton::USE_LEFT);
    addInputOption("Grab Left: ", Ego::Input::InputDevice::InputButton::GRAB_LEFT);

    addInputOption("Use Right: ", Ego::Input::InputDevice::InputButton::USE_RIGHT);
    addInputOption("Grab Right: ", Ego::Input::InputDevice::InputButton::GRAB_RIGHT);
    addInputOption("Inventory: ", Ego::Input::InputDevice::InputButton::INVENTORY);

    addInputOption("Camera Zoom In: ", Ego::Input::InputDevice::InputButton::CAMERA_ZOOM_IN);
    addInputOption("Camera Zoom Out: ", Ego::Input::InputDevice::InputButton::CAMERA_ZOOM_OUT);
    addInputOption("Camera Rotate Left: ", Ego::Input::InputDevice::InputButton::CAMERA_LEFT);
    addInputOption("Camera Rotate Right: ", Ego::Input::InputDevice::InputButton::CAMERA_RIGHT);

    //Shift all buttons to the right of the widest label
    for(std::shared_ptr<Ego::GUI::Component> &component : ComponentContainer::iterator()) {
        std::shared_ptr<Ego::GUI::Button> button = std::dynamic_pointer_cast<Ego::GUI::Button>(component);
        if(button) {
            button->setPosition(Point2f(button->getX() + _maxLabelWidth, button->getY()));
        }
    }

    //Back button
    auto backButton = std::make_shared<Ego::GUI::Button>("Save Settings", SDLK_ESCAPE);
    backButton->setPosition(Point2f(20, SCREEN_HEIGHT-80));
    backButton->setSize(Vector2f(200, 30));
    backButton->setOnClickFunction(
    [this]{
        endState();

        // save the new input settings
        input_settings_save_vfs("/controls.txt");
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

bool InputOptionsScreen::notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEventArgs& e)
{
    if(_activeButton == nullptr) {
        return ComponentContainer::notifyKeyboardKeyPressed(e.getKey());
    }
    else {
        getActiveInputDevice().setInputMapping(_activeBinding, e.getKey());
        _activeButton->setText(getActiveInputDevice().getMappedInputName(_activeBinding));
        _activeBinding = Ego::Input::InputDevice::InputButton::COUNT;
        _activeButton->setEnabled(true);
        _activeButton = nullptr;
        return true;
    }
}


void InputOptionsScreen::addInputOption(const std::string &label, const Ego::Input::InputDevice::InputButton binding)
{
    //Label
    auto name = std::make_shared<Ego::GUI::Label>(label);
    name->setPosition(Point2f(_bindingButtonPosX, _bindingButtonPosY));
    addComponent(name);
    _maxLabelWidth = std::max<float>(_maxLabelWidth, name->getWidth());

    //Button
    std::shared_ptr<Ego::GUI::Button> inputOption = std::make_shared<Ego::GUI::Button>(getActiveInputDevice().getMappedInputName(binding));
    inputOption->setPosition(Point2f(_bindingButtonPosX + 50, _bindingButtonPosY));
    inputOption->setSize(Vector2f(200, 25));
    inputOption->setOnClickFunction(
    [this, inputOption, binding]{
    	inputOption->setText("[Press Key]");

        //Unselect last button if applicable
        if(_activeButton != nullptr) {
            _activeButton->setText(getActiveInputDevice().getMappedInputName(_activeBinding));
            _activeButton->setEnabled(true);
        }

        //Make this the new selected button
        _activeButton = inputOption;
        _activeBinding = binding;
        inputOption->setEnabled(false);
    });
    addComponent(inputOption);

    //Move down to next position and wrap around if out of screen space
    _bindingButtonPosY += name->getHeight();
    if(_bindingButtonPosY >= _gameEngine->getUIManager()->getScreenHeight()) {
        _bindingButtonPosY = 50;
        _bindingButtonPosX += _gameEngine->getUIManager()->getScreenWidth() / 2;
    }
}

Ego::Input::InputDevice& InputOptionsScreen::getActiveInputDevice() const
{
    return Ego::Input::InputDevice::DeviceList[0];
}

} //GameStates
} //Ego
