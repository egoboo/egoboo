#include "CharacterWindow.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/InventorySlot.hpp"
#include "game/GUI/LevelUpWindow.hpp"
#include "game/Entities/_Include.hpp"
#include "game/player.h"

namespace Ego
{
namespace GUI
{

static const int LINE_SPACING_OFFSET = 5; //To make space between lines less

CharacterWindow::CharacterWindow(const std::shared_ptr<Object> &object) : InternalWindow(object->getName()),
    _character(object),
    _levelUpButton(nullptr),
    _levelUpWindow()
{
    int yPos = 0;

    setSize(340, 320);

    // draw the character's main icon
    std::shared_ptr<Image> characterIcon = std::make_shared<Image>(_character->getProfile()->getIcon(_character->skin));
    characterIcon->setPosition(5, 32);
    characterIcon->setSize(32, 32);
    addComponent(characterIcon);

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
    classLevelLabel->setPosition(characterIcon->getX() + characterIcon->getWidth() + 5, characterIcon->getY());
    addComponent(classLevelLabel);

    //Attributes
    std::shared_ptr<Label> attributeLabel = std::make_shared<Label>("ATTRIBUTES");
    attributeLabel->setPosition(characterIcon->getX(), characterIcon->getY() + characterIcon->getHeight() + 5);
    attributeLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(attributeLabel);

    yPos = attributeLabel->getY() + attributeLabel->getHeight() - LINE_SPACING_OFFSET;
    for(int i = 0; i < Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES; ++i) {
        yPos += addAttributeLabel(attributeLabel->getX(), yPos, static_cast<Ego::Attribute::AttributeType>(i));
    }

    //Defences
    std::shared_ptr<Label> defenceLabel = std::make_shared<Label>("DEFENCES");
    defenceLabel->setPosition(getX() + getWidth()/2 + 20, attributeLabel->getY());
    defenceLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(defenceLabel);    

    yPos = defenceLabel->getY() + defenceLabel->getHeight() - LINE_SPACING_OFFSET;
    for(int type = 0; type < DAMAGE_COUNT; ++type) {
        yPos += addResistanceLabel(defenceLabel->getX(), yPos, static_cast<DamageType>(type));
    }

    //Inventory
    const int slotSize = (getWidth() - 15 - _character->getInventory().getMaxItems()*5) / _character->getInventory().getMaxItems();
    int xPos = 10;
    yPos += 5;
    for(size_t i = 0; i < _character->getInventory().getMaxItems(); ++i) {
        std::shared_ptr<InventorySlot> slot = std::make_shared<InventorySlot>(_character->getInventory(), i, _character->is_which_player);
        slot->setSize(slotSize, slotSize);
        slot->setPosition(xPos, yPos);
        xPos += slot->getWidth() + 5;
        addComponent(slot);

        //Newline?
        if(xPos + slot->getWidth() > getWidth()) {
            yPos += slot->getHeight() + 5;
            xPos = 10;
        }
    }

    //If the character is a local player, then we consume that players input events for inventory managment
    if(_character->isPlayer()) {
        PlaStack.get_ptr(_character->is_which_player)->inventoryMode = true;
    }

    //LevelUp button
    if(_character->isPlayer())
    {
        setSize(getWidth(), getHeight() + 40);

        _levelUpButton = std::make_shared<Button>("LEVEL UP");
        _levelUpButton->setSize(120, 30);
        _levelUpButton->setPosition(getWidth()/2 - _levelUpButton->getWidth()/2, getHeight() - _levelUpButton->getHeight() - 15);
        _levelUpButton->setOnClickFunction(
            [this](){
                std::shared_ptr<LevelUpWindow> window = std::make_shared<LevelUpWindow>(_character);
                getParent()->addComponent(window);
                //destroy();
                _levelUpWindow = window;
                _levelUpButton->setVisible(false);
            }
        );
        addComponent(_levelUpButton);

        //Make level up button visible if needed
        _levelUpButton->setVisible(PlaStack.get_ptr(_character->is_which_player)->_unspentLevelUp);
    }
}

CharacterWindow::~CharacterWindow()
{
    //If the character is a local player, then we no longer consume that players input events
    if(_character->isPlayer()) {
        PlaStack.get_ptr(_character->is_which_player)->inventoryMode = false;
    }

    //If the level up window is open, close it as well
    std::shared_ptr<InternalWindow> window = _levelUpWindow.lock();
    if(window) {
        window->destroy();
    }
}

int CharacterWindow::addAttributeLabel(const int x, const int y, const Ego::Attribute::AttributeType type)
{
    //Label
    std::shared_ptr<Label> label = std::make_shared<Label>(Ego::Attribute::toString(type) + ":");
    label->setPosition(x, y);
    label->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(label);

    //Value
    std::shared_ptr<Label> value = std::make_shared<Label>("");

    //Special case regeneration values, use decimals
    if(type == Ego::Attribute::MANA_REGEN || type == Ego::Attribute::LIFE_REGEN) {
        std::stringstream valueString;
        valueString << std::setprecision(2) << std::fixed << _character->getAttribute(type);
        value->setText(valueString.str());
    }
    else {
        value->setText(std::to_string(std::lround(_character->getAttribute(type))));        
    }

    value->setPosition(getWidth()/2 - 20, label->getY());
    value->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(value);

    return label->getHeight()-LINE_SPACING_OFFSET;
}

int CharacterWindow::addResistanceLabel(const int x, const int y, const DamageType type)
{
    //Enum to string
    std::string damageName;
    switch(type)
    {
        case DAMAGE_POKE:  damageName = "Poke"; break;
        case DAMAGE_SLASH: damageName = "Slash"; break;
        case DAMAGE_CRUSH: damageName = "Crush"; break;
        case DAMAGE_FIRE:  damageName = "Fire"; break;
        case DAMAGE_ZAP:   damageName = "Zap"; break;
        case DAMAGE_ICE:   damageName = "Ice"; break;
        case DAMAGE_EVIL:  damageName = "Evil"; break;
        case DAMAGE_HOLY:  damageName = "Holy"; break;
        default: throw Ego::Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }

    //Label
    std::shared_ptr<Label> label = std::make_shared<Label>(damageName + ":");
    label->setPosition(x, y);
    label->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    label->setColor(Ego::Math::Colour4f(DamageType_getColour(type), 1.0f));
    addComponent(label);

    //Value
    std::shared_ptr<Label> value = std::make_shared<Label>(std::to_string(std::lround(_character->getRawDamageResistance(type))));
    value->setPosition(label->getX() + 50, label->getY());
    value->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    value->setColor(Ego::Math::Colour4f(DamageType_getColour(type), 1.0f));
    addComponent(value);

    //Percent
    std::shared_ptr<Label> percent = std::make_shared<Label>("(" + std::to_string(std::lround(_character->getDamageReduction(type)*100)) + "%)");
    percent->setPosition(label->getX() + 75, label->getY());
    percent->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    percent->setColor(Ego::Math::Colour4f(DamageType_getColour(type), 1.0f));
    addComponent(percent);

    return label->getHeight()-LINE_SPACING_OFFSET;
}

bool CharacterWindow::notifyMouseMoved(const int x, const int y)
{
    //Make level up button visible if needed
    if(_character->isPlayer()) {
        _levelUpButton->setVisible(_levelUpWindow.expired() && PlaStack.get_ptr(_character->is_which_player)->_unspentLevelUp);
    }

    return InternalWindow::notifyMouseMoved(x, y);
}

} //GUI
} //Ego
