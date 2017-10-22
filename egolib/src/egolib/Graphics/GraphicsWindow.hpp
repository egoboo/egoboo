#pragma once

#include "egolib/Events/_Include.hpp"
#include "egolib/Math/_Include.hpp"

namespace Ego {

class GraphicsWindow
{
public:
    /// Raised if the window was resized.
    id::signal<void(const Events::WindowResizedEventArgs&)> Resized;

    /// Raised if the mouse pointer entered the window.
    id::signal<void(const Events::WindowMousePointerEnteredEventArgs&)> MouseEntered;

    /// Raised if the mouse pointer left the window.
    id::signal<void(const Events::WindowMousePointerLeftEventArgs&)> MouseLeft;

    /// Raised if the the window received the keyboard input focus.
    id::signal<void(const Events::WindowReceivedKeyboardInputFocusEventArgs&)> KeyboardFocusReceived;

    /// Raised if the window lost the keyboard input focus.
    id::signal<void(const Events::WindowLostKeyboardInputFocusEventArgs&)> KeyboardFocusLost;

    /// Raised if the window was shown.
    id::signal<void(const Events::WindowShownEventArgs&)> Shown;

    /// Raised if the window was hidden.
    id::signal<void(const Events::WindowHiddenEventArgs&)> Hidden;

protected:
    /// @brief Construct this graphics window with the specified window properties.
    /// @throw id::runtime_error window creation failed
    GraphicsWindow();

public:
    /// @brief Destruct this graphics window.
    virtual ~GraphicsWindow();

    /// @brief Set the window title.
    /// @param title the window title
    virtual void setTitle(const std::string& title) = 0;

    /// @brief Get the window title.
    /// @return the window title
    virtual std::string getTitle() const = 0;

    /// @brief Set if grab mode is enabled for this window.
    /// @param enabled @a true to enable grab mode for this window, @a false to disable it for this window
    /// @remark If grab mode is enabled, then the mouse is confined to the window.
    /// If grab mode is enabled for a window, grab mode is disabled for all ther windows.
    virtual void setGrabEnabled(bool enabled) = 0;

    /// @brief Get if grab mode is enabled for this window.
    /// @return @a true if grab mode is enabled for this window, @a false if it is disabled for this window
    virtual bool isGrabEnabled() const = 0;

    /// @brief Get the size of this window.
    /// @return the size of this window
    virtual Vector2f getSize() const = 0;

    /// @brief Set the size of this window.
    /// @param size the size of this window
    virtual void setSize(const Vector2f& size) = 0;

    /// @brief Get the position, in screen coordinates, of the left/top corner of this window.
    /// @return the position, in screen coordinates, of the left/top corner of this window
    virtual Point2f getPosition() const = 0;

    /// @brief Set the position, in screen coordinates, of the left/top corner of this window.
    /// @param position the position, in screen coordinates, of the left/top corner of this window
    virtual void setPosition(const Point2f& position) = 0;

    /// @brief Get a pointer to the backend windo.
    /// @return a pointer to the backend window
    virtual SDL_Window *get() = 0;

    /// @brief Center the window in its parent.
    virtual void center() = 0;

    /// @brief Set the window icon.
    /// @remark Ignored under OSX as the Info.plist icon is used.
    virtual void setIcon(SDL_Surface *icon) = 0;

    /// @brief Get the size of the drawable of this window.
    /// @return the size of the drawable of this window
    virtual Vector2f getDrawableSize() const = 0;

    /// @brief Update this window.
    virtual void update() = 0;

    /// @brief Get the display index.
    /// @return the display index
    virtual int getDisplayIndex() const = 0;

}; // class GraphicsWindow

} // namespace Ego
