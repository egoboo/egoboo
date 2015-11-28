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

/// @file egolib/Console/Console.cpp
/// @brief A quake-style console that can be used for anything.
/// @details

#include "egolib/Console/Console.hpp"

#include "egolib/file_common.h"
#include "egolib/strutil.h"
#include "egolib/vfs.h"

#include "egolib/Graphics/FontManager.hpp"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Extensions/SDL_extensions.h"

#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


Ego::Core::Console *egolib_console_top = nullptr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *  This class is to wrap the font's std::shared_ptr for use in calloc and friends
 * @todo
 *  Remove this when egolib_console_t is a proper C++ class.
 */
class egolib_console_FontWrapper final
{
public:
    egolib_console_FontWrapper(const std::shared_ptr<Ego::Font> &font) :
    _font(font)
    {
        // ctor
    }
    ~egolib_console_FontWrapper()
    {
        // dtor
    }
    std::shared_ptr<Ego::Font> _font;
};

namespace Ego {
namespace Core {

const std::string ConsoleSettings::InputSettings::Prompt = ">";

ConsoleHistory::ConsoleHistory()
	: _index(0), _size(0) {
	for (int i = 0; i < ConsoleSettings::HistorySettings::Length; ++i) {
		_buffer[i][0] = '\0';
	}
}

const char *ConsoleHistory::get_saved() {
	if (_index == _size) {
		return "";
	} else {
		return _buffer[_index];
	}
}

void ConsoleHistory::add_saved(char *line) {
	if (0 == strlen(line)) {
		return;
	}
	// (1) If the history is full:
	if (_size == ConsoleSettings::HistorySettings::Length) {
		// (1.1) Shift all elements up.
		for (size_t i = 0; i < ConsoleSettings::HistorySettings::Length - 1; ++i) {
			strncpy(_buffer[i], _buffer[i + 1], ConsoleSettings::LineSettings::Length);
		}
		// (1.2) Decrement the size.
		_size--;
	}
	// (2) Prepend the new element.
	strncpy(_buffer[0], line, ConsoleSettings::LineSettings::Length);
	// (3) Increment the size.
	_size++;
	// (3) No line is focused.
	_index = _size;
}

void ConsoleHistory::up() {
	--_index;
	if (_index < 0) {
		_index = 0;
	}
}

void ConsoleHistory::down() {
	++_index;
	if (_index >= _size) {
		_index = _size;
	}
}

ConsoleHandler *ConsoleHandler::_singleton = nullptr;

ConsoleHandler::ConsoleHandler() {
	/// @author BB
	/// @details initialize the console. This must happen after the screen has been defines,
	///     otherwise sdl_scr.x == sdl_scr.y == 0 and the screen will be defined to
	///     have no area...

	SDL_Rect blah;

	blah.x = 0;
	blah.y = 0;
	blah.w = sdl_scr.x;
	blah.h = sdl_scr.y * 0.25f;

	// without a callback, this console just dumps the input and generates no output
	egolib_console_top = new Ego::Core::Console(blah, nullptr, nullptr);
}

ConsoleHandler::~ConsoleHandler() {
	Ego::Core::Console *console = egolib_console_top; egolib_console_top = egolib_console_top->pnext;
	delete console;
}

void ConsoleHandler::initialize()
{
	if (!_singleton) {
		_singleton = new ConsoleHandler();
	}
}

void ConsoleHandler::uninitialize() {
	if (_singleton) {
		delete _singleton;
		_singleton = nullptr;
	}
}

ConsoleHandler& ConsoleHandler::get() {
	if (nullptr == _singleton) {
		throw std::logic_error("console handler singleton not initialized");
	}
	return *_singleton;
}

bool ConsoleHandler::unlink(Ego::Core::Console *console)
{
    bool retval = false;

    if (!console)
    {
        return false;
    }

    if (console == egolib_console_top)
    {
        egolib_console_top = console->pnext;
        retval = true;
    }
    else
    {
        // find the console that points to this one
		Ego::Core::Console *tmp = egolib_console_top;
        while (nullptr != tmp && nullptr != tmp->pnext)
        {
            if (tmp->pnext == console)
            {
                retval = true;
                tmp->pnext = console->pnext;
                break;
            }
            tmp = tmp->pnext;
        }
    }

    return retval;
}

bool ConsoleHandler::push_front(Ego::Core::Console *console)
{
    if (!console)
    {
        return false;
    }

    console->pnext = egolib_console_top;
    egolib_console_top = console;

    return true;
}

void ConsoleHandler::draw_begin()
{
	auto& renderer = Ego::Renderer::get();
    // do not use the ATTRIB_PUSH macro, since the glPopAttrib() is in a different function
    GL_DEBUG( glPushAttrib )( GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT );

    // don't worry about hidden surfaces
	renderer.setDepthTestEnabled(false);

    // draw draw front and back faces of polygons
	renderer.setCullingMode(Ego::CullingMode::None);

	renderer.setBlendingEnabled(true);
	renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

	renderer.setViewportRectangle(0, 0, sdl_scr.x, sdl_scr.y);

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame

    // store the GL_PROJECTION matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
	Matrix4f4f matrix = Ego::Math::Transform::ortho(0, sdl_scr.x, sdl_scr.y, 0, -1, 1);
	renderer.loadMatrix(matrix);

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
	renderer.loadMatrix(Matrix4f4f::identity());
}

void ConsoleHandler::draw_end()
{
    // Restore the GL_PROJECTION matrix
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();

    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // Re-enable any states disabled by gui_beginFrame
    // do not use the ATTRIB_POP macro, since the glPushAttrib() is in a different function
    GL_DEBUG( glPopAttrib )();
}


void ConsoleHandler::draw_all()
{
	Ego::Core::Console *console = egolib_console_top;

    if (!console)
    {
        return;
    }
	draw_begin();
    console->draw();
	draw_end();
}

SDL_Event *ConsoleHandler::handle_event(SDL_Event *event)
{
	Ego::Core::Console *console = egolib_console_top;

	if (!event)
	{
		return nullptr;
	}
	if (!console)
	{
		return event;
	}

	// Only handle keyboard events.
	if (SDL_TEXTINPUT != event->type && SDL_TEXTEDITING != event->type && SDL_KEYDOWN != event->type)
		return event;

	SDL_Scancode vkey = SDL_SCANCODE_UNKNOWN;
	bool is_alt = false;
	bool is_shift = false;

	if (SDL_KEYDOWN == event->type)
	{
		// Grab the virtual scancode.
		vkey = event->key.keysym.scancode;

		// Get the key modifiers.
		SDL_Keymod kmod = SDL_GetModState();

		// Is alt or shift down?
		is_alt = HAS_SOME_BITS(kmod, KMOD_ALT | KMOD_CTRL);
		is_shift = HAS_SOME_BITS(kmod, KMOD_SHIFT);

		// If the virtual key code for the backquote is pressed,
		// toggle the console on the top of the console stack.
		if (!is_alt && !is_shift && (SDL_SCANCODE_GRAVE == vkey || (console->on && SDL_SCANCODE_ESCAPE == vkey)))
		{
			if (!console->on)
			{
				console->on = true;
				console->buffer_carat = 0;
				console->buffer[0] = CSTR_END;

				SDL_StartTextInput();
				return nullptr;
			}
			else
			{
				console->on = false;
				console->buffer_carat = 0;
				console->buffer[0] = CSTR_END;

				SDL_StopTextInput();
				return nullptr;
			}
		}
	}

	// Only grab the keycodes if the console is on.
	if (!console->on)
	{
		return event;
	}

	// Handle any console commands.
	if (SDL_KEYDOWN == event->type && !is_alt && !is_shift)
	{
		// backspace: delete character before the carat.
		if (SDL_SCANCODE_BACKSPACE == vkey)
		{
			while (console->buffer_carat > 0)
			{
				console->buffer_carat--;
				char a = console->buffer[console->buffer_carat];
				if ((a & 0x80) == 0x00 || (a & 0xC0) == 0xC0)
					break;
			}
			console->buffer[console->buffer_carat] = CSTR_END;

			event = nullptr;
		}
		else if (SDL_SCANCODE_UP == vkey)
		{
			console->history.up();
			strcpy(console->buffer,console->history.get_saved());
			console->buffer_carat = strlen(console->buffer);
			event = nullptr;
		}
		else if (SDL_SCANCODE_DOWN == vkey)
		{
			console->history.down();
			strcpy(console->buffer, console->history.get_saved());
			console->buffer_carat = strlen(console->buffer);
			event = nullptr;
		}
		else if (SDL_SCANCODE_LEFT == vkey)
		{
			console->buffer_carat--;
			console->buffer_carat = Ego::Math::constrain(console->buffer_carat, (size_t)0, (size_t)(ConsoleSettings::LineSettings::Length - 1));

			event = nullptr;
		}
		else if (SDL_SCANCODE_RIGHT == vkey)
		{
			console->buffer_carat++;
			console->buffer_carat = Ego::Math::constrain(console->buffer_carat, (size_t)0, (size_t)(ConsoleSettings::LineSettings::Length - 1));

			event = nullptr;
		}
		else if (SDL_SCANCODE_RETURN == vkey || SDL_SCANCODE_KP_ENTER == vkey)
		{
			console->buffer[console->buffer_carat] = CSTR_END;

			// Add this command line to the list of saved command line.
			console->history.add_saved(console->buffer);

			// Add the command line to the output buffer.
			console->print("%s %s\n", ConsoleSettings::InputSettings::Prompt.c_str(), console->buffer);

			// Actually execute the command line.
			console->run();

			// Blank the command line.
			console->buffer_carat = 0;
			console->buffer[0] = CSTR_END;

			event = nullptr;
		}
	}

	if (nullptr == event || SDL_KEYDOWN == event->type) return nullptr;

	bool addToLine = SDL_TEXTINPUT == event->type;
	char *text = SDL_TEXTINPUT == event->type ? event->text.text : event->edit.text;
	size_t textLength = strlen(text);

	// handle normal keystrokes
	if (console->buffer_carat + textLength + 1 < ConsoleSettings::LineSettings::Length)
	{
		strcat(console->buffer + console->buffer_carat, event->text.text);
		console->buffer[console->buffer_carat + textLength] = CSTR_END;
		if (addToLine)
			console->buffer_carat += strlen(event->text.text);

		event = nullptr;
	}

	return event;
}

} // namespace Core
} // namespace Ego

