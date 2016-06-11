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

namespace Ego {
namespace Core {

TimerService::TimerService() {
    Log::get().info("Intializing SDL timer services ... ");
    if (SDL_InitSubSystem(SDL_INIT_TIMER) < 0) {
        Log::get().message(" failure!\n");
        Id::EnvironmentErrorException error(__FILE__, __LINE__, "SDL timer", SDL_GetError());
        Log::get().error("%s\n", ((std::string)error).c_str());
        throw error;
    } else {
        Log::get().message(" success!\n");
    }

}

TimerService::~TimerService() {
    SDL_QuitSubSystem(SDL_INIT_TIMER);
}

uint32_t TimerService::getTicks() {
    return SDL_GetTicks();
}

EventService::EventService() {
    Log::get().info("Intializing SDL event queue services ... ");
    if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0) {
        Log::get().message(" failure!\n");
        Id::EnvironmentErrorException error(__FILE__, __LINE__, "SDL events", SDL_GetError());
        Log::get().error("%s\n", ((std::string)error).c_str());
        throw error;
    } else {
        Log::get().message(" success!\n");
    }
}

EventService::~EventService() {
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
}

VideoService::VideoService() {
    Log::get().info("Intializing SDL Video ... ");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        Log::get().message(" failure!\n");
        Id::EnvironmentErrorException error(__FILE__, __LINE__, "SDL Video", SDL_GetError());
        Log::get().error("%s\n", ((std::string)error).c_str());
        throw error;
    } else {
        Log::get().message(" success!\n");
    }
}

VideoService::~VideoService() {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

AudioService::AudioService() {
    Log::get().info("Intializing SDL Audio ... ");
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        Log::get().message(" failure!\n");
        Id::EnvironmentErrorException error(__FILE__, __LINE__, "SDL Audio", SDL_GetError());
        Log::get().error("%s\n", ((std::string)error).c_str());
        throw error;
    } else {
        Log::get().message(" success!\n");
    }
}

AudioService::~AudioService() {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

InputService::InputService() {
    Log::get().info("intializing SDL joystick/game controller/haptic support");
    SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    //
    if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0) {
        Log::get().info("unable to initialize joystick support!\n");
    } else {
        Log::get().message("joytick support initialized!\n");
    }
    //
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == 0) {
        Log::get().info("unable to initialize game controller support!\n");
    } else {
        Log::get().message("game controller support initialized!\n");
    }
    //
    if (SDL_WasInit(SDL_INIT_HAPTIC) == 0) {
        Log::get().info("unable to initialize haptic support");
    } else {
        Log::get().message("haptic support initialized!\n");
    }
}

InputService::~InputService() {
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
}

const std::string System::VERSION = "0.1.9";

System::System(const std::string& binaryPath) {
    // Initialize the virtual file system.
    vfs_init(binaryPath.c_str(), nullptr);
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
    try {
        timerService = std::make_unique<TimerService>();
    } catch (...) {
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
    try {
        eventService = std::make_unique<EventService>();
    } catch (...) {
        timerService = nullptr;
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
    try {
        videoService = std::make_unique<VideoService>();
    } catch (...) {
        eventService = nullptr;
        timerService = nullptr;
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
    try {
        audioService = std::make_unique<AudioService>();
    } catch (...) {
        videoService = nullptr;
        eventService = nullptr;
        timerService = nullptr;
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
    try {
        inputService = std::make_unique<InputService>();
    } catch (...) {
        audioService = nullptr;
        videoService = nullptr;
        eventService = nullptr;
        timerService = nullptr;
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
}

System::System(const std::string& binaryPath, const std::string& egobooPath) {
    // Initialize the virtual file system.
    vfs_init(binaryPath.c_str(), egobooPath.c_str());
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
    try {
        timerService = std::make_unique<TimerService>();
    } catch (...) {
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
    try {
        eventService = std::make_unique<EventService>();
    } catch (...) {
        timerService = nullptr;
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
    try {
        videoService = std::make_unique<VideoService>();
    } catch (...) {
        eventService = nullptr;
        timerService = nullptr;
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
    try {
        audioService = std::make_unique<AudioService>();
    } catch (...) {
        videoService = nullptr;
        eventService = nullptr;
        timerService = nullptr;
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
    try {
        inputService = std::make_unique<InputService>();
    } catch (...) {
        audioService = nullptr;
        videoService = nullptr;
        eventService = nullptr;
        timerService = nullptr;
        SDL_Quit();
        setup_end();
        /*sys_uninitialize();*/
        Log::uninitialize();
        /*vfs_uninitialize();*/
        std::rethrow_exception(std::current_exception());
    }
}

System::~System() {
    inputService = nullptr;
    audioService = nullptr;
    videoService = nullptr;
    eventService = nullptr;
    timerService = nullptr;
    setup_end();
    /*sys_uninitialize();*/
    Log::get().message("Exiting Egoboo Engine %s.\n", VERSION.c_str());
    Log::uninitialize();
    setup_clear_base_vfs_paths();
    /*vfs_uninitialize();*/
}

} // namespace Core
} // namespace Ego
