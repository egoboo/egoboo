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

#include "egolib/strutil.h"
#include "egolib/Graphics/FontManager.hpp"
#include "egolib/Graphics/GraphicsSystem.hpp"
#include "egolib/Graphics/GraphicsWindow.hpp"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/InputControl/InputSystem.hpp"

namespace Ego { namespace Core {

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

SDL_Event *Console::handle_event(SDL_Event *event)
{
	if (!event)
	{
		return nullptr;
	}
	// Only handle keyboard events.
	if (SDL_TEXTINPUT != event->type && SDL_TEXTEDITING != event->type && SDL_KEYDOWN != event->type)
	{
		return event;
	}

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
		if (!is_alt && !is_ctrl && !is_shift && (SDL_SCANCODE_GRAVE == vkey || (on && SDL_SCANCODE_ESCAPE == vkey)))
		{
			if (!on)
			{
				on = true;
				input.clear();

				SDL_StartTextInput();
				return nullptr;
			}
			else
			{
				on = false;
				input.clear();

				SDL_StopTextInput();
				return nullptr;
			}
		}
	}

	// Only grab the keycodes if the console is on.
	if (!on)
	{
		return event;
	}

	// Handle any console commands.
	if (SDL_KEYDOWN == event->type && !is_alt && !is_ctrl && !is_shift)
	{
		// backspace: delete character before the carat.
		if (SDL_SCANCODE_BACKSPACE == vkey)
		{
			input.deleteLeft();
			event = nullptr;
		}
		else if (SDL_SCANCODE_UP == vkey)
		{
			history.up();
			input.setText(history.get_saved());
			event = nullptr;
		}
		else if (SDL_SCANCODE_DOWN == vkey)
		{
			history.down();
			input.setText(history.get_saved());
			event = nullptr;
		}
		else if (SDL_SCANCODE_LEFT == vkey)
		{
			input.moveLeft();
			event = nullptr;
		}
		else if (SDL_SCANCODE_RIGHT == vkey)
		{
			input.moveRight();
			event = nullptr;
		}
		else if (SDL_SCANCODE_RETURN == vkey || SDL_SCANCODE_KP_ENTER == vkey)
		{
			input.buffer[input.carat] = CSTR_END;
			std::string command = input.buffer;

			// Add this command line to the list of saved command line.
			history.add_saved(command);

			// Add the command line to the output buffer.
			std::ostringstream os;
			os << ConsoleSettings::InputSettings::Prompt << " "
			   << command << std::endl;
			add_output(os.str());

			// Actually execute the command line.
			ExecuteCommand(command);

			// Blank the command line.
			input.clear();

			event = nullptr;
		}
	}

	if (nullptr == event || SDL_KEYDOWN == event->type) return nullptr;

	bool addToLine = SDL_TEXTINPUT == event->type;
	char *text = SDL_TEXTINPUT == event->type ? event->text.text : event->edit.text;
	size_t textLength = strlen(text);

	// handle normal keystrokes
	if (input.carat + textLength + 1 < ConsoleSettings::LineSettings::Length)
	{
		strcat(input.buffer + input.carat, event->text.text);
		input.buffer[input.carat + textLength] = CSTR_END;
		if (addToLine)
			input.carat += strlen(event->text.text);

		event = nullptr;
	}

	return event;
}

void Console::draw()
{
    auto& renderer = Renderer::get();

    // don't worry about hidden surfaces
    renderer.setDepthTestEnabled(false);

    // draw draw front and back faces of polygons
    renderer.setCullingMode(idlib::culling_mode::none);

    renderer.setBlendingEnabled(true);
    renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one_minus_source0_alpha);

    auto drawableSize = GraphicsSystem::get().window->getDrawableSize();
    renderer.setViewportRectangle(0, 0, drawableSize(0), drawableSize(1));

    // Set the projecton matrix.
    auto windowSize = GraphicsSystem::get().window->getSize();
    Matrix4f4f matrix = Math::Transform::ortho(0, windowSize.x(), windowSize.y(), 0, -1, 1);
    renderer.setProjectionMatrix(matrix);

    // Set the view and the world matrix.
    renderer.setViewMatrix(Matrix4f4f::identity());
    renderer.setWorldMatrix(Matrix4f4f::identity());

    int windowHeight = GraphicsSystem::get().window->getSize().y();

    if (!windowHeight || !this->on)
    {
        return;
    }

    renderer.getTextureUnit().setActivated(nullptr);

