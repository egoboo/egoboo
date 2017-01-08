#include "egolib/Graphics/GraphicsSystem.hpp"
#include "egolib/Graphics/GraphicsWindow.hpp"


namespace Ego {

int GraphicsSystem::gfx_width = 800;
int GraphicsSystem::gfx_height = 600;

SDLX_video_parameters_t GraphicsSystem::sdl_vparam;
oglx_video_parameters_t GraphicsSystem::ogl_vparam;

GraphicsWindow *GraphicsSystem::window = nullptr;

bool GraphicsSystem::initialized = false;

GraphicsWindow *GraphicsSystem::createWindow(const WindowProperties& windowProperties)
{
    try
    {
        return new Ego::GraphicsWindow(windowProperties);
    }
    catch (...)
    {
        return nullptr;
    }
}

void GraphicsSystem::initialize() {
    if (initialized) {
        return;
    }
    // Download the window parameters from the Egoboo configuration.
    SDLX_video_parameters_t::download(sdl_vparam, egoboo_config_t::get());

    // Set immutable parameters.
    sdl_vparam.windowProperties.opengl = true;
    sdl_vparam.contextProperties.doublebuffer = true;
    sdl_vparam.contextProperties.accelerated_visual = GL_TRUE;
    sdl_vparam.contextProperties.accumulationBufferDepth = Ego::ColourDepth(32, 8, 8, 8, 8);

    // Download the context parameters from the Egoboo configuration.
    oglx_video_parameters_t::download(ogl_vparam, egoboo_config_t::get());

    Log::get() << Log::Entry::create(Log::Level::Info, __FILE__, __LINE__, "setting SDL video mode", Log::EndOfEntry);

    bool setVideoMode = false;

    // Actually set the video mode.
    if (!SDL_GL_set_mode(sdl_vparam, ogl_vparam)) {
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "unable to set SDL video mode: ", SDL_GetError(), Log::EndOfEntry);
        if (egoboo_config_t::get().graphic_fullscreen.getValue())
        {
            Log::get() << Log::Entry::create(Log::Level::Info, __FILE__, __LINE__, "SDL error with fullscreen mode, retrying with windowed mode", Log::EndOfEntry);
            sdl_vparam.windowProperties.fullscreen = false;
            if (SDL_GL_set_mode(sdl_vparam, ogl_vparam)) {
                Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "unable to set SDL video mode: ", SDL_GetError(), Log::EndOfEntry);
            } else {
                egoboo_config_t::get().graphic_fullscreen.setValue(false);
                setVideoMode = true;
            }
        }
    } else {
        setVideoMode = true;
    }

    if (!setVideoMode) {
        auto e = Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "unable to set any video mode", Log::EndOfEntry);
        Log::get() << e;
        throw std::runtime_error(e.getText());
    } else {
        gfx_width = (float)gfx_height / (float)sdl_vparam.resolution.height() * (float)sdl_vparam.resolution.width();
    }

    GraphicsWindow *window = Ego::GraphicsSystem::window;

    {
        // Setup the cute windows manager icon.
        const std::string fileName = "icon.bmp";
        auto pathName = "mp_data/" + fileName;
        SDL_Surface *theSurface = IMG_Load_RW(vfs_openRWopsRead(pathName.c_str()), 1);
        if (!theSurface) {
            Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to load icon ", "`", pathName, "`: ",  SDL_GetError(), Log::EndOfEntry);
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
    delete window;
    window = nullptr;
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
