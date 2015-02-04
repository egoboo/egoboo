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
/// @author Zefz aka Johan Jansen
#pragma once

#include "egolib/platform.h"
#include "egolib/egoboo_setup.h"

//Forward declarations
class GameState;
class CameraSystem;
class AudioSystem;
class GameModule;
class ObjectHandler;
struct ego_mesh_t;
struct status_list_t;

class GameEngine
{
public:
	static const uint32_t GAME_TARGET_FPS = 60;	///< Desired frame renders per second
	static const uint32_t GAME_TARGET_UPS = 50;	///< Desired game logic updates per second

	static const uint32_t DELAY_PER_RENDER_FRAME = 1000 / GAME_TARGET_FPS; ///< milliseconds between each render
	static const uint32_t DELAY_PER_UPDATE_FRAME = 1000 / GAME_TARGET_UPS; ///< milliseconds between each update

	GameEngine();

	void start();

	inline bool isRunning() const {return !_terminateRequested;}

	void shutdown();

	void setGameState(std::shared_ptr<GameState> gameState);

	void pushGameState(std::shared_ptr<GameState> gameState);

private:
	void updateOneFrame();

	void renderOneFrame();

	bool initialize();

	/// @details This function releases all loaded things in memory and cleans up everything properly
	void uninitialize();

	bool loadConfiguration(bool syncFromFile);

	void pollEvents();

private:
	bool _isInitialized;
	bool _terminateRequested;		///< true if the GameEngine should deinitialize and shutdown
	uint32_t _updateTimeout;		///< Timestamp when updateOneFrame() should be run again
	uint32_t _renderTimeout;		///< Timestamp when renderOneFrame() should be run again
	std::forward_list<std::shared_ptr<GameState>> _gameStateStack;
	std::shared_ptr<GameState> _currentGameState;
	std::mutex _gameStateMutex;
	egoboo_config_t _config;
};

extern std::unique_ptr<GameEngine> _gameEngine;

//TODO: remove these globals
extern CameraSystem _cameraSystem;
extern AudioSystem  _audioSystem;
extern std::unique_ptr<GameModule> _currentModule;
extern ObjectHandler _gameObjects;
extern ego_mesh_t *PMesh;
extern status_list_t StatusList;
