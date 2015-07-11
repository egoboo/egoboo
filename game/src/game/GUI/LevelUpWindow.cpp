#include "LevelUpWindow.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Image.hpp"
#include "game/Entities/_Include.hpp"

namespace Ego
{
namespace GUI
{

class PerkButton : public GUIComponent
{
public:
    PerkButton(const Ego::Perks::PerkID id) :
        _perk(Ego::Perks::PerkHandler::get().getPerk(id)),
        _label(_perk.getName())
    {
        _label.setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    }

    void draw() override
    {
        // Draw backdrop
        auto &renderer = Ego::Renderer::get();
        oglx_texture_t::bind(nullptr);


        renderer.setColour(_perk.getColour());

        struct Vertex
        {
            float x, y;
        };

        auto vb = _gameEngine->getUIManager()->_vertexBuffer;
        Vertex *v = static_cast<Vertex *>(vb->lock());
        v->x = getX(); v->y = getY(); v++;
        v->x = getX(); v->y = getY() + getHeight(); v++;
        v->x = getX() + getWidth(); v->y = getY() + getHeight(); v++;
        v->x = getX() + getWidth(); v->y = getY();
        vb->unlock();
        renderer.render(*vb, Ego::PrimitiveType::Quadriliterals, 0, 4);

        //Icon
        _gameEngine->getUIManager()->drawImage(_perk.getIcon(), getX(), getY(), getWidth(), getHeight(), Ego::Math::Colour4f(0, 0, 0, 0.5f));

        //Name
        _label.draw();
    }

    void setPosition(int x, int y) override
    {
        _label.setCenterPosition(x + getWidth()/2, y + getHeight() + _label.getHeight());
        GUIComponent::setPosition(x, y);
    }

private:
    const Ego::Perks::Perk& _perk;
    Ego::DeferredOpenGLTexture _texture;
    Label _label;
};

LevelUpWindow::LevelUpWindow(const std::shared_ptr<Object> &object) : InternalWindow("Level Up!"),
    _character(object)
{
    int yPos = 0;

    //Place us in the center of the screen
    setSize(510, 320);
    setCenterPosition(_gameEngine->getUIManager()->getScreenWidth()/2, _gameEngine->getUIManager()->getScreenHeight()/2);

    // draw the character's main icon
    std::shared_ptr<Image> characterIcon = std::make_shared<Image>(TextureManager::get().get_valid_ptr(_character->getProfile()->getIcon(_character->skin)));
    characterIcon->setPosition(5, 32);
    characterIcon->setSize(32, 32);
    addComponent(characterIcon);

    std::stringstream buffer;

    //Name
    buffer << object->getName() << " is now a ";

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

    //Class
    buffer << _character->getProfile()->getClassName() << '!';

    std::shared_ptr<Label> classLevelLabel = std::make_shared<Label>(buffer.str());
    classLevelLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    classLevelLabel->setPosition(characterIcon->getX() + characterIcon->getWidth() + 5, characterIcon->getY());
    addComponent(classLevelLabel);

    std::shared_ptr<Label> selectPerkLabel = std::make_shared<Label>("Select your Perk:");
    selectPerkLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_DEFAULT));
    selectPerkLabel->setPosition(getWidth()/2 - selectPerkLabel->getWidth()/2, classLevelLabel->getY() + classLevelLabel->getHeight());
    addComponent(selectPerkLabel);

    //Figure out what perks this player can learn
    std::vector<Ego::Perks::PerkID> perkPool = _character->getValidPerks();
    for(size_t i = perkPool.size(); i < 3; ++i) {
        //Ensure at least 3 perks are selectable, add TOUGHNESS as default perk
        perkPool.push_back(Ego::Perks::TOUGHNESS);
    }

    //Perk buttons
    const size_t NR_OF_PERKS = 3;
    const int PERK_BUTTON_SIZE = (getWidth() - 40 - 10*NR_OF_PERKS) / NR_OF_PERKS;
    for(size_t i = 0; i < NR_OF_PERKS; ++i) {
        //Select a random perk
        const size_t randomIndex = Random::next(perkPool.size()-1);
        std::shared_ptr<PerkButton> perkButton = std::make_shared<PerkButton>(perkPool[randomIndex]);
        perkButton->setSize(PERK_BUTTON_SIZE, PERK_BUTTON_SIZE);
        perkButton->setPosition(20 + i * (perkButton->getWidth()+10), selectPerkLabel->getY() + selectPerkLabel->getHeight());
        addComponent(perkButton);

        //Remove perk from pool
        perkPool.erase(perkPool.begin() + randomIndex);
    }

    //Play level up sound
    AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_LEVELUP));
}

LevelUpWindow::~LevelUpWindow()
{
}

} //GUI
} //Ego 