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

#include "game/Core/GameEngine.hpp"
#include "egolib/egolib.h"
#include "game/Graphics/CameraSystem.hpp"
#include "game/GameStates/MainMenuState.hpp"
#include "game/GameStates/PlayingState.hpp"
#include "egolib/Profiles/_Include.hpp"
#include "egolib/FileFormats/Globals.hpp"
#include "game/GUI/UIManager.hpp"
#include "game/graphic.h"
#include "game/renderer_2d.h"
#include "game/game.h"
#include "game/Entities/_Include.hpp"
#include "game/Physics/CollisionSystem.hpp"

//Global singelton
std::unique_ptr<GameEngine> _gameEngine;

//Declaration of class constants
const uint32_t GameEngine::GAME_TARGET_FPS;
const uint32_t GameEngine::GAME_TARGET_UPS;

const uint64_t GameEngine::DELAY_PER_RENDER_FRAME;
const uint64_t GameEngine::DELAY_PER_UPDATE_FRAME;

const uint32_t GameEngine::MAX_FRAMESKIP;

const std::string GameEngine::GAME_VERSION = "2.9.0";

GameEngine::GameEngine() :
    _startupTimestamp(),
	_isInitialized(false),
	_terminateRequested(false),
	_updateTimeout(0),
	_renderTimeout(0),
	_gameStateStack(),
	_currentGameState(nullptr),
    _clearGameStateStackRequested(false),
	_config(),
    _drawCursor(true),
    _screenshotReady(true),
    _screenshotRequested(false),

    _lastFrameEstimation(0),
    _frameSkip(0),
    _lastFPSCount(0),
    _lastUPSCount(0),
    _estimatedFPS(GAME_TARGET_FPS),
    _estimatedUPS(GAME_TARGET_UPS),

    //Submodules
    _uiManager(nullptr)
{
    //ctor
}

void GameEngine::shutdown()
{
    _terminateRequested = true;
}

uint64_t GameEngine::getMicros() const
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _startupTimestamp).count();
}

void GameEngine::start()
{    
    initialize();

    //Initialize clock timeout	
    _startupTimestamp = std::chrono::high_resolution_clock::now();
    _updateTimeout = getMicros() + DELAY_PER_UPDATE_FRAME;
    _renderTimeout = getMicros() + DELAY_PER_RENDER_FRAME;

    while(!_terminateRequested)
    {
        // Test the panic button
        const uint8_t *keyboardState = SDL_GetKeyboardState(nullptr);
        if ( keyboardState[SDL_SCANCODE_Q] && keyboardState[SDL_SCANCODE_LCTRL] )
        {
            // Terminate the program
            shutdown();
            break;
        }

        // Check if it is time to update everything
        for(_frameSkip = 0; _frameSkip < MAX_FRAMESKIP && getMicros() >= _updateTimeout; ++_frameSkip)
        {
            updateOneFrame();
            _updateTimeout += DELAY_PER_UPDATE_FRAME;
        }

        //Prevent accumalating more than 1 second of game updates (can happen in severe frame drops or breakpoints while debugging)
        const uint64_t now = getMicros();
        if(now > _updateTimeout + GAME_TARGET_UPS*DELAY_PER_UPDATE_FRAME) {
            _updateTimeout = now + DELAY_PER_UPDATE_FRAME;
            _renderTimeout = now;
        }

        // Check if it is time to draw everything
        if(getMicros() >= _renderTimeout)
        {
            // Calculate estimations for FPS and UPS
            estimateFrameRate();

            // Draw the current frame
            renderOneFrame();

            // Stabilize FPS throttle every so often in case rendering is lagging behind
            if(game_frame_all % GAME_TARGET_FPS == 0)
            {
                _renderTimeout = getMicros() + DELAY_PER_RENDER_FRAME;
            }
            else
            {
                _renderTimeout += DELAY_PER_RENDER_FRAME;
            }
        }
        else
        {
            //Don't hog CPU if we have nothing to do
            int delay = std::min<int64_t>(_renderTimeout-getMicros(), _updateTimeout-getMicros());
            if(delay > 1)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(delay));
            }
        }
    }

    uninitialize();
}

