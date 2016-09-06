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
#include "game/GUI/Layout.hpp"
#include "game/GUI/JoinBounds.hpp"

namespace Ego {
namespace GUI {

static const float WindowWidth = 420;
static const float BorderPadding = 32;
static const float Spacing = 8;
static const int LINE_SPACING_OFFSET = -2; //To make space between lines less

CharacterWindow::CharacterWindow(const std::shared_ptr<Object> &object) : InternalWindow(object->getName()),
    _character(object),
    _levelUpButton(nullptr),
    _levelUpWindow(),
    _characterStatisticsTab(),
    _knownPerksTab(),
    _activeEnchantsTab() {
    setSize(Vector2f(WindowWidth + 2 * BorderPadding, 400));

    const float xoffset = BorderPadding;
    const float yoffset = _titleBar->getBounds().getSize().y() + Spacing;

    _characterStatisticsTab = std::make_shared<Tab>("Character");
    _characterStatisticsTab->setWidth(WindowWidth);
    _characterStatisticsTab->setPosition(Point2f(xoffset, yoffset));
    addComponent(_characterStatisticsTab);

    _knownPerksTab = std::make_shared<Tab>("Perks");
    _knownPerksTab->setWidth(WindowWidth);
    _knownPerksTab->setPosition(Point2f(xoffset, yoffset));
    addComponent(_knownPerksTab);

    _activeEnchantsTab = std::make_shared<Tab>("Enchants");
    _activeEnchantsTab->setWidth(WindowWidth);
    _activeEnchantsTab->setPosition(Point2f(xoffset, yoffset));
    addComponent(_activeEnchantsTab);

    buildCharacterStatisticTab(_characterStatisticsTab);
    buildKnownPerksTab(_knownPerksTab);
    buildActiveEnchantsTab(_activeEnchantsTab);

    _knownPerksTab->setEnabled(false);
    _knownPerksTab->setVisible(false);
    _activeEnchantsTab->setEnabled(false);
    _activeEnchantsTab->setVisible(false);
    _characterStatisticsTab->setEnabled(true);
    _characterStatisticsTab->setVisible(true);

    JoinBounds joinBounds;
    joinBounds({_activeEnchantsTab, _characterStatisticsTab, _knownPerksTab});
    float y = yoffset + joinBounds({_activeEnchantsTab, _characterStatisticsTab, _knownPerksTab}).getSize().y();

    //float y = getHeight();
    const float tabButtonWidth = 120;
    const float tabButtonHeight = 30;
    // Character tab button
    auto characterTab = std::make_shared<Button>(_characterStatisticsTab->getTitle());
    characterTab->setSize(Vector2f(tabButtonWidth, tabButtonHeight));
    characterTab->setPosition(Point2f(BorderPadding, y));
    characterTab->setParent(this);
    addComponent(characterTab);
    characterTab->setOnClickFunction([this] {
        _activeEnchantsTab->setEnabled(false);
        _activeEnchantsTab->setVisible(false);
        _knownPerksTab->setEnabled(false);
        _knownPerksTab->setVisible(false);
        _characterStatisticsTab->setEnabled(true);
        _characterStatisticsTab->setVisible(true);
    });



    // Perks tab button
    auto perksTab = std::make_shared<Button>(_knownPerksTab->getTitle());
    perksTab->setSize(Vector2f(tabButtonWidth, tabButtonHeight));
    perksTab->setPosition(Point2f(BorderPadding + (tabButtonWidth + 5) * 1, y));
    perksTab->setParent(this);
    addComponent(perksTab);
    perksTab->setOnClickFunction([this] {
        _characterStatisticsTab->setEnabled(false);
        _characterStatisticsTab->setVisible(false);
        _activeEnchantsTab->setEnabled(false);
        _activeEnchantsTab->setVisible(false);
        _knownPerksTab->setEnabled(true);
        _knownPerksTab->setVisible(true);
    });

    // Active enchants tab button
    auto enchantsTab = std::make_shared<Button>(_activeEnchantsTab->getTitle());
    enchantsTab->setSize(Vector2f(tabButtonWidth, tabButtonHeight));
    enchantsTab->setPosition(Point2f(BorderPadding + (tabButtonWidth + 5) * 2, y));
    enchantsTab->setParent(this);
    addComponent(enchantsTab);
    enchantsTab->setOnClickFunction([this] {
        _characterStatisticsTab->setEnabled(false);
        _characterStatisticsTab->setVisible(false);
        _knownPerksTab->setEnabled(false);
        _knownPerksTab->setVisible(false);
        _activeEnchantsTab->setEnabled(true);
        _activeEnchantsTab->setVisible(true);
    });
    setHeight(y + tabButtonHeight + BorderPadding);
}

CharacterWindow::~CharacterWindow() {
    //If the character is a local player, then we no longer consume that players input events
    if (_character->isPlayer()) {
        _currentModule->getPlayer(_character->is_which_player)->setInventoryMode(false);
    }

    //If the level up window is open, close it as well
    std::shared_ptr<InternalWindow> window = _levelUpWindow.lock();
    if (window) {
        window->destroy();
    }
}

int CharacterWindow::addAttributeLabel(std::shared_ptr<Tab> target, const Point2f& position, const Attribute::AttributeType type) {
    //Label
    std::shared_ptr<Label> label = std::make_shared<Label>(Attribute::toString(type) + ":");
    label->setPosition(position);
    label->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    label->setParent(target.get());
    target->addComponent(label);

    //Value
    std::shared_ptr<Label> value = std::make_shared<Label>("");

    //Special case regeneration values, use decimals
    if (type == Attribute::MANA_REGEN || type == Attribute::LIFE_REGEN) {
        std::stringstream valueString;
        valueString << std::setprecision(2) << std::fixed << _character->getAttribute(type);
        value->setText(valueString.str());
    } else {
        value->setText(std::to_string(std::lround(_character->getAttribute(type))));
    }

    value->setPosition(Point2f(target->getWidth() / 2 - 20, label->getY()));
    value->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    value->setParent(target.get());
    target->addComponent(value);

    return label->getHeight() - LINE_SPACING_OFFSET;
}

int CharacterWindow::addResistanceLabel(std::shared_ptr<Tab> target, const Point2f& position, const DamageType type) {
    //Enum to string
    std::string damageName;
    switch (type) {
        case DAMAGE_POKE:  damageName = "Poke"; break;
        case DAMAGE_SLASH: damageName = "Slash"; break;
        case DAMAGE_CRUSH: damageName = "Crush"; break;
        case DAMAGE_FIRE:  damageName = "Fire"; break;
        case DAMAGE_ZAP:   damageName = "Zap"; break;
        case DAMAGE_ICE:   damageName = "Ice"; break;
        case DAMAGE_EVIL:  damageName = "Evil"; break;
        case DAMAGE_HOLY:  damageName = "Holy"; break;
        default: throw Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }

    //Label
    std::shared_ptr<Label> label = std::make_shared<Label>(damageName + ":");
    label->setPosition(position);
    label->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    label->setColour(Math::Colour4f(DamageType_getColour(type), 1.0f));
    label->setParent(target.get());
    target->addComponent(label);

    //Value
    std::shared_ptr<Label> value = std::make_shared<Label>(std::to_string(std::lround(_character->getRawDamageResistance(type))));
    value->setPosition(label->getPosition() + Vector2f(50, 0));
    value->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    value->setColour(Math::Colour4f(DamageType_getColour(type), 1.0f));
    value->setParent(target.get());
    target->addComponent(value);

    //Percent
    std::shared_ptr<Label> percent = std::make_shared<Label>("(" + std::to_string(std::lround(_character->getDamageReduction(type) * 100)) + "%)");
    percent->setPosition(label->getPosition() + Vector2f(75, 0));
    percent->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    percent->setColour(Math::Colour4f(DamageType_getColour(type), 1.0f));
    percent->setParent(target.get());
    target->addComponent(percent);

    return label->getHeight() - LINE_SPACING_OFFSET;
}

void CharacterWindow::drawContainer(DrawingContext& drawingContext) {
    this->InternalWindow::drawContainer(drawingContext);
}

void CharacterWindow::drawAll(DrawingContext& drawingContext) {
    // Render the container itself.
    drawContainer(drawingContext);

    // Draw reach GUI component.
    _gameEngine->getUIManager()->beginRenderUI();
    for (const std::shared_ptr<Component> component : iterator()) {
        if (!component->isVisible()) continue;  // Ignore hidden/destroyed components.
        component->draw(drawingContext);
    }
    _gameEngine->getUIManager()->endRenderUI();
}

void CharacterWindow::draw(DrawingContext& drawingContext) {
    drawAll(drawingContext);
}

bool CharacterWindow::notifyMouseMoved(const Events::MouseMovedEventArgs& e) {
    //Make level up button visible if needed
    if (_character->isPlayer()) {
        _levelUpButton->setVisible(_levelUpWindow.expired() && _currentModule->getPlayer(_character->is_which_player)->hasUnspentLevel());
    }

    return InternalWindow::notifyMouseMoved(e);
}

void CharacterWindow::buildCharacterStatisticTab(std::shared_ptr<Tab> target) {
    int xPos, yPos = 0;

    // draw the character's main icon
    std::shared_ptr<Image> characterIcon = std::make_shared<Image>(_character->getProfile()->getIcon(_character->skin));
    characterIcon->setPosition(Point2f(0, 0));
    characterIcon->setSize(Vector2f(32, 32));
    target->addComponent(characterIcon);

    std::stringstream buffer;

    if (_character->isAlive()) {
        //Level
        buffer << std::to_string(_character->getExperienceLevel());
        switch (_character->getExperienceLevel()) {
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
        if (_character->getGender() == Gender::Male)   buffer << "male ";
        else if (_character->getGender() == Gender::Female) buffer << "female ";
    } else {
        buffer << "Dead ";
    }

    //Class
    buffer << _character->getProfile()->getClassName();

    std::shared_ptr<Label> classLevelLabel = std::make_shared<Label>(buffer.str());
    classLevelLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    classLevelLabel->setPosition(characterIcon->getPosition() + Vector2f(characterIcon->getWidth() + 5, 0));
    classLevelLabel->setParent(target.get());
    target->addComponent(classLevelLabel);

    //Attributes
    std::shared_ptr<Label> attributeLabel = std::make_shared<Label>("ATTRIBUTES");
    attributeLabel->setPosition(characterIcon->getPosition() + Vector2f(0, characterIcon->getHeight() + 5));
    attributeLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    attributeLabel->setParent(target.get());
    target->addComponent(attributeLabel);

    yPos = attributeLabel->getY() + attributeLabel->getHeight() - LINE_SPACING_OFFSET;
    for (int i = 0; i < Attribute::NR_OF_PRIMARY_ATTRIBUTES; ++i) {
        yPos += addAttributeLabel(target, Point2f(attributeLabel->getX(), yPos), static_cast<Attribute::AttributeType>(i));
    }

    //Defences
    std::shared_ptr<Label> defenceLabel = std::make_shared<Label>("DEFENCES");
    defenceLabel->setPosition(Point2f(target->getWidth() / 2 + 20, attributeLabel->getY()));
    defenceLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    defenceLabel->setParent(target.get());
    target->addComponent(defenceLabel);

    yPos = defenceLabel->getY() + defenceLabel->getHeight() - LINE_SPACING_OFFSET;
    for (int type = 0; type < DAMAGE_COUNT; ++type) {
        yPos += addResistanceLabel(target, Point2f(defenceLabel->getX(), yPos), static_cast<DamageType>(type));
    }

    //Inventory
    const int slotSize = (target->getWidth() - 15 - _character->getInventory().getMaxItems() * 5) / _character->getInventory().getMaxItems();
    xPos = 0;
    yPos += 5;
    std::vector<std::shared_ptr<Component>> slots;
    for (size_t i = 0; i < _character->getInventory().getMaxItems(); ++i) {
        std::shared_ptr<InventorySlot> slot = std::make_shared<InventorySlot>(_character->getInventory(), i, _character->isPlayer() ? _currentModule->getPlayer(_character->is_which_player) : nullptr);
        slot->setSize(Vector2f(slotSize, slotSize));
        slot->setParent(target.get());
        target->addComponent(slot);
        slots.push_back(slot);
    }
    LayoutRows layoutRows(Point2f(xPos, yPos), target->getWidth(), 5.0f, 5.0f);
    layoutRows(slots);

    //If the character is a local player, then we consume that players input events for inventory managment
    if (_character->isPlayer()) {
        _currentModule->getPlayer(_character->is_which_player)->setInventoryMode(true);
    }

    JoinBounds joinBounds;
    auto bounds = joinBounds(slots.cbegin(), slots.cend());

    yPos = bounds.getMax().y() + 5;
    //LevelUp button
    if (_character->isPlayer()) {
        _levelUpButton = std::make_shared<Button>("LEVEL UP");
        _levelUpButton->setSize(Vector2f(120, 30));
        _levelUpButton->setPosition(Point2f(target->getWidth() / 2 - _levelUpButton->getWidth() / 2,
                                            yPos));
        _levelUpButton->setOnClickFunction(
            [this]() {
            std::shared_ptr<LevelUpWindow> window = std::make_shared<LevelUpWindow>(_character);
            getParent()->addComponent(window);
            //destroy();
            _levelUpWindow = window;
            _levelUpButton->setVisible(false);
        }
        );
        _levelUpButton->setParent(target.get());
        target->addComponent(_levelUpButton);

        //Make level up button visible if needed
        _levelUpButton->setVisible(_currentModule->getPlayer(_character->is_which_player)->hasUnspentLevel());
    }
    float newHeight = yPos + 30 + BorderPadding;
    target->setHeight(newHeight);
}

void CharacterWindow::buildKnownPerksTab(std::shared_ptr<Tab> target) {
    //List of perks known
    std::shared_ptr<ScrollableList> perksKnown = std::make_shared<ScrollableList>();
    perksKnown->setSize(Vector2f(getWidth() - 60, getHeight() * 0.60f));
    perksKnown->setPosition(Point2f(0, 0 + 8));
    perksKnown->setParent(target.get());
    target->addComponent(perksKnown);

    //Perk icon
    std::shared_ptr<Image> perkIcon = std::make_shared<Image>();
    perkIcon->setPosition(Point2f(10, getHeight() - 80));
    perkIcon->setSize(Vector2f(64, 64));
    perkIcon->setVisible(false);
    perkIcon->setParent(target.get());
    target->addComponent(perkIcon);

    //Perk Name
    std::shared_ptr<Label> newPerkLabel = std::make_shared<Label>("No Perk Selected");
    newPerkLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    newPerkLabel->setPosition(Point2f(20, getHeight() - perkIcon->getHeight() - 40));
    newPerkLabel->setColour(Math::Colour4f::yellow());
    newPerkLabel->setParent(target.get());
    target->addComponent(newPerkLabel);

    //Perk description
    std::shared_ptr<Label> perkDescription = std::make_shared<Label>("Select a perk to view details...");
    perkDescription->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    perkDescription->setPosition(Point2f(perkIcon->getX() + perkIcon->getWidth(), newPerkLabel->getY() + newPerkLabel->getHeight()));
    perkDescription->setParent(target.get());
    target->addComponent(perkDescription);

    //Make list of all perks that this character knows
    for (size_t i = 0; i < Perks::NR_OF_PERKS; ++i) {
        const Perks::PerkID perkID = static_cast<Perks::PerkID>(i);

        //Do we know it?
        if (_character->hasPerk(perkID)) {
            const Perks::Perk &perk = Perks::PerkHandler::get().getPerk(perkID);

            std::shared_ptr<IconButton> perkButton = std::make_shared<IconButton>(perk.getName(), perk.getIcon());
            perkButton->setSize(Vector2f(perksKnown->getWidth() - 50, 32));
            
            perkButton->setIconTint(perk.getColour());

            //Display detailed info about this perk if clicked
            perkButton->setOnClickFunction([&perk, perkIcon, perkDescription] {
                perkIcon->setVisible(true);
                perkIcon->setImage(perk.getIcon().getFilePath());
                perkIcon->setTint(perk.getColour());
                perkDescription->setText(perk.getDescription());
            });
            perkButton->setParent(perksKnown.get());
            perksKnown->addComponent(perkButton);
        }
    }

    perksKnown->forceUpdate();
}

void CharacterWindow::buildActiveEnchantsTab(std::shared_ptr<Tab> target) {
    //List of active enchants
    std::shared_ptr<ScrollableList> activeEnchants = std::make_shared<ScrollableList>();
    activeEnchants->setSize(Vector2f((target->getWidth() - 20) / 2, getHeight() * 0.60f));
    activeEnchants->setPosition(Point2f(10, 40));
    activeEnchants->setParent(target.get());
    target->addComponent(activeEnchants);

    //Enchant Name
    std::shared_ptr<Label> enchantName = std::make_shared<Label>("No Enchant Selected");
    enchantName->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    enchantName->setPosition(Point2f(activeEnchants->getX() + activeEnchants->getWidth() + 5, activeEnchants->getY()));
    enchantName->setColour(Math::Colour4f::yellow());
    enchantName->setParent(target.get());
    target->addComponent(enchantName);

    //List of effects a given enchant has
    std::shared_ptr<ScrollableList> enchantEffects = std::make_shared<ScrollableList>();
    enchantEffects->setSize(Vector2f((target->getWidth() - 20) / 2, activeEnchants->getHeight() - enchantName->getHeight()));
    enchantEffects->setPosition(Point2f(enchantName->getX(), enchantName->getY() + enchantName->getHeight()));
    enchantEffects->setParent(target.get());
    target->addComponent(enchantEffects);

    //Count number of unique enchants and merge all others
    std::unordered_map<std::string, std::vector<std::shared_ptr<Enchantment>>> enchantCount;
    for (const std::shared_ptr<Enchantment> &enchant : _character->getActiveEnchants()) {
        if (!enchant->getProfile()->getEnchantName().empty()) {
            enchantCount[enchant->getProfile()->getEnchantName()].push_back(enchant);
        } else {
            enchantCount["Miscellaneous"].push_back(enchant);
        }
    }

    for (const auto& element : enchantCount) {
        std::string prefix = element.second.size() > 1 ? "x" + std::to_string(element.second.size()) + " " : "";

        //Replace underscores with spaces
        std::string name = element.first;
        std::transform(name.begin(), name.end(), name.begin(), [](char ch) {
            return ch == '_' ? ' ' : ch;
        });

        std::shared_ptr<Button> button = std::make_shared<Button>(prefix + name);
        button->setSize(Vector2f(activeEnchants->getWidth() - 50, activeEnchants->getHeight() / 6));
        button->setParent(activeEnchants.get());
        activeEnchants->addComponent(button);
        button->setOnClickFunction([this, element, name, enchantName, enchantEffects] {
            enchantName->setText(name);
            describeEnchantEffects(element.second, enchantEffects);

        });
    }
}

void CharacterWindow::describeEnchantEffects(const std::vector<std::shared_ptr<Enchantment>> &enchantments, std::shared_ptr<ScrollableList> list) {
    //Accumulate effects
    std::unordered_map<Attribute::AttributeType, float> effects;
    for (const std::shared_ptr<Enchantment> &enchant : enchantments) {
        for (const EnchantModifier& modifier : enchant->getModifiers()) {
            effects[modifier._type] += modifier._value;
        }
    }

    //Now describe each effect
    list->clearComponents();
    for (const auto &element : effects) {
        //No effect?
        if (std::abs(element.second) < std::numeric_limits<float>::epsilon()) continue;

        //Add a label description
        try {
            std::ostringstream out;
            out << Attribute::toString(element.first) << std::setprecision(2) << ": " << element.second;

            std::shared_ptr<Label> label = std::make_shared<Label>(out.str());
            label->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
            label->setColour(element.second > 0 ? Math::Colour4f::green() : Math::Colour4f::red());
            list->addComponent(label);
        } catch (Id::UnhandledSwitchCaseException &ex) {
            //Simply ignore effects that cannot be translated into a description
            continue;
        }
    }
    list->forceUpdate();
}

} // namespace GUI
} // namespace Ego
