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

#include "egolib/text/document.hpp"
#include "egolib/Math/Standard.hpp"

namespace Ego {
// Forward declaration.
class Font;
}

namespace Ego { namespace Core {

struct ConsoleSettings {
	/// The length: The maximum number of lines in a console.
	static const int Length = 32;
	struct OutputSettings {
		/// The length: The maximum number of characters in the output.
		static const int Length = 4096;
	};
	struct InputSettings {
		/// The prompt (a symbol to indicate readiness). Default is &gt;.
		static const std::string Prompt;
		/// The length: The maximum number of characters in a input line.
		static const int Length = 1024;
	};
	struct LineSettings {
		/// The length: The maximum number of characters in a line.
		static const int Length = 256;
	};
	struct HistorySettings {
		// The length: the maximum number of lines in a history.
		static const int Length = 32;
	};
};

/// @brief The history of a console.
/// <p>
/// A history is a bounded lifo queue:
/// If an element is added and the history is full, then the oldest element is removed.
/// </p>
/// 
/// <p>
/// The other part of a history is a browser.
/// </p>
struct ConsoleHistory {
    std::deque<std::string> list;
    size_t index;

	/// @brief Construct this history.
	ConsoleHistory();

	/// @brief Get the focused line.
	/// @return the focused line
	/// If the entry index is @a size, the empty string is returned
	const std::string& get_saved();

	/// @post If the string is empty, the history was not observably modified.
	/// Otherwise:
	/// If the history was full, the oldest element was removed.
	/// The new element is the most recent element.
	/// The entry index was set to @a size.
	void add_saved(const std::string& line);

	/// @brief Scroll up the history.
	/// Decrement the entry index if it is not @a 0.
	void up();

	/// @brief Scroll down the history.
	/// Increment the index if it is not @a size.
	void down();
};

struct Console;

struct ConsoleCreateFunctor
{
	Console *operator()(const Rectangle2f& rectangle) const;
};

struct ConsoleDestroyFunctor
{
	void operator()(Console *p) const;
};

/// The encapsulation of the data necessary to run a generic Quake-like console in Egoboo
struct Console : public id::singleton<Console, ConsoleCreateFunctor, ConsoleDestroyFunctor>
{
protected:
	// Befriend with the create functor.
	friend ConsoleCreateFunctor;

	// Befriend with the destroy functor.
	friend ConsoleDestroyFunctor;

public:
    template <size_t CapacityArg>
    struct Buffer
    {
        size_t carat;
        char buffer[CapacityArg + 1];

        /// @brief Construct this buffer.
        Buffer() : carat(0)
        {
            buffer[0] = '\0';
        }

        /// @brief Move the carat left (if possible).
        void moveLeft()
        {
            if (carat > 0)
            {
                carat--;
            }
        }

        /// @brief Move the carat right (if possible).
        void moveRight()
        {
            if (carat < CapacityArg)
            {
                carat++;
            }
        }

        void deleteLeft()
        {
            while (carat > 0)
            {
                carat--;
                char a = buffer[carat];
                if ((a & 0x80) == 0x00 || (a & 0xC0) == 0xC0)
                    break;
            }
            buffer[carat] = '\0';
        }

        /// @brief Set text.
        /// @param text the text
        void setText(const std::string& text)
        {
            strncpy(buffer, text.c_str(), CapacityArg);
            buffer[CapacityArg] = '\0';
            carat = strlen(buffer);
        }

        /// @brief Cleat the input buffer.
        void clear()
        {
            buffer[0] = '\0';
            carat = 0;
        }

        /// @brief Get the capacity of this buffer.
        /// @return the capacity
        static constexpr size_t capacity()
        {
            return CapacityArg;
        }
    };

	Console(const Rectangle2f& rectangle);
	virtual ~Console();

	id::signal<void(std::string)> ExecuteCommand;

    void draw();

    void show();
    void hide();

	ConsoleHistory& getHistory();

	/// @brief Append text to the output.
	/// @param text the text
    void add_output(const std::string& text);

	/// @return
	/// @a nullptr if
	/// - @a event is @a nullptr or
	/// - @a event is not @a nullptr and the event was handled by the console.
	/// @a event in all other cases.
	SDL_Event *handle_event(SDL_Event *event);

private:
	Buffer<ConsoleSettings::LineSettings::Length> input;

	id::document m_document;

	std::shared_ptr<Ego::Font> pfont;

	/// @brief Is the console visible?
	bool on;

	Rectangle2f rectangle;

	ConsoleHistory history;

};

} } // namespace Ego::Core
