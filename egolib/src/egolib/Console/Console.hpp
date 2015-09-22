//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file  egolib/console.h
/// @brief A quake-style console that can be used for anything.

#pragma once

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------------------------------

// opaque console struct
struct egolib_console_t;



//--------------------------------------------------------------------------------------------
// struct s_egolib_console
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// GLOBAL FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

/// @todo: Remove when egolib_console_t is a proper C++ class
class egolib_console_FontWrapper;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define EGOBOO_CONSOLE_LINES   32
#define EGOBOO_CONSOLE_LENGTH 256
#define EGOBOO_CONSOLE_PROMPT '>'
#define EGOBOO_CONSOLE_OUTPUT 4096

#define EGOBOO_CONSOLE_WRITE_LEN 1024

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

namespace Ego {
namespace Core {

/// The encapsulation of the data necessary to run a generic Quake-like console in Egoboo
struct Console
{
	static Console *_singleton;
	static void startup();
	static void shutdown();
	/// console callback used to implement specializations of the egolib_console
	typedef bool (*Callback)(Console *console, void *data);
	Console *pnext;

    Callback run_func;
    void *run_data;

    egolib_console_FontWrapper *pfont;

    /**
     * @brief
     *  Is the console visible?
     */
    bool on;

    SDL_Rect rect;

    int save_count;
    int save_index;
    char save_buffer[EGOBOO_CONSOLE_LINES][EGOBOO_CONSOLE_LENGTH];

    size_t buffer_carat;
    char buffer[EGOBOO_CONSOLE_LENGTH];

    size_t output_carat;
    char output_buffer[EGOBOO_CONSOLE_OUTPUT];

	Console(SDL_Rect rectangle, Callback callback, void *data);
	virtual ~Console();

    bool draw();

    void show();
    void hide();

    bool run();

    void print(const char *format, ...) GCC_PRINTF_FUNC(2);
    void printv(const char *format, va_list args);

    /**
     * @brief
     *  Get a saved line.
     * @return
     *  the saved line
     * @todo
     *  Semantics if there are no saved lines?
     */
    const char *get_saved();
    /**
     * @brief
     *  Add a line to the saved lines.
     * @param line
     *  the line
     * @post
     *  If the array of lines was full, the first line was removed from the array.
     *  The given line was appended to the array. The save index refers to the appended line.
     */
    void add_saved(char *line);

    void add_output(char *line);
	
};

} // namespace Core
} // namespace Ego

struct egolib_console_handler_t
{
protected:
    static void draw_begin();
    static void draw_end();
public:
    static void draw_all();
    static bool push_front(Ego::Core::Console *console);
    /**
     * @brief
     *  Remove the console from the console stack.
     * @param console
     *  the console
     * @return
     *  @a true if the console was removed, @a false otherwise
     */
    static bool unlink(Ego::Core::Console *console);

    /**
    * @return
    *  @a nullptr if
    *  - @a event is @a nullptr or
    *  - @a event is not @a nullptr and the event was handled by some console.
    *  @a event in all other cases.
    */
    static SDL_Event *handle_event(SDL_Event *event);

    static void initialize();
    static void uninitialize();
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern Ego::Core::Console *egolib_console_top;
