#include "egolib/Graphics/GraphicsWindow.hpp"

namespace Ego {

GraphicsWindow::GraphicsWindow(const SDLX_sdl_video_flags_t& flags) : window(nullptr), flags(flags) {
    uint32_t flags_sdl = SDLX_sdl_video_flags_t::upload(flags);
    window = SDL_CreateWindow("SDL 2.x Window",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              320, 240, flags_sdl);
    if (nullptr == window) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "unable to create window");
    }
}

GraphicsWindow::~GraphicsWindow() {
    if (nullptr != window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

void GraphicsWindow::setTitle(const std::string& title) {
    SDL_SetWindowTitle(window, title.c_str());
}

void GraphicsWindow::center() {
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void GraphicsWindow::setIcon(SDL_Surface *icon) {
#if !defined(ID_OSX)
    SDL_SetWindowIcon(window, icon);
#endif
}

Size2i GraphicsWindow::getSize() const {
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return Size2i(width, height);
}

Size2i GraphicsWindow::getDrawableSize() const {
    int width, height;
#if SDL_VERSION_ATLEAST(2, 0, 1)
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_OPENGL) {
        SDL_GL_GetDrawableSize(window, &width, &height);
    } else {
        SDL_GetWindowSize(window, &width, &height);
    }
#else
    SDL_GetWindowSize(window, &width, &height);
#endif
    return Size2i(width, height);
}

Point2i GraphicsWindow::getPosition() const {
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    return Point2i(x, y);
}

void GraphicsWindow::setPosition(Point2i leftTop) {
    SDL_SetWindowPosition(window, leftTop.getX(), leftTop.getY());
}

void GraphicsWindow::setSize(Size2i size) {
    SDL_SetWindowSize(window, size.getWidth(), size.getHeight());
}

int GraphicsWindow::getDisplayIndex() {
    return SDL_GetWindowDisplayIndex(window);
}

void GraphicsWindow::setGrabEnabled(bool enabled) {
    SDL_SetWindowGrab(window, enabled ? SDL_TRUE : SDL_FALSE);
}

SDLX_sdl_video_flags_t GraphicsWindow::getFlags() const {
    SDLX_sdl_video_flags_t flags;
    SDLX_sdl_video_flags_t::download(flags, SDL_GetWindowFlags(window));
    return flags;
}

SDL_Window *GraphicsWindow::get() {
    return window;
}

}