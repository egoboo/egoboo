#pragma once

#include "egolib/Extensions/SDL_extensions.h"
#include "egolib/Extensions/WindowProperties.hpp"
#include "egolib/Math/_Include.hpp"

namespace Ego {

struct GraphicsWindow {
	SDL_Window *window;
    GraphicsWindow(const WindowProperties& windowProperties);
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

    

    int getDisplayIndex();

    void setGrabEnabled(bool enabled);

    WindowProperties getProperties() const;
	
    SDL_Window *get();

}; // struct GraphicsWindow

} // namespace Ego
