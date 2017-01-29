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
#include "egolib/Graphics/GraphicsSystem.hpp"
#include "egolib/Graphics/GraphicsWindow.hpp"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/InputControl/InputSystem.hpp"
#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Extensions/SDL_extensions.h"

#include "egolib/_math.h"

namespace Ego {
namespace Core {

const std::string ConsoleSettings::InputSettings::Prompt = ">";

ConsoleHistory::ConsoleHistory()
    : index(0), list()
{}

const std::string& ConsoleHistory::get_saved()
{
    if (index == list.size())
    {
        static const std::string empty = std::string();
        return empty;
    }
    else
    {
        return list[index];
    }
}

void ConsoleHistory::add_saved(const std::string& line)
{
    if (0 == line.size())
    {
        return;
    }
    // If the history is full ...
    if (list.size() == ConsoleSettings::HistorySettings::Length)
    {
        // ... remove the last entry.
        list.pop_back();
    }
    // Push to front.
    list.push_front(line);
    // No line is focused.
    index = list.size();
}

void ConsoleHistory::up()
{
    if (index > 0)
    {
        --index;
    }
}

void ConsoleHistory::down()
{
    if (index < list.size())
    {
        ++index;
    }
}

ConsoleHandler::ConsoleHandler() :
    top(nullptr)
{
    /// @author BB
    /// @details initialize the console. This must happen after the screen has been defined,
    ///     otherwise windowSize.width() == windowSize.height() == 0 and the screen will be defined to
    ///     have no area...

    SDL_Rect blah;

    auto windowSize = GraphicsSystem::window->getSize();
    blah.x = 0;
    blah.y = 0;
    blah.w = windowSize.width();
    blah.h = windowSize.height() * 0.25f;

    // without a callback, this console just dumps the input and generates no output
    top = new Console(blah, nullptr, nullptr);
}

ConsoleHandler::~ConsoleHandler()
{
    Console *console = top; top = top->pnext;
    delete console;
}

bool ConsoleHandler::unlink(Console *console)
{
    bool retval = false;

    if (!console)
    {
        return false;
    }

    if (console == top)
    {
        top = console->pnext;
        retval = true;
    }
    else
    {
        // find the console that points to this one
        Console *tmp = top;
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

bool ConsoleHandler::push_front(Console *console)
{
    if (!console)
    {
        return false;
    }

    console->pnext = top;
    top = console;

    return true;
}

void ConsoleHandler::draw_begin()
{
}

void ConsoleHandler::draw_end()
{
    auto& renderer = Ego::Renderer::get();
    renderer.setDepthTestEnabled(true);
    renderer.setScissorTestEnabled(false);
}

void ConsoleHandler::draw_all()
{
    Console *console = top;

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
    Console *console = top;

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
    bool is_ctrl = false;
    bool is_shift = false;

    if (SDL_KEYDOWN == event->type)
    {
        // Grab the virtual scancode.
        vkey = event->key.keysym.scancode;

        // Get the key modifiers.
        ModifierKeys modifierKeys = Input::InputSystem::get().getModifierKeys();

        // Is alt or shift down?
        is_alt = (ModifierKeys::LeftAlt == (modifierKeys & ModifierKeys::LeftAlt))
            || (ModifierKeys::RightAlt == (modifierKeys & ModifierKeys::RightAlt));
        is_ctrl = (ModifierKeys::LeftControl == (modifierKeys & ModifierKeys::LeftControl))
            || (ModifierKeys::RightControl == (modifierKeys & ModifierKeys::RightControl));
        is_shift = (ModifierKeys::LeftShift == (modifierKeys & ModifierKeys::LeftShift))
            || (ModifierKeys::RightShift == (modifierKeys & ModifierKeys::RightShift));

        // If the virtual key code for the backquote is pressed,
        // toggle the console on the top of the console stack.
        if (!is_alt && !is_ctrl && !is_shift && (SDL_SCANCODE_GRAVE == vkey || (console->on && SDL_SCANCODE_ESCAPE == vkey)))
        {
            if (!console->on)
            {
                console->on = true;
                console->input.clear();

                SDL_StartTextInput();
                return nullptr;
            }
            else
            {
                console->on = false;
                console->input.clear();

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
    if (SDL_KEYDOWN == event->type && !is_alt && !is_ctrl && !is_shift)
    {
        // backspace: delete character before the carat.
        if (SDL_SCANCODE_BACKSPACE == vkey)
        {
            console->input.deleteLeft();
            event = nullptr;
        }
        else if (SDL_SCANCODE_UP == vkey)
        {
            console->history.up();
            console->input.setText(console->history.get_saved());
            event = nullptr;
        }
        else if (SDL_SCANCODE_DOWN == vkey)
        {
            console->history.down();
            console->input.setText(console->history.get_saved());
            event = nullptr;
        }
        else if (SDL_SCANCODE_LEFT == vkey)
        {
            console->input.moveLeft();
            event = nullptr;
        }
        else if (SDL_SCANCODE_RIGHT == vkey)
        {
            console->input.moveRight();
            event = nullptr;
        }
        else if (SDL_SCANCODE_RETURN == vkey || SDL_SCANCODE_KP_ENTER == vkey)
        {
            console->input.buffer[console->input.carat] = CSTR_END;

            // Add this command line to the list of saved command line.
            console->history.add_saved(console->input.buffer);

            // Add the command line to the output buffer.
            std::ostringstream os;
            os << ConsoleSettings::InputSettings::Prompt << " "
               << console->input.buffer << std::endl;
            console->add_output(os.str().c_str());

            // Actually execute the command line.
            console->run();

            // Blank the command line.
            console->input.clear();

            event = nullptr;
        }
    }

    if (nullptr == event || SDL_KEYDOWN == event->type) return nullptr;

    bool addToLine = SDL_TEXTINPUT == event->type;
    char *text = SDL_TEXTINPUT == event->type ? event->text.text : event->edit.text;
    size_t textLength = strlen(text);

    // handle normal keystrokes
    if (console->input.carat + textLength + 1 < ConsoleSettings::LineSettings::Length)
    {
        strcat(console->input.buffer + console->input.carat, event->text.text);
        console->input.buffer[console->input.carat + textLength] = CSTR_END;
        if (addToLine)
            console->input.carat += strlen(event->text.text);

        event = nullptr;
    }

    return event;
}

} // namespace Core
} // namespace Ego

namespace Ego {
namespace Core {

void Console::draw()
{
    auto& renderer = Renderer::get();

    // don't worry about hidden surfaces
    renderer.setDepthTestEnabled(false);

    // draw draw front and back faces of polygons
    renderer.setCullingMode(CullingMode::None);

    renderer.setBlendingEnabled(true);
    renderer.setBlendFunction(BlendFunction::SourceAlpha, BlendFunction::OneMinusSourceAlpha);

    auto drawableSize = GraphicsSystem::window->getDrawableSize();
    renderer.setViewportRectangle(0, 0, drawableSize.width(), drawableSize.height());

    // Set the projecton matrix.
    auto windowSize = GraphicsSystem::window->getSize();
    Matrix4f4f matrix = Transform::ortho(0, windowSize.width(), windowSize.height(), 0, -1, 1);
    renderer.setProjectionMatrix(matrix);

    // Set the view and the world matrix.
    renderer.setViewMatrix(Matrix4f4f::identity());
    renderer.setWorldMatrix(Matrix4f4f::identity());

    int windowHeight = GraphicsSystem::window->getSize().height();

    if (!windowHeight || !this->on)
    {
        return;
    }

    SDL_Rect *pwin = &(this->rect);

    renderer.getTextureUnit().setActivated(nullptr);

    // The colour white.
    static const auto white = Colour4f::white();
    // The colour black.
    static const auto black = Colour4f::black();
    // The vertex data structure and the vertex descriptor.
    struct Vertex { float x, y; };
    static const VertexDescriptor vertexDescriptor({VertexElementDescriptor(0, VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Position)});


    renderer.setColour(white);
    renderer.setLineWidth(5);
    VertexBuffer vertexBuffer(4, vertexDescriptor);
    {
        BufferScopedLock lock(vertexBuffer);
        Vertex *vertex = lock.get<Vertex>();
        vertex->x = pwin->x;           vertex->y = pwin->y;           ++vertex;
        vertex->x = pwin->x + pwin->w; vertex->y = pwin->y;           ++vertex;
        vertex->x = pwin->x + pwin->w; vertex->y = pwin->y + pwin->h; ++vertex;
        vertex->x = pwin->x;           vertex->y = pwin->y + pwin->h;
    }
    renderer.render(vertexBuffer, PrimitiveType::LineLoop, 0, 4);
    renderer.setLineWidth(1);

    renderer.setColour(black);
    renderer.render(vertexBuffer, PrimitiveType::Quadriliterals, 0, 4);


    int textWidth, textHeight, height;

    // clip the viewport
    renderer.setScissorTestEnabled(true);
    renderer.setScissorRectangle(pwin->x, windowHeight - (pwin->y + pwin->h), pwin->w, pwin->h);

    height = pwin->h;

    char buffer[ConsoleSettings::InputSettings::Length];

    // draw the current command line
    sprintf(buffer, "%s ", ConsoleSettings::InputSettings::Prompt.c_str());

    strncat(buffer, this->input.buffer, 1022);
    buffer[1022] = CSTR_END;

    this->pfont->getTextSize(buffer, &textWidth, &textHeight);
    height -= textHeight;
    this->pfont->drawText(buffer, pwin->x, height - textHeight, white);

    if (CSTR_END == this->output.buffer[0])
    {
        return;
    }
    // grab the line offsets
    std::vector<TextRange> console_lines;
    char *pstr = this->output.buffer;
    while (pstr)
    {
        size_t len = strcspn(pstr, "\n");

        console_lines.push_back(TextRange(pstr - this->output.buffer, len));

        if (0 == len)
        {
            break;
        }

        pstr += len + 1;
    }

    // draw the last output line and work backwards
    for (size_t i = console_lines.size(); i >= 1 && height > 0; --i)
    {
        size_t j = i - 1;
        size_t len = std::min((size_t)1023, console_lines[j].getLength());

        strncpy(buffer, this->output.buffer + console_lines[j].getStart(), len);
        buffer[len] = CSTR_END;

        this->pfont->getTextSize(buffer, &textWidth, &textHeight);
        height -= textHeight;
        this->pfont->drawText(buffer, pwin->x, height - textHeight, white);
    }
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

void Console::add_output(const char *line)
{
	if (!line)
	{
		return;
	}

	// How many characters are we adding?
	size_t lineLength = strlen(line);

	// initialize the pointers for the copy operation
	const char *src = line;
	char *dst = this->output.buffer + this->output.carat;
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
	else if (this->output.carat + lineLength > ConsoleSettings::OutputSettings::Length)
	{
		// the length of the buffer after adding szNew would be too large
		// get rid of some of the input buffer and then add szNew

		size_t offset = (this->output.carat + lineLength) - ConsoleSettings::OutputSettings::Length - 1;

		// move the memory so that we create some space
		memmove(this->output.buffer, this->output.buffer + offset, this->output.carat - offset);

		// update the copy parameters
		this->output.carat -= offset;
		dst = this->output.buffer - this->output.carat;
	}

	this->output.carat += snprintf(dst, ConsoleSettings::OutputSettings::Length - this->output.carat, "%s", src);
	this->output.buffer[ConsoleSettings::OutputSettings::Length - 1] = CSTR_END;
}

Console::Console(SDL_Rect rectangle, Console::Callback callback, void *data)
    : history(), input(), output()
{
    // reset all the console data
    this->on = false;

    this->pnext = nullptr;

    // set the console's font
    this->pfont = FontManager::get().loadFont("mp_data/pc8x8.fon", 12);

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
{}

void Console::show()
{
    // Turn the console on.
    this->on = true;

    // Fix the keyrepeat.
    if (nullptr == ConsoleHandler::get().top)
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
    if (nullptr == ConsoleHandler::get().top)
    {
        SDL_StopTextInput();
    }
    else
    {
        SDL_StartTextInput();
    }
}

ConsoleHistory& Console::getHistory()
{
    return history;
}

} // namespace Core
} // namespace Ego
