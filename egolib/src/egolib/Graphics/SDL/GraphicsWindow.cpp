#include "egolib/Graphics/SDL/GraphicsWindow.hpp"

#include "egolib/egoboo_setup.h"
#include "egolib/Image/SDL_Image_Extensions.h"
#include "egolib/Image/ImageManager.hpp"
#include "egolib/Extensions/ogl_extensions.h"

namespace Ego {
namespace SDL {

static void UploadColorBufferSettings()
{
    auto& configuration = egoboo_config_t::get();
    int depth = configuration.graphic_colorBuffer_bitDepth.getValue();
    // (1) Shall not above 32 nor below 0.
    if (depth < 0 || depth > 32) depth = 32;
    // (2) Shall be 16 or 32.
    int redDepth = depth / 4,
        greenDepth = depth / 4,
        blueDepth = depth / 4,
        alphaDepth = depth / 4;
    depth = redDepth + greenDepth + blueDepth + alphaDepth;
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, depth);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, redDepth);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, greenDepth);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, blueDepth);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alphaDepth);
}

static void UploadDepthBufferSettings()
{
    auto& configuration = egoboo_config_t::get();
    int depth = configuration.graphic_depthBuffer_bitDepth.getValue();
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth);
}

static void UploadStencilBufferSettings()
{
    auto& configuration = egoboo_config_t::get();
    int depth = configuration.graphic_stencilBuffer_bitDepth.getValue();
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, depth);
}

static void UploadAccumulationBufferSettings()
{
    auto& configuration = egoboo_config_t::get();
    configuration.graphic_accumulationBuffer_bitDepth;
    int depth = configuration.graphic_colorBuffer_bitDepth.getValue();
    int redDepth = depth / 4,
        greenDepth = depth / 4,
        blueDepth = depth / 4,
        alphaDepth = depth / 4;
    depth = redDepth + greenDepth + blueDepth + alphaDepth;
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, redDepth);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, greenDepth);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, blueDepth);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alphaDepth);
}

GraphicsWindow::GraphicsWindow()
    : Ego::GraphicsWindow(), window(nullptr)
{
    auto& config = egoboo_config_t::get();
    // (2) Color, depth, and accumulation buffer depth.
    UploadColorBufferSettings();
    UploadDepthBufferSettings();
    UploadStencilBufferSettings();
    UploadAccumulationBufferSettings();
    // (1) Enable multisample antialasing.
    // The GL_MULTISAMPLE/GL_MULTISAMPLE_ARB superseed GL_POINT_SMOOTH, GL_LINE_SMOOTH, and GL_POLYGON_SMOOTH.
    if (config.graphic_antialiasing.getValue() > 0)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config.graphic_antialiasing.getValue());
    }
    else
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    }
    // (2) Double buffering and hardware acceleration.
    if (config.graphic_doubleBuffering_enable.getValue())
    {
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }
    else
    {
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
    }
    // (3) Hardware acceleration (Do not try to set this under Linux).
#if !defined(ID_LINUX)
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
#endif
    // (2) Upload window properties.
    uint32_t windowFlags = SDL_WINDOW_OPENGL;
    if (config.graphic_fullscreen.getValue())
    {
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    }
    if (config.graphic_window_resizable.getValue())
    {
        windowFlags |= SDL_WINDOW_RESIZABLE;
    }
    if (config.graphic_window_borderless.getValue())
    {
        windowFlags |= SDL_WINDOW_BORDERLESS;
    }
    if (config.graphic_window_allowHighDpi.getValue())
    {
        windowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
    }
    if (config.graphic_fullscreen.getValue() &&
        config.graphic_window_fullscreenDesktop.getValue())
    {
        throw idlib::runtime_error(__FILE__, __LINE__,
                                   config.graphic_fullscreen.getName() + " and " +
                                   config.graphic_window_fullscreenDesktop.getName() + " are mutually exclusive");
    }
    if (config.graphic_window_fullscreenDesktop.getValue())
    {
        windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    // (3) Create the window.
    window = SDL_CreateWindow("SDL 2.x Window",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              320, 240, windowFlags);
    if (nullptr == window)
    {
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__,
                                         "unable to create SDL window: ",
                                         SDL_GetError(), Log::EndOfEntry);
        throw idlib::runtime_error(__FILE__, __LINE__, "unable to create SDL window");
    }
    displayIndex = SDL_GetWindowDisplayIndex(window);
    if (displayIndex < 0)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__,
                                         "unable to create SDL window: ",
                                         SDL_GetError(), Log::EndOfEntry);
        throw idlib::runtime_error(__FILE__, __LINE__, "unable to create SDL window");
    }
}

