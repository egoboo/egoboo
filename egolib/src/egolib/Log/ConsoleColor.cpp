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

#include "egolib/Log/ConsoleColor.hpp"

#ifdef __WINDOWS__
#include <windows.h>
#endif

namespace Log {

/**
 * @remark
 *  Colored console output implementation is platform-specific.
 */
void setConsoleColor(ConsoleColor color) {
	// Windows implementation to set console colour
#ifdef _WIN32
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	switch (color) {
	case ConsoleColor::Red:
		SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
		break;

	case ConsoleColor::Yellow:
		SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		break;

	case ConsoleColor::White:
		SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		break;

	case ConsoleColor::Gray:
	case ConsoleColor::Default:
		SetConsoleTextAttribute(consoleHandle, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
		break;
	}
#endif

	// Linux implementation to set console colour
#if defined(ID_LINUX)
	switch (color) {
	case ConsoleColor::Red:
		fputs("\e[0;31m", stdout);
		break;

	case ConsoleColor::Yellow:
		fputs("\e[1;33m", stdout);
		break;

	case ConsoleColor::White:
		fputs("\e[0;37m", stdout);
		break;

	case ConsoleColor::Gray:
		fputs("\e[0;30m", stdout);
		break;

	case ConsoleColor::Default:
		fputs("\e[0m", stdout);
		break;
	}
	fflush(stdout);
#endif
}

} // namespace Log
