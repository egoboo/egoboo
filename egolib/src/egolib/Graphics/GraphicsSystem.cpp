#include "egolib/Graphics/GraphicsSystem.hpp"
#include "egolib/Graphics/GraphicsWindow.hpp"


namespace Ego {

int GraphicsSystem::gfx_width = 800;
int GraphicsSystem::gfx_height = 600;

SDLX_video_parameters_t GraphicsSystem::sdl_vparam;
oglx_video_parameters_t GraphicsSystem::ogl_vparam;

GraphicsWindow *GraphicsSystem::window = nullptr;

bool GraphicsSystem::initialized = false;

void GraphicsSystem::initialize() {
    if (initialized) {
        return;
    }
    // Download the window parameters from the Egoboo configuration.
    SDLX_video_parameters_t::download(sdl_vparam, egoboo_config_t::get());

    // Set immutable parameters.
    sdl_vparam.windowProperties.opengl = true;
    sdl_vparam.gl_att.doublebuffer = true;
    sdl_vparam.gl_att.accelerated_visual = GL_TRUE;
    sdl_vparam.gl_att.accumulationBufferDepth = Ego::ColourDepth(32, 8, 8, 8, 8);

    // Download the context parameters from the Egoboo configuration.
    oglx_video_parameters_t::download(ogl_vparam, egoboo_config_t::get());

    Log::get().info("Opening SDL Video Mode...\n");

    bool setVideoMode = false;

    // Actually set the video mode.
    if (!SDL_GL_set_mode(nullptr, &sdl_vparam, &ogl_vparam, Ego::GraphicsSystem::initialized)) {
        Log::get().message("Failed!\n");
        if (egoboo_config_t::get().graphic_fullscreen.getValue()) {
            Log::get().info("SDL error with fullscreen mode on: %s\n", SDL_GetError());
            Log::get().info("Trying again in windowed mode...\n");
            sdl_vparam.windowProperties.fullscreen = false;
            if (!SDL_GL_set_mode(nullptr, &sdl_vparam, &ogl_vparam, Ego::GraphicsSystem::initialized)) {
                Log::get().message("Failed!\n");
            } else {
                egoboo_config_t::get().graphic_fullscreen.setValue(false);
                setVideoMode = true;
            }
        }
    } else {
        setVideoMode = true;
    }

    if (!setVideoMode) {
        Log::get().message("Failed!\n");
        std::ostringstream os;
        os << "unable to set any video mode - SDL_GetError() = " << SDL_GetError() << std::endl;
        Log::get().error("%s", os.str().c_str());
        throw std::runtime_error(os.str());
    } else {
        gfx_width = (float)gfx_height / (float)sdl_vparam.resolution.height() * (float)sdl_vparam.resolution.width();
        Log::get().message("Success!\n");
    }

    GraphicsWindow *window = sdl_scr.window;

    {
        // Setup the cute windows manager icon.
        const std::string fileName = "icon.bmp";
        auto pathName = "mp_data/" + fileName;
        SDL_Surface *theSurface = IMG_Load_RW(vfs_openRWopsRead(pathName.c_str()), 1);
        if (!theSurface) {
            Log::get().warn("unable to load icon `%s` - reason: %s\n", pathName.c_str(), SDL_GetError());
        } else {
            window->setIcon(theSurface);
            // ...and the surface containing the icon pixel data is no longer required.
            SDL_FreeSurface(theSurface);

        }
    }

    // Set the window title.
    window->setTitle("SDL 2.x OpenGL Window");

    initialized = true;
}

void GraphicsSystem::uninitialize() {
    if (!initialized) {
        return;
    }
    delete sdl_scr.window;
    sdl_scr.window = nullptr;
}

void GraphicsSystem::setCursorVisibility(bool show) {
    int result = SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
    if (result < 0) {
        throw Id::EnvironmentErrorException(__FILE__, __LINE__, "SDL", std::string("SDL_ShowCursor(")  + (show ? "SDL_ENABLE" : "SDL_DISABLE") + ") failed - reason `" + SDL_GetError() + "`");
    }
}

bool GraphicsSystem::getCursorVisibility() {
    int result = SDL_ShowCursor(SDL_QUERY);
    if (result < 0) {
        throw Id::EnvironmentErrorException(__FILE__, __LINE__, "SDL", std::string("SDL_GetShowCursor(SDL_Query) failed - reason `") + SDL_GetError() + "`");
    }
    return result == SDL_ENABLE;
}

} // namespace Ego
