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
#include "game/gamestates/SelectModuleState.hpp"
#include "game/core/GameEngine.hpp"
#include "egolib/egoboo_setup.h"
#include "game/ui.h"
#include "game/graphic.h"
#include "game/gui/Button.hpp"

//For loading stuff
#include "game/graphics/CameraSystem.hpp"
#include "game/profiles/ProfileSystem.hpp"
#include "game/game.h"
#include "game/graphic_billboard.h"
#include "game/link.h"
#include "game/collision.h"
#include "game/renderer_2d.h"
#include "game/bsp.h"

LoadingState::LoadingState(std::shared_ptr<MenuLoadModuleData> module, const std::list<std::shared_ptr<LoadPlayerElement>> &players) :
	_finishedLoading(false),
	_loadingThread(),
	_loadModule(module)
	//_players(players)
{
	//ctor
}

LoadingState::~LoadingState()
{
	//Wait until thread is dead
	if(_loadingThread.joinable()) {
		_loadingThread.join();
	}
}

void LoadingState::update()
{
	if(!_finishedLoading) {
		loadModuleData();
	}
}

void LoadingState::drawContainer()
{
	ui_beginFrame(0);
	{
	    draw_mouse_cursor();
	}
	ui_endFrame();
}

void LoadingState::beginState()
{
	//Start the loading thread
	//_loadingThread = std::thread(&LoadingState::loadModuleData, this);
}

void LoadingState::loadModuleData()
{
	//Make sure all data is cleared first
    game_quit_module();

    BillboardList_init_all();

    //initialize math objects
    make_randie();
    make_turntosin();

    // Linking system
    log_info("Initializing module linking... ");
    if (link_build_vfs( "mp_data/link.txt", LinkList)) log_message("Success!\n");
    else log_message( "Failure!\n" );

    // initialize the collision system
    collision_system_begin();

    //Ready message display
    DisplayMsg_reset();

    // intialize the "profile system"
    _profileSystem.begin();

    // do some graphics initialization
    //make_lightdirectionlookup();
    gfx_system_make_enviro();

    // try to start a new module
    if(!game_begin_module(_loadModule)) {
    	log_warning("Failed to load module!\n");
    	endState();
    	return;
    }

    //Complete!
    _finishedLoading = true;
    //_gameEngine->setGameState(std::make_shared<PlayingState>());


    std::shared_ptr<Button> startButton = std::make_shared<Button>("Start");
    startButton->setSize(200, 30);
    startButton->setPosition(400, 300);
    startButton->setOnClickFunction(
    	[]{
		    // set up the cameras *after* game_begin_module() or the player devices will not be initialized
		    // and camera_system_begin() will not set up thte correct view
		    _cameraSystem.begin(local_stats.player_count);

		    // make sure the cameras are centered on something or there will be a graphics error
		    _cameraSystem.resetAllTargets(PMesh);

		    obj_BSP_system_begin(getMeshBSP()); 

			_gameEngine->setGameState(std::make_shared<PlayingState>());
    	});
    addComponent(startButton);
}