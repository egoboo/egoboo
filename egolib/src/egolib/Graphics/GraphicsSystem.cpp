#include "egolib/Graphics/GraphicsSystem.hpp"


namespace Ego {

int GraphicsSystem::gfx_width = 800;
int GraphicsSystem::gfx_height = 600;

SDLX_video_parameters_t GraphicsSystem::sdl_vparam;
oglx_video_parameters_t GraphicsSystem::ogl_vparam;

SDL_Window *GraphicsSystem::window = nullptr;

bool GraphicsSystem::initialized = false;

void GraphicsSystem::initialize() {
    if (initialized) {
        return;
    }
    // Download the window parameters from the Egoboo configuration.
    SDLX_video_parameters_t::download(sdl_vparam, egoboo_config_t::get());

    // Set immutable parameters.
    sdl_vparam.flags.opengl = true;
    sdl_vparam.gl_att.doublebuffer = true;
    sdl_vparam.gl_att.accelerated_visual = GL_TRUE;
    sdl_vparam.gl_att.accumulationBufferDepth = Ego::ColorDepth(32, 8, 8, 8, 8);

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
            sdl_vparam.flags.full_screen = SDL_FALSE;
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
        gfx_width = (float)gfx_height / (float)sdl_vparam.verticalResolution * (float)sdl_vparam.horizontalResolution;
        Log::get().message("Success!\n");
    }

    SDL_Window *window = sdl_scr.window;

#if !defined(ID_OSX)
    {
        // Setup the cute windows manager icon, don't do this on Mac.
        const std::string fileName = "icon.bmp";
        auto pathName = "mp_data/" + fileName;
        SDL_Surface *theSurface = IMG_Load_RW(vfs_openRWopsRead(pathName.c_str()), 1);
        if (!theSurface) {
            Log::get().warn("unable to load icon `%s` - reason: %s\n", pathName.c_str(), SDL_GetError());
        } else {
            SDL_SetWindowIcon(window, theSurface);
        }
    }
#endif

    // Set the window title.
    SDL_SetWindowTitle(window, "SDL OpenGL Window");

    initialized = true;
}

void GraphicsSystem::uninitialize() {
    if (!initialized) {
        return;
    }
    SDL_DestroyWindow(sdl_scr.window);
    sdl_scr.window = nullptr;
}

void GraphicsSystem::setTitle(const std::string& title) {
    if (!initialized) {
        return;
    }
    SDL_SetWindowTitle(sdl_scr.window, title.c_str());
}

} // namespace Ego