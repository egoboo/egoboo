#pragma once

#include "egolib/Extensions/SDL_extensions.h"
#include "egolib/Extensions/WindowProperties.hpp"
#include "egolib/Signal/Signal.hpp"
#include "egolib/Events/WindowEventArgs.hpp"
#include "egolib/Math/_Include.hpp"

namespace Ego {

struct GraphicsWindow {
	SDL_Window *window;
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
    /**
     * @brief Construct this graphics window with the specified window properties.
     * @param windowProperties the window properties
     */
    GraphicsWindow(const WindowProperties& windowProperties);
    /**
     * @brief Destruct this graphics window.
     */
    virtual ~GraphicsWindow();
	
	/** 
	 * @brief Set the window title.
	 * @param title the window title
	 */
    void setTitle(const std::string& title);

    /**
     * @brief Center the window in its parent.
     */
    void center();

    /**
     * @brief Set the window icon.
     * @remark Ignored under OSX as the Info.plist icon is used.
     */
    void setIcon(SDL_Surface *icon);

    /** 
     * @brief Get the size of this window.
     * @return the size of this window
     */
    Size2i getSize() const;
    /**
     * @brief Set the size of this window.
     * @param size the size of this window
     */
    void setSize(Size2i size);

    Size2i getDrawableSize() const;

    /**
     * @brief Get the position, in screen coordinates, of the left/top corner of this window.
     * @return the position, in screen coordinates, of the left/top corner of this window
     */
    Point2i getPosition() const;
    /** 
     * @brief Set the position, in screen coordinates, of the left/top corner of this window.
     * @param position the position, in screen coordinates, of the left/top corner of this window
     */
    void setPosition(Point2i leftTop);

    /**
     * @brief Update this window.
     */
    void update();

    int getDisplayIndex();

    void setGrabEnabled(bool enabled);

    WindowProperties getProperties() const;
	
    SDL_Window *get();

}; // struct GraphicsWindow

} // namespace Ego
