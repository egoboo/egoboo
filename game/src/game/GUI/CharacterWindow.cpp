#include "CharacterWindow.hpp"
#include "game/GUI/Label.hpp"
#include "game/Entities/_Include.hpp"

CharacterWindow::CharacterWindow(const std::shared_ptr<Object> &object) : InternalWindow(object->getName()),
    _character(object)
{
    setSize(320, 240);

    std::stringstream buffer;

    if(_character->isAlive())
    {
        //Level
        buffer << std::to_string(_character->getExperienceLevel());
        switch(_character->getExperienceLevel())
        {
            case 1:
                buffer << "st";
            break;

            case 2:
                buffer << "nd";
            break;

            case 3:
                buffer << "rd";
            break;

            default:
                buffer << "th";
            break;
        }
        buffer << " level ";

        //Gender
        if     (_character->getGender() == GENDER_MALE)   buffer << "male ";
        else if(_character->getGender() == GENDER_FEMALE) buffer << "female ";        
    }
    else
    {
        buffer << "Dead ";
    }

    //Class
    buffer << _character->getProfile()->getClassName();

    std::shared_ptr<Label> classLevelLabel = std::make_shared<Label>(buffer.str());
    classLevelLabel->setPosition(5, 32);
    addComponent(classLevelLabel);

    //Statistics
    std::shared_ptr<Label> strengthLabel = std::make_shared<Label>("Strength: ");
    strengthLabel->setPosition(classLevelLabel->getX(), classLevelLabel->getY() + classLevelLabel->getHeight());
    strengthLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(strengthLabel);

    std::shared_ptr<Label> dexterityLabel = std::make_shared<Label>("Dexterity: ");
    dexterityLabel->setPosition(strengthLabel->getX(), strengthLabel->getY() + strengthLabel->getHeight());
    dexterityLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(dexterityLabel);

    std::shared_ptr<Label> wisdomLabel = std::make_shared<Label>("Wisdom: ");
    wisdomLabel->setPosition(dexterityLabel->getX(), dexterityLabel->getY() + dexterityLabel->getHeight());
    wisdomLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(wisdomLabel);

    std::shared_ptr<Label> intelligenceLabel = std::make_shared<Label>("Intellect: ");
    intelligenceLabel->setPosition(wisdomLabel->getX(), wisdomLabel->getY() + wisdomLabel->getHeight());
    intelligenceLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(intelligenceLabel);

    //Defences
    std::shared_ptr<Label> pokeLabel = std::make_shared<Label>("Poke: ");
    pokeLabel->setPosition(classLevelLabel->getX() + getWidth()/2, classLevelLabel->getY() + classLevelLabel->getHeight());
    pokeLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(pokeLabel);

    std::shared_ptr<Label> slashLabel = std::make_shared<Label>("Slash: ");
    slashLabel->setPosition(pokeLabel->getX(), pokeLabel->getY() + pokeLabel->getHeight());
    slashLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(slashLabel);

    std::shared_ptr<Label> crushLabel = std::make_shared<Label>("Crush: ");
    crushLabel->setPosition(slashLabel->getX(), slashLabel->getY() + slashLabel->getHeight());
    crushLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(crushLabel);

    std::shared_ptr<Label> fireLabel = std::make_shared<Label>("Fire: ");
    fireLabel->setPosition(crushLabel->getX(), crushLabel->getY() + crushLabel->getHeight());
    fireLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(fireLabel);

    std::shared_ptr<Label> zapLabel = std::make_shared<Label>("Zap: ");
    zapLabel->setPosition(fireLabel->getX(), fireLabel->getY() + fireLabel->getHeight());
    zapLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(zapLabel);

    std::shared_ptr<Label> iceLabel = std::make_shared<Label>("Ice: ");
    iceLabel->setPosition(zapLabel->getX(), zapLabel->getY() + zapLabel->getHeight());
    iceLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(iceLabel);

    std::shared_ptr<Label> evilLabel = std::make_shared<Label>("Evil: ");
    evilLabel->setPosition(iceLabel->getX(), iceLabel->getY() + iceLabel->getHeight());
    evilLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(evilLabel);

    std::shared_ptr<Label> holyLabel = std::make_shared<Label>("Holy: ");
    holyLabel->setPosition(evilLabel->getX(), evilLabel->getY() + evilLabel->getHeight());
    holyLabel->setFont(_gameEngine->getUIManager()->getGameFont());
    addComponent(holyLabel);
}
