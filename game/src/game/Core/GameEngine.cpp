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
#include "egolib/Audio/AudioSystem.hpp"
#include "game/Graphics/CameraSystem.hpp"
#include "game/GameStates/MainMenuState.hpp"
#include "game/Profiles/_Include.hpp"
#include "game/GUI/UIManager.hpp"
#include "game/graphic.h"
#include "game/renderer_2d.h"
#include "game/graphic_texture.h"
#include "game/game.h"
#include "game/collision.h"
#include "egolib/egolib.h"
#include "game/Entities/ParticleHandler.hpp"

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
            //terminate the program
            shutdown();
            break;
        }

		//Check if it is time to update everything
		for(_frameSkip = 0; _frameSkip < MAX_FRAMESKIP && SDL_GetTicks() >= _updateTimeout; ++_frameSkip)
		{
			updateOneFrame();
            _updateTimeout += DELAY_PER_UPDATE_FRAME;
		}

		//Check if it is time to draw everything
		if(SDL_GetTicks() >= _renderTimeout)
		{
            //Calculate estimations for FPS and UPS
            estimateFrameRate();

            //Draw the current frame
			renderOneFrame();

            //Stabilize FPS throttle every so often in case rendering is lagging behind
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
	//Fall through to next state if needed
    if(_currentGameState->isEnded())
    {
        _gameStateStack.pop_front();

        //No more states? Default back to main menu
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

    //Handle all SDL events    
    pollEvents();

    _currentGameState->update();

    //Check for screenshots
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
    static int y = 20;

    _uiManager->beginRenderUI();
        _uiManager->getDefaultFont()->drawTextBox(text, 20, y, 800, 600, 25);
    _uiManager->endRenderUI();

    gfx_request_flip_pages();
    gfx_do_flip_pages();

    y += 25;
}

bool GameEngine::initialize()
{
	//Initialize logging next, so that we can use it everywhere.
    log_init("/debug/log.txt", LOG_DEBUG);

    // start initializing the various subsystems
    log_message("Starting Egoboo %s ...\n", GAME_VERSION.c_str());
    log_info("PhysFS file system version %s has been initialized...\n", vfs_getVersion());

    //Initialize OS specific stuff
    sys_initialize();

    // read the "setup.txt" file
    setup_read_vfs();

    // download the "setup.txt" values into the cfg struct
    loadConfiguration(true);

    //Initialize SDL
    log_info( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
    if ( SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTTHREAD) < 0 )
    {
        log_message( "Failure!\n" );
        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    // do basic system initialization
    input_system_init();

    //Initialize graphics
    gfx_system_begin();
    log_info("Initializing SDL_image version %d.%d.%d... ", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL);
    GLSetup_SupportedFormats();
    gfx_system_init_all_graphics();
    gfx_do_clear_screen();

    // setup the system gui
    _uiManager = std::unique_ptr<UIManager>(new UIManager());

    //Tell them we are loading the game (This is earliest point we can render text to screen)
    renderPreloadText("Initializing game...");
    
#ifdef __MACOSX__
    // Run the Cocoa event loop a few times so the window appears
    for (int i = 0; i < 4; i++) SDL_PumpEvents();
#endif

    // Load basic textures
    gfx_system_load_basic_textures();

    // Initialize the sound system.
    renderPreloadText("Loading audio...");
    _audioSystem.initialize(cfg);
    _audioSystem.loadAllMusic();
    _audioSystem.playMusic(AudioSystem::MENU_SONG);
    _audioSystem.loadGlobalSounds();


    renderPreloadText("Configurating game data...");

	// synchronize the config values with the various game subsystems
    // do this after the ego_init_SDL() and gfx_system_init_OpenGL() in case the config values are clamped
    // to valid values
    loadConfiguration(false);
    config_synch(&cfg, false);

    // read all the scantags
    scantag_read_all_vfs("mp_data/scancode.txt");

    // load input
    input_settings_load_vfs("/controls.txt", -1);

    // initialize the random treasure system
    init_random_treasure_tables_vfs("mp_data/randomtreasure.txt");

    // initialize the console
    egolib_console_begin();

    // Initialize the profile system.
    _profileSystem.initialize();

    // Initialize the collision system.
    CollisionSystem::initialize();

    // Initialize the model system.
    model_system_begin();
    ego_mesh_t::ctor(PMesh);


    renderPreloadText("Loading modules...");
    _profileSystem.loadModuleProfiles();

    renderPreloadText("Loading save games...");
    _profileSystem.loadAllSavedCharacters("mp_players");



    // clear out the import and remote directories
    renderPreloadText("Finished!");
    vfs_empty_temp_directories();

    //Start the main menu
    pushGameState(std::make_shared<MainMenuState>());

    return true;
}

void GameEngine::uninitialize()
{
    log_info( "memory_cleanUp() - Attempting to clean up loaded things in memory... " );

    // synchronize the config values with the various game subsystems
    config_synch(&cfg, true);

    // quit the setup system, making sure that the setup file is written
    setup_write_vfs();
    setup_end();

    // delete all the graphics allocated by SDL and OpenGL
    gfx_system_delete_all_graphics();

    // make sure that the current control configuration is written
    input_settings_save_vfs( "controls.txt", -1 );

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
    _profileSystem.uninitialize();

    // Uninitialize the audio system.
    _audioSystem.uninitialize();

    // shut down the log services
    log_message( "Success!\n" );
    log_info("Exiting Egoboo %s the good way...\n", GAME_VERSION.c_str());
    log_shutdown();

    //Shutdown SDL last
	SDL_Quit();
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

bool GameEngine::loadConfiguration(bool syncFromFile)
{
    // synchronize settings from a pre-loaded setup.txt? (this will load setup.txt into *pcfg)
    if (syncFromFile)
    {
        if (!setup_download(&cfg)) return false;
    }

    // status display
    StatusList.on = cfg.show_stats;

    // fps display
    fpson = cfg.fps_allowed;

    // message display
    DisplayMsg_count = Math::constrain(cfg.message_count_req, EGO_MESSAGE_MIN, EGO_MESSAGE_MAX);
    DisplayMsg_on    = cfg.message_count_req > 0;

    // Adjust the particle limit.
    ParticleHandler::get().setDisplayLimit(cfg.particle_count_req);

    // camera options
    _cameraSystem.getCameraOptions().turnMode = cfg.autoturncamera;

    // sound options
    _audioSystem.setConfiguration(cfg);

    // renderer options
    gfx_config_download_from_egoboo_config(&gfx, &cfg);

    // texture options
    oglx_texture_parameters_download_gfx(&tex_params, &cfg);

    return true;
}

void GameEngine::pollEvents()
{
    // message processing loop
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        //Console has first say in events
        if (cfg.dev_mode)
        {
            if (NULL == egolib_console_handle_events(&event))
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
        try
        {
            // initialize the virtual filesystem first
            vfs_init(argv[0], nullptr);
            setup_init_base_vfs_paths();

            _gameEngine = std::unique_ptr<GameEngine>(new GameEngine());

            _gameEngine->start();
        }
        catch (Ego::Exception& ex)
        {
            std::cerr << "unhandled exception: " << std::endl
                      << (std::string)ex << std::endl;
            return EXIT_FAILURE;
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "unhandled exception: " << std::endl
                  << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