GraphicsWindow::~GraphicsWindow()
{
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

std::string GraphicsWindow::getTitle() const
{
    return SDL_GetWindowTitle(window);
}

void GraphicsWindow::setTitle(const std::string& title)
{
    SDL_SetWindowTitle(window, title.c_str());
}

void GraphicsWindow::setGrabEnabled(bool enabled)
{
    SDL_SetWindowGrab(window, enabled ? SDL_TRUE : SDL_FALSE);
}

bool GraphicsWindow::isGrabEnabled() const
{
    return SDL_TRUE == SDL_GetWindowGrab(window);
}

Vector2f GraphicsWindow::getSize() const
{
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return Vector2f(width, height);
}

void GraphicsWindow::setSize(const Vector2f& size)
{
    SDL_SetWindowSize(window, size.x(), size.y());
}

Point2f GraphicsWindow::getPosition() const
{
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    return Point2f(x, y);
}

void GraphicsWindow::setPosition(const Point2f& position)
{
    SDL_SetWindowPosition(window, position.x(), position.y());
}

void GraphicsWindow::center()
{
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void GraphicsWindow::setIcon(SDL_Surface *icon)
{
#if !defined(ID_OSX)
    SDL_SetWindowIcon(window, icon);
#endif
}

Vector2f GraphicsWindow::getDrawableSize() const
{
    int width, height;
#if SDL_VERSION_ATLEAST(2, 0, 1)
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_OPENGL)
    {
        SDL_GL_GetDrawableSize(window, &width, &height);
    }
    else
    {
        SDL_GetWindowSize(window, &width, &height);
    }
#else
    SDL_GetWindowSize(window, &width, &height);
#endif
    return Vector2f(width, height);
}

void GraphicsWindow::update()
{
    int newDisplayIndex = SDL_GetWindowDisplayIndex(window);
    if (newDisplayIndex < 0)
    {
        Log::Entry e(Log::Level::Warning, __FILE__, __LINE__);
        e << "unable to get display index" << Log::EndOfEntry;
        Log::get() << e;
    }
    displayIndex = newDisplayIndex;
    SDL_Event event;
    int result;
    SDL_PumpEvents();
    // Process window events.
    result = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT);
    while (result == 1)
    {
        switch (event.window.event)
        {
            case SDL_WINDOWEVENT_ENTER:
                this->MousePointerEntered(Events::MousePointerEnteredEvent());
                break;
            case SDL_WINDOWEVENT_LEAVE:
                this->MousePointerExited(Events::MousePointerExitedEvent());
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                this->KeyboardFocusReceived(Events::KeyboardInputFocusReceivedEvent());
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                this->KeyboardFocusLost(Events::KeyboardInputFocusLostEvent());
                break;
            case SDL_WINDOWEVENT_RESIZED:
                this->WindowResized(Events::WindowResizedEvent());
                break;
            case SDL_WINDOWEVENT_SHOWN:
                this->WindowShown(Events::WindowShownEvent());
                break;
            case SDL_WINDOWEVENT_HIDDEN:
                this->WindowHidden(Events::WindowHiddenEvent());
                break;
            case SDL_WINDOWEVENT_EXPOSED:
                break;
        }
        result = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT);
    }
    if (result < 0)
    {
        /* @todo Emit a warning. */
    }
}

int GraphicsWindow::getDisplayIndex() const
{
    return displayIndex;
}

SDL_Window *GraphicsWindow::get()
{
    return window;
}

std::shared_ptr<SDL_Surface> GraphicsWindow::getContents() const
{ 
    SDL_Surface *surface = nullptr;
    if (HAS_NO_BITS(SDL_GetWindowFlags(window), SDL_WINDOW_OPENGL))
    {
        surface = SDL_GetWindowSurface(window);
        if (!surface)
        {
            throw idlib::environment_error(__FILE__, __LINE__, "SDL", "unable to get window contents");
        }
    }
    else
    {
        Ego::OpenGL::PushClientAttrib pca(GL_CLIENT_PIXEL_STORE_BIT);
        {
            // create a SDL surface
            const auto& pixel_descriptor = pixel_descriptor::get<idlib::pixel_format::R8G8B8>();
            auto drawableSize = getDrawableSize();
            surface =
                SDL_CreateRGBSurface(SDL_SWSURFACE, drawableSize.x(), drawableSize.y(),
                                     pixel_descriptor.get_color_depth().depth(),
                                     pixel_descriptor.get_red().get_mask(),
                                     pixel_descriptor.get_green().get_mask(),
                                     pixel_descriptor.get_blue().get_mask(),
                                     pixel_descriptor.get_alpha().get_mask());
            if (!surface)
            {
                throw idlib::environment_error(__FILE__, __LINE__, "SDL OpenGL", "unable to get window contents");
            }

            // Now lock the surface so that we can read it
            if (-1 == SDL_LockSurface(surface))
            {
                SDL_FreeSurface(surface);
                throw idlib::environment_error(__FILE__, __LINE__, "SDL OpenGL", "unable to get window contents");
            }
            SDL_Rect rect = { 0, 0, (int)drawableSize.x(), (int)drawableSize.y() };
            // Must copy the pixels row-by-row,
            // since the OpenGL video memory is flipped vertically relative to the SDL Screen memory.
            uint8_t *pixels = (uint8_t *)surface->pixels;
            for (auto y = rect.y; y < rect.y + rect.h; y++)
            {
                if (idlib::get_byte_order() == idlib::byte_order::big_endian)
                    glReadPixels(rect.x, (rect.h - y) - 1, rect.w, 1, GL_RGB, GL_UNSIGNED_BYTE, pixels);
                else
                    glReadPixels(rect.x, (rect.h - y) - 1, rect.w, 1, GL_BGR, GL_UNSIGNED_BYTE, pixels);
                pixels += surface->pitch;
            }
            if (Ego::OpenGL::Utilities::isError())
            {
                SDL_UnlockSurface(surface);
                SDL_FreeSurface(surface);
                throw idlib::environment_error(__FILE__, __LINE__, "SDL OpenGL", "unable to get window contents");
            }
            SDL_UnlockSurface(surface);
        }
    }
    try
    {
        auto pixel_format = Ego::SDL::getPixelFormat(surface);
        auto pixel_descriptor = Ego::pixel_descriptor::get(pixel_format);
        return ImageManager::get().createImage((size_t)surface->w, (size_t)surface->h, (size_t)surface->pitch, pixel_descriptor, surface->pixels);
    }
    catch (...)
    {
        SDL_FreeSurface(surface);
        std::rethrow_exception(std::current_exception());
    }
}
} // namespace SDL
} // namespace Ego
