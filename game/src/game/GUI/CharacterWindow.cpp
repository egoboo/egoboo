#include "CharacterWindow.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/InventorySlot.hpp"
#include "game/GUI/LevelUpWindow.hpp"
#include "game/GUI/ScrollableList.hpp"
#include "game/GUI/IconButton.hpp"
#include "game/Entities/_Include.hpp"
#include "game/Logic/Player.hpp"

namespace Ego {
namespace GUI {

static const int LINE_SPACING_OFFSET = -2; //To make space between lines less

CharacterWindow::CharacterWindow(const std::shared_ptr<Object> &object) : InternalWindow(object->getName()),
    _character(object),
    _levelUpButton(nullptr),
    _levelUpWindow(),
    _characterStatisticsTab(),
    _knownPerksTab(),
    _activeEnchantsTab()
{
    setSize(420, 370);

    buildCharacterStatisticTab();
    buildKnownPerksTab();
    buildActiveEnchantsTab();

    setComponentList(_characterStatisticsTab);
}

CharacterWindow::~CharacterWindow()
{
    //If the character is a local player, then we no longer consume that players input events
    if(_character->isPlayer()) {
        _currentModule->getPlayer(_character->is_which_player)->setInventoryMode(false);
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
    _characterStatisticsTab.push_back(label);

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
    _characterStatisticsTab.push_back(value);

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
    _characterStatisticsTab.push_back(label);

    //Value
    std::shared_ptr<Label> value = std::make_shared<Label>(std::to_string(std::lround(_character->getRawDamageResistance(type))));
    value->setPosition(label->getX() + 50, label->getY());
    value->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    value->setColor(Ego::Math::Colour4f(DamageType_getColour(type), 1.0f));
    _characterStatisticsTab.push_back(value);

    //Percent
    std::shared_ptr<Label> percent = std::make_shared<Label>("(" + std::to_string(std::lround(_character->getDamageReduction(type)*100)) + "%)");
    percent->setPosition(label->getX() + 75, label->getY());
    percent->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    percent->setColor(Ego::Math::Colour4f(DamageType_getColour(type), 1.0f));
    _characterStatisticsTab.push_back(percent);

    return label->getHeight()-LINE_SPACING_OFFSET;
}

bool CharacterWindow::notifyMouseMoved(const Ego::Events::MouseMovedEventArgs& e)
{
    //Make level up button visible if needed
    if(_character->isPlayer()) {
        _levelUpButton->setVisible(_levelUpWindow.expired() && _currentModule->getPlayer(_character->is_which_player)->hasUnspentLevel());
    }

    return InternalWindow::notifyMouseMoved(e);
}

void CharacterWindow::buildCharacterStatisticTab()
{
    int yPos = 0;

    // draw the character's main icon
    std::shared_ptr<Image> characterIcon = std::make_shared<Image>(_character->getProfile()->getIcon(_character->skin));
    characterIcon->setPosition(5, 32);
    characterIcon->setSize(32, 32);
    _characterStatisticsTab.push_back(characterIcon);
    
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
        if     (_character->getGender() == Gender::Male)   buffer << "male ";
        else if(_character->getGender() == Gender::Female) buffer << "female ";        
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
    _characterStatisticsTab.push_back(classLevelLabel);

    //Attributes
    std::shared_ptr<Label> attributeLabel = std::make_shared<Label>("ATTRIBUTES");
    attributeLabel->setPosition(characterIcon->getX(), characterIcon->getY() + characterIcon->getHeight() + 5);
    attributeLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    _characterStatisticsTab.push_back(attributeLabel);

    yPos = attributeLabel->getY() + attributeLabel->getHeight() - LINE_SPACING_OFFSET;
    for(int i = 0; i < Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES; ++i) {
        yPos += addAttributeLabel(attributeLabel->getX(), yPos, static_cast<Ego::Attribute::AttributeType>(i));
    }

    //Defences
    std::shared_ptr<Label> defenceLabel = std::make_shared<Label>("DEFENCES");
    defenceLabel->setPosition(getX() + getWidth()/2 + 20, attributeLabel->getY());
    defenceLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    _characterStatisticsTab.push_back(defenceLabel);    

    yPos = defenceLabel->getY() + defenceLabel->getHeight() - LINE_SPACING_OFFSET;
    for(int type = 0; type < DAMAGE_COUNT; ++type) {
        yPos += addResistanceLabel(defenceLabel->getX(), yPos, static_cast<DamageType>(type));
    }

    //Inventory
    const int slotSize = (getWidth() - 15 - _character->getInventory().getMaxItems()*5) / _character->getInventory().getMaxItems();
    int xPos = 10;
    yPos += 5;
    for(size_t i = 0; i < _character->getInventory().getMaxItems(); ++i) {
        std::shared_ptr<InventorySlot> slot = std::make_shared<InventorySlot>(_character->getInventory(), i, _character->isPlayer() ? _currentModule->getPlayer(_character->is_which_player) : nullptr);
        slot->setSize(slotSize, slotSize);
        slot->setPosition(xPos, yPos);
        xPos += slot->getWidth() + 5;
        _characterStatisticsTab.push_back(slot);

        //Newline?
        if(xPos + slot->getWidth() > getWidth()) {
            yPos += slot->getHeight() + 5;
            xPos = 10;
        }
    }

    //If the character is a local player, then we consume that players input events for inventory managment
    if(_character->isPlayer()) {
        _currentModule->getPlayer(_character->is_which_player)->setInventoryMode(true);
    }

    //LevelUp button
    if(_character->isPlayer())
    {
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
        _characterStatisticsTab.push_back(_levelUpButton);

        //Make level up button visible if needed
        _levelUpButton->setVisible(_currentModule->getPlayer(_character->is_which_player)->hasUnspentLevel());
    }

    //Perks tab
    std::shared_ptr<Button> perksTab = std::make_shared<Button>("Perks");
    perksTab->setSize(120, 30);
    perksTab->setPosition(20, getHeight() - perksTab->getHeight() - 15);
    perksTab->setOnClickFunction([this]{
        setComponentList(_knownPerksTab);
    });
    _characterStatisticsTab.push_back(perksTab);

    //Active enchants tab
    std::shared_ptr<Button> enchantsTab = std::make_shared<Button>("Enchants");
    enchantsTab->setSize(120, 30);
    enchantsTab->setPosition(getWidth() - enchantsTab->getWidth() - 20, getHeight() - enchantsTab->getHeight() - 15);
    enchantsTab->setOnClickFunction([this]{
        setComponentList(_activeEnchantsTab);
    });
    _characterStatisticsTab.push_back(enchantsTab);
}

void CharacterWindow::buildKnownPerksTab()
{
    //List of perks known
    std::shared_ptr<ScrollableList> perksKnown = std::make_shared<ScrollableList>();
    perksKnown->setSize(getWidth() - 60, getHeight() * 0.60f);
    perksKnown->setPosition(10, 40);
    _knownPerksTab.push_back(perksKnown);

    //Perk icon
    std::shared_ptr<Image> perkIcon = std::make_shared<Image>();
    perkIcon->setPosition(10, getHeight() - 80);
    perkIcon->setSize(64, 64);
    perkIcon->setVisible(false);
    _knownPerksTab.push_back(perkIcon);

    //Perk Name
    std::shared_ptr<Label> newPerkLabel = std::make_shared<Label>("No Perk Selected");
    newPerkLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    newPerkLabel->setPosition(20, getHeight() - perkIcon->getHeight() - 40);
    newPerkLabel->setColor(Ego::Math::Colour4f::yellow());
    _knownPerksTab.push_back(newPerkLabel);

    //Perk description
    std::shared_ptr<Label> perkDescription = std::make_shared<Label>("Select a perk to view details...");
    perkDescription->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    perkDescription->setPosition(perkIcon->getX() + perkIcon->getWidth(), newPerkLabel->getY() + newPerkLabel->getHeight());
    _knownPerksTab.push_back(perkDescription);

    //Make list of all perks that this character knows
    for(size_t i = 0; i < Ego::Perks::NR_OF_PERKS; ++i) {
        const Ego::Perks::PerkID perkID = static_cast<Ego::Perks::PerkID>(i);

        //Do we know it?
        if(_character->hasPerk(perkID)) {
            const Ego::Perks::Perk &perk = Ego::Perks::PerkHandler::get().getPerk(perkID);
            
            std::shared_ptr<IconButton> perkButton = std::make_shared<IconButton>(perk.getName(), perk.getIcon());
            perkButton->setSize(perksKnown->getWidth() - 50, 32);
            perkButton->setIconTint(perk.getColour());

            //Display detailed info about this perk if clicked
            perkButton->setOnClickFunction( [&perk, perkIcon, perkDescription] {
                perkIcon->setVisible(true);
                perkIcon->setImage(perk.getIcon().getFilePath());                
                perkIcon->setTint(perk.getColour());
                perkDescription->setText(perk.getDescription());
            });

            perksKnown->addComponent(perkButton);
        }
    }

    perksKnown->forceUpdate();
}

void CharacterWindow::buildActiveEnchantsTab()
{
    //List of active enchants
    std::shared_ptr<ScrollableList> activeEnchants = std::make_shared<ScrollableList>();
    activeEnchants->setSize( (getWidth()-20) / 2, getHeight() * 0.60f);
    activeEnchants->setPosition(10, 40);
    _activeEnchantsTab.push_back(activeEnchants);

    //Enchant Name
    std::shared_ptr<Label> enchantName = std::make_shared<Label>("No Enchant Selected");
    enchantName->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    enchantName->setPosition(activeEnchants->getX()+activeEnchants->getWidth()+5, activeEnchants->getY());
    enchantName->setColor(Ego::Math::Colour4f::yellow());
    _activeEnchantsTab.push_back(enchantName);

    //List of effects a given enchant has
    std::shared_ptr<ScrollableList> enchantEffects = std::make_shared<ScrollableList>();
    enchantEffects->setSize( (getWidth()-20) /2, activeEnchants->getHeight() - enchantName->getHeight());
    enchantEffects->setPosition(enchantName->getX(), enchantName->getY() + enchantName->getHeight());
    _activeEnchantsTab.push_back(enchantEffects);

    //Count number of unique enchants and merge all others
    std::unordered_map<std::string, std::vector<std::shared_ptr<Ego::Enchantment>>> enchantCount;
    for(const std::shared_ptr<Ego::Enchantment> &enchant : _character->getActiveEnchants()) {
        if(!enchant->getProfile()->getEnchantName().empty()) {
            enchantCount[enchant->getProfile()->getEnchantName()].push_back(enchant);
        }
        else {
            enchantCount["Miscellaneous"].push_back(enchant);
        }
    }

    for(const auto& element : enchantCount) {
        std::string prefix = element.second.size() > 1 ? "x"+std::to_string(element.second.size()) + " " : "";

        //Replace underscores with spaces
        std::string name = element.first ;
        std::transform(name.begin(), name.end(), name.begin(), [](char ch) {
            return ch == '_' ? ' ' : ch;
        });

        std::shared_ptr<Button> button = std::make_shared<Button>(prefix + name);
        button->setSize(activeEnchants->getWidth() - 50, activeEnchants->getHeight() / 6);
        button->setOnClickFunction([this, element, name, enchantName, enchantEffects] {
            enchantName->setText(name);
            describeEnchantEffects(element.second, enchantEffects);

        });
        activeEnchants->addComponent(button);
    }
}

void CharacterWindow::describeEnchantEffects(const std::vector<std::shared_ptr<Ego::Enchantment>> &enchantments, std::shared_ptr<ScrollableList> list)
{
    //Accumulate effects
    std::unordered_map<Ego::Attribute::AttributeType, float> effects;
    for(const std::shared_ptr<Ego::Enchantment> &enchant : enchantments) {
        for(const EnchantModifier& modifier : enchant->getModifiers()) {
            effects[modifier._type] += modifier._value;
        }
    }

    //Now describe each effect
    list->clearComponents();
    for(const auto &element : effects) {
        //No effect?
        if(std::abs(element.second) < std::numeric_limits<float>::epsilon()) continue;

        //Add a label description
        try {
            std::ostringstream out;
            out << Ego::Attribute::toString(element.first) << std::setprecision(2) << ": " << element.second;

            std::shared_ptr<Label> label = std::make_shared<Label>(out.str());
            label->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
            label->setColor(element.second > 0 ? Ego::Math::Colour4f::green() : Ego::Math::Colour4f::red());
            list->addComponent(label);
        }
        catch(Id::UnhandledSwitchCaseException &ex) {
            //Simply ignore effects that cannot be translated into a description
            continue;
        }
    }
    list->forceUpdate();
}

} //GUI
} //Ego