void GameEngine::estimateFrameRate()
{
    const uint64_t now = getMicros();
    const float dt = (now-_lastFrameEstimation) / 1e6f;

    //Throttle estimations to ten times per second
    if(dt < 0.1f) {
        return;
    }

    _estimatedFPS = (game_frame_all-_lastFPSCount) / dt;
    _estimatedUPS = (update_wld-_lastUPSCount) / dt;

    _lastFPSCount = game_frame_all;
    _lastUPSCount = update_wld;
    _lastFrameEstimation = now;
}

void GameEngine::updateOneFrame()
{
    //Handle clearing the game state stack first. Should be done before any GUIComponents
    //become locked by the event or rendering loop
    if(_clearGameStateStackRequested) {
        _gameStateStack.clear();
        _gameStateStack.push_front(_currentGameState);
        _clearGameStateStackRequested = false;
    }

    // Fall through to next state if needed
    while(_currentGameState->isEnded())
    {
        if(!_gameStateStack.empty()) {
            _gameStateStack.pop_front();
        }

        // No more states? Default back to main menu
        if(_gameStateStack.empty())
        {
            pushGameState(std::make_shared<MainMenuState>());
        }
        else
        {
            _currentGameState = _gameStateStack.front();
            _currentGameState->beginState();
            _updateTimeout = getMicros() + DELAY_PER_UPDATE_FRAME;
            _renderTimeout = getMicros() + DELAY_PER_RENDER_FRAME;
        }
    }

    // Handle all SDL events    
    pollEvents();

    //Deferred loading for any textures requested by other threads
    TextureManager::get().updateDeferredLoading();

    //Update current game state
    _currentGameState->update();

    // Check for screenshots
    if (keyb.is_key_down(SDLK_F11))
    {
        requestScreenshot();
    }
}

void GameEngine::renderOneFrame()
{
    // clear the screen
    gfx_request_clear_screen();
    gfx_do_clear_screen();

    _currentGameState->drawAll();
    game_frame_all++;

    //Draw mouse cursor last
    if(_drawCursor)
    {
        draw_mouse_cursor();
    }

    // flip the graphics page
    gfx_request_flip_pages();
    gfx_do_flip_pages();

    //Save screenshot if it has been requested
    if(_screenshotRequested)
    {
        if(_screenshotReady)
        {
            _screenshotReady = false;
            _screenshotRequested = false;
            
            if (!dump_screenshot())
            {
                DisplayMsg_printf("Error writing screenshot!"); // send a failure message to the screen
				Log::get().warn("Error writing screenshot\n");      // Log the error in log.txt
            }
        }
    }
    else
    {
        _screenshotReady = true;
    }
}

void GameEngine::renderPreloadText(const std::string &text)
{
    static std::string preloadText("");

    preloadText += text + "\n";
    
    gfx_request_clear_screen();
    gfx_do_clear_screen();

    _uiManager->beginRenderUI();
        _uiManager->getDefaultFont()->drawTextBox(preloadText, 20, 20, 800, 600, 25);
    _uiManager->endRenderUI();

    gfx_request_flip_pages();
    gfx_do_flip_pages();
}

