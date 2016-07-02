#include "game/GameStates/MapEditorSelectModuleState.hpp"
#include "game/GameStates/MapEditorState.hpp"
#include "game/GUI/ScrollableList.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/IconButton.hpp"
#include "game/GUI/Label.hpp"

namespace Ego
{
namespace GameStates
{

MapEditorSelectModuleState::MapEditorSelectModuleState() :
	_selectedModule(nullptr),
	_selectedButton(nullptr),
	_moduleName(std::make_shared<Ego::GUI::Label>()),
	_moduleDescription(nullptr)
{
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    //Name of selected module
    _moduleName->setPosition(SCREEN_WIDTH/2 + 20, 20);
    addComponent(_moduleName);

    //Scrollable list of all modules
    auto scrollableList = std::make_shared<Ego::GUI::ScrollableList>();
    scrollableList->setPosition(8, 8);
    scrollableList->setSize(Vector2f(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 56));

    for (const auto &loadModule : ProfileSystem::get().getModuleProfiles())
    {
    	const std::string folderName = loadModule->getPath().substr(loadModule->getPath().find_last_of('/') + 1);

        auto module = std::make_shared<Ego::GUI::IconButton>(folderName, loadModule->getIcon());
       	module->setSize(Vector2f(scrollableList->getWidth()-50, 50)); 

        module->setOnClickFunction([this, module, loadModule] { 
        	module->setEnabled(false);
        	_selectedButton->setEnabled(true);
        	_selectedButton = module;
        	setSelectedModule(loadModule);
       });

        //Select first module by default
        if(_selectedButton == nullptr) {
        	_selectedButton = module;
        	_selectedButton->doClick();
        }
        
        scrollableList->addComponent(module);
    }
    scrollableList->forceUpdate();
    addComponent(scrollableList);

    auto editModuleButton = std::make_shared<Ego::GUI::Button>("Open Editor", SDLK_RETURN);
    editModuleButton->setSize(Vector2f(150, 40));
    editModuleButton->setPosition(SCREEN_WIDTH - editModuleButton->getWidth() - 5, SCREEN_HEIGHT - editModuleButton->getHeight() - 5);
    editModuleButton->setOnClickFunction([this]{
    	_gameEngine->setGameState(std::make_shared<MapEditorState>(_selectedModule));
    });
    addComponent(editModuleButton);

    auto newModuleButton = std::make_shared<Ego::GUI::Button>("New Module");
    newModuleButton->setSize(Vector2f(150, 40));
    newModuleButton->setPosition(SCREEN_WIDTH/2 - newModuleButton->getWidth()/2, SCREEN_HEIGHT - newModuleButton->getHeight() - 5);
    //newModuleButton->setOnClickFunction([this]{
    //	TODO
    //});
    addComponent(newModuleButton);

    auto backButton = std::make_shared<Ego::GUI::Button>("Back", SDLK_ESCAPE);
    backButton->setSize(Vector2f(150, 40));
    backButton->setPosition(5, SCREEN_HEIGHT - backButton->getHeight() - 5);
    backButton->setOnClickFunction([this]{
    	this->endState();
    });
    addComponent(backButton);
}

void MapEditorSelectModuleState::setSelectedModule(const std::shared_ptr<ModuleProfile> &profile)
{
	_selectedModule = profile;

	_moduleName->setText(profile->getName());

    // Module description
    std::stringstream buffer;
    if (_selectedModule->getMaxPlayers() > 1)
    {
        if (_selectedModule->getMaxPlayers() == _selectedModule->getMinPlayers())
        {
            buffer << _selectedModule->getMinPlayers() << " Players" << '\n';
        }
        else
        {
            buffer << std::to_string(_selectedModule->getMinPlayers()) << '-' << std::to_string(_selectedModule->getMaxPlayers()) << " Players" << '\n';
        }
    }
    else if (_selectedModule->isStarterModule())
    {
        buffer << "Starter Module" << '\n';
    }
    else
    {
        buffer << "Single Player" << '\n';
    }

    for (const std::string &line : _selectedModule->getSummary())
    {
        buffer << line << '\n';;
    }

    const std::shared_ptr<Ego::Font> &font = _gameEngine->getUIManager()->getFont(Ego::GUI::UIManager::FONT_DEBUG);
    _moduleDescription = font->layoutTextBox(buffer.str(), _gameEngine->getUIManager()->getScreenWidth() / 2 - 20, 0, font->getLineSpacing(), nullptr, nullptr);
}

void MapEditorSelectModuleState::drawContainer()
{
	auto& UI = _gameEngine->getUIManager();

    UI->beginRenderUI();

    int yPos = _moduleName->getY() + _moduleName->getHeight();

    // Now difficulty
    if (_selectedModule->getRank() > 0)
    {
        UI->getFont(Ego::GUI::UIManager::FONT_DEBUG)->drawTextBox("DIFFICULITY: ", _moduleName->getX(), yPos, 200, 50, 25);
        yPos += 20;

        // Draw one skull per rated difficulty
        const std::shared_ptr<Ego::Texture> &skullTexture = TextureManager::get().getTexture("mp_data/skull");
        for (int i = 0; i < _selectedModule->getRank(); ++i)
        {
            draw_icon_texture(skullTexture, _moduleName->getX() + i*skullTexture->getWidth(), yPos, 0xFF, 0, 20, true);
        }

        yPos += 30;
    }

    _moduleDescription->render(_moduleName->getX(), yPos, Ego::Math::Colour4f::white());

    //Module image
	UI->drawImage(_selectedModule->getIcon().get(), Point2f(_moduleName->getX() + UI->getScreenWidth()/4 - 64, yPos + 200), Vector2f(128, 128));    

    UI->endRenderUI();
}

} //GameStates
} //Ego
