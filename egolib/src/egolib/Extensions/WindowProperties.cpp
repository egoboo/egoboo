#include "egolib/Extensions/WindowProperties.hpp"

namespace Ego {
	
WindowProperties::WindowProperties() :
    resizable(0),
    borderless(0),
    fulldesktop(0), 
    highdpi(0), 
    fullscreen(0), 
    opengl(1) {
}

WindowProperties::WindowProperties(const WindowProperties& other) :
    resizable(other.resizable),
    borderless(other.borderless),
    fulldesktop(other.fulldesktop),
    highdpi(other.highdpi),
    fullscreen(other.fullscreen),
    opengl(other.fulldesktop) {
}

const WindowProperties& WindowProperties::operator=(const WindowProperties& other) {
    resizable = other.resizable;
    borderless = other.borderless;
    fulldesktop = other.fulldesktop;
    highdpi = other.highdpi;
    fullscreen = other.fullscreen;
    opengl = other.opengl;
    return *this;
}
	
uint32_t WindowProperties::upload() const {
    uint32_t bits = 0;
    bits |= fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
    bits |= opengl ? SDL_WINDOW_OPENGL : 0;
    bits |= resizable ? SDL_WINDOW_RESIZABLE : 0;
    bits |= borderless ? SDL_WINDOW_BORDERLESS : 0;
    bits |= highdpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0;
    bits |= fulldesktop ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    return bits;
}

void WindowProperties::download(uint32_t source) {
    fullscreen = SDL_WINDOW_FULLSCREEN == source & SDL_WINDOW_FULLSCREEN;;
    opengl = SDL_WINDOW_OPENGL == source & SDL_WINDOW_OPENGL;
    resizable = SDL_WINDOW_RESIZABLE == source & SDL_WINDOW_RESIZABLE;
    borderless = SDL_WINDOW_BORDERLESS == source & SDL_WINDOW_BORDERLESS;
    highdpi = SDL_WINDOW_ALLOW_HIGHDPI == source & SDL_WINDOW_ALLOW_HIGHDPI;
    fulldesktop = SDL_WINDOW_FULLSCREEN_DESKTOP == source & SDL_WINDOW_FULLSCREEN_DESKTOP;
}

} // namespace Ego

Log::Entry& operator<<(Log::Entry& target, const Ego::WindowProperties& source) {
    target << "window properties" << Log::EndOfLine;
    target << " fullscreen = " << (source.fullscreen ? "yes" : "no") << Log::EndOfLine
        << " fulldesktop = " << (source.fulldesktop ? "yes" : "no") << Log::EndOfLine
        << " OpenGL = " << (source.opengl ? "yes" : "no") << Log::EndOfLine
        << " resizable = " << (source.resizable ? "yes" : "no") << Log::EndOfLine
        << " borderless = " << (source.borderless ? "yes" : "no") << Log::EndOfLine
        << " high DPI (Apple 'Retina') = " << (source.highdpi ? "yes" : "no") << Log::EndOfLine;
    return target;
}
