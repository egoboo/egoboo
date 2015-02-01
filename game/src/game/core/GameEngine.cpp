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
/// @author Johan Jansen

#include "game/core/GameEngine.hpp"

//Declaration of class constants
const uint32_t GameEngine::TARGET_FPS;
const uint32_t GameEngine::TARGET_UPS;

const uint32_t GameEngine::DELAY_PER_RENDER_FRAME;
const uint32_t GameEngine::DELAY_PER_UPDATE_FRAME;

GameEngine::GameEngine() :
	_isInitialized(false),
	_terminateRequested(false),
	_updateTimeout(0),
	_renderTimeout(0)
{
	//ctor
}

void GameEngine::shutdown()
{
	_terminateRequested = true;
}

void GameEngine::start()
{
	initialize();

	//Initialize clock timeout	
	_updateTimeout = SDL_GetTicks() + DELAY_PER_UPDATE_FRAME;
	_renderTimeout = SDL_GetTicks() + DELAY_PER_RENDER_FRAME;

	while(!_terminateRequested)
	{

		//Check if it is time to update everything
		uint32_t currentTick = SDL_GetTicks();
		if(currentTick >= _updateTimeout)
		{
			_updateTimeout = currentTick + DELAY_PER_UPDATE_FRAME - (currentTick-_updateTimeout);
			updateOneFrame();
		}

		//Check if it is time to draw everything
		currentTick = SDL_GetTicks();
		if(currentTick >= _renderTimeout)
		{
			_renderTimeout = currentTick + DELAY_PER_RENDER_FRAME - (currentTick-_renderTimeout);
			renderOneFrame();
		}

		//Don't hog CPU if we have nothing to do
		currentTick = SDL_GetTicks();
		uint32_t delay = std::min(_renderTimeout-currentTick, _updateTimeout-currentTick);
		if(delay > 1) {
			SDL_Delay(delay);
		}
	}

	uninitialize();
}

void GameEngine::updateOneFrame()
{

}

void GameEngine::renderOneFrame()
{

}

bool GameEngine::initialize()
{

}

void GameEngine::uninitialize()
{

}

