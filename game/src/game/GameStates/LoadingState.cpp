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

/// @file game/GameStates/LoadingState.cpp
/// @details Main state where the players are currently playing a module
/// @todo Wrong doc comment.
/// @author Johan Jansen

#include "game/GameStates/LoadingState.hpp"
#include "game/GameStates/PlayingState.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/graphic.h"
#include "game/GUI/Button.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Image.hpp"
#include "game/GUI/ProgressBar.hpp"

//For loading stuff
#include "game/Graphics/CameraSystem.hpp"
#include "game/game.h"
#include "game/graphic_billboard.h"
#include "game/link.h"
#include "game/renderer_2d.h"
#include "game/Module/Module.hpp"
#include "game/Graphics/TextureAtlasManager.hpp"

LoadingState::LoadingState(std::shared_ptr<ModuleProfile> module, const std::list<std::string> &playersToLoad) :
    _loadingThread(),
    _loadingLabel(nullptr),
    _loadModule(module),
    _playersToLoad(playersToLoad),
    _globalGameTips(),
    _localGameTips(),
    _progressBar(std::make_shared<GUI::ProgressBar>())
{
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();

    //Load background
    std::shared_ptr<Image> background = std::make_shared<Image>("mp_data/menu/menu_logo");
    background->setSize(background->getTextureWidth(), background->getTextureHeight());
    background->setPosition(SCREEN_WIDTH/2-background->getTextureWidth()/2, SCREEN_HEIGHT/2-background->getTextureHeight()/2-100);
    addComponent(background);

    std::shared_ptr<Label> mainLabel = std::make_shared<Label>("LOADING MODULE");
    mainLabel->setPosition(SCREEN_WIDTH/2 - mainLabel->getWidth()/2, 20);
    addComponent(mainLabel);

    _loadingLabel = std::make_shared<Label>("Initializing...");
    addComponent(_loadingLabel);

    //Load game hints. It's OK if we dont get any local hints
    loadLocalModuleHints();
    loadGlobalHints();

    //Add the progress bar
    _progressBar->setSize(400, 30);
    _progressBar->setPosition(SCREEN_WIDTH/2 - _progressBar->getWidth()/2, SCREEN_HEIGHT-50);
    _progressBar->setMaxValue(100);
    addComponent(_progressBar);

    //Add a random game tip
    std::shared_ptr<Label> gameTip = std::make_shared<Label>(getRandomHint());
    gameTip->setPosition(SCREEN_WIDTH/2 - gameTip->getWidth()/2, SCREEN_HEIGHT/2);
    addComponent(gameTip);
}

LoadingState::~LoadingState()
{
    // Wait until thread is dead
    if(_loadingThread.joinable()) {
        _loadingThread.join();
    }
}

void LoadingState::setProgressText(const std::string &loadingText, const uint8_t progress)
{
    //Always make loading text centered
    _loadingLabel->setText(loadingText);
    _loadingLabel->setPosition(_gameEngine->getUIManager()->getScreenWidth()/2 - _loadingLabel->getWidth()/2, 40);    

    _progressBar->setValue(progress);
}

void LoadingState::update()
{
    
}

void LoadingState::drawContainer()
{

}

void LoadingState::beginState()
{
    //Start the background loading thread
    _loadingThread = std::thread(&LoadingState::loadModuleData, this);
    AudioSystem::get().playMusic("loading_screen.ogg");
}


bool LoadingState::loadPlayers()
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

    if(g_importList.count > 0) {

        if(game_copy_imports(&g_importList) == rv_success) {
            return true;
        }
        else {
            // erase the data in the import folder
            vfs_removeDirectoryAndContents( "import", VFS_TRUE );
            return false;
        }
    }

    return false;
}

void LoadingState::loadModuleData()
{
    const int SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth();
    const int SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight();
    
    setProgressText("Tidying some space...", 0);

    //Make sure all data is cleared first
    game_quit_module();

    setProgressText("Calculating some math...", 10);
    BillboardSystem::get().reset();

    //initialize math objects
    make_turntosin();

    // Linking system
    setProgressText("Initializing module linking... ", 20);
    if (!link_build_vfs( "mp_data/link.txt", LinkList)) Log::get().warn("Failed to initialize module linking\n");

    // initialize the collision system
    setProgressText("Beautifying graphics...", 40);

    //Ready message display
    DisplayMsg_reset();

    // Reset all loaded "profiles" in the "profile system".
    ProfileSystem::get().reset();

    // do some graphics initialization
    gfx_system_make_enviro();

    //Load players if needed
    if(!_playersToLoad.empty()) {
        setProgressText("Loading players...", 50);
        if(!loadPlayers()) {
			Log::get().warn("Failed to load players!\n");
            endState();
            return;
        }
    }

    // try to start a new module
    setProgressText("Loading module data...", 60);
    if(!game_begin_module(_loadModule)) {
		Log::get().warn("Failed to load module!\n");
        endState();
        return;
    }
    _currentModule->setImportPlayers(_playersToLoad);

    setProgressText("Almost done...", 90);

    // set up the cameras *after* game_begin_module() or the player devices will not be initialized
    // and camera_system_begin() will not set up thte correct view
    std::shared_ptr<CameraSystem> cameraSystem = CameraSystem::request(local_stats.player_count);

    // Fade out music when finished loading
    AudioSystem::get().stopMusic();

    // make sure the per-module configuration settings are correct
    config_synch(&egoboo_config_t::get(), true, false);

    // Complete!
    setProgressText("Finished!", 100);

    // Hit that gong
    AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_GAME_READY));

    //1 second delay to let music finish, this prevents a frame lag on module startup
    std::this_thread::sleep_for(std::chrono::seconds(2));

    //Add the start button once we are finished loading
    std::shared_ptr<Button> startButton = std::make_shared<Button>("Press Space to begin", SDLK_SPACE);
    startButton->setSize(400, 30);
    startButton->setPosition(SCREEN_WIDTH/2 - startButton->getWidth()/2, SCREEN_HEIGHT-50);
    startButton->setOnClickFunction(
        [cameraSystem]{

            //Have to do this function in the OpenGL context thread or else it will fail
            Ego::Graphics::TextureAtlasManager::get().loadTileSet();

            //Hush gong
            AudioSystem::get().fadeAllSounds();
            _gameEngine->setGameState(std::make_shared<PlayingState>(cameraSystem));
        });
    addComponent(startButton);

    //Hide the progress bar
    _progressBar->setVisible(false);
}


bool LoadingState::loadGlobalHints()
{
    // Open the file with all the tips
    ReadContext ctxt("mp_data/gametips.txt");
    if (!ctxt.ensureOpen())
    {
		Log::get().warn("Unable to load the game tips and hints file `%s`\n", ctxt.getLoadName().c_str());
        return false;
    }

    // Load the data
    while (ctxt.skipToColon(true))
    {
        char buffer[1024+1];

        //Read the line
        vfs_read_string_lit(ctxt, buffer, 1024);

        //Make it look nice
        str_add_linebreaks(buffer, 1024, 50);

        _globalGameTips.push_back(buffer);
    }

    ctxt.close();

    if(_globalGameTips.empty()) {
		Log::get().warn( "Could not load the game tips and hints. (\"basicdat/gametips.txt\")\n" );
    }
 
    return !_globalGameTips.empty();
}

bool LoadingState::loadLocalModuleHints()
{
    STRING buffer;

    // Open all the tips
    snprintf(buffer, SDL_arraysize( buffer ), "mp_modules/%s/gamedat/gametips.txt", _loadModule->getFolderName().c_str());
    ReadContext ctxt(buffer);
    if (!ctxt.ensureOpen())
    {
        return false;
    }
    // Load the data
    while (ctxt.skipToColon(true))
    {

        //Read the line
        vfs_read_string_lit(ctxt, buffer, SDL_arraysize(buffer));

        //Make it look nice
        str_add_linebreaks(buffer, SDL_arraysize(buffer), 50);

        _localGameTips.push_back(buffer);
    }

    return !_localGameTips.empty();
}

const std::string LoadingState::getRandomHint() const
{
    if(_globalGameTips.empty() && _localGameTips.empty())
    {
        // no hints loaded, use the default hint
        return "Don't die...";
    }
    else if(!_localGameTips.empty())
    {
        //33% chance for a global tip
        if(!_globalGameTips.empty() && Random::getPercent() <= 33) {
            return Random::getRandomElement(_globalGameTips);
        }

        //Prefer local tips if we have them
        return Random::getRandomElement(_localGameTips);
    }

    //Return generic global tip
    return Random::getRandomElement(_globalGameTips);
}
