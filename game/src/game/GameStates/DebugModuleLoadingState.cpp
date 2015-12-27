//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file game/GameStates/DebugModuleLoadingState.cpp
/// @details Debugging state where one can debug loading modules
/// @author Johan Jansen, penguinflyer5234

#include "game/GameStates/DebugModuleLoadingState.hpp"
#include "game/GameStates/PlayingState.hpp"
#include "game/GameStates/LoadPlayerElement.hpp"
#include "game/Core/GameEngine.hpp"
#include "egolib/egoboo_setup.h"
#include "game/graphic.h"
#include "game/GUI/Button.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/ScrollableList.hpp"
#include "egolib/Math/Random.hpp"
#include "egolib/Audio/AudioSystem.hpp"

//For loading stuff
#include "game/Graphics/CameraSystem.hpp"
#include "game/game.h"
#include "game/graphic_billboard.h"
#include "game/link.h"
#include "game/renderer_2d.h"
#include "egolib/fileutil.h"

struct DebugModuleLoadingState::ModuleGUIContainer : public ComponentContainer, public GUIComponent
{
    ModuleGUIContainer(const std::shared_ptr<ModuleProfile> &profile) :
        _profile(profile),
        _moduleName(std::make_shared<Button>(_profile->getFolderName())),
        _loadingText(std::make_shared<Label>("Not Loaded"))
    {
        _moduleName->setSize(200, 30);
        
        addComponent(_moduleName);
        addComponent(_loadingText);
        
        const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
        setSize(SCREEN_WIDTH - 50, 30);
    }
    
    void setOnClick(const std::function<void()> &onClick)
    {
        _moduleName->setOnClickFunction(onClick);
    }
    
    void setButtonEnabled(bool enabled)
    {
        _moduleName->setEnabled(enabled);
    }
    
    void draw() override
    {
        drawAll();
    }
    
    void drawContainer() override {}
    
    void setPosition(const int x, const int y) override
    {
        GUIComponent::setPosition(x, y);
        _moduleName->setPosition(x, y);
        _loadingText->setPosition(x + 215, y);
    }
    
    bool notifyMouseClicked(const int button, const int x, const int y) override
    {
        return ComponentContainer::notifyMouseClicked(button, x, y);
    }
    
    bool notifyMouseMoved(const int x, const int y) override
    {
        return ComponentContainer::notifyMouseMoved(x, y);
    }
    
    std::shared_ptr<ModuleProfile> _profile;
    std::shared_ptr<Button> _moduleName;
    std::shared_ptr<Label> _loadingText;
};

DebugModuleLoadingState::DebugModuleLoadingState() :
#ifdef _MSC_VER
	_finishedLoading({0}),
#else
	_finishedLoading(false),
#endif
	_loadingThread(),
    _scrollableList(),
    _playersToLoad(),
    _moduleList(),
    _toLoad()
{
    const auto &playerList = ProfileSystem::get().getSavedPlayers();
    if (!playerList.empty()) _playersToLoad.emplace_back(playerList[0]->getProfile()->getPathname());
    
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();
    
    _scrollableList = std::make_shared<ScrollableList>();
    _scrollableList->setPosition(8, 8);
    _scrollableList->setSize(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 56);
    
    for (const auto &loadModule : ProfileSystem::get().getModuleProfiles())
    {
        std::shared_ptr<ModuleGUIContainer> module = std::make_shared<ModuleGUIContainer>(loadModule);
        if (loadModule->getImportAmount()) module->setButtonEnabled(!_playersToLoad.empty());
        
        std::weak_ptr<ModuleGUIContainer> modulePtr(module);
        module->setOnClick([this, modulePtr] { addToQueue(modulePtr.lock()); });
        
        _scrollableList->addComponent(module);
        _moduleList.emplace_back(module);
    }
    _scrollableList->forceUpdate();
    addComponent(_scrollableList);
    
    std::shared_ptr<Button> back = std::make_shared<Button>("Back");
    back->setPosition(8, SCREEN_HEIGHT - 30 - 8);
    back->setSize(150, 30);
    back->setOnClickFunction([this] { endState(); });
    addComponent(back);
    
    std::shared_ptr<Button> loadAll = std::make_shared<Button>("Load All");
    loadAll->setPosition(SCREEN_WIDTH - 150 - 8, SCREEN_HEIGHT - 30 - 8);
    loadAll->setSize(150, 30);
    loadAll->setOnClickFunction([this] { for (const auto &a : _moduleList) addToQueue(a); });
    addComponent(loadAll);
}

DebugModuleLoadingState::~DebugModuleLoadingState()
{
	//Wait until thread is dead
	if(_loadingThread.joinable()) {
		_loadingThread.join();
	}
}

void DebugModuleLoadingState::addToQueue(const std::shared_ptr<ModuleGUIContainer> &toAdd)
{
    if (!toAdd) return;
    if (std::find(_toLoad.begin(), _toLoad.end(), toAdd) != _toLoad.end()) return;
    _toLoad.emplace_back(toAdd);
    toAdd->_loadingText->setText("Waiting...");
}

