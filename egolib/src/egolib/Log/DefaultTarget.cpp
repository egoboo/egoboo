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
#include <fstream>

namespace Log {

static constexpr size_t MAX_LOG_MESSAGE = 1024; ///< Max length of log messages.

DefaultTarget::DefaultTarget(const std::string& filename, Level level)
	: Target(level),
	  _applicationPath(SDL_GetBasePath()),
	  _file(std::make_unique<std::ofstream>(_applicationPath + filename, std::ios_base::out)) {
	
	 if(!_file->is_open()) {
	 	std::string message = "Unable to create file for logging: " + filename + " (" + strerror(errno) + ")";
	 	error("%s\n", message.c_str());
	 	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning", message.c_str(), nullptr);
	 }
	 else {
	 	info("Log system initialized (logging to \"%s%s\")\n", _applicationPath, filename.c_str());
	 }
}

DefaultTarget::~DefaultTarget() {
	_file->close();
	SDL_free(_applicationPath);
}

void DefaultTarget::writev(Level level, const char *format, va_list args) {
	std::array<char, MAX_LOG_MESSAGE> logBuffer;

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
	vsnprintf(logBuffer.data(), logBuffer.size() - 1, format, args);

	if (_file->is_open())
	{
		// Log to file
		(*_file) << prefix << logBuffer.data();
	}

	// Log to console
	fputs(prefix, stdout);
	fputs(logBuffer.data(), stdout);

	// Restore default color
	setConsoleColor(ConsoleColor::Default);
}

} // namespace Log
