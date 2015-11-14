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

#include "egolib/Core/System.hpp"
#include "egolib/egoboo_setup.h"

namespace Ego
{
namespace Core
{

TimerService::TimerService()
{
	Log::get().info("Intializing SDL timer services ... ");
    if (SDL_InitSubSystem(SDL_INIT_TIMER) < 0)
    {
		Log::get().message(" failure!\n");
        Id::EnvironmentErrorException error(__FILE__, __LINE__, "SDL timer", SDL_GetError());
		Log::get().error("%s\n", ((std::string)error).c_str());
        throw error;
    }
    else
    {
		Log::get().message(" success!\n");
    }

}

TimerService::~TimerService()
{
    SDL_QuitSubSystem(SDL_INIT_TIMER);
}

uint32_t TimerService::getTicks()
{
    return SDL_GetTicks();
}

EventService::EventService()
{
	Log::get().info("Intializing SDL event queue services ... ");
    if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0)
    {
		Log::get().message(" failure!\n");
        Id::EnvironmentErrorException error(__FILE__, __LINE__, "SDL events", SDL_GetError());
		Log::get().error("%s\n", ((std::string)error).c_str());
        throw error;
    }
    else
    {
		Log::get().message(" success!\n");
    }
}

EventService::~EventService()
{
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
}

VideoService::VideoService()
{
	Log::get().info("Intializing SDL Video ... ");
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
	{
		Log::get().message(" failure!\n");
		Id::EnvironmentErrorException error(__FILE__, __LINE__, "SDL Video", SDL_GetError());
		Log::get().error("%s\n", ((std::string)error).c_str());
		throw error;
	}
	else
	{
		Log::get().message(" success!\n");
	}
}

VideoService::~VideoService()
{
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

AudioService::AudioService()
{
	Log::get().info("Intializing SDL Audio ... ");
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		Log::get().message(" failure!\n");
		Id::EnvironmentErrorException error(__FILE__, __LINE__, "SDL Audio", SDL_GetError());
		Log::get().error("%s\n", ((std::string)error).c_str());
		throw error;
	}
	else
	{
		Log::get().message(" success!\n");
	}
}

AudioService::~AudioService()
{
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

InputService::InputService()
{
	Log::get().info("Intializing SDL Joystick/GameController/Haptic ... ");
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) < 0)
	{
		Log::get().message(" failure!\n");
		Id::EnvironmentErrorException error(__FILE__, __LINE__, "SDL Joystick/GameController/Haptic", SDL_GetError());
		Log::get().error("%s\n", ((std::string)error).c_str());
		throw error;
	}
	else
	{
		Log::get().message(" success!\n");
	}
}

InputService::~InputService()
{
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
}

const std::string System::VERSION = "0.1.9";

System *System::_singleton = nullptr;

System::System(const char *binaryPath, const char *egobooPath)
{
    // Initialize the virtual file system.
    vfs_init(binaryPath, egobooPath);
    /*
    // Uncomment to display the search paths.
    vfs_listSearchPaths();
    */
    setup_init_base_vfs_paths();
    /*
    // Uncomment to display the search paths.
    vfs_listSearchPaths();
    */
    // Initialize logging, so that we can use it everywhere.
    Log::initialize("/debug/log.txt", Log::Level::Debug);

    // Start initializing the various subsystems.
	Log::get().message("Starting Egoboo Engine %s\n", VERSION.c_str());
	Log::get().info("PhysFS file system version %s has been initialized...\n", vfs_getVersion());

    // Load "setup.txt".
    setup_begin();

    // Download "setup.txt" into the Egoboo configuration.
    setup_download(&egoboo_config_t::get());

    // Initialize SDL.
	Log::get().message("Initializing SDL version %d.%d.%d ... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    try
    {
        _timerService = std::unique_ptr<TimerService>(new TimerService());
    }
    catch (...)
    {
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
    try
    {
        _eventService = new EventService();
    }
    catch (...)
    {
        _timerService.reset();
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
	try
	{
		_videoService = new VideoService();
	}
	catch (...)
	{
		delete _eventService;
		_eventService = nullptr;
        _timerService.reset();
		SDL_Quit();
		setup_end();
		/*sys_uninitialize();*/
		Log::uninitialize();
		/*vfs_uninitialize();*/
		std::rethrow_exception(std::current_exception());
	}
	try
	{
		_audioService = new AudioService();
	}
	catch (...)
	{
		delete _videoService;
		_videoService = nullptr;
		delete _eventService;
		_eventService = nullptr;
        _timerService.reset();
		SDL_Quit();
		setup_end();
		/*sys_uninitialize();*/
		Log::uninitialize();
		/*vfs_uninitialize();*/
		std::rethrow_exception(std::current_exception());
	}
	try
	{
		_inputService = new InputService();
	}
	catch (...)
	{
		delete _audioService;
		_audioService = nullptr;
		delete _videoService;
		_videoService = nullptr;
		delete _eventService;
		_eventService = nullptr;
        _timerService.reset();
		SDL_Quit();
		setup_end();
		/*sys_uninitialize();*/
		Log::uninitialize();
		/*vfs_uninitialize();*/
		std::rethrow_exception(std::current_exception());
	}
}

System::~System()
{
	delete _inputService;
	_inputService = nullptr;
	delete _audioService;
	_audioService = nullptr;
	delete _videoService;
	_videoService = nullptr;
    delete _eventService;
    _eventService = nullptr;
    _timerService.reset();
    setup_end();
    /*sys_uninitialize();*/
	Log::get().message("Exiting Egoboo Engine %s.\n", VERSION.c_str());
    Log::uninitialize();
    setup_clear_base_vfs_paths();
    /*vfs_uninitialize();*/
}

System& System::get()
{
    if (!_singleton)
    {
        throw std::logic_error("system not initialized");
    }
    return *_singleton;
}

void System::initialize(const char *binaryPath, const char *egobooPath)
{
    if (_singleton)
    {
        return;
    }
    _singleton = new System(binaryPath, egobooPath);
}

void System::uninitialize()
{
    if (!_singleton)
    {
        return;
    }
    delete _singleton;
    _singleton = nullptr;
}

} // namespace Core
} // namespace Ego
