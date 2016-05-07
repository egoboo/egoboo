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

void GraphicsWindow::setIcon(SDL_Surface *icon) {
#if !defined(ID_OSX)
    SDL_SetWindowIcon(window, icon);
#endif
}

void GraphicsWindow::getSize(int& width, int& height) {
    SDL_GetWindowSize(window, &width, &height);
}

void GraphicsWindow::getDrawableSize(int& width, int& height) {
#if SDL_VERSION_ATLEAST(2, 0, 1)
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_OPENGL) {
        return SDL_GL_GetDrawableSize(window, &width, &height);
    }
#endif
    SDL_GetWindowSize(window, &width, &height);
}

void GraphicsWindow::setPosition(int left, int top) {
    SDL_SetWindowPosition(window, left, top);
}

void GraphicsWindow::setSize(int width, int height) {
    SDL_SetWindowSize(window, width, height);
}

int GraphicsWindow::getDisplayIndex() {
    return SDL_GetWindowDisplayIndex(window);
}

void GraphicsWindow::setGrabEnabled(bool enabled) {
    SDL_SetWindowGrab(window, enabled ? SDL_TRUE : SDL_FALSE);
}

SDL_Window *GraphicsWindow::get() {
    return window;
}

}