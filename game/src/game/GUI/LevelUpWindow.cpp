#include "LevelUpWindow.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Image.hpp"
#include "game/Entities/_Include.hpp"
#include "game/player.h"

namespace Ego
{
namespace GUI
{
    static const int PERK_THUMBNAIL_SIZE = 64;

class PerkButton : public GUIComponent
{
public:
    PerkButton(const Ego::Perks::PerkID id) :
        _perk(Ego::Perks::PerkHandler::get().getPerk(id)),
        _mouseOver(false),
        _hoverFadeEffect(0.0f)
    {
        //ctor
    }

    void draw() override
    {
        int shakeEffectX = getX();
        int shakeEffectY = getY();

        //Apply shake effect when mouse is over
        if(_mouseOver) {
            shakeEffectX += Random::next(1, 4) - 2;
            shakeEffectY += Random::next(1, 4) - 2;
        }

        // Draw backdrop
        auto &renderer = Ego::Renderer::get();
		renderer.getTextureUnit().setActivated(nullptr);

        if(_mouseOver && _hoverFadeEffect < 2.0f) {
            _hoverFadeEffect += 2.0f / GameEngine::GAME_TARGET_FPS;
        }
        else if(!_mouseOver && _hoverFadeEffect > 0.0f) {
            _hoverFadeEffect -= 4.0f / GameEngine::GAME_TARGET_FPS;
        }

        renderer.setColour(_perk.getColour().brighter(_hoverFadeEffect));

        struct Vertex
        {
            float x, y;
        };

        auto vb = _gameEngine->getUIManager()->_vertexBuffer;
        Vertex *v = static_cast<Vertex *>(vb->lock());
        v->x = shakeEffectX; v->y = shakeEffectY; v++;
        v->x = shakeEffectX; v->y = shakeEffectY + getHeight(); v++;
        v->x = shakeEffectX + getWidth(); v->y = shakeEffectY + getHeight(); v++;
        v->x = shakeEffectX + getWidth(); v->y = shakeEffectY;
        vb->unlock();
        renderer.render(*vb, Ego::PrimitiveType::Quadriliterals, 0, 4);

        //Icon
        _gameEngine->getUIManager()->drawImage(_perk.getIcon(), shakeEffectX, shakeEffectY, getWidth(), getHeight(), Ego::Math::Colour4f(0, 0, 0, 0.75f));
    }

    bool notifyMouseMoved(const int x, const int y) override
    {
        //Change perk description if mouse moves
        if(_mouseOver) {
            if(!contains(x, y)) {
                _mouseOver = false;

                LevelUpWindow *parentWindow = dynamic_cast<LevelUpWindow*>(getParent());
                if(parentWindow->getCurrentPerk() == _perk.getID()) {
                    parentWindow->setHoverPerk(Ego::Perks::NR_OF_PERKS);
                }

                return true;
            }
        }
        else {
            if(contains(x, y)) {
                _mouseOver = true;

                LevelUpWindow *parentWindow = dynamic_cast<LevelUpWindow*>(getParent());
                parentWindow->setHoverPerk(_perk.getID());

                AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_GUI_HOVER));
                return true;
            }
        }

        return false;
    }

    bool notifyMouseClicked(const int button, const int x, const int y) override
    {
        if(_mouseOver && button == SDL_BUTTON_LEFT)
        {
            static_cast<LevelUpWindow*>(getParent())->doLevelUp(this);
            AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_PERK_SELECT));
            return true;
        }
        return false;
    }

    const Ego::Perks::Perk& getPerk() const
    {
        return _perk;
    }

private:
    const Ego::Perks::Perk& _perk;
    const std::shared_ptr<Label> _descriptionLabel;
    const std::shared_ptr<Label> _perkIncreaseLabel;
    Ego::DeferredTexture _texture;
    bool _mouseOver;
    float _hoverFadeEffect;
};

LevelUpWindow::LevelUpWindow(const std::shared_ptr<Object> &object) : InternalWindow("Level Up!"),
    _character(object),

    _currentPerk(Ego::Perks::NR_OF_PERKS),    
    _descriptionLabel(std::make_shared<Label>()),
    _perkIncreaseLabel(std::make_shared<Label>()),

    _fadeInLabels(),
    _attributeValues(),
    _attributeIncrease(),
    _selectedPerk(nullptr),
    _animationSpeed(0.0f, 0.0f),
    _animationPos(0.0f, 0.0f),
    _attributeRevealTime(0)
{
    setSize(510, 340);

    //Place us in the center of the screen
    setCenterPosition(_gameEngine->getUIManager()->getScreenWidth()/2, _gameEngine->getUIManager()->getScreenHeight()/2);

    // draw the character's main icon
    std::shared_ptr<Image> characterIcon = std::make_shared<Image>(_character->getProfile()->getIcon(_character->skin));
    characterIcon->setPosition(5, 32);
    characterIcon->setSize(32, 32);
    addComponent(characterIcon);

    std::stringstream buffer;

    //Name
    buffer << object->getName() << " is now a ";

    //Level
    buffer << std::to_string(_character->getExperienceLevel() + 1);
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

    //Class
    buffer << _character->getProfile()->getClassName() << '!';

    std::shared_ptr<Label> classLevelLabel = std::make_shared<Label>(buffer.str());
    classLevelLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    classLevelLabel->setPosition(characterIcon->getX() + characterIcon->getWidth() + 5, characterIcon->getY());
    addComponent(classLevelLabel);

    //Perk stuff
    std::shared_ptr<Label> selectPerkLabel = std::make_shared<Label>("Select your Perk:");
    selectPerkLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_DEFAULT));
    selectPerkLabel->setPosition(getWidth()/2 - selectPerkLabel->getWidth()/2, classLevelLabel->getY() + classLevelLabel->getHeight());
    addComponent(selectPerkLabel);

    //Figure out what perks this player can learn
    std::vector<Ego::Perks::PerkID> perkPool = _character->getValidPerks();
    for(size_t i = perkPool.size(); i < 5; ++i) {
        //Ensure at least 5 perks are selectable, add TOUGHNESS as default perk
        perkPool.push_back(Ego::Perks::TOUGHNESS);
    }

    _descriptionLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    _descriptionLabel->setCenterPosition(getX() + getWidth()/2, getHeight() - _descriptionLabel->getHeight(), true);
    addComponent(_descriptionLabel);

    _perkIncreaseLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(_perkIncreaseLabel);

    //No perk by default
    setHoverPerk(Ego::Perks::NR_OF_PERKS);

    //Set random seed for deterministic level ups (no aborting or re-loading game for better results)
    Random::setSeed(_character->getLevelUpSeed());

    //Perk buttons (Jack of All Trades gives +2 perks)
    const size_t NR_OF_PERKS = _character->hasPerk(Ego::Perks::JACK_OF_ALL_TRADES) ? 5 : 3;
    const int PERK_BUTTON_SIZE = (getWidth() - 40 - 10*NR_OF_PERKS) / NR_OF_PERKS;
    for(size_t i = 0; i < NR_OF_PERKS; ++i) {
        //Select a random perk
        const size_t randomIndex = Random::next(perkPool.size()-1);
        std::shared_ptr<PerkButton> perkButton = std::make_shared<PerkButton>(perkPool[randomIndex]);
        perkButton->setSize(PERK_BUTTON_SIZE, PERK_BUTTON_SIZE);
        perkButton->setPosition(20 + i * (perkButton->getWidth()+10), selectPerkLabel->getY() + selectPerkLabel->getHeight());
        addComponent(perkButton);

        _desciptionLabelOffset = perkButton->getY() + perkButton->getHeight();

        //Remove perk from pool
        perkPool.erase(perkPool.begin() + randomIndex);
    }

    //Play level up sound
    AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_LEVELUP));
}

LevelUpWindow::~LevelUpWindow()
{
}

