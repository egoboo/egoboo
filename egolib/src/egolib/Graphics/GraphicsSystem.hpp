#pragma once

#include "egolib/platform.h"
#include "egolib/Graphics/GraphicsSystemNew.hpp"
#include "egolib/Extensions/SDL_extensions.h"
#include "egolib/Extensions/SDL_GL_extensions.h"

namespace Ego {

// Forward declaration.
struct GraphicsWindow;
struct GraphicsContext;

struct GraphicsSystem {
public:
    static int gfx_width;
    static int gfx_height;
    static SDLX_video_parameters_t sdl_vparam;
    static oglx_video_parameters_t ogl_vparam;
    static bool initialized;
    
    /// @brief A pointer to the (single) SDL window if it exists, a null pointer otherwise.
    static GraphicsWindow *window;
    
    /// @brief A pointer to the (single) SDL/OpenGL context if it exists, a null pointer otherwise.
    static GraphicsContext *context;
    
    /// @brief Initialize the graphics system.
    /// @remark This method is a no-op if the graphics system is initialized.
    static void initialize();

    /// @brief Uninitialize the graphics system.
    /// @remark This method is a no-op if the graphics system is uninitialized.
    static void uninitialize();
    /// @brief Create a graphics context.
    /// @param window a pointer to a window
    /// @param contextProperties the context properties
    /// @return a pointer to the graphics context on success, a null pointer on failure
    static GraphicsContext *createContext(GraphicsWindow *window, const ContextProperties& contextProperties);
    
    /// @hrief Create a graphics window.
    /// @param windowProperties the window properties
    /// @return a pointer to the graphics window on success, a null pointer on failure
    static GraphicsWindow *createWindow(const WindowProperties& windowProperties);
};

} // namespace Ego