namespace Ego {
namespace Core {

bool Console::draw()
{
	int windowHeight = sdl_scr.y;

	if (!windowHeight || !this->on)
	{
		return false;
	}

	SDL_Rect *pwin = &(this->rect);

	auto& renderer = Renderer::get();
	renderer.getTextureUnit().setActivated(nullptr);
	auto white = Math::Colour4f::white();
	auto black = Math::Colour4f::black();

	renderer.setColour(white);
	renderer.setLineWidth(5);
	GL_DEBUG(glBegin)(GL_LINE_LOOP);
	{
		GL_DEBUG(glVertex2i)(pwin->x, pwin->y);
		GL_DEBUG(glVertex2i)(pwin->x + pwin->w, pwin->y);
		GL_DEBUG(glVertex2i)(pwin->x + pwin->w, pwin->y + pwin->h);
		GL_DEBUG(glVertex2i)(pwin->x, pwin->y + pwin->h);
	}
	GL_DEBUG_END();

	renderer.setLineWidth(1);

	renderer.setColour(black);
	GL_DEBUG(glBegin)(GL_QUADS);
	{
		GL_DEBUG(glVertex2i)(pwin->x, pwin->y);
		GL_DEBUG(glVertex2i)(pwin->x + pwin->w, pwin->y);
		GL_DEBUG(glVertex2i)(pwin->x + pwin->w, pwin->y + pwin->h);
		GL_DEBUG(glVertex2i)(pwin->x, pwin->y + pwin->h);
	}
	GL_DEBUG_END();

	ATTRIB_PUSH(__FUNCTION__, GL_SCISSOR_BIT | GL_ENABLE_BIT);
	{
		int textWidth, textHeight, height;

		// clip the viewport
		renderer.setScissorTestEnabled(true);
		renderer.setScissorRectangle(pwin->x, windowHeight - (pwin->y + pwin->h), pwin->w, pwin->h);

		height = pwin->h;

		char buffer[ConsoleSettings::InputSettings::Length];

		// draw the current command line
		sprintf(buffer, "%s ", ConsoleSettings::InputSettings::Prompt.c_str());

		strncat(buffer, buffer, 1022);
		buffer[1022] = CSTR_END;

		this->pfont->_font->getTextSize(buffer, &textWidth, &textHeight);
		height -= textHeight;
		this->pfont->_font->drawText(buffer, pwin->x, height - textHeight, white);

		if (CSTR_END != this->output_buffer[0])
		{
			// grab the line offsets
			size_t console_line_count = 0;
			size_t console_line_offsets[1024];
			size_t console_line_lengths[1024];
			char *pstr = this->output_buffer;
			while (pstr)
			{
				size_t len = strcspn(pstr, "\n");

				console_line_offsets[console_line_count] = pstr - this->output_buffer;
				console_line_lengths[console_line_count] = len;

				if (0 == len)
				{
					break;
				}

				pstr += len + 1;
				console_line_count++;
			}

			// draw the last output line and work backwards
			for (size_t i = console_line_count; i >= 1 && height > 0; --i)
			{
				size_t j = i - 1;
				size_t len = std::min((size_t)1023, console_line_lengths[j]);

				strncpy(buffer, this->output_buffer + console_line_offsets[j], len);
				buffer[len] = CSTR_END;

				this->pfont->_font->getTextSize(buffer, &textWidth, &textHeight);
				height -= textHeight;
				this->pfont->_font->drawText(buffer, pwin->x, height - textHeight, white);
			}
		}
	}
	ATTRIB_POP(__FUNCTION__);

	return true;
}


void Console::printv(const char *format, va_list args)
{
	char buffer[ConsoleSettings::InputSettings::Length] = EMPTY_CSTR;
	vsnprintf(buffer, ConsoleSettings::InputSettings::Length - 1, format, args);
	add_output(buffer);
}

void Console::print(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	this->printv(format, args);
	va_end(args);
}

void Console::add_output(char *line)
{
	if (!line)
	{
		return;
	}

	// How many characters are we adding?
	size_t lineLength = strlen(line);

	// initialize the pointers for the copy operation
	char *src = line;
	char *dst = this->output_buffer + this->output_carat;
	//copy_len = out_len;

	// check to make sure that the ranges are valid
	if (lineLength > ConsoleSettings::OutputSettings::Length)
	{
		// we need to replace the entire output buffer with
		// a portion of szNew

		size_t offset = lineLength - ConsoleSettings::OutputSettings::Length - 1;

		// update the copy parameters
		src = line + offset;
		//copy_len = out_len - offset;
	}
	else if (this->output_carat + lineLength > ConsoleSettings::OutputSettings::Length)
	{
		// the length of the buffer after adding szNew would be too large
		// get rid of some of the input buffer and then add szNew

		size_t offset = (this->output_carat + lineLength) - ConsoleSettings::OutputSettings::Length - 1;

		// move the memory so that we create some space
		memmove(this->output_buffer, this->output_buffer + offset, this->output_carat - offset);

		// update the copy parameters
		this->output_carat -= offset;
		dst = this->output_buffer - this->output_carat;
	}

	this->output_carat += snprintf(dst, ConsoleSettings::OutputSettings::Length - this->output_carat, "%s", src);
	this->output_buffer[ConsoleSettings::OutputSettings::Length - 1] = CSTR_END;
}

Console::Console(SDL_Rect rectangle, Console::Callback callback, void *data)
	: history()
{
	// reset all the console data
	this->on = false;

	this->buffer[0] = '\0';
	this->buffer_carat = 0;

	this->output_buffer[0] = '\0';
	this->output_carat = 0;

	this->pnext = nullptr;

	// set the console's font
	this->pfont = new egolib_console_FontWrapper(Ego::FontManager::loadFont("mp_data/pc8x8.fon", 12));

	// set the console's rectangle
	this->rect = rectangle;

	// register the "run" callback
	this->run_func = callback;
	this->run_data = data;
}

bool Console::run()
{
	bool retval = false;

	if (nullptr != this->run_func)
	{
		retval = this->run_func(this, this->run_data);
	}

	return retval;
}

Console::~Console()
{
	// Delete its font.
	delete this->pfont;
	this->pfont = nullptr;
}

void Console::show()
{
    // Turn the console on.
	this->on = true;

    // Fix the keyrepeat.
    if (nullptr == egolib_console_top)
    {
        SDL_StopTextInput();
    }
    else
    {
        SDL_StartTextInput();
    }
}

void Console::hide()
{
	// Turn the console off.
	this->on = false;

    // Fix the keyrepeat.
    if (nullptr == egolib_console_top)
    {
        SDL_StopTextInput();
    }
    else
    {
        SDL_StartTextInput();
    }
}

ConsoleHistory& Console::getHistory() {
	return history;
}

} // namespace Core
} // namespace Ego
