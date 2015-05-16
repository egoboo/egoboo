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
#include "egolib/Profiles/_Include.hpp"
#include "game/GUI/UIManager.hpp"
#include "game/graphic.h"
#include "game/renderer_2d.h"
#include "game/game.h"
#include "game/collision.h"
#include "game/Entities/_Include.hpp"

//Global singelton
std::unique_ptr<GameEngine> _gameEngine;

//Declaration of class constants
const uint32_t GameEngine::GAME_TARGET_FPS;
const uint32_t GameEngine::GAME_TARGET_UPS;

const uint32_t GameEngine::DELAY_PER_RENDER_FRAME;
const uint32_t GameEngine::DELAY_PER_UPDATE_FRAME;

const uint32_t GameEngine::MAX_FRAMESKIP;

const std::string GameEngine::GAME_VERSION = "2.9.0";

GameEngine::GameEngine() :
	_isInitialized(false),
	_terminateRequested(false),
	_updateTimeout(0),
	_renderTimeout(0),
	_gameStateStack(),
	_currentGameState(nullptr),
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

void GameEngine::start()
{
    initialize();

    //Initialize clock timeout	
    _updateTimeout = SDL_GetTicks() + DELAY_PER_UPDATE_FRAME;
    _renderTimeout = SDL_GetTicks() + DELAY_PER_RENDER_FRAME;

    while(!_terminateRequested)
    {
        // Test the panic button
        if ( SDL_KEYDOWN( keyb, SDLK_q ) && SDL_KEYDOWN( keyb, SDLK_LCTRL ) )
        {
            // Terminate the program
            shutdown();
            break;
        }

        // Check if it is time to update everything
        for(_frameSkip = 0; _frameSkip < MAX_FRAMESKIP && SDL_GetTicks() >= _updateTimeout; ++_frameSkip)
        {
            updateOneFrame();
            _updateTimeout += DELAY_PER_UPDATE_FRAME;
        }

        // Check if it is time to draw everything
        if(SDL_GetTicks() >= _renderTimeout)
        {
            // Calculate estimations for FPS and UPS
            estimateFrameRate();

            // Draw the current frame
            renderOneFrame();

            // Stabilize FPS throttle every so often in case rendering is lagging behind
            if(game_frame_all % GAME_TARGET_FPS == 0)
            {
                _renderTimeout = SDL_GetTicks() + DELAY_PER_RENDER_FRAME;
            }
            else
            {
                _renderTimeout += DELAY_PER_RENDER_FRAME;
            }
        }
        else
        {
            //Don't hog CPU if we have nothing to do
            int delay = std::min<int>(_renderTimeout-SDL_GetTicks(), _updateTimeout-SDL_GetTicks());
            if(delay > 1)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
        }
    }

    uninitialize();
}

void GameEngine::estimateFrameRate()
{
    const float dt = (SDL_GetTicks()-_lastFrameEstimation) * 0.001f;

    //Throttle estimations to four times per second
    if(dt < 0.25f) {
        return;
    }

    _estimatedFPS = (game_frame_all-_lastFPSCount) / dt;
    _estimatedUPS = (update_wld-_lastUPSCount) / dt;

    _lastFPSCount = game_frame_all;
    _lastUPSCount = update_wld;
    _lastFrameEstimation = SDL_GetTicks();
}

