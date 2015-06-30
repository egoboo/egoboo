#include "CharacterWindow.hpp"
#include "game/GUI/Label.hpp"
#include "game/Entities/_Include.hpp"

static const int LINE_SPACING_OFFSET = 5; //To make space between lines less

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
    classLevelLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    classLevelLabel->setPosition(5, 32);
    addComponent(classLevelLabel);

    //Attribute Labels
    int maxWidth = 0;

    std::shared_ptr<Label> attributeLabel = std::make_shared<Label>("ATTRIBUTES");
    attributeLabel->setPosition(classLevelLabel->getX(), classLevelLabel->getY() + classLevelLabel->getHeight() + 5);
    attributeLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(attributeLabel);    

    std::shared_ptr<Label> strengthLabel = std::make_shared<Label>("Strength: ");
    strengthLabel->setPosition(attributeLabel->getX(), attributeLabel->getY() + attributeLabel->getHeight()-LINE_SPACING_OFFSET);
    strengthLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(strengthLabel);
    maxWidth = std::max(maxWidth, strengthLabel->getWidth()+10);

    std::shared_ptr<Label> dexterityLabel = std::make_shared<Label>("Dexterity: ");
    dexterityLabel->setPosition(strengthLabel->getX(), strengthLabel->getY() + strengthLabel->getHeight()-LINE_SPACING_OFFSET);
    dexterityLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(dexterityLabel);
    maxWidth = std::max(maxWidth, dexterityLabel->getWidth()+10);

    std::shared_ptr<Label> wisdomLabel = std::make_shared<Label>("Wisdom: ");
    wisdomLabel->setPosition(dexterityLabel->getX(), dexterityLabel->getY() + dexterityLabel->getHeight()-LINE_SPACING_OFFSET);
    wisdomLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(wisdomLabel);
    maxWidth = std::max(maxWidth, wisdomLabel->getWidth()+10);

    std::shared_ptr<Label> intelligenceLabel = std::make_shared<Label>("Intellect: ");
    intelligenceLabel->setPosition(wisdomLabel->getX(), wisdomLabel->getY() + wisdomLabel->getHeight()-LINE_SPACING_OFFSET);
    intelligenceLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(intelligenceLabel);
    maxWidth = std::max(maxWidth, intelligenceLabel->getWidth()+10);

    //Now attribute Values
    std::shared_ptr<Label> strengthValue = std::make_shared<Label>(std::to_string(_character->getStrenght()));
    strengthValue->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    strengthValue->setPosition(strengthLabel->getX() + maxWidth, strengthLabel->getY());
    addComponent(strengthValue);

    std::shared_ptr<Label> dexterityValue = std::make_shared<Label>(std::to_string(_character->getDexterity()));
    dexterityValue->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    dexterityValue->setPosition(dexterityLabel->getX() + maxWidth, dexterityLabel->getY());
    addComponent(dexterityValue);

    std::shared_ptr<Label> wisdomValue = std::make_shared<Label>(std::to_string(_character->getWisdom()));
    wisdomValue->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    wisdomValue->setPosition(wisdomLabel->getX() + maxWidth, wisdomLabel->getY());
    addComponent(wisdomValue);

    std::shared_ptr<Label> intelligenceValue = std::make_shared<Label>(std::to_string(_character->getIntelligence()));
    intelligenceValue->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    intelligenceValue->setPosition(intelligenceLabel->getX() + maxWidth, intelligenceLabel->getY());
    addComponent(intelligenceValue);

    //Defences
    std::shared_ptr<Label> defenceLabel = std::make_shared<Label>("DEFENCES");
    defenceLabel->setPosition(classLevelLabel->getX() + getWidth()/2, classLevelLabel->getY() + classLevelLabel->getHeight());
    defenceLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(defenceLabel);    

    std::shared_ptr<Label> pokeLabel = std::make_shared<Label>("Poke: ");
    pokeLabel->setPosition(defenceLabel->getX(), defenceLabel->getY() + defenceLabel->getHeight()-LINE_SPACING_OFFSET);
    pokeLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(pokeLabel);

    std::shared_ptr<Label> slashLabel = std::make_shared<Label>("Slash: ");
    slashLabel->setPosition(pokeLabel->getX(), pokeLabel->getY() + pokeLabel->getHeight()-LINE_SPACING_OFFSET);
    slashLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(slashLabel);

    std::shared_ptr<Label> crushLabel = std::make_shared<Label>("Crush: ");
    crushLabel->setPosition(slashLabel->getX(), slashLabel->getY() + slashLabel->getHeight()-LINE_SPACING_OFFSET);
    crushLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(crushLabel);

    std::shared_ptr<Label> fireLabel = std::make_shared<Label>("Fire: ");
    fireLabel->setPosition(crushLabel->getX(), crushLabel->getY() + crushLabel->getHeight()-LINE_SPACING_OFFSET);
    fireLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(fireLabel);

    std::shared_ptr<Label> zapLabel = std::make_shared<Label>("Zap: ");
    zapLabel->setPosition(fireLabel->getX(), fireLabel->getY() + fireLabel->getHeight()-LINE_SPACING_OFFSET);
    zapLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(zapLabel);

    std::shared_ptr<Label> iceLabel = std::make_shared<Label>("Ice: ");
    iceLabel->setPosition(zapLabel->getX(), zapLabel->getY() + zapLabel->getHeight()-LINE_SPACING_OFFSET);
    iceLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(iceLabel);

    std::shared_ptr<Label> evilLabel = std::make_shared<Label>("Evil: ");
    evilLabel->setPosition(iceLabel->getX(), iceLabel->getY() + iceLabel->getHeight()-LINE_SPACING_OFFSET);
    evilLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(evilLabel);

    std::shared_ptr<Label> holyLabel = std::make_shared<Label>("Holy: ");
    holyLabel->setPosition(evilLabel->getX(), evilLabel->getY() + evilLabel->getHeight()-LINE_SPACING_OFFSET);
    holyLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(holyLabel);
}
