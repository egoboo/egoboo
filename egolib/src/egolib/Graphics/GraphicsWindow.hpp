#pragma once

#include "egolib/Extensions/SDL_extensions.h"

namespace Ego {

struct GraphicsWindow {
	SDL_Window *window;
    SDLX_sdl_video_flags_t flags;
    GraphicsWindow(const SDLX_sdl_video_flags_t& flags);
    virtual ~GraphicsWindow();
	
	/** 
	 * @brief Set the window title.
	 * @param title the window title
	 */
    void setTitle(const std::string& title);

    /**
     * @brief Set the window icon.
     * @remark Ignored under OSX as the Info.plist icon is used.
     */
    void setIcon(SDL_Surface *icon);

    void getSize(int& width, int& height);

    void getDrawableSize(int& width, int& height);

    void setPosition(int left, int top);

    void setSize(int width, int height);

    int getDisplayIndex();

    void setGrabEnabled(bool enabled);

    SDLX_sdl_video_flags_t getFlags() const;
	
    SDL_Window *get();

}; // struct GraphicsWindow

} // namespace Ego