bool GameEngine::initialize()
{
    /* ********************************************************************************** */
    // >>> This must be done as the crappy old systems do not "pull" their configuration.
    //      More recent systems like video or audio system pull their configuraiton data
    //      by the time they are initialized.

    // Message display.
    DisplayMsg_count = Ego::Math::constrain<uint8_t>(egoboo_config_t::get().hud_simultaneousMessages_max.getValue(), EGO_MESSAGE_MIN, EGO_MESSAGE_MAX);
    DisplayMsg_on = egoboo_config_t::get().hud_simultaneousMessages_max.getValue() > 0;

    // camera options
    CameraSystem::getCameraOptions().turnMode = egoboo_config_t::get().camera_control.getValue();

    // renderer options
    gfx_config_t::download(gfx, egoboo_config_t::get());

    // texture options
    oglx_texture_parameters_t::download(g_ogl_textureParameters, egoboo_config_t::get());

    // <<<
    /* ********************************************************************************** */

    // Initialize the input system.
    InputSystem::initialize();

    // Initialize the image manager.
    ImageManager::initialize();

    // Initialize GFX system.
    GFX::initialize();

	// TODO: REMOVE THIS.
	gfx_system_init_all_graphics();
	gfx_do_clear_screen();

	// Initialize the audio system.
	AudioSystem::initialize();

	// Initialize the particle handler.
	ParticleHandler::initialize();

	// Initialize the console.
	Ego::Core::ConsoleHandler::initialize();


    // load the bitmapped font (must be done after gfx_system_init_all_graphics())
    font_bmp_load_vfs("mp_data/font_new_shadow", "mp_data/font.txt");

    // setup the system gui
    _uiManager = std::unique_ptr<UIManager>(new UIManager());

    //Tell them we are loading the game (This is earliest point we can render text to screen)
    renderPreloadText("Initializing game...");
    
#ifdef ID_OSX
    // Run the Cocoa event loop a few times so the window appears
    for (int i = 0; i < 4; i++) SDL_PumpEvents();
#endif

    // Initialize the sound system.
    renderPreloadText("Loading audio...");
    auto& audioSystem = AudioSystem::get();
    audioSystem.loadAllMusic();
    playMainMenuSong();
    audioSystem.loadGlobalSounds();

    // synchronize the config values with the various game subsystems
    // do this after the ego_init_SDL() and gfx_system_init_OpenGL() in case the config values are clamped
    // to valid values
    renderPreloadText("Configurating game data...");
    config_synch(&egoboo_config_t::get(), false, false);

    // load input
    input_settings_load_vfs("/controls.txt", -1);

    // initialize the random treasure system
    init_random_treasure_tables_vfs("mp_data/randomtreasure.txt");

    // Initialize Perks
    Ego::Perks::PerkHandler::initialize();

    // Initialize the profile system.
    Ego::Core::Singleton<ParticleProfileSystem>::initialize(); //explicit static member call to avoid ambigious call
    ProfileSystem::initialize();

    // Initialize the collision system.
    Ego::Physics::CollisionSystem::initialize();

    // Load all modules
    renderPreloadText("Loading modules...");
    ProfileSystem::get().loadModuleProfiles();

    // Check savegame folder
    renderPreloadText("Loading save games...");
    ProfileSystem::get().loadAllSavedCharacters("mp_players");

    // clear out the import and remote directories
    renderPreloadText("Finished!");
    vfs_empty_temp_directories();

    //Start the main menu
    pushGameState(std::make_shared<MainMenuState>());

    return true;
}

void GameEngine::uninitialize()
{
	Log::get().message("Uninitializing Egoboo %s\n",GAME_VERSION.c_str());

    _gameStateStack.clear();
    _currentGameState.reset();
    _currentModule.release();

    // synchronize the config values with the various game subsystems
    config_synch(&egoboo_config_t::get(), true, true);

    // delete all the graphics allocated by SDL and OpenGL
    gfx_system_release_all_graphics();

    // make sure that the current control configuration is written
    input_settings_save_vfs("controls.txt", -1);

    // @todo This should be 'UIManager::uninitialize'.
    _uiManager.reset(nullptr);

    // Uninitialize the collision system.
    Ego::Physics::CollisionSystem::uninitialize();

    // Uninitialize the scripting system.
    scripting_system_end();

    // Uninitialize the profile system.
    ProfileSystem::uninitialize();

    // Uninitialize the console.
    Ego::Core::ConsoleHandler::uninitialize();

	// Uninitialize the particle handler.
	ParticleHandler::uninitialize();

    // Uninitialize the audio system.
    AudioSystem::uninitialize();

    // Uninitialize the GFX system.
    GFX::uninitialize();

    // Uninitialize the image manager.
    ImageManager::uninitialize();

	// Uninitialize the input system.
	InputSystem::uninitialize();

    // Shut down the log services.
	Log::get().message("Exiting Egoboo %s. See you next time\n", GAME_VERSION.c_str());
}

void GameEngine::setGameState(std::shared_ptr<GameState> gameState)
{
    _clearGameStateStackRequested = true;
    pushGameState(gameState);
}