void GameEngine::updateOneFrame()
{
    // Fall through to next state if needed
    if(_currentGameState->isEnded())
    {
        _gameStateStack.pop_front();

        // No more states? Default back to main menu
        if(_gameStateStack.empty())
        {
            pushGameState(std::make_shared<MainMenuState>());
        }
        else
        {
            _currentGameState = _gameStateStack.front();
            _currentGameState->beginState();
            _updateTimeout = SDL_GetTicks() + DELAY_PER_UPDATE_FRAME;
            _renderTimeout = SDL_GetTicks() + DELAY_PER_RENDER_FRAME;
        }
    }

    // Handle all SDL events    
    pollEvents();

    _currentGameState->update();

    // Check for screenshots
    if (SDL_KEYDOWN(keyb, SDLK_F11))
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
                log_warning("Error writing screenshot\n");      // Log the error in log.txt
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
    // Status display.
    StatusList.on = egoboo_config_t::get().hud_displayStatusBars.getValue();

    // Message display.
    DisplayMsg_count = Ego::Math::constrain(egoboo_config_t::get().hud_simultaneousMessages_max.getValue(),
        (uint8_t)EGO_MESSAGE_MIN, (uint8_t)EGO_MESSAGE_MAX);
    DisplayMsg_on = egoboo_config_t::get().hud_simultaneousMessages_max.getValue() > 0;

    // Adjust the particle limit.
    ParticleHandler::get().setDisplayLimit(egoboo_config_t::get().graphic_simultaneousParticles_max.getValue());
    

    // camera options
    CameraSystem::getCameraOptions().turnMode = egoboo_config_t::get().camera_control.getValue();

    // renderer options
    gfx_config_t::download(&gfx, &egoboo_config_t::get());

    // texture options
    oglx_texture_parameters_t::download(&g_ogl_textureParameters, &egoboo_config_t::get());

    // <<<
    /* ********************************************************************************** */

    // do basic system initialization
    input_system_init();

    // Initialize the image manager.
    ImageManager::initialize();

    // Initialize GFX system.
    GFX::initialize();
    gfx_system_init_all_graphics();
    gfx_do_clear_screen();

    // setup the system gui
    _uiManager = std::unique_ptr<UIManager>(new UIManager());

    //Tell them we are loading the game (This is earliest point we can render text to screen)
    renderPreloadText("Initializing game...");
    
#ifdef ID_OSX
    // Run the Cocoa event loop a few times so the window appears
    for (int i = 0; i < 4; i++) SDL_PumpEvents();
#endif

    // Load basic textures
    gfx_system_load_basic_textures();

    // Initialize the sound system.
    renderPreloadText("Loading audio...");
    AudioSystem::initialize();
    auto& audioSystem = AudioSystem::get();
    audioSystem.loadAllMusic();
    audioSystem.playMusic(AudioSystem::MENU_SONG);
    audioSystem.loadGlobalSounds();


    renderPreloadText("Configurating game data...");


    // synchronize the config values with the various game subsystems
    // do this after the ego_init_SDL() and gfx_system_init_OpenGL() in case the config values are clamped
    // to valid values
    config_synch(&egoboo_config_t::get(), false, false);

    // read all the scantags
    scantag_read_all_vfs("mp_data/scancode.txt");

    // load input
    input_settings_load_vfs("/controls.txt", -1);

    // initialize the random treasure system
    init_random_treasure_tables_vfs("mp_data/randomtreasure.txt");

    // Initialize the console.
    egolib_console_handler_t::initialize();

    // Initialize the profile system.
    ProfileSystem::initialize();

    // Initialize the collision system.
    CollisionSystem::initialize();

    // Initialize the model system.
    model_system_begin();
    ego_mesh_t::ctor(PMesh);


    renderPreloadText("Loading modules...");
    ProfileSystem::get().loadModuleProfiles();

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
    log_message("Uninitializing Egoboo %s\n",GAME_VERSION.c_str());

    _gameStateStack.clear();
    _currentGameState.reset();
    PMod.release();

    // synchronize the config values with the various game subsystems
    config_synch(&egoboo_config_t::get(), true, true);

    // delete all the graphics allocated by SDL and OpenGL
    gfx_system_delete_all_graphics();

    // make sure that the current control configuration is written
    input_settings_save_vfs("controls.txt", -1);

    // @todo This should be 'UIManager::uninitialize'.
    _uiManager.reset(nullptr);

    // Uninitialize the collision system.
    CollisionSystem::uninitialize();

    // Uninitialize the scripting system.
    scripting_system_end();

    // Deallocate all dynamically allocated memory for characters, particles, enchants, and models.
    _gameObjects.clear();
    model_system_end();

    // Uninitialize the profile system.
    ProfileSystem::uninitialize();

    // Uninitialize the console.
    egolib_console_handler_t::uninitialize();

    // Uninitialize the audio system.
    AudioSystem::uninitialize();

    // Uninitialize the GFX system.
    GFX::uninitialize();

    // Uninitialize the image manager.
    ImageManager::uninitialize();

    // Shut down the log services.
    log_message("Exiting Egoboo %s. See you next time\n", GAME_VERSION.c_str());
}

void GameEngine::setGameState(std::shared_ptr<GameState> gameState)
{
    _gameStateStack.clear();
    pushGameState(gameState);
}

void GameEngine::pushGameState(std::shared_ptr<GameState> gameState)
{
    _gameStateStack.push_front(gameState);
    _currentGameState = _gameStateStack.front();
    _currentGameState->beginState();
    _updateTimeout = SDL_GetTicks() + DELAY_PER_UPDATE_FRAME;
    _renderTimeout = SDL_GetTicks() + DELAY_PER_RENDER_FRAME;
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
            if (!egolib_console_handler_t::handle_event(&event))
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

            case SDL_ACTIVEEVENT:
                switch(event.active.type)
                {
                    case SDL_APPACTIVE:
                        if(1 == event.active.gain)
                        {
                            // the application has recovered from being minimized
                            // the textures need to be reloaded into OpenGL memory
                            gfx_system_reload_all_textures();
                        }
                    break;

                    case SDL_APPMOUSEFOCUS:
                        // gained or lost mouse focus
                        mous.on = (1 == event.active.gain) ? true : false;
                    break;

                    case SDL_APPINPUTFOCUS:
                        // gained or lost keyboard focus
                        keyb.on = (1 == event.active.gain) ? true : false;
                    break;
                }
            break;

            case SDL_VIDEORESIZE:
                if ( SDL_VIDEORESIZE == event.resize.type )
                {
                    // The video has been resized.
                    // If the game is active, some camera info mught need to be recalculated
                    // and possibly the auto-formatting for the menu system and the ui system

                    // grab all the new SDL screen info
                    SDLX_Get_Screen_Info(&sdl_scr, SDL_FALSE);
                }
            break;

            case SDL_VIDEOEXPOSE:
                // something has been done to the screen and it needs to be re-drawn.
                // For instance, a window above the app window was moved. This has no
                // effect on the game at the moment.
            break;

            case SDL_MOUSEBUTTONDOWN:
                if ( event.button.button == SDL_BUTTON_WHEELUP )
                {
                    _currentGameState->notifyMouseScrolled(1);
                    input_cursor.z++;
                    input_cursor.wheel_event = true;
                }
                else if ( event.button.button == SDL_BUTTON_WHEELDOWN )
                {
                    _currentGameState->notifyMouseScrolled(-1);
                    input_cursor.z--;
                    input_cursor.wheel_event = true;
                }
                else
                {
                    _currentGameState->notifyMouseClicked(event.button.button, event.button.x, event.button.y);
                    input_cursor.pending_click = true;
                }
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
