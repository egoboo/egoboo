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

//For loading stuff
#include "game/Graphics/CameraSystem.hpp"
#include "game/game.h"
#include "game/graphic_billboard.h"
#include "game/link.h"
#include "game/collision.h"
#include "game/renderer_2d.h"
#include "game/bsp.h"
#include "game/Module/Module.hpp"

LoadingState::LoadingState(std::shared_ptr<ModuleProfile> module, const std::list<std::string> &playersToLoad) :
    _finishedLoading({0}),
    _loadingThread(),
    _loadingLabel(nullptr),
    _loadModule(module),
    _playersToLoad(playersToLoad),
    _globalGameTips(),
    _localGameTips()
{
    //Load background
    std::shared_ptr<Image> background = std::make_shared<Image>("mp_data/menu/menu_logo");
    background->setSize(background->getTextureWidth(), background->getTextureHeight());
    background->setPosition(GFX_WIDTH/2-background->getTextureWidth()/2, GFX_HEIGHT/2-background->getTextureHeight()/2-100);
    addComponent(background);

    std::shared_ptr<Label> mainLabel = std::make_shared<Label>("LOADING MODULE");
    mainLabel->setPosition(GFX_WIDTH/2 - mainLabel->getWidth()/2, 20);
    addComponent(mainLabel);

    _loadingLabel = std::make_shared<Label>("Initializing...");
    addComponent(_loadingLabel);

    //Load game hints. It's OK if we dont get any local hints
    loadLocalModuleHints();
    loadGlobalHints();

    //Add a random game tip
    std::shared_ptr<Label> gameTip = std::make_shared<Label>(getRandomHint());
    gameTip->setPosition(GFX_WIDTH/2 - gameTip->getWidth()/2, GFX_HEIGHT/2);
    addComponent(gameTip);
}

LoadingState::~LoadingState()
{
    // Wait until thread is dead:
    if(_loadingThread.joinable()) {
        _loadingThread.join();
    }
}

//TODO: HACK (no multithreading yet)
void LoadingState::singleThreadRedrawHack(const std::string &loadingText)
{
    // clear the screen
    gfx_request_clear_screen();
    gfx_do_clear_screen();

    //Always make loading text centered
    _loadingLabel->setText(loadingText);
    _loadingLabel->setPosition(GFX_WIDTH/2 - _loadingLabel->getWidth()/2, 40);

    drawAll();

    // flip the graphics page
    gfx_request_flip_pages();
    gfx_do_flip_pages();
}
//TODO: HACK END


void LoadingState::update()
{
	if(!_finishedLoading) {
		loadModuleData();
	}
}

void LoadingState::drawContainer()
{

}

void LoadingState::beginState()
{
    //Start the background loading thread
    //_loadingThread = std::thread(&LoadingState::loadModuleData, this);
    AudioSystem::get().playMusic(27); //TODO: needs to be referenced by string
}


bool LoadingState::loadPlayers()
{
    // blank out any existing data
    import_list_init(g_importList);

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
    singleThreadRedrawHack("Tidying some space...");

    //Make sure all data is cleared first
    game_quit_module();

    singleThreadRedrawHack("Calculating some math...");
    BillboardSystem::get()._billboardList.reset();

    //initialize math objects
    make_turntosin();

    // Linking system
    log_info("Initializing module linking... ");
    if (link_build_vfs( "mp_data/link.txt", LinkList)) log_message("Success!\n");
    else log_message( "Failure!\n" );

    // initialize the collision system
    singleThreadRedrawHack("Preparing collisions...");
    CollisionSystem::get()->reset();

    //Ready message display
    DisplayMsg_reset();

    // Reset all loaded "profiles" in the "profile system".
    ProfileSystem::get().reset();

    // do some graphics initialization
    //make_lightdirectionlookup();
    gfx_system_make_enviro();

    //Load players if needed
    if(!_playersToLoad.empty()) {
        singleThreadRedrawHack("Loading players...");
        if(!loadPlayers()) {
            log_warning("Failed to load players!\n");
            endState();
            return;
        }
    }

    // try to start a new module
    singleThreadRedrawHack("Loading module data...");
    if(!game_begin_module(_loadModule)) {
        log_warning("Failed to load module!\n");
        endState();
        return;
    }
    _currentModule->setImportPlayers(_playersToLoad);

    singleThreadRedrawHack("Almost done...");

    // set up the cameras *after* game_begin_module() or the player devices will not be initialized
    // and camera_system_begin() will not set up thte correct view
    std::shared_ptr<CameraSystem> cameraSystem = CameraSystem::request(local_stats.player_count);

    obj_BSP_system_begin(getMeshBSP()); 

    // Fade out music when finished loading
    AudioSystem::get().stopMusic();

    // Must wait until music has finished fading out or else update loop will lag first few updates
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Complete!
    singleThreadRedrawHack("Finished!");
    _finishedLoading = true;

    // Hit that gong
    AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_GAME_READY));

    //Add the start button once we are finished loading
    std::shared_ptr<Button> startButton = std::make_shared<Button>("Press Space to begin", SDLK_SPACE);
    startButton->setSize(400, 30);
    startButton->setPosition(GFX_WIDTH/2 - startButton->getWidth()/2, GFX_HEIGHT-50);
    startButton->setOnClickFunction(
        [cameraSystem]{
            //Hush gong
            AudioSystem::get().fadeAllSounds();
            _gameEngine->setGameState(std::make_shared<PlayingState>(cameraSystem));
        });
    addComponent(startButton);
}


bool LoadingState::loadGlobalHints()
{
    // Open the file with all the tips
    ReadContext ctxt("mp_data/gametips.txt");
    if (!ctxt.ensureOpen())
    {
        log_warning("Unable to load the game tips and hints file `%s`\n", ctxt.getLoadName().c_str());
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
        log_warning( "Could not load the game tips and hints. (\"basicdat/gametips.txt\")\n" );
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
