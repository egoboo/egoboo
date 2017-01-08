#pragma once

#include "egolib/Extensions/SDL_extensions.h"
#include "egolib/Extensions/WindowProperties.hpp"
#include "egolib/Signal/Signal.hpp"
#include "egolib/Events/WindowEventArgs.hpp"
#include "egolib/Math/_Include.hpp"

namespace Ego {

struct GraphicsWindow
{
private:
    SDL_Window *window;

public:

    /// Raised if the window was resized.
    Signal<void(const Events::WindowEventArgs&)> Resized;

    /// Raised if the mouse entered the window.
    Signal<void(const Events::WindowEventArgs&)> MouseEntered;

    /// Raised if the mouse left the window.
    Signal<void(const Events::WindowEventArgs&)> MouseLeft;

    /// Raised if the the window received the keyboard focus.
    Signal<void(const Events::WindowEventArgs&)> KeyboardFocusReceived;

    /// Raised if the window lost the keyboard focus.
    Signal<void(const Events::WindowEventArgs&)> KeyboardFocusLost;

    /// Raised if the window was shown.
    Signal<void(const Events::WindowEventArgs&)> Shown;

    /// Raised if the window was hidden.
    Signal<void(const Events::WindowEventArgs&)> Hidden;

public:
    /// @brief Construct this graphics window with the specified window properties.
    /// @param windowProperties the window properties
    /// @throw Id::RuntimeErrorException window creation failed
    GraphicsWindow(const WindowProperties& windowProperties);

    /// @brief Destruct this graphics window.
    virtual ~GraphicsWindow();

public:
    /// @brief Set the window title.
    /// @param title the window title
    void setTitle(const std::string& title);

    /// @brief Get the window title.
    /// @return the window title
    std::string getTitle() const;

public:
    /// @brief Set if grab mode is enabled for this window.
    /// @param enabled @a true to enable grab mode for this window, @a false to disable it for this window
    /// @remark If grab mode is enabled, then the mouse is confined to the window.
    /// If grab mode is enabled for a window, grab mode is disabled for all ther windows.
    void setGrabEnabled(bool enabled);

    /// @brief Get if grab mode is enabled for this window.
    /// @return @a true if grab mode is enabled for this window, @a false if it is disabled for this window
    bool isGrabEnabled() const;

public:
    /// @brief Get the size of this window.
    /// @return the size of this window
    Size2i getSize() const;

    /// @brief Set the size of this window.
    /// @param size the size of this window
    void setSize(const Size2i& size);

public:
    /// @brief Get the position, in screen coordinates, of the left/top corner of this window.
    /// @return the position, in screen coordinates, of the left/top corner of this window
    Point2i getPosition() const;

    /// @brief Set the position, in screen coordinates, of the left/top corner of this window.
    /// @param position the position, in screen coordinates, of the left/top corner of this window
    void setPosition(const Point2i& position);

public:
    /// @brief Get a pointer to the backend windo.
    /// @return a pointer to the backend window
    SDL_Window *get();

public:
    /// @brief Center the window in its parent.
    void center();

    /// @brief Set the window icon.
    /// @remark Ignored under OSX as the Info.plist icon is used.
    void setIcon(SDL_Surface *icon);

    /// @brief Get the size of the drawable of this window.
    /// @return the size of the drawable of this window
    Size2i getDrawableSize() const;

    /// @brief Update this window.
    void update();

    int getDisplayIndex();

    WindowProperties getProperties() const;

}; // struct GraphicsWindow

} // namespace Ego
