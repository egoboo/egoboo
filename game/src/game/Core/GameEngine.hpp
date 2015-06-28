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
class UIManager;
class PlayingState;

class GameEngine
{
public:
    static const uint32_t GAME_TARGET_FPS = 60;	///< Desired frame renders per second
    static const uint32_t GAME_TARGET_UPS = 50;	///< Desired game logic updates per second

    static const uint32_t DELAY_PER_RENDER_FRAME = 1000 / GAME_TARGET_FPS; ///< milliseconds between each render
    static const uint32_t DELAY_PER_UPDATE_FRAME = 1000 / GAME_TARGET_UPS; ///< milliseconds between each update

    static const uint32_t MAX_FRAMESKIP = 10;	///< Maximum render frames to skip if logic updates are lagging behind

    static const std::string GAME_VERSION;		///< Version of the game

    /**
    * @brief
    *	Default constructor of a GameEngine. Actual initialization, allocation and loading is
    *	done by the private initialize() function which is called by start()
    **/
    GameEngine();

    /**
    * @brief
    *	A blocking function that initializes the GameEngine and enders the MainLoop until
    *	shutdown() is called, at which point it will deinitialize and terminate the program.
    *	This function should only be called by the main function that creates the GameEngine.
    **/
    void start();

    /**
    * @return
    *	true if the GameEngine is currently running and is not terminated
    **/
    inline bool isRunning() const {
        return !_terminateRequested;
    }

    /**
    * @brief
    *	Tells the GameEngine it should shutdown and exit. The GameEngine will try to
    *	deallocate and release all resources in a clean program exit.
    **/
    void shutdown();

    /**
    * @brief
    *	Clears the GameState stack and sets the current GameState to the specified
    *	state. Note that if the Stack is empty, the GameEngine will default to
    *  	the MainMenuState.
    **/
    void setGameState(std::shared_ptr<GameState> gameState);

    /**
    * @brief
    *	Pushes a new GameState on the stack and sets it as the current state.
    *	When the pushed state ends, the previous element on the stack will become
    *	the current state.
    **/
    void pushGameState(std::shared_ptr<GameState> gameState);

    /**
    * @return
    *	Get estimated number of Frame Renders Per Second
    **/
    float getFPS() const;

    /**
    * @return
    *	Get estimated number of Logic Updates Per Second.
    *	This should be around 50 (GAME_TARGET_UPS)
    **/
    float getUPS() const;

    /**
    * @return
    *	Gets number of render frames that were skipped last update loop so that
    *	the Game Logic Loop could catch up. This value should optimally be zero
    *	and is never higher than MAX_FRAMESKIP.
    **/
    int getFrameSkip() const;

    /**
    * @brief
    *	Requests the GameEngine to generate a screenshot from the next frame that
    *	will be rendered and store it to a image file. Calling this function
    *	multiple times in the same frame has no effect (only one screenshot will be made).
    **/
    void requestScreenshot() {
        _screenshotRequested = true;
    }

    /**
    * @brief
    * 	Tell the game engine that it is allowed to render a mouse cursor
    **/
    void enableMouseCursor() {
        _drawCursor = true;
    }

    /**
    * @brief
    * 	Tell the game engine that it should not draw a mouse cursor
    **/
    void disableMouseCursor() {
        _drawCursor = false;
    }

    /**
    * @brief
    *	Get instance of the UIManager associated with the current GameEngine
    **/
    inline const std::unique_ptr<UIManager>& getUIManager() const {
        return _uiManager;
    }

    /**
    * @brief
    *   Get high resolution timestamp of when the GameEngine was booted with the start() function
    **/
    inline const std::chrono::high_resolution_clock::time_point& getBootTime() const {
        return _startupTimestamp;
    }

    /**
    * @brief
    *   Gets the current GameState as a PlayingState instance
    * @return
    *   the current GameState if it is a PlayingState or nullptr otherwise
    **/
    std::shared_ptr<PlayingState> getActivePlayingState() const;

private:
    /**
    * @brief
    *	Run one update frame of the current GameState. Also handles SDL_Events such as input
    *	and controls the GameState flow. Input events will be propogated through any GUIComponents
    *	contained in the current active GameState.
    **/
    void updateOneFrame();

    /**
    * @brief
    *	Render the current frame of the active GameState. Will first render the GameState itself
    *	and will then render all GUI components on top of it. Only rendering should be done in this
    *	method and no game logic updates.
    **/
    void renderOneFrame();

    /**
    * @brief
    *	Initializes all SDL subsystems and loads settings and any resources before the game is started.
    **/
    bool initialize();

    /// @details This function releases all loaded things in memory and cleans up everything properly
    void uninitialize();

    /**
    * @brief
    *	Handles all SDL events queued in the SDL FIFO and propogates any relevant input events to the
    *	active GameState.
    **/
    void pollEvents();

    /**
    * @brief
    *	Recalculate the estimated FPS and UPS value. Should be called each game loop iteration.
    **/
    void estimateFrameRate();

    /**
    * @brief
    *	A small hacky function to render text before the game actually starts, so that the player knows
    *	the game is actually booting and is not hanging.
    **/
    void renderPreloadText(const std::string &text);

private:
    std::chrono::high_resolution_clock::time_point _startupTimestamp;
    bool _isInitialized;
    bool _terminateRequested;		///< true if the GameEngine should deinitialize and shutdown
    uint32_t _updateTimeout;		///< Timestamp when updateOneFrame() should be run again
    uint32_t _renderTimeout;		///< Timestamp when renderOneFrame() should be run again
    std::forward_list<std::shared_ptr<GameState>> _gameStateStack;
    std::shared_ptr<GameState> _currentGameState;
    std::mutex _gameStateMutex;
    egoboo_config_t _config;
    bool _drawCursor;
    bool _screenshotReady;
    bool _screenshotRequested;

    //For estimating frame rates
    uint32_t _lastFrameEstimation;
    int _frameSkip;
    uint32_t _lastFPSCount;
    uint32_t _lastUPSCount;
    float _estimatedFPS;
    float _estimatedUPS;

    //GameEngine Submodules
    std::unique_ptr<UIManager> _uiManager;
};

extern std::unique_ptr<GameEngine> _gameEngine;

//TODO: remove these globals
extern std::unique_ptr<GameModule> _currentModule;
extern ego_mesh_t *PMesh;
extern status_list_t StatusList;
