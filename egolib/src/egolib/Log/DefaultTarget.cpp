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

/// @file  egolib/Log/DefaultTarget.cpp
/// @brief Log default target

#include "egolib/Log/DefaultTarget.hpp"

#include "egolib/strutil.h"
#include "egolib/Log/ConsoleColor.hpp"

namespace Log {

static constexpr size_t MAX_LOG_MESSAGE = 1024; ///< Max length of log messages.

DefaultTarget::DefaultTarget(const std::string& filename, Level level)
	: Target(level) {
	_file = vfs_openWrite(filename);
	if (!_file) {
		throw std::runtime_error("unable to open log file `" + filename + "`");
	}
}

DefaultTarget::~DefaultTarget() {
	if (_file) {
		vfs_close(_file);
		_file = nullptr;
	}
}

void DefaultTarget::writev(Level level, const char *format, va_list args) {
	char logBuffer[MAX_LOG_MESSAGE] = EMPTY_CSTR;

	// Add prefix
	const char *prefix;
	switch (level) {
	case Log::Level::Error:
		setConsoleColor(ConsoleColor::Red);
		prefix = "FATAL ERROR: ";
		break;

	case Level::Warning:
		setConsoleColor(ConsoleColor::Yellow);
		prefix = "WARNING: ";
		break;

	case Level::Info:
		setConsoleColor(ConsoleColor::White);
		prefix = "INFO: ";
		break;

	case Level::Debug:
		setConsoleColor(ConsoleColor::Gray);
		prefix = "DEBUG: ";
		break;

	default:
	case Level::Message:
		setConsoleColor(ConsoleColor::White);
		prefix = ""; // no prefix
		break;
	}

	// Build log message
	vsnprintf(logBuffer, MAX_LOG_MESSAGE - 1, format, args);

	if (nullptr != _file)
	{
		// Log to file
		vfs_puts(prefix, _file);
		vfs_puts(logBuffer, _file);
	}

	// Log to console
	fputs(prefix, stdout);
	fputs(logBuffer, stdout);

	// Restore default color
	setConsoleColor(ConsoleColor::Default);
}

} // namespace Log