void GameEngine::pushGameState(std::shared_ptr<GameState> gameState)
{
    _gameStateStack.push_front(gameState);
    _currentGameState = _gameStateStack.front();
    _currentGameState->beginState();
    _updateTimeout = getMicros() + DELAY_PER_UPDATE_FRAME;
    _renderTimeout = getMicros() + DELAY_PER_RENDER_FRAME;
}

void GameEngine::pollEvents()
{
    // message processing loop
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        //Console has first say in events
        if (egoboo_config_t::get().debug_developerMode_enable.getValue())
        {
            if (!Ego::Core::ConsoleHandler::get().handle_event(&event))
            {
                continue;
            }
        }

        // check for messages
        switch (event.type)
        {
            // exit if the window is closed
            case SDL_QUIT:
                shutdown();
                return;
                
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_SHOWN:
                        /// @todo: this shouldn't be needed?
                        gfx_system_reload_all_textures();
                    break;
                    case SDL_WINDOWEVENT_ENTER:
                        mous.on = true;
                    break;
                    case SDL_WINDOWEVENT_LEAVE:
                        mous.on = false;
                    break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        keyb.on = true;
                    break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        keyb.on = false;
                    break;
                    case SDL_WINDOWEVENT_RESIZED:
                        // The video has been resized.
                        // If the game is active, some camera info mught need to be recalculated
                        // and possibly the auto-formatting for the menu system and the ui system
                        
                        // grab all the new SDL screen info
                        SDLX_Get_Screen_Info(sdl_scr, false);
                        break;
                    case SDL_WINDOWEVENT_EXPOSED:
                        // something has been done to the screen and it needs to be re-drawn.
                        // For instance, a window above the app window was moved. This has no
                        // effect on the game at the moment.
                    break;
                        
                }
                break;
                
            case SDL_MOUSEWHEEL:
                _currentGameState->notifyMouseScrolled(event.wheel.y);
            break;
                
            case SDL_MOUSEBUTTONDOWN:
                _currentGameState->notifyMouseClicked(event.button.button, event.button.x, event.button.y);
            break;

            case SDL_MOUSEBUTTONUP:
                _currentGameState->notifyMouseReleased(event.button.button, event.button.x, event.button.y);
            break;
                
            case SDL_MOUSEMOTION:
                mous.x = event.motion.x;
                mous.y = event.motion.y;
                _currentGameState->notifyMouseMoved(event.motion.x, event.motion.y);
            break;
                
            case SDL_KEYDOWN:
                _currentGameState->notifyKeyDown(event.key.keysym.sym);
            break;
        }
    } // end of message processing
}

float GameEngine::getFPS() const
{
    return _estimatedFPS;
}

float GameEngine::getUPS() const
{
    return _estimatedUPS;
}

int GameEngine::getFrameSkip() const
{
    return _frameSkip;
}

std::shared_ptr<PlayingState> GameEngine::getActivePlayingState() const
{
    return std::dynamic_pointer_cast<PlayingState>(_currentGameState);
}


/**
 * @brief
 *  The entry point of the program.
 * @param argc
 *  the number of command-line arguments (number of elements in the array pointed by @a argv)
 * @param argv
 *  the command-line arguments (a static constant array of @a argc pointers to static constant zero-terminated strings)
 * @return
 *  EXIT_SUCCESS upon regular termination, EXIT_FAILURE otherwise
 */
int SDL_main(int argc, char **argv)
{
    try
    {
        Ego::Core::System::initialize(argv[0],nullptr);
        try
        {
            _gameEngine = std::unique_ptr<GameEngine>(new GameEngine());

            _gameEngine->start();
        }
        catch (...)
        {
            Ego::Core::System::uninitialize();
            std::rethrow_exception(std::current_exception());
		}
		Ego::Core::System::uninitialize();
    }
    catch (const Ego::Core::Exception& ex)
    {
        std::cerr << "unhandled exception: " << std::endl
                  << (std::string)ex << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "unhandled exception: " << std::endl
                  << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "unhandled exception" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

uint32_t GameEngine::getCurrentUpdateFrame() const
{
    return update_wld;
}
