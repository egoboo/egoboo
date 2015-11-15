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

/// @file egolib/log.c
/// @brief Basic logging functionality.
/// @details

#include "egolib/log.h"

#include "egolib/file_common.h"
#include "egolib/strutil.h"
#include "egolib/egoboo_setup.h"
#include "egolib/platform.h"
#include "egolib/vfs.h"

#ifdef __WINDOWS__
#include <windows.h>
#endif

enum class ConsoleColor {
	Red,
	Yellow,
	White,
	Gray,
	Default,
};

/**
 * Setting console colours is not cross-platform, so we have to do it with macros
 */
static void setConsoleColor(ConsoleColor color) {
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

namespace Log {

	Target::Target(Level level) : _level(level) {}

	Target::~Target() {}

	Level Target::getLevel() const {
		return _level;
	}

	void Target::logv(Level level, const char *format, va_list args) {
		if (getLevel() >= level) {
			writev(level, format, args);
		}
	}

	void Target::log(Level level, const char *format, ...) {
		va_list args;
		va_start(args, format);
		writev(level, format, args);
		va_end(args);
	}

	void Target::messagev(const char *format, va_list args) {
		logv(Level::Message, format, args);
	}

	void Target::message(const char *format, ...) {
		va_list args;
		va_start(args, format);
		logv(Level::Message, format, args);
		va_end(args);
	}

	void Target::debugv(const char *format, va_list args) {
		// Only if developer mode is enabled.
		if (!egoboo_config_t::get().debug_developerMode_enable.getValue()) {
			return;
		}
		logv(Level::Debug, format, args);
	}

	void Target::debug(const char *format, ...) {
		va_list args;
		va_start(args, format);
		logv(Level::Debug, format, args);
		va_end(args);
	}

	void Target::infov(const char *format, va_list args) {
		logv(Level::Info, format, args);
	}

	void Target::info(const char *format, ...) {
		va_list args;
		va_start(args, format);
		logv(Level::Info, format, args);
		va_end(args);
	}

	void Target::warnv(const char *format, va_list args) {
		logv(Level::Warning, format, args);
	}

	void Target::warn(const char *format, ...) {
		va_list args;
		va_start(args, format);
		logv(Level::Warning, format, args);
		va_end(args);
	}

	void Target::error(const char *format, ...) {
		va_list args;
		va_start(args, format);
		logv(Level::Error, format, args);
		va_end(args);
	}

}

static constexpr size_t MAX_LOG_MESSAGE = 1024; ///< Max length of log messages.

namespace Log {
	struct DefaultTarget : Target {
		/**
		 * @brief
		 *  The log file.
		 */
		vfs_FILE *_file;
		DefaultTarget(const std::string& filename, Level level = Level::Warning);
		virtual ~DefaultTarget();
		void writev(Level level, const char *format, va_list args) override;
	};

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
}

/**
 * @brief
 *  The single target of this log system.
 */
static std::unique_ptr<Log::Target> g_target = nullptr;
static bool _atexit_registered = false;

void Log::initialize(const std::string& filename, Log::Level level) {
	if (!g_target) {
		g_target = std::make_unique<DefaultTarget>(filename, level);
	}
	if (!_atexit_registered) {
		if (atexit(Log::uninitialize)) {
			g_target = nullptr;
			throw std::runtime_error("unable to initialize logging system");
		}
		_atexit_registered = true;
	}
}

void Log::uninitialize() {
	if (g_target) {
		g_target = nullptr;
	}
}

Log::Target& Log::get() {
	if (!g_target) {
		throw std::logic_error("logging system is not initialized");
	}
	return *g_target;
}
