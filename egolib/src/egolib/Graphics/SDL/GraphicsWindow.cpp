#include "egolib/Graphics/SDL/GraphicsWindow.hpp"

#include "egolib/egoboo_setup.h"

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
        throw id::runtime_error(__FILE__, __LINE__,
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
        throw id::runtime_error(__FILE__, __LINE__, "unable to create SDL window");
    }
    displayIndex = SDL_GetWindowDisplayIndex(window);
    if (displayIndex < 0)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__,
                                         "unable to create SDL window: ",
                                         SDL_GetError(), Log::EndOfEntry);
        throw id::runtime_error(__FILE__, __LINE__, "unable to create SDL window");
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

Size2i GraphicsWindow::getSize() const
{
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return Size2i(width, height);
}

void GraphicsWindow::setSize(const Size2i& size)
{
    SDL_SetWindowSize(window, size.width(), size.height());
}

Point2i GraphicsWindow::getPosition() const
{
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    return Point2i(x, y);
}

void GraphicsWindow::setPosition(const Point2i& position)
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

Size2i GraphicsWindow::getDrawableSize() const
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
    return Size2i(width, height);
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
                this->MouseEntered(Events::WindowMousePointerEnteredEventArgs());
                break;
            case SDL_WINDOWEVENT_LEAVE:
                this->MouseLeft(Events::WindowMousePointerLeftEventArgs());
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                this->KeyboardFocusReceived(Events::WindowReceivedKeyboardInputFocusEventArgs());
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                this->KeyboardFocusLost(Events::WindowLostKeyboardInputFocusEventArgs());
                break;
            case SDL_WINDOWEVENT_RESIZED:
                this->Resized(Events::WindowResizedEventArgs());
                break;
            case SDL_WINDOWEVENT_SHOWN:
                this->Shown(Events::WindowShownEventArgs());
                break;
            case SDL_WINDOWEVENT_HIDDEN:
                this->Hidden(Events::WindowHiddenEventArgs());
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

} // namespace SDL
} // namespace Ego
