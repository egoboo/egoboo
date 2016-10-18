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

const std::string SystemService::VERSION = "0.1.9";

SystemService::SystemService(const std::string& binaryPath) {
    // Initialize virtual file system.
    vfs_init(binaryPath.c_str(), nullptr);
    // Add search paths.
    /*
    // Uncomment to display the search paths.
    vfs_listSearchPaths();
    */
    setup_init_base_vfs_paths();
    /*
    // Uncomment to display the search paths.
    vfs_listSearchPaths();
    */

    // Initialize logging.
    Log::initialize("/debug/log.txt", Log::Level::Debug);

    // Say hello.
    Log::get().message("Starting Egoboo Engine %s\n", VERSION.c_str());

    // Load "setup.txt" and download "setup.txt" into the Egoboo configuration.
    Setup::begin();
    Setup::download(egoboo_config_t::get());

    // Initialize SDL.
    Log::get().message("Initializing SDL version %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTS);
}

SystemService::SystemService(const std::string& binaryPath, const std::string& egobooPath) {
    // Initialize virtual file system.
    vfs_init(binaryPath.c_str(), egobooPath.c_str());
    // Add search paths.
    /*
    // Uncomment to display the search paths.
    vfs_listSearchPaths();
    */
    setup_init_base_vfs_paths();
    /*
    // Uncomment to display the search paths.
    vfs_listSearchPaths();
    */
    
    // Initialize logging.
    Log::initialize("/debug/log.txt", Log::Level::Debug);
    
    // Say hello.
    Log::get().message("Starting Egoboo Engine %s\n", VERSION.c_str());

    // Load "setup.txt".
    Setup::begin();

    // Load "setup.txt" and download "setup.txt" into the Egoboo configuration.
    Setup::download(egoboo_config_t::get());

    // Initialize SDL.
    Log::get().message("Initializing SDL version %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    SDL_Init(SDL_INIT_TIMER| SDL_INIT_EVENTS);
}

SystemService::~SystemService() {
    // Uninitialize SDL.
    Log::get().message("Uninitializing SDL version %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    SDL_Quit();
    // Save "setup.txt".
    Setup::end();
    // Say bye.
    Log::get().message("Exiting Egoboo Engine %s.\n", VERSION.c_str());
    // Uninitialize logging.
    Log::uninitialize();
    // Remove search paths.
    /*
    // Uncomment to display the search paths.
    vfs_listSearchPaths();
    */
    setup_clear_base_vfs_paths();
    /*
    // Uncomment to display the search paths.
    vfs_listSearchPaths();
    */
    // Uninitialize virtual file system.
#if 0
    vfs_uninit();
#endif
}

uint32_t SystemService::getTicks() {
    return SDL_GetTicks();
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

System::System(const std::string& binaryPath) {
    try {
        systemService = new SystemService(binaryPath);
    } catch (...) {
        std::rethrow_exception(std::current_exception());
    }
    try {
        videoService = new VideoService();
    } catch (...) {
        if (systemService) {
            delete systemService;
            systemService = nullptr;
        }
        std::rethrow_exception(std::current_exception());
    }
    try {
        audioService = new AudioService();
    } catch (...) {
        if (videoService) {
            delete videoService;
            videoService = nullptr;
        }
        if (systemService) {
            delete systemService;
            systemService = nullptr;
        }
        std::rethrow_exception(std::current_exception());
    }
    try {
        inputService = new InputService();
    } catch (...) {
        if (audioService) {
            delete audioService;
            audioService = nullptr;
        }
        if (videoService) {
            delete videoService;
            videoService = nullptr;
        }
        if (systemService) {
            delete systemService;
            systemService = nullptr;
        }
        std::rethrow_exception(std::current_exception());
    }
}

System::System(const std::string& binaryPath, const std::string& egobooPath) {
    try {
        systemService = new SystemService(binaryPath);
    } catch (...) {
        std::rethrow_exception(std::current_exception());
    }
    try {
        videoService = new VideoService();
    } catch (...) {
        if (systemService) {
            delete systemService;
            systemService = nullptr;
        }
        std::rethrow_exception(std::current_exception());
    }
    try {
        audioService = new AudioService();
    } catch (...) {
        if (videoService) {
            delete videoService;
            videoService = nullptr;
        }
        if (systemService) {
            delete systemService;
            systemService = nullptr;
        }
        std::rethrow_exception(std::current_exception());
    }
    try {
        inputService = new InputService();
    } catch (...) {
        if (audioService) {
            delete audioService;
            audioService = nullptr;
        }
        if (videoService) {
            delete videoService;
            videoService = nullptr;
        }
        if (systemService) {
            delete systemService;
            systemService = nullptr;
        }
        std::rethrow_exception(std::current_exception());
    }
}

System::~System() {
    if (inputService) {
        delete inputService;
        inputService = nullptr;
    }
    if (audioService) {
        delete audioService;
        audioService = nullptr;
    }
    if (videoService) {
        delete videoService;
        videoService = nullptr;
    }
    if (systemService) {
        delete systemService;
        systemService = nullptr;
    }
}

} // namespace Core
} // namespace Ego
