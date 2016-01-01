#pragma once

#include "egolib/platform.h"
#include "egolib/Extensions/SDL_extensions.h"
#include "egolib/Extensions/SDL_GL_extensions.h"

namespace Ego {

struct GraphicsSystem {
public:
    static int gfx_width;
    static int gfx_height;
    static SDLX_video_parameters_t sdl_vparam;
    static oglx_video_parameters_t ogl_vparam;
    static bool initialized;
    /**
     * @brief Initialize the graphics system.
     * @remark This method is a no-op if the graphics system is initialized.
     */
    static void initialize();
    /**
     * @brief Uninitialize the graphics system.
     * @remark This method is a no-op if the graphics system is uninitialized.
     */
    static void uninitialize();
    /**
     * @brief Set the window title.
     * @param title the window title
     * @remark This method is a no-op if the graphics system is uninitialized.
     */
    static void setTitle(const std::string& title);
};
} // namespace Ego