    // The colour white.
    static const auto white = Math::Colour4f::white();
    // The colour black.
    static const auto black = Math::Colour4f::black();
    // The vertex data structure and the vertex descriptor.
    struct Vertex { float x, y; };
    static const VertexDescriptor vertexDescriptor({VertexElementDescriptor(0, idlib::vertex_component_syntactics::SINGLE_2, idlib::vertex_component_semantics::POSITION)});


    renderer.setColour(white);
    renderer.setLineWidth(5);
    VertexBuffer vertexBuffer(4, vertexDescriptor.get_size());
    {
        BufferScopedLock lock(vertexBuffer);
        Vertex *vertex = lock.get<Vertex>();
        vertex->x = rectangle.get_min().x();  vertex->y = rectangle.get_min().y(); ++vertex;
        vertex->x = rectangle.get_max().x();  vertex->y = rectangle.get_min().y(); ++vertex;
        vertex->x = rectangle.get_max().x();  vertex->y = rectangle.get_max().y(); ++vertex;
        vertex->x = rectangle.get_min().x();  vertex->y = rectangle.get_max().y();
    }
    renderer.render(vertexBuffer, vertexDescriptor, idlib::primitive_type::line_loop, 0, 4);
    renderer.setLineWidth(1);

    renderer.setColour(black);
    renderer.render(vertexBuffer, vertexDescriptor, idlib::primitive_type::quadriliterals, 0, 4);


    int height;

    // clip the viewport
    renderer.setScissorTestEnabled(true);
    renderer.setScissorRectangle(rectangle.get_min().x(),
		                         windowHeight - rectangle.get_max().y(),
		                         rectangle.get_size().x(),
		                         rectangle.get_size().y());

    height = rectangle.get_size().y();

    char buffer[ConsoleSettings::InputSettings::Length];

    // draw the current command line
    sprintf(buffer, "%s ", ConsoleSettings::InputSettings::Prompt.c_str());

    strncat(buffer, this->input.buffer, 1022);
    buffer[1022] = CSTR_END;

	int text_width, text_height;
    this->pfont->getTextSize(buffer, &text_width, &text_height);
    height -= text_height;
    this->pfont->drawText(buffer, rectangle.get_min().x(), height - text_height, white);

    if (m_document.get_text().empty())
	{
        return;
    }

    // grab the line offsets
    //std::vector<idlib::text_line> console_lines;
	auto temporary = m_document.get_text();
	idlib::text_parser<char> parser;
	std::vector<idlib::text_line> console_lines;
	parser(temporary,
		   [](auto character) {
				return '\n' == character; 
			},
		   [this, temporary, &console_lines](auto string_start, auto string_end, auto substring_begin, auto substring_end) {
				auto x = std::string(substring_begin, substring_end);
				if (x != "\n") {
					auto r = idlib::text_range(std::distance(string_start, substring_begin),
						                       std::distance(substring_begin, substring_end));
					auto l = idlib::text_line(r);
					int w, h;
					this->pfont->getTextSize(x, &w, &h);
					l.set_width(w);
					l.set_height(h);
					console_lines.push_back(l);
				}
			});

    // draw the last output line and work backwards
    for (size_t i = console_lines.size(); i >= 1 && height > 0; --i)
    {
        size_t j = i - 1;
		auto& l = console_lines[j];
		auto txt = temporary.substr(l.get_range().get_start(),
			                        l.get_range().get_length());
        height -= l.get_height();
        this->pfont->drawText(txt, rectangle.get_min().x(), height - l.get_height(), white);
    }

	renderer.setDepthTestEnabled(true);
	renderer.setScissorTestEnabled(false);
}

void Console::add_output(const std::string& text)
{
	if (text.empty())
	{
		return;
	}
	m_document.append_text(text);
}

Console::Console(const Rectangle2f& rectangle)
    : history(), input(), m_document()
{
	m_document.set_max_length(ConsoleSettings::OutputSettings::Length);
	m_document.set_trim_policy(idlib::document_trim_policy::leading);

    // reset all the console data
    this->on = false;

    // set the console's font
    this->pfont = FontManager::get().loadFont("mp_data/pc8x8.fon", 12);

    // set the console's rectangle
    this->rectangle = rectangle;
}

Console::~Console()
{}

void Console::show()
{
    this->on = true;
    if (on)
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
    if (!on)
    {
        SDL_StopTextInput();
    }
    else
    {
        SDL_StartTextInput();
    }
}

ConsoleHistory& Console::getHistory()
{ return history; }

Console *ConsoleCreateFunctor::operator()(const Rectangle2f& rectangle) const
{ return new Console(rectangle); }

void ConsoleDestroyFunctor::operator()(Console *p) const
{ delete p; }

} } // namespace Ego::Core
