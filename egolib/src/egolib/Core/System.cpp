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
#include "egolib/Core/EnvironmentError.hpp"
#include "egolib/system.h"
#include "egolib/egoboo_setup.h"

namespace Ego
{
namespace Core
{

TimerService::TimerService()
{
    log_info("Intializing SDL timer services ... ");
    if (SDL_InitSubSystem(SDL_INIT_TIMER) < 0)
    {
        log_message(" failure!\n");
        EnvironmentError error(__FILE__, __LINE__, "SDL timer", SDL_GetError());
        log_error("%s\n", ((std::string)error).c_str());
        throw error;
    }
    else
    {
        log_message(" success!\n");
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
    log_info("Intializing SDL event threading services ... ");
    if (SDL_InitSubSystem(SDL_INIT_EVENTTHREAD) < 0)
    {
        log_message(" failure!\n");
        EnvironmentError error(__FILE__, __LINE__, "SDL events", SDL_GetError());
        log_error("%s\n", ((std::string)error).c_str());
        throw error;
    }
    else
    {
        log_message(" success!\n");
    }
}

EventService::~EventService()
{
    SDL_QuitSubSystem(SDL_INIT_EVENTTHREAD);
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
    log_initialize("/debug/log.txt", LOG_DEBUG);
    // Initialize system-dependent elements.
    sys_initialize();
    // Start initializing the various subsystems.
    log_message("Starting Egoboo Engine %s\n", VERSION.c_str());
    log_info("PhysFS file system version %s has been initialized...\n", vfs_getVersion());
    // Load "setup.txt".
    setup_begin();
    // Download "setup.txt" into the Egoboo configuration.
    setup_download(&egoboo_config_t::get());
    // Initialize SDL.
    log_message("Initializing SDL version %d.%d.%d ... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    try
    {
        _timerService = new TimerService();
    }
    catch (...)
    {
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        log_uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
    try
    {
        _eventService = new EventService();
    }
    catch (...)
    {
        delete _timerService;
        _timerService = nullptr;
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        log_uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
}

System::~System()
{
    delete _eventService;
    _eventService = nullptr;
    delete _timerService;
    _timerService = nullptr;
    setup_end();
    /*sys_uninitialize();*/
    log_message("Exiting Egoboo Engine %s.\n", VERSION.c_str());
    log_uninitialize();
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
