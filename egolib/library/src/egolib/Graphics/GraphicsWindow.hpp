#pragma once

#include "egolib/integrations/video.hpp"
#include <SDL.h>

namespace Ego {

class GraphicsWindow : public idlib::window
{
protected:
    /// @brief Construct this graphics window with the specified window properties.
    /// @throw idlib::runtime_error window creation failed
    GraphicsWindow();

public:
    /// @brief Destruct this graphics window.
    virtual ~GraphicsWindow();

    /// @brief Get a pointer to the backend windo.
    /// @return a pointer to the backend window
    virtual SDL_Window *get() = 0;

    /// @brief Set the window icon.
    /// @remark Ignored under OSX as the Info.plist icon is used.
    virtual void setIcon(SDL_Surface *icon) = 0;

    /// @brief Get the display index.
    /// @return the display index
    virtual int getDisplayIndex() const = 0;

    /// @brief Get a copy the contents of the window.
    /// @return the copy of the contents of the window
    virtual std::shared_ptr<SDL_Surface> getContents() const = 0;
}; // class GraphicsWindow

} // namespace Ego
