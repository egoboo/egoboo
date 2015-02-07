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

/// @file game/gamestates/LoadingState.cpp
/// @details Main state where the players are currently playing a module
/// @author Johan Jansen

#include "game/gamestates/LoadingState.hpp"
#include "game/gamestates/PlayingState.hpp"
#include "game/profiles/ModuleProfile.hpp"
#include "game/core/GameEngine.hpp"
#include "egolib/egoboo_setup.h"
#include "game/ui.h"
#include "game/graphic.h"
#include "game/gui/Button.hpp"
#include "game/gui/Label.hpp"
#include "egolib/math/Random.hpp"
#include "game/audio/AudioSystem.hpp"

//For loading stuff
#include "game/graphics/CameraSystem.hpp"
#include "game/profiles/ProfileSystem.hpp"
#include "game/game.h"
#include "game/graphic_billboard.h"
#include "game/link.h"
#include "game/collision.h"
#include "game/renderer_2d.h"
#include "game/bsp.h"
#include "egolib/fileutil.h"

LoadingState::LoadingState(std::shared_ptr<ModuleProfile> module, const std::list<std::shared_ptr<LoadPlayerElement>> &players) :
	_finishedLoading(false),
	_loadingThread(),
    _loadingLabel(nullptr),
	_loadModule(module),
	//_players(players),
    _globalGameTips(),
    _localGameTips()
{
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
	//Wait until thread is dead
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
    _audioSystem.playMusic(27); //TODO: needs to be referenced by string
}

void LoadingState::loadModuleData()
{
    singleThreadRedrawHack("Tidying some space...");

	//Make sure all data is cleared first
    game_quit_module();

    singleThreadRedrawHack("Calculating some math...");
    BillboardList_init_all();

    //initialize math objects
    make_randie();
    make_turntosin();

    // Linking system
    log_info("Initializing module linking... ");
    if (link_build_vfs( "mp_data/link.txt", LinkList)) log_message("Success!\n");
    else log_message( "Failure!\n" );

    // initialize the collision system
    singleThreadRedrawHack("Preparing collisions...");
    collision_system_begin();

    //Ready message display
    DisplayMsg_reset();

    // intialize the "profile system"
    _profileSystem.begin();

    // do some graphics initialization
    //make_lightdirectionlookup();
    gfx_system_make_enviro();

    // try to start a new module
    singleThreadRedrawHack("Loading module data...");
    if(!game_begin_module(_loadModule)) {
    	log_warning("Failed to load module!\n");
    	endState();
    	return;
    }

    //Complete!
    singleThreadRedrawHack("Finished!");
    _finishedLoading = true;
    //_gameEngine->setGameState(std::make_shared<PlayingState>());

    //Add the start button once we are finished loading
    std::shared_ptr<Button> startButton = std::make_shared<Button>("Press Space to begin", SDLK_SPACE);
    startButton->setSize(400, 30);
    startButton->setPosition(GFX_WIDTH/2 - startButton->getWidth()/2, GFX_HEIGHT-50);
    startButton->setOnClickFunction(
    	[]{
            //Hush gong
            _audioSystem.fadeAllSounds();

		    // set up the cameras *after* game_begin_module() or the player devices will not be initialized
		    // and camera_system_begin() will not set up thte correct view
		    _cameraSystem.begin(local_stats.player_count);

		    // make sure the cameras are centered on something or there will be a graphics error
		    _cameraSystem.resetAllTargets(PMesh);

		    obj_BSP_system_begin(getMeshBSP()); 

			_gameEngine->setGameState(std::make_shared<PlayingState>());
    	});
    addComponent(startButton);

    //Fade out music when finished loading
    _audioSystem.stopMusic();

    //Hit that gong
    _audioSystem.playSoundFull(_audioSystem.getGlobalSound(GSND_GAME_READY));
}


bool LoadingState::loadGlobalHints()
{
    // Open the file with all the tips
    vfs_FILE *fileread = vfs_openRead( "mp_data/gametips.txt" );
    if ( NULL == fileread )
    {
        log_warning( "Could not load the game tips and hints. (\"mp_data/gametips.txt\")\n" );
        return false;
    }

    // Load the data
    while (!vfs_eof(fileread) && goto_colon_vfs(NULL, fileread, true))
    {
        STRING buffer;

        //Read the line
        vfs_get_string(fileread, buffer, SDL_arraysize(buffer));

        //Make it look nice
        str_decode(buffer, SDL_arraysize(buffer), buffer);
        str_add_linebreaks(buffer, SDL_arraysize(buffer), 50);

        _globalGameTips.push_back(buffer);
    }

    vfs_close(fileread);

    if(_globalGameTips.empty()) {
        log_warning( "Could not load the game tips and hints. (\"basicdat/gametips.txt\")\n" );
    }
 
    return !_globalGameTips.empty();
}

bool LoadingState::loadLocalModuleHints()
{
  STRING buffer;

    // Open all the tips
    snprintf(buffer, SDL_arraysize( buffer ), "mp_modules/%s/gamedat/gametips.txt", _loadModule->getName());
    vfs_FILE *fileread = vfs_openRead( buffer );
    if ( NULL == fileread ) return false;

    // Load the data
    while (!vfs_eof(fileread) && goto_colon_vfs(NULL, fileread, true))
    {

        //Read the line
        vfs_get_string(fileread, buffer, SDL_arraysize(buffer));

        //Make it look nice
        str_decode(buffer, SDL_arraysize(buffer), buffer);
        str_add_linebreaks(buffer, SDL_arraysize(buffer), 50);

        _localGameTips.push_back(buffer);
    }

    vfs_close( fileread );

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