//TODO: HACK (no multithreading yet)
void DebugModuleLoadingState::singleThreadRedrawHack(const std::string &loadingText)
{
    // clear the screen
    gfx_request_clear_screen();
    gfx_do_clear_screen();
    
    _toLoad.front()->_loadingText->setText(loadingText);
    
    drawAll();
    
    // flip the graphics page
    gfx_request_flip_pages();
    gfx_do_flip_pages();
    SDL_PumpEvents();
}
//TODO: HACK END


void DebugModuleLoadingState::update()
{
    if (!_toLoad.empty()) loadModuleData();
}

void DebugModuleLoadingState::drawContainer()
{

}

void DebugModuleLoadingState::beginState()
{
	//Start the background loading thread
	//_loadingThread = std::thread(&LoadingState2::loadModuleData, this);
    AudioSystem::get().playMusic("loading_screen.ogg");
}


bool DebugModuleLoadingState::loadPlayers()
{
    // blank out any existing data
    import_list_t::init(g_importList);

    // loop through the selected players and store all the valid data in the list of imported players
    for(const std::string &loadPath : _playersToLoad)
    {
        // get a new import data pointer
        import_element_t *import_ptr = g_importList.lst + g_importList.count;
        g_importList.count++;

        //figure out which player we are (1, 2, 3 or 4)
        import_ptr->local_player_num = g_importList.count-1;

        // set the import info
        import_ptr->slot            = (import_ptr->local_player_num) * MAX_IMPORT_PER_PLAYER;
        import_ptr->player          = (import_ptr->local_player_num);

        strncpy( import_ptr->srcDir, loadPath.c_str(), SDL_arraysize( import_ptr->srcDir ) );
        import_ptr->dstDir[0] = CSTR_END;
    }

    if (g_importList.count > 0)
    {
        if (game_copy_imports(&g_importList) == rv_success)
        {
            return true;
        }
        else
        {
            // erase the data in the import folder
            vfs_removeDirectoryAndContents("import", VFS_TRUE);
            return false;
        }
    }

    return false;
}

void DebugModuleLoadingState::loadModuleData()
{
    auto &_loadModule = _toLoad.front()->_profile;
    try
    {
        singleThreadRedrawHack("Tidying some space...");

        //Make sure all data is cleared first
        game_quit_module();

        singleThreadRedrawHack("Calculating some math...");
        BillboardSystem::get().reset();

        //initialize math objects
        make_turntosin();

        // Linking system
		Log::get().info("Initializing module linking... ");
        if (link_build_vfs( "mp_data/link.txt", LinkList)) Log::get().message("Success!\n");
        else Log::get().message( "Failure!\n" );

        // initialize the collision system
        singleThreadRedrawHack("Beautifying graphics...");

        //Ready message display
        DisplayMsg_reset();

        // Reset all "profiles" in the "profile system".
        ProfileSystem::get().reset();

        // do some graphics initialization
        //make_lightdirectionlookup();
        gfx_system_make_enviro();

        //Load players if needed
        if(!_playersToLoad.empty())
        {
            singleThreadRedrawHack("Loading players...");
            if(!loadPlayers()) {
                throw std::runtime_error("Failed to load players!\n");
            }
        }

        // try to start a new module
        singleThreadRedrawHack("Loading module data...");
        if (!game_begin_module(_loadModule))
        {
            throw std::runtime_error("Failed to load module!");
        }

        singleThreadRedrawHack("Almost done...");

        // set up the cameras *after* game_begin_module() or the player devices will not be initialized
        // and camera_system_begin() will not set up thte correct view
        CameraSystem::request(local_stats.player_count);

        //Fade out music when finished loading
        AudioSystem::get().stopMusic();

        //Complete!
        singleThreadRedrawHack("Finished!");
    }
    catch (Ego::Core::Exception &ex)
    {
        std::string out = std::string("Ego::Exception: ") + std::string(ex);
        singleThreadRedrawHack(out);
		Log::get().warn("error loading %s... %s\n", _loadModule->getFolderName().c_str(), out.c_str());
    }
    catch (std::exception &ex)
    {
        std::string out = std::string("std::exception: ") + ex.what();
        singleThreadRedrawHack(out);
		Log::get().warn("error loading %s... %s\n", _loadModule->getFolderName().c_str(), out.c_str());
    }
    catch (std::string &ex)
    {
        std::string out = std::string("std::string: ") + ex;
        singleThreadRedrawHack(out);
		Log::get().warn("error loading %s... %s\n", _loadModule->getFolderName().c_str(), out.c_str());
    }
    catch (char *ex)
    {
        std::string out = std::string("C string: ") + ex;
        singleThreadRedrawHack(out);
		Log::get().warn("error loading %s... %s\n", _loadModule->getFolderName().c_str(), out.c_str());
    }
    catch (...)
    {
        std::string out = "unknown error";
        singleThreadRedrawHack(out);
		Log::get().warn("error loading %s... %s\n", _loadModule->getFolderName().c_str(), out.c_str());
    }
    _toLoad.pop_front();
}