void LevelUpWindow::doLevelUp(PerkButton *selectedPerk)
{
    //Set random seed for deterministic level ups (no aborting or re-loading game for better results)
    Random::setSeed(_character->getLevelUpSeed());

    //Calculate attribute improvements
    std::array<float, Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES> increase;
    for(uint8_t i = 0; i < Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES; ++i) {
        const Ego::Attribute::AttributeType type = static_cast<Ego::Attribute::AttributeType>(i);
        increase[i] = Random::next(_character->getProfile()->getAttributeGain(type));
    }

    //Gain new Perk
    _character->addPerk(selectedPerk->getPerk().getID());

    //Gain attribute bonus from perks
    increase[selectedPerk->getPerk().getType()] += 1.0f;

    //Some perks give flat attribute bonuses
    switch(selectedPerk->getPerk().getID())
    {
        case Ego::Perks::TOUGHNESS:
            increase[Ego::Attribute::MAX_LIFE] += 2.0f;
        break;

        case Ego::Perks::SOLDIERS_FORTITUDE:
            increase[Ego::Attribute::LIFE_REGEN] += 0.15f;
        break;

        case Ego::Perks::TROLL_BLOOD:
            increase[Ego::Attribute::LIFE_REGEN] += 0.25f;
        break;        

        case Ego::Perks::GIGANTISM:
            increase[Ego::Attribute::MIGHT] += 2.00f;
            increase[Ego::Attribute::AGILITY] -= 2.00f;
        break;

        case Ego::Perks::BRUTE:
            increase[Ego::Attribute::MIGHT] += 1.00f;
            increase[Ego::Attribute::INTELLECT] -= 2.00f;
        break;

        case Ego::Perks::DRAGON_BLOOD:
            increase[Ego::Attribute::MANA_REGEN] += 0.25f;
        break;

        case Ego::Perks::ACROBATIC:
            _character->increaseBaseAttribute(Ego::Attribute::NUMBER_OF_JUMPS, 1.0f);
        break;

        case Ego::Perks::MASTER_ACROBAT:
            _character->increaseBaseAttribute(Ego::Attribute::NUMBER_OF_JUMPS, 1.0f);
        break;

        case Ego::Perks::POWER:
            increase[Ego::Attribute::MAX_MANA] += 2.00f;
        break;

        case Ego::Perks::PERFECTION:
            increase[Ego::Attribute::INTELLECT] += 1.00f;
            increase[Ego::Attribute::AGILITY] += 1.00f;
        break;

        case Ego::Perks::ANCIENT_BLUD:
            increase[Ego::Attribute::LIFE_REGEN] += 0.25f;
        break;

        case Ego::Perks::SPELL_MASTERY:
            increase[Ego::Attribute::SPELL_POWER] += 1.0f;
        break;

        case Ego::Perks::MYSTIC_INTELLECT:
            increase[Ego::Attribute::MAX_MANA] += 1.0f;
            increase[Ego::Attribute::MANA_REGEN] += 0.1f;
        break;

        case Ego::Perks::MEDITATION:
            increase[Ego::Attribute::MANA_REGEN] += 0.15f;
        break;

        case Ego::Perks::BOOKWORM:
            increase[Ego::Attribute::INTELLECT] += 2.00f;
            increase[Ego::Attribute::MIGHT] -= 2.00f;
        break;

        case Ego::Perks::NIGHT_VISION:
            _character->increaseBaseAttribute(Ego::Attribute::DARKVISION, 1.0f);
        break;

        case Ego::Perks::SENSE_KURSES:
            _character->increaseBaseAttribute(Ego::Attribute::SENSE_KURSES, 1.0f);
        break;

        case Ego::Perks::SENSE_INVISIBLE:
            _character->increaseBaseAttribute(Ego::Attribute::SEE_INVISIBLE, 1.0f);
        break;

        default:
            //nothing
        break;
    }

    //Increase character level by 1
    _character->experiencelevel += 1;
    SET_BIT(_character->ai.alert, ALERTIF_LEVELUP);
    PlaStack.get_ptr(_character->is_which_player)->_unspentLevelUp = false;

    //Generate random seed for next level increase
    _character->randomizeLevelUpSeed();

    //Might slightly increases character size
    if(increase[Ego::Attribute::MIGHT] != 0) {
        _character->fat_goto += _character->getProfile()->getSizeGainPerMight() * 0.1f * increase[Ego::Attribute::MIGHT];
        _character->fat_goto_time += SIZETIME;
    }

    //Clear away all GUI components
    for(const std::shared_ptr<GUIComponent> &component : iterator())
    {
        component->destroy();
    }

    //Selected perk animation
    _selectedPerk = std::make_shared<Image>(selectedPerk->getPerk().getIcon().getFilePath());
    _selectedPerk->setPosition(selectedPerk->getX() - getX(), selectedPerk->getY() - getY());
    _selectedPerk->setSize(selectedPerk->getWidth(), selectedPerk->getHeight());
    _selectedPerk->setTint(selectedPerk->getPerk().getColour());
    addComponent(_selectedPerk);

    //Make icon move into the corner and use about 1 second to do so
    const Vector2f DESIRED_ICON_POS = Vector2f(float(getX() - PERK_THUMBNAIL_SIZE - 10), 
                                               float(getY() + getHeight() - PERK_THUMBNAIL_SIZE - 20));
    _animationPos[0] = _selectedPerk->getX();
    _animationPos[1] = _selectedPerk->getY();
    _animationSpeed = (DESIRED_ICON_POS - _animationPos) * (1.0f / GameEngine::GAME_TARGET_FPS);

    std::shared_ptr<Label> newPerkLabel = std::make_shared<Label>("NEW PERK: " + selectedPerk->getPerk().getName());
    newPerkLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    newPerkLabel->setPosition(20, getHeight() - PERK_THUMBNAIL_SIZE - 40);
    newPerkLabel->setColor(Ego::Math::Colour4f::yellow());
    addComponent(newPerkLabel);
    _fadeInLabels.push_back(newPerkLabel);

    std::shared_ptr<Label> perkDescription = std::make_shared<Label>(selectedPerk->getPerk().getDescription());
    perkDescription->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    perkDescription->setPosition(PERK_THUMBNAIL_SIZE + 60, DESIRED_ICON_POS[1]-getY());
    addComponent(perkDescription);
    _fadeInLabels.push_back(perkDescription);

    //Attribute Header Label
    std::shared_ptr<Label> attributeIncrease = std::make_shared<Label>("ATTRIBUTE INCREASE:");
    attributeIncrease->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    attributeIncrease->setPosition(20, 40);
    addComponent(attributeIncrease);
    _fadeInLabels.push_back(attributeIncrease);

    //Figure out the widest attribute name width
    int attributeWidthSpacing = 0;
    for(uint8_t i = 0; i < Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES; ++i) {
        int width;
        _gameEngine->getUIManager()->getFont(UIManager::FONT_GAME)->getTextSize(Ego::Attribute::toString(static_cast<Ego::Attribute::AttributeType>(i)), &width, nullptr);
        attributeWidthSpacing = std::max(attributeWidthSpacing, width+10);
    }

    //Attributes
    for(uint8_t i = 0; i < Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES; ++i) {
        const Ego::Attribute::AttributeType type = static_cast<Ego::Attribute::AttributeType>(i);

        //Name
        std::shared_ptr<Label> attributeLabel = std::make_shared<Label>(Ego::Attribute::toString(type));
        attributeLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));

        float x, y;
        if(i < Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES/2) {
            x = 20;
            y = 40 + attributeIncrease->getHeight() + i * 25;
        }
        else {
            x = 10 + getWidth()/2;
            y = 40 + attributeIncrease->getHeight() + (i-Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES/2) * 25;
        }
        attributeLabel->setPosition(x, y);
        attributeLabel->setAlpha(0.0f);
        addComponent(attributeLabel);
        _fadeInLabels.push_back(attributeLabel);

        //Value
        std::shared_ptr<Label> value = std::make_shared<Label>();
        if(type == Ego::Attribute::MANA_REGEN || type == Ego::Attribute::LIFE_REGEN) { //special case for regen values (2 decimals)
            std::stringstream valueString;
            valueString << std::setprecision(2) << std::fixed << _character->getAttribute(type);
            value->setText(valueString.str());
        }
        else {
            value->setText(std::to_string(std::lround(_character->getAttribute(type))));        
        }
        value->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
        value->setPosition(x + attributeWidthSpacing, y);
        value->setAlpha(0.0f);
        _attributeValues[type] = value;
        _fadeInLabels.push_back(value);
        addComponent(value);

        //Change in attribute
        if(std::abs(increase[i]) > std::numeric_limits<float>::epsilon()) {
            _attributeIncrease[type] = std::make_shared<Label>();
            std::stringstream valueString;
            valueString << (increase[i] > 0 ? "+" : "") << std::setprecision(2) << std::fixed << increase[i];
            _attributeIncrease[type]->setText(valueString.str());
            _attributeIncrease[type]->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
            _attributeIncrease[type]->setPosition(x + attributeWidthSpacing + 50, y);
            _attributeIncrease[type]->setColor(increase[i] > 0 ? Ego::Math::Colour4f::yellow() : Ego::Math::Colour4f::red());
            _attributeIncrease[type]->setVisible(false);
            addComponent(_attributeIncrease[type]);
        }

        //Actually give attributes to character
        _character->increaseBaseAttribute(type, increase[i]);
    }

    //Make sure the animation is drawn above other GUI components inside this window
    _selectedPerk->bringToFront();
}

void LevelUpWindow::drawContainer()
{
    //Draw the window itself
    InternalWindow::drawContainer();

    //Update animations if needed
    if(_fadeInLabels.empty()) {
        return;
    }

    for(const std::shared_ptr<Label>& label : _fadeInLabels)
    {
        //about 4 second fade time
        if(label->getColour().getAlpha() < 1.0f) {
            label->setAlpha(std::min(1.0f, label->getColour().getAlpha() + (0.25f / GameEngine::GAME_TARGET_FPS)));
        }
    }

    //Make icon shrink
    if(_selectedPerk->getWidth() > PERK_THUMBNAIL_SIZE) {
        _selectedPerk->setSize(_selectedPerk->getWidth()-2, _selectedPerk->getHeight()-2);
    }

    //Move icon into corner (use about 1 second to get there)
    const int DESIRED_ICON_X = getX() + PERK_THUMBNAIL_SIZE - 10;
    const int DESIRED_ICON_Y = getY() + getHeight() - PERK_THUMBNAIL_SIZE - 20;
    bool animationComplete = true;

    if(_selectedPerk->getX() > DESIRED_ICON_X) {
        _animationPos[0] += _animationSpeed[0];
        _selectedPerk->setX(_animationPos[0]);
        animationComplete = false;
    }
    if(_selectedPerk->getY() < DESIRED_ICON_Y) {
        _animationPos[1] += _animationSpeed[1];
        _selectedPerk->setY(_animationPos[1]);
        animationComplete = false;
    }

    //This reveals the attribute score increases step by step (reveal one per 500ms)
    if(animationComplete && ::Time::now<::Time::Unit::Ticks>() > _attributeRevealTime) {
        _attributeRevealTime = ::Time::now<::Time::Unit::Ticks>() + 500;
        for(size_t i = 0; i < _attributeIncrease.size(); ++i) {
            if(!_attributeIncrease[i]) continue;
            if(_attributeIncrease[i]->isVisible()) continue;
            _attributeIncrease[i]->setVisible(true);
            AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_COINFALL));
            return;
        }

        //All animations complete!
        _fadeInLabels.clear();
    }
}

void LevelUpWindow::setHoverPerk(Ego::Perks::PerkID id)
{
    _currentPerk = id;

    if(id == Ego::Perks::NR_OF_PERKS)
    {
        _descriptionLabel->setText("Select your new perk...");
        _perkIncreaseLabel->setText("Hover mouse over a perk to see the benefits.");
        _perkIncreaseLabel->setColor(Ego::Math::Colour4f::purple());
    }
    else
    {
        const Ego::Perks::Perk &perk = Ego::Perks::PerkHandler::get().getPerk(id);
        _descriptionLabel->setText(perk.getDescription());
        _perkIncreaseLabel->setText(perk.getName() + "\n+1 " + Ego::Attribute::toString(perk.getType()));
        _perkIncreaseLabel->setColor(perk.getColour());
    }

    _perkIncreaseLabel->setCenterPosition(getX() + getWidth()/2, getY() + _desciptionLabelOffset + 10, true);        
    _descriptionLabel->setCenterPosition(getX() + getWidth()/2, _perkIncreaseLabel->getY() + _perkIncreaseLabel->getHeight()-10, true);
}

Ego::Perks::PerkID LevelUpWindow::getCurrentPerk() const
{
    return _currentPerk;
}

} //GUI
} //Ego